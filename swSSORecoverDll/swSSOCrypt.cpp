//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//							 http://www.swsso.fr
//                   
//                             sylvain@swsso.fr
//
//-----------------------------------------------------------------------------
// 
//  This file is part of swSSO.
//  
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
// 
//-----------------------------------------------------------------------------
// swSSOCrypt.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

HCRYPTPROV ghProv=NULL;

#pragma warning(disable:4200) 
typedef struct
{
    BLOBHEADER header;
    DWORD dwKeySize;
	BYTE KeyData[]; // génère le warning C4200
} KEYBLOB;
#pragma warning(default:4200)

const char gcszKeystoreHeader[]="SWSSO-KEYSTORE-VERSION:1";
BOOL gbCryptInitCalled=FALSE;

/*---------------- FORMAT DU FICHIER KEYSTORE -----------------
SWSSO-KEYSTORE-VERSION:1
0001:SEL longueur PBKDF2_SALT_LEN*2:0702000000A4000052FAB3EEB439FCDA...
0002:SEL longueur PBKDF2_SALT_LEN*2:0702000000A4000052FAB3EEB439FCDA...
---------------- FORMAT DU FICHIER KEYSTORE -----------------*/

typedef struct
{
	int iKeyId;								// 1-9999
	char szSaltData[PBKDF2_SALT_LEN*2+1];
	char *szPrivateKeyData;					// 0702000000A4000052FAB3EEB439FCDA...
} SWCRYPTKEY;

SWCRYPTKEY gtabPrivateKey[MAX_NB_PRIVATEKEYS];
int giNbPrivateKeys=0;

char buf4096[4096];

//-----------------------------------------------------------------------------
// swCryptInit()
//-----------------------------------------------------------------------------
// Initialisation de l'environnement crypto (Microsoft Enhanced Cryptographic 
// Provider en version PROV_RSA_FULL)
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptInit()
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=SWCRYPT_ERROR;
	DWORD dwLastError=0;

	brc=CryptAcquireContext(&ghProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_VERIFYCONTEXT);
	if (!brc) 
	{ 
		dwLastError=GetLastError();
		TRACE((TRACE_INFO,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV | PROV_RSA_AES)=0x%08lx",dwLastError)); 
		goto end; 
	}
	 rc=0;
end:
	if (rc==0)
	{
		giNbPrivateKeys=0;
		ZeroMemory(gtabPrivateKey,sizeof(SWCRYPTKEY)*MAX_NB_PRIVATEKEYS);
		gbCryptInitCalled=TRUE;
	}
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

/*
int swCryptInit()
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=SWCRYPT_ERROR;
	DWORD dwLastError=0;

	brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV,PROV_RSA_AES,0);
	if (brc) { rc=0; goto end; }

	dwLastError=GetLastError();
	TRACE((TRACE_INFO,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV | PROV_RSA_AES)=0x%08lx",dwLastError)); 
	if (dwLastError==NTE_BAD_KEYSET)
	{
		brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_NEWKEYSET);
		if (brc) { rc=0; goto end; }
		dwLastError=GetLastError();
		TRACE((TRACE_ERROR,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV | PROV_RSA_AES | CRYPT_NEWKEYSET)=0x%08lx",dwLastError)); 
	}
	else if (dwLastError==NTE_KEYSET_NOT_DEF) // provider non disponible, on doit être sous XP, on essaie l'autre
	{
		brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV_XP,PROV_RSA_AES,0);
		if (brc) { rc=0; goto end; }

		dwLastError=GetLastError();
		TRACE((TRACE_INFO,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV_XP | PROV_RSA_AES)=0x%08lx",dwLastError)); 
		if (dwLastError==NTE_BAD_KEYSET) 
		{
			brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV_XP,PROV_RSA_AES,CRYPT_NEWKEYSET);
			if (brc) { rc=0; goto end; }
			dwLastError=GetLastError();
			TRACE((TRACE_ERROR,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV_XP | PROV_RSA_AES | CRYPT_NEWKEYSET)=0x%08lx",dwLastError)); 
		}
	}
end:
	if (rc==0)
	{
		giNbPrivateKeys=0;
		ZeroMemory(gtabPrivateKey,sizeof(SWCRYPTKEY)*MAX_NB_PRIVATEKEYS);
		gbCryptInitCalled=TRUE;
	}
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}
*/

//-----------------------------------------------------------------------------
// swCryptTerm()
//-----------------------------------------------------------------------------
// Libération du CSP 
//-----------------------------------------------------------------------------
void swCryptTerm()
{
	TRACE((TRACE_ENTER,_F_,""));
	int i;

	if (ghProv!=NULL) 
	{ 
		if (!CryptReleaseContext(ghProv,0))
		{
			TRACE((TRACE_ERROR,_F_,"CryptReleaseContext=%d",GetLastError())); 
		}
		ghProv=NULL; 
	}

	if (gbCryptInitCalled) // nécessaire car si tableau non ZeroMémorySé, risque de free hasardeux...
	{
		for (i=0;i<giNbPrivateKeys;i++)
		{
			if (gtabPrivateKey[i].szPrivateKeyData!=NULL) 
			{
				TRACE((TRACE_DEBUG,_F_,"recoveryInfosKeyId=%04d free(0x%08lx)",gtabPrivateKey[i].iKeyId,gtabPrivateKey[i].szPrivateKeyData));
				free(gtabPrivateKey[i].szPrivateKeyData);
				gtabPrivateKey[i].szPrivateKeyData=NULL;
			}
		}
	}
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swKeystoreLoad()
//-----------------------------------------------------------------------------
// Chargement du keystore contenant des clés privées 
// Les clés ne sont pas déchiffrées au moment du chargement, elles restent chiffrées en mémoire
//-----------------------------------------------------------------------------
int swKeystoreLoad(char *szKeystoreFile)
{
	TRACE((TRACE_ENTER,_F_, "%s",szKeystoreFile));
	int rc=SWCRYPT_ERROR;
	int errno;
	FILE *hf=NULL;
	char buf5[5];
	char *pszPrivateKeyData;

	giNbPrivateKeys=0;
	ZeroMemory(gtabPrivateKey,sizeof(SWCRYPTKEY)*MAX_NB_PRIVATEKEYS);
	
	// ouverture du fichier keystore
	errno=fopen_s(&hf,szKeystoreFile,"r");
	if (errno!=0) 
	{ 
		TRACE((TRACE_ERROR,_F_,"fopen(%s)=%d",szKeystoreFile,errno)); 
		rc=SWCRYPT_FILENOTFOUND; 
		goto end;	
	}
	// lecture de l'entete
	if (fgets(buf4096,sizeof(buf4096),hf)==NULL) 
	{ 
		TRACE((TRACE_ERROR,_F_,"Erreur lecture entete")); 
		rc=SWCRYPT_FILEREAD;
		goto end; 
	}
	buf4096[strlen(buf4096)-1]=0; // supprimer le CRLF
	// vérification de l'entete
	if (strcmp(buf4096,gcszKeystoreHeader)!=0) 
	{ 
		TRACE((TRACE_ERROR,_F_,"Entete incorrecte : %s",buf4096)); 
		rc=SWCRYPT_FILEREAD;
		goto end; 
	}
	TRACE((TRACE_INFO,_F_,buf4096));
	// lecture des lignes
	while (fgets(buf4096,sizeof(buf4096),hf)!=NULL)
	{
		if (strlen(buf4096)<100) { TRACE((TRACE_ERROR,_F_,"LIGNE INATTENDUE, on passe : %s",buf4096));goto end; }
		TRACE((TRACE_DEBUG,_F_,"ligne lue longueur=%d",strlen(buf4096)));
		buf4096[strlen(buf4096)-1]=0; // suppression CRLF
		memcpy(buf5,buf4096,4);
		buf5[4]=0;
		gtabPrivateKey[giNbPrivateKeys].iKeyId=atoi(buf5);
		TRACE((TRACE_INFO,_F_,"recoveryInfosKeyId=%04d index=%d",gtabPrivateKey[giNbPrivateKeys].iKeyId,giNbPrivateKeys));
		// lecture du sel, on le garde en (faux) base 64
		memcpy(gtabPrivateKey[giNbPrivateKeys].szSaltData,buf4096+5,PBKDF2_SALT_LEN*2);
		TRACE((TRACE_DEBUG,_F_,"salt=%s",gtabPrivateKey[giNbPrivateKeys].szSaltData));
		// lecture de la clé, on la garde en (faux) base 64 et chiffrée
		
		pszPrivateKeyData=buf4096+5+PBKDF2_SALT_LEN*2+1;
		gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData=(char*)malloc(strlen(pszPrivateKeyData)+1);
		if (gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",strlen(pszPrivateKeyData)+1)); goto end; }
		strcpy_s(gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData,strlen(pszPrivateKeyData)+1,pszPrivateKeyData);
		//TRACE((TRACE_DEBUG,_F_,"szPrivateKeyData=%s",pszPrivateKeyData));
		TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData,(int)strlen(gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData),"gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData"));
		
		giNbPrivateKeys++;
	}
	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d giNbPrivateKeys=%d",rc,giNbPrivateKeys));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptGetPrivateKeyFromSZData()
//-----------------------------------------------------------------------------
// Obtient une clé privée à partir de sa représentation SZ
//-----------------------------------------------------------------------------
// [in]  szSaltData = sel associé à la clé
// [in]  szPrivateKeyData = clé chiffrée par un mdp et encodée base 64 (ne contient pas l'id)
// [in]  szPassword = mot de passe de la clé
// [out] phPrivateKey = handle clé
//-----------------------------------------------------------------------------
int swCryptGetPrivateKeyFromSZData(char *szSaltData,char *szPrivateKeyData,char *szPassword,HCRYPTKEY *phPrivateKey)
{
	TRACE((TRACE_ENTER,_F_, "szPrivateKeyData=%s",szPrivateKeyData));
	int rc=SWCRYPT_ERROR;
	char *pPrivateKeyData=NULL;
	DWORD dwPrivateKeyStringLen,dwPrivateKeyDataLen;
	HCRYPTKEY hSessionKey=NULL;
	BYTE Salt[PBKDF2_SALT_LEN];

	//TRACE((TRACE_PWD,_F_,"swPassword=%s",szPassword));
	// décodage (pseudo) base 64
	dwPrivateKeyStringLen=(int)strlen(szPrivateKeyData);
	TRACE((TRACE_DEBUG,_F_,"dwPrivateKeyStringLen=%d",dwPrivateKeyStringLen));
	dwPrivateKeyDataLen=dwPrivateKeyStringLen/2;
	TRACE((TRACE_DEBUG,_F_,"dwPrivateKeyDataLen=%d",dwPrivateKeyDataLen));
	pPrivateKeyData=(char*)malloc(dwPrivateKeyDataLen);
	if (pPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwPrivateKeyDataLen)); goto end; }
	if (swCryptDecodeBase64(szPrivateKeyData,pPrivateKeyData,dwPrivateKeyDataLen)!=0) goto end;
	// calcule la clé de session protégeant la clé privée à partir du mot de passe fourni
	if (swCryptDecodeBase64(szSaltData,(char*)Salt,PBKDF2_SALT_LEN)!=0) goto end;
	if (swCryptDeriveKey(Salt,szPassword,&hSessionKey)!=0) goto end;
	// import de la clé privée dans le CSP
	if (!CryptImportKey(ghProv,(unsigned char*)pPrivateKeyData,dwPrivateKeyDataLen,hSessionKey,CRYPT_EXPORTABLE,phPrivateKey))
		{ TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); rc=SWCRYPT_BADPWD; goto end; }
	TRACE((TRACE_INFO,_F_,"*phPrivateKey=0x%08lx",*phPrivateKey));
	rc=0;
end:
	if (hSessionKey!=NULL) CryptDestroyKey(hSessionKey);
	if (pPrivateKeyData!=NULL) free(pPrivateKeyData);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swKeystoreGetPrivateKey()
//-----------------------------------------------------------------------------
// Obtient la clé privée iKeyId
//-----------------------------------------------------------------------------
int swKeystoreGetPrivateKey(int iKeyId,char *szPassword,HCRYPTKEY *phPrivateKey)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%04d",iKeyId));
	int rc=SWCRYPT_ERROR;
	BOOL bFound=FALSE;
	int i;

	//TRACE((TRACE_PWD,_F_,"szPassword=%s",szPassword));
	// cherche l'id de la clé dans le keystore
	for (i=0;i<giNbPrivateKeys;i++)
	{
		if (gtabPrivateKey[i].iKeyId==iKeyId) { bFound=TRUE; break; }
	}
	if (!bFound) { TRACE((TRACE_ERROR,_F_,"Key %04d not found",iKeyId)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"Key %04d found (index %d)",iKeyId,i));
	rc=swCryptGetPrivateKeyFromSZData(gtabPrivateKey[i].szSaltData,gtabPrivateKey[i].szPrivateKeyData,szPassword,phPrivateKey);
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swKeystoreGetFirstPrivateKey()
//-----------------------------------------------------------------------------
// Obtient la première clé privée trouvée
//-----------------------------------------------------------------------------
int swKeystoreGetFirstPrivateKey(char *szPassword,HCRYPTKEY *phPrivateKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=SWCRYPT_ERROR;
	//TRACE((TRACE_PWD,_F_,"swPassword=%s",szPassword));
	rc=swCryptGetPrivateKeyFromSZData(gtabPrivateKey[0].szSaltData,gtabPrivateKey[0].szPrivateKeyData,szPassword,phPrivateKey);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncodeBase64()
//-----------------------------------------------------------------------------
// Le relecteur attentif verra tout de suite que ce n'est pas un encodage
// base 64... Au moment où j'ai écrit ça, j'ai voulu me simplifier la vie...
// Et maintenant, pour des raisons de compatibilité ascendante, je préfère
// laisser comme ça pour l'instant... Comme d'hab, le provisoire dure ;-)
//-----------------------------------------------------------------------------
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString)
{
	TRACE((TRACE_ENTER,_F_,""));
	int i;
    for (i=0;i<lenSrcData;i++) 
    {
		wsprintf(pszDestString+2*i,"%02X",pSrcData[i]);
	}
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptDecodeBase64()
//-----------------------------------------------------------------------------
// Cf. remarque dans swCryptEncodeBase64()
//-----------------------------------------------------------------------------
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData)
{
	TRACE((TRACE_ENTER,_F_,""));
	int iPosSrc=0;
	int iPosDest=0;
	int lenSrcString;
	int rc=-1;
	unsigned char uc;

	// si la longueur de la chaine n'est pas paire, on arrête tout de suite !
	lenSrcString=(int)strlen(szSrcString);
	TRACE((TRACE_DEBUG,_F_,"lenSrcString=%d lenDestData=%d",lenSrcString,lenDestData)); 
	if ((lenSrcString%2)!=0) { TRACE((TRACE_ERROR,_F_,"lenSrcString=%d",lenSrcString)); goto end; }

	while (iPosSrc<lenSrcString)
	{
		// vérifie qu'on ne déborde pas de pData
		if (iPosDest>=lenDestData) goto end; 
		// quartet de poids fort
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=(unsigned char)(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=(unsigned char)(szSrcString[iPosSrc]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",szSrcString[iPosSrc]));goto end; }
		iPosSrc++;
		uc=uc*16;
		// quartet de poids faible
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=uc+(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=uc+(szSrcString[iPosSrc]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",szSrcString[iPosSrc]));goto end; }
		iPosSrc++;
		//TRACE((TRACE_DEBUG,_F_,"%c%c -> 0x%02x",szSrcString[iPosSrc-2],szSrcString[iPosSrc-1],uc));
		pDestData[iPosDest]=(char)uc;
		iPosDest++;
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
    return rc;
}

#define DRAIN_BUFFER_SIZE 0x20000 // (128 Ko)
//-----------------------------------------------------------------------------
// swGenPBKDF2Salt()
//-----------------------------------------------------------------------------
// Génère le sel pour gPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenPBKDF2Salt(BYTE *pSalt)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	BYTE *pBuffer128Ko=NULL;

	// Pour être sûr d'avoir un aléa non attaquable, je suis la préco du §8.1.2 du document 
	// "Cryptanalysis of the Windows Random Number Generator" de Leo Dorrendorf :
	// 1. Request and discard 128 kilobytes of WRNG output.
	// 2. Request as many bytes as needed to generate the secure token.
	// 3. Request and discard 128 kilobytes of WRNG output.
	// Remarque : c'est vraiment du luxe dans notre cas !
	
	// 1. Request and discard 128 kilobytes of WRNG output.
	pBuffer128Ko=(BYTE*)malloc(DRAIN_BUFFER_SIZE);
	if (pBuffer128Ko==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",128*1024)); goto end; }
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 1/2"));
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);

	// 2. Request as many bytes as needed to generate the secure token.
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,pSalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2KeySalt)=0x%08lx",GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG,_F_,pSalt,PBKDF2_SALT_LEN,"pSalt"));
	
	rc=0;
end:
	if (pBuffer128Ko!=NULL) free(pBuffer128Ko);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swXORBuff() : XOR de deux buffer de même taille
// utilisation des unsigned char pour eviter erreur taille DWORD 32b 64b ?
// non optimisé
//-----------------------------------------------------------------------------
// [in/out]result = buffer à XORer
// [in] key = clé pour le xor
// [in] len = longueur du buffer
//-----------------------------------------------------------------------------
static void swXORBuff(BYTE *result, BYTE *key, int len)
{
    int i;
    for(i=0; i<len; i++)
    {
        result[i] ^= key[i];
    }
}

//-----------------------------------------------------------------------------
// swPBKDF2() : implémentation de PBKDF2 RFC 2898 (http://www.ietf.org/rfc/rfc2898.txt)
// Limitée à 2 blocs de 160 bits en sortie, dont seuls les 256 premiers sont
// fournis à l'appelant. Ce résultat est utilisé pour :
// 1) Alimenter la fonction de dérivation de clé AES-256 (CryptDeriveKey)
// 2) Stocker le mot de passe maître
//-----------------------------------------------------------------------------
// Avec 2 blocs, l'algo proposé par la RFC donne :
// DK = T_1 || T_2
// T_1 = F (P, S, c, 1)
// T_2 = F (P, S, c, 2)
// où :
// P = password, an octet string
// S = salt, an octet string
// c = iteration count, a positive integer
// F (P, S, c, i) = U_1 \xor U_2 \xor ... \xor U_c
//   U_1 = PRF (P, S || INT (i))
//   U_2 = PRF (P, U_1)
//   ...
//   U_c = PRF (P, U_{c-1})
//-----------------------------------------------------------------------------
// MERCI A GUILLAUME POUR SA PROPOSITION D'IMPLEMENTATION !
//-----------------------------------------------------------------------------
// [out]bufResult    = pointeur vers un buffer de bytes (alloué par l'appelant)
// [in] bufResultLen = taille de clé souhaitée (max HASH_LENx2)
// [in] szPwd = mot de passe maitre a deriver
// [in] bufSalt = buffer contenant le sel 
// [in] bufSaltLen = taille du buffer de sel (PBKDF2_SALT_LEN)
// [in] iNbIterations = nombre d'itérations
//-----------------------------------------------------------------------------
int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations)
{
    TRACE((TRACE_ENTER,_F_,"bufResultLen=%d iNbIterations=%d",bufResultLen,iNbIterations));
	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)bufSalt,bufSaltLen,"sel"));
	
	int brc;
	int c; // itérations
    int rc=-1;
    HCRYPTKEY hKey=NULL;
    HCRYPTHASH hHMAC=NULL;
    HMAC_INFO  hmacinfo;
    KEYBLOB *pKey=NULL;
	int iKeySize;
    DWORD dwLenHash;
    BYTE bufU_c[HASH_LEN]; // stocke successivement U_1, U_2, ..., U_c
	BYTE bufT[HASH_LEN*2]; // stocke le résultat final : T_1 à l'index 0 et T_2 à l'index HASH_LEN
	BYTE bufSaltWithBlocIndex[PBKDF2_SALT_LEN+4];
	int iBloc;

	if (bufResultLen>HASH_LEN*2) { TRACE((TRACE_ERROR,_F_,"bufResultLen=%d > valeur autorisée HASH_LEN*2=%d",bufResultLen,HASH_LEN*2)); goto end; }
	if (bufSaltLen!=PBKDF2_SALT_LEN) { TRACE((TRACE_ERROR,_F_,"bufSaltLen=%d != valeur imposée PBKDF2_SALT_LEN=%d",bufSaltLen,PBKDF2_SALT_LEN)); goto end; }

    // Création de la clé HMAC en mode PLAINTEXTKEYBLOB, à partir du mot de passe
	iKeySize=(int)sizeof(KEYBLOB)+(int)strlen(szPwd);
    pKey=(KEYBLOB*)malloc(iKeySize);
	if (pKey==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iKeySize)); goto end; }
    ZeroMemory(pKey,iKeySize);

	// Création de la clé symétrique pour le HMAC
	// cf. doc de CryptImportKey() -> http://msdn.microsoft.com/en-us/library/aa380207(v=vs.85).aspx :
	// "When importing a Hash-Based Message Authentication Code (HMAC) key, the caller must identify 
	// the imported key as a PLAINTEXTKEYBLOB type"
	// "The HMAC algorithms do not have their own algorithm identifiers; use CALG_RC2 instead. 
	// CRYPT_IPSEC_HMAC_KEY allows the import of RC2 keys longer than 16 bytes."
	// cf. tests vectors de la RFC 2202 HMAC-SHA1
    pKey->header.bType=PLAINTEXTKEYBLOB;
    pKey->header.bVersion=CUR_BLOB_VERSION;
    pKey->header.reserved=0;
    pKey->header.aiKeyAlg=CALG_RC2;
    pKey->dwKeySize=(int)strlen(szPwd);
	memcpy(pKey->KeyData,szPwd,pKey->dwKeySize);

	// test case 1 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=20;
	memset(pKey->KeyData,0x0b,pKey->dwKeySize);*/
	// fin test case 1 RFC 2202 HMAC-SHA1
	// test case 7 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=80;
	memset(pKey->KeyData,0xaa,pKey->dwKeySize);*/
	// fin test case 7 RFC 2202 HMAC-SHA1

	//TRACE_BUFFER((TRACE_PWD,_F_,(BYTE*)pKey,iKeySize,"pKey (iKeySize=%d)",iKeySize));
    brc= CryptImportKey(ghProv,(LPBYTE)pKey,iKeySize,NULL,CRYPT_IPSEC_HMAC_KEY,&hKey);
    if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }

	// Initialisation du buffer resultat a zero 
	ZeroMemory(bufT,HASH_LEN*2);

	// Itération pour sortir 2 blocs de 160 bits chacun (T_1 et T_2)
	for (iBloc=1;iBloc<=2;iBloc++) 
	{
		// concaténation de l'index de bloc au sel
		memcpy(bufSaltWithBlocIndex,bufSalt,PBKDF2_SALT_LEN);
		bufSaltWithBlocIndex[bufSaltLen]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+1]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+2]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+3]=(BYTE)iBloc;
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufSaltWithBlocIndex,bufSaltLen+4,"bufSaltWithBlocIndex"));

	    // Création du HMAC
		// cf. http://msdn.microsoft.com/en-us/library/aa382379(VS.85).aspx
		
		brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
		// Initialisation des paramètres du HMAC
		ZeroMemory(&hmacinfo,sizeof(hmacinfo));
		hmacinfo.HashAlgid=CALG_SHA1;
		brc=CryptSetHashParam(hHMAC,HP_HMAC_INFO, (BYTE*)&hmacinfo,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetHashParam()=0x%08lx",GetLastError())); goto end; }

		// test case 1 RFC 2202 HMAC-SHA1
		/*brc=CryptHashData(hHMAC,(const BYTE*)"Hi There",8, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam()=0x%08lx",GetLastError())); goto end; }
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"Test case 1 RFC 2202 HMAC-SHA1"));
		if (brc) goto end;*/
		// fin test case 1 RFC 2202 HMAC-SHA1
		// test case 7 RFC 2202 HMAC-SHA1
		/*brc=CryptHashData(hHMAC,(const BYTE*)"Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data",73, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam()=0x%08lx",GetLastError())); goto end; }
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"Test case 1 RFC 2202 HMAC-SHA1"));
		if (brc) goto end;*/
		// fin test case 7 RFC 2202 HMAC-SHA1

		// HMAC du sel enrichi de l'id de bloc
		brc=CryptHashData(hHMAC,bufSaltWithBlocIndex,bufSaltLen+4, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptHashData(bufSaltWithBlocIndex)=0x%08lx",GetLastError())); goto end; }
	
		// Récupération du hash
		dwLenHash=sizeof(bufU_c);
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_1)=0x%08lx",GetLastError())); goto end; }
		//TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"bufU_1"));
		
		// Destruction du HMAC
		brc=CryptDestroyHash(hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
		hHMAC=NULL;

		// XOR dans le buffer résultat
		swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);

		// Iterations
		for (c=2;c<=iNbIterations;c++) // on démarre à 1, la 1ère itération étant faite précédemment hors de la boucle
		{
		    // Création du HMAC
			brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
			// Initialisation des paramètres du HMAC
			ZeroMemory(&hmacinfo,sizeof(hmacinfo));
			hmacinfo.HashAlgid=CALG_SHA1;
			brc=CryptSetHashParam(hHMAC,HP_HMAC_INFO,(BYTE*)&hmacinfo,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetHashParam()=0x%08lx",GetLastError())); goto end; }

			// HMAC du résultat de l'itération précédente
			brc=CryptHashData(hHMAC,bufU_c,HASH_LEN,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptHashData(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// Recup du hash
			brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// Détruire le HMAC
			brc=CryptDestroyHash(hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
			hHMAC=NULL;

			// XOR dans le resultat
			swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);
		}
		//TRACE_BUFFER((TRACE_DEBUG,_F_,bufT+(iBloc-1)*HASH_LEN,HASH_LEN,"bufT_%d",iBloc));
	}
	// Les 2 blocs sont alimentés, on extrait les bufResultLen pour alimenter bufResult
	memcpy(bufResult,bufT,bufResultLen);
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufResult,bufResultLen,"bufResult"));
	rc=0;
end:
	if (hHMAC!=NULL) CryptDestroyHash(hHMAC);
    if (hKey!=NULL) CryptDestroyKey(hKey);
    if (pKey!=NULL) free(pKey);
	
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));   
    return rc;
}

//-----------------------------------------------------------------------------
// swCreateAESKeyFromKeyData()
//-----------------------------------------------------------------------------
// Crée la clé AES à partir de données de la clé (256 bits / 32 octets)
// et l'importe dans le ghProv
//-----------------------------------------------------------------------------
// [in] cszMasterPwd = mot de passe maitre
// [in/out] hKey = handle de clé
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey)
{
	int iAESKeySize;
	KEYBLOB *pAESKey=NULL;
	BOOL brc;
	int rc=-1;
	TRACE((TRACE_ENTER,_F_,""));
	BYTE ZeroBuf[AES256_KEY_LEN];

	// 
	ZeroMemory(ZeroBuf,AES256_KEY_LEN);
	if (memcmp(pAESKeyData,ZeroBuf,AES256_KEY_LEN)==0) 
	{
		TRACE((TRACE_ERROR,_F_,"pAESKeyData is zerobuf")); 
		goto end;
	}

	// Crée la clé AES
	iAESKeySize=sizeof(KEYBLOB)+AES256_KEY_LEN;
	pAESKey=(KEYBLOB*)malloc(iAESKeySize);
	if (pAESKey==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iAESKeySize)); goto end; }
	ZeroMemory(pAESKey,iAESKeySize);
	pAESKey->header.bType=PLAINTEXTKEYBLOB;
	pAESKey->header.bVersion=CUR_BLOB_VERSION;
	pAESKey->header.reserved=0;
	pAESKey->header.aiKeyAlg=CALG_AES_256;
	pAESKey->dwKeySize=AES256_KEY_LEN;
	memcpy(pAESKey->KeyData,pAESKeyData,AES256_KEY_LEN);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pAESKey,iAESKeySize,"pAESKey (iAESKeySize=%d)",iAESKeySize));
	brc= CryptImportKey(ghProv,(LPBYTE)pAESKey,iAESKeySize,NULL,0,phKey);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	if (pAESKey!=NULL) free(pAESKey);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDeriveKey()
//-----------------------------------------------------------------------------
// Dérive une clé du mot de passe pour protéger la clé privée RSA
//-----------------------------------------------------------------------------
// [in] pszPwd = mot de passe 
// [out] hkey = handle de clé de session générée
//-----------------------------------------------------------------------------
int swCryptDeriveKey(BYTE *pSalt,const char *pszPwd,HCRYPTKEY *phKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	HCRYPTHASH hHash=NULL;
	BYTE AESKeyData[AES256_KEY_LEN];

	if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszPwd,pSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
	if (swCreateAESKeyFromKeyData(AESKeyData,phKey)!=0) goto end;
	
	rc=0;
end:
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDecryptDataRSA()
//-----------------------------------------------------------------------------
// Déchiffre les données avec la clé privée KeyId
//-----------------------------------------------------------------------------
// [in] iKeyId : clé à utiliser
// [in] szKeystorePassword : mot de passe du keystore
// [in] pEncryptedData : bloc de données à chiffrer
// [in] dwEncryptedDataLen : taille des données (attention, doit être <245, limite CSP Microsoft)
// [out] ppData : données déchiffrées
//-----------------------------------------------------------------------------
int swCryptDecryptDataRSA(int iKeyId,char *szKeystorePassword,BYTE *pEncryptedData,DWORD dwEncryptedDataLen,BYTE **ppData,DWORD *pdwDataLen)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;
	BOOL brc;
	HCRYPTKEY hPrivateKey=NULL;
	
	// récupère la clé privée dans le keystore
	if (swKeystoreGetPrivateKey(iKeyId,szKeystorePassword,&hPrivateKey)!=0) goto end;

	// déchiffrement des données
	*pdwDataLen=dwEncryptedDataLen;
	brc=CryptDecrypt(hPrivateKey,0,TRUE,0,pEncryptedData,pdwDataLen);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDecrypt=0x%08lx",GetLastError())); goto end; }

	*ppData=(BYTE*)malloc(*pdwDataLen);
	if (*ppData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",*pdwDataLen)); goto end; }
	memcpy(*ppData,pEncryptedData,*pdwDataLen);

	rc=0;
end:
	if (rc!=0 && *ppData!=NULL) free(*ppData);
	if (hPrivateKey!=NULL) CryptDestroyKey(hPrivateKey);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptAES()
//-----------------------------------------------------------------------------
// Chiffrement de données avec clé AES. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pData = données source
// [in] dwLenData = taille données source
// [in] hKey = clé de chiffrement
// [rc] chaine chiffrée encodée base64 terminée par 0. A libérer par l'appelant.
//      Format du buffer de sortie (avant encodage base64) :
//      iv 16 octets (alea) + données (multiple de 16, taille du bloc) + padding 16 octets
///-----------------------------------------------------------------------------
char *swCryptEncryptAES(BYTE *pData,DWORD dwLenData,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	char *pszDest=NULL;
	BYTE *pDataToEncrypt=NULL;
	DWORD dwLenDataToEncrypt;
	DWORD dwLenDest;
	BOOL brc;
	DWORD dwMode=CRYPT_MODE_CBC;
	
	// allocation buffer pour iv+données+padding
	dwLenDataToEncrypt=16+dwLenData+16;
	pDataToEncrypt=(BYTE*)malloc(dwLenDataToEncrypt);
	if (pDataToEncrypt==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%ld)",dwLenDataToEncrypt)); goto end; }
	ZeroMemory(pDataToEncrypt,dwLenDataToEncrypt);
	// iv
	brc = CryptGenRandom(ghProv,16,pDataToEncrypt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()")); goto end; }
	// data
	memcpy(pDataToEncrypt+16,pData,dwLenData);
	// chiffrement
	brc=CryptSetKeyParam(hKey,KP_IV,pDataToEncrypt,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKey,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }
	brc=CryptEncrypt(hKey,0,TRUE,0,pDataToEncrypt+16,&dwLenData,dwLenData+16);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptEncrypt()=0x%08lx",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDataToEncrypt,16,"iv"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDataToEncrypt+16,dwLenData,"Donnees chiffrees"));
	// allocation de la chaine retour et encodage faux base 64
	dwLenDest=dwLenDataToEncrypt*2+1;
	pszDest=(char*)malloc(dwLenDest); // sera libéré par l'appelant
	if (pszDest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwLenDest)); goto end; }
	swCryptEncodeBase64(pDataToEncrypt,dwLenDataToEncrypt,pszDest);
	TRACE((TRACE_DEBUG,_F_,"pszDest=%s",pszDest));
end:
	if (pDataToEncrypt!=NULL) free(pDataToEncrypt);
	TRACE((TRACE_LEAVE,_F_,"pszDest=0x%08lx",pszDest));
	return pszDest;
}
