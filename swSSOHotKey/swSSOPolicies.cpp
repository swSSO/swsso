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
// swSSOPolicies.cpp
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#include "stdafx.h"

int giPastePwd_Ctrl=0;
int giPastePwd_Alt=0;
int giPastePwd_Shift=0;
int giPastePwd_Win=0;
int giPastePwd_Key=0;

//-----------------------------------------------------------------------------
// LoadPolicies()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void LoadPolicies(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc;
	HKEY hKey=NULL;
	DWORD dwValue,dwValueSize,dwValueType;

	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_HOTKEY,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_CTRL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPastePwd_Ctrl=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_ALT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPastePwd_Alt=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_SHIFT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPastePwd_Shift=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_WIN,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPastePwd_Win=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_KEY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPastePwd_Key=(BOOL)dwValue; 

		RegCloseKey(hKey);
	}


#ifdef TRACES_ACTIVEES
	TRACE((TRACE_INFO,_F_,"HOTKEY --------------"));
	TRACE((TRACE_INFO,_F_,"giPastePwd_Ctrl =%d",giPastePwd_Ctrl));
	TRACE((TRACE_INFO,_F_,"giPastePwd_Alt  =%d",giPastePwd_Alt));
	TRACE((TRACE_INFO,_F_,"giPastePwd_Shift=%d",giPastePwd_Shift));
	TRACE((TRACE_INFO,_F_,"giPastePwd_Win  =%d",giPastePwd_Win));
	TRACE((TRACE_INFO,_F_,"giPastePwd_Key  =0x%08lx",giPastePwd_Key));
	
#endif

	TRACE((TRACE_LEAVE,_F_, ""));
}
