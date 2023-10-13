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

#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define AES256_KEY_LEN 32   // 256 bits

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();
int  swCryptExportKey(HCRYPTKEY hRSAKey, int iKeyId, char *szPassword, char *szPublicKeyFilex86, char *szPublicKeyFilex64, char* szPrivateKeyFile);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);
void swGenRegBinValue(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDeriveKey(const char *pszPwd,HCRYPTKEY *phKey);


