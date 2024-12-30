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
// swSSOCrypt.h
//-----------------------------------------------------------------------------

#define SALT_LEN 128		// longueur du sel appliqué au mdp maitre avant hash (versions < 0.93)
#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define PBKDF2_PWD_LEN 32	// 256 bits
#define AES256_KEY_LEN 32   // 256 bits

#define PWDTYPE_ALPHA			1
#define PWDTYPE_NUM				2
#define PWDTYPE_SPECIALCHARS	4

typedef struct 
{
	BOOL bPBKDF2PwdSaltReady;
	BYTE bufPBKDF2PwdSalt[PBKDF2_SALT_LEN]; // 0.93B6 ISSUE#56 : sel pour le stockage du mot de passe
	BOOL bPBKDF2KeySaltReady;
	BYTE bufPBKDF2KeySalt[PBKDF2_SALT_LEN]; // 0.93B6 ISSUE#56 : sel pour la dérivation de la clé de chiffrement des mots de passe secondaires
} T_SALT;

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();
void swCryptDestroyKey(HCRYPTKEY hKey);
int  swCryptSaltAndHashPassword(char *bufSalt, const char *szPwd,char **pszHashedPwd, int iNbIterations,bool bV72);
void swCryptEncodeBase64(const unsigned char* pSrcData, int lenSrcData, char* pszDestString, int sizeofDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);

int swGenPBKDF2Salt(T_SALT *pSalts);

int  swCryptEncryptData(unsigned char *iv,unsigned char *pData,DWORD lData,HCRYPTKEY hKey);
int  swCryptDecryptDataAES256(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey);

int swCryptDeriveKey(const char *pszMasterPwd,HCRYPTKEY *phKey);
char *swCryptEncryptString(const char *pszSource,HCRYPTKEY hKey);
char *swCryptDecryptString(const char *pszSource,HCRYPTKEY hKey);

int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations,BOOL bSHA256);

void swGenerateRandomPwd(char *pszPwd,int iPwdLen,int iPwdType);
int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey);
char *swCryptEncryptString(const char *pszSource,HCRYPTKEY hKey);