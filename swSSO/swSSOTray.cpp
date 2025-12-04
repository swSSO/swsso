//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2026 - Sylvain WERDEFROY
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
// swSSOTray.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions liées au Systray (icône barre de tâches)
//-----------------------------------------------------------------------------

#include "stdafx.h"

unsigned int gMsgTaskbarRestart=0; // message registré pour recevoir les notifs de recréation du systray
int BeginChangeAppPassword(void);
int SignUpForThisSite(void);

static int giRefreshTimer=10;

char gszNewAppPwd[LEN_PWD+1];
char gszNewAppId[LEN_ID+1];
#define TB_PWD_SUBCLASS_ID 1
#define TB_PWD_CLEAR_SUBCLASS_ID 2
#define TB_ID_SUBCLASS_ID 3
static BOOL gbPwdSubClass=FALSE;
static BOOL gbPwdClearSubClass=FALSE;
static BOOL gbIdSubClass=FALSE;

char* gpszClipboardPassword = NULL; // pour la fonction d'assistance au changement de mot de passe
char* gpszPasteId = NULL; // pour le copier-coller depuis la treewiew de la fenêtre de gestion des sites
int giPasteIdOrPassword=0; // 0=Id, 1=Pwd
int giActionPasteIdOrPassword=-1; // id de l'action dont on va copier l'id ou le pwd


HWND gwPopChangeAppPwdDialogProc=NULL;
HWND gwSaveNewAppPwdDialogProc=NULL;
HWND gwConfirmNewAppPwdDialogProc=NULL;
HWND gwSignUp=NULL;

HWND gwBrowser;

typedef struct
{
	HWND wBrowser;
	char *pszWindowTitle;
}
T_SIGNUP_WINDOW;

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
	if (gbShowMenu_AskThisApp)
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_ASK_THIS_APP,GetString(IDS_MENU_ASK_THIS_APP));
	}
	if (!gbAdmin) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SSO_NOW,GetString(IDS_MENU_SSO_NOW));
	if (!gbAdmin && gbShowMenu_AppPasswordMenu && giLastApplicationSSO!=-1) // ISSUE#107
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_CHANGEAPPPWD,GetString(IDS_MENU_CHANGEAPPPWD));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	if (gbShowMenu_SignUp)
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SIGNUP,GetString(IDS_MENU_SIGNUP));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_APPNSITES,GetString(IDS_MENU_APPNSITES));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_PROPRIETES,GetString(IDS_MENU_PROP));
	if (gbShowMenu_Help) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_AIDE,GetString(IDS_MENU_AIDE)); // ISSUE#306
	
	// ISSUE#292
	if (gbAdmin) // en mode admin, pas de menu "Mot de passe Windows" ni de mot de passe "Portail", qque soit la config
	{
		gbUseADPasswordForAppLogin=FALSE;
		*gszCfgPortal=0;
	}
	// if (!gbAdmin) // ISSUE#292 : on n'interdit pas de changer le mot de passe maitre 
	{
		if ((giPwdProtection==PP_ENCRYPTED && !gbNoMasterPwd) || *gszCfgPortal!=0 || gbUseADPasswordForAppLogin) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
		if (giPwdProtection==PP_ENCRYPTED && !gbNoMasterPwd) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_MDP,GetString(IDS_MENU_MDP));
		if (gbUseADPasswordForAppLogin) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_MDP_WINDOWS,GetString(IDS_MENU_MDP_WINDOWS));
		// 1.22 : plus de menu portail 
		// if (*gszCfgPortal!=0) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_PORTAL,GetString(IDS_MENU_PORTAL));
	}
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	if (gbShowMenu_RefreshRights) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_REFRESH_RIGHTS,GetString(IDS_MENU_REFRESH_RIGHTS));
	if (gbSSOActif)
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_ACTIVER,GetString(IDS_MENU_DESACTIVER));
	else
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_ACTIVER,GetString(IDS_MENU_ACTIVER));
	if (gbShowMenu_Quit) // ISSUE#257
	{
		InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_QUITTER,GetString(IDS_MENU_QUITTER));
	}
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
		strcpy_s(nid.szTip,sizeof(nid.szTip),gbAdmin?GetString(IDS_SYSTRAY_ADMIN):GetString(IDS_ACTIVE)); //max64
		if (gwPropertySheet!=NULL) ShowWindow(gwPropertySheet,SW_SHOW);
		if (gwAppNsites!=NULL) ShowWindow(gwAppNsites,SW_SHOW);
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_UNLOCK_SUCCESS,NULL,NULL,NULL,NULL,0);
	}
	else // DESACTIVATION
	{
		// ISSUE#258 : en mode de fonctionnement sans mot de passe, demande le mot de passe admin
		if (gbNoMasterPwd && gcszK1[0]!='1' && !gbAdmin)
		{
			if (AskAdminPwd()!=0) { gbSSOActif=!gbSSOActif ; goto end; }
		}
		
		nid.hIcon=ghIconSystrayInactive;
		strcpy_s(nid.szTip,sizeof(nid.szTip),gbAdmin?GetString(IDS_SYSTRAY_ADMIN):GetString(IDS_DESACTIVE)); //max64
		if (gwPropertySheet!=NULL) ShowWindow(gwPropertySheet,SW_HIDE);
		if (gwAppNsites!=NULL) ShowWindow(gwAppNsites,SW_HIDE);
		if (gwAskPwd!=NULL) { EndDialog(gwAskPwd,IDCANCEL); gwAskPwd=NULL; }
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_LOCK,NULL,NULL,NULL,NULL,0);
	}
	rc=Shell_NotifyIcon(NIM_MODIFY,&nid);
	if (!rc) 
	{
		TRACE((TRACE_ERROR,_F_,"Shell_NotifyIcon[hIcon=0x%08lx szTip='%s']",nid.hIcon,nid.szTip));
	}
end:
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
			if (gLastLoginTime.wYear==0) goto end; // utilisateur pas encore connecté (contrôle nécessaire suite à ISSUE#175)
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
						if (gbShowLaunchAppWithoutCtrl)
							ShowAppNsites(giLastApplicationConfig,TRUE);	
						else
						{
							if (gbShowMenu_LaunchApp) ShowLaunchApp(); // ISSUE#289
						}
					}
					else
					{
						TRACE((TRACE_INFO,_F_, "WM_APP + WM_LBUTTONDBLCLK"));
						// 0.63B3 : le double click déverrouille si verrouillé, ouvre la config sinon
						if (gbSSOActif)
						{
							// ShowAppNsites(-1); ISSUE#108
							if (gbShowLaunchAppWithoutCtrl)
							{
								if (gbShowMenu_LaunchApp) ShowLaunchApp(); // ISSUE#289
							}
							else
								ShowAppNsites(giLastApplicationConfig,TRUE);
						}
						else
							SSOActivate(w);
					}
					if (HIBYTE(GetAsyncKeyState(VK_SHIFT))!=0 && HIBYTE(GetAsyncKeyState(VK_CONTROL))!=0) AddApplicationFromCurrentWindow(FALSE);
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
				case TRAY_MENU_AIDE: // ISSUE#306
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_AIDE"));
					Help();
					break;
				case TRAY_MENU_REFRESH_RIGHTS:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_REFRESH_RIGHTS"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					RefreshRights(TRUE,TRUE);
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
				case TRAY_MENU_MDP_WINDOWS:
					char szLastADPwdChange2[50+1];
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_MDP_WINDOWS"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					if (AskADPwd(FALSE)==0)
					{
						*szLastADPwdChange2=0;
						if (GetLastADPwdChange2(szLastADPwdChange2,sizeof(szLastADPwdChange2))==0) 
						{
							TRACE((TRACE_INFO,_F_,"lastADPwdChange2 dans l'AD    : %s",szLastADPwdChange2));
							strcpy_s(gszLastADPwdChange2,sizeof(gszLastADPwdChange2),szLastADPwdChange2);
						} 
						else // si AD non dispo, pas grave, on verra la prochaine fois
						{
							TRACE((TRACE_ERROR,_F_,"Impossible de récupérer la date de dernier changement de mdp dans l'AD"));
						}
						SaveConfigHeader();
					}
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
					// ISSUE#149
					if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
					AddApplicationFromCurrentWindow(FALSE);
					break;
				case TRAY_MENU_ASK_THIS_APP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_ASK_THIS_APP"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					AddApplicationFromCurrentWindow(TRUE);
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
					if (gwPopChangeAppPwdDialogProc!=NULL)
						SetForegroundWindow(gwPopChangeAppPwdDialogProc);
					else if (gwSaveNewAppPwdDialogProc!=NULL)
						SetForegroundWindow(gwSaveNewAppPwdDialogProc);
					else if (gwConfirmNewAppPwdDialogProc)
						SetForegroundWindow(gwConfirmNewAppPwdDialogProc);
					else 
						BeginChangeAppPassword();
					break;
				case TRAY_MENU_SIGNUP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_SIGNUP"));
					if (!gbSSOActif && !gbReactivateWithoutPwd)
					{
						if (AskPwd(NULL,TRUE)!=0) goto end;
						SSOActivate(w);
					}
					SignUpForThisSite();
					break;
				case TRAY_MENU_QUITTER:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_QUITTER"));
					// ISSUE#257 : en mode de fonctionnement sans mot de passe, demande le mot de passe admin
					if (gbNoMasterPwd && gcszK1[0]!='1' && !gbAdmin)
					{
						if (AskAdminPwd()!=0) goto end;
					}
					gbWillTerminate=TRUE;
					if (gwMessageBox3B!=NULL) { EndDialog(gwMessageBox3B,-1); gwMessageBox3B=NULL; }
					if (gwChooseConfig!=NULL) { EndDialog(gwChooseConfig,-1); gwChooseConfig=NULL; }
					if (gwIdAndPwdDialogProc!=NULL) { EndDialog(gwIdAndPwdDialogProc,-1); gwIdAndPwdDialogProc=NULL; }
					if (gwChangeApplicationPassword!=NULL) { EndDialog(gwChangeApplicationPassword,-1); gwChangeApplicationPassword=NULL; }
					if (gwSelectAccount!=NULL) { EndDialog(gwSelectAccount,-1); gwSelectAccount=NULL; }
					PostQuitMessage(0);
					break;
				case TRAY_PASTE_PASSWORD:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_PASTE_PASSWORD gpszClipboardPassword=0x%08lx",gpszClipboardPassword));
					if (gpszClipboardPassword!=NULL) KBSim(NULL,FALSE,0,gpszClipboardPassword,TRUE);
					break;
				case TRAY_PASTE_IDORPASSWORD:
					if (giPasteIdOrPassword!=0 && giActionPasteIdOrPassword!=-1) // ISSUE#410
					{ 
						char *pszPasteIdOrPassword=NULL;
						UninstallHotKey(); 
						if (giPasteIdOrPassword==1) // colle l'id
						{
							pszPasteIdOrPassword=(char*)malloc(LEN_ID+1);
							if (pszPasteIdOrPassword!=NULL) strcpy_s(pszPasteIdOrPassword,LEN_ID+1,gptActions[giActionPasteIdOrPassword].szId1Value);
						}
						else if (giPasteIdOrPassword==2) // colle le pwd
						{
							pszPasteIdOrPassword=swCryptDecryptString(gptActions[giActionPasteIdOrPassword].szPwdEncryptedValue,ghKey1);
						}
						if (pszPasteIdOrPassword!=NULL)
						{
							KBSim(NULL,FALSE,0,pszPasteIdOrPassword,TRUE);
							SecureZeroMemory(pszPasteIdOrPassword,strlen(pszPasteIdOrPassword));
							free(pszPasteIdOrPassword);
							giActionPasteIdOrPassword=-1;
						}
					}
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
		case WM_QUERYENDSESSION:
			TRACE((TRACE_INFO,_F_,"WM_QUERYENDSESSION"));
			if (giStat!=0 && !gbAdmin) swStat();
			return TRUE;
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
	if (gbAdmin)
	{
		strcpy_s(nid.szTip,sizeof(nid.szTip),GetString(IDS_SYSTRAY_ADMIN));
	}
	else
	{
		strcpy_s(nid.szTip,sizeof(nid.szTip),GetString(gbSSOActif ? IDS_ACTIVE : IDS_DESACTIVE)); //max=64 // #113 cas de la recréation du systray
	}

	// ISSUE#337
	if (gbShowSystrayIcon)
	{
		if (!Shell_NotifyIcon(NIM_ADD,&nid)) 
		{
			TRACE((TRACE_ERROR,_F_,"Shell_NotifyIcon()=0x%08lx",GetLastError())); 
			goto end; 
		}
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
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			char szMsg[1024];
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
			sprintf_s(szMsg,sizeof(szMsg),GetString(lp==1?IDS_MSG_CHANGE_APP_PWD_1:IDS_MSG_CHANGE_APP_PWD_2),gszPastePwd_Text);
			SetDlgItemText(w,TX_FRAME2,szMsg);
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
			gwPopChangeAppPwdDialogProc=w;
			break;
		}
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
					// L'utilisateur ne veut plus voir le message
					if (IsDlgButtonChecked(w,CK_VIEW))
					{
						gbDisplayChangeAppPwdDialog=FALSE;
						SaveConfigHeader();
					}
					EndDialog(w,IDOK);
					break;
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
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
		case WM_DESTROY:
			gwPopChangeAppPwdDialogProc=NULL;
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
	DrawBitmap(ghLogoExclamation, dc, 50, 75, 60, 60);
	DrawBitmap(ghLogoFondBlanc90, dc, 0, 0, 60, 80);
end:
	if (dc != NULL) EndPaint(w, &ps);
	TRACE((TRACE_LEAVE, _F_, ""));
}

//-----------------------------------------------------------------------------
// DrawQuestionBar()
//-----------------------------------------------------------------------------
// Affichage d'un bandeau blanc avec icone question
//-----------------------------------------------------------------------------
void DrawQuestionBar(HWND w)
{
	TRACE((TRACE_ENTER, _F_, "w=0x%08lx", w));

	PAINTSTRUCT ps;
	RECT rect;
	HDC dc = NULL;

	if (!GetClientRect(w, &rect)) { TRACE((TRACE_ERROR, _F_, "GetClientRect(0x%08lx)=%ld", w, GetLastError()));  goto end; }
	dc = BeginPaint(w, &ps);
	if (dc == NULL) { TRACE((TRACE_ERROR, _F_, "BeginPaint()=%ld", GetLastError())); goto end; }
	if (!BitBlt(dc, 0, 0, rect.right, 75, 0, 0, 0, WHITENESS)) { TRACE((TRACE_ERROR, _F_, "BitBlt(WHITENESS)=%ld", GetLastError())); }
	DrawBitmap(ghLogoQuestion, dc, 5, 15, 42, 42);
end:
	if (dc != NULL) EndPaint(w, &ps);
	TRACE((TRACE_LEAVE, _F_, ""));
}

HINSTANCE ghinstHotKeyDLL=NULL; 
HHOOK ghHookKeyboard=NULL;

//-----------------------------------------------------------------------------
// InstallHotKey()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int InstallHotKey(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HOOKPROC hookproc=NULL;

	ghinstHotKeyDLL=LoadLibrary("swSSOHotKey.dll"); 
	if (ghinstHotKeyDLL==NULL)  { TRACE((TRACE_ERROR, _F_, "LoadLibrary(swSSOHotKey.dll)", GetLastError())); goto end; }

	hookproc=(HOOKPROC)GetProcAddress(ghinstHotKeyDLL,"KeyboardProc"); 
	if (hookproc==NULL) { TRACE((TRACE_ERROR, _F_, "GetProcAddress(KeyboardProc)", GetLastError())); goto end; }

	ghHookKeyboard=SetWindowsHookEx(WH_KEYBOARD,hookproc,ghinstHotKeyDLL,0);
	if (ghHookKeyboard==NULL) { TRACE((TRACE_ERROR, _F_, "SetWindowsHookEx(WH_KEYBOARD)", GetLastError())); goto end; }

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// UninstallHotKey()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int UninstallHotKey(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	if (ghHookKeyboard!=NULL) { UnhookWindowsHookEx(ghHookKeyboard); ghHookKeyboard=NULL; }

	if (ghinstHotKeyDLL!=NULL) { FreeLibrary(ghinstHotKeyDLL); ghinstHotKeyDLL=NULL; }

	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SaveNewAppPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de mise en coffre du nouveau mot de passe d'une appli
//-----------------------------------------------------------------------------
static int CALLBACK SaveNewAppPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			char szMsg[1024];
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			//gwChangeMasterPwd=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation champs de saisie
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_PWD_CLEAR),EM_LIMITTEXT,LEN_PWD,0);
			// personnalise le message avec la bonne combinaison de touche
			sprintf_s(szMsg,sizeof(szMsg),GetString(IDS_MSG_CHANGE_APP_PWD_3),gszPastePwd_Text);
			SetDlgItemText(w,TX_FRAME2,szMsg);
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
			gwSaveNewAppPwdDialogProc=w;
			break;
		}
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
					gpszClipboardPassword=(char*)malloc(LEN_PWD+1);
					if (gpszClipboardPassword!=NULL)
					{
						GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gpszClipboardPassword,LEN_PWD+1);
					}
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
						SecureZeroMemory(szPwd,LEN_PWD+1);
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
		case WM_DESTROY:
			gwSaveNewAppPwdDialogProc=NULL;
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
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// ISSUE#144
			SetDlgItemText(w,TX_FRAME2,gptActions[giLastApplicationSSO].szApplication);
			SetTextBold(w,TX_FRAME2);
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			MACRO_SET_SEPARATOR_75;
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			gwConfirmNewAppPwdDialogProc=w;
			break;
		}
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
					EndDialog(w,LOWORD(wp));
					break;
				}
			}
			break;
		case WM_PAINT:
			DrawQuestionBar(w);
			rc = TRUE;
			break;
		case WM_HELP:
			Help();
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
		case WM_DESTROY:
			gwConfirmNewAppPwdDialogProc=NULL;
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
	int ret;
		
	if (giLastApplicationSSO==-1) goto end; // ne peut pas se produire en théorie puisque le menu systray n'est affiché que si !=1

	if (AskPwd(NULL,FALSE)!=0) goto end;

	if (InstallHotKey()!=0) goto end;

	// déchiffre le mot de passe de l'application, essaie de le saisir (si configuré) et le garde en mémoire pour que l'utilisateur puisse le coller
	if ((*gptActions[giLastApplicationSSO].szPwdEncryptedValue!=0))
	{
		gpszClipboardPassword=swCryptDecryptString(gptActions[giLastApplicationSSO].szPwdEncryptedValue,ghKey1);
		if (gpszClipboardPassword!=NULL) 
		{
			// si remplissage de l'ancien mot de passe demandé et que la fenêtre est toujours présente
			if (gbOldPwdAutoFill && gptActions[giLastApplicationSSO].wLastSSO!=NULL && IsWindow(gptActions[giLastApplicationSSO].wLastSSO))
			{
				// mise au premier plan de la fenêtre
				SetForegroundWindow(gptActions[giLastApplicationSSO].wLastSSO);
				// saisie à l'aveugle de l'ancien mot de passe, en espérant que ce soit le champ avec le focus...
				TRACE((TRACE_INFO,_F_,"Fenetre 0x%08lx tjs presente, saisie de l'ancien mdp",gptActions[giLastApplicationSSO].wLastSSO));
				KBSim(gptActions[giLastApplicationSSO].wLastSSO,FALSE,200,gpszClipboardPassword,TRUE);	
				bOldPwdFillDone=1;
			}
		}
	}
	
	// 1ère popup : informe l'utilisateur que son mot de passe a été copié dans le presse papier
	if (gbDisplayChangeAppPwdDialog)
	{
		ret=DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_POP_CHANGE_APP_PWD),NULL,PopChangeAppPwdDialogProc,bOldPwdFillDone);
		if (ret==IDCANCEL)
		{
			TRACE((TRACE_INFO,_F_,"Annulation par l'utilisateur"));
			goto end;
		}
	}
	while (!bDone)
	{
		// 2ème popup : demande à l'utilisateur de saisir son nouveau mot de passe et lui permet de le copier dans le presse papier
		if ((DialogBox(ghInstance,MAKEINTRESOURCE(IDD_SAVE_NEW_APP_PWD),NULL,SaveNewAppPwdDialogProc)==IDOK) && !gbWillTerminate) // ISSUE#196
		{
			// 3ème popup : demande à l'utilisateur de confirmer l'enregistrement du nouveau mot de passe
			if ((DialogBox(ghInstance,MAKEINTRESOURCE(IDD_CONFIRM_NEW_APP_PWD),NULL,ConfirmNewAppPwdDialogProc)==IDOK) && !gbWillTerminate)
			{
				// L'utilisateur a confirmé l'enregistrement, on chiffre le mot de passe et on l'enregistre
				// ISSUE#156 : change le mot de passe de toutes les applications du groupe (reconnues par le pwdGroup!=-1)
				// commence par changer le mot de passe de l'application, comme en v1.02
				pszEncryptedPassword=swCryptEncryptString(gszNewAppPwd,ghKey1);
				if (pszEncryptedPassword==NULL) goto end;
				strcpy_s(gptActions[giLastApplicationSSO].szPwdEncryptedValue,sizeof(gptActions[giLastApplicationSSO].szPwdEncryptedValue),pszEncryptedPassword);
				free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
				pszEncryptedPassword=NULL;
				if (gptActions[giLastApplicationSSO].iPwdGroup!=-1) // change les autres applis
				{
					int i;
					for (i=0;i<giNbActions;i++)
					{
						if ((gptActions[i].iPwdGroup==gptActions[giLastApplicationSSO].iPwdGroup) &&
							(*gptActions[i].szId1Value!=0) && (*gptActions[giLastApplicationSSO].szId1Value!=0) && // nouvelle condition ISSUE#235
							(_stricmp(gptActions[i].szId1Value,gptActions[giLastApplicationSSO].szId1Value)==0))    // nouvelle condition ISSUE#235
						{
							pszEncryptedPassword=swCryptEncryptString(gszNewAppPwd,ghKey1);
							if (pszEncryptedPassword==NULL) goto end;
							strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedPassword);
							free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
							pszEncryptedPassword=NULL;
						}
					}
				}
				// sauvegarde
				SaveApplications();
				if (gwAppNsites!=NULL) ShowAppNsites(giLastApplicationConfig,FALSE); // ISSUE#192
				// log
				swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_CHANGE_APP_PWD,gptActions[giLastApplicationSSO].szApplication,gptActions[giLastApplicationSSO].szId1Value,NULL,NULL,0);
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
	gwPopChangeAppPwdDialogProc=NULL;
	gwSaveNewAppPwdDialogProc=NULL;
	gwConfirmNewAppPwdDialogProc=NULL;
	if (gpszClipboardPassword!=NULL) { SecureZeroMemory(gpszClipboardPassword,strlen(gpszClipboardPassword)); free(gpszClipboardPassword); gpszClipboardPassword=NULL; }
	UninstallHotKey();
	SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RefreshRights()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int RefreshRights(BOOL bForced,BOOL bReportSync)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;

	if (gwAppNsites!=NULL)
	{
		if (bReportSync) MessageBox(NULL,GetString(IDS_CLOSE_APPNSITES_FIRST),"swSSO",MB_ICONEXCLAMATION | MB_OK);
		goto end;
	}
	LoadPolicies();
	GetConfigHeader();
	
	gtConfigSync.iNbConfigsAdded=0;
	gtConfigSync.iNbConfigsDeleted=0;
	gtConfigSync.iNbConfigsDisabled=0;
	gtConfigSync.iNbConfigsModified=0;

	if (gbInternetManualPutConfig) 
	{
		giNbDomains=GetAllDomains(gtabDomains);
	}
	else
	{
		// Il faut aussi récupérer la liste des domaines pour renseigner le label du domaine de l'utilisateur
		// (s'il est vide et que le domaineId est différent de -1=tous ou 1=commun)
		if (*gszDomainLabel==0)
		{
			if (giDomainId!=-1 && giDomainId!=1)
			{
				giNbDomains=GetAllDomains(gtabDomains);
			}
			GetDomainLabel(giDomainId); 
			SaveConfigHeader();
		}
	}
	if (gbGetNewConfigsAtStart || gbGetModifiedConfigsAtStart)
	{
		rc=GetNewOrModifiedConfigsFromServer(bForced);
	}
	if (gbRemoveDeletedConfigsAtStart) // Supprime les configs qui ne sont plus présentes sur le serveur
	{
		rc=DeleteConfigsNotOnServer();
	}
	
	if (bReportSync)
	{
		if (rc==0)
		{
			ReportConfigSync(0,TRUE,TRUE);
		}
		else if (!gbDisplayConfigsNotifications) // si gbDisplayConfigsNotifications, un message d'erreur aura été affiché avant
		{
			ReportConfigSync(IDS_REFRESH_RIGHTS_ERROR,TRUE,TRUE);
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SignUpDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre d'inscription sur un site
//-----------------------------------------------------------------------------
static int CALLBACK SignUpDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			char szPwd[LEN_PWD+1];
			SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SetWindowText(w,((T_SIGNUP_WINDOW*)lp)->pszWindowTitle);
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation champs de saisie
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_PWD_CLEAR),EM_LIMITTEXT,LEN_PWD,0);
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			MACRO_SET_SEPARATOR_90;
			RevealPasswordField(w,FALSE);
			gSubClassData.gw=w;
			gbPwdSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
			gbPwdClearSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
			gbIdSubClass=SetWindowSubclass(GetDlgItem(w,TB_ID),(SUBCLASSPROC)IdProc,TB_ID_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
	
			swGenerateRandomPwd(szPwd,12,PWDTYPE_ALPHA|PWDTYPE_NUM|PWDTYPE_SPECIALCHARS);
			ShowWindow(GetDlgItem(w,TB_PWD),SW_HIDE);
			ShowWindow(GetDlgItem(w,TB_PWD_CLEAR),SW_SHOW);			
			SetDlgItemText(w,TB_PWD_CLEAR,szPwd);
			SecureZeroMemory(szPwd,sizeof(szPwd));
			CheckDlgButton(w,CK_VIEW,BST_CHECKED);

			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			gwSignUp=w;
			break;
		}
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
					SetBkMode((HDC)wp, TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					GetDlgItemText(w,TB_ID,gszNewAppId,LEN_ID+1);
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gszNewAppPwd,LEN_PWD+1);
					EndDialog(w,LOWORD(wp));
					break;
				}
				case IDCANCEL:
				{
					EndDialog(w,LOWORD(wp));
					break;
				}
				case CK_VIEW:
				{
					RevealPasswordField(w,IsDlgButtonChecked(w,CK_VIEW));
					break;
				}
				case PB_COPY_ID:
				{
					GetDlgItemText(w,TB_ID,gszNewAppId,LEN_ID+1);
					ClipboardCopy(gszNewAppId);
					break;
				}
				case PB_COPY_PWD:
				{
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gszNewAppPwd,LEN_PWD+1);
					if (BrowserFillPasswords(gwBrowser,gszNewAppPwd)!=0)
					{
						MessageBox(w,GetString(IDS_FILL_PWD_ERROR),"swSSO",MB_OK | MB_ICONSTOP);
						ClipboardCopy(gszNewAppPwd); // ajouter un message d'erreur qui dit que le mdp est dans le presse papier...
					}
					SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
					break;
				}
				case TB_ID:
				case TB_PWD:
				case TB_PWD_CLEAR:
				{
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szId[LEN_PWD+1];
						int lenId,lenPwd;
						char szPwd[LEN_PWD+1];
						lenId=GetDlgItemText(w,TB_ID,szId,LEN_ID+1);
						lenPwd=GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,szPwd,LEN_PWD+1);
						SecureZeroMemory(szPwd,LEN_PWD+1);
						EnableWindow(GetDlgItem(w,IDOK),(lenPwd==0 || lenId==0) ? FALSE : TRUE);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,90,ghLogoFondBlanc90);
			rc = TRUE;
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
		case WM_DESTROY:
			if (gbPwdSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID);
			if (gbPwdClearSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID);
			if (gbIdSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)IdProc,TB_ID_SUBCLASS_ID);
			gwSignUp=NULL;
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// SignUpForThisSite()
//-----------------------------------------------------------------------------
// Accompagnement à l'inscription sur un site
//-----------------------------------------------------------------------------
int SignUpForThisSite(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char *pszEncryptedPassword=NULL;
	char *pszFQDN=NULL;
	char szAppName[LEN_APPLICATION_NAME+1];
	char szTitle[512+1];
	int lenFQDN;
	char *pszURL=NULL;
	T_SIGNUP_WINDOW tSignupWindow;

	pszURL=swGetTopWindowWithURL(&gwBrowser,szTitle,sizeof(szTitle));
	if (pszURL==NULL) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	free(pszURL);pszURL=NULL;

	pszFQDN=UniversalGetFQDN(gwBrowser);
	if (pszFQDN==NULL || *pszFQDN==0) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	
	lenFQDN=strlen(pszFQDN);
	tSignupWindow.pszWindowTitle=(char*)malloc(lenFQDN+50);
	if (tSignupWindow.pszWindowTitle==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenFQDN+50)); goto end; }
	sprintf_s(tSignupWindow.pszWindowTitle,lenFQDN+50,"%s %s",GetString(IDS_TITLE_SIGNUP),pszFQDN);
	
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_SIGNUP),NULL,SignUpDialogProc,(LPARAM)&tSignupWindow)==IDOK)
	{
		if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
		ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));
		gptActions[giNbActions].tLastSSO=-1;
		gptActions[giNbActions].wLastSSO=NULL;
		gptActions[giNbActions].iWaitFor=giWaitBeforeNewSSO;
		gptActions[giNbActions].bActive=FALSE;
		gptActions[giNbActions].bAutoLock=TRUE;
		gptActions[giNbActions].bConfigSent=FALSE;
		gptActions[giNbActions].iPwdGroup=-1;
		gptActions[giNbActions].bSafe=TRUE;
		gptActions[giNbActions].iCategoryId=0;
		strcpy_s(gptActions[giNbActions].szURL,pszFQDN);
		pszFQDN[LEN_APPLICATION_NAME]=0; // tronque le FQDN au cas où
		strcpy_s(szAppName,LEN_APPLICATION_NAME+1,pszFQDN);
		GenerateApplicationName(giNbActions,szAppName);
		strcpy_s(gptActions[giNbActions].szId1Value,LEN_ID+1,gszNewAppId);
		pszEncryptedPassword=swCryptEncryptString(gszNewAppPwd,ghKey1);
		if (pszEncryptedPassword==NULL) goto end;
		strcpy_s(gptActions[giNbActions].szPwdEncryptedValue,sizeof(gptActions[giNbActions].szPwdEncryptedValue),pszEncryptedPassword);
		free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
		pszEncryptedPassword=NULL;
		giNbActions++;
		SaveApplications();
	}
end:
	if (pszFQDN==NULL) free(pszFQDN);
	ClipboardDelete();
	SecureZeroMemory(gszNewAppId,LEN_ID+1);
	SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

#if 0
//-----------------------------------------------------------------------------
// SignUpDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre d'inscription sur un site
//-----------------------------------------------------------------------------
static int CALLBACK SignUpDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SetWindowText(w,(char*)lp);
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation champs de saisie
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_PWD_CLEAR),EM_LIMITTEXT,LEN_PWD,0);
			// positionnement en bas à droite de l'écran, près de l'icone swSSO
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
			GetWindowRect(w,&rect);
			SetWindowPos(w,NULL,cx-(rect.right-rect.left)-100,cy-(rect.bottom-rect.top)-100,0,0,SWP_NOSIZE | SWP_NOZORDER);
			MACRO_SET_SEPARATOR_90;
			RevealPasswordField(w,FALSE);
			gSubClassData.gw=w;
			gbPwdSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
			gbPwdClearSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
			gbIdSubClass=SetWindowSubclass(GetDlgItem(w,TB_ID),(SUBCLASSPROC)IdProc,TB_ID_SUBCLASS_ID,(DWORD_PTR)&gSubClassData);
			
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			gwSignUp=w;
			break;
		}
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
					SetBkMode((HDC)wp, TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					GetDlgItemText(w,TB_ID,gszNewAppId,LEN_ID+1);
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gszNewAppPwd,LEN_PWD+1);
					EndDialog(w,LOWORD(wp));
					break;
				}
				case IDCANCEL:
				{
					EndDialog(w,LOWORD(wp));
					break;
				}
				case CK_VIEW:
				{
					RevealPasswordField(w,IsDlgButtonChecked(w,CK_VIEW));
					break;
				}
				case PB_COPY_ID:
				{
					GetDlgItemText(w,TB_ID,gszNewAppId,LEN_ID+1);
					ClipboardCopy(gszNewAppId);
					break;
				}
				case PB_COPY_PWD:
				{
					GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,gszNewAppPwd,LEN_PWD+1);
					if (BrowserFillPasswords(gwBrowser,gszNewAppPwd)!=0)
					{
						MessageBox(w,GetString(IDS_FILL_PWD_ERROR),"swSSO",MB_OK | MB_ICONSTOP);
						ClipboardCopy(gszNewAppPwd); // ajouter un message d'erreur qui dit que le mdp est dans le presse papier...
					}
					SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
					break;
				}
				case TB_ID:
				case TB_PWD:
				case TB_PWD_CLEAR:
				{
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szId[LEN_PWD+1];
						int lenId,lenPwd;
						char szPwd[LEN_PWD+1];
						lenId=GetDlgItemText(w,TB_ID,szId,LEN_ID+1);
						lenPwd=GetDlgItemText(w,IsDlgButtonChecked(w,CK_VIEW)?TB_PWD_CLEAR:TB_PWD,szPwd,LEN_PWD+1);
						SecureZeroMemory(szPwd,LEN_PWD+1);
						EnableWindow(GetDlgItem(w,IDOK),(lenPwd==0 || lenId==0) ? FALSE : TRUE);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,90,ghLogoFondBlanc90);
			rc = TRUE;
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
		case WM_DESTROY:
			if (gbPwdSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID);
			if (gbPwdClearSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID);
			if (gbIdSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)IdProc,TB_ID_SUBCLASS_ID);
			gwSignUp=NULL;
			break;
	}
	return rc;
}


//-----------------------------------------------------------------------------
// SignUpForThisSite()
//-----------------------------------------------------------------------------
// Accompagnement à l'inscription sur un site
//-----------------------------------------------------------------------------
int SignUpForThisSite(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char *pszEncryptedPassword=NULL;
	char *pszFQDN=NULL;
	char *pszWindowTitle=NULL;
	char szAppName[LEN_APPLICATION_NAME+1];
	char szTitle[512+1];
	int lenFQDN;
	HWND w;
	
	if (swGetTopWindow(&w,szTitle,sizeof(szTitle))!=0) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }

	pszFQDN=UniversalGetFQDN(w);
	if (pszFQDN==NULL) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	
	lenFQDN=strlen(pszFQDN);
	pszWindowTitle=(char*)malloc(lenFQDN+50);
	if (pszWindowTitle==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenFQDN+50)); goto end; }
	sprintf_s(pszWindowTitle,lenFQDN+50,"%s %s",GetString(IDS_TITLE_SIGNUP),pszFQDN);
	
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_SIGNUP),NULL,SignUpDialogProc,(LPARAM)pszWindowTitle)==IDOK)
	{
		if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
		ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));
		gptActions[giNbActions].tLastSSO=-1;
		gptActions[giNbActions].wLastSSO=NULL;
		gptActions[giNbActions].iWaitFor=giWaitBeforeNewSSO;
		gptActions[giNbActions].bActive=FALSE;
		gptActions[giNbActions].bAutoLock=TRUE;
		gptActions[giNbActions].bConfigSent=FALSE;
		gptActions[giNbActions].iPwdGroup=-1;
		gptActions[giNbActions].bSafe=TRUE;
		gptActions[giNbActions].iCategoryId=0;
		strcpy_s(gptActions[giNbActions].szURL,pszFQDN);
		pszFQDN[LEN_APPLICATION_NAME]=0; // tronque le FQDN au cas où
		strcpy_s(szAppName,LEN_APPLICATION_NAME+1,pszFQDN);
		GenerateApplicationName(giNbActions,szAppName);
		strcpy_s(gptActions[giNbActions].szId1Value,LEN_ID+1,gszNewAppId);
		pszEncryptedPassword=swCryptEncryptString(gszNewAppPwd,ghKey1);
		if (pszEncryptedPassword==NULL) goto end;
		strcpy_s(gptActions[giNbActions].szPwdEncryptedValue,sizeof(gptActions[giNbActions].szPwdEncryptedValue),pszEncryptedPassword);
		free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
		pszEncryptedPassword=NULL;
		giNbActions++;
		SaveApplications();
	}
end:
	if (pszFQDN==NULL) free(pszFQDN);
	ClipboardDelete();
	SecureZeroMemory(gszNewAppId,LEN_ID+1);
	SecureZeroMemory(gszNewAppPwd,LEN_PWD+1);
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
#endif