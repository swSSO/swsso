//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2013 - Sylvain WERDEFROY
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
// swSSOTray.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions liées au Systray (icône barre de tâches)
//-----------------------------------------------------------------------------

#include "stdafx.h"



unsigned int gMsgTaskbarRestart=0; // message registré pour recevoir les notifs de recréation du systray

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

//-----------------------------------------------------------------------------
// ShowContextMenu()
//-----------------------------------------------------------------------------
// Affichage du menu contextuel suite à un clic-droit sur l'icone SSO
//-----------------------------------------------------------------------------
// [in] handle de la fenetre qui a généré le message (=fenetre technique)
//-----------------------------------------------------------------------------
static void ShowContextMenu(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=NULL;
	
	hMenu=CreatePopupMenu();
	if (hMenu==NULL) goto end;
	
	// 0.92B6 : on laisse le menu même si case décochée, mais on fait config en local
	// 0.92B8 : nouveau paramètre dans les policies pour masquer menu "ajouter cette application"
	 if (gbShowMenu_AddApp) 
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_THIS_APPLI,GetString(IDS_MENU_THIS_APPLI));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SSO_NOW,GetString(IDS_MENU_SSO_NOW));
	if (gbShowMenu_LaunchApp)
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_LAUNCH_APP,GetString(IDS_LAUNCH_ONE_APP));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_APPNSITES,GetString(IDS_MENU_APPNSITES));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_PROPRIETES,GetString(IDS_MENU_PROP));
	if (giPwdProtection==PP_ENCRYPTED || *gszCfgPortal!=0) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	if (giPwdProtection==PP_ENCRYPTED) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_MDP,GetString(IDS_MENU_MDP));
	if (*gszCfgPortal!=0) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_PORTAL,GetString(IDS_MENU_PORTAL));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	if (gbSSOActif)
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_ACTIVER,GetString(IDS_MENU_DESACTIVER));
	else
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_ACTIVER,GetString(IDS_MENU_ACTIVER));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_QUITTER,GetString(IDS_MENU_QUITTER));
	SetForegroundWindow(w);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,pt.x, pt.y, 0, w, NULL );
end:	
	if (hMenu!=NULL) DestroyMenu(hMenu);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// SSOActivate()
//-----------------------------------------------------------------------------
// Active/Désactive swSSO selon l'état courant (gbSSOActif)
//-----------------------------------------------------------------------------
// [in] handle de la fenetre qui a généré le message (=fenetre technique)
//-----------------------------------------------------------------------------
void SSOActivate(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));

	NOTIFYICONDATA nid;
	BOOL rc;
	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd=w;
	nid.uID=0;
	nid.uFlags=NIF_TIP | NIF_ICON;
	gbSSOActif=!gbSSOActif;
	if (gbSSOActif) // ACTIVATION
	{
		nid.hIcon=ghIconSystrayActive;
		strcpy_s(nid.szTip,sizeof(nid.szTip),GetString(IDS_ACTIVE)); //max64
		if (gwPropertySheet!=NULL) ShowWindow(gwPropertySheet,SW_SHOW);
		if (gwAppNsites!=NULL) ShowWindow(gwAppNsites,SW_SHOW);
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_UNLOCK_SUCCESS,NULL,NULL,NULL,0);
	}
	else // DESACTIVATION
	{
		nid.hIcon=ghIconSystrayInactive;
		strcpy_s(nid.szTip,sizeof(nid.szTip),GetString(IDS_DESACTIVE)); //max64
		//0.83 : supprime la clé de la mémoire
		//0.96 : sauf si SSO Windows (et si on décide de le faire, attention effet de bord dans AskPWd()).
		if (giPwdProtection!=PP_WINDOWS)
		{
			if (ghKey1!=NULL) { CryptDestroyKey(ghKey1); ghKey1=NULL; }
		}
		if (gwPropertySheet!=NULL) ShowWindow(gwPropertySheet,SW_HIDE);
		if (gwAppNsites!=NULL) ShowWindow(gwAppNsites,SW_HIDE);
		if (gwAskPwd!=NULL) { EndDialog(gwAskPwd,IDCANCEL); gwAskPwd=NULL; }
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_LOCK,NULL,NULL,NULL,0);
	}
	rc=Shell_NotifyIcon(NIM_MODIFY,&nid);
	if (!rc) 
	{
		TRACE((TRACE_ERROR,_F_,"Shell_NotifyIcon[hIcon=0x%08lx szTip='%s']",nid.hIcon,nid.szTip));
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// MainWindowProc()
//-----------------------------------------------------------------------------
// WindowProc de la fenêtre technique
//-----------------------------------------------------------------------------
// [in] RAS
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MainWindowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp) 
{
	switch (msg) 
	{
		case WM_CREATE:
			TRACE((TRACE_INFO,_F_, "WM_CREATE"));
            gMsgTaskbarRestart=RegisterWindowMessage(TEXT("TaskbarCreated"));
 			TRACE((TRACE_INFO,_F_, "gMsgTaskbarRestart=%d",gMsgTaskbarRestart));
           break;
		case WM_APP: 
			switch(lp)
			{
				case WM_LBUTTONDBLCLK:
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
						if (AskPwd(NULL,TRUE)!=0) goto end;
					if (HIBYTE(GetAsyncKeyState(VK_SHIFT))!=0) 
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + SHIFT + WM_LBUTTONDBLCLK"));
						ReactivateApplicationFromCurrentWindow();
					}
					else if (HIBYTE(GetAsyncKeyState(VK_CONTROL))!=0) 
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + CTRL + WM_LBUTTONDBLCLK"));
						if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
						{
							if (AskPwd(NULL,TRUE)!=0) goto end;
							SSOActivate(w);
						}
						ShowLaunchApp();
					}
					else
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + WM_LBUTTONDBLCLK"));
						// 0.63B3 : le double click déverrouille si verrouillé, ouvre la config sinon
						if (gbSSOActif)
							ShowAppNsites(-1);
						else
							SSOActivate(w);
					}
					if (HIBYTE(GetAsyncKeyState(VK_SHIFT))!=0 && HIBYTE(GetAsyncKeyState(VK_CONTROL))!=0) AddApplicationFromCurrentWindow();
					break;
				case WM_RBUTTONUP:
					TRACE((TRACE_INFO,_F_, "WM_APP + WM_RBUTTONDOWN"));
					ShowContextMenu(w);
					break;
				case WM_CONTEXTMENU:
					TRACE((TRACE_INFO,_F_, "WM_APP + WM_CONTEXTMENU"));
					ShowContextMenu(w);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case TRAY_MENU_LAUNCH_APP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_LAUNCH_APP"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShowLaunchApp();
					break;
				case TRAY_MENU_ACTIVER:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_ACTIVER"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
						if (AskPwd(NULL,TRUE)!=0) goto end;
					SSOActivate(w);
					break;
				case TRAY_MENU_PROPRIETES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_PROPRIETES"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShowConfig();
					break;
				case TRAY_MENU_APPNSITES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_APPNSITES"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShowAppNsites(-1);
					break;
				case TRAY_MENU_MDP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_MDP"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					WindowChangeMasterPwd(FALSE);
					break;
				case TRAY_MENU_PORTAL:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_PORTAL"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShellExecute(NULL,"open",gszCfgPortal,NULL,"",SW_SHOW );
					break;
				case TRAY_MENU_THIS_APPLI:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_THIS_APPLI"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					AddApplicationFromCurrentWindow();
					break;
				case TRAY_MENU_SSO_NOW:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_SSO_NOW"));
					if (!gbSSOActif && giPwdProtection>=PP_ENCRYPTED)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ReactivateApplicationFromCurrentWindow();
					if (giTimer==0) // solution de secours du bug #98 (le timer ne s'est pas déclenché)
					{
						TRACE((TRACE_ERROR,_F_,"*************************************************"));
						TRACE((TRACE_ERROR,_F_,"************ Le timer etait arrete ! ************"));
						TRACE((TRACE_ERROR,_F_,"*************************************************"));
						LaunchTimer();
					}
					break;
				case TRAY_MENU_QUITTER:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_QUITTER"));
					PostQuitMessage(0);
					break;
			}
			break;
		case WM_WTSSESSION_CHANGE: // 0.63BETA4
			TRACE((TRACE_INFO,_F_,"gbSessionLock=%d wp=%d gbSSOActif=%d",gbSessionLock,wp,gbSSOActif));
			if (gbSessionLock && wp==WTS_SESSION_LOCK && gbSSOActif)
			{
				TRACE((TRACE_INFO,_F_,"swSSO va être verouillé"));
				SSOActivate(w);
			}
			break;
		default:
			if(msg==gMsgTaskbarRestart) // notif recréation systray
			{
				TRACE((TRACE_INFO,_F_, "gMsgTaskbarRestart received !"));
				CreateSystray(w); 
			}
			return DefWindowProc(w,msg,wp,lp);
	}	
end:
	return 0; //DefWindowProc(w,msg,wp,lp);
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// CreateMainWindow()
//-----------------------------------------------------------------------------
// Création de la fenêtre technique qui recevra tous les messages du Systray
// et les notifications de verrouillage de session Windows
//-----------------------------------------------------------------------------
// [rc] handle de la fenêtre créée, NULL si échec.
//-----------------------------------------------------------------------------
HWND CreateMainWindow(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HWND wMain=NULL;
	ATOM atom=0;
	WNDCLASS wndClass;
	
	ZeroMemory(&wndClass,sizeof(WNDCLASS));
	wndClass.style=0;
	wndClass.lpfnWndProc=MainWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = ghInstance;
	wndClass.hCursor = NULL;
	wndClass.lpszMenuName = 0;
	wndClass.hbrBackground = NULL;
	wndClass.hIcon = NULL;
	wndClass.lpszClassName = "swSSOClass";
	
	atom=RegisterClass(&wndClass);
	if (atom==0) goto end;
	
	wMain=CreateWindow("swSSOClass","swSSOTray",0,0,0,0,0,NULL,NULL,ghInstance,0);
	if (wMain==NULL) goto end;

end:
	TRACE((TRACE_LEAVE,_F_, "wMain=0x%08lx",wMain));
	return wMain;
}

//-----------------------------------------------------------------------------
// CreateSystray()
//-----------------------------------------------------------------------------
// Création du Systray
//-----------------------------------------------------------------------------
// [in] wMain : handle de la fenêtre technique pour les notifications
// [rc] 0 si OK
//-----------------------------------------------------------------------------
int CreateSystray(HWND wMain)
{
	TRACE((TRACE_ENTER,_F_, ""));

	NOTIFYICONDATA nid;
	int rc=-1;
	
	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd=wMain;
	nid.uID=0;
	nid.uFlags=NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage=WM_APP;
	nid.hIcon=(gbSSOActif ? ghIconSystrayActive : ghIconSystrayInactive); // #113 cas de la recréation du systray
	strcpy_s(nid.szTip,sizeof(nid.szTip),GetString(gbSSOActif ? IDS_ACTIVE : IDS_DESACTIVE)); //max=64 // #113 cas de la recréation du systray

	if (!Shell_NotifyIcon(NIM_ADD,&nid)) 
	{
		TRACE((TRACE_ERROR,_F_,"Shell_NotifyIcon()=0x%08lx",GetLastError())); 
		goto end; 
	}
	TRACE((TRACE_INFO,_F_,"Shell_NotifyIcon() OK"));
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// DestroySystray()
//-----------------------------------------------------------------------------
// Destruction du Systray
//-----------------------------------------------------------------------------
// [in] wMain : handle de la fenêtre technique pour les notifications
//-----------------------------------------------------------------------------
void DestroySystray(HWND wMain)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	NOTIFYICONDATA nid;
	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd=wMain;
	nid.uID=0;
	Shell_NotifyIcon(NIM_DELETE,&nid);
	
	TRACE((TRACE_LEAVE,_F_, ""));
}

