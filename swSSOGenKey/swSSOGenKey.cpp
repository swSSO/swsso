//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
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
#include "stdafx.h"

void main(int argc,char **argv)
{
	TRACE_OPEN();
	HCRYPTKEY hRSAKey=NULL;
	char szPassword[200+1];
	char szPublicKeyFormatx64[]="swSSO-PublicKey-x64-%04d.reg";
	char szPublicKeyFormatx86[]="swSSO-PublicKey-x86-%04d.reg";
	char szPrivateKeyFormat[]="swSSO-PrivateKey-%04d.txt";
	char szPublicKeyFilex64[_MAX_PATH+1];
	char szPublicKeyFilex86[_MAX_PATH+1];
	char szPrivateKeyFile[_MAX_PATH+1];
	int iKeyId;
	HANDLE hf=INVALID_HANDLE_VALUE;

	puts("\nswSSOGenKey (C) 2009-2017 Sylvain Werdefroy");
	puts("swSSO password recovery -- RSA key generation tool\n");
	if (argc!=2 || atoi(argv[1])<1 || atoi(argv[1])>9999)
	{
		puts("Usage: swssogenkey.exe <keyid (1-9999)>");
		goto end;
	}
	iKeyId=atoi(argv[1]);
	wsprintf(szPublicKeyFilex64,szPublicKeyFormatx64,iKeyId);
	wsprintf(szPublicKeyFilex86,szPublicKeyFormatx86,iKeyId);
	wsprintf(szPrivateKeyFile,szPrivateKeyFormat,iKeyId);
	
	hf=CreateFile(szPrivateKeyFile,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf!=INVALID_HANDLE_VALUE) { printf("Error: file %s already exists\n",szPrivateKeyFile); goto end; }
	CloseHandle(hf);

	hf=CreateFile(szPublicKeyFilex64,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf!=INVALID_HANDLE_VALUE) { printf("Error: file %s already exists\n",szPublicKeyFilex86); goto end; }
	CloseHandle(hf);

	hf=CreateFile(szPublicKeyFilex86,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf!=INVALID_HANDLE_VALUE) { printf("Error: file %s already exists\n",szPublicKeyFilex64); goto end; }
	CloseHandle(hf);

	if (swCryptInit()!=0) goto end;
	printf("RSA 2048 bits key generation (id:%04d). Please wait...\n",iKeyId);
	
	if (!CryptGenKey(ghProv,CALG_RSA_KEYX,0x08000000 | CRYPT_EXPORTABLE,&hRSAKey))
	{
		printf("Error: CryptGenKey()=0x%08lx\n",GetLastError());
		goto end;
	}
	puts("Key generation done.");
	*szPassword=0;
	while (strlen(szPassword)<10)
	{
		puts("Please enter a strong password to protect the key (minimum length:10) : ");
		if (gets_s(szPassword,sizeof(szPassword))==NULL) goto end;
	}

	if (swCryptExportKey(hRSAKey,iKeyId,szPassword,szPublicKeyFilex86,szPublicKeyFilex64,szPrivateKeyFile)!=0) 
	{
		printf("Error: swCryptExportKey()=0x%08lx\n",GetLastError());
		goto end;
	}	
	puts("Key export done.");
	printf("-> Public key file x86: %s\n",szPublicKeyFilex86);
	printf("-> Public key file x64: %s\n",szPublicKeyFilex64);
	printf("-> Private key file:    %s\n",szPrivateKeyFile);
	
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf);
	if (hRSAKey!=NULL) CryptDestroyKey(hRSAKey);

	swCryptTerm();
	TRACE_CLOSE();
}