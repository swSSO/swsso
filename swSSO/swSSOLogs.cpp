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
// swSSOLogs.cpp -> nouveau en 0.93
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
int swLogEvent(WORD wType,DWORD dwMsg,char *pszParam1,char *pszParam2,char *pszParam3,char *pszParam4,int iAction)
{
	TRACE((TRACE_ENTER,_F_,"wType=0x%04x dwMsg=0x%08lx",wType,dwMsg));
	int rc=-1;
	HANDLE hEventLog=NULL;
	char *pInsertStrings[4] = {NULL,NULL,NULL}; // ISSUE#230
	WORD wNumStrings=0;
	HANDLE hfLogs=INVALID_HANDLE_VALUE;
	char buf2048[2048];
	char szHeader[512]; 

	DWORD dwMsgLen;
	DWORD dw;
	DWORD len;
	SYSTEMTIME horodate;
	

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
		case LOG_LEVEL_INFO_MANAGED: 
			if ((dwMsg==MSG_SECONDARY_LOGIN_SUCCESS || dwMsg==MSG_SECONDARY_LOGIN_BAD_PWD) &&
				(gptActions[iAction].iConfigId==0)) { TRACE((TRACE_DEBUG,_F_,"giLogLevel=%d config %d non managee",giLogLevel,iAction)); goto end; }
			break;
	}

	if (pszParam1!=NULL) { pInsertStrings[0]=pszParam1; wNumStrings++; }
	if (pszParam2!=NULL) { pInsertStrings[1]=pszParam2; wNumStrings++; }
	if (pszParam3!=NULL) { pInsertStrings[2]=pszParam3; wNumStrings++; }
	if (pszParam4!=NULL) { pInsertStrings[3]=pszParam4; wNumStrings++; }

	
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
		

		len=wsprintf(szHeader,"%02d/%02d-%02d:%02d:%02d:%03d %s@%s [%d] ",
			(int)horodate.wDay,(int)horodate.wMonth,
			(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
			gszUserName,gszComputerName,LOWORD(dwMsg)); 
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
		hEventLog=RegisterEventSource(NULL,"swSSO");
		if (hEventLog==NULL)
		{
			TRACE((TRACE_ERROR,_F_,"RegisterEventSource()=%d",GetLastError()));
			goto end;
		}	
		//
		if (!ReportEvent(hEventLog,wType,0,dwMsg,gpSid,wNumStrings,0,(LPCSTR*)pInsertStrings, NULL))
		{
			TRACE((TRACE_ERROR,_F_,"ReportEvent(wType=0x%04x dwMsg=0x%08lx)=%d",wType,dwMsg,GetLastError()));
			goto end;
		}
	}

	rc=0;
end:
	
	if (hEventLog!=NULL) DeregisterEventSource(hEventLog);
	if (hfLogs!=INVALID_HANDLE_VALUE) CloseHandle(hfLogs); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swStat() 
//-----------------------------------------------------------------------------
// Ecrit le fichier de stat swsso.stat (nouveau 0.99 - ISSUE#106)
// Dans le même répertoire que le fichier swsso.ini
// Format CSV, une ligne unique :
// USERNAME;COMPUTERNAME;date dernière connexion réussie AAAAMMJJ;nb d'applications actives;nbsssoréalisés
// En 1.06 changement de format
// SHA1(USERNAME);date dernière connexion réussie AAAAMMJJ;nb applis actives;nbsssoréalisés;nb applis actives enrôlées depuis le serveur
//-----------------------------------------------------------------------------
int swStat(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	HANDLE hfStat=INVALID_HANDLE_VALUE;
	char buf2048[2048];
	char szFilename[_MAX_PATH+2]; // pas beau mais comme .stat fait 1 car de plus que .ini, ça ne marchera pas mais au moins le buffer d'explosera pas..
	DWORD len, dw;
	char *p;
	int iNbActiveApps,iNbActiveAppsFromServer;
	HCRYPTHASH hHash=NULL;
	DWORD lenHash;
	unsigned char bufHashValue[HASH_LEN];
	char szHashValue[HASH_LEN*2+1];
	char *pszResult=NULL;
	char szTruncatedComputerName[7+1];
			
	if (gLastLoginTime.wYear==0) // ISSUE#171
	{ 
		TRACE((TRACE_INFO,_F_,"gLastLoginTime=0, utilisateur non connecte --> ne génère pas de stat"));
		rc=0; goto end; 
	}

	// nom du fichier : swsso.ini -> swsso.stat
	strcpy_s(szFilename,sizeof(szFilename),gszCfgFile);
	p=strrchr(szFilename,'.');
	if (p==NULL) { TRACE((TRACE_ERROR,_F_,"gszCfgFile=%s",gszCfgFile)); goto end; }
	memcpy(p+1,"stat",5);
	
	// récup du nb d'applis
	GetNbActiveApps(&iNbActiveApps,&iNbActiveAppsFromServer);
	
	// hash du username
	if (!CryptCreateHash(ghProv,CALG_SHA1,0,0,&hHash)) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash(CALG_SHA1)=0x%08lx",GetLastError())); goto end; }
	if (!CryptHashData(hHash,(BYTE*)gszUserName,strlen(gszUserName),0)) { TRACE((TRACE_ERROR,_F_,"CryptHashData()=0x%08lx",GetLastError())); goto end; }
	lenHash=HASH_LEN;
	if (!CryptGetHashParam(hHash,HP_HASHVAL,bufHashValue,&lenHash,0)) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)=0x%08lx",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufHashValue,lenHash,"hash"));
	swCryptEncodeBase64(bufHashValue,HASH_LEN,szHashValue);

	// tronque le computer name
	strncpy_s(szTruncatedComputerName,sizeof(szTruncatedComputerName),gszComputerName,7);

	if (giStat & 1) // stats fichier
	{
		// ouverture du fichier
		hfStat=CreateFile(szFilename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hfStat==INVALID_HANDLE_VALUE)
		{
			TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFilename,GetLastError()));
			goto end;
		}

		// SHA1(USERNAME);date dernière connexion réussie AAAAMMJJ;nb applis actives;nbsssoréalisés;nb applis actives enrôlées depuis le serveur
		len=wsprintf(buf2048,"%s;%04d%02d%02d;%d;%d;%d;%s",
			szHashValue,
			(int)gLastLoginTime.wYear,(int)gLastLoginTime.wMonth,(int)gLastLoginTime.wDay,
			iNbActiveApps,guiNbWINSSO+guiNbWEBSSO+guiNbPOPSSO,iNbActiveAppsFromServer,szTruncatedComputerName); 
		if (!WriteFile(hfStat,buf2048,len,&dw,NULL))
		{
			TRACE((TRACE_ERROR,_F_,"WriteFile(%s,%ld)=%d",szFilename,len,GetLastError()));
			goto end;
		}
	}
	if (giStat & 2) // stat upload
	{
		sprintf_s(buf2048,sizeof(buf2048),"%s?action=uploadstats&shausername=%s&logindate=%04d%02d%02d&nconfigs=%d&nsso=%d&nenrolled=%d&computername=%s",
			gszWebServiceAddress,
			szHashValue,
			(int)gLastLoginTime.wYear,(int)gLastLoginTime.wMonth,(int)gLastLoginTime.wDay,
			iNbActiveApps,guiNbWINSSO+guiNbWEBSSO+guiNbPOPSSO,iNbActiveAppsFromServer,szTruncatedComputerName); 
		TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",buf2048));
		pszResult=HTTPRequest(gszServerAddress,giServerPort,buf2048,8,NULL); // remarque : RAF du résultat
	}
	rc=0;
end:
	if (pszResult!=NULL) free(pszResult);
	if (hHash!=NULL) CryptDestroyHash(hHash);
	if (hfStat!=INVALID_HANDLE_VALUE) CloseHandle(hfStat); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

