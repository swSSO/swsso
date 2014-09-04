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

	if (lpPreviousAuthentInfoType!=NULL && lpPreviousAuthentInfo!=NULL) // ISSUE#173 : traitement du changement de mot de passe à l'ouverture de session
	{
		// Construit et envoie la requête à SVC
		TRACE((TRACE_INFO,_F_,"lpPreviousAuthentInfoType=%S",lpPreviousAuthentInfoType));
		TRACE((TRACE_INFO,_F_,"lpPreviousAuthentInfo=0x%08lx",lpPreviousAuthentInfo));
		if (swBuildAndSendRequest(lpPreviousAuthentInfoType,lpPreviousAuthentInfo)!=0) goto end;
	}
	
	// Construit et envoie la requête à SVC
	TRACE((TRACE_INFO,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	TRACE((TRACE_INFO,_F_,"lpAuthentInfo=0x%08lx",lpAuthentInfo));
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
		TRACE((TRACE_INFO,_F_,"lpPreviousAuthentInfoType=%S",lpPreviousAuthentInfoType));
		TRACE((TRACE_INFO,_F_,"lpPreviousAuthentInfo=0x%08lx",lpPreviousAuthentInfo));
		if (swBuildAndSendRequest(lpPreviousAuthentInfoType,lpPreviousAuthentInfo)!=0) goto end;
	}

	// Construit et envoie la requête à SVC
	TRACE((TRACE_INFO,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	TRACE((TRACE_INFO,_F_,"lpAuthentInfo=0x%08lx",lpAuthentInfo));
	if (swBuildAndSendRequest(lpAuthentInfoType,lpAuthentInfo)!=0) goto end;

end:
	TRACE((TRACE_LEAVE,_F_,""));
	return WN_SUCCESS;
}
