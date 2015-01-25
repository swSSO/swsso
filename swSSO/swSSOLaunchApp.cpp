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
// swSSOLaunchApp.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

HWND gwLaunchApp=NULL;
static int giRefreshTimer=10;
static int giSetForegroundTimer=20;
int giLaunchedApp=-1; // index de l'application lancée

//-----------------------------------------------------------------------------
// MoveControls()
//-----------------------------------------------------------------------------
// Repositionne les contrôles suite à redimensionnement de la fenêtre
//-----------------------------------------------------------------------------
static void MoveControls(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	RECT rect;
	GetClientRect(w,&rect);
	
	SetWindowPos(GetDlgItem(w,TV_APPLICATIONS),NULL,10,10,rect.right-20,rect.bottom-48,SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,IDOK),NULL,rect.right-162,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,IDCANCEL),NULL,rect.right-82,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,CK_VISIBLE),NULL,12,rect.bottom-28,0,0,SWP_NOSIZE | SWP_NOZORDER);
		
	InvalidateRect(w,NULL,FALSE);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// SaveWindowPos()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static void SaveWindowPos(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	RECT rect;
	GetWindowRect(w,&rect);
	char s[8+1];

	if (IsIconic(w)) goto end;

	gx2=rect.left;
	gy2=rect.top;
	gcx2=rect.right-rect.left;
	gcy2=rect.bottom-rect.top;
	wsprintf(s,"%d",gx2);  WritePrivateProfileString("swSSO","x2",s,gszCfgFile);
	wsprintf(s,"%d",gy2);  WritePrivateProfileString("swSSO","y2",s,gszCfgFile);
	wsprintf(s,"%d",gcx2); WritePrivateProfileString("swSSO","cx2",s,gszCfgFile);
	wsprintf(s,"%d",gcy2); WritePrivateProfileString("swSSO","cy2",s,gszCfgFile);
	WritePrivateProfileString("swSSO","LaunchTopMost",gbLaunchTopMost?"YES":"NO",gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// OnInitDialog()
//-----------------------------------------------------------------------------
void OnInitDialog(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	int cx;
	int cy;
	RECT rect;
	
	gwLaunchApp=w;

	// Positionnement et dimensionnement de la fenêtre
	if (gx2!=-1 && gy2!=-1 && gcx2!=-1 && gcy2!=-1)
	{
		SetWindowPos(w,NULL,gx2,gy2,gcx2,gcy2,SWP_NOZORDER);
	}
	else // position par défaut
	{
		cx = GetSystemMetrics( SM_CXSCREEN );
		cy = GetSystemMetrics( SM_CYSCREEN );
		GetWindowRect(w,&rect);
		SetWindowPos(w,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}

	// icone ALT-TAB
	SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
	SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
	
	// Chargement de l'image list
	TreeView_SetImageList(GetDlgItem(w,TV_APPLICATIONS),ghImageList,TVSIL_STATE);

	// Remplissage de la treeview
	FillTreeView(w);

	// case à cocher "Toujours visible"
	CheckDlgButton(w,CK_VISIBLE,gbLaunchTopMost?BST_CHECKED:BST_UNCHECKED);
	SetWindowPos(w,gbLaunchTopMost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

	// timer de refresh
	if (giSetForegroundTimer==giTimer) giSetForegroundTimer=21;
	SetTimer(w,giSetForegroundTimer,100,NULL);

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// LaunchAppDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de lancement d'application 
//-----------------------------------------------------------------------------
static int CALLBACK LaunchAppDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
//	TRACE((TRACE_DEBUG,_F_,"msg=0x%08lx LOWORD(wp)=0x%04x HIWORD(wp)=%d lp=%d",msg,LOWORD(wp),HIWORD(wp),lp));
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:		// ------------------------------------------------------- WM_INITDIALOG
			OnInitDialog(w);
			MoveControls(w); 
			break;
		case WM_DESTROY:
			//
			break;
		case WM_HELP:
			Help();
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (SetForeground)"));
			if (giSetForegroundTimer==(int)wp) 
			{ 
				KillTimer(w,giSetForegroundTimer);
				SetForegroundWindow(w); 
				SetFocus(w); 
			}
			break;
		case WM_COMMAND:		// ------------------------------------------------------- WM_COMMAND
			//TRACE((TRACE_DEBUG,_F_,"WM_COMMAND LOWORD(wp)=0x%04x HIWORD(wp)=%d lp=%d",LOWORD(wp),HIWORD(wp),lp));
			switch (LOWORD(wp))
			{
				case IDOK:
					if (HIWORD(wp)==0) // les notifications autres que "from a control" ne nous intéressent pas !
					{
						LaunchSelectedApp(w);
					}
					break;
				case IDCANCEL:
					if (HIWORD(wp)==0) // les notifications autres que "from a control" ne nous intéressent pas !
					{
						SaveWindowPos(w);
						EndDialog(w,IDCANCEL);
					}
					break;
				case CK_VISIBLE:
					gbLaunchTopMost=(IsDlgButtonChecked(w,CK_VISIBLE)==BST_CHECKED);
					SetWindowPos(w,gbLaunchTopMost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
					break;
			}
			break;
		case WM_SIZE:			// ------------------------------------------------------- WM_SIZE
			MoveControls(w); 
			break;
		case WM_NOTIFY:			// ------------------------------------------------------- WM_NOTIFY
			switch (((NMHDR FAR *)lp)->code) 
			{
				case NM_DBLCLK:
					if (wp==TV_APPLICATIONS) 
					{
						LaunchSelectedApp(w);
					}
					break;
				case TVN_KEYDOWN:
					{
						LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)lp;
						switch (ptvkd->wVKey)
						{
							case VK_F5:
								FillTreeView(w);
								break;
						}
					}
					break;
			}
			break;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// ShowLaunchApp()
// ----------------------------------------------------------------------------------
// Fenêtre de lancement des applications
// ----------------------------------------------------------------------------------
int ShowLaunchApp(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=1;
	
	// si fenêtre déjà affichée, la replace au premier plan
	if (gwLaunchApp!=NULL)
	{
		ShowWindow(gwLaunchApp,SW_SHOW);
		SetForegroundWindow(gwLaunchApp);
		goto end;
	}

	DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_LAUNCH_APP),HWND_DESKTOP,LaunchAppDialogProc,0);
	gwLaunchApp=NULL;
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
