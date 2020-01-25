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
// swSSOConfig.cpp
//-----------------------------------------------------------------------------
// Fenêtre de config + fonctions de lecture/écriture de config
//-----------------------------------------------------------------------------

#include "stdafx.h"

// globales globales ;-)
char gszCfgFile[_SW_MAX_PATH+1];
char gszCfgVersion[5+1];
BOOL gbSessionLock=TRUE;     // 0.63B4 : true si verrouillage sur suspension de session windows
char gszCfgPortal[_SW_MAX_PATH+1];
BOOL gbInternetCheckVersion=FALSE;		// 0.80 : autorise swSSO à se connecter à internet pour vérifier la version
BOOL gbInternetCheckBeta=FALSE;			// 0.80 : y compris pour les beta
BOOL gbInternetGetConfig=FALSE;			// 0.80 : autorise swSSO à se connecter à internet pour récupérer des configurations
//BOOL gbInternetPutConfig=FALSE;			// 0.80 : autorise swSSO à se connecter à internet pour uploader automatiquement les configurations
BOOL gbInternetManualPutConfig=FALSE;	// 0.89 : autorise swSSO à se connecter à internet pour uploader manuellement les configurations
BOOL gbInternetUseProxy=FALSE;			// 0.80 : utilise un proxy pour se connecter à internet
char gszProxyURL[LEN_PROXY_URL+1];		// 0.80 : URL proxy
char gszProxyUser[LEN_PROXY_USER+1];	// 0.80 : compte utilisateur pour authentification proxy
char gszProxyPwd[LEN_PROXY_PWD+1];		// 0.80 : mot de passe pour authentification proxy
BOOL gbTmpInternetUseProxy=FALSE;		// 0.80 : utilise un proxy pour se connecter à internet (pour bouton "j'ai de la chance")
char gszTmpProxyURL[LEN_PROXY_URL+1];	// 0.80 : URL proxy (pour bouton "j'ai de la chance")
char gszTmpProxyUser[LEN_PROXY_USER+1];	// 0.80 : compte utilisateur pour authentification proxy (pour bouton "j'ai de la chance")
char gszTmpProxyPwd[LEN_PROXY_PWD+1];	// 0.80 : mot de passe pour authentification proxy (pour bouton "j'ai de la chance")
BOOL gbParseWindowsOnStart=TRUE;		// 0.93B4 : parse / ne parse pas les fenêtres ouvertes au lancement de SSO
int  giNbExcludedHandles=0;
HWND gTabExcludedHandles[MAX_EXCLUDED_HANDLES];
int  giDomainId=1;						// 0.94B1 : gestion des domaines
BOOL gbDisplayChangeAppPwdDialog ; // ISSUE#107
char gszLastADPwdChange[14+1]="";					// 1.03 : date de dernier changement de mdp dans l'AD, format AAAAMMJJHHMMSS
char gszLastADPwdChange2[50+1]="";					// 1.12 : date de dernier changement de mdp dans l'AD, format hilong,lowlong (cf. ISSUE#281)
char gszEncryptedADPwd[LEN_ENCRYPTED_AES256+1]="";	// 1.03 : valeur du mot de passe AD (fourni par l'utilisateur) ou en 1.08 récupéré en mode chainé Windows
BOOL gbSSOInternetExplorer=TRUE;		// ISSUE#176
BOOL gbSSOFirefox=TRUE;					// ISSUE#176
BOOL gbSSOChrome=TRUE;					// ISSUE#176
BOOL gbSSOEdge=TRUE;					// 1.20
BOOL gbShowLaunchAppWithoutCtrl=FALSE;	// ISSUE#254
int giLanguage=0; // 0=langue de l'OS, 1=FR, 2=EN
wchar_t gwszDefaultLanguage[256]=L"";
HWND gwIdAndPwdDialogProc=NULL;

int gx,gy,gcx,gcy; 		// positionnement de la fenêtre sites et applications
int gx2,gy2,gcx2,gcy2,gbLaunchTopMost; 	// positionnement de lancement d'application
int gx3,gy3,gcx3,gcy3; 		// positionnement de la fenêtre publishto
int gx4,gy4,gcx4,gcy4;		// positionnement de la fenêtre de sélection d'un compte existant
char gszLastConfigUpdate[14+1]; 		// 0.91 : date (AAAAMMJJHHMMSS) de dernière mise à jour des configurations depuis le serveur
HWND gwPropertySheet=NULL;

// globales locales ;-)
static HWND gwChangeMasterPwd=NULL;
static char buf100[100];
static char buf2048[2048]; // 0.76 (précaution...)
static char gszEnumComputerNames[512]; // utilisée pour l'énumération de computernames. Ne pas utiliser ailleurs !!!

BOOL gbDontAskId,gbDontAskPwd; // 0.80 pour fenetre de saisie id et mdp (ajout d'une application)
BOOL gbDontAskId2,gbDontAskId3,gbDontAskId4;

// pour l'énumération des computernames...
static char *gpNextComputerName=NULL;
static char *gpComputerNameContext=NULL;

char gcszK2[]="22222222";

static int giRefreshTimer=10;

char *gpszURLBeingAdded=NULL;
char *gpszTitleBeingAdded=NULL;

T_SALT gSalts; // sels pour le stockage du mdp primaire et la dérivation de clé de chiffrement des mdp secondaires
int giActionIdPwdAsked=-1;
const char gcszCfgVersion[]="093";

T_CONFIG_SYNC gtConfigSync;

typedef BOOL (WINAPI *SETPROCESSPREFERREDUILANGUAGES)(DWORD dwFlags,PCZZWSTR pwszLanguagesBuffer,PULONG pulNumLanguages);
typedef BOOL (WINAPI *GETUSERPREFERREDUILANGUAGES)(DWORD dwFlags,PULONG pulNumLanguages,PZZWSTR pwszLanguagesBuffer, PULONG pcchLanguagesBuffer);

int giMasterPwdExpiration;

HWND gwChangeApplicationPassword=NULL;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

static int HowManyComputerNames(void);

// ----------------------------------------------------------------------------------
// PSPAboutProc()
// ----------------------------------------------------------------------------------
// Dialog proc de l'onglet A propos de 
// ----------------------------------------------------------------------------------
static int CALLBACK PSPAboutProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_,"WM_INITDIALOG"));
			LOGFONT logfont;
			HFONT hFont;
			int cx;
			int cy;
			RECT rect;
			int iNbActiveApps,iNbActiveAppsFromServer;
			
			if (gwPropertySheet==NULL) // nécessaire pour ne pas le faire à chaque premier affichage de l'onglet
			{
				gwPropertySheet=GetParent(w);
				// icone ALT-TAB
				SendMessage(gwPropertySheet,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
				SendMessage(gwPropertySheet,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// 0.53 positionnement de la fenêtre !
				cx = GetSystemMetrics( SM_CXSCREEN );
				cy = GetSystemMetrics( SM_CYSCREEN );
				TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
				GetWindowRect(gwPropertySheet,&rect);
				SetWindowPos(gwPropertySheet,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}
			// liens URL et MAILTO soulignes
			hFont=(HFONT)SendMessage(w,WM_GETFONT,0,0);
			if(hFont!=NULL)
			{
				if(GetObject(hFont, sizeof(LOGFONT), (LPSTR)&logfont)!= NULL)
					logfont.lfUnderline=TRUE;
				hFont=CreateFontIndirect(&logfont);
				if(hFont!=NULL) PostMessage(GetDlgItem(w,TX_URL),WM_SETFONT,(LPARAM)hFont,TRUE);
			}
			char s[100];
			// 0.63BETA5 : détail nb sso popups
			wsprintf(s,"%d (%d web - %d popups - %d windows)",guiNbWINSSO+guiNbWEBSSO+guiNbPOPSSO,guiNbWEBSSO,guiNbPOPSSO,guiNbWINSSO);
			SetDlgItemText(w,TX_NBSSO,s);
			GetNbActiveApps(&iNbActiveApps,&iNbActiveAppsFromServer);
			wsprintf(s,"%d / %d",iNbActiveApps,giNbActions);
			SetDlgItemText(w,TX_NBAPP,s);
			SetDlgItemText(w,TX_CONFIGFILE,gszCfgFile);

			if (!gbEnableOption_ViewIni) { ShowWindow(GetDlgItem(w,TX_CONFIGFILE),SW_HIDE); ShowWindow(GetDlgItem(w,TX_CONFIGFILE2),SW_HIDE); }
			if (!gbEnableOption_OpenIni) ShowWindow(GetDlgItem(w,PB_OUVRIR),SW_HIDE);
			if (!gbEnableOption_Portal)  ShowWindow(GetDlgItem(w,PB_PARCOURIR),SW_HIDE);
			if (!gbEnableOption_Reset)  ShowWindow(GetDlgItem(w,PB_RESET),SW_HIDE);

			// remplit avec les valeurs de config (c'était dans PSN_SETACTIVE avant... ???)
			if (giPwdProtection==PP_ENCRYPTED)
			{
				if (gbNoMasterPwd && gcszK1[0]!='1')
					SetDlgItemText(w,TX_PWDENCRYPTED,GetString(IDS_PP_NOMASTERPWD));
				else
					SetDlgItemText(w,TX_PWDENCRYPTED,GetString(IDS_PP_ENCRYPTED));
			}
			else 
			{
				SetDlgItemText(w,TX_PWDENCRYPTED,GetString(IDS_PP_WINDOWS));
			}

			if (*gszCfgPortal==0) // pas de portail défini
			{
				if (gbEnableOption_Portal) // l'utilisateur a le droit de définir un portail
				{
					SetDlgItemText(w,TX_PORTAL,GetString(IDS_DEFAULT_PORTAL));
				}
				else // pas le droit de définir un portail, on masque tout
				{
					ShowWindow(GetDlgItem(w,TX_PORTAL),SW_HIDE);
					ShowWindow(GetDlgItem(w,TX_PORTAL2),SW_HIDE);
				}
			}
			else
			{
				SetDlgItemText(w,TX_PORTAL,gszCfgPortal);
			}
			// 0.89B2 #94 : affichage de l'URL serveur
			if (gbEnableOption_ViewServerInfos)
			{
				if (gbLastRequestOnFailOverServer)
					wsprintf(buf2048,"%s%s",gszServerAddress2,gszWebServiceAddress2);
				else
					wsprintf(buf2048,"%s%s",gszServerAddress,gszWebServiceAddress);
				SetDlgItemText(w,TX_SERVER,buf2048);
			}
			else
			{
				ShowWindow(GetDlgItem(w,TX_SERVER),SW_HIDE);
				ShowWindow(GetDlgItem(w,TX_SERVER2),SW_HIDE);
			}
			// 0.90 positionnement séparateurs (bug RESEDIT) -> alignement sur bouton licence
			{ 
				RECT rectSeparator,rectBouton;
				GetWindowRect(GetDlgItem(w,PB_LICENCE),&rectBouton);
				
				GetWindowRect(GetDlgItem(w,IDC_SEP_LICENCE),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_LICENCE),NULL,0,0,rectBouton.right-rectSeparator.left,2,SWP_NOMOVE);
				GetWindowRect(GetDlgItem(w,IDC_SEP_INFORMATIONS),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_INFORMATIONS),NULL,0,0,rectBouton.right-rectSeparator.left,2,SWP_NOMOVE);
				GetWindowRect(GetDlgItem(w,IDC_SEP_STATISTIQUES),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_STATISTIQUES),NULL,0,0,rectBouton.right-rectSeparator.left,2,SWP_NOMOVE);
				GetWindowRect(GetDlgItem(w,IDC_SEP_COFFRE),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_COFFRE),NULL,0,0,rectBouton.right-rectSeparator.left,2,SWP_NOMOVE);
			}
			rc=FALSE;
			break;
		}
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_COFFRE:
				case TX_STATISTIQUES:
				case TX_INFORMATIONS:
				case TX_LICENCE:
					SetTextColor((HDC)wp,RGB(0x20,0x20,0xFF));
					break;
				case TX_URL:
					SetTextColor((HDC)wp,RGB(0,0,255));
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH); // 0.60
					break;
			}
			break;
		case WM_HELP:
			Help();
			break;

		case WM_SETCURSOR:
		{
			POINT point={0,0};
			HWND hwndChild=NULL;
			INT iDlgCtrlID=0;
		
			GetCursorPos(&point);
			MapWindowPoints(HWND_DESKTOP,w,&point,1);
			hwndChild=ChildWindowFromPoint(w,point);
			iDlgCtrlID=GetDlgCtrlID(hwndChild);
			if (iDlgCtrlID==TX_URL)
			{
				if(IsWindowVisible(hwndChild))
				{
					SetCursor(ghCursorHand);
					rc=TRUE;
				}
			}
			break;
		}

		case WM_LBUTTONDOWN:
		{
			POINT pt;
			pt.x=LOWORD(lp); 
			pt.y=HIWORD(lp);
			MapWindowPoints(w,HWND_DESKTOP,&pt,1);
			RECT rect;
			GetWindowRect(GetDlgItem(w,TX_URL),&rect);
			if ((pt.x >= rect.left)&&(pt.x <= rect.right)&& (pt.y >= rect.top) &&(pt.y <= rect.bottom))
			{
				char szURL[50];
				*szURL=0;
				GetWindowText(GetDlgItem(w,TX_URL),szURL,sizeof(szURL));
				if (strcmp(szURL,"https://www.swsso.fr")==0)
				{
					strcpy_s(szURL,"https://www.swsso.fr/?page_id=201");
				}
				else
				{
					strcpy_s(szURL,"https://www.swsso.fr/?page_id=31929");
				}
				ShellExecute(NULL,"open",szURL,NULL,"",SW_SHOW );
			}
			break;
		}
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE  :
					InvalidateRect(w,NULL,TRUE);
					break;
				case PSN_APPLY:
					GetDlgItemText(w,TX_PORTAL,gszCfgPortal,sizeof(gszCfgPortal));
					// correction bug portail en 0.90B2
					if (strcmp(gszCfgPortal,GetString(IDS_DEFAULT_PORTAL))==0) *gszCfgPortal=0;
					SaveConfigHeader();
					PropSheet_UnChanged(gwPropertySheet,w);
					break;
			}
			break;
		case WM_COMMAND: // 0.63BETA5 : ouverture du .ini depuis la fenêtre about.
			switch (LOWORD(wp))
			{
				case PB_OUVRIR:
					ShellExecute(NULL,"open",gszCfgFile,NULL,"",SW_SHOW );
					break;
				case PB_LICENCE:
					ShellExecute(NULL,"open","http://www.gnu.org/licenses/gpl.html",NULL,"",SW_SHOW );
					break;
				case PB_PARCOURIR:
					OPENFILENAME ofn;
					ZeroMemory(&ofn,sizeof(OPENFILENAME));
					ofn.lStructSize=sizeof(OPENFILENAME);
					ofn.hwndOwner=w;
					ofn.hInstance=NULL;
					ofn.lpstrFilter="*.json";
					ofn.lpstrCustomFilter=NULL;
					ofn.nMaxCustFilter=0;
					ofn.nFilterIndex=0;
					ofn.lpstrFile=gszCfgPortal;
					ofn.nMaxFile=sizeof(gszCfgPortal);
					ofn.lpstrFileTitle=NULL;
					ofn.nMaxFileTitle=NULL;
					ofn.lpstrInitialDir=NULL;
					ofn.lpstrTitle=NULL;
					ofn.Flags=OFN_PATHMUSTEXIST;
					ofn.nFileOffset;
					ofn.lpstrDefExt="json";
					if (GetSaveFileName(&ofn)) 
					{
						SetDlgItemText(w,TX_PORTAL,gszCfgPortal);
						SavePortal();
						PropSheet_Changed(gwPropertySheet,w);
					}
					break;
				case PB_RESET:
					if (MessageBox(NULL,GetString(IDS_CONFIRM_RESET),"swSSO",MB_YESNO | MB_ICONQUESTION)==IDYES)
					{
						// suppression de toutes les configurations
						giNbActions=0;
						giNbCategories=1;
						SaveApplications();
						// réinitialisation depuis le serveur
					}
					break;
			}
			break;
		case WM_PAINT:
			{
		        PAINTSTRUCT ps;
		        HDC dc=BeginPaint(w,&ps);
				DrawTransparentBitmap(ghLogo,dc,10,18,52,44,RGB(255,0,128));
				EndPaint(w,&ps);
				rc=TRUE;
 			}
			break;
		case WM_DESTROY:
			DeleteObject((HFONT)SendMessage(GetDlgItem(w,TX_URL),WM_GETFONT,0,0));  
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// SetLanguage()
//-----------------------------------------------------------------------------
// Change la langue de l'IHM
//-----------------------------------------------------------------------------
void SetLanguage(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	ULONG pul;
	HMODULE ghKernelDll=NULL;
	SETPROCESSPREFERREDUILANGUAGES lpfnSetProcessPreferredUILanguages=NULL;
	
	ghKernelDll=LoadLibrary("Kernel32.dll"); 
	if (ghKernelDll==NULL)  { TRACE((TRACE_ERROR, _F_, "LoadLibrary(Kernel32.dll)", GetLastError())); goto end; }

	lpfnSetProcessPreferredUILanguages=(SETPROCESSPREFERREDUILANGUAGES)GetProcAddress(ghKernelDll,"SetProcessPreferredUILanguages");
	if (lpfnSetProcessPreferredUILanguages==NULL) { TRACE((TRACE_ERROR, _F_, "GetProcAddress(SetProcessPreferredUILanguages)", GetLastError())); goto end; }

	switch (giLanguage)
	{
		case 1:
			lpfnSetProcessPreferredUILanguages(MUI_LANGUAGE_NAME,L"fr-FR",&pul);
			break;
		case 2:
			lpfnSetProcessPreferredUILanguages(MUI_LANGUAGE_NAME,L"en-US",&pul);
			break;
		default:
			lpfnSetProcessPreferredUILanguages(MUI_LANGUAGE_NAME,gwszDefaultLanguage,&pul);
			break;
	}
end:
	if (ghKernelDll!=NULL) FreeLibrary(ghKernelDll);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// GetLanguage()
//-----------------------------------------------------------------------------
// Récupère la langue courante
//-----------------------------------------------------------------------------
void GetOSLanguage(void)
{
	TRACE((TRACE_ENTER,_F_, ""));

	HMODULE ghKernelDll=NULL;
	GETUSERPREFERREDUILANGUAGES lpfnGetUserPreferredUILanguages=NULL;
	ULONG ulNbLanguages,ulSize;
	ulSize=256;	
	
	ghKernelDll=LoadLibrary("Kernel32.dll"); 
	if (ghKernelDll==NULL)  { TRACE((TRACE_ERROR, _F_, "LoadLibrary(Kernel32.dll)", GetLastError())); goto end; }

	lpfnGetUserPreferredUILanguages=(GETUSERPREFERREDUILANGUAGES)GetProcAddress(ghKernelDll,"GetUserPreferredUILanguages");
	if (lpfnGetUserPreferredUILanguages==NULL) { TRACE((TRACE_ERROR, _F_, "GetProcAddress(GetUserPreferredUILanguages)", GetLastError())); goto end; }

	if (!lpfnGetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&ulNbLanguages,gwszDefaultLanguage,&ulSize))
	{
		TRACE((TRACE_ERROR,_F_,"lpfnGetUserPreferredUILanguages()=%ld",GetLastError()));
	}

end:
	if (ghKernelDll!=NULL) FreeLibrary(ghKernelDll);
	TRACE((TRACE_LEAVE,_F_, ""));
}



// ----------------------------------------------------------------------------------
// PSPBrowserProc()
// ----------------------------------------------------------------------------------
// Dialog proc de l'onglet navigateurs
// ----------------------------------------------------------------------------------
static int CALLBACK PSPBrowserProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			
			if (gwPropertySheet==NULL) // nécessaire pour ne pas le faire à chaque premier affichage de l'onglet
			{
				gwPropertySheet=GetParent(w);
				// icone ALT-TAB
				SendMessage(gwPropertySheet,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
				SendMessage(gwPropertySheet,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// 0.53 positionnement de la fenêtre !
				cx = GetSystemMetrics( SM_CXSCREEN );
				cy = GetSystemMetrics( SM_CYSCREEN );
				TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
				GetWindowRect(gwPropertySheet,&rect);
				SetWindowPos(gwPropertySheet,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}	
			CheckDlgButton(w,CK_IE,gbSSOInternetExplorer?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_FIREFOX,gbSSOFirefox?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_CHROME,gbSSOChrome?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_EDGE,gbSSOEdge?BST_CHECKED:BST_UNCHECKED);

			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_DEFAULT));
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)"Français");
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)"English");
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_SETCURSEL,giLanguage,0);

			SendMessage(GetDlgItem(w,SPIN_MASTER_PWD_EXPIRATION),UDM_SETRANGE,0,MAKELPARAM(giMasterPwdMaxExpiration==-1?60:giMasterPwdMaxExpiration,0));
			SetDlgItemInt(w,TB_MASTER_PWD_EXPIRATION,giMasterPwdExpiration,FALSE);

			rc=FALSE;
			break;

		case WM_HELP:
			Help();
			break;

		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_BROWSER:
				case TX_SECURITE:
					SetTextColor((HDC)wp,RGB(0x20,0x20,0xFF)); 
					break;
			}
			break;

		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_KILLACTIVE : // bouton OK ou appliquer
					SetWindowLong(w,DWL_MSGRESULT,FALSE);
					break;
				case PSN_SETACTIVE  :
					break;
				case PSN_APPLY:
					{
						BOOL bTranslated;
						gbSSOInternetExplorer=IsDlgButtonChecked(w,CK_IE)==BST_CHECKED?TRUE:FALSE;
						gbSSOFirefox=IsDlgButtonChecked(w,CK_FIREFOX)==BST_CHECKED?TRUE:FALSE;
						gbSSOChrome=IsDlgButtonChecked(w,CK_CHROME)==BST_CHECKED?TRUE:FALSE;
						gbSSOEdge=IsDlgButtonChecked(w,CK_EDGE)==BST_CHECKED?TRUE:FALSE;
						giLanguage=SendMessage(GetDlgItem(w,CB_LANGUE),CB_GETCURSEL,0,0);
						if (giLanguage==CB_ERR) giLanguage=0;
						SetLanguage();
						giMasterPwdExpiration=GetDlgItemInt(w,TB_MASTER_PWD_EXPIRATION,&bTranslated,FALSE);
						if (giMasterPwdExpiration>giMasterPwdMaxExpiration) giMasterPwdExpiration=giMasterPwdMaxExpiration;
						SaveConfigHeader();
						PropSheet_UnChanged(gwPropertySheet,w);
					}
					break;
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wp)==EN_CHANGE) PropSheet_Changed(gwPropertySheet,w);
			if (HIWORD(wp)==CBN_SELCHANGE) PropSheet_Changed(gwPropertySheet,w);
			switch (LOWORD(wp))
			{
				case CK_IE:
				case CK_FIREFOX:
				case CK_CHROME:
					PropSheet_Changed(gwPropertySheet,w);
					break;
			}
			break;
	}
	return rc;
}

		
// ----------------------------------------------------------------------------------
// IdAndPwdDialogProc()
// ----------------------------------------------------------------------------------
// Dialogproc de la boite de saisie des id/mdp d'une appli récupérée dans config centrale
// ----------------------------------------------------------------------------------
int CALLBACK IdAndPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			gwIdAndPwdDialogProc=w;
			// récupération de la structure T_IDANDPWDDIALOG passée en LPARAM
			T_IDANDPWDDIALOG *params=(T_IDANDPWDDIALOG*)lp;
			giActionIdPwdAsked=params->iAction;
			// stockage pour la suite
			SetWindowLong(w,DWL_USER,lp);
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation champs de saisie
			SendMessage(GetDlgItem(w,TB_ID),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID2),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID3),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID4),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			// remplissage des champs si valeur connue
			SetDlgItemText(w,TB_ID,gptActions[params->iAction].szId1Value);
			SetDlgItemText(w,TB_ID2,gptActions[params->iAction].szId2Value);
			SetDlgItemText(w,TB_ID3,gptActions[params->iAction].szId3Value);
			SetDlgItemText(w,TB_ID4,gptActions[params->iAction].szId4Value);
			if (*gptActions[params->iAction].szPwdEncryptedValue!=0)
			{
				char *pszDecryptedValue=swCryptDecryptString(gptActions[params->iAction].szPwdEncryptedValue,ghKey1);
				if (pszDecryptedValue!=NULL) 
				{
					SetDlgItemText(w,TB_PWD,pszDecryptedValue);
					SecureZeroMemory(pszDecryptedValue,strlen(pszDecryptedValue));
					free(pszDecryptedValue);
				}
			}
			// masquage de champs 
			EnableWindow(GetDlgItem(w,TB_ID),!gbDontAskId);
			EnableWindow(GetDlgItem(w,TB_PWD),!gbDontAskPwd);
			ShowWindow(GetDlgItem(w,TB_ID2),gbDontAskId2?SW_HIDE:SW_SHOW); ShowWindow(GetDlgItem(w,TX_ID2),gbDontAskId2?SW_HIDE:SW_SHOW); 
			ShowWindow(GetDlgItem(w,TB_ID3),gbDontAskId3?SW_HIDE:SW_SHOW); ShowWindow(GetDlgItem(w,TX_ID3),gbDontAskId3?SW_HIDE:SW_SHOW); 
			ShowWindow(GetDlgItem(w,TB_ID4),gbDontAskId4?SW_HIDE:SW_SHOW); ShowWindow(GetDlgItem(w,TX_ID4),gbDontAskId4?SW_HIDE:SW_SHOW); 
			if (gbDontAskId && gbDontAskPwd) EnableWindow(GetDlgItem(w,IDOK),TRUE);
			// nom des champs complémentaires, si présents
			if (!gbDontAskId2)
			{
				if (*gptActions[params->iAction].szId2Name==0)
				{
					strcpy_s(buf100,sizeof(buf100),GetString(IDS_ID2));
					wsprintf(buf2048,GetString(IDS_VALUE),buf100);
				}
				else
					wsprintf(buf2048,GetString(IDS_VALUE),gptActions[params->iAction].szId2Name);
				SetDlgItemText(w,TX_ID2,buf2048);
			}
			if (!gbDontAskId3)
			{
				if (*gptActions[params->iAction].szId3Name==0)
				{
					strcpy_s(buf100,sizeof(buf100),GetString(IDS_ID3));
					wsprintf(buf2048,GetString(IDS_VALUE),buf100);
				}
				else
					wsprintf(buf2048,GetString(IDS_VALUE),gptActions[params->iAction].szId3Name);
				SetDlgItemText(w,TX_ID3,buf2048);
			}
			if (!gbDontAskId4)
			{
				if (*gptActions[params->iAction].szId4Name==0)
				{
					strcpy_s(buf100,sizeof(buf100),GetString(IDS_ID4));
					wsprintf(buf2048,GetString(IDS_VALUE),buf100);
				}
				else
					wsprintf(buf2048,GetString(IDS_VALUE),gptActions[params->iAction].szId4Name);
				SetDlgItemText(w,TX_ID4,buf2048);
			}
			// positionnement et dimensionnement de la fenêtre + replacement des boutons OK / Cancel
			int cx;
			int cy;
			RECT rect,clientRect,rectOK,rectCancel,rectLink;
			int iNbToHide=((gbDontAskId2?1:0)+(gbDontAskId3?1:0)+(gbDontAskId4?1:0));

			// dimensionnement de la fenêtre
			GetWindowRect(w,&rect);
			GetClientRect(w,&clientRect);
			SetWindowPos(w,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top-32*iNbToHide,SWP_NOMOVE | SWP_NOZORDER);

			// Correction BUG #109 en 0.90B1
			// positionnement bidon à 0,0 pour pouvoir faire les positionnements de bouton 
			// sinon par défaut la fenêtre peut éventuellement être décalée en fonction des barres d'outils
			// ouvertes à gauche ou en haut !
			SetWindowPos(w,NULL,0,0,0,0,SWP_NOSIZE | SWP_NOZORDER);

			// replacement des boutons OK / Cancel
			GetWindowRect(GetDlgItem(w,IDOK),&rectOK);
			GetWindowRect(GetDlgItem(w,IDCANCEL),&rectCancel);
			TRACE((TRACE_DEBUG,_F_,"rectOK=(%d,%d,%d,%d)",rectOK.left,rectOK.right,rectOK.top,rectOK.bottom));
			// remarque : (rect.bottom-rect.top-clientRect.bottom) = taille de la barre de titre
			SetWindowPos(GetDlgItem(w,IDOK),NULL,rectOK.left,rectOK.top-32*iNbToHide-(rect.bottom-rect.top-clientRect.bottom),0,0,SWP_NOSIZE | SWP_NOZORDER);
			SetWindowPos(GetDlgItem(w,IDCANCEL),NULL,rectCancel.left,rectCancel.top-32*iNbToHide-(rect.bottom-rect.top-clientRect.bottom),0,0,SWP_NOSIZE | SWP_NOZORDER);

			// replacement du lien "je veux réutiliser un identifiant existant"
			GetWindowRect(GetDlgItem(w,TX_EXISTING),&rectLink);
			// remarque : (rect.bottom-rect.top-clientRect.bottom) = taille de la barre de titre
			SetWindowPos(GetDlgItem(w,TX_EXISTING),NULL,rectLink.left,rectLink.top-32*iNbToHide-(rect.bottom-rect.top-clientRect.bottom),0,0,SWP_NOSIZE | SWP_NOZORDER);
			
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			GetWindowRect(w,&rect);
			if (params->bCenter) // centrage par rapport au parent
			{
				RECT rectParent;
				HWND wParent;
				wParent=GetParent(w);
				if (wParent==HWND_DESKTOP)
				{
					rectParent.left=0;
					rectParent.top=0;
					rectParent.right=cx;
					rectParent.bottom=cy;
				}
				else
				{
					GetWindowRect(wParent,&rectParent);
				}
				SetWindowPos(w,NULL,rectParent.left+((rectParent.right-rectParent.left)-(rect.right-rect.left))/2,
									rectParent.top+ ((rectParent.bottom-rectParent.top)-(rect.bottom-rect.top))/2,
									0,0,SWP_NOSIZE | SWP_NOZORDER);

			}
			else // positionnement en bas à droite de l'écran, près de l'icone swSSO
			{
				SetWindowPos(w,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}
			// lien souligné
			LOGFONT logfont;
			HFONT hFont;
			hFont=(HFONT)SendMessage(w,WM_GETFONT,0,0);
			if(hFont!=NULL)
			{
				if(GetObject(hFont, sizeof(LOGFONT), (LPSTR)&logfont)!= NULL) logfont.lfUnderline=TRUE;
				hFont=CreateFontIndirect(&logfont);
				if(hFont!=NULL) PostMessage(GetDlgItem(w,TX_EXISTING),WM_SETFONT,(LPARAM)hFont,TRUE);
			}

			// titre en gras
			SetWindowText(w,GetString(params->iTitle));
			SetDlgItemText(w,TX_FRAME,params->szText);
			SetTextBold(w,TX_FRAME);
			MACRO_SET_SEPARATOR;
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
		}
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
				//SetFocus(w); ATTENTION, REMET LE FOCUS SUR LE MDP ET FOUT LA MERDE SI SAISIE DEJA COMMENCEE !
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					char szPassword[LEN_PWD+1];
					char *pszEncryptedPassword=NULL;
					T_IDANDPWDDIALOG *params=(T_IDANDPWDDIALOG*)GetWindowLong(w,DWL_USER);

					GetDlgItemText(w,TB_ID,gptActions[params->iAction].szId1Value,sizeof(gptActions[params->iAction].szId1Value));
					GetDlgItemText(w,TB_ID2,gptActions[params->iAction].szId2Value,sizeof(gptActions[params->iAction].szId2Value));
					GetDlgItemText(w,TB_ID3,gptActions[params->iAction].szId3Value,sizeof(gptActions[params->iAction].szId3Value));
					GetDlgItemText(w,TB_ID4,gptActions[params->iAction].szId4Value,sizeof(gptActions[params->iAction].szId4Value));
					GetDlgItemText(w,TB_PWD,szPassword,sizeof(szPassword));
					if (*szPassword!=0) // TODO -> CODE A REVOIR PLUS TARD (PAS BEAU SUITE A ISSUE#83)
					{
						pszEncryptedPassword=swCryptEncryptString(szPassword,ghKey1);
						SecureZeroMemory(szPassword,strlen(szPassword));
						if (pszEncryptedPassword!=NULL)
						{
							strcpy_s(gptActions[params->iAction].szPwdEncryptedValue,sizeof(gptActions[params->iAction].szPwdEncryptedValue),pszEncryptedPassword);
							// NEW pour ISSUE#367 : replique le mot de passe sur toutes les configs du password group, 
							// uniquement si password group >= 20 et que la configuration n'est pas issue d'un ajout de compte
							if (gptActions[params->iAction].iPwdGroup>=20 && !gptActions[params->iAction].bAddAccount)
							{
								for (int i=0;i<giNbActions;i++)
								{
									if (i==params->iAction) continue; 
									if (gptActions[i].iPwdGroup==gptActions[params->iAction].iPwdGroup)
									{
										TRACE((TRACE_DEBUG,_F_,"Changement mot de passe appli %s induit par config %s",gptActions[i].szApplication,gptActions[params->iAction].szApplication));
										strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedPassword);
										// ISSUE#395 : sauvegarde la valeur du nouveau mot de passe dans le .ini
										WritePrivateProfileString(gptActions[i].szApplication,"pwdValue",gptActions[i].szPwdEncryptedValue,gszCfgFile);
									}
								}
							}
							free(pszEncryptedPassword); // 0.85B9 !
						}
						else
						{
							*gptActions[params->iAction].szPwdEncryptedValue=0;
						}
					}
					else
					{
						strcpy_s(gptActions[params->iAction].szPwdEncryptedValue,sizeof(gptActions[params->iAction].szPwdEncryptedValue),szPassword);
					}
					// ISSUE#113 : refresh sur la fenêtre gestion des sites et applications
					if (gwAppNsites!=NULL && IsWindow(gwAppNsites) && !gbAjoutDeCompteEnCours)
					{
						ShowApplicationDetails(gwAppNsites,params->iAction);
					}
					EndDialog(w,IDOK);
				}
					break;
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_ID:
				case TB_ID2:
				case TB_ID3:
				case TB_ID4:
				case TB_PWD:
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szPassword[LEN_PWD+1];
						char szId[LEN_ID+1];
						int lenId,lenPwd,lenId2,lenId3,lenId4;
						lenId=GetDlgItemText(w,TB_ID,szId,sizeof(szId));
						lenId2=GetDlgItemText(w,TB_ID2,szId,sizeof(szId));
						lenId3=GetDlgItemText(w,TB_ID3,szId,sizeof(szId));
						lenId4=GetDlgItemText(w,TB_ID4,szId,sizeof(szId));
						lenPwd=GetDlgItemText(w,TB_PWD,szPassword,sizeof(szPassword));
						EnableWindow(GetDlgItem(w,IDOK),((lenId==0 && !gbDontAskId) || 
														 (lenPwd==0 && !gbDontAskPwd) ||
														 (lenId2==0 && !gbDontAskId2) ||
														 (lenId3==0 && !gbDontAskId3) ||
														 (lenId4==0 && !gbDontAskId4)) ? FALSE : TRUE);
					}
					break;
			}
			break; // 1.12B2-AC-TIE6
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
				case TX_EXISTING:
					SetTextColor((HDC)wp,RGB(0,0,255));
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_SETCURSOR:
		{
			POINT point={0,0};
			HWND hwndChild=NULL;
			INT iDlgCtrlID=0;
		
			GetCursorPos(&point);
			MapWindowPoints(HWND_DESKTOP,w,&point,1);
			hwndChild=ChildWindowFromPoint(w,point);
			iDlgCtrlID=GetDlgCtrlID(hwndChild);
			if (iDlgCtrlID==TX_EXISTING)
			{
				if(IsWindowVisible(hwndChild))
				{
					SetCursor(ghCursorHand);
					rc=TRUE;
				}
			}
			
			break;
		}
		case WM_LBUTTONDOWN:
		{
			POINT pt;
			pt.x=LOWORD(lp); 
			pt.y=HIWORD(lp);
			MapWindowPoints(w,HWND_DESKTOP,&pt,1);
			RECT rect;
			GetWindowRect(GetDlgItem(w,TX_EXISTING),&rect);
			if ((pt.x >= rect.left)&&(pt.x <= rect.right)&& (pt.y >= rect.top) &&(pt.y <= rect.bottom))
			{
				int iAction;
				iAction=ShowSelectAccount(w);
				if (iAction!=-1) // l'utilisateur a sélectionné une appli dans la liste
				{
					SetDlgItemText(w,TB_ID,gptActions[iAction].szId1Value);
					if (*gptActions[iAction].szPwdEncryptedValue!=0)
					{
						char *pszDecryptedValue=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
						if (pszDecryptedValue!=NULL) 
						{
							SetDlgItemText(w,TB_PWD,pszDecryptedValue);
							SecureZeroMemory(pszDecryptedValue,strlen(pszDecryptedValue));
							free(pszDecryptedValue);
						}
					}
					if (!gbDontAskId2) SetDlgItemText(w,TB_ID2,gptActions[iAction].szId2Value);
					if (!gbDontAskId3) SetDlgItemText(w,TB_ID3,gptActions[iAction].szId3Value);
					if (!gbDontAskId4) SetDlgItemText(w,TB_ID4,gptActions[iAction].szId4Value);
					SetFocus(GetDlgItem(w,IDOK));
				}
			}
			break;
		}
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
		case WM_DESTROY:
			DeleteObject((HFONT)SendMessage(GetDlgItem(w,TX_EXISTING),WM_GETFONT,0,0));  
			break;
	}
	return rc;
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// ShowConfig()
// ----------------------------------------------------------------------------------
// Affichage de la fenêtre de config
// ----------------------------------------------------------------------------------
int ShowConfig(void)
{
	TRACE((TRACE_ENTER,_F_, ""));

	MSG msg;
	BOOL bRet;
	BOOL bQuit=FALSE;
	int iPage;

	if (gwPropertySheet!=NULL)
	{
		ShowWindow(gwPropertySheet,SW_SHOW);
		SetForegroundWindow(gwPropertySheet);
		goto end;
	}

	HPROPSHEETPAGE hpsp[2];
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;

	ZeroMemory(&psh,sizeof(PROPSHEETHEADER));
	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));

	psp.dwSize=sizeof(PROPSHEETPAGE);
	psp.dwFlags=PSP_DEFAULT;

	psh.dwSize=sizeof(PROPSHEETHEADER);
	psh.dwFlags=PSH_MODELESS; //PSH_DEFAULT; // PSH_PROPTITLE

	psh.hwndParent=HWND_DESKTOP;
	psh.pszCaption="swSSO - Options";
	//psh.nStartPage=(gbEnableOption_ShowOptions?0:1);
	psh.nStartPage=0;
	iPage=0;

	psp.pszTemplate=MAKEINTRESOURCE(PSP_BROWSER);
	psp.pfnDlgProc=PSPBrowserProc;
	psp.lParam=0;
	hpsp[iPage]=CreatePropertySheetPage(&psp);
	if (hpsp[iPage]==NULL) goto end;
	iPage++;
	
	psp.pszTemplate=MAKEINTRESOURCE(PSP_ABOUT);
	psp.pfnDlgProc=PSPAboutProc;
	psp.lParam=0;
	hpsp[iPage]=CreatePropertySheetPage(&psp);
	if (hpsp[iPage]==NULL) goto end;

	psh.nPages=iPage+1;
	psh.phpage=hpsp;
	
	gwPropertySheet=(HWND)PropertySheet(&psh);
	// boucle de message (propertysheet modeless)
	while ((bRet=GetMessage(&msg,NULL,0,0))!=0)
	{
		if(bRet==-1) { TRACE((TRACE_ERROR,_F_,"GetMessage()=-1")); goto end; }
		CheckIfQuitMessage(msg.message);
		if(!PropSheet_IsDialogMessage(gwPropertySheet,&msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
		if (PropSheet_GetCurrentPageHwnd(gwPropertySheet)==NULL)
		{  
			DestroyWindow(gwPropertySheet);
			gwPropertySheet=NULL;
			break;
		}
	}
	if (bQuit || bRet==0)
	{
		PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_QUITTER,0),0);
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return 0;
}


// ----------------------------------------------------------------------------------
// GetConfigHeader()
// ----------------------------------------------------------------------------------
// Lecture de la section [swSSO] qui contient les infos de version et de format
// ----------------------------------------------------------------------------------
int GetConfigHeader()
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	char szPwdProtection[9+1];
	int rc=-1;
	BOOL bCheckVersion;
	BOOL bChangeIni=FALSE;
	
	// ISSUE#164 vérifie l'intégrité du fichier .ini. Il faut le faire avant de toucher au .ini
	// Si le fichier .ini n'existe pas (cas de la première utilisation), on ne fait pas évidemment pas la vérification...
	// HANDLE hf=INVALID_HANDLE_VALUE;
	// hf=CreateFile(gszCfgFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	// if (hf!=INVALID_HANDLE_VALUE) // le fichier existe
	// {	
	// 		CloseHandle(hf);
	//		if (CheckIniHash()!=0) goto end;
	// }
	// Il ne faut pas non plus vérifier l'intégrité lorsque swSSO n'a jamais été lancé sinon impossible de fournir un .ini avec des options par défaut...
	// Du coup on se base sur la présence des sels : sels présents on vérifie l'intégrité, sinon non.
	// Si l'utilisateur supprime les sels pour faire sauter la vérification de l'intégrité, swSSO ne se lancera pas (impossible de se connecter sans sels)
	char szTmpPBKDF2Salt[PBKDF2_SALT_LEN*2+1];
	GetPrivateProfileString("swSSO","pwdSalt","",szTmpPBKDF2Salt,sizeof(szTmpPBKDF2Salt),gszCfgFile);
	if (strlen(szTmpPBKDF2Salt)==PBKDF2_SALT_LEN*2) 
	{
		if (CheckIniHash()!=0) goto end;
	}
	
	GetPrivateProfileString("swSSO","version","",gszCfgVersion,sizeof(gszCfgVersion),gszCfgFile);
	TRACE((TRACE_INFO,_F_,"Version du fichier de configuration : %s",gszCfgVersion));
	GetPrivateProfileString("swSSO","pwdProtection","",szPwdProtection,sizeof(szPwdProtection),gszCfgFile);
	if (strcmp(szPwdProtection,"ENCRYPTED")==0)
		giPwdProtection=PP_ENCRYPTED;
	else if (strcmp(szPwdProtection,"WINDOWS")==0)
		giPwdProtection=PP_WINDOWS;
	else if (*szPwdProtection!=0)
	{
		TRACE((TRACE_ERROR,_F_,"pwdProtection=%d : valeur non supportee",giPwdProtection));
		goto end;
	}
	TRACE((TRACE_INFO,_F_,"pwdProtection=%d",giPwdProtection));
	// ISSUE#185 : les valeurs par défaut sont désormais lues en base de registre dans swSSOPolicies
	gbSessionLock=GetConfigBoolValue("swSSO","sessionLock",gbSessionLock_DefaultValue,TRUE);
	bCheckVersion=gbInternetCheckVersion_DefaultValue;
	if (bCheckVersion)
	{
		bCheckVersion=strcmp(gszServerAddress,"ws.swsso.fr")==0; 	// ISSUE#134
	}
	gbInternetCheckVersion=GetConfigBoolValue("swSSO","internetCheckVersion",bCheckVersion,TRUE);
	gbInternetCheckBeta=GetConfigBoolValue("swSSO","internetCheckBeta",gbInternetCheckBeta_DefaultValue,TRUE);
	gbInternetGetConfig=GetConfigBoolValue("swSSO","internetGetConfig",gbInternetGetConfig_DefaultValue,TRUE);
	gbInternetManualPutConfig=GetConfigBoolValue("swSSO","internetManualPutConfig",gbAdmin?TRUE:gbInternetManualPutConfig_DefaultValue,TRUE);
	GetPrivateProfileString("swSSO","Portal",gszCfgPortal_DefaultValue,gszCfgPortal,sizeof(gszCfgPortal),gszCfgFile);
	gbLaunchTopMost=GetConfigBoolValue("swSSO","LaunchTopMost",gbLaunchTopMost_DefaultValue,TRUE);
	gbParseWindowsOnStart=GetConfigBoolValue("swSSO","parseWindowsOnStart",gbParseWindowsOnStart_DefaultValue,TRUE);// 0.93B4 ISSUE#50 (?)
	gbDisplayChangeAppPwdDialog=GetConfigBoolValue("swSSO","displayChangeAppPwdDialog",gbDisplayChangeAppPwdDialog_DefaultValue,TRUE); // ISSUE#107
	gbSSOInternetExplorer=GetConfigBoolValue("swSSO","InternetExplorer",gbSSOInternetExplorer_DefaultValue,TRUE); // ISSUE#176
	gbSSOFirefox=GetConfigBoolValue("swSSO","Firefox",gbSSOFirefox_DefaultValue,TRUE); // ISSUE#176
	gbSSOChrome=GetConfigBoolValue("swSSO","Chrome",gbSSOChrome_DefaultValue,TRUE); // ISSUE#176
	gbSSOEdge=GetConfigBoolValue("swSSO","Edge",gbSSOEdge_DefaultValue,TRUE); // 1.20
	gbShowLaunchAppWithoutCtrl=GetConfigBoolValue("swSSO","ShowLaunchAppWithoutCtrl",gbShowLaunchAppWithoutCtrl_DefaultValue,TRUE); // ISSUE#254
	
	gx=GetPrivateProfileInt("swSSO","x",-1,gszCfgFile);
	gy=GetPrivateProfileInt("swSSO","y",-1,gszCfgFile);
	gcx=GetPrivateProfileInt("swSSO","cx",-1,gszCfgFile);
	gcy=GetPrivateProfileInt("swSSO","cy",-1,gszCfgFile);
	gx2=GetPrivateProfileInt("swSSO","x2",-1,gszCfgFile);
	gy2=GetPrivateProfileInt("swSSO","y2",-1,gszCfgFile);
	gcx2=GetPrivateProfileInt("swSSO","cx2",-1,gszCfgFile);
	gcy2=GetPrivateProfileInt("swSSO","cy2",-1,gszCfgFile);
	gx3=GetPrivateProfileInt("swSSO","x3",-1,gszCfgFile);
	gy3=GetPrivateProfileInt("swSSO","y3",-1,gszCfgFile);
	gcx3=GetPrivateProfileInt("swSSO","cx3",-1,gszCfgFile);
	gcy3=GetPrivateProfileInt("swSSO","cy3",-1,gszCfgFile);
	GetPrivateProfileString("swSSO","lastConfigUpdate","",gszLastConfigUpdate,sizeof(gszLastConfigUpdate),gszCfgFile);
	
	gx4=GetPrivateProfileInt("swSSO","x4",-1,gszCfgFile);
	gy4=GetPrivateProfileInt("swSSO","y4",-1,gszCfgFile);
	gcx4=GetPrivateProfileInt("swSSO","cx4",-1,gszCfgFile);
	gcy4=GetPrivateProfileInt("swSSO","cy4",-1,gszCfgFile);

	giMasterPwdExpiration=GetPrivateProfileInt("swSSO","MasterPwdExpiration",-1,gszCfgFile);
	if (giMasterPwdExpiration==-1) // non défini
	{
			giMasterPwdExpiration=5;
	}
	else // défini par l'utilisateur, on borne à la valeur max de la policy, si définie
	{
		if (giMasterPwdMaxExpiration!=-1 && giMasterPwdExpiration>giMasterPwdMaxExpiration) giMasterPwdExpiration=giMasterPwdMaxExpiration;
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// AddComputerName()
// ----------------------------------------------------------------------------------
// Ajoute szComputer dans la liste Computers de la section swSSO, sauf si déjà présent.
// ----------------------------------------------------------------------------------
int AddComputerName(const char *szComputer)
{
	TRACE((TRACE_ENTER,_F_, "szComputer=%s",szComputer));
	int rc=-1;
	char *p=NULL;
	char *pContexte=NULL;

	// on commence par vérifier qu'il n'est pas déjà là...
	GetPrivateProfileString("swSSO","Computers","",buf2048,sizeof(buf2048),gszCfgFile);
	p=strtok_s(buf2048,":",&pContexte);
	while (p!=NULL)
	{
		TRACE((TRACE_DEBUG,_F_,"Already present: %s",p));
		if (strcmp(p,szComputer)==0) goto end; // déjà présent, on arrête
		p=strtok_s(NULL,":",&pContexte);
	}
	// pas trouvé, on l'ajoute
	// mais attention, strtok a pété le buf2048, donc il faut le relire dans la config...
	GetPrivateProfileString("swSSO","Computers","",buf2048,sizeof(buf2048),gszCfgFile);
	strcat_s(buf2048,sizeof(buf2048),szComputer);
	strcat_s(buf2048,sizeof(buf2048),":");
	WritePrivateProfileString("swSSO","Computers",buf2048,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// HowManyComputerNames()
// ----------------------------------------------------------------------------------
static int HowManyComputerNames(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;
	char *p;

	GetPrivateProfileString("swSSO","Computers","",buf2048,sizeof(buf2048),gszCfgFile);
	p=strchr(buf2048,':');
	while (p!=NULL)
	{
		rc++;
		p=strchr(p+1,':');
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetNextComputerName()
// ----------------------------------------------------------------------------------
// Fournit la valeur suivante de computername pour le bouton "J'ai de la chance"
// ----------------------------------------------------------------------------------
char *GetNextComputerName(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pRet=NULL;

	// pour l'énumération des computernames...
	if (gpNextComputerName!=NULL) // c'est bon, on en a encore un en stock
	{
		pRet=gpNextComputerName;
		gpNextComputerName=strtok_s(NULL,":",&gpComputerNameContext);
	}
	else // on n'a pas encore énuméré ou alors on a épuisé le stock
	{
		GetPrivateProfileString("swSSO","Computers","",gszEnumComputerNames,sizeof(gszEnumComputerNames),gszCfgFile);
		gpNextComputerName=strtok_s(gszEnumComputerNames,":",&gpComputerNameContext);
		pRet=gpNextComputerName;
		gpNextComputerName=strtok_s(NULL,":",&gpComputerNameContext);
	}

	if (pRet==NULL) { TRACE((TRACE_LEAVE,_F_, "pRet=NULL")); }
	else			{ TRACE((TRACE_LEAVE,_F_, "pRet=%s",pRet)); }
	return pRet;
}

// ----------------------------------------------------------------------------------
// SaveLastConfigUpdate()
// ----------------------------------------------------------------------------------
// Sauvegarde la date de dernière mise à jour des configs depuis le serveur
// ----------------------------------------------------------------------------------
void SaveLastConfigUpdate()
{
	TRACE((TRACE_ENTER,_F_, ""));
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	sprintf_s(gszLastConfigUpdate,sizeof(gszLastConfigUpdate),"%04d%02d%02d%02d%02d%02d",
													localTime.wYear,localTime.wMonth,localTime.wDay,
													localTime.wHour,localTime.wMinute,localTime.wSecond);
	TRACE((TRACE_DEBUG,_F_,"gszLastConfigUpdate=%s",gszLastConfigUpdate));
	WritePrivateProfileString("swSSO","lastConfigUpdate",gszLastConfigUpdate,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	TRACE((TRACE_LEAVE,_F_, "")); 
}
// ----------------------------------------------------------------------------------
// SaveConfigHeader()
// ----------------------------------------------------------------------------------
// Ecriture de la section [swSSO] dans le fichier de config
// ----------------------------------------------------------------------------------
int SaveConfigHeader()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szPwdProtection[9+1]=""; // ISSUE#295
	char szItem[120+1]="";

	WritePrivateProfileString("swSSO","version",gcszCfgVersion,gszCfgFile);
	if (giPwdProtection==PP_ENCRYPTED)
		strcpy_s(szPwdProtection,sizeof(szPwdProtection),"ENCRYPTED");
	else if (giPwdProtection==PP_WINDOWS)
		strcpy_s(szPwdProtection,sizeof(szPwdProtection),"WINDOWS");
	WritePrivateProfileString("swSSO","pwdProtection",szPwdProtection,gszCfgFile);
	strcpy_s(gszCfgVersion,sizeof(gszCfgVersion),gcszCfgVersion);
	WritePrivateProfileString("swSSO","Portal",gszCfgPortal,gszCfgFile);

	WritePrivateProfileString("swSSO","sessionLock",gbSessionLock?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetCheckVersion",gbInternetCheckVersion?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetCheckBeta",gbInternetCheckBeta?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetGetConfig",gbInternetGetConfig?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetPutConfig",NULL,gszCfgFile); // 0.90B1
	WritePrivateProfileString("swSSO","internetManualPutConfig",gbInternetManualPutConfig?"YES":"NO",gszCfgFile);

	// 0.80B7 : sauvegarde de la config proxy liée au computername
	sprintf_s(szItem,sizeof(szItem),"internetUseProxy-%s",gszComputerName);
	WritePrivateProfileString("swSSO",szItem,gbInternetUseProxy?"YES":"NO",gszCfgFile);
	sprintf_s(szItem,sizeof(szItem),"ProxyURL-%s",gszComputerName);
	WritePrivateProfileString("swSSO",szItem,gszProxyURL,gszCfgFile);
	sprintf_s(szItem,sizeof(szItem),"ProxyUser-%s",gszComputerName);
	WritePrivateProfileString("swSSO",szItem,gszProxyUser,gszCfgFile);
	sprintf_s(szItem,sizeof(szItem),"ProxyPwd-%s",gszComputerName);
	// chiffrement du mot de passe proxy
	if (*gszProxyPwd==0) // pas de mot de passe
	{
		WritePrivateProfileString("swSSO",szItem,"",gszCfgFile);
	}
	else
	{
		char* pszEncryptedPassword=swCryptEncryptString(gszProxyPwd,ghKey1);
		if (pszEncryptedPassword!=NULL)  // si erreur de chiffrement du mot de passe, on ne le sauvegarde pas...
		{
			WritePrivateProfileString("swSSO",szItem,pszEncryptedPassword,gszCfgFile);
			free(pszEncryptedPassword); // 1.12B2-AC-TIE4
		}
	}

	// 0.80B9 : mémorise le nom de l'ordinateur (pour bouton "j'ai de la chance" dans config proxy)
	AddComputerName(gszComputerName);

	// 0.93B4 ISSUE#50 (?)
	WritePrivateProfileString("swSSO","parseWindowsOnStart",gbParseWindowsOnStart?"YES":"NO",gszCfgFile);

	// ISSUE#107
	WritePrivateProfileString("swSSO","displayChangeAppPwdDialog",gbDisplayChangeAppPwdDialog?"YES":"NO",gszCfgFile);

	// 1.03 : date de dernier changement mot de passe AD
	if (gbUseADPasswordForAppLogin)
	{
		// ISSUE#281 : n'écrit la valeur que si non vide
		// WritePrivateProfileString("swSSO","lastADPwdChange",gszLastADPwdChange,gszCfgFile);
		if (*gszLastADPwdChange!=0) WritePrivateProfileString("swSSO","lastADPwdChange",gszLastADPwdChange,gszCfgFile);
		if (*gszLastADPwdChange2!=0) WritePrivateProfileString("swSSO","lastADPwdChange2",gszLastADPwdChange2,gszCfgFile);
		WritePrivateProfileString("swSSO","ADPwd",gszEncryptedADPwd,gszCfgFile);
	}
	// ISSUE#176
	WritePrivateProfileString("swSSO","InternetExplorer",gbSSOInternetExplorer?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","Firefox",gbSSOFirefox?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","Chrome",gbSSOChrome?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","Edge",gbSSOEdge?"YES":"NO",gszCfgFile);
	// ISSUE#254
	WritePrivateProfileString("swSSO","ShowLaunchAppWithoutCtrl",gbShowLaunchAppWithoutCtrl?"YES":"NO",gszCfgFile);
	sprintf_s(szItem,sizeof(szItem),"%d",giLanguage);
	WritePrivateProfileString("swSSO","Language",szItem,gszCfgFile);
	// ISSUE#309
	sprintf_s(szItem,sizeof(szItem),"%d",giMasterPwdExpiration);
	WritePrivateProfileString("swSSO","MasterPwdExpiration",szItem,gszCfgFile);
	// ISSUE#164
	StoreIniEncryptedHash(); 
	rc=0;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// StoreMasterPwd()
// ----------------------------------------------------------------------------------
// Inscription du mot de passe maitre dans section [swSSO]
// ATTENTION : le sel doit avoir été tiré avant l'appel à cette fonction
// ----------------------------------------------------------------------------------
int StoreMasterPwd(const char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szPBKDF2PwdSalt[PBKDF2_SALT_LEN*2+1];
	char szPBKDF2KeySalt[PBKDF2_SALT_LEN*2+1];
	char szPBKDF2ConfigPwd[PBKDF2_PWD_LEN*2+1];
	BYTE PBKDF2ConfigPwd[PBKDF2_PWD_LEN];
	
	HCURSOR hCursorOld=SetCursor(ghCursorWait);

	// (le sel doit avoir été tiré avant l'appel à StoreMasterPwd !)
	if (!swIsPBKDF2PwdSaltReady()) { TRACE((TRACE_ERROR,_F_,"swIsPBKDF2SaltReady()=FALSE")); goto end; }

	// calcule le hash du nouveau mot de passe maitre 
	if (swPBKDF2(PBKDF2ConfigPwd,sizeof(PBKDF2ConfigPwd),szPwd,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;

	// encodage base64 et stockage des sels et du mot de passe dans le fichier .ini
	swCryptEncodeBase64(gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,szPBKDF2KeySalt);
	WritePrivateProfileString("swSSO","keySalt",szPBKDF2KeySalt,gszCfgFile);
	if (giPwdProtection==PP_ENCRYPTED)
	{
		swCryptEncodeBase64(gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,szPBKDF2PwdSalt);
		swCryptEncodeBase64(PBKDF2ConfigPwd,sizeof(PBKDF2ConfigPwd),szPBKDF2ConfigPwd);
		WritePrivateProfileString("swSSO","pwdSalt",szPBKDF2PwdSalt,gszCfgFile);
		WritePrivateProfileString("swSSO","pwdValue",szPBKDF2ConfigPwd,gszCfgFile);
	}
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DPAPIStoreMasterPwd()
// ----------------------------------------------------------------------------------
// Nouveau en v0.76 -> stockage du mdp maitre sécurisé par DPAPI
// ----------------------------------------------------------------------------------
int DPAPIStoreMasterPwd(const char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	BOOL brc;
	int rc=-1;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataSalt;
	char *pszBase64=NULL;
	char szKey[MAX_COMPUTERNAME_LENGTH+UNLEN+56+1]="";

	DataIn.pbData=(BYTE*)szPwd;
	DataIn.cbData=strlen(szPwd)+1;
	DataOut.pbData=NULL;
	DataOut.cbData=0;

	sprintf_s(szKey,sizeof(szKey),"pwdDAPIValue-%s@%s",gszUserName,gszComputerName);
	DataSalt.pbData=(BYTE*)szKey;
	DataSalt.cbData=strlen(szKey);

	// chiffrement du mot de passe (pour cette machine seulement)
	//brc = CryptProtectData(&DataIn,L"swSSO",&DataSalt,NULL,NULL,CRYPTPROTECT_LOCAL_MACHINE,&DataOut); 0.90B1 
	brc = CryptProtectData(&DataIn,L"swSSO",&DataSalt,NULL,NULL,0,&DataOut);
	if (!brc) 
	{
		TRACE((TRACE_ERROR,_F_, "CryptProtectData()"));
		goto end;
	}
	
	// encodage base64 et écriture dans swsso.ini
	pszBase64=(char*)malloc(DataOut.cbData*2+1);
	if (pszBase64==NULL) goto end;
	swCryptEncodeBase64(DataOut.pbData,DataOut.cbData,pszBase64);

	WritePrivateProfileString("swSSO",szKey,pszBase64,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	if (pszBase64!=NULL) free (pszBase64);
	if (DataOut.pbData!=NULL) LocalFree(DataOut.pbData);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DPAPIGetMasterPwd()
// ----------------------------------------------------------------------------------
// Nouveau en v0.76 -> lecture du mdp maitre sécurisé par DPAPI
// ----------------------------------------------------------------------------------
int DPAPIGetMasterPwd(char *pszPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szBase64[1024+1];
	char DAPIData[512];
	char szKey[20+MAX_COMPUTERNAME_LENGTH+UNLEN+1]="";
	
	BOOL brc;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataSalt;

	DataOut.pbData=NULL;
	DataOut.cbData=0;

	// lecture dans swsso.ini et décodage base64 
	sprintf_s(szKey,sizeof(szKey),"pwdDAPIValue-%s@%s",gszUserName,gszComputerName);
	DataSalt.pbData=(BYTE*)szKey;
	DataSalt.cbData=strlen(szKey);
	GetPrivateProfileString("swSSO",szKey,"",szBase64,sizeof(szBase64),gszCfgFile);
	if (*szBase64==0) goto end;
	swCryptDecodeBase64(szBase64,DAPIData,sizeof(DAPIData));
	DataIn.pbData=(BYTE*)DAPIData;
	DataIn.cbData=strlen(szBase64)/2;

	// déchiffrement du mot de passe 
	brc = CryptUnprotectData(&DataIn,NULL,&DataSalt,NULL,NULL,0,&DataOut);
	if (!brc) 
	{
		TRACE((TRACE_ERROR,_F_, "CryptUnprotectData()"));
		goto end;
	}

	strcpy_s(pszPwd,LEN_PWD+1,(char*)DataOut.pbData);
	rc=0;
end:
	if (DataOut.pbData!=NULL) LocalFree(DataOut.pbData);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GenWriteCheckSynchroValue()
//-----------------------------------------------------------------------------
// Génère et écrit une valeur checksyncho dans swsso.ini
//-----------------------------------------------------------------------------
int GenWriteCheckSynchroValue(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	BYTE bufSynchroValue[16+64+16]; // iv + données utiles + padding
	char szSynchroValue[192+1]; // (16+64+16)*2+1 = 193
	
	// Génère un aléa pour l'iv et les données à chiffrer
	brc=CryptGenRandom(ghProv,16+64,bufSynchroValue);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()=%ld",GetLastError())); goto end; }
	// Chiffre  avec la clé ghKey1
	if (swCryptEncryptData(bufSynchroValue,bufSynchroValue+16,64,ghKey1)!=0) goto end;
	// Encode en faux base 64
	swCryptEncodeBase64(bufSynchroValue,16+64+16,szSynchroValue);
	// Ecrit dans le .ini
	WritePrivateProfileString("swSSO","CheckSynchro",szSynchroValue,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ReadVerifyCheckSynchroValue()
//-----------------------------------------------------------------------------
// Lit et vérifie la valeur checksynchro
// Vérifie que la clé permet bien de déchiffrer les mot de passe maitre
// En version SSO WINDOWS, c'est le moyen de contrôler que le mot de passe Windows
// est bien synchronisé avec la clé de chiffrement des mdp secondaires de swSSO
//-----------------------------------------------------------------------------
int ReadVerifyCheckSynchroValue(int iKeyId)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;
	
	BYTE bufSynchroValue[16+64+16]; // iv + données utiles + padding
	char szSynchroValue[192+1]; // (16+64+16)*2+1 = 193

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (!gAESKeyInitialized[iKeyId]) { TRACE((TRACE_ERROR,_F_,"AESKey(%d) not initialized",iKeyId)); goto end; }

	// Lit dans le .ini
	GetPrivateProfileString("swSSO","CheckSynchro","",szSynchroValue,sizeof(szSynchroValue),gszCfgFile);
	if (*szSynchroValue==0) // pas de synchro value ?!?
	{
		TRACE((TRACE_ERROR,_F_,"GetPrivateProfileString(CheckSynchro) -> valeur absente !"));
		goto end;
	}
	// Décode le faux base 64
	swCryptDecodeBase64(szSynchroValue,(char*)bufSynchroValue,sizeof(bufSynchroValue));
	// Chiffre  avec la clé ghKey1
	if (swCryptDecryptDataAES256(bufSynchroValue,bufSynchroValue+16,80,iKeyId)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur de déchiffrement, cas de désynchro !"));
		giBadPwdCount++;
		goto end;
	}
		
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// InitWindowsSSO()
//-----------------------------------------------------------------------------
// Appelée lors du premier lancement pour un nouvel utilisateur (ini inexistant)
//-----------------------------------------------------------------------------
int InitWindowsSSO(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	BYTE AESKeyData[AES256_KEY_LEN];
	char bufRequest[1280];
	char bufResponse[1024];
	DWORD dwLenResponse;
	char szPBKDF2Salt[PBKDF2_SALT_LEN*2+1];
	
	// Génère les sels PSKS
	if (swGenPBKDF2Salt()!=0) goto end;

	// Envoie les sels à swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:PUTPSKS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,gszUPN,strlen(gszUPN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN*2,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	// TODO un jour : analyser la réponse
	// Ecrit les sels dans le .ini
	swCryptEncodeBase64(gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,szPBKDF2Salt);
	WritePrivateProfileString("swSSO","pwdSalt",szPBKDF2Salt,gszCfgFile);
	swCryptEncodeBase64(gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,szPBKDF2Salt);
	WritePrivateProfileString("swSSO","keySalt",szPBKDF2Salt,gszCfgFile);

	// Demande le keydata à swssosvc
	// Construit la requête à envoyer à swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	if (dwLenResponse!=PBKDF2_PWD_LEN+AES256_KEY_LEN)
	{
		TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld (attendu=%d)",dwLenResponse,PBKDF2_PWD_LEN+AES256_KEY_LEN)); goto end;
	}
	// Crée la clé de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	swStoreAESKey(AESKeyData,ghKey1);
	if (GenWriteCheckSynchroValue()!=0) goto end;
	rc=0;
end:
	if (rc!=0) MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONSTOP);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// MigrationWindowsSSO()
//-----------------------------------------------------------------------------
// Active la synchronisation mot de passe swSSO / mot de passe Windows
//-----------------------------------------------------------------------------
int MigrationWindowsSSO(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	BYTE AESKeyData[AES256_KEY_LEN];
	char bufRequest[1280];
	char bufResponse[1024];
	DWORD dwLenResponse;
	char szKey[20+MAX_COMPUTERNAME_LENGTH+UNLEN+1]="";

	// Lecture des sels
	if (swReadPBKDF2Salt()!=0) goto end;

	// Envoie les sels à swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:PUTPSKS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,gszUPN,strlen(gszUPN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN*2,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	// TODO un jour : analyser la réponse

	// Demande le keydata à swssosvc
	// Construit la requête à envoyer à swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	if (dwLenResponse!=PBKDF2_PWD_LEN+AES256_KEY_LEN)
	{
		TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld",dwLenResponse)); goto end;
	}
	// Crée la clé de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	if (swStoreAESKey(AESKeyData,ghKey2)!=0) goto end;

	// Fait le transchiffrement du fichier .ini de ghKey1 vers ghKey2
	if (swTranscrypt()!=0) goto end; 
	
	// Copie la clé dans ghKey1 qui est utilisée pour chiffrer les mots de passe secondaires
	swStoreAESKey(AESKeyData,ghKey1);
	// enregistrement des actions, comme ça les identifiants sont rechiffrés automatiquement avec la nouvelle clé
	if(SaveApplications()!=0) goto end;
	
	// Supprime le pwdvalue, on n'en a plus besoin :-)
	WritePrivateProfileString("swSSO","pwdValue",NULL,gszCfgFile);
	WritePrivateProfileString("swSSO","pwdProtection","WINDOWS",gszCfgFile);
	// Supprime l'éventuelle clé pwdadpivalue
	sprintf_s(szKey,sizeof(szKey),"pwdDAPIValue-%s@%s",gszUserName,gszComputerName);
	WritePrivateProfileString("swSSO",szKey,NULL,gszCfgFile);
	// Et par contre on crée une nouvelle valeur CheckSynchro = aléa chiffré puis jeté
	if (GenWriteCheckSynchroValue()!=0) goto end;
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	if (rc!=0) MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONSTOP);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// CheckWindowsPwd()
//-----------------------------------------------------------------------------
// Vérifie le mot de passe Windows et crée la clé de chiffrement des mdp
// secondaires (ghKey1)
// rc=0 OK, rc=-1 erreur ou annulation, rc=-3 recouvrement avec clic sur bouton continuer
//-----------------------------------------------------------------------------
int CheckWindowsPwd(BOOL *pbMigrationWindowsSSO)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BYTE AESKeyData[AES256_KEY_LEN];
	char bufRequest[1280];
	char bufResponse[1024];
	DWORD dwLenResponse;
	int rc=-1;
	BOOL bMustReopenSession=FALSE;
	BOOL bMustReboot=FALSE;

	// Lecture des sels
	if (swReadPBKDF2Salt()!=0) goto end;

	// Envoie les sels à swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:PUTPSKS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,gszUPN,strlen(gszUPN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN*2+PBKDF2_SALT_LEN*2,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		bMustReboot=TRUE;
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	// TODO un jour : analyser la réponse

	// Demande le keydata à swssosvc
	// Construit la requête à envoyer à swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		bMustReboot=TRUE;
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	if (dwLenResponse!=PBKDF2_PWD_LEN+AES256_KEY_LEN)
	{
		bMustReopenSession=TRUE;
		TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld",dwLenResponse)); goto end;
	}
	// Crée la clé de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	if (swStoreAESKey(AESKeyData,ghKey1)) goto end;
	
	// Teste que la clé est OK, sinon essaie avec la précédente clé si existante et 
	// transchiffre le fichier si OK : V02:GETPHKD:OLD:domain(256octets)username(256octets)
	if (ReadVerifyCheckSynchroValue(ghKey1)!=0)
	{
		TRACE((TRACE_INFO,_F_,"ReadVerifyCheckSynchroValue(CUR) failed"));
		// ISSUE#156 : pour y voir plus clair dans les traces
		SecureZeroMemory(bufRequest,sizeof(bufRequest));
		memcpy(bufRequest,"V02:GETPHKD:OLD:",16);
		memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
		memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,USER_LEN);
		if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
		{
			TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()"));
			goto end;
		}
		if (dwLenResponse!=PBKDF2_PWD_LEN+AES256_KEY_LEN)
		{
			TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld",dwLenResponse));
			goto end;
		}
		// Crée la clé de chiffrement des mots de passe secondaires
		memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
		if (swStoreAESKey(AESKeyData,ghKey1)) goto end;
		if (ReadVerifyCheckSynchroValue(ghKey1)==0)
		{
			TRACE((TRACE_INFO,_F_,"Un transchiffrement va être effectué"));
			*pbMigrationWindowsSSO=TRUE; // demande la migration (sera faite une fois les configs chargées)
		}
		else
		{
			// Ni le mot de passe courant ni l'ancien ne permettent de déchiffrer...
			// On est dans un cas de désynchro, l'utilisateur doit faire appel 
			// au support pour resynchroniser son mot de passe.
			TRACE((TRACE_ERROR,_F_,"ReadVerifyCheckSynchroValue(CUR and OLD) failed..."));
			goto end;
		}
	}

	rc=0;
end:
	if (rc!=0) 
	{
		if (bMustReboot) // ISSUE#186
		{
			MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_LOGON),"swSSO",MB_OK | MB_ICONSTOP);
		}
		else if (bMustReopenSession) // ISSUE#186
		{
			MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_LOGON3),"swSSO",MB_OK | MB_ICONSTOP);
		}
		else 
		{
			MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_LOGON),"swSSO",MB_OK | MB_ICONSTOP);
		}
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// CheckMasterPwd()
// ----------------------------------------------------------------------------------
// Vérification du mot de passe maitre
// ----------------------------------------------------------------------------------
int CheckMasterPwd(const char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	//TRACE((TRACE_PWD,_F_, "pwd=%s",szPwd));
	int rc=-1;
	
	char szPBKDF2ConfigPwd[PBKDF2_PWD_LEN*2+1];
	BYTE PBKDF2ConfigPwd[PBKDF2_PWD_LEN];
	BYTE PBKDF2UserPwd[PBKDF2_PWD_LEN];

	HCURSOR hCursorOld=SetCursor(ghCursorWait);

	if (giBadPwdCount>5) goto end;
	
	// Lecture des sels
	if (swReadPBKDF2Salt()!=0) goto end;
	// Lecture du mot de passe
	GetPrivateProfileString("swSSO","pwdValue","",szPBKDF2ConfigPwd,sizeof(szPBKDF2ConfigPwd),gszCfgFile);
	if (strlen(szPBKDF2ConfigPwd)!=PBKDF2_PWD_LEN*2) goto end;
	swCryptDecodeBase64(szPBKDF2ConfigPwd,(char*)PBKDF2ConfigPwd,sizeof(PBKDF2ConfigPwd));
	TRACE_BUFFER((TRACE_DEBUG,_F_,PBKDF2ConfigPwd,sizeof(PBKDF2ConfigPwd),"PBKDF2ConfigPwd"));

	// calcul du hash
	if (swPBKDF2((BYTE*)PBKDF2UserPwd,sizeof(PBKDF2UserPwd),szPwd,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,PBKDF2UserPwd,sizeof(PBKDF2UserPwd),"PBKDF2UserPwd"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,PBKDF2ConfigPwd,sizeof(PBKDF2ConfigPwd),"PBKDF2UserPwd"));

	// comparaison
	if (memcmp(PBKDF2UserPwd,PBKDF2ConfigPwd,PBKDF2_PWD_LEN)!=0)
	{
		TRACE((TRACE_INFO,_F_,"Mot de passe incorrect"));
		giBadPwdCount++;
		goto end;
	}
	rc=0;
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ChangeMasterPasswordDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de changement du mot de passe maitre
//-----------------------------------------------------------------------------
static int CALLBACK ChangeMasterPasswordDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			gwChangeMasterPwd=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			//SendMessage(GetDlgItem(w,TB_OLD_PWD),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			//SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			//SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			SendMessage(GetDlgItem(w,TB_OLD_PWD),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
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
				//SetFocus(w); ATTENTION, REMET LE FOCUS SUR LE MDP ET FOUT LA MERDE SI SAISIE DEJA COMMENCEE !
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
					char szOldPwd[LEN_PWD+1];
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					GetDlgItemText(w,TB_OLD_PWD,szOldPwd,sizeof(szOldPwd));
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
					if (CheckMasterPwd(szOldPwd)!=0)
					{
						swLogEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_PWD_CHANGE_BAD_PWD,NULL,NULL,NULL,NULL,0);
						SendDlgItemMessage(w,TB_OLD_PWD,EM_SETSEL,0,-1);
						MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						if (giBadPwdCount>5) PostQuitMessage(-1);
					}
					else
					{
						giBadPwdCount=0;
			
						if (ChangeMasterPwd(szNewPwd1)!=0)
						{
							MessageBox(w,GetString(IDS_CHANGE_PWD_FAILED),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							MessageBox(w,GetString(IDS_CHANGE_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
							EndDialog(w,IDOK);
						}
					}
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_OLD_PWD:
				case TB_NEW_PWD1:
				case TB_NEW_PWD2:
				{
					char szOldPwd[LEN_PWD+1];
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					int lenOldPwd,lenNewPwd1,lenNewPwd2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						BOOL bOK;
						lenOldPwd=GetDlgItemText(w,TB_OLD_PWD,szOldPwd,sizeof(szOldPwd));
						lenNewPwd1=GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
						lenNewPwd2=GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
						bOK=FALSE;
						if (lenOldPwd!=0 && lenNewPwd1!=0 && lenNewPwd1==lenNewPwd2)
						{
							bOK=((strcmp(szNewPwd1,szNewPwd2)==0 && strcmp(szNewPwd1,szOldPwd)) ? TRUE : FALSE);
						}
						EnableWindow(GetDlgItem(w,IDOK),bOK);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
		case WM_ACTIVATE:
			InvalidateRect(w,NULL,FALSE);
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// ForceChangeMasterPasswordDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de changement IMPOSE du mot de passe maitre
// Utilisée aussi pour demander un nouveau mot de passe suite à un recovery
//-----------------------------------------------------------------------------
static int CALLBACK ForceChangeMasterPasswordDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			gwChangeMasterPwd=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
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
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
			
					// vérifie que pas égal au mot de passe actuel
					if (CheckMasterPwd(szNewPwd1)==0)
					{
						MessageBox(w,GetString(IDS_FORCE_CHANGE),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemText(w,TB_NEW_PWD1,"");
						SetDlgItemText(w,TB_NEW_PWD2,"");
						SetFocus(GetDlgItem(w,TB_NEW_PWD1));
					}
					else
					{
						if (ChangeMasterPwd(szNewPwd1)!=0)
						{
							MessageBox(w,GetString(IDS_CHANGE_PWD_FAILED),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							MessageBox(w,GetString(IDS_CHANGE_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
							EndDialog(w,IDOK);
						}
					}
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_NEW_PWD1:
				case TB_NEW_PWD2:
				{
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					int lenNewPwd1,lenNewPwd2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						BOOL bOK;
						lenNewPwd1=GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
						lenNewPwd2=GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
						bOK=FALSE;
						if (lenNewPwd1!=0 && lenNewPwd1==lenNewPwd2)
						{
							bOK=((strcmp(szNewPwd1,szNewPwd2)==0) ? TRUE : FALSE);
						}
						EnableWindow(GetDlgItem(w,IDOK),bOK);
					}
					break;
				}
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

// ----------------------------------------------------------------------------------
// WindowChangeMasterPwd()
// ----------------------------------------------------------------------------------
// Fenêtre de changement du mot de passe maitre
// ----------------------------------------------------------------------------------
int WindowChangeMasterPwd(BOOL bForced)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	if (giBadPwdCount>5) 
	{
		PostQuitMessage(-1);
		goto end;
	}

	if (gwChangeMasterPwd!=NULL) 
	{
		SetForegroundWindow(gwChangeMasterPwd);
		goto end;
	}
	DialogBox(ghInstance,MAKEINTRESOURCE(IDD_CHANGE_PWD),NULL,ChangeMasterPasswordDialogProc);
	rc=0;
	gwChangeMasterPwd=NULL;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetMasterPwdLastChange()
// ----------------------------------------------------------------------------------
// Récupère la date du dernier changement du mot de passe maitre
// ----------------------------------------------------------------------------------
long GetMasterPwdLastChange(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	long lrc=0; // y'a longtemps...

	char *pszDecryptedTime=NULL;
	char szEncryptedTime[LEN_ENCRYPTED_AES256+1];

	GetPrivateProfileString("swSSO","LastPwdChange","",szEncryptedTime,sizeof(szEncryptedTime),gszCfgFile);
	if (*szEncryptedTime==0) // pas de ligne changement de mot de passe...
	{
		//on suppose que le vilain utilisateur a supprimé la ligne et on impose changement immédiat ?
		// effet de bord : pour les utilisateurs actuels, changement imposé dès activation de la politique.
		goto end;
	}
	pszDecryptedTime=swCryptDecryptString(szEncryptedTime,ghKey1);
	if (pszDecryptedTime==NULL) goto end;
	lrc=atoi(pszDecryptedTime);
end:
	if (pszDecryptedTime!=NULL) free(pszDecryptedTime);
	TRACE((TRACE_LEAVE,_F_, "lrc=%d",lrc));
	return lrc;
}

// ----------------------------------------------------------------------------------
// ChangeWindowsPwd()
// ----------------------------------------------------------------------------------
// Le mot de passe Windows a changé, répercute le changement
// ----------------------------------------------------------------------------------
int ChangeWindowsPwd(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BYTE AESKeyData[AES256_KEY_LEN];
	char bufRequest[1024];
	char bufResponse[1024];
	DWORD dwLenResponse;
	
	// Demande le nouveau keydata à swssosvc
	// Construit la requête à envoyer à swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)==0) 
	{
		if (dwLenResponse==PBKDF2_PWD_LEN+AES256_KEY_LEN)
		{
			// Crée la clé de chiffrement des mots de passe secondaires
			memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
			swStoreAESKey(AESKeyData,ghKey2);
			// Fait le transchiffrement du fichier .ini de ghKey1 vers ghKey2
			if (swTranscrypt()!=0) goto end;
			// Copie la clé dans ghKey1 qui est utilisée pour chiffrer les mots de passe secondaires
			swStoreAESKey(AESKeyData,ghKey1);
			// enregistrement des actions, comme ça les identifiants sont rechiffrés automatiquement avec la nouvelle clé
			if(SaveApplications()!=0) goto end;
			// Met à jour la valeur de checksynchro
			if (GenWriteCheckSynchroValue()!=0) goto end;
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld",dwLenResponse));
			goto end;
		}
	}
	else
	{ 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()"));
		goto end;
	}

	rc=0;
end:
	if (rc==0)
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_PRIMARY_PWD_CHANGE_SUCCESS,NULL,NULL,NULL,NULL,0);
	else
		swLogEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_PWD_CHANGE_ERROR,NULL,NULL,NULL,NULL,0);

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// ChangeMasterPwd()
// ----------------------------------------------------------------------------------
// Changement du mot de passe maitre
// ----------------------------------------------------------------------------------
int ChangeMasterPwd(const char *szNewPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char *pszDecryptedPwd=NULL;
	char *pszTranscryptedPwd=NULL;	

	// génère de nouveaux sels (ben oui, autant le changer en même temps que le mot de passe maitre)
	// qui seront pris en compte pour la dérivation de la clé AES et le stockage du mot de passe
	if (swGenPBKDF2Salt()!=0) goto end;

	// dérivation de ghKey2 à partir du nouveau mot de passe
	if (swCryptDeriveKey(szNewPwd,ghKey2)!=0) goto end;

	// Transchiffrement
	if (swTranscrypt()!=0) goto end; 

	// dérivation de ghKey1 à partir du nouveau mot de passe pour la suite
	if(swCryptDeriveKey(szNewPwd,ghKey1)!=0) goto end;
	
	// enregistrement du nouveau mot maitre dans swSSO.ini
	if(StoreMasterPwd(szNewPwd)!=0) goto end;

	// si rememberme, stockage protégé DAPI
	if (gbRememberOnThisComputer)
	{
		if (DPAPIStoreMasterPwd(szNewPwd)!=0) goto end;
	}

	// enregistrement des actions, comme ça les identifiants sont rechiffrés automatiquement avec la nouvelle clé
	if(SaveApplications()!=0) goto end;
	
	rc=0;
end:
	if (rc==0)
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_PRIMARY_PWD_CHANGE_SUCCESS,NULL,NULL,NULL,NULL,0);
	else
		swLogEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_PWD_CHANGE_ERROR,NULL,NULL,NULL,NULL,0);

	if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
	if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// LogTranscryptError()
//-----------------------------------------------------------------------------
// Loggue les erreurs de transchiffrement dans un fichier portant le même nom
// et créé dans le même dossier que le fichier .ini, mais avec extension .err
//-----------------------------------------------------------------------------
int LogTranscryptError(char *szLogMessage)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HANDLE hf=INVALID_HANDLE_VALUE; 
	char szFilename[_MAX_PATH+1];
	char *p;
	DWORD dw;

	// nom du fichier : swsso.ini -> swsso.log
	strcpy_s(szFilename,sizeof(szFilename),gszCfgFile);
	p=strrchr(szFilename,'.');
	if (p==NULL) { TRACE((TRACE_ERROR,_F_,"gszCfgFile=%s",gszCfgFile)); goto end; }
	memcpy(p+1,"err",4);

	// écrit le message dans le fichier log
	hf=CreateFile(szFilename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFilename,GetLastError())); goto end; }
	// se positionne à la fin
	SetFilePointer(hf,0,0,FILE_END);
	// ecrit
	if (!WriteFile(hf,szLogMessage,strlen(szLogMessage),&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile()=%d",GetLastError())); goto end; }

	rc=0;
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// swTranscrypt()
// ----------------------------------------------------------------------------------
// Transchiffre le fichier swSSO.ini de ghKey1 vers ghKey2
// ----------------------------------------------------------------------------------
int swTranscrypt(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int i;
	char *pszDecryptedPwd=NULL;
	char *pszTranscryptedPwd=NULL;
	int lenEncryptedProxyPwd;
	char szEncryptedProxyPwd[LEN_ENCRYPTED_AES256+1]; 
	char *pszComputerName;
	char szItem[120+1]="";
	char szMessage[1024+1];
	
	// parcours de la table des actions et transchiffrement
	for (i=0;i<giNbActions;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"Action[%d](%s)",i,gptActions[i].szApplication));
		if (*gptActions[i].szPwdEncryptedValue==0) goto suite;
		// déchiffrement mot de passe
		pszDecryptedPwd=swCryptDecryptString(gptActions[i].szPwdEncryptedValue,ghKey1);
		if (pszDecryptedPwd==NULL)
		{
			// goto end; // ISSUE#276 : en cas d'erreur, génère un log, marque l'entrée comme invalide et continue
			*gptActions[i].szPwdEncryptedValue=0;
			sprintf_s(szMessage,sizeof(szMessage),"Erreur de tranchiffrement du mot de passe de la config #%d (%s)\r\n",i,gptActions[i].szApplication);
			LogTranscryptError(szMessage);
			if (!gptActions[i].bError) 
			{ 
				giNbTranscryptError++; // incrément seulement si pas déjà marquée en erreur à cause de l'identifiant
				gptActions[i].bError=TRUE;
			}
			goto suite;
		}
		// rechiffrement avec la nouvelle clé
		pszTranscryptedPwd=swCryptEncryptString(pszDecryptedPwd,ghKey2);
		SecureZeroMemory(pszDecryptedPwd,strlen(pszDecryptedPwd));
		if (pszTranscryptedPwd==NULL)
		{
			// goto end; // ISSUE#276 : en cas d'erreur, génère un log, marque l'entrée comme invalide et continue
			*gptActions[i].szPwdEncryptedValue=0;
			sprintf_s(szMessage,sizeof(szMessage),"Erreur de tranchiffrement du mot de passe de la config #%d (%s)\r\n",i,gptActions[i].szApplication);
			LogTranscryptError(szMessage);
			if (!gptActions[i].bError) 
			{ 
				giNbTranscryptError++; // incrément seulement si pas déjà marquée en erreur à cause de l'identifiant
				gptActions[i].bError=TRUE;
			}
			goto suite;
		}
		strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszTranscryptedPwd);
suite:
		if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
		if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
	}

	// 0.83B2 : transchiffrement de TOUS les mots de passe proxy !
	for (i=0;i<HowManyComputerNames();i++)
	{
		// récup du prochain computer name
		pszComputerName=GetNextComputerName();
		// lecture de la valeur du mdp proxy pour ce computername
		sprintf_s(szItem,sizeof(szItem),"ProxyPwd-%s",pszComputerName);
		lenEncryptedProxyPwd=GetPrivateProfileString("swSSO",szItem,"",szEncryptedProxyPwd,sizeof(szEncryptedProxyPwd),gszCfgFile);
		TRACE((TRACE_INFO,_F_,"ProxyPwd-%s : len=%d",pszComputerName,lenEncryptedProxyPwd));
		if (lenEncryptedProxyPwd==LEN_ENCRYPTED_AES256) // mot de passe chiffré. Dans tous les autres cas (pas de mot de passe ou non chiffré, on ne fait rien !)
		{
			pszDecryptedPwd=swCryptDecryptString(szEncryptedProxyPwd,ghKey1);
			if (pszDecryptedPwd==NULL) goto suitead;
			//TRACE((TRACE_PWD,_F_,"pszDecryptedPwd=%s",pszDecryptedPwd));
			// déchiffrement OK avec ghKey1, on rechiffre avec ghKey2
			pszTranscryptedPwd=swCryptEncryptString(pszDecryptedPwd,ghKey2);
			// 0.85B9 : remplacement de memset(pszDecryptedPwd,0,strlen(pszDecryptedPwd));
			SecureZeroMemory(pszDecryptedPwd,strlen(pszDecryptedPwd));
			if (pszTranscryptedPwd==NULL) goto suitead;
			// écriture dans le fichier ini
			WritePrivateProfileString("swSSO",szItem,pszTranscryptedPwd,gszCfgFile);
			StoreIniEncryptedHash(); // ISSUE#164
		}
		if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
		if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
	}
suitead:
	// 1.03 - transchiffre aussi le mot de passe AD, si présent
	if (gbUseADPasswordForAppLogin)
	{
		if (*gszEncryptedADPwd!=0)
		{
			// déchiffrement mot de passe
			pszDecryptedPwd=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
			if (pszDecryptedPwd==NULL) goto bientotlafin;
			// rechiffrement avec la nouvelle clé
			pszTranscryptedPwd=swCryptEncryptString(pszDecryptedPwd,ghKey2);
			SecureZeroMemory(pszDecryptedPwd,strlen(pszDecryptedPwd));
			if (pszTranscryptedPwd==NULL) goto bientotlafin;
			strcpy_s(gszEncryptedADPwd,sizeof(gszEncryptedADPwd),pszTranscryptedPwd);
			if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
			if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
		}
	}
bientotlafin:
	rc=0;
//end:
	if (giNbTranscryptError!=0)
	{
		sprintf_s(szMessage,GetString(IDS_TRANSCRYPT_ERROR),giNbTranscryptError);
		MessageBox(NULL,szMessage,"swSSO",MB_OK | MB_ICONEXCLAMATION);
	}
	if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
	if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ChangeApplicationPasswordDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de changement de mot de passe d'application
//-----------------------------------------------------------------------------
static int CALLBACK ChangeApplicationPasswordDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	CheckIfQuitMessage(msg);
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			gwChangeApplicationPassword=w;
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			//SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			//SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
			//SetDlgItemText(w,TX_APP_NAME,gptActions[lp].szApplication);
			wsprintf(buf2048,GetString(IDS_NEW_APP_PWD),gptActions[lp].szApplication);
			SetDlgItemText(w,TX_FRAME,buf2048);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			//
			SetWindowLong(w,DWL_USER,lp);
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
				//SetFocus(w); ATTENTION, REMET LE FOCUS SUR LE MDP ET FOUT LA MERDE SI SAISIE DEJA COMMENCEE !
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					int iAction=GetWindowLong(w,DWL_USER);
					TRACE((TRACE_DEBUG,_F_, "WM_COMMAND -> IDOK (action=%ld)",iAction));
					char szNewPwd1[LEN_PWD+1];
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					// chiffre le mot de passe
					char *pszEncryptedPassword=swCryptEncryptString(szNewPwd1,ghKey1);
					SecureZeroMemory(szNewPwd1,strlen(szNewPwd1));
					if (pszEncryptedPassword!=NULL)
					{
						strcpy_s(gptActions[iAction].szPwdEncryptedValue,sizeof(gptActions[iAction].szPwdEncryptedValue),pszEncryptedPassword);
						free(pszEncryptedPassword);
					}
					WritePrivateProfileString(gptActions[iAction].szApplication,"pwdValue",gptActions[iAction].szPwdEncryptedValue,gszCfgFile);
					StoreIniEncryptedHash(); // ISSUE#164
					EndDialog(w,IDOK);
					break;
				}
				case IDCANCEL:
					TRACE((TRACE_DEBUG,_F_, "WM_COMMAND -> IDCANCEL"));
					EndDialog(w,IDCANCEL);
					break;
				case TB_NEW_PWD1:
				case TB_NEW_PWD2:
				{
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					int lenNewPwd1,lenNewPwd2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						BOOL bOK;
						lenNewPwd1=GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
						lenNewPwd2=GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
						bOK=FALSE;
						if (lenNewPwd1!=0 && lenNewPwd1==lenNewPwd2)
						{
							bOK=((strcmp(szNewPwd1,szNewPwd2)==0) ? TRUE : FALSE);
						}
						EnableWindow(GetDlgItem(w,IDOK),bOK);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
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
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// SavePortal()
// ----------------------------------------------------------------------------------
// Génération du fichier XML portail (0.78)
// Remplacé en 1.22 par la génération d'un fichier JSON pour swSSO Mobile
// ----------------------------------------------------------------------------------
void SavePortal()
{
	SaveJSON(gszCfgPortal);
}

// ----------------------------------------------------------------------------------
// ReactivateApplicationFromCurrentWindow()
// ----------------------------------------------------------------------------------
// Réinitialise les flags divers de l'application actuellement affichée à l'écran
// ----------------------------------------------------------------------------------
void ReactivateApplicationFromCurrentWindow(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HWND w;
	char szTitle[512+1];
	int lenTitle;
	int i;
	char szClassName[128+1];
	char *pszURL=NULL;

	pszURL=swGetTopWindowWithURL(&w,szTitle,sizeof(szTitle));
	if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"Top Window non trouvee !")); goto end; }
	free(pszURL);pszURL=NULL;

	GetClassName(w,szClassName,sizeof(szClassName));
    TRACE((TRACE_DEBUG,_F_,"szClassName          =%s",szClassName));
	TRACE((TRACE_DEBUG,_F_,"szTitle              =%s",szTitle));

	for (i=0;i<giNbActions;i++)
	{
		// ISSUE#215 : nouvelle gestion des popups pour Chrome 36+ (c'était merdique avant je ne conserve pas la compatibilité)	
		if (gptActions[i].iType==POPSSO && strncmp(szClassName,"Chrome_WidgetWin_",17)==0) pszURL=GetChromePopupURL(w);
		if (pszURL!=NULL)
		{
			if (swStringMatch(pszURL,gptActions[i].szURL))
			{
				//gptActions[i].wLastDetect=NULL;
				//gptActions[i].tLastDetect=-1;
				LastDetect_RemoveWindow(w);
				gptActions[i].tLastSSO=-1;
				gptActions[i].wLastSSO=NULL;
				gptActions[i].iWaitFor=giWaitBeforeNewSSO;
				gptActions[i].bWaitForUserAction=FALSE;
			}
			free(pszURL); pszURL=NULL;
		}
		else
		{
			lenTitle=strlen(gptActions[i].szTitle);
			// if (_strnicmp(szTitle,gptActions[i].szTitle,lenTitle)==0)
			// 0.92B3 : évolution de la reconnaissance des titres, prise en compte des * 
			TRACE((TRACE_DEBUG,_F_,"gptActions[%d].szTitle=%s",i,gptActions[i].szTitle));
			if (swStringMatch(szTitle,gptActions[i].szTitle))
			{
				TRACE((TRACE_DEBUG,_F_,"Ca matche, reinitialisation action %d",i));
				//gptActions[i].wLastDetect=NULL;
				//gptActions[i].tLastDetect=-1;
				LastDetect_RemoveWindow(w);
				gptActions[i].tLastSSO=-1;
				gptActions[i].wLastSSO=NULL;
				gptActions[i].iWaitFor=giWaitBeforeNewSSO;
				gptActions[i].bWaitForUserAction=FALSE;
			}
		}
	}
end:
	if (pszURL!=NULL) free(pszURL);
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// GenerateConfigAndOpenAppNsites()
// ----------------------------------------------------------------------------------
// Génère une proposition de configuration et l'affiche à l'utilisateur dans la 
// fenêtre de gestion des sites et applications
// ----------------------------------------------------------------------------------
void GenerateConfigAndOpenAppNsites(int iType, int iBrowser, char *pszTitle, char *pszURL)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszIEWindowTitle=NULL;
	char *p=NULL;
	int i;

	// Nouveau avec le signup, recherche si on a une config créée par signup pour ce FQDN
	i=giNbActions; // valeur par défaut = création d'une nouvelle action
	if (iBrowser==BROWSER_CHROME || iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4 || iBrowser==BROWSER_EDGE)
	{
		for (i=0;i<giNbActions;i++)
		{
			if (gptActions[i].bSafe && *gptActions[i].szURL!=0 && strstr(pszURL,gptActions[i].szURL)!=NULL)
			{
				goto suite; // oui, oui, je sais
			}
		}
	}
	// si pas trouvé, i=giNbActions, donc c'est OK pour l'ajout d'une nouvelle action
	ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));
suite:
	gptActions[i].tLastSSO=-1;
	gptActions[i].wLastSSO=NULL;
	gptActions[i].iWaitFor=giWaitBeforeNewSSO;
	gptActions[i].bActive=TRUE; 
	gptActions[i].bAutoLock=TRUE;
	gptActions[i].bConfigSent=FALSE;
	gptActions[i].bSaved=FALSE; 
	gptActions[i].iPwdGroup=-1;
	gptActions[i].bSafe=FALSE;
	if (iType==UNK)
	{
		// le UNK était utile dans la requete WEB pour récupérer configs WEB et XEB, maintenant il 
		// s'agit de proposer une config par défaut la plus aboutie possible
		// ISSUE#162
		// if (iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4) iType=XEBSSO;
		if (iBrowser==BROWSER_CHROME || iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4 || iBrowser==BROWSER_EDGE) iType=XEBSSO;
	}
	gptActions[i].iType=iType;

	if (iBrowser==BROWSER_CHROME || iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4 || iBrowser==BROWSER_EDGE || iType==XINSSO)
	{
		strcpy_s(gptActions[i].szId1Name,sizeof(gptActions[i].szId1Name),"-1");
		strcpy_s(gptActions[i].szPwdName,sizeof(gptActions[i].szPwdName),"1");
		strcpy_s(gptActions[i].szValidateName,sizeof(gptActions[i].szValidateName),"[ENTER]");
	}
	// construction du titre
	if (iType==WEBSSO || iType==XEBSSO) // tronque la fin du titre qui contient le nom du navigateur
	{
		p=strstr(pszTitle," - Mozilla Firefox");
		if (p!=NULL) *p=0;
		p=strstr(pszTitle," - Google Chrome");
		if (p!=NULL) *p=0;
		pszIEWindowTitle=GetIEWindowTitle();
		if (pszIEWindowTitle!=NULL)
		{
			p=strstr(pszTitle,pszIEWindowTitle);
			if (p!=NULL) *p=0;
		}
		else
		{
			p=strstr(pszTitle," - Microsoft Internet Explorer");
			if (p!=NULL) *p=0;
			p=strstr(pszTitle," - Windows Internet Explorer");
			if (p!=NULL) *p=0;
			p=strstr(pszTitle," - Internet Explorer");
			if (p!=NULL) *p=0;
		}
		p=strstr(pszTitle,"?- Microsoft Edge");
		if (p!=NULL) *p=0;
		p=strstr(pszTitle," - Maxthon");
		if (p!=NULL) *p=0;
	}
	strncpy_s(gptActions[i].szTitle,sizeof(gptActions[i].szTitle),pszTitle,LEN_TITLE-1);
	// 0.92B4 : ajoute une * à la fin du titre suggéré
	// 0.92B6 : sauf si c'est une popup dans ce cas le titre est toujours complet ?
	if (iType!=POPSSO) strcat_s(gptActions[i].szTitle,sizeof(gptActions[i].szTitle),"*"); 
	// construction du application name
	strncpy_s(gptActions[i].szApplication,sizeof(gptActions[i].szApplication),pszTitle,LEN_APPLICATION_NAME-5);
	GenerateApplicationName(i,gptActions[i].szApplication);
	// construction URL
	if (pszURL!=NULL)
	{
		// ISSUE#305 : avec Chrome, si l'URL ne commence pas par http://, on ajoute https (avant la 1.22, on ajoutait http cf. ISSUE#385)
		if (iBrowser==BROWSER_CHROME && _strnicmp(pszURL,"http://",7)!=0 && _strnicmp(pszURL,"https://",8)!=0 && _strnicmp(pszURL,"file://",7)!=0)
		{
			strcpy_s(gptActions[i].szURL,sizeof(gptActions[i].szURL),"https://"); // ISSUE#385
			strncat_s(gptActions[i].szURL,sizeof(gptActions[i].szURL),pszURL,LEN_URL-1);
		}
		else
		{
			strncpy_s(gptActions[i].szURL,sizeof(gptActions[i].szURL),pszURL,LEN_URL-1);
		}
	}

	TRACE((TRACE_DEBUG,_F_,"i=%d iType=%d szURL=%s",i,gptActions[i].iType,gptActions[i].szURL));
	// et hop, c'est fait (la sauvegarde sera faite ou pas par l'utilisateur dans la fenêtre appNsites)
	// si la fenêtre appnsites est ouverte, il ne faut pas faire le backup car il a été fait au moment de l'ouverture de la fenêtre 
	// et on veut que l'annulation annule tout ce qu'il a fait depuis. Sinon on fait le backup avant d'ouvrir la fenêtre.
	if (gwAppNsites==NULL) BackupAppsNcategs(); // il faut bien le faire avant le ++
	if (i==giNbActions) giNbActions++; 
	if (gwAppNsites!=NULL) 
	{
		EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),TRUE); // ISSUE#114
	}
	ShowAppNsites(i,FALSE);

	if (pszIEWindowTitle!=NULL) free(pszIEWindowTitle);
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// AddApplicationFromCurrentWindow()
// ----------------------------------------------------------------------------------
// Ajoute l'application actuellement affichée sur le dessus
// ----------------------------------------------------------------------------------
int AddApplicationFromCurrentWindow(BOOL bJustDisplayTheMessage)
{
	TRACE((TRACE_ENTER,_F_, ""));
						
	int rc=-1;
	BSTR bstrXML=NULL;
	HWND w;
	char szTitle[512+1];
	char *pszURL=NULL;
	char szClassName[128+1];
	BOOL bConfigFound=FALSE;
	BOOL bReplaceOldConfig=FALSE;
	int iType=UNK;
	BOOL bServerAvailable=FALSE;
	int iBrowser=BROWSER_NONE;

	pszURL=swGetTopWindowWithURL(&w,szTitle,sizeof(szTitle));
	if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"Top Window non trouvee !")); goto end; }
	free(pszURL);pszURL=NULL;

	// Récupérer l'URL pour IE, Firefox et popup Firefox (ne pas faire pour fenêtres Windows et popup IE)
	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"szClassName=%s",szClassName));
	if (strcmp(szClassName,gcszMozillaDialogClassName)==0) // POPUP FIREFOX
	{
		pszURL=GetFirefoxPopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup firefox non trouvee")); goto end; }
		iType=POPSSO;
	}
	else if (strcmp(szClassName,"IEFrame")==0 || // IE 
			 strcmp(szClassName,"rctrl_renwnd32")==0 || // Outlook 97 à 2003 (au moins, à vérifier pour 2007)
			 strcmp(szClassName,"OpusApp")==0 || // Word 97 à 2003 (au moins, à vérifier pour 2007)
			 strcmp(szClassName,"ExploreWClass")==0 || 
			 strcmp(szClassName,"CabinetWClass")==0) // Explorateur Windows
			 // Ne surtout pas ajouter #32770 sinon on embarque les popup IE et c'est raté !
	{
		pszURL=GetIEURL(w,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL IE non trouvee")); goto end; }
		iType=UNK; // permet de récupérer les configs WEB ou XEB 
		iBrowser=BROWSER_IE;
	}
	else if (strcmp(szClassName,"#32770")==0 && 
			(strcmp(szTitle,"Sécurité de Windows")==0 || strcmp(szTitle,"Windows Security")==0)) // POPUP IE8 SUR W7 [ISSUE#5] (FR et US uniquement... pas beau)
	{
		pszURL=GetW7PopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL W7 popup non trouvee")); goto end; }
		iType=POPSSO;
	}
	else if (strcmp(szClassName,"#32770")==0 && 
		    (strncmp(szTitle,"Connexion à",strlen("Connexion à"))==0 
			 || strncmp(szTitle,"Connect to",strlen("Connect to"))==0)) // POPUP IE (ISSUE #8)
	{
		iType=POPSSO;
	}
	else if (strcmp(szClassName,gcszMozillaUIClassName)==0) // FIREFOX3
	{
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX3,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox3 non trouvee")); goto end; }
		iType=UNK; // permet de récupérer les configs WEB ou XEB 
		iBrowser=BROWSER_FIREFOX3;
	}
	else if (strcmp(szClassName,gcszMozillaClassName)==0) // FIREFOX4
	{
		SetForegroundWindow(w); // nécessaire pour ISSUE#371
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX4,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox4+ non trouvee")); goto end; }
		iType=UNK; // permet de récupérer les configs WEB ou XEB 
		iBrowser=BROWSER_FIREFOX4;
	}
	else if (strcmp(szClassName,"Maxthon2_Frame")==0) // Maxthon
	{
		pszURL=GetMaxthonURL();
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Maxthon non trouvee")); goto end; }
		iType=WEBSSO; // pas UNK car XEBSSO pas encore supporté pour Maxthon
		iBrowser=BROWSER_MAXTHON;
	}
	else if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0) // ISSUE#77 : Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	{
		ForceChromeAccessibility(w);
		// ISSUE#215 : nouvelle gestion des popups pour Chrome 36+ (c'était merdique avant je ne conserve pas la compatibilité)	
		pszURL=GetChromePopupURL(w);
		if (pszURL==NULL) // pas de popup trouvée, c'est du contenu chrome Web
		{
			iType=UNK; // permet de récupérer les configs WEB ou XEB 
			pszURL=GetChromeURL(w);
			if (pszURL==NULL) pszURL=GetChromeURL51(w); // ISSUE#282
			// ISSUE#382 : avec Chrome 69, GetChromeURL et  GetChromeURL51 ne fonctionnent pas, il faut NewGetChromeURL
			if (pszURL==NULL || *pszURL==0) pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); // ISSUE#314
			if (pszURL==NULL || *pszURL==0) { Sleep(100); pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); } // Suite à ISSUE#404, Chrome n'a pas eu le temps de monter l'interface IAccessible après l'appel à ForceChromeAccessibility
			if (gpAccessibleChromeURL!=NULL) { gpAccessibleChromeURL->Release(); gpAccessibleChromeURL=NULL; }
			// ISSUE#142 : si pszURL=NULL, mieux vaut s'arrêter même si en fait ça ne crashe pas car bien géré partout
			// ISSUE#155
			// if (pszURL == NULL) { TRACE((TRACE_ERROR, _F_, "URL Chrome non trouvee")); goto suite; }
			if (pszURL == NULL) { TRACE((TRACE_ERROR, _F_, "URL Chrome non trouvee")); goto end; }
			iBrowser=BROWSER_CHROME;
		}
		else // trouvé une popup Chrome, lecture du titre de la popup et de l'URL du site
		{
			iType=POPSSO;
		}
	}
	// ISSUE#347
	else if (strcmp(szClassName,"ApplicationFrameWindow")==0)
	{
		IUIAutomationElement *pDocument=NULL;
		pszURL=GetEdgeURL(w,&pDocument);
		if (pDocument!=NULL) pDocument->Release(); // on n'en a pas besoin pour la suite, on le jette tout de suite
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Edge non trouvee !")); goto end; }
		iType=UNK; // permet de récupérer les configs WEB ou XEB 
		iBrowser=BROWSER_EDGE;
	}
	// ISSUE#297 : prise en compte des popups de sécurité de Windows 10 anniversaire (dans IE, EDGE, etc.)
	else if (strcmp(szClassName,"Credential Dialog Xaml Host")==0 &&  
		    (strcmp(szTitle,"Sécurité Windows")==0 || strcmp(szTitle,"Windows Security")==0)) // POPUP W10 anniversaire... IE, EDGE, partages réseau, etc.
	{
		pszURL=GetW10PopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W10 non trouvee")); goto end; }
		iType=POPSSO;
	}
	else iType=XINSSO;

	// le ShowAppNsites fait beaucoup plus tard va recharger la treeview, il faut faire un GetApplicationDeltails 
	// tout de suite sinon on perdra une éventuelle config commencée par l'utilisateur
	// ISSUE#152 : tant pis, on ne fait plus, ça génère trop de cas foireux car le giLastApplicationConfig ne varie
	//             pas seulement en fonction de la config sélectionnée dans la liste mais aussi en fonction des applis
	//             détectées (pour justement pouvoir mettre le focus sur la config dans appnsites)
	// if (gwAppNsites!=NULL) GetApplicationDetails(gwAppNsites,giLastApplicationConfig);

	GenerateConfigAndOpenAppNsites(iType,iBrowser,szTitle,pszURL); // fait une proposition de config, l'utilisateur complètera
	
end:
	if (pszURL!=NULL) free(pszURL);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// InternetCheckVersion
// ----------------------------------------------------------------------------------
// Vérification des mises à jour sur internet
// Format de la chaine retournée : 079-0801 (079-0000 si pas de beta en cours)
// ----------------------------------------------------------------------------------
void InternetCheckVersion()
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szParams[255+1];
	char *pszResult=NULL;
	char *pReleaseVersion=NULL,*pBetaVersion=NULL;
	char szMsg[255+1];
	char szBeta[10+1];
	BOOL bNewVersion=FALSE;
	BOOL bBeta=FALSE;
	int iCurrentVersion,iInternetVersion;
	DWORD dwStatusCode;

	strcpy_s(szParams,sizeof(szParams),"?action=getversion");
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,3,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	
	TRACE((TRACE_INFO,_F_,"Version sur internet = %s",pszResult));
	if (strlen(pszResult)!=8) goto end;

	*szBeta=0;
	pszResult[3]=0;
	pReleaseVersion=pszResult;
	pBetaVersion=pszResult+4;
	TRACE((TRACE_INFO,_F_,"ReleaseVersion : %s (locale) %s (centrale)",gcszCurrentVersion,pReleaseVersion));
	TRACE((TRACE_INFO,_F_,"BetaVersion    : %s (locale) %s (centrale)",gcszCurrentBeta,pBetaVersion));
	
	if (strcmp(pReleaseVersion,"000")==0) goto end;// ISSUE#90 : dans ce cas on ne vérifie plus la version.

	if (!gbInternetCheckBeta) // ne vérifie pas la présence de version beta
	{
		if (strcmp(pReleaseVersion,gcszCurrentVersion)!=0) 
		{
			// ISSUE#135
			iCurrentVersion=atoi(gcszCurrentVersion);
			iInternetVersion=atoi(pReleaseVersion);
			if (iInternetVersion>iCurrentVersion)
			{
				bNewVersion=TRUE;
				wsprintf(szMsg,GetString(IDS_NEW_VERSION),pReleaseVersion[0],pReleaseVersion[1],pReleaseVersion[2],"");
			}
		}
	}
	else // vérifie la présence de beta
	{
		if (strcmp(pBetaVersion,"0000")==0)
		{
			if (strcmp(pReleaseVersion,gcszCurrentVersion)!=0) 
			{
				// ISSUE#135
				iCurrentVersion=atoi(gcszCurrentVersion);
				iInternetVersion=atoi(pReleaseVersion);
				if (iInternetVersion>iCurrentVersion)
				{
					bNewVersion=TRUE;
					wsprintf(szMsg,GetString(IDS_NEW_VERSION),pReleaseVersion[0],pReleaseVersion[1],pReleaseVersion[2],"");
				}
			}
		}
		else if (strcmp(pBetaVersion,gcszCurrentBeta)!=0)
		{
			bBeta=TRUE;
			// ISSUE#135
			iCurrentVersion=atoi(gcszCurrentBeta);
			iInternetVersion=atoi(pBetaVersion);
			if (iInternetVersion>iCurrentVersion)
			{
				bNewVersion=TRUE;
				if (pBetaVersion[3]>'0' && pBetaVersion[3]<='9') // c'est une version beta qui est en ligne
					wsprintf(szBeta," beta %c",pBetaVersion[3]);
				else if (pBetaVersion[3]>='A' && pBetaVersion[3]<='Z')
					wsprintf(szBeta," beta %d",pBetaVersion[3]-'A'+10);
				wsprintf(szMsg,GetString(IDS_NEW_VERSION),pBetaVersion[0],pBetaVersion[1],pBetaVersion[2],szBeta);
			}
		}
	}
	if (bNewVersion)
	{
		if (MessageBox(NULL,szMsg,"swSSO",MB_YESNO | MB_ICONQUESTION)==IDYES)
		{
			if (bBeta)
				ShellExecute(NULL,"open","http://www.swsso.fr/download/beta",NULL,"",SW_SHOW );
			else
				ShellExecute(NULL,"open","http://www.swsso.fr",NULL,"",SW_SHOW );
		}
	}
end:
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// SyncSecondaryPasswordGroup()
//-----------------------------------------------------------------------------
// Modifie les mots de passe du groupe avec le mot de passe AD, uniquement
// pour les configs qui ont l'ID AD
//-----------------------------------------------------------------------------
int SyncSecondaryPasswordGroup(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int i;
	char *pszADPassword=NULL;
	char *pszEncryptedPassword=NULL;
	
	if (!gbSyncSecondaryPasswordActive) goto end;
	if (giSyncSecondaryPasswordGroup==-1) goto end;

	// récupère le mot de passe AD
	pszADPassword=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
	if (pszADPassword==NULL) goto end;

	for (i=0;i<giNbActions;i++)
	{
		if ((gptActions[i].iPwdGroup==giSyncSecondaryPasswordGroup) &&
			(*gptActions[i].szId1Value!=0) && (*gszUserName!=0) && 
			(_stricmp(gptActions[i].szId1Value,gszUserName)==0))
		{
			TRACE((TRACE_DEBUG,_F_,"Changement mot de passe appli #%d (%s) avec mdp AD pour le user %s",i,gptActions[i].szApplication,gszUserName));
			pszEncryptedPassword=swCryptEncryptString(pszADPassword,ghKey1);
			if (pszEncryptedPassword==NULL) goto end;
			strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedPassword);
			free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
			pszEncryptedPassword=NULL;
		}
	}
	SaveApplications();
	rc=0;
end:
	if (pszADPassword!=NULL) { SecureZeroMemory(pszADPassword,strlen(pszADPassword)); free (pszADPassword); }
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SyncConfigsPwdAndOptionnalyLogin() -- ISSUE#390
//-----------------------------------------------------------------------------
// Synchronise toutes les configurations du groupe avec celle passée en paramètre
// Si le n° du groupe est < 40, ne synchronise que le mot de passe et seulement si l'identifiant est non vide et égal
// Si le n° du groupe est >=40, synchronise systématiquement identifiant et mot de passe, sauf si config issue d'un ajout de compte
//-----------------------------------------------------------------------------
int SyncConfigsPwdAndOptionnalyLogin(int iAction)
{
	TRACE((TRACE_ENTER,_F_,"iAction=%d",iAction));
	int rc=-1;
	int i;
	char *pszEncryptedValue=NULL;
	char *pszDecryptedValue=NULL;
	BOOL bSyncId=FALSE;
	BOOL bSyncPwd=FALSE;

	TRACE((TRACE_DEBUG,_F_,"Changement mdp groupé induit par appli %s groupe=%d",gptActions[iAction].szApplication,gptActions[iAction].iPwdGroup));

	// déchiffre le mot de passe à synchroniser sur les autres configs
	pszDecryptedValue=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
	if (pszDecryptedValue==NULL) goto end;

	for (i=0;i<giNbActions;i++)
	{
		if (i==iAction) continue;
		if (gptActions[i].iPwdGroup==gptActions[iAction].iPwdGroup) 
		{
			// pour les groupes>=40, synchro systématique sauf si issue d'un ajout de compte
			if (gptActions[iAction].iPwdGroup>=40 && !gptActions[iAction].bAddAccount && !gptActions[i].bAddAccount)
			{
				if (*gptActions[iAction].szId1Value!=0) bSyncId=TRUE;
				bSyncPwd=TRUE;
			}
			// pour les groupes < 40, ne synchronise que le mot de passe et à condition que 
			// l'id soit non vide et égal à celui de la config de référence
			else if ((*gptActions[i].szId1Value!=0) && (*gptActions[iAction].szId1Value!=0) &&  
					(_stricmp(gptActions[i].szId1Value,gptActions[iAction].szId1Value)==0))
			{
				bSyncId=FALSE;
				bSyncPwd=TRUE;
			}
			else
			{
				bSyncId=FALSE;
				bSyncPwd=FALSE;
			}
			if (bSyncId)
			{
				TRACE((TRACE_DEBUG,_F_,"Changement id appli %s induit par appli %s",gptActions[i].szApplication,gptActions[iAction].szApplication));
				strcpy_s(gptActions[i].szId1Value,sizeof(gptActions[i].szId1Value),gptActions[iAction].szId1Value);
			}
			if (bSyncPwd)
			{
				TRACE((TRACE_DEBUG,_F_,"Changement mdp appli %s induit par appli %s",gptActions[i].szApplication,gptActions[iAction].szApplication));
				pszEncryptedValue=swCryptEncryptString(pszDecryptedValue,ghKey1);
				if (pszEncryptedValue==NULL) goto end;
				strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedValue);
				free(pszEncryptedValue); // forcément pas NULL sinon on ne serait pas là
				pszEncryptedValue=NULL;
			}
		}
	}
	rc=0;
end:
	if (pszDecryptedValue!=NULL)
	{
		SecureZeroMemory(pszDecryptedValue,strlen(pszDecryptedValue));
		free(pszDecryptedValue);
	}
	if (pszEncryptedValue!=NULL) free(pszEncryptedValue);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SyncAllConfigsLogionAndPwd() -- ISSUE#390
//-----------------------------------------------------------------------------
// Parcourt l'ensemble des configurations et pour celles dont le groupe est >= 40
// synchronise l'identifiant et le mot de passe avec les 1ères valeurs non vides
// trouvées dans une configuration du même groupe
//-----------------------------------------------------------------------------
int SyncAllConfigsLoginAndPwd(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	int tabGroups[40];
	int rc=-1;
	int iConfig;
	int iGroupe;
	int nGroupes=0;
	int iConfigWithIdAndPwd;
	BOOL bAlreadyAdded;

	// construit la liste des groupes >= 40
	for (iConfig=0;iConfig<giNbActions;iConfig++)
	{
		// exclut de la synchro les configurations issues d'un ajout de compte
		if (gptActions[iConfig].iPwdGroup>=40 && !gptActions[iConfig].bAddAccount)
		{
			// ajoute groupe si pas déjà fait
			bAlreadyAdded=FALSE;
			for (iGroupe=0;iGroupe<nGroupes;iGroupe++)
			{
				if (tabGroups[iGroupe]==gptActions[iConfig].iPwdGroup) 
				{ 
					bAlreadyAdded=TRUE; 
					break; 
				}
			}
			if (!bAlreadyAdded)
			{
				tabGroups[nGroupes]=gptActions[iConfig].iPwdGroup;
				TRACE((TRACE_DEBUG,_F_,"tabGroups[%d]=%d",nGroupes,tabGroups[nGroupes]));
				nGroupes++;
			}
		}
	}
	// parcourt la liste des groupes et synchronise les identifiants et mots de passe
	for (iGroupe=0;iGroupe<nGroupes;iGroupe++)
	{
		TRACE((TRACE_DEBUG,_F_,"Début synchronisation du groupe %d",tabGroups[iGroupe]));
		// parcourt les configs du groupe pour en trouver une avec id et mdp non vide
		iConfigWithIdAndPwd=-1;
		for (iConfig=0;iConfig<giNbActions;iConfig++)
		{
			if ((gptActions[iConfig].iPwdGroup==tabGroups[iGroupe]) &&
				(*gptActions[iConfig].szId1Value!=0) &&
				(*gptActions[iConfig].szPwdEncryptedValue!=0))
			{
				iConfigWithIdAndPwd=iConfig;
				break;
			}
		}
		if (iConfigWithIdAndPwd==-1)
		{
			TRACE((TRACE_DEBUG,_F_,"Aucune config avec id et mdp trouvee dans ce groupe -> pas de synchro"));
		}
		else
		{
			TRACE((TRACE_DEBUG,_F_,"Config avec id et mdp trouvee dans ce groupe : %s",gptActions[iConfigWithIdAndPwd].szApplication));
			SyncConfigsPwdAndOptionnalyLogin(iConfigWithIdAndPwd);
		}
		TRACE((TRACE_DEBUG,_F_,"Fin synchronisation du groupe %d",tabGroups[iGroupe]));
	}
	rc=0;
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//////////////////////// WM_PAINT pour bitmap dans onglet about
#if 0
		case WM_PAINT:
			{
				HANDLE ghLogoBar=(HICON)LoadImage(ghInstance, 
					MAKEINTRESOURCE(IDB_LOGO_BAR), 
					IMAGE_BITMAP,
					0,
					0,
					LR_DEFAULTCOLOR);
				if (ghLogoBar==NULL) MessageBox(NULL,"LoadImage()","",MB_OK);
				HDC dc=GetDC(w); 
if (dc==NULL) MessageBox(NULL,"GetDC()","",MB_OK);
				HDC dcMemory=CreateCompatibleDC(dc); 
if (dcMemory==NULL) MessageBox(NULL,"CreateCompatibleDC()","",MB_OK);
				HGDIOBJ oldBitmap=SelectObject(dcMemory,ghLogoBar);
if (oldBitmap==NULL) MessageBox(NULL,"SelectObject()","",MB_OK);
				RECT clientRectSheet,clientRectFrame;
				GetClientRect(w,&clientRectSheet);
				GetClientRect(w,&clientRectFrame);
				if (BitBlt(dc,250,35,100,50,dcMemory,0,0,SRCCOPY)==0)
					MessageBox(NULL,"BitBlt()","",MB_OK);
				SelectObject(dcMemory,oldBitmap);
				ReleaseDC(w,dcMemory);
				ReleaseDC(w,dc);
				rc=0;
			}
			break;
#endif

