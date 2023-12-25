//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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

#include "stdafx.h"
#define TRAY_PASTE_PASSWORD 99 // pour la fonction d'assistance au changement de mot de passe
#define TRAY_PASTE_IDORPASSWORD 98 // pour le copier-coller depuis la treewiew de la fenêtre de gestion des sites

//-----------------------------------------------------------------------------
// KeyboardProc()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
SWSSOHOTKEY_API LRESULT CALLBACK KeyboardProc(int code,WPARAM wp,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "code=%d wp=0x%08lx lp=0x%08lx",code,wp,lp));
	
	if (code==HC_ACTION && (lp & 0x80000000))
	{
		if (wp==0x56 && !(lp & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) // CTRL+V = copier-coller depuis la treewiew de la fenêtre de gestion des sites
		{
			TRACE((TRACE_INFO, _F_, "Collage mot de passe -- copier-coller depuis la treewiew de la fenêtre de gestion des sitess"));
			// cherche fenêtre technique swSSO et lui envoie un message
			HWND w = FindWindow("swSSOClass", "swSSOTray");
			if (w == NULL)
			{
				TRACE((TRACE_ERROR, _F_, "swSSO window not found"));
				goto end;
			}
			// il faut relacher les touches sinon la saisie se passe mal ensuite !
			keybd_event(VK_CONTROL, LOBYTE(MapVirtualKey(VK_CONTROL, 0)), KEYEVENTF_KEYUP, 0);
			TRACE((TRACE_DEBUG, _F_, "PostMessage to HWND=0x%08lx", w));
			PostMessage(w, WM_COMMAND, MAKEWORD(TRAY_PASTE_IDORPASSWORD, 0), 0);
		}
		else // assistance au changement de mot de passe
		{
		
			int iCtrl,iAlt,iShift,iWin;
		
			iCtrl=(giPastePwd_Ctrl==1)	 ? GetKeyState(VK_CONTROL) & 0x8000 : 1;
			iAlt=(giPastePwd_Alt==1)	 ? GetKeyState(VK_MENU) & 0x8000 : 1;
			iShift=(giPastePwd_Shift==1) ? GetKeyState(VK_SHIFT) & 0x8000 : 1;
			iWin=(giPastePwd_Win==1)	 ? GetKeyState(VK_LWIN) & 0x8000 : 1;

			TRACE((TRACE_DEBUG,_F_,"iCtrl=%d iAlt=%d iShift=%d iWin=%d",iCtrl,iAlt,iShift,iWin));
			if (wp==(WPARAM)giPastePwd_Key && ((lp & 0x8000)==0) && iCtrl && iAlt && iShift && iWin)
			{
				TRACE((TRACE_INFO,_F_,"Collage mot de passe -- assistance au changement de mot de passe"));
				// cherche fenêtre technique swSSO et lui envoie un message
				HWND w=FindWindow("swSSOClass","swSSOTray");
				if (w==NULL)
				{
					TRACE((TRACE_ERROR,_F_,"swSSO window not found"));
					goto end;
				}
				// il faut relacher les touches sinon la saisie se passe mal ensuite !
				if (iCtrl==0x8000) keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0); 
				if (iAlt==0x8000) keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),KEYEVENTF_KEYUP,0); 
				if (iShift==0x8000) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0); 
				if (iWin==0x8000) keybd_event(VK_LWIN,LOBYTE(MapVirtualKey(VK_LWIN,0)),KEYEVENTF_KEYUP,0); 

				TRACE((TRACE_DEBUG,_F_,"PostMessage to HWND=0x%08lx",w));
				PostMessage(w,WM_COMMAND,MAKEWORD(TRAY_PASTE_PASSWORD,0),0);
			}
		}
	}

end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return CallNextHookEx(NULL, code, wp, lp);
}