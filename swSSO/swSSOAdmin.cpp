//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2017 - Sylvain WERDEFROY
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
// swSSOAdmin.cpp
//-----------------------------------------------------------------------------
// Fonctions spécifiques au mode admin avec login/mdp
//-----------------------------------------------------------------------------

#include "stdafx.h"
static int giRefreshTimer=10;

//-----------------------------------------------------------------------------
// CheckAdminIdPwd()
//-----------------------------------------------------------------------------
// Vérifie le login/mdp admin avec le serveur et stocke le cookie de session
//-----------------------------------------------------------------------------
int CheckAdminIdPwd(char *szId, char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szParams[512+1];
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	DWORD dwStatusCode;

	hCursorOld=SetCursor(ghCursorWait);

	sprintf_s(szParams,sizeof(szParams),"?action=login&id=%s&pwd=%s",szId,szPwd);
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	if (pszResult[0]=='0')
		rc=0;

end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// AskAdminIdPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de saisie du login/mdp admin
//-----------------------------------------------------------------------------
static int CALLBACK AskAdminIdPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_ID),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			MACRO_SET_SEPARATOR;
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
				case TX_FRAME:
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
					char szId[LEN_ID+1];
					char szPwd[LEN_PWD+1];
					int ret;
					GetDlgItemText(w,TB_ID,szId,sizeof(szId));
					GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
					ret=CheckAdminIdPwd(szId,szPwd);
					SecureZeroMemory(szPwd,strlen(szPwd));
					if (ret==0)
						EndDialog(w,IDOK);
					else
						MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_ICONEXCLAMATION);
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_ID:
				case TB_PWD:
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szId[LEN_ID + 1]; 
						char szPwd[LEN_PWD+1];
						int lenId,lenPwd;
						lenId=GetDlgItemText(w,TB_ID,szId,sizeof(szId));
						lenPwd=GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
						SecureZeroMemory(szPwd, strlen(szPwd));
						EnableWindow(GetDlgItem(w,IDOK),(lenId==0||lenPwd==0)?FALSE:TRUE);
					}
					break;
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// AskAdminIdPwd() 
//-----------------------------------------------------------------------------
// Demande le login/mdp admin, vérifie et garde le cookie de session
//-----------------------------------------------------------------------------
int AskAdminIdPwd()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_ADMIN_LOGIN),NULL,AskAdminIdPwdDialogProc)==IDOK) rc=0;

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

