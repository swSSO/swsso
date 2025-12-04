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
// swSSOLogs.cpp
//-----------------------------------------------------------------------------
// Gestions des logs
//-----------------------------------------------------------------------------

#include "stdafx.h"

//-----------------------------------------------------------------------------
// swLogEvent() 
//-----------------------------------------------------------------------------
// Ecrit un log dans fichier et/ou journal d'événement Windows et/ou serveur 
// syslog en fonction de la config EnterpriseOptions en base de registre
// Remarque : syslog = implémentation ultérieure !
//-----------------------------------------------------------------------------
int swLogEvent(WORD wType,DWORD dwMsg,char *pszParam1,char *pszParam2,char *pszParam3)
{
	TRACE((TRACE_ENTER,_F_,"wType=0x%04x dwMsg=0x%08lx",wType,dwMsg));
	int rc=-1;
	HANDLE hEventLog=NULL;
	char *pInsertStrings[3] = {NULL,NULL,NULL};
	WORD wNumStrings=0;
	HANDLE hfLogs=INVALID_HANDLE_VALUE;
	char buf2048[2048];
	char szHeader[512]; 
	char szComputerName[100 + 1]="";
	char szUserName[100 + 1]="";
	DWORD lenComputerName;
	DWORD lenUserName;

	DWORD dwMsgLen;
	DWORD dw;
	DWORD len;
	SYSTEMTIME horodate;
	
	DWORD cbRDN,cbSid;
	SID_NAME_USE eUse;
	SID *pSid=NULL;
	char *pszRDN=NULL;

	if ((giLogLevel==LOG_LEVEL_NONE) || (*gszLogFileName==0 && !gbWindowsEventLog))
	{
		TRACE((TRACE_DEBUG,_F_,"Pas de log demande, on sort !"));
		goto end; 
	}
	
	switch (giLogLevel)
	{
		case LOG_LEVEL_ERROR: 
			if (wType!=EVENTLOG_ERROR_TYPE) { TRACE((TRACE_DEBUG,_F_,"giLogLevel=%d wType=%d",giLogLevel,wType)); goto end; }
			break;
		case LOG_LEVEL_WARNING: 
			if (wType!=EVENTLOG_ERROR_TYPE && wType!=EVENTLOG_WARNING_TYPE) { TRACE((TRACE_DEBUG,_F_,"giLogLevel=%d wType=%d",giLogLevel,wType)); goto end; }
			break;
	}

	if (pszParam1!=NULL) { pInsertStrings[0]=pszParam1; wNumStrings++; }
	if (pszParam2!=NULL) { pInsertStrings[1]=pszParam2; wNumStrings++; }
	if (pszParam3!=NULL) { pInsertStrings[2]=pszParam3; wNumStrings++; }

	lenUserName=sizeof(szUserName); GetUserName(szUserName,&lenUserName);

	if (*gszLogFileName!=0) // log dans fichier
	{
		// ouverture du fichier
		hfLogs=CreateFile(gszLogFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hfLogs==INVALID_HANDLE_VALUE)
		{
			TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",gszLogFileName,GetLastError()));
			goto end;
		}
		if (SetFilePointer(hfLogs,0,0,FILE_END)==INVALID_SET_FILE_POINTER)
		{
			TRACE((TRACE_ERROR,_F_,"SetFilePointer(%s,END)=%d",gszLogFileName,GetLastError()));
			goto end;
		}
		// écriture début de ligne = horodate + computername + username + messageid
		GetLocalTime(&horodate);
		lenComputerName=sizeof(szComputerName); GetComputerName(szComputerName,&lenComputerName);

		len=sprintf_s(szHeader,sizeof(szHeader),"%02d/%02d-%02d:%02d:%02d:%03d %s@%s [%d] ",
			(int)horodate.wDay,(int)horodate.wMonth,
			(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
			szUserName,szComputerName,LOWORD(dwMsg)); 
		if (!WriteFile(hfLogs,szHeader,len,&dw,NULL))
		{
			TRACE((TRACE_ERROR,_F_,"WriteFile(%s,%ld)=%d",gszLogFileName,len,GetLastError()));
			goto end;
		}
		// formattage message 
		// ?? FORMAT_MESSAGE_ALLOCATE_BUFFER : sample : ms-help://MS.W7SDK.1033/MS.W7SDKCOM.1033/wes/wes/sample_message_text_file.htm
		dwMsgLen=FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,NULL,dwMsg,0x040c,buf2048,sizeof(buf2048),pInsertStrings);
		if (dwMsgLen==0)
		{
			TRACE((TRACE_ERROR,_F_,"FormatMessage(wType=0x%04x dwMsg=0x%08lx)=%d",wType,dwMsg,GetLastError()));
			goto end;
		}
		//memcpy(buf2048+dwMsgLen,"\r\n\0",3);
		//dwMsgLen+=2;
		buf2048[dwMsgLen]=0;
		// écriture message
		if (!WriteFile(hfLogs,buf2048,dwMsgLen,&dw,NULL))
		{
			TRACE((TRACE_ERROR,_F_,"WriteFile(%s,%ld)=%d",gszLogFileName,dwMsgLen,GetLastError()));
			goto end;
		}
	}	

	if (gbWindowsEventLog) // log dans le journal d'événements de Windows
	{
		hEventLog=RegisterEventSource(NULL,"swSSORecover");
		if (hEventLog==NULL)
		{
			TRACE((TRACE_ERROR,_F_,"RegisterEventSource()=%d",GetLastError()));
			goto end;
		}
		
		// détermine le SID de l'utilisateur courant
		cbSid=0;
		cbRDN=0;
		LookupAccountName(NULL,szUserName,NULL,&cbSid,NULL,&cbRDN,&eUse); // pas de test d'erreur, car la fonction échoue forcément
		if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
		{
			TRACE((TRACE_ERROR,_F_,"LookupAccountName[1](%s)=%d",szUserName,GetLastError()));
			goto end;
		}
		pSid=(SID *)malloc(cbSid); if (pSid==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbSid)); goto end; }
		pszRDN=(char *)malloc(cbRDN); if (pszRDN==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbRDN)); goto end; }
		if(!LookupAccountName(NULL,szUserName,pSid,&cbSid,pszRDN,&cbRDN,&eUse))
		{
			TRACE((TRACE_ERROR,_F_,"LookupAccountName[2](%s)=%d",szUserName,GetLastError()));
			goto end;
		}
		TRACE((TRACE_INFO,_F_,"LookupAccountName(%s) pszRDN=%s",szUserName,pszRDN));
		//
		if (!ReportEvent(hEventLog,wType,0,dwMsg,pSid,wNumStrings,0,(LPCSTR*)pInsertStrings, NULL))
		{
			TRACE((TRACE_ERROR,_F_,"ReportEvent(wType=0x%04x dwMsg=0x%08lx)=%d",wType,dwMsg,GetLastError()));
			goto end;
		}
	}

	rc=0;
end:
	if (pSid!=NULL) free(pSid);
	if (pszRDN!=NULL) free(pszRDN);

	if (hEventLog!=NULL) DeregisterEventSource(hEventLog);
	if (hfLogs!=INVALID_HANDLE_VALUE) CloseHandle(hfLogs); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
