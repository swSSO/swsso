//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2025 - Sylvain WERDEFROY
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
	char bufRequest[1280];
	int lenRequest=0;
	char szLogonDomainName[DOMAIN_LEN];
	// int lenUserName;
	DWORD lenLogonDomainName;
	char *p=NULL;

	// R�cup�ration des authentifiants en fonction de la m�thode d'authent
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

	// Conversion de cha�nes UNICODE_STRING
	memset(szUserName,0,sizeof(szUserName));
	memset(bufPassword,0,sizeof(bufPassword));
	memset(szLogonDomainName,0,sizeof(szLogonDomainName));
	TRACE((TRACE_DEBUG,_F_,"usLogonDomainName.Length=%d",usLogonDomainName.Length));
	// domaine
	if (usLogonDomainName.Length==0) // ISSUE#346
	{
		TRACE((TRACE_INFO,_F_,"Domaine vide"));
		lenLogonDomainName=0;
	}
	else if (usLogonDomainName.Length==2) // ISSUE#413
	{
		TRACE((TRACE_INFO,_F_,"Domaine longueur 2 (sans doute '.'), remplace par le computername"));
		lenLogonDomainName=sizeof(szLogonDomainName); 
		if (!GetComputerName(szLogonDomainName,&lenLogonDomainName))
		{
			TRACE((TRACE_ERROR,_F_,"GetComputerName(%d), keep usLogonDomainName (length=1)",GetLastError()));
			strcpy_s(szLogonDomainName,sizeof(szLogonDomainName),".");
		}
	}
	else
	{
		ret=WideCharToMultiByte(CP_ACP,0,usLogonDomainName.Buffer,usLogonDomainName.Length/2,szLogonDomainName,sizeof(szLogonDomainName),NULL,NULL);
		if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usLogonDomainName)=%d",GetLastError())); goto end; }
		lenLogonDomainName=(DWORD)strlen(szLogonDomainName);
	}
	TRACE((TRACE_DEBUG,_F_,"szLogonDomainName=%s",szLogonDomainName));
	// utilisateur
	ret=WideCharToMultiByte(CP_ACP,0,usUserName.Buffer,usUserName.Length/2,szUserName,sizeof(szUserName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usUserName)=%d",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"szUserName=%s",szUserName));
	// mot de passe
	TRACE((TRACE_DEBUG,_F_,"usPassword.MaximumLength=%d",usPassword.MaximumLength));
	ret=WideCharToMultiByte(CP_ACP,0,usPassword.Buffer,usPassword.Length/2,bufPassword,sizeof(bufPassword),NULL,NULL);
	SecureZeroMemory(usPassword.Buffer,usPassword.MaximumLength);
	if (ret==0) { TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usPassword)=%d",GetLastError())); goto end; }
	// ISSUE#173
	// Si le mot de passe est vide, on sort (cas du NPLogonNotify/lpPreviousAuthentInfo apr�s changement de mot de passe)
	if (*bufPassword==0) { TRACE((TRACE_INFO,_F_,"MOT DE PASSE VIDE, ON SORT")); rc=1; goto end; }
	// Chiffre le mot de passe
	if (swProtectMemoryInit()!=0) goto end;
	// ISSUE#156 : remplacement de CRYPTPROTECTMEMORY_SAME_LOGON par CRYPTPROTECTMEMORY_CROSS_PROCESS
	//             et � r�ception le service rechiffrera avec CRYPTPROTECTMEMORY_SAME_PROCESS
	if (swProtectMemory(bufPassword,sizeof(bufPassword),CRYPTPROTECTMEMORY_CROSS_PROCESS)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufPassword,PWD_LEN,"bufPassword"));
	// Construit la requ�te � envoyer � swSSOSVC : 
	// Avant     : V02:PUTPASS:domain(256octets)username(256octets)password(256octets)
	// ISSUE#360 : V03:PUTPASS:domain(256octets)username(256octets)UPN(256octets)password(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:PUTPASS:",12);
	memcpy(bufRequest+12,szLogonDomainName,lenLogonDomainName+1);
	p=strchr(szUserName,'@');
	if (p!=NULL && lenLogonDomainName==0) // ISSUE#360 : domaine vide et @ dans le username, c'est un UPN
	{
		memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,szUserName,strlen(szUserName)+1);
	}
	else // sinon c'est un samaccountname
	{
		memcpy(bufRequest+12+DOMAIN_LEN,szUserName,strlen(szUserName)+1);
	}
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2,bufPassword,PWD_LEN);
	lenRequest=12+DOMAIN_LEN+USER_LEN*2+PWD_LEN;

	// Envoie la requ�te
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

	// Ouverture du pipe cr�� par le service swssosvc
	iNbTry=0;
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	while (hPipe==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%d (SVC pas pr�t - essai %d)",GetLastError(),iNbTry));
		if (iNbTry > giServiceTimeOut) // ISSUE#370 : configurable en base de registre, valeur par d�faut 60 secondes
		{
			swLogEvent(EVENTLOG_ERROR_TYPE,MSG_SERVICE_NOT_STARTED,NULL,NULL,NULL);
			goto end;
		}
		Sleep(1000);
		iNbTry++;
		hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	}
	
	// Envoi de la requ�te
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,lenRequest,"Request to write"));
	if (!WriteFile(hPipe,bufRequest,lenRequest,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenRequest,GetLastError()));
		goto end;
	} 

	// Lecture la r�ponse
	if (!ReadFile(hPipe,bufResponse,sizeof(bufResponse),&cbRead,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeof(bufResponse),GetLastError()));
		goto end;
	}  
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,cbRead,"Response"));

	// V�rification de la r�ponse
	// TODO

	rc=0;
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}