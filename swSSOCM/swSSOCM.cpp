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

/*
TESTS SUR WINDOWS 7 :
Ouverture de session : NPLogon(mdp,null)
Changement de mdp dans la session : NPLogon(mdp_new,mdp_new) puis NPPassword(mdp_new,mdp_old)
Changement de mdp forcé à l'ouverture : NPLogon(mdp_new,mdp_new) puis NPPassword(mdp_new,mdp_old) puis NPLogon(mdp_new,NULL)
==> Conclusion : il faut jeter les notifs NPLogon(mdp_new,mdp_new), elles foutent la grouille !

TESTS SUR WINDOWS 7 AVEC SOPHOS
Ouverture de session : NPLogon(mdp,mdp) 2 fois puis NPLogon(mdp,null)
Changement de mdp dans la session : NPLogon(mdp_new,mdp_new) 2 fois puis NPPassword(mdp_new,mdp_old)
Changement de mdp forcé à l'ouverture : NPLogon(mdp_new,mdp_new) 
*/

#include "stdafx.h"

DWORD APIENTRY NPGetCaps(__in DWORD nIndex)
{
	DWORD rc=0;
	TRACE((TRACE_ENTER,_F_,"nIndex=0x%08lx",nIndex));
	if (nIndex==WNNC_START) 
		rc=1;
	else if (nIndex==WNNC_NET_TYPE)
		rc=WNNC_CRED_MANAGER;
	else if (nIndex==WNNC_DRIVER_VERSION)
		rc=1;
	else if (nIndex==WNNC_SPEC_VERSION)
		rc=WNNC_SPEC_VERSION51;
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}

#ifdef TRACES_ACTIVEES
//-----------------------------------------------------------------------------
// swTraceAuthentInfo()
//-----------------------------------------------------------------------------
void swTraceAuthentInfo(LPCWSTR lpAuthentInfoType,LPVOID lpAuthentInfo)
{
	TRACE((TRACE_ENTER,_F_,""));
	int ret;
	UNICODE_STRING usUserName,usPassword,usLogonDomainName;
	char szUserName[USER_LEN];
	char bufPassword[PWD_LEN];
	char szLogonDomainName[DOMAIN_LEN];
	int lenUserName,lenLogonDomainName;
	
	if (lpAuthentInfoType==NULL) { TRACE((TRACE_ERROR,_F_,"lpAuthentInfoType=NULL")); goto end; }
	if (lpAuthentInfo==NULL) { TRACE((TRACE_ERROR,_F_,"lpAuthentInfo=NULL")); goto end; }

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
	TRACE((TRACE_DEBUG,_F_,"usUserName       =0x%08lx",usUserName.Buffer));
	TRACE((TRACE_DEBUG,_F_,"usPassword       =0x%08lx",usPassword.Buffer));
	TRACE((TRACE_DEBUG,_F_,"usLogonDomainName=0x%08lx",usLogonDomainName.Buffer));

	// Conversion de chaînes UNICODE_STRING
	memset(szUserName,0,sizeof(szUserName));
	memset(bufPassword,0,sizeof(bufPassword));
	memset(szLogonDomainName,0,sizeof(szLogonDomainName));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)usLogonDomainName.Buffer,usLogonDomainName.Length,"usLogonDomainName"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)usUserName.Buffer,usUserName.Length,"usUserName"));
	TRACE_BUFFER((TRACE_PWD,_F_,(unsigned char*)usPassword.Buffer,usPassword.Length,"usPassword"));
	// domaine
	ret=WideCharToMultiByte(CP_ACP,0,usLogonDomainName.Buffer,usLogonDomainName.Length/2,szLogonDomainName,sizeof(szLogonDomainName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usLogonDomainName)=%d",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"szLogonDomainName=%s",szLogonDomainName));
	lenLogonDomainName=(int)strlen(szLogonDomainName);
	// utilisateur
	ret=WideCharToMultiByte(CP_ACP,0,usUserName.Buffer,usUserName.Length/2,szUserName,sizeof(szUserName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usUserName)=%d",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"szUserName=%s",szUserName));
	lenUserName=(int)strlen(szUserName);
	// mot de passe
	TRACE((TRACE_DEBUG,_F_,"usPassword.MaximumLength=%d",usPassword.MaximumLength));
	ret=WideCharToMultiByte(CP_ACP,0,usPassword.Buffer,usPassword.Length/2,bufPassword,sizeof(bufPassword),NULL,NULL);
	if (ret==0) { TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usPassword)=%d",GetLastError())); goto end; }
	TRACE((TRACE_PWD,_F_,"bufPassword=%s",bufPassword));
	SecureZeroMemory(bufPassword,sizeof(bufPassword));
end:
	TRACE((TRACE_LEAVE,_F_,""));
	return;
}


#endif

DWORD APIENTRY NPLogonNotify(
  __in   PLUID lpLogon,
  __in   LPCWSTR lpAuthentInfoType,
  __in   LPVOID lpAuthentInfo,
  __in   LPCWSTR lpPreviousAuthentInfoType,
  __in   LPVOID lpPreviousAuthentInfo,
  __in   LPWSTR lpStationName,
  __in   LPVOID StationHandle,
  __out  LPWSTR *lpLogonScript
)
{
	UNREFERENCED_PARAMETER(lpLogon);
	UNREFERENCED_PARAMETER(lpStationName);
	UNREFERENCED_PARAMETER(StationHandle);
	UNREFERENCED_PARAMETER(lpLogonScript);
	
	TRACE((TRACE_ENTER,_F_,""));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfoType=        0x%08lx",lpAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfo=            0x%08lx",lpAuthentInfo));
#ifdef TRACES_ACTIVEES
	swTraceAuthentInfo(lpAuthentInfoType,lpAuthentInfo);
#endif
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfoType=0x%08lx",lpPreviousAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo=    0x%08lx",lpPreviousAuthentInfo));
#ifdef TRACES_ACTIVEES
	swTraceAuthentInfo(lpPreviousAuthentInfoType,lpPreviousAuthentInfo);
#endif

	// virer les notifs NPLogon(mdp_new,mdp_new)
	if (lpPreviousAuthentInfoType!=NULL && lpPreviousAuthentInfo!=NULL)
	{
		UNICODE_STRING p1,p2;
		if (wcscmp(lpAuthentInfoType,L"MSV1_0:Interactive")==0)
		{
			p1=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
			p2=((MSV1_0_INTERACTIVE_LOGON*)lpPreviousAuthentInfo)->Password;
		}
		else
		{
			p1=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
			p2=((KERB_INTERACTIVE_LOGON*)lpPreviousAuthentInfo)->Password;
		}
		TRACE((TRACE_DEBUG,_F_,"lpAuthentInfo.Password.Buffer=        0x%08lx",p1.Buffer));
		TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo.Password.Buffer=0x%08lx",p2.Buffer));
		if (p1.Buffer==p2.Buffer)
		{
			TRACE((TRACE_INFO,_F_,"lpAuthentInfo.Password=lpPreviousAuthentInfo.Password --> cas du NPLogon(new,new) --> ON IGNORE !"));
			goto end;
		}
	}
	if (lpPreviousAuthentInfoType!=NULL && lpPreviousAuthentInfo!=NULL) // ISSUE#173 : traitement du changement de mot de passe à l'ouverture de session
	{
		// Construit et envoie la requête à SVC
		if (swBuildAndSendRequest(lpPreviousAuthentInfoType,lpPreviousAuthentInfo)!=0) goto end;
	}
	// Construit et envoie la requête à SVC
	if (swBuildAndSendRequest(lpAuthentInfoType,lpAuthentInfo)!=0) goto end;

end:
	TRACE((TRACE_LEAVE,_F_,""));
	return WN_SUCCESS;
}
 
DWORD APIENTRY NPPasswordChangeNotify(
  __in  LPCWSTR lpAuthentInfoType,
  __in  LPVOID lpAuthentInfo,
  __in  LPCWSTR lpPreviousAuthentInfoType,
  __in  LPVOID lpPreviousAuthentInfo,
  __in  LPWSTR lpStationName,
  __in  LPVOID StationHandle,
  __in  DWORD dwChangeInfo
)
{
	UNREFERENCED_PARAMETER(lpStationName);
	UNREFERENCED_PARAMETER(StationHandle);
	UNREFERENCED_PARAMETER(dwChangeInfo);

	TRACE((TRACE_ENTER,_F_,""));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfoType=        0x%08lx",lpAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfo=            0x%08lx",lpAuthentInfo));
#ifdef TRACES_ACTIVEES
	swTraceAuthentInfo(lpAuthentInfoType,lpAuthentInfo);
#endif
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfoType=0x%08lx",lpPreviousAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo=    0x%08lx",lpPreviousAuthentInfo));
#ifdef TRACES_ACTIVEES
	swTraceAuthentInfo(lpPreviousAuthentInfoType,lpPreviousAuthentInfo);
#endif
	
	if (lpPreviousAuthentInfoType!=NULL && lpPreviousAuthentInfo!=NULL) // ISSUE#173 : traitement du changement de mot de passe à l'ouverture de session
	{
		// Construit et envoie la requête à SVC
		if (swBuildAndSendRequest(lpPreviousAuthentInfoType,lpPreviousAuthentInfo)!=0) goto end;
	}
	// Construit et envoie la requête à SVC
	if (swBuildAndSendRequest(lpAuthentInfoType,lpAuthentInfo)!=0) goto end;

end:
	TRACE((TRACE_LEAVE,_F_,""));
	return WN_SUCCESS;
}
