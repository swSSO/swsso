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


#pragma once

#include "targetver.h"

#define PWD_LEN 256
#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite à moi...

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <Npapi.h>
#include <Ntsecapi.h>
#include <ShellAPI.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <WinCrypt.h>
#include "swSSOCM.h"
#include "swSSOTrace.h"
#include "swSSOProtectMemory.h"
#include "swSSOLogs.h"
#include "swSSOLogMessages.h"

// swSSOCMPipe
int swBuildAndSendRequest(LPCWSTR lpAuthentInfoType,LPVOID lpAuthentInfo);
int swPipeWrite(char *bufRequest,int lenRequest);


