// swSSOCM.cpp : Defines the exported functions for the DLL application.
//

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
	UNREFERENCED_PARAMETER(lpPreviousAuthentInfoType);
	UNREFERENCED_PARAMETER(lpPreviousAuthentInfo);
	UNREFERENCED_PARAMETER(lpStationName);
	UNREFERENCED_PARAMETER(StationHandle);
	UNREFERENCED_PARAMETER(lpLogonScript);
	
	TRACE((TRACE_ENTER,_F_,""));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfo=0x%08lx",lpAuthentInfo));
	
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
	UNREFERENCED_PARAMETER(lpPreviousAuthentInfoType);
	UNREFERENCED_PARAMETER(lpPreviousAuthentInfo);
	UNREFERENCED_PARAMETER(lpStationName);
	UNREFERENCED_PARAMETER(StationHandle);
	UNREFERENCED_PARAMETER(dwChangeInfo);

	TRACE((TRACE_ENTER,_F_,""));
	TRACE((TRACE_INFO,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	TRACE((TRACE_INFO,_F_,"lpAuthentInfo=0x%08lx",lpAuthentInfo));
	TRACE((TRACE_INFO,_F_,"dwChangeInfo=0x%08lx",dwChangeInfo));
	
	// Construit et envoie la requête à SVC
	if (swBuildAndSendRequest(lpAuthentInfoType,lpAuthentInfo)!=0) goto end;
	
end:
	TRACE((TRACE_LEAVE,_F_,""));
	return WN_SUCCESS;
}
