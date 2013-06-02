//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2013 - Sylvain WERDEFROY
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

#pragma warning(disable:4200) 
typedef struct
{
    BLOBHEADER header;
    DWORD dwKeySize;
	BYTE KeyData[]; // génère le warning C4200
} KEYBLOB;
#pragma warning(default:4200)

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();
void swCryptDestroyKey(HCRYPTKEY hKey);
int  swCryptSaltAndHashPassword(char *bufSalt, const char *szPwd,char **pszHashedPwd, int iNbIterations,bool bV72);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);

BOOL swIsPBKDF2KeySaltReady(void);
BOOL swIsPBKDF2PwdSaltReady(void);
int swGenPBKDF2Salt(void);

int  swCryptEncryptData(unsigned char *iv,unsigned char *pData,DWORD lData,HCRYPTKEY hKey);
int  swCryptDecryptDataAES256(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey);
int  swCryptDecryptData3DES(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey);

int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey);
int swCryptDeriveKey(const char *pszMasterPwd,HCRYPTKEY *phKey,BYTE *pAESKeyData);
char *swCryptEncryptString(const char *pszSource,HCRYPTKEY hKey);
char *swCryptDecryptString(const char *pszSource,HCRYPTKEY hKey);

int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations);

void swGenerateRandomPwd(char *pszPwd,int iPwdLen,int iPwdType);

