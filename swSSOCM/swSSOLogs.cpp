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
	char szUserName[100 + 1]="";
	DWORD lenUserName;


	DWORD cbRDN,cbSid;
	SID_NAME_USE eUse;
	SID *pSid=NULL;
	char *pszRDN=NULL;

	if (pszParam1!=NULL) { pInsertStrings[0]=pszParam1; wNumStrings++; }
	if (pszParam2!=NULL) { pInsertStrings[1]=pszParam2; wNumStrings++; }
	if (pszParam3!=NULL) { pInsertStrings[2]=pszParam3; wNumStrings++; }

	lenUserName=sizeof(szUserName); GetUserName(szUserName,&lenUserName);

	hEventLog=RegisterEventSource(NULL,"swSSOCM");
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

	rc=0;
end:
	if (pSid!=NULL) free(pSid);
	if (pszRDN!=NULL) free(pszRDN);

	if (hEventLog!=NULL) DeregisterEventSource(hEventLog);
	if (hfLogs!=INVALID_HANDLE_VALUE) CloseHandle(hfLogs); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
