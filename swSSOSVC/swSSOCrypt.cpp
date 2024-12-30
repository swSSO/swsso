
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
// Toutes les fonctions crypto utilisées pour la sécurisation du mot de passe 
// maitre et le chiffrement/déchiffrement des mots de passe secondaires
//-----------------------------------------------------------------------------
// 0.65 : => perte compatibilité v0.50 et v0.51
//        => le mot de passe maitre ne traine plus en mémoire (la clé, oui...)
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

const char gcszAlpha[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char gcszNum[]="1234567890";
const char gcszSpecialChars[]="&'(-_)=+}]{[,?;.:/!*$";


//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// swPBKDF2()
// Limitée à 2 blocs de 160 bits en sortie, dont seuls les 256 premiers sont
// fournis à l'appelant. Ce résultat est utilisé pour :
// 1) Alimenter la fonction de génération de clé AES-256 (swCreateAESKeyFromKeyData)
// 2) Stocker le mot de passe maître
// ISSUE#412 : remplacement de l'implémentation maison par la fonction de bcrypt.lib 
// (tant pis pour la compatibilité XP)
//-----------------------------------------------------------------------------
// [out]bufResult    = pointeur vers un buffer de bytes (alloué par l'appelant)
// [in] bufResultLen = taille de clé souhaitée (max SHA256_LEN*2)
// [in] szPwd = mot de passe maitre a deriver
// [in] bufSalt = buffer contenant le sel 
// [in] bufSaltLen = taille du buffer de sel (PBKDF2_SALT_LEN)
// [in] iNbIterations = nombre d'itérations
//-----------------------------------------------------------------------------
int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations,BOOL bSHA256)
{
    TRACE((TRACE_ENTER,_F_,"bufResultLen=%d iNbIterations=%d",bufResultLen,iNbIterations));
	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)bufSalt,bufSaltLen,"sel"));

	int rc=-1;
	BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS status = 0;
    LPCWSTR algName = bSHA256 ? BCRYPT_SHA256_ALGORITHM : BCRYPT_SHA1_ALGORITHM; // SHA-256 ou SHA-1 selon `bSHA256`

    status=BCryptOpenAlgorithmProvider(&hAlg,algName,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG);
	if (!BCRYPT_SUCCESS(status)) { TRACE((TRACE_ERROR,_F_,"BCryptOpenAlgorithmProvider()=%d",status)); goto end; }

    status=BCryptDeriveKeyPBKDF2(
        hAlg,                              // Handle de l'algorithme
        (PUCHAR)szPwd,                     // Clé (mot de passe)
        (ULONG)strlen(szPwd),              // Taille de la clé
        (PUCHAR)bufSalt,                   // Sel
        (ULONG)bufSaltLen,                 // Taille du sel
        (ULONG)iNbIterations,              // Nombre d'itérations
        bufResult,                         // Résultat dérivé
        (ULONG)bufResultLen,               // Taille du buffer pour le résultat
        0                                  // Flags (réservé, doit être 0)
    );
    if (!BCRYPT_SUCCESS(status)) { TRACE((TRACE_ERROR,_F_,"BCryptDeriveKeyPBKDF2()=%d",status)); goto end; }
	rc=0;
end:
	if (hAlg!=NULL) BCryptCloseAlgorithmProvider(hAlg, 0);
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufResult,bufResultLen,"bufResult"));
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));   
    return rc;
}

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
// swCryptDestroyKey()
//-----------------------------------------------------------------------------
// Libération d'une clé
//-----------------------------------------------------------------------------
void swCryptDestroyKey(HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	if (hKey!=NULL) CryptDestroyKey(hKey); 
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptEncodeBase64()
//-----------------------------------------------------------------------------
// Le relecteur attentif verra tout de suite que ce n'est pas un encodage
// base 64... Au moment où j'ai écrit ça, j'ai voulu me simplifier la vie...
// Et maintenant, pour des raisons de compatibilité ascendante, je préfère
// laisser comme ça pour l'instant... Comme d'hab, le provisoire dure ;-)
//-----------------------------------------------------------------------------
// TODO : faire un vrai encodage base64
//-----------------------------------------------------------------------------
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString,int sizeofDestString)
{
	TRACE((TRACE_ENTER,_F_,""));
	int i;
    for (i=0;i<lenSrcData;i++) 
    {
		sprintf_s(pszDestString+2*i,sizeofDestString-2*i,"%02X",pSrcData[i]);
	}
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptDecodeBase64()
//-----------------------------------------------------------------------------
// Cf. remarque dans swCryptEncodeBase64()
//-----------------------------------------------------------------------------
// TODO : faire un vrai decodage base64
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
	lenSrcString=strlen(szSrcString);
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
// Génère deux sels aléatoires pour gbufPBKDF2PwdSalt et gbufPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenPBKDF2Salt(T_SALT *pSalts)
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
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,pSalts->bufPBKDF2PwdSalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2PwdSalt)=0x%08lx",GetLastError())); goto end; }
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,pSalts->bufPBKDF2KeySalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2KeySalt)=0x%08lx",GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG,_F_,pSalts->bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"pSalts->bufPBKDF2PwdSalt"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pSalts->bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"pSalts->bufPBKDF2KeySalt"));

	pSalts->bPBKDF2KeySaltReady=TRUE;
	pSalts->bPBKDF2PwdSaltReady=TRUE;
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
// swCryptEncryptData()
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les données à chiffrer / chiffrées
// [in] lData = taille des données à chiffrer (lData en entrée = lData en sortie)
// [in] hKey = clé de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptEncryptData(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=-1;
	DWORD dwMode=CRYPT_MODE_CBC;

	brc=CryptSetKeyParam(hKey,KP_IV,iv,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKey,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }

	brc=CryptEncrypt(hKey,0,TRUE,0,pData,&lData,lData+16);
	if (!brc) 
	{
		TRACE((TRACE_ERROR,_F_,"CryptEncrypt()=0x%08lx",GetLastError()));
		goto end;
	}
	TRACE_BUFFER((TRACE_DEBUG,_F_,iv,16,"iv"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pData,64,"Donnees chiffrees"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pData+64,16,"ov"));
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptString()
//-----------------------------------------------------------------------------
// Chiffrement d'une chaine de caractère. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pszSource = chaine source (terminée par 0 !)
// [in] hKey = clé de chiffrement
// [rc] chaine chiffrée encodée base64 terminée par 0. A libérer par l'appelant.
//      Format du buffer de sortie (avant encodage base64) :
//      iv 16 octets (alea) + 64 octets (4 blocs de 16 octets) + ov 16 octets
///-----------------------------------------------------------------------------
char *swCryptEncryptString(const char *pszSource,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	char *pszDest=NULL;
	char unsigned *pSourceCopy=NULL;
	int lenSource,lenSourceCopy,lenDest;
	BOOL brc;
	
	lenSource=strlen(pszSource);
	if (lenSource>63) { TRACE((TRACE_ERROR,_F_,"lenSource=%d>63 (taille max chiffrable avec cette fonction=63(utile)+1(0)",lenSource)); goto end; } // 1.12B2-AC-TIE1
	lenSourceCopy=16+64+16; // iv + taille d'un multiple de 16 (128 bits) pour chiffrement AES
	lenDest=lenSourceCopy*2+1;
	//TRACE((TRACE_PWD,_F_,"pszSource=%s",pszSource));
	TRACE((TRACE_DEBUG,_F_,"lenSource=%d lenSourceCopy=%d lenDest=%d",lenSource,lenSourceCopy,lenDest));

	// 0.51 : init avec random sur taille max, puis copie du mot de passe.
	pSourceCopy=(unsigned char*)malloc(lenSourceCopy);
	if (pSourceCopy==NULL) goto end;

	brc = CryptGenRandom(ghProv,lenSourceCopy,pSourceCopy);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()")); goto end; }
	// on laisse les 16 premiers octets en alea (c'est l'iv)
	// on prend le 0 de fin de chaine qui permettra de distingue le mdp du padding
	memcpy(pSourceCopy+16,pszSource,lenSource+1); 
	
	if (swCryptEncryptData(pSourceCopy,pSourceCopy+16,64,hKey)!=0) goto end;

	pszDest=(char*)malloc(lenDest); // sera libéré par l'appelant
	if (pszDest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenDest)); goto end; }
	swCryptEncodeBase64(pSourceCopy,lenSourceCopy,pszDest,lenDest);

	TRACE((TRACE_DEBUG,_F_,"pszDest=%s",pszDest));

end:
	if (pSourceCopy!=NULL) free(pSourceCopy); // free ajouté en 0.93B6
	TRACE((TRACE_LEAVE,_F_,"pszDest=0x%08lx",pszDest));
	return pszDest;
}