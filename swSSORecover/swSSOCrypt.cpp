//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//
//                   
//                       sylvain.werdefroy@gmail.com
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
	BYTE KeyData[]; // g�n�re le warning C4200
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
	else if (dwLastError==NTE_KEYSET_NOT_DEF) // provider non disponible, on doit �tre sous XP, on essaie l'autre
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

//-----------------------------------------------------------------------------
// swCryptTerm()
//-----------------------------------------------------------------------------
// Lib�ration du CSP 
//-----------------------------------------------------------------------------
void swCryptTerm()
{
	TRACE((TRACE_ENTER,_F_,""));
	int i;

	if (ghProv!=NULL) { CryptReleaseContext(ghProv,0); ghProv=NULL; }

	if (gbCryptInitCalled) // n�cessaire car si tableau non ZeroM�moryS�, risque de free hasardeux...
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
// Chargement du keystore contenant des cl�s priv�es 
// Les cl�s ne sont pas d�chiffr�es au moment du chargement, elles restent chiffr�es en m�moire
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
	// v�rification de l'entete
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
		// lecture de la cl�, on la garde en (faux) base 64 et chiffr�e
		
		pszPrivateKeyData=buf4096+5+PBKDF2_SALT_LEN*2+1;
		gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData=(char*)malloc(strlen(pszPrivateKeyData)+1);
		if (gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",strlen(pszPrivateKeyData)+1)); goto end; }
		strcpy_s(gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData,strlen(pszPrivateKeyData)+1,pszPrivateKeyData);
		//TRACE((TRACE_DEBUG,_F_,"szPrivateKeyData=%s",pszPrivateKeyData));
		TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData,strlen(gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData),"gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData"));
		
		giNbPrivateKeys++;
	}
	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d giNbPrivateKeys=%d",rc,giNbPrivateKeys));
	return rc;
}

//-----------------------------------------------------------------------------
// swKeystoreSave()
//-----------------------------------------------------------------------------
// Sauvegarde du keystore contenant des cl�s priv�es 
//-----------------------------------------------------------------------------
int swKeystoreSave(char *szKeystoreFile)
{
	TRACE((TRACE_ENTER,_F_, "%s",szKeystoreFile));
	int rc=SWCRYPT_ERROR;
	int i;
	FILE *hf=NULL;

	// ouverture du fichier keystore
	errno=fopen_s(&hf,szKeystoreFile,"w");
	if (errno!=0) 
	{ 
		TRACE((TRACE_ERROR,_F_,"fopen(%s)=%d",szKeystoreFile,errno)); 
		rc=SWCRYPT_FILEWRITE; 
		goto end;	
	}
	// �criture de l'entete
	fputs(gcszKeystoreHeader,hf);
	fputs("\n",hf);
	// lecture de l'entete
	for (i=0;i<giNbPrivateKeys;i++)
	{
		if (fprintf_s(hf,"%04d:%s:%s\n",gtabPrivateKey[i].iKeyId,gtabPrivateKey[i].szSaltData,gtabPrivateKey[i].szPrivateKeyData)==NULL) 
		{ 
			TRACE((TRACE_ERROR,_F_,"Erreur ecriture cle %04d (index:%d)",gtabPrivateKey[i].iKeyId,i)); 
			rc=SWCRYPT_FILEWRITE;
			goto end; 
		}
	}

	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptGetPrivateKeyFromSZData()
//-----------------------------------------------------------------------------
// Obtient une cl� priv�e � partir de sa repr�sentation SZ
//-----------------------------------------------------------------------------
// [in]  szSaltData = sel associ� � la cl�
// [in]  szPrivateKeyData = cl� chiffr�e par un mdp et encod�e base 64 (ne contient pas l'id)
// [in]  szPassword = mot de passe de la cl�
// [out] phPrivateKey = handle cl�
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
	// d�codage (pseudo) base 64
	dwPrivateKeyStringLen=strlen(szPrivateKeyData);
	TRACE((TRACE_DEBUG,_F_,"dwPrivateKeyStringLen=%d",dwPrivateKeyStringLen));
	dwPrivateKeyDataLen=dwPrivateKeyStringLen/2;
	TRACE((TRACE_DEBUG,_F_,"dwPrivateKeyDataLen=%d",dwPrivateKeyDataLen));
	pPrivateKeyData=(char*)malloc(dwPrivateKeyDataLen);
	if (pPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwPrivateKeyDataLen)); goto end; }
	if (swCryptDecodeBase64(szPrivateKeyData,pPrivateKeyData,dwPrivateKeyDataLen)!=0) goto end;
	// calcule la cl� de session prot�geant la cl� priv�e � partir du mot de passe fourni
	if (swCryptDecodeBase64(szSaltData,(char*)Salt,PBKDF2_SALT_LEN)!=0) goto end;
	if (swCryptDeriveKey(Salt,szPassword,&hSessionKey)!=0) goto end;
	// import de la cl� priv�e dans le CSP
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
// swCryptGetSZDataFromPrivateKey()
//-----------------------------------------------------------------------------
// Obtient la cl� au format SZ pour stockage dans le keystore
//-----------------------------------------------------------------------------
// [in] hPrivateKey = handle cl�
// [in] sel
// [in] szPassword = mot de passe � attribuer � la cl�
// [out] ppszPrivateKeyData = cl� chiffr�e par un mdp et encod�e base 64 (hors id)
//-----------------------------------------------------------------------------
int swCryptGetSZDataFromPrivateKey(HCRYPTKEY hPrivateKey,BYTE *pSalt,char *szPassword,char **ppszPrivateKeyData)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=SWCRYPT_ERROR;
	HCRYPTKEY hSessionKey=NULL;
	DWORD dwPrivateKeyStringLen,dwPrivateKeyDataLen;
	BYTE *pPrivateKeyData=NULL;
	BOOL brc;

	//TRACE((TRACE_PWD,_F_,"swPassword=%s",szPassword));
	// exporte la cl� du CSP vers un BLOB chiffr� par mot de passe
	rc=swCryptDeriveKey(pSalt,szPassword,&hSessionKey);
	if (rc!=0) goto end;
	brc=CryptExportKey(hPrivateKey,hSessionKey,PRIVATEKEYBLOB,0,NULL,&dwPrivateKeyDataLen);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptExportKey(0)=0x%08lx",GetLastError())); goto end; }
	pPrivateKeyData=(BYTE*)malloc(dwPrivateKeyDataLen);
	if (pPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwPrivateKeyDataLen)); goto end; }
	brc=CryptExportKey(hPrivateKey,hSessionKey,PRIVATEKEYBLOB,0,pPrivateKeyData,&dwPrivateKeyDataLen);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptExportKey(1)=0x%08lx",GetLastError())); goto end; }
	// encodage base 64
	dwPrivateKeyStringLen=dwPrivateKeyDataLen*2+1;
	*ppszPrivateKeyData=(char*)malloc(dwPrivateKeyStringLen);
	if (*ppszPrivateKeyData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwPrivateKeyStringLen)); goto end; }	
	swCryptEncodeBase64(pPrivateKeyData,dwPrivateKeyDataLen,*ppszPrivateKeyData);

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
// Obtient la cl� priv�e iKeyId
//-----------------------------------------------------------------------------
int swKeystoreGetPrivateKey(int iKeyId,char *szPassword,HCRYPTKEY *phPrivateKey)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%04d",iKeyId));
	int rc=SWCRYPT_ERROR;
	BOOL bFound=FALSE;
	int i;
	//TRACE((TRACE_PWD,_F_,"swPassword=%s",szPassword));
	// cherche l'id de la cl� dans le keystore
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
// Obtient la premi�re cl� priv�e trouv�e
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
// base 64... Au moment o� j'ai �crit �a, j'ai voulu me simplifier la vie...
// Et maintenant, pour des raisons de compatibilit� ascendante, je pr�f�re
// laisser comme �a pour l'instant... Comme d'hab, le provisoire dure ;-)
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

	// si la longueur de la chaine n'est pas paire, on arr�te tout de suite !
	lenSrcString=strlen(szSrcString);
	TRACE((TRACE_DEBUG,_F_,"lenSrcString=%d lenDestData=%d",lenSrcString,lenDestData)); 
	if ((lenSrcString%2)!=0) { TRACE((TRACE_ERROR,_F_,"lenSrcString=%d",lenSrcString)); goto end; }

	while (iPosSrc<lenSrcString)
	{
		// v�rifie qu'on ne d�borde pas de pData
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
// G�n�re le sel pour gPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenPBKDF2Salt(BYTE *pSalt)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	BYTE *pBuffer128Ko=NULL;

	// Pour �tre s�r d'avoir un al�a non attaquable, je suis la pr�co du �8.1.2 du document 
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
// swKeystoreImportPrivateKey()
//-----------------------------------------------------------------------------
// Importe une nouvelle cl� priv�e dans le keystore
//-----------------------------------------------------------------------------
// [in] szPrivateKeyFile : fichier contenant la cl� � importer
// [in] szPrivateKeyPassword : mot de passe prot�geant la cl�
// [in] szKeystorePassword : mot de passe du keystore
//-----------------------------------------------------------------------------
int swKeystoreImportPrivateKey(char *szPrivateKeyFile, char *szPrivateKeyPassword, char *szKeystorePassword)
{
	TRACE((TRACE_ENTER,_F_,"szPrivateKeyFile=%s",szPrivateKeyFile));
	int rc=SWCRYPT_ERROR;
	FILE *hf=NULL;
	char buf5[5];
	int i;
	HCRYPTKEY hPrivateKey=NULL;
	BYTE Salt[PBKDF2_SALT_LEN];

	//TRACE((TRACE_PWD,_F_,"szPrivateKeyPassword=%s",szPrivateKeyPassword));
	//TRACE((TRACE_PWD,_F_,"szKeystorePassword  =%s",szKeystorePassword));
	// ouvre le fichier cl� priv�e
	errno=fopen_s(&hf,szPrivateKeyFile,"r");
	if (errno!=0) { TRACE((TRACE_ERROR,_F_,"fopen(%s)=%d",szPrivateKeyFile,errno)); goto end;	}
	
	// lit la ligne (unique, au format id:sel:cl�)
	if (fgets(buf4096,sizeof(buf4096),hf)==NULL) { TRACE((TRACE_ERROR,_F_,"Erreur de lecture")); goto end;	}
	TRACE((TRACE_DEBUG,_F_,"ligne lue longueur=%d",strlen(buf4096)));
	memcpy(buf5,buf4096,4);
	buf5[4]=0;
	gtabPrivateKey[giNbPrivateKeys].iKeyId=atoi(buf5);
	TRACE((TRACE_INFO,_F_,"PrivateKeyId=%04d",gtabPrivateKey[giNbPrivateKeys].iKeyId));
	
	// v�rifie que cette cl� n'est pas d�j� dans le keystore
	for (i=0;i<giNbPrivateKeys;i++)
	{
		if (gtabPrivateKey[i].iKeyId==gtabPrivateKey[giNbPrivateKeys].iKeyId) 
		{ 
			TRACE((TRACE_ERROR,_F_,"La cl� %04d est d�j� dans le keystore",gtabPrivateKey[i].iKeyId));
			rc=SWCRYPT_KEYEXISTS; 
			goto end; 
		}
	}
	// lit le sel associ� � la cl�
	memcpy(gtabPrivateKey[giNbPrivateKeys].szSaltData,buf4096+5,PBKDF2_SALT_LEN*2);
	gtabPrivateKey[giNbPrivateKeys].szSaltData[5+PBKDF2_SALT_LEN*2]=0;
	TRACE((TRACE_DEBUG,_F_,"szSalt=%s",gtabPrivateKey[giNbPrivateKeys].szSaltData));

	// importe la cl� dans le CSP en la d�chiffrant avec le mot de passe "szPrivateKeyPassword"
	rc=swCryptGetPrivateKeyFromSZData(gtabPrivateKey[giNbPrivateKeys].szSaltData,buf4096+5+PBKDF2_SALT_LEN*2+1,szPrivateKeyPassword,&hPrivateKey);
	if (rc!=0) 
	{
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_IMPORTKEY_BADPWD,buf5,NULL,NULL);
		goto end;
	}
	
	// g�n�re un sel
	if (swGenPBKDF2Salt(Salt)!=0) goto end;
	swCryptEncodeBase64(Salt,PBKDF2_SALT_LEN,gtabPrivateKey[giNbPrivateKeys].szSaltData);

	// exporte la cl� du CSP en la prot�geant avec le mot de passe szKeystorePassword + stockage dans structure m�moire keystore
	rc=swCryptGetSZDataFromPrivateKey(hPrivateKey,Salt,szKeystorePassword,&(gtabPrivateKey[giNbPrivateKeys].szPrivateKeyData));
	if (rc!=0) goto end;
	
	giNbPrivateKeys++;

	swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_IMPORT_KEY,buf5,NULL,NULL);

	rc=0;
end:
	if(hPrivateKey!=NULL) CryptDestroyKey(hPrivateKey);
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d giNbPrivateKeys=%d",rc,giNbPrivateKeys));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptErrorMsg()
//-----------------------------------------------------------------------------
// Affiche un message d'erreur en fonction du code SWCRYPT_*
//-----------------------------------------------------------------------------
void swCryptErrorMsg(HWND w,int ret)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int iIDS;
	switch(ret)
	{
		case SWCRYPT_BADPWD:
			iIDS=IDS_ERROR_BADPWD;
			break;
		case SWCRYPT_KEYEXISTS:
			iIDS=IDS_ERROR_KEYEXISTS;
			break;
		case SWCRYPT_FILENOTFOUND:
			iIDS=IDS_ERROR_FILENOTFOUND;
			break;
		case SWCRYPT_FILEREAD:
			iIDS=IDS_ERROR_FILEREAD;
			break;
		case SWCRYPT_FILEWRITE:
			iIDS=IDS_ERROR_FILEWRITE;
			break;
		case SWCRYPT_ERROR: // pas de break, c'est fait expr�s
		default:
			iIDS=IDS_ERROR_GENERIC;
	}
	MessageBox(w,GetString(iIDS),"swSSO",MB_ICONEXCLAMATION | MB_OK);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// swXORBuff() : XOR de deux buffer de m�me taille
// utilisation des unsigned char pour eviter erreur taille DWORD 32b 64b ?
// non optimis�
//-----------------------------------------------------------------------------
// [in/out]result = buffer � XORer
// [in] key = cl� pour le xor
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
// swPBKDF2() : impl�mentation de PBKDF2 RFC 2898 (http://www.ietf.org/rfc/rfc2898.txt)
// Limit�e � 2 blocs de 160 bits en sortie, dont seuls les 256 premiers sont
// fournis � l'appelant. Ce r�sultat est utilis� pour :
// 1) Alimenter la fonction de d�rivation de cl� AES-256 (CryptDeriveKey)
// 2) Stocker le mot de passe ma�tre
//-----------------------------------------------------------------------------
// Avec 2 blocs, l'algo propos� par la RFC donne :
// DK = T_1 || T_2
// T_1 = F (P, S, c, 1)
// T_2 = F (P, S, c, 2)
// o� :
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
// [out]bufResult    = pointeur vers un buffer de bytes (allou� par l'appelant)
// [in] bufResultLen = taille de cl� souhait�e (max HASH_LENx2)
// [in] szPwd = mot de passe maitre a deriver
// [in] bufSalt = buffer contenant le sel 
// [in] bufSaltLen = taille du buffer de sel (PBKDF2_SALT_LEN)
// [in] iNbIterations = nombre d'it�rations
//-----------------------------------------------------------------------------
int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations)
{
    TRACE((TRACE_ENTER,_F_,"bufResultLen=%d iNbIterations=%d",bufResultLen,iNbIterations));
	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)bufSalt,bufSaltLen,"sel"));
	
	int brc;
	int c; // it�rations
    int rc=-1;
    HCRYPTKEY hKey=NULL;
    HCRYPTHASH hHMAC=NULL;
    HMAC_INFO  hmacinfo;
    KEYBLOB *pKey=NULL;
	int iKeySize;
    DWORD dwLenHash;
    BYTE bufU_c[HASH_LEN]; // stocke successivement U_1, U_2, ..., U_c
	BYTE bufT[HASH_LEN*2]; // stocke le r�sultat final : T_1 � l'index 0 et T_2 � l'index HASH_LEN
	BYTE bufSaltWithBlocIndex[PBKDF2_SALT_LEN+4];
	int iBloc;

	if (bufResultLen>HASH_LEN*2) { TRACE((TRACE_ERROR,_F_,"bufResultLen=%d > valeur autoris�e HASH_LEN*2=%d",bufResultLen,HASH_LEN*2)); goto end; }
	if (bufSaltLen!=PBKDF2_SALT_LEN) { TRACE((TRACE_ERROR,_F_,"bufSaltLen=%d != valeur impos�e PBKDF2_SALT_LEN=%d",bufSaltLen,PBKDF2_SALT_LEN)); goto end; }

    // Cr�ation de la cl� HMAC en mode PLAINTEXTKEYBLOB, � partir du mot de passe
	iKeySize=sizeof(KEYBLOB)+strlen(szPwd);
    pKey=(KEYBLOB*)malloc(iKeySize);
	if (pKey==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iKeySize)); goto end; }
    ZeroMemory(pKey,iKeySize);

	// Cr�ation de la cl� sym�trique pour le HMAC
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

	//TRACE_BUFFER((TRACE_PWD,_F_,(BYTE*)pKey,iKeySize,"pKey (iKeySize=%d)",iKeySize));
    brc= CryptImportKey(ghProv,(LPBYTE)pKey,iKeySize,NULL,CRYPT_IPSEC_HMAC_KEY,&hKey);
    if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }

	// Initialisation du buffer resultat a zero 
	ZeroMemory(bufT,HASH_LEN*2);

	// It�ration pour sortir 2 blocs de 160 bits chacun (T_1 et T_2)
	for (iBloc=1;iBloc<=2;iBloc++) 
	{
		// concat�nation de l'index de bloc au sel
		memcpy(bufSaltWithBlocIndex,bufSalt,PBKDF2_SALT_LEN);
		bufSaltWithBlocIndex[bufSaltLen]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+1]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+2]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+3]=(BYTE)iBloc;
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufSaltWithBlocIndex,bufSaltLen+4,"bufSaltWithBlocIndex"));

	    // Cr�ation du HMAC
		// cf. http://msdn.microsoft.com/en-us/library/aa382379(VS.85).aspx
		
		brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
		// Initialisation des param�tres du HMAC
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
	
		// R�cup�ration du hash
		dwLenHash=sizeof(bufU_c);
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_1)=0x%08lx",GetLastError())); goto end; }
		//TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"bufU_1"));
		
		// Destruction du HMAC
		brc=CryptDestroyHash(hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
		hHMAC=NULL;

		// XOR dans le buffer r�sultat
		swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);

		// Iterations
		for (c=2;c<=iNbIterations;c++) // on d�marre � 1, la 1�re it�ration �tant faite pr�c�demment hors de la boucle
		{
		    // Cr�ation du HMAC
			brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
			// Initialisation des param�tres du HMAC
			ZeroMemory(&hmacinfo,sizeof(hmacinfo));
			hmacinfo.HashAlgid=CALG_SHA1;
			brc=CryptSetHashParam(hHMAC,HP_HMAC_INFO,(BYTE*)&hmacinfo,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetHashParam()=0x%08lx",GetLastError())); goto end; }

			// HMAC du r�sultat de l'it�ration pr�c�dente
			brc=CryptHashData(hHMAC,bufU_c,HASH_LEN,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptHashData(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// Recup du hash
			brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// D�truire le HMAC
			brc=CryptDestroyHash(hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
			hHMAC=NULL;

			// XOR dans le resultat
			swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);
		}
		//TRACE_BUFFER((TRACE_DEBUG,_F_,bufT+(iBloc-1)*HASH_LEN,HASH_LEN,"bufT_%d",iBloc));
	}
	// Les 2 blocs sont aliment�s, on extrait les bufResultLen pour alimenter bufResult
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
// Cr�e la cl� AES � partir de donn�es de la cl� (256 bits / 32 octets)
// et l'importe dans le ghProv
//-----------------------------------------------------------------------------
// [in] cszMasterPwd = mot de passe maitre
// [in/out] hKey = handle de cl�
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

	// Cr�e la cl� AES
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
// D�rive une cl� du mot de passe pour prot�ger la cl� priv�e RSA
//-----------------------------------------------------------------------------
// [in] pszPwd = mot de passe 
// [out] hkey = handle de cl� de session g�n�r�e
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
// D�chiffre les donn�es avec la cl� priv�e KeyId
//-----------------------------------------------------------------------------
// [in] iKeyId : cl� � utiliser
// [in] szKeystorePassword : mot de passe du keystore
// [in] pEncryptedData : bloc de donn�es � chiffrer
// [in] dwEncryptedDataLen : taille des donn�es (attention, doit �tre <245, limite CSP Microsoft)
// [out] ppData : donn�es d�chiffr�es
//-----------------------------------------------------------------------------
int swCryptDecryptDataRSA(int iKeyId,char *szKeystorePassword,BYTE *pEncryptedData,DWORD dwEncryptedDataLen,BYTE **ppData,DWORD *pdwDataLen)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;
	BOOL brc;
	HCRYPTKEY hPrivateKey=NULL;
	
	// r�cup�re la cl� priv�e dans le keystore
	if (swKeystoreGetPrivateKey(iKeyId,szKeystorePassword,&hPrivateKey)!=0) goto end;

	// d�chiffrement des donn�es
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
// Chiffrement de donn�es avec cl� AES. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pData = donn�es source
// [in] dwLenData = taille donn�es source
// [in] hKey = cl� de chiffrement
// [rc] chaine chiffr�e encod�e base64 termin�e par 0. A lib�rer par l'appelant.
//      Format du buffer de sortie (avant encodage base64) :
//      iv 16 octets (alea) + donn�es (multiple de 16, taille du bloc) + padding 16 octets
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
	
	// allocation buffer pour iv+donn�es+padding
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
	pszDest=(char*)malloc(dwLenDest); // sera lib�r� par l'appelant
	if (pszDest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwLenDest)); goto end; }
	swCryptEncodeBase64(pDataToEncrypt,dwLenDataToEncrypt,pszDest);
	TRACE((TRACE_DEBUG,_F_,"pszDest=%s",pszDest));
end:
	if (pDataToEncrypt!=NULL) free(pDataToEncrypt);
	TRACE((TRACE_LEAVE,_F_,"pszDest=0x%08lx",pszDest));
	return pszDest;
}

//-----------------------------------------------------------------------------
// swChangeKeystorePassword() -- ISSUE#120
//-----------------------------------------------------------------------------
// Changement du mot de passe du keystore
// Remarque : la v�rification de la conformit� � la politique de mot de passe
//            n'est pas faite ici, elle doit avoir �t� faite avant l'appel
//-----------------------------------------------------------------------------
// [in] szOldPwd = mot de passe actuel
// [in] szNewPwd = nouveau mot de passe
// [rc] 0 si OK
///-----------------------------------------------------------------------------
int swChangeKeystorePassword(char *szOldPwd,char *szNewPwd)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	int i;
	HCRYPTKEY hPrivateKey=NULL;
	BYTE Salt[PBKDF2_SALT_LEN];

	for (i=0;i<giNbPrivateKeys;i++)
	{
		if (swCryptGetPrivateKeyFromSZData(gtabPrivateKey[i].szSaltData,gtabPrivateKey[i].szPrivateKeyData,szOldPwd,&hPrivateKey)!=0) goto end;
		if (swCryptDecodeBase64(gtabPrivateKey[i].szSaltData,(char*)Salt,PBKDF2_SALT_LEN)!=0) goto end;
		if (swCryptGetSZDataFromPrivateKey(hPrivateKey,Salt,szNewPwd,&gtabPrivateKey[i].szPrivateKeyData)!=0) goto end;
		if(hPrivateKey!=NULL) { CryptDestroyKey(hPrivateKey); hPrivateKey=NULL; }
	}
	rc=swKeystoreSave(gszKeystoreFile);
	if (rc!=0) goto end;
	rc=0;
end:
	if(hPrivateKey!=NULL) { CryptDestroyKey(hPrivateKey); hPrivateKey=NULL; }
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}