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
#include <windows.h>
#include <Sddl.h>
#include <stdio.h>
#include <tchar.h>
#include "swSSOTrace.h"
#include "swSSOCrypt.h"
#include "swSSOProtectMemory.h"
#include <Psapi.h>

// swSSOSVCInstall
bool IsInstalled(void);
int Install(void);
int Uninstall(void);

// swSSOSVCData
void swInitData(void);
int swWaitForMessage(void);
int swCreatePipe(void);
void swDestroyPipe(void);
