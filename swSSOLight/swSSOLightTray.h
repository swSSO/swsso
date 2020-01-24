//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2020 - Sylvain WERDEFROY
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
// swSSOTray.h
//-----------------------------------------------------------------------------

HWND CreateMainWindow(void);
int  CreateSystray(HWND wMain);
void DestroySystray(HWND wMain);
void SSOActivate(HWND w);
int RefreshRights(BOOL bForced,BOOL bReportSync);

extern unsigned int gMsgTaskbarRestart;
extern HWND gwSignUp;

#define TRAY_MENU_PROPRIETES 2
#define TRAY_MENU_QUITTER    3
#define TRAY_MENU_MDP		4
#define TRAY_MENU_THIS_APPLI	6
#define TRAY_MENU_APPNSITES  7
#define TRAY_MENU_SSO_NOW	8
#define TRAY_MENU_LAUNCH_APP 9
#define TRAY_MENU_SIGNUP 15
