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
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfoType=0x%08lx",lpPreviousAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo=    0x%08lx",lpPreviousAuthentInfo));

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
		TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo.Password.BUffer=0x%08lx",p2.Buffer));
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
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfoType=0x%08lx",lpPreviousAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpPreviousAuthentInfo=    0x%08lx",lpPreviousAuthentInfo));
	
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
