//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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
// stdafx.h
//-----------------------------------------------------------------------------

#include <mshtml.h>
#include <oleacc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include <tchar.h>
#include <wincrypt.h>  
#include <Wtsapi32.h>
#include <msxml2.h>
#include <Winhttp.h>
#include <Exdisp.h>
//#include <LM.h>
//#include <Lmapibuf.h>
//#include <Lmstats.h>
#include <Lmcons.h>
#include <MsHtmcid.h>
#include <sddl.h>
#include <Iads.h>
#include <Adshlp.h>
#include <TlHelp32.h>
#define SECURITY_WIN32
#include <Security.h>
#include <uiautomation.h>

#include "resource.h"
#include "swSSOTrace.h"
#include "swSSOCrypt.h"
#include "swSSOAppNsites.h"
#include "swSSOConfig.h"
#include "swSSOTools.h"
#include "swSSOLaunchApp.h"
#include "swSSOPolicies.h"
#include "swSSODomains.h"
#include "swSSORecovery.h"
#include "swSSOMain.h"
#include "swSSOTray.h"
#include "swSSOWin.h"
#include "swSSOWeb.h"
#include "swSSOXeb.h"
#include "swSSOFirefox.h"
#include "swSSOChrome.h"
#include "swSSOFirefoxTools.h"
#include "swSSOIETools.h"
#include "swSSOEdge.h"
#include "ISimpleDOMNode.h"
#include "ISimpleDOMDocument.h"
#include "swSSOLogs.h"
#include "swSSOLogMessages.h"
#include "swSSOAD.h"
#include "swSSOSelectAccount.h"
#include "swSSOAdminNoPwd.h"
#include "swSSOAdmin.h"
#include "swSSOMobile.h"
#include "swSSOProtectMemory.h"







