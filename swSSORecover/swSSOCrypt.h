//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2014 - Sylvain WERDEFROY
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

#define HASH_LEN 20   // SHA1=160 bits
#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define AES256_KEY_LEN 32   // 256 bits

#define MAX_NB_PRIVATEKEYS 100

#define SWCRYPT_ERROR			1 // erreur générique
#define SWCRYPT_BADPWD			2 // mot de passe de la clé à importer incorrect
#define SWCRYPT_KEYEXISTS		3 // cette clé est déjà dans le keystore
#define SWCRYPT_FILENOTFOUND	4 
#define SWCRYPT_FILEREAD		5 
#define SWCRYPT_FILEWRITE		6 

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();

int  swCryptDeriveKey(BYTE *pSalt,const char *pszMasterPwd,HCRYPTKEY *phKey);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);

int swCryptGetPrivateKeyFromSZData(char *szSaltData,char *szPrivateKeyData,char *szPassword,HCRYPTKEY *phPrivateKey);
int swCryptGetSZDataFromPrivateKey(HCRYPTKEY hPrivateKey,BYTE *pSalt,char *szPassword,char **ppszPrivateKeyData);

int swKeystoreLoad(char *szKeystoreFile);
int swKeystoreSave(char *szKeystoreFile);

int swKeystoreImportPrivateKey(char *szPrivateKeyFile, char *szPrivateKeyPassword, char *szKeystorePassword);
int swKeystoreGetPrivateKey(int iKeyId,char *szPassword,HCRYPTKEY *phPrivateKey);
int swKeystoreGetFirstPrivateKey(char *szPassword,HCRYPTKEY *phPrivateKey);

void swCryptErrorMsg(HWND w,int ret);

int swCryptDecryptDataRSA(int iKeyId,char *szKeystorePassword,BYTE *pEncryptedData,DWORD dwEncryptedDataLen,BYTE **ppData,DWORD *pdwDataLen);
char *swCryptEncryptAES(BYTE *pData,DWORD dwLenData,HCRYPTKEY hKey);

int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey);
