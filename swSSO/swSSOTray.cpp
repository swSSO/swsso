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
// swSSOTray.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions liées au Systray (icône barre de tâches)
//-----------------------------------------------------------------------------

#include "stdafx.h"

unsigned int gMsgTaskbarRestart=0; // message registré pour recevoir les notifs de recréation du systray
int BeginChangeAppPassword(void);

static int giRefreshTimer=10;

char gszNewAppPwd[LEN_PWD+1];

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
	// ISSUE#99 / 0.99B1 : ajout de gbShowMenu_AddThisApp pour dissocier gbShowMenu_AddApp
	// if (gbShowMenu_AddApp) 
	if (gbShowMenu_AddThisApp)
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_THIS_APPLI,GetString(IDS_MENU_THIS_APPLI));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SSO_NOW,GetString(IDS_MENU_SSO_NOW));
	if (gbShowMenu_AppPasswordMenu && giLastApplicationSSO!=-1) // ISSUE#107
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_CHANGEAPPPWD,GetString(IDS_MENU_CHANGEAPPPWD));
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
		//1.01 (ISSUE#140) : sauf si mode réactivation sans saisie de mot de passe
		if (giPwdProtection!=PP_WINDOWS && !gbReactivateWithoutPwd)
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
					if (!gbSSOActif && !gbReactivateWithoutPwd)
						if (AskPwd(NULL,TRUE)!=0) goto end;
					if (HIBYTE(GetAsyncKeyState(VK_SHIFT))!=0) 
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + SHIFT + WM_LBUTTONDBLCLK"));
						ReactivateApplicationFromCurrentWindow();
					}
					else if (HIBYTE(GetAsyncKeyState(VK_CONTROL))!=0) 
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + CTRL + WM_LBUTTONDBLCLK"));
						if (!gbSSOActif && !gbReactivateWithoutPwd)
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
							// ShowAppNsites(-1); ISSUE#108
							ShowAppNsites(giLastApplicationConfig,TRUE);
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
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShowLaunchApp();
					break;
				case TRAY_MENU_ACTIVER:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_ACTIVER"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
						if (AskPwd(NULL,TRUE)!=0) goto end;
					SSOActivate(w);
					break;
				case TRAY_MENU_PROPRIETES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_PROPRIETES"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShowConfig();
					break;
				case TRAY_MENU_APPNSITES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_APPNSITES"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					// ShowAppNsites(-1); ISSUE#108
					ShowAppNsites(giLastApplicationConfig,TRUE);
					break;
				case TRAY_MENU_MDP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_MDP"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					WindowChangeMasterPwd(FALSE);
					break;
				case TRAY_MENU_PORTAL:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_PORTAL"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					ShellExecute(NULL,"open",gszCfgPortal,NULL,"",SW_SHOW );
					break;
				case TRAY_MENU_THIS_APPLI:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_THIS_APPLI"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					AddApplicationFromCurrentWindow();
					break;
				case TRAY_MENU_SSO_NOW:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_SSO_NOW"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
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
				case TRAY_MENU_CHANGEAPPPWD: // ISSUE#107
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_CHANGEAPPPWD"));
					BeginChangeAppPassword();
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

//-----------------------------------------------------------------------------
// PopChangeAppPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la popup de début de procédure de changement de mdp d'une appli
//-----------------------------------------------------------------------------
static int CALLBACK PopChangeAppPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			//gwChangeMasterPwd=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// titre en gras
			SetTextBold(w,TX_APP_NAME);
			SetTextBold(w,TX_ID);
			SetDlgItemText(w,TX_ID,gptActions[giLastApplicationSSO].szId1Value);
			SetDlgItemText(w,TX_APP_NAME,gptActions[giLastApplicationSSO].szApplication);
			// affiche un message différent si la saisie automatique du mot de passe n'a pas été faite ou n'était pas demandée
			SetDlgItemText(w,TX_FRAME2,GetString(lp==1?IDS_MSG_CHANGE_APP_PWD_1:IDS_MSG_CHANGE_APP_PWD_2));
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			// case à cocher
			MACRO_SET_SEPARATOR_90;
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME1:
				case TX_FRAME2:
				case TX_FRAME3:
				case TX_ID:
				case TX_APP_NAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					// L'utilisateur ne veut plus voir le message
					if (IsDlgButtonChecked(w,CK_VIEW))
					{
						gbDisplayChangeAppPwdDialog=FALSE;
						SaveConfigHeader();
					}
					EndDialog(w,IDOK);
					break;
				}
				case IDCANCEL:
				{
					EndDialog(w,IDCANCEL);
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,90,ghLogoFondBlanc90);
			rc=TRUE;
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// DrawLogoAndWarningBar()
//-----------------------------------------------------------------------------
// Affichage d'un bandeau blanc avec logo swSSO et warning 
//-----------------------------------------------------------------------------
void DrawLogoAndWarningBar(HWND w)
{
	TRACE((TRACE_ENTER, _F_, "w=0x%08lx", w));

	PAINTSTRUCT ps;
	RECT rect;
	HDC dc = NULL;

	if (!GetClientRect(w, &rect)) { TRACE((TRACE_ERROR, _F_, "GetClientRect(0x%08lx)=%ld", w, GetLastError()));  goto end; }
	dc = BeginPaint(w, &ps);
	if (dc == NULL) { TRACE((TRACE_ERROR, _F_, "BeginPaint()=%ld", GetLastError())); goto end; }
	if (!BitBlt(dc, 0, 0, rect.right, 140, 0, 0, 0, WHITENESS)) { TRACE((TRACE_ERROR, _F_, "BitBlt(WHITENESS)=%ld", GetLastError())); }
	DrawBitmap(ghLogoFondBlanc90, dc, 0, 0, 60, 80);
	DrawBitmap(ghLogoExclamation, dc, 50, 75, 60, 60);
end:
	if (dc != NULL) EndPaint(w, &ps);
	TRACE((TRACE_LEAVE, _F_, ""));
}

//-----------------------------------------------------------------------------
// SaveNewAppPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de mise en coffre du nouveau mot de passe d'une appli
//-----------------------------------------------------------------------------
static int CALLBACK SaveNewAppPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);

	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			//gwChangeMasterPwd=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation chamsp de saisie
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_PWD_CLEAR),EM_LIMITTEXT,LEN_PWD,0);
			// Avertissement en gras
			SetTextBold(w, TX_FRAME3);
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			MACRO_SET_SEPARATOR_140;
			RevealPasswordField(w,FALSE);
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME1:
				case TX_FRAME2:
				case TX_FRAME3:
					SetBkMode((HDC)wp, TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				case IDCANCEL:
				{
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gszNewAppPwd,LEN_PWD+1);
					EndDialog(w,LOWORD(wp));
					break;
				}
				case CK_VIEW:
				{
					RevealPasswordField(w,IsDlgButtonChecked(w,CK_VIEW));
					break;
				}
				case IDB_COPIER:
				{
					char szPwd[LEN_PWD+1];
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,szPwd,LEN_PWD+1);
					ClipboardCopy(szPwd);
					SecureZeroMemory(szPwd,LEN_PWD+1);
					break;
				}
				case TB_PWD:
				case TB_PWD_CLEAR:
				{
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szPwd[LEN_PWD+1];
						int len;
						len=GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,szPwd,LEN_PWD+1);
						EnableWindow(GetDlgItem(w,IDOK),len==0 ? FALSE : TRUE);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoAndWarningBar(w);
			rc = TRUE;
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// ConfirmNewAppPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la popup de confirmation d'enregistrement du nouveau mdp d'une appli
//-----------------------------------------------------------------------------
static int CALLBACK ConfirmNewAppPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);

	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			char szMessage[512];
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// ISSUE#144
			wsprintf(szMessage,GetString(IDS_MSG_CONFIRM_PWD_SAVE),gptActions[giLastApplicationSSO].szApplication);
			SetDlgItemText(w,TX_FRAME1,szMessage);
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				case IDCANCEL:
				{
					EndDialog(w,LOWORD(wp));
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// BeginChangeAppPassword()
//-----------------------------------------------------------------------------
// Changement du mot de passe d'une application
//-----------------------------------------------------------------------------
int BeginChangeAppPassword(void)
{
	TRACE((TRACE_ENTER,_F_, "giLastApplicationSSO=%d",giLastApplicationSSO));
	int rc=-1;
	BOOL bDone=FALSE;
	char *pszEncryptedPassword=NULL;
	BOOL bOldPwdFillDone=0;
	
	if (giLastApplicationSSO==-1) goto end; // ne peut pas se produire en théorie puisque le menu systray n'est affiché que si !=1

	// déchiffre le mot de passe de l'application, essaie de le saisir et le copie dans le presse papier
	if ((*gptActions[giLastApplicationSSO].szPwdEncryptedValue!=0))
	{
		char *pszPassword=swCryptDecryptString(gptActions[giLastApplicationSSO].szPwdEncryptedValue,ghKey1);
		if (pszPassword!=NULL) 
		{
			// copie le mot de passedans le presse-papier
			ClipboardCopy(pszPassword);
			// si remplissage de l'ancien mot de passe demandé et que la fenêtre est toujours présente
			if (gbOldPwdAutoFill && gptActions[giLastApplicationSSO].wLastSSO!=NULL && IsWindow(gptActions[giLastApplicationSSO].wLastSSO))
			{
				// mise au premier plan de la fenêtre
				SetForegroundWindow(gptActions[giLastApplicationSSO].wLastSSO);
				// saisie à l'aveugle de l'ancien mot de passe, en espérant que ce soit le champ avec le focus...
				TRACE((TRACE_INFO,_F_,"Fenetre 0x%08lx tjs presente, saisie de l'ancien mdp",gptActions[giLastApplicationSSO].wLastSSO));
				KBSim(FALSE,200,pszPassword,TRUE);	
				bOldPwdFillDone=1;
			}
			SecureZeroMemory(pszPassword,strlen(pszPassword));
			free(pszPassword);
		}
	}
	
	// 1ère popup : informe l'utilisateur que son mot de passe a été copié dans le presse papier
	if (gbDisplayChangeAppPwdDialog)
	{
		if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_POP_CHANGE_APP_PWD),NULL,PopChangeAppPwdDialogProc,bOldPwdFillDone)==IDCANCEL) 
		{
			TRACE((TRACE_INFO,_F_,"Annulation par l'utilisateur"));
			goto end;
		}
	}

	while (!bDone)
	{
		// 2ème popup : demande à l'utilisateur de saisir son nouveau mot de passe et lui permet de le copier dans le presse papier
		if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_SAVE_NEW_APP_PWD),NULL,SaveNewAppPwdDialogProc)==IDOK)
		{
			// 3ème popup : demande à l'utilisateur de confirmer l'enregistrement du nouveau mot de passe
			if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_CONFIRM_NEW_APP_PWD),NULL,ConfirmNewAppPwdDialogProc)==IDOK)
			{
				// l'utilisateur a confirmé l'enregistrement, on chiffre le mot de passe et on l'enregistre
				pszEncryptedPassword=swCryptEncryptString(gszNewAppPwd,ghKey1);
				SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
				if (pszEncryptedPassword==NULL) goto end;
				strcpy_s(gptActions[giLastApplicationSSO].szPwdEncryptedValue,sizeof(gptActions[giLastApplicationSSO].szPwdEncryptedValue),pszEncryptedPassword);
				// sauvegarde
				SaveApplications();
				// log
				swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_CHANGE_APP_PWD,gptActions[giLastApplicationSSO].szApplication,gptActions[giLastApplicationSSO].szId1Value,NULL,0);
				// fini !
				bDone=TRUE;
				rc=0;
			}
			else
			{
				// l'utilisateur a annulé l'enregistrement, retour à la 2ème popup
			}
		}
		else
		{
			// l'utilisateur a annulé dans la fenêtre de saisie du nouveau mot de passe, on sort
			bDone=TRUE;
		}
	}
	// changement de mot de passe terminé (ou abandonné) : vidage du presse papier
	ClipboardDelete();

end:
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}