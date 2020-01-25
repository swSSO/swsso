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
// swSSOTray.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions liées au Systray (icône barre de tâches)
//-----------------------------------------------------------------------------

#include "stdafx.h"

unsigned int gMsgTaskbarRestart=0; // message registré pour recevoir les notifs de recréation du systray
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

static char *gpszClipboardPassword=NULL;

HWND gwPopChangeAppPwdDialogProc=NULL;
HWND gwSaveNewAppPwdDialogProc=NULL;
HWND gwConfirmNewAppPwdDialogProc=NULL;
HWND gwSignUp=NULL;

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
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SIGNUP,GetString(IDS_MENU_SIGNUP));
	//InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_THIS_APPLI,GetString(IDS_MENU_THIS_APPLI));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_SSO_NOW,GetString(IDS_MENU_SSO_NOW));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_APPNSITES,GetString(IDS_MENU_APPNSITES));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_PROPRIETES,GetString(IDS_MENU_PROP));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_MDP,GetString(IDS_MENU_MDP));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAY_MENU_QUITTER,GetString(IDS_MENU_QUITTER));
	SetForegroundWindow(w);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,pt.x, pt.y, 0, w, NULL );
end:	
	if (hMenu!=NULL) DestroyMenu(hMenu);
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
					ShowLaunchApp();
					break;
				case TRAY_MENU_PROPRIETES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_PROPRIETES"));
					ShowConfig();
					break;
				case TRAY_MENU_APPNSITES:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_APPNSITES"));
					ShowAppNsites(giLastApplicationConfig,TRUE);
					break;
				case TRAY_MENU_MDP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_MDP"));
					WindowChangeMasterPwd(FALSE);
					break;
				/*case TRAY_MENU_THIS_APPLI:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_THIS_APPLI"));
					if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
					AddApplicationFromCurrentWindow(FALSE);
					break;*/
				case TRAY_MENU_SSO_NOW:
					{	
						TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_SSO_NOW"));
						// regarde si la config existe déjà
						char *pszURL=NULL;
						char szTitle[LEN_TITLE+1];
						int i;
						BOOL bFound=FALSE;
						HWND w;
						pszURL=swGetTopWindowWithURL(&w,szTitle,sizeof(szTitle));
						if (pszURL!=NULL)
						{
							for (i=0;i<giNbActions;i++)
							{
								if (!gptActions[i].bSafe && *gptActions[i].szURL!=0 && swURLMatch(pszURL,gptActions[i].szURL))
								{
									// trouvé la config, si le SSO n'a pas été déclenché il faut le réactiver
									bFound=TRUE;
									ReactivateApplicationFromCurrentWindow();
									if (giTimer==0) // solution de secours du bug #98 (le timer ne s'est pas déclenché)
									{
										TRACE((TRACE_ERROR,_F_,"*************************************************"));
										TRACE((TRACE_ERROR,_F_,"************ Le timer etait arrete ! ************"));
										TRACE((TRACE_ERROR,_F_,"*************************************************"));
										LaunchTimer();
									}
								}
							}
							free(pszURL);
							if (!bFound) // config non trouvée, on l'ajoute
							{
								if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
								AddApplicationFromCurrentWindow(FALSE);
							}
						}
					}	
					break;
				case TRAY_MENU_SIGNUP:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_SIGNUP"));
					SignUpForThisSite();
					break;
				case TRAY_MENU_QUITTER:
					TRACE((TRACE_INFO,_F_, "WM_COMMAND + TRAY_MENU_QUITTER"));
					gbWillTerminate=TRUE;
					if (gwMessageBox3B!=NULL) { EndDialog(gwMessageBox3B,-1); gwMessageBox3B=NULL; }
					if (gwChooseConfig!=NULL) { EndDialog(gwChooseConfig,-1); gwChooseConfig=NULL; }
					if (gwIdAndPwdDialogProc!=NULL) { EndDialog(gwIdAndPwdDialogProc,-1); gwIdAndPwdDialogProc=NULL; }
					if (gwChangeApplicationPassword!=NULL) { EndDialog(gwChangeApplicationPassword,-1); gwChangeApplicationPassword=NULL; }
					if (gwSelectAccount!=NULL) { EndDialog(gwSelectAccount,-1); gwSelectAccount=NULL; }
					PostQuitMessage(0);
					break;
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
					ClipboardCopy(gszNewAppPwd);
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
	char *pszURL=NULL;

	pszURL=swGetTopWindowWithURL(&w,szTitle,sizeof(szTitle));
	if (pszURL==NULL) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	free(pszURL);pszURL=NULL;

	pszFQDN=UniversalGetFQDN(w);
	if (pszFQDN==NULL || *pszFQDN==0) { MessageBox(NULL,GetString(IDS_ERROR_URL),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	
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
