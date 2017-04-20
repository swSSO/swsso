//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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

#include "stdafx.h"

//-----------------------------------------------------------------------------
// swBuildAndSendRequest()
//-----------------------------------------------------------------------------
// rc : 0 OK, -1=erreur, 1=mot de passe vide
//-----------------------------------------------------------------------------
int swBuildAndSendRequest(LPCWSTR lpAuthentInfoType,LPVOID lpAuthentInfo)
{
	TRACE((TRACE_ENTER,_F_,""));

	int rc=-1;
	int ret;
	UNICODE_STRING usUserName,usPassword,usLogonDomainName;
	char szUserName[USER_LEN];
	char bufPassword[PWD_LEN];
	char bufRequest[1024];
	int lenRequest=0;
	char szLogonDomainName[DOMAIN_LEN];
	int lenUserName,lenLogonDomainName;
	
	// Récupération des authentifiants en fonction de la méthode d'authent
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	if (wcscmp(lpAuthentInfoType,L"MSV1_0:Interactive")==0)
	{
		usUserName=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->UserName;
		usPassword=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
		usLogonDomainName=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->LogonDomainName;
	}
	else if (wcscmp(lpAuthentInfoType,L"Kerberos:Interactive")==0)
	{
		usUserName=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->UserName;
		usPassword=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
		usLogonDomainName=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->LogonDomainName;
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
		goto end;
	}

	// Conversion de chaînes UNICODE_STRING
	memset(szUserName,0,sizeof(szUserName));
	memset(bufPassword,0,sizeof(bufPassword));
	memset(szLogonDomainName,0,sizeof(szLogonDomainName));
	// domaine
	ret=WideCharToMultiByte(CP_ACP,0,usLogonDomainName.Buffer,usLogonDomainName.Length/2,szLogonDomainName,sizeof(szLogonDomainName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usLogonDomainName)=%d",GetLastError())); goto end; }
	lenLogonDomainName=(int)strlen(szLogonDomainName);
	// utilisateur
	ret=WideCharToMultiByte(CP_ACP,0,usUserName.Buffer,usUserName.Length/2,szUserName,sizeof(szUserName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usUserName)=%d",GetLastError())); goto end; }
	lenUserName=(int)strlen(szUserName);
	// mot de passe
	TRACE((TRACE_DEBUG,_F_,"usPassword.MaximumLength=%d",usPassword.MaximumLength));
	ret=WideCharToMultiByte(CP_ACP,0,usPassword.Buffer,usPassword.Length/2,bufPassword,sizeof(bufPassword),NULL,NULL);
	SecureZeroMemory(usPassword.Buffer,usPassword.MaximumLength);
	if (ret==0) { TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usPassword)=%d",GetLastError())); goto end; }
	// ISSUE#173
	// Si le mot de passe est vide, on sort (cas du NPLogonNotify/lpPreviousAuthentInfo après changement de mot de passe)
	if (*bufPassword==0) { TRACE((TRACE_INFO,_F_,"MOT DE PASSE VIDE, ON SORT")); rc=1; goto end; }
	// Chiffre le mot de passe
	if (swProtectMemoryInit()!=0) goto end;
	// ISSUE#156 : remplacement de CRYPTPROTECTMEMORY_SAME_LOGON par CRYPTPROTECTMEMORY_CROSS_PROCESS
	//             et à réception le service rechiffrera avec CRYPTPROTECTMEMORY_SAME_PROCESS
	if (swProtectMemory(bufPassword,sizeof(bufPassword),CRYPTPROTECTMEMORY_CROSS_PROCESS)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufPassword,PWD_LEN,"bufPassword"));
	// Construit la requête à envoyer à swSSOSVC : V02:PUTPASS:domain(256octets)username(256octets)password(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:PUTPASS:",12);
	memcpy(bufRequest+12,szLogonDomainName,strlen(szLogonDomainName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,szUserName,strlen(szUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,bufPassword,PWD_LEN);
	lenRequest=12+DOMAIN_LEN+USER_LEN+PWD_LEN;

	// Envoie la requête
	rc=swPipeWrite(bufRequest,lenRequest);
	if (rc!=0) goto end; 

	rc=0;
end:
	swProtectMemoryTerm();
	SecureZeroMemory(bufPassword,sizeof(bufPassword));
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swPipeWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swPipeWrite(char *bufRequest,int lenRequest)
{
	HANDLE hPipe=INVALID_HANDLE_VALUE;
	int rc=-1;
	char bufResponse[1024];
    DWORD cbRead,cbWritten;
	int iNbTry;

	TRACE((TRACE_ENTER,_F_,"lenRequest=%d",lenRequest));

	// Ouverture du pipe créé par le service swssosvc
	iNbTry=0;
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	while (hPipe==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%d (SVC pas prêt - essai %d)",GetLastError(),iNbTry));
		if (iNbTry > 30)
		{
			swLogEvent(EVENTLOG_ERROR_TYPE,MSG_SERVICE_NOT_STARTED,NULL,NULL,NULL);
			goto end;
		}
		Sleep(1000);
		iNbTry++;
		hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	}
	
	// Envoi de la requête
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,lenRequest,"Request to write"));
	if (!WriteFile(hPipe,bufRequest,lenRequest,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenRequest,GetLastError()));
		goto end;
	} 

	// Lecture la réponse
	if (!ReadFile(hPipe,bufResponse,sizeof(bufResponse),&cbRead,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeof(bufResponse),GetLastError()));
		goto end;
	}  
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,cbRead,"Response"));

	// Vérification de la réponse
	// TODO

	rc=0;
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}