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

//-----------------------------------------------------------------------------
// KeyboardProc()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
SWSSOHOTKEY_API LRESULT CALLBACK KeyboardProc(int code,WPARAM wp,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "code=%d wp=0x%08lx lp=0x%08lx",code,wp,lp));
	
	if (code==HC_ACTION)
	{
		int np = (lp & 0x8000);
		int nCtrl = GetKeyState(VK_CONTROL) & 0x8000;
		int nAlt = GetKeyState(VK_MENU) & 0x8000;
		
		if (wp == 0x4C && np==0 && nCtrl!=0 && nAlt!=0)
		{
			TRACE((TRACE_INFO,_F_,"Combinaison coller mot de passe"));
			// cherche fenêtre technique swSSO et lui envoie un message
			HWND w=FindWindow("swSSOClass","swSSOTray");
			if (w==NULL)
			{
				TRACE((TRACE_ERROR,_F_,"swSSO window not found"));
				goto end;
			}
#define TRAY_PASTE_PASSWORD 99
			TRACE((TRACE_DEBUG,_F_,"PostMessage to HWND=0x%08lx",w));
			PostMessage(w,WM_APP, 0, MAKELONG(TRAY_PASTE_PASSWORD,0));
		}
	}

end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return CallNextHookEx(NULL, code, wp, lp);
}