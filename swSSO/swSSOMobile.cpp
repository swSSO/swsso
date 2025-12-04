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
// swSSOMobile.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"

//-----------------------------------------------------------------------------
// SaveJSON()
//-----------------------------------------------------------------------------
// Sauve le coffre de mots de passe en JSON pour export mobile
//-----------------------------------------------------------------------------
int SaveJSON(char *szFullpathname)
{
	TRACE((TRACE_ENTER, _F_, "%s",szFullpathname));
	int rc=-1;
	int i;
	DWORD dw;
	HANDLE hf=INVALID_HANDLE_VALUE;
	char szBuf[1024];
	char szPBKDF2PwdSalt[PBKDF2_SALT_LEN*2+1];
	char szPBKDF2KeySalt[PBKDF2_SALT_LEN*2+1];
	char szCheckSynchroValue[192+1]; // (16+64+16)*2+1 = 193 -- en mode PP_WINDOWS
	char szPBKDF2MasterPwd[PBKDF2_PWD_LEN*2+1]; // -- en mode PP_ENCRYPTED
	char szIdEncryptedValue[LEN_ENCRYPTED_AES256+1];
	char szAppEncryptedName[LEN_ENCRYPTED_AES256+1];

	if (szFullpathname==NULL || *szFullpathname==0) goto end;

	if (giPwdProtection!=PP_ENCRYPTED && giPwdProtection!=PP_WINDOWS)
	{
		TRACE((TRACE_ERROR,_F_,"Export mobile non supporté avec giPwdProtection=%d",giPwdProtection)); goto end;
	}
	// création du fichier json
	hf=CreateFile(szFullpathname,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(CREATE_ALWAYS,%s)",szFullpathname)); goto end; }
	// stockage des sels
	swCryptEncodeBase64(gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,szPBKDF2PwdSalt,sizeof(szPBKDF2PwdSalt));
	swCryptEncodeBase64(gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,szPBKDF2KeySalt,sizeof(szPBKDF2KeySalt));
	sprintf_s(szBuf,sizeof(szBuf),"{\n\"strPwdSalt\":\"%s\",\n\"strKeySalt\":\"%s\",",szPBKDF2PwdSalt,szPBKDF2KeySalt);
	if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
	// stockage du pbkdf2 du mdp maitre (PP_ENCRYPTED) ou de la valeur de vérification (PP_WINDOWS)
	if (giPwdProtection==PP_ENCRYPTED)
	{
		GetPrivateProfileString("swSSO","pwdValue","",szPBKDF2MasterPwd,sizeof(szPBKDF2MasterPwd),gszCfgFile);
		sprintf_s(szBuf,sizeof(szBuf),"\n\"strIniPwdValue\":\"%s\",",szPBKDF2MasterPwd);
	}
	else // PP_WINDOWS
	{
		BOOL brc;
		BYTE bufCheckSynchroValue[16+64+16]; // iv + données utiles + padding
		// Génère un aléa pour l'iv et les données à chiffrer
		brc=CryptGenRandom(ghProv,16+64,bufCheckSynchroValue);
		if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()=%ld",GetLastError())); goto end; }
		// Chiffre  avec la clé ghKey1
		if (swCryptEncryptData(bufCheckSynchroValue,bufCheckSynchroValue+16,64,ghKey1)!=0) goto end;
		// Encode en faux base 64
		swCryptEncodeBase64(bufCheckSynchroValue,16+64+16,szCheckSynchroValue,sizeof(szCheckSynchroValue));
		sprintf_s(szBuf,sizeof(szBuf),"\n\"strCheckSynchroValue\":\"%s\",",szCheckSynchroValue);
	}
	if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
	// stockage de l'entête apps
	sprintf_s(szBuf,sizeof(szBuf),"\n\"apps\":[\n");
	if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
	// stockage des apps
	for (i=0;i<giNbActions;i++)
	{
		// app (chiffré pour ne pas perdre les accents & cie... et puis finalement c'est pas plus mal que ce ne soit pas en clair)
		*szAppEncryptedName=0;
		if (*gptActions[i].szApplication!=0)
		{
			char *pszEncrypted=swCryptEncryptString(gptActions[i].szApplication,ghKey1);
			if (pszEncrypted!=NULL)
			{
				strcpy_s(szAppEncryptedName,sizeof(szAppEncryptedName),pszEncrypted);
				free(pszEncrypted);
			}
		}
		sprintf_s(szBuf,sizeof(szBuf),"{\n\"strApp\":\"%s\",",szAppEncryptedName);
		if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
		// id (chiffré)
		*szIdEncryptedValue=0;
		if (*gptActions[i].szId1Value!=0)
		{
			char *pszEncrypted=swCryptEncryptString(gptActions[i].szId1Value,ghKey1);
			if (pszEncrypted!=NULL)
			{
				strcpy_s(szIdEncryptedValue,sizeof(szIdEncryptedValue),pszEncrypted);
				free(pszEncrypted);
			}
		}
		sprintf_s(szBuf,sizeof(szBuf),"\n\"strId\":\"%s\",",szIdEncryptedValue);
		if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
		// pwd (chiffré)
		sprintf_s(szBuf,sizeof(szBuf),"\n\"strPassword\":\"%s\"\n}",gptActions[i].szPwdEncryptedValue);
		if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
		if (i!=(giNbActions-1))
		{
			strcpy_s(szBuf,sizeof(szBuf),",\n");
			if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }
		}
	}
	// stockage fin 
	sprintf_s(szBuf,sizeof(szBuf),"\n]\n}");
	if (!WriteFile(hf,szBuf,strlen(szBuf),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s),len=%d",szBuf,strlen(szBuf))); goto end; }

	FlushFileBuffers(hf);
	rc = 0;
end:
	if (hf != INVALID_HANDLE_VALUE) CloseHandle(hf);
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	return rc;
}