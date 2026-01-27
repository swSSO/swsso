//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2026 - Sylvain WERDEFROY
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

const char gcszAlpha[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char gcszNum[]="1234567890";
const char gcszSpecialChars[]="&'(-_)=+}]{[,?;.:/!*$";

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

//-----------------------------------------------------------------------------
// swCreateAESKeyFromProtectedKeyData()
//-----------------------------------------------------------------------------
// Crée la clé AES à partir de la sauvegarde protégée et l'importe dans le ghProv
// L'appelant doit détruire la clé juste après utilisation pour qu'elle reste
// le moins longtemps possible en mémoire
//-----------------------------------------------------------------------------
// [in] iKeyId = ID de la clé
// [in/out] hKey = handle de clé
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
static int swCreateAESKeyFromProtectedKeyData(int iKeyId,HCRYPTKEY *phKey)
{
	TRACE((TRACE_ENTER,_F_,"iKeyId=%d",iKeyId));
	int iAESKeySize=-1;
	KEYBLOB *pAESKey=NULL;
	BOOL brc;
	int rc=-1;

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (!gAESKeyInitialized[iKeyId]) { TRACE((TRACE_ERROR,_F_,"AESKey(%d) not initialized",iKeyId)); goto end; }

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
	
	memcpy(pAESKey->KeyData,gAESProtectedKeyData[iKeyId],AES256_KEY_LEN);
	if (swUnprotectMemory(pAESKey->KeyData,AES256_KEY_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
	
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pAESKey,iAESKeySize,"pAESKey (iAESKeySize=%d)",iAESKeySize));
	brc= CryptImportKey(ghProv,(LPBYTE)pAESKey,iAESKeySize,NULL,0,phKey);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	if (pAESKey!=NULL) 
	{
		if (iAESKeySize!=-1) SecureZeroMemory(pAESKey,iAESKeySize);
		free(pAESKey);
	}
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

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
	else if (dwLastError==NTE_BAD_FLAGS || dwLastError==NTE_TEMPORARY_PROFILE) // ISSUE#362 : profil mandatory, il faut faire avec CRYPT_VERIFYCONTEXT
	{
		brc=CryptAcquireContext(&ghProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_VERIFYCONTEXT);
		if (brc) { rc=0; goto end; }
		dwLastError=GetLastError();
		TRACE((TRACE_ERROR,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV_XP | PROV_RSA_AES | CRYPT_VERIFYCONTEXT)=0x%08lx",dwLastError)); 
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
// swIsPBKDF2KeySaltReady()
//-----------------------------------------------------------------------------
// Retourne TRUE si le sel a déjà été généré et est prêt à être utilisé
//-----------------------------------------------------------------------------
BOOL swIsPBKDF2KeySaltReady(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=gSalts.bPBKDF2KeySaltReady;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swIsPBKDF2PwdSaltReady()
//-----------------------------------------------------------------------------
// Retourne TRUE si le sel a déjà été généré et est prêt à être utilisé
//-----------------------------------------------------------------------------
BOOL swIsPBKDF2PwdSaltReady(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=gSalts.bPBKDF2PwdSaltReady;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

#define DRAIN_BUFFER_SIZE 0x20000 // (128 Ko)
//-----------------------------------------------------------------------------
// swGenPBKDF2Salt()
//-----------------------------------------------------------------------------
// Génère deux sels aléatoires pour gbufPBKDF2PwdSalt et gbufPBKDF2KeySalt
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
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,gSalts.bufPBKDF2PwdSalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2PwdSalt)=0x%08lx",GetLastError())); goto end; }
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2KeySalt)=0x%08lx",GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2PwdSalt"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2KeySalt"));

	gSalts.bPBKDF2KeySaltReady=TRUE;
	gSalts.bPBKDF2PwdSaltReady=TRUE;
	rc=0;
end:
	if (pBuffer128Ko!=NULL) free(pBuffer128Ko);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// swReadPBKDF2Salt()
//-----------------------------------------------------------------------------
// Lit les sels dans le .ini
//-----------------------------------------------------------------------------
int swReadPBKDF2Salt(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	char szPBKDF2Salt[PBKDF2_SALT_LEN*2+1];
	
	// Lecture du sel mot de passe
	GetPrivateProfileString("swSSO","pwdSalt","",szPBKDF2Salt,sizeof(szPBKDF2Salt),gszCfgFile);
	if (strlen(szPBKDF2Salt)!=PBKDF2_SALT_LEN*2) goto end;
	swCryptDecodeBase64(szPBKDF2Salt,(char*)gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	gSalts.bPBKDF2PwdSaltReady=TRUE;
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gbufPBKDF2PwdSalt"));
	// Lecture du sel dérivation de clé
	GetPrivateProfileString("swSSO","keySalt","",szPBKDF2Salt,sizeof(szPBKDF2Salt),gszCfgFile);
	if (strlen(szPBKDF2Salt)!=PBKDF2_SALT_LEN*2) goto end;
	swCryptDecodeBase64(szPBKDF2Salt,(char*)gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	gSalts.bPBKDF2KeySaltReady=TRUE;
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gbufPBKDF2KeySalt"));

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swStoreAESKey()
//-----------------------------------------------------------------------------
// Stocke les données permettant de reconstruire la clé
//-----------------------------------------------------------------------------
// [in] AESKeyData : valeur de la clé
// [in] iKeyId : identifiant de la clé
//-----------------------------------------------------------------------------
int swStoreAESKey(BYTE *AESKeyData,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;
	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }

	// stocke les données permettant de reconstruire la clé 
	gAESKeyInitialized[iKeyId]=TRUE;
	swProtectMemory(AESKeyData,AES256_KEY_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS);
	memcpy_s(gAESProtectedKeyData[iKeyId],AES256_KEY_LEN,AESKeyData,AES256_KEY_LEN);
			
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDeriveKey()
//-----------------------------------------------------------------------------
// Calcul de la clé de chiffrement des mots de passe par dérivation du mot de 
// passe maitre et la stocke dans gAESKeyDataPartN[2][AES256_KEY_LEN/4];
//-----------------------------------------------------------------------------
// [in] cszMasterPwd = mot de passe maitre
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptDeriveKey(const char *pszMasterPwd,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_,"iKeyId=%d",iKeyId));

	int rc=-1;
	BYTE AESKeyData[AES256_KEY_LEN];

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	// Obtient 256 bits (32 octets) auprès de PBKDF2 pour générer une clé AES-256
	if (!swIsPBKDF2KeySaltReady()) { TRACE((TRACE_ERROR,_F_,"swIsPBKDF2SaltReady()=FALSE")); goto end; }
	// ISSUE#412
	if (atoi(gszCfgVersion)<125) // ancienne version, dérive la clé avec 10000 itérations HMAC-SHA1
	{
		// Ancienne dérivation dans ghKey1 > permet de déchiffrer le fichier version 093
		if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszMasterPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000,FALSE)!=0) goto end;
		if (swStoreAESKey(AESKeyData,ghKey1)!=0) goto end;
		// Nouvelle dérivation dans ghKey2 > permet de transchiffrer une fois les données chargées
		if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszMasterPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,600000,TRUE)!=0) goto end;
		// Nouvelle dérivation du mdp > permet de le stocker dans le .ini plus tard après migration
		if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszMasterPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,600000,TRUE)!=0) goto end;
		if (swStoreAESKey(AESKeyData,ghKey2)!=0) goto end;
		if (swPBKDF2(gPBKDF2ConfigPwd,sizeof(gPBKDF2ConfigPwd),pszMasterPwd,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,600000,TRUE)!=0) goto end;
	}
	else // new : dérive la clé avec 600 000 itérations HMAC-SHA256
	{
		if (swPBKDF2(AESKeyData,AES256_KEY_LEN,pszMasterPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,600000,TRUE)!=0) goto end;
		// stocke les données permettant de reconstruire la clé 
		if (swStoreAESKey(AESKeyData,iKeyId)!=0) goto end;
	}

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptData()
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les données à chiffrer / chiffrées
// [in] lData = taille des données à chiffrer (lData en entrée = lData en sortie)
// [in] iKeyId = id de la clé de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptEncryptData(unsigned char *iv, unsigned char *pData,DWORD lData,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_,"iKeyId=%d",iKeyId));
	BOOL brc;
	int rc=-1;
	DWORD dwMode=CRYPT_MODE_CBC;
	HCRYPTKEY hKey=NULL;

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (swCreateAESKeyFromProtectedKeyData(iKeyId,&hKey)!=0) goto end;

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
	if (hKey!=NULL) CryptDestroyKey(hKey);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptString()
//-----------------------------------------------------------------------------
// Chiffrement d'une chaine de caractère. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pszSource = chaine source (terminée par 0 !)
// [in] iKeyId = id de la clé de chiffrement
// [rc] chaine chiffrée encodée base64 terminée par 0. A libérer par l'appelant.
//      Format du buffer de sortie (avant encodage base64) :
//      iv 16 octets (alea) + 64 octets (4 blocs de 16 octets) + ov 16 octets
///-----------------------------------------------------------------------------
char *swCryptEncryptString(const char *pszSource,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_,"iKeyId=%d",iKeyId));
	char *pszDest=NULL;
	char unsigned *pSourceCopy=NULL;
	int lenSource,lenSourceCopy,lenDest;
	BOOL brc;
	
	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	
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
	
	if (swCryptEncryptData(pSourceCopy,pSourceCopy+16,64,iKeyId)!=0) goto end;

	pszDest=(char*)malloc(lenDest); // sera libéré par l'appelant
	if (pszDest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenDest)); goto end; }
	swCryptEncodeBase64(pSourceCopy,lenSourceCopy,pszDest,lenDest);

	TRACE((TRACE_DEBUG,_F_,"pszDest=%s",pszDest));

end:
	if (pSourceCopy!=NULL) free(pSourceCopy); // free ajouté en 0.93B6
	TRACE((TRACE_LEAVE,_F_,"pszDest=0x%08lx",pszDest));
	return pszDest;
}

//-----------------------------------------------------------------------------
// swCryptDecryptDataAES()
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les données à déchiffrer / déchiffrées
// [in] lData = taille des données à déchiffrer (lData en entrée = lData en sortie)
// [in] iKeyId = id de la clé de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptDecryptDataAES256(unsigned char *iv, unsigned char *pData,DWORD lData,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	BOOL brc;
	DWORD dwMode=CRYPT_MODE_CBC;
	HCRYPTKEY hKey=NULL;

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (swCreateAESKeyFromProtectedKeyData(iKeyId,&hKey)!=0) goto end;

	brc=CryptSetKeyParam(hKey,KP_IV,iv,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKey,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }
	
	brc = CryptDecrypt(hKey,0,true,0,pData,&lData);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDecrypt()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	if (hKey!=NULL) CryptDestroyKey(hKey);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDecryptString()
//-----------------------------------------------------------------------------
// Déchiffrement d'une chaine de caractère. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pszSource = chaine source chiffrée encodée en base 64 (terminée par 0 !)
// [in] hKey = clé de chiffrement
// [rc] chaine déchiffrée encodée. A libérer par l'appelant.
//-----------------------------------------------------------------------------
char *swCryptDecryptString(const char *pszSource,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_,"iKeyId=%d",iKeyId));
	char *pszDest=NULL;
	char *pszRet=NULL;
	int lenSource,lenDest;
	int lenVecteur;

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }

	lenSource=strlen(pszSource);
	lenDest=lenSource/2;

	pszDest=(char*)malloc(lenDest+1); // pour le 0 après déchiffrement
	if (pszDest==NULL) goto end;

	if (swCryptDecodeBase64(pszSource,pszDest,lenDest)!=0) goto end;
	
	if (lenSource==LEN_ENCRYPTED_AES256)
	{
		lenVecteur=16;
		if (swCryptDecryptDataAES256((unsigned char*)pszDest,(unsigned char*)pszDest+16,64+16,iKeyId)!=0) goto end; 
		pszDest[lenDest]=0; // 0 de fin de chaine.
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"lenSource=%d (au lieu de %d (AES-256) attendu !)",lenSource,LEN_ENCRYPTED_AES256));
		goto end;
	}

	pszRet=(char*)malloc(lenDest-lenVecteur); // réalloue un buf de 16 de moins et recopie
	if (pszRet==NULL) goto end;
	memcpy(pszRet,pszDest+lenVecteur,lenDest-lenVecteur);

end:
	if (pszDest!=NULL && pszDest!=pszRet) free(pszDest);
	TRACE((TRACE_LEAVE,_F_,"pszRet=0x%08lx",pszRet));
	return pszRet;
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

//-----------------------------------------------------------------------------
// swGenerateRandomPwd()
//-----------------------------------------------------------------------------
// L'appelant a alloué le buffer de longueur suffisante
// Charge à lui de libérer et surtout d'effacer de manière sécurisée
//-----------------------------------------------------------------------------
void swGenerateRandomPwd(char *pszPwd,int iPwdLen,int iPwdType)
{
	TRACE((TRACE_ENTER,_F_,"iPwdLen=%d iPwdType=%d",iPwdLen,iPwdType));
	BOOL brc;
	BYTE *pBuf=NULL;
	char szCharSet[100]="";
	int i;
	int iLenCharSet=0;
	
	pBuf=(BYTE*)malloc(iPwdLen);
	if (pBuf==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iPwdLen)); goto end; }

	brc=CryptGenRandom(ghProv,iPwdLen,pBuf);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2PwdSalt)=0x%08lx",GetLastError())); goto end; }

	if (iPwdType & PWDTYPE_ALPHA)		 strcat_s(szCharSet,sizeof(szCharSet),gcszAlpha); 
	if (iPwdType & PWDTYPE_NUM)			 strcat_s(szCharSet,sizeof(szCharSet),gcszNum); 
	if (iPwdType & PWDTYPE_SPECIALCHARS) strcat_s(szCharSet,sizeof(szCharSet),gcszSpecialChars); 
	
	TRACE((TRACE_DEBUG,_F_,"szCharSet=%s",szCharSet));
	iLenCharSet=strlen(szCharSet);
	for (i=0;i<iPwdLen;i++)
	{
		pszPwd[i]=szCharSet[pBuf[i]%iLenCharSet];
		//TRACE((TRACE_PWD,_F_,"pszPwd[%d]=%c",i,pszPwd[i]));
	}
	pszPwd[iPwdLen]=0;
	
end:
	if (pBuf!=NULL) free(pBuf);
	TRACE((TRACE_LEAVE,_F_,""));
}

// ---------------------------------------------------------------------------- -
// swGenAESKey()
//-----------------------------------------------------------------------------
// Génère deux sels aléatoires pour gbufPBKDF2PwdSalt et gbufPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenAESKey(BYTE *pAESKeyData,DWORD dwAESKeyLen)
{
	TRACE((TRACE_ENTER, _F_, ""));
	int rc = -1;
	BOOL brc;
	BYTE* pBuffer128Ko = NULL;

	// Pour être sûr d'avoir un aléa non attaquable, je suis la préco du §8.1.2 du document 
	// "Cryptanalysis of the Windows Random Number Generator" de Leo Dorrendorf :
	// 1. Request and discard 128 kilobytes of WRNG output.
	// 2. Request as many bytes as needed to generate the secure token.
	// 3. Request and discard 128 kilobytes of WRNG output.
	// Remarque : c'est vraiment du luxe dans notre cas !

	// 1. Request and discard 128 kilobytes of WRNG output.
	pBuffer128Ko = (BYTE*)malloc(DRAIN_BUFFER_SIZE);
	if (pBuffer128Ko == NULL) { TRACE((TRACE_ERROR, _F_, "malloc(%d)", 128 * 1024)); goto end; }
	brc = CryptGenRandom(ghProv, DRAIN_BUFFER_SIZE, pBuffer128Ko);
	if (!brc) { TRACE((TRACE_ERROR, _F_, "CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx", GetLastError())); goto end; }
	TRACE((TRACE_DEBUG, _F_, "CryptGenRandom(DRAIN_BUFFER_SIZE) 1/2"));
	SecureZeroMemory(pBuffer128Ko, DRAIN_BUFFER_SIZE);

	// 2. Request as many bytes as needed to generate the secure token.
	brc = CryptGenRandom(ghProv, dwAESKeyLen, pAESKeyData);
	if (!brc) { TRACE((TRACE_ERROR, _F_, "CryptGenRandom(pAESKeyData)=0x%08lx", GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc = CryptGenRandom(ghProv, DRAIN_BUFFER_SIZE, pBuffer128Ko);
	if (!brc) { TRACE((TRACE_ERROR, _F_, "CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx", GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko, DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG, _F_, "CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG, _F_, pAESKeyData, dwAESKeyLen, "pAESKeyData"));

	rc = 0;
end:
	if (pBuffer128Ko != NULL) free(pBuffer128Ko);
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DPAPIStoreAESKey()
// ----------------------------------------------------------------------------------
// Stockage de KeyData dans le .ini (pour le mode mdp Windows quand Windows ne donne pas le mdp cf. ISSUE#416)
// ----------------------------------------------------------------------------------
int DPAPIStoreAESKey(BYTE* pAESKeyData, DWORD dwAESKeyLen)
{
	TRACE((TRACE_ENTER, _F_, ""));

	BOOL brc;
	int rc = -1;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataSalt;
	char* pszBase64 = NULL;
	char szKey[MAX_COMPUTERNAME_LENGTH + UNLEN + 56 + 1] = "";

	DataIn.pbData = pAESKeyData;
	DataIn.cbData = dwAESKeyLen;
	DataOut.pbData = NULL;
	DataOut.cbData = 0;

	sprintf_s(szKey, sizeof(szKey), "kdDPAPIValue-1-%s@%s", gszUserName, gszComputerName);
	DataSalt.pbData = (BYTE*)szKey;
	DataSalt.cbData = strlen(szKey);

	brc = CryptProtectData(&DataIn, L"swSSO", &DataSalt, NULL, NULL, 0, &DataOut);
	if (!brc)
	{
		TRACE((TRACE_ERROR, _F_, "CryptProtectData()"));
		goto end;
	}
	TRACE_BUFFER((TRACE_DEBUG, _F_, (BYTE*)pAESKeyData, dwAESKeyLen, "pAESKey (iAESKeySize=%d)", dwAESKeyLen));
	// encodage base64 et écriture dans swsso.ini
	pszBase64 = (char*)malloc(DataOut.cbData * 2 + 1);
	if (pszBase64 == NULL) goto end;
	swCryptEncodeBase64(DataOut.pbData, DataOut.cbData, pszBase64, DataOut.cbData * 2 + 1);

	WritePrivateProfileString("swSSO", szKey, pszBase64, gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc = 0;
end:
	if (pszBase64 != NULL) free(pszBase64);
	if (DataOut.pbData != NULL) LocalFree(DataOut.pbData);
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DPAPIGetAESKey()
// ----------------------------------------------------------------------------------
// Lecture de KeyData dans le .ini (pour le mode mdp Windows quand Windows ne donne pas le mdp cf. ISSUE#416)
// ----------------------------------------------------------------------------------
int DPAPIGetAESKey(BYTE* pAESKeyData, DWORD dwAESKeyLen)
{
	TRACE((TRACE_ENTER, _F_, ""));
	int rc = -1;
	char szBase64[1024 + 1];
	char DAPIData[512];
	char szKey[20 + MAX_COMPUTERNAME_LENGTH + UNLEN + 1] = "";

	BOOL brc;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataSalt;

	DataOut.pbData = NULL;
	DataOut.cbData = 0;

	// lecture dans swsso.ini et décodage base64 
	sprintf_s(szKey, sizeof(szKey), "kdDPAPIValue-1-%s@%s", gszUserName, gszComputerName);
	DataSalt.pbData = (BYTE*)szKey;
	DataSalt.cbData = strlen(szKey);
	GetPrivateProfileString("swSSO", szKey, "", szBase64, sizeof(szBase64), gszCfgFile);
	if (*szBase64 == 0) goto end;
	swCryptDecodeBase64(szBase64, DAPIData, sizeof(DAPIData));
	DataIn.pbData = (BYTE*)DAPIData;
	DataIn.cbData = strlen(szBase64) / 2;

	brc = CryptUnprotectData(&DataIn, NULL, &DataSalt, NULL, NULL, 0, &DataOut);
	if (!brc)
	{
		TRACE((TRACE_ERROR, _F_, "CryptUnprotectData()"));
		goto end;
	}
	memcpy_s(pAESKeyData, dwAESKeyLen, DataOut.pbData, DataOut.cbData);
	TRACE_BUFFER((TRACE_DEBUG, _F_, (BYTE*)pAESKeyData, dwAESKeyLen, "pAESKey (iAESKeySize=%d)", dwAESKeyLen));
	rc = 0;
end:
	if (DataOut.pbData != NULL) LocalFree(DataOut.pbData);
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	return rc;
}
