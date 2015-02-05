//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2015 - Sylvain WERDEFROY
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

#pragma warning(disable:4200) 
typedef struct
{
    BLOBHEADER header;
    DWORD dwKeySize;
	BYTE KeyData[]; // génère le warning C4200
} KEYBLOB;
#pragma warning(default:4200)

HCRYPTPROV ghProv=NULL;
char gszBuf[8192];
BYTE gPBKDF2KeySalt[PBKDF2_SALT_LEN];

//-----------------------------------------------------------------------------
// swCryptInit()
//-----------------------------------------------------------------------------
// Initialisation de l'environnement crypto 
// Jusqu'à la version 0.92 : MS_ENHANCED_PROV / PROV_RSA_FULL
// A partir de la 0.93 : MS_ENH_RSA_AES_PROV ou MS_ENH_RSA_AES_PROV_XP / PROV_RSA_AES
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptInit()
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=-1;
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
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptTerm()
//-----------------------------------------------------------------------------
// Libération du CSP 
//-----------------------------------------------------------------------------
void swCryptTerm()
{
	TRACE((TRACE_ENTER,_F_,""));
	if (ghProv!=NULL) { CryptReleaseContext(ghProv,0); ghProv=NULL; }
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptExportKey()
//-----------------------------------------------------------------------------
// Export de la clé, format du fichier :
// 0000:32 octets de sel:blob de la clé privée chiffrée
//-----------------------------------------------------------------------------
int swCryptExportKey(HCRYPTKEY hRSAKey, int iKeyId, char *szPassword, char *szPublicKeyFilex86,char *szPublicKeyFilex64, char* szPrivateKeyFile)
{
	int rc=-1;
	BOOL brc;
	unsigned char *pDataPub=NULL;
	DWORD dwDataPubLen=0;
	unsigned char *pDataPriv=NULL;
	DWORD dwDataPrivLen=0;
	char *pszPub=NULL;
	char *pszPriv=NULL;
	DWORD dw;
	HANDLE hf=INVALID_HANDLE_VALUE;
	HCRYPTKEY hSessionKey=NULL;

	// export clé publique vers un PUBLICKEYBLOB
	brc=CryptExportKey(hRSAKey,0,PUBLICKEYBLOB,0,NULL,&dwDataPubLen);
	if (!brc) { printf("Erreur lors de l'exportation de la cle publique (CryptExportKey(0)=0x%08lx)\n",GetLastError()); goto end; }
	pDataPub=(unsigned char*)malloc(dwDataPubLen);
	if (pDataPub==NULL) { printf("Erreur d'allocation memoire (%d)",dwDataPubLen); goto end; }
	brc=CryptExportKey(hRSAKey,0,PUBLICKEYBLOB,0,pDataPub,&dwDataPubLen);
	if (!brc) {	printf("Erreur lors de l'exportation de la cle publique (CryptExportKey(1)=0x%08lx)\n",GetLastError()); goto end; }
	
	// génération du fichier .reg contenant la clé publique à partir du PUBLICKEYBLOB
	pszPub=(char*)malloc(dwDataPubLen*3+1);
	if (pszPub==NULL) { printf("Erreur d'allocation memoire (%d)\n",dwDataPubLen*3+1); goto end; }
	swGenRegBinValue(pDataPub,dwDataPubLen,pszPub);

	// x86
	hf=CreateFile(szPublicKeyFilex86,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { printf("Erreur de creation du fichier %s (%d)\n",szPublicKeyFilex86,GetLastError()); goto end; }
	strcpy_s(gszBuf,sizeof(gszBuf),"Windows Registry Editor Version 5.00\r\n\r\n[HKEY_LOCAL_MACHINE\\SOFTWARE\\swSSO\\EnterpriseOptions]\r\n");
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex86,GetLastError()); goto end; }
	wsprintf(gszBuf,"\"RecoveryKeyId\"=dword:%08lx\r\n\"RecoveryKeyValue\"=hex:",iKeyId);
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex86,GetLastError()); goto end; }
	brc=WriteFile(hf,pszPub,strlen(pszPub),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex86,GetLastError()); goto end; }
	CloseHandle(hf);
	
	// x64
	hf=CreateFile(szPublicKeyFilex64,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { printf("Erreur de creation du fichier %s (%d)\n",szPublicKeyFilex64,GetLastError()); goto end; }
	strcpy_s(gszBuf,sizeof(gszBuf),"Windows Registry Editor Version 5.00\r\n\r\n[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\swSSO\\EnterpriseOptions]\r\n");
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex64,GetLastError()); goto end; }
	wsprintf(gszBuf,"\"RecoveryKeyId\"=dword:%08lx\r\n\"RecoveryKeyValue\"=hex:",iKeyId);
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex64,GetLastError()); goto end; }
	brc=WriteFile(hf,pszPub,strlen(pszPub),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPublicKeyFilex64,GetLastError()); goto end; }
	CloseHandle(hf);

	// export clé privée vers un PRIVATEKEYBLOB, protégée par une clé de session dérivée du mot de passe
	rc=swCryptDeriveKey(szPassword,&hSessionKey);
	if (rc!=0) goto end;

	brc=CryptExportKey(hRSAKey,hSessionKey,PRIVATEKEYBLOB,0,NULL,&dwDataPrivLen);
	if (!brc) {	printf("Erreur lors de l'exportation de la cle privee (CryptExportKey(0)=0x%08lx)\n",GetLastError()); goto end; }
	pDataPriv=(unsigned char*)malloc(dwDataPrivLen);
	if (pDataPriv==NULL) { printf("Erreur d'allocation memoire (%d)\n",dwDataPrivLen); goto end; }
	brc=CryptExportKey(hRSAKey,hSessionKey,PRIVATEKEYBLOB,0,pDataPriv,&dwDataPrivLen);
	if (!brc) {	printf("Erreur lors de l'exportation de la cle privee (CryptExportKey(1)=0x%08lx)\n",GetLastError()); goto end; }
	
	// génération du fichier de clé privée à partir du PRIVATEKEYBLOB
	pszPriv=(char*)malloc(dwDataPrivLen*2+1);
	if (pszPriv==NULL) { printf("Erreur d'allocation memoire (%d)\n",dwDataPrivLen*2+1); goto end; }	
	swCryptEncodeBase64(pDataPriv,dwDataPrivLen,pszPriv);

	hf=CreateFile(szPrivateKeyFile,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { printf("Erreur de creation du fichier %s (%d)\n",szPrivateKeyFile,GetLastError()); goto end; }

	wsprintf(gszBuf,"%04d:",iKeyId);
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPrivateKeyFile,GetLastError()); goto end; }

	swCryptEncodeBase64(gPBKDF2KeySalt,PBKDF2_SALT_LEN,gszBuf);
	brc=WriteFile(hf,gszBuf,strlen(gszBuf),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",gszBuf,GetLastError()); goto end; }

	brc=WriteFile(hf,":",1,&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",":",GetLastError()); goto end; }

	brc=WriteFile(hf,pszPriv,strlen(pszPriv),&dw,NULL);
	if (!brc) { printf("Erreur d'ecriture dans le fichier %s (%d)\n",szPrivateKeyFile,GetLastError()); goto end; }
	
	CloseHandle(hf);

	rc=0;
end:
	if (hSessionKey!=NULL) CryptDestroyKey(hSessionKey);
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf);
	if (pszPub!=NULL) free(pszPub);
	if (pszPriv!=NULL) free(pszPriv);
	if (pDataPriv!=NULL) free(pDataPriv);
	if (pDataPub!=NULL) free(pDataPub);
	return rc;
}

//-----------------------------------------------------------------------------
// swGenRegBinValue()
//-----------------------------------------------------------------------------
// Génération d'une valeur binaire pour fichier .reg
//-----------------------------------------------------------------------------
void swGenRegBinValue(const unsigned char *pSrcData,int lenSrcData,char *pszDestString)
{
	int i;
    for (i=0;i<lenSrcData-1;i++) 
    {
		wsprintf(pszDestString+3*i,"%02X,",pSrcData[i]);
	}
	wsprintf(pszDestString+3*i,"%02X",pSrcData[i]);
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
	int i;
    for (i=0;i<lenSrcData;i++) 
    {
		wsprintf(pszDestString+2*i,"%02X",pSrcData[i]);
	}
}

//-----------------------------------------------------------------------------
// swCryptDecodeBase64()
//-----------------------------------------------------------------------------
// Cf. remarque dans swCryptEncodeBase64()
//-----------------------------------------------------------------------------
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData)
{
	int iPosSrc=0;
	int iPosDest=0;
	int lenSrcString;
	int rc=-1;
	unsigned char uc;

	// si la longueur de la chaine n'est pas paire, on arrête tout de suite !
	lenSrcString=strlen(szSrcString);
	if ((lenSrcString%2)!=0) goto end;

	while (iPosSrc<lenSrcString)
	{
		// vérifie qu'on ne déborde pas de pData
		if (iPosDest>=lenDestData) goto end; 
		// quartet de poids fort
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=(unsigned char)(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=(unsigned char)(szSrcString[iPosSrc]-'0');
		else goto end;
		iPosSrc++;
		uc=uc*16;
		// quartet de poids faible
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=uc+(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=uc+(szSrcString[iPosSrc]-'0');
		else goto end;
		iPosSrc++;
		pDestData[iPosDest]=(char)uc;
		iPosDest++;
	}
	rc=0;
end:
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
	TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));
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
	iKeySize=sizeof(KEYBLOB)+strlen(szPwd);
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
    pKey->dwKeySize=strlen(szPwd);
	memcpy(pKey->KeyData,szPwd,pKey->dwKeySize);

	// test case 1 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=20;
	memset(pKey->KeyData,0x0b,pKey->dwKeySize);*/
	// fin test case 1 RFC 2202 HMAC-SHA1
	// test case 7 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=80;
	memset(pKey->KeyData,0xaa,pKey->dwKeySize);*/
	// fin test case 7 RFC 2202 HMAC-SHA1

	TRACE_BUFFER((TRACE_PWD,_F_,(BYTE*)pKey,iKeySize,"pKey (iKeySize=%d)",iKeySize));
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

#define DRAIN_BUFFER_SIZE 0x20000 // (128 Ko)
//-----------------------------------------------------------------------------
// swGenPBKDF2Salt()
//-----------------------------------------------------------------------------
// Génère le sel pour gPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenPBKDF2Salt(void)
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
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,gPBKDF2KeySalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2KeySalt)=0x%08lx",GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG,_F_,gPBKDF2KeySalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2PwdSalt"));
	
	rc=0;
end:
	if (pBuffer128Ko!=NULL) free(pBuffer128Ko);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
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
int swCryptDeriveKey(const char *pszPwd,HCRYPTKEY *phKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	HCRYPTHASH hHash=NULL;
	BYTE AESKeyData[AES256_KEY_LEN];

	// Obtient 256 bits (32 octets) auprès de PBKDF2 pour générer une clé AES-256
	if (swGenPBKDF2Salt()!=0) goto end;
	if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszPwd,gPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
	if (swCreateAESKeyFromKeyData(AESKeyData,phKey)!=0) goto end;
	
	rc=0;
end:
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}
