//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
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

#include "stdafx.h"

int giServiceTimeOut=60;

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

	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_CM,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SERVICE_TIMEOUT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giServiceTimeOut=(int)dwValue; 
		else
		{
			TRACE((TRACE_INFO,_F_,"RegQueryValueEx()=%d",rc));
		}
		RegCloseKey(hKey);
	}
	else
	{
		TRACE((TRACE_INFO,_F_,"RegOpenKeyEx()=%d",rc));
	}
#ifdef TRACES_ACTIVEES
	TRACE((TRACE_INFO,_F_,"giServiceTimeOut=%d",giServiceTimeOut));
#endif
	TRACE((TRACE_LEAVE,_F_, ""));
}
