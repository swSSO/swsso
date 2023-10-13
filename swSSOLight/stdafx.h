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

#include "..\swSSO\swSSOTrace.h"
#include "..\swSSO\swSSOCrypt.h"
#include "swSSOLightAppNsites.h"
#include "swSSOLightConfig.h"
#include "swSSOLightTools.h"
#include "..\swSSO\swSSOLaunchApp.h"
#include "swSSOLightPolicies.h"
#include "swSSOLight.h"
#include "swSSOLightTray.h"
#include "..\swSSO\swSSOWin.h"
#include "..\swSSO\swSSOWeb.h"
#include "..\swSSO\swSSOXeb.h"
#include "..\swSSO\swSSOFirefox.h"
#include "..\swSSO\swSSOChrome.h"
#include "..\swSSO\swSSOFirefoxTools.h"
#include "..\swSSO\swSSOIETools.h"
#include "..\swSSO\swSSOEdge.h"
#include "..\swSSO\ISimpleDOMNode.h"
#include "..\swSSO\ISimpleDOMDocument.h"
#include "swSSOLightLogs.h"
#include "..\swSSO\swSSOLogMessages.h"
#include "..\swSSO\swSSOSelectAccount.h"
#include "..\swSSO\swSSOMobile.h"
#include "..\swSSO\swSSOProtectMemory.h"
#include "..\swSSO\swSSOBrowser.h"

