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
// swSSOTray.h
//-----------------------------------------------------------------------------

HWND CreateMainWindow(void);
int  CreateSystray(HWND wMain);
void DestroySystray(HWND wMain);
void SSOActivate(HWND w);
int RefreshRights(BOOL bForced,BOOL bReportSync);

extern unsigned int gMsgTaskbarRestart;
extern HWND gwSignUp;
extern char* gpszClipboardPassword ; // pour la fonction d'assistance au changement de mot de passe
extern char* gpszPasteId; // pour le copier-coller depuis la treewiew de la fenêtre de gestion des sites
extern int giPasteIdOrPassword; // 0=Id, 1=Pwd
extern char* gpszPasteIdOrPassword; // pour le copier-coller depuis la treewiew de la fenêtre de gestion des sites

int InstallHotKey(void);
int UninstallHotKey(void);

#define TRAY_MENU_ACTIVER    1
#define TRAY_MENU_PROPRIETES 2
#define TRAY_MENU_QUITTER    3
#define TRAY_MENU_MDP		4
#define TRAY_MENU_PORTAL		5
#define TRAY_MENU_THIS_APPLI	6
#define TRAY_MENU_APPNSITES  7
#define TRAY_MENU_SSO_NOW	8
#define TRAY_MENU_LAUNCH_APP 9
#define TRAY_MENU_CHANGEAPPPWD 10
#define TRAY_MENU_MDP_WINDOWS 11
#define TRAY_MENU_REFRESH_RIGHTS 12
#define TRAY_MENU_AIDE	13
#define TRAY_MENU_ASK_THIS_APP	14
#define TRAY_MENU_SIGNUP 15
#define TRAY_PASTE_PASSWORD 99 // pour la fonction d'assistance au changement de mot de passe
#define TRAY_PASTE_IDORPASSWORD 98 // pour le copier-coller depuis la treewiew de la fenêtre de gestion des sites