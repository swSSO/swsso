//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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
// Fen�tre de config + fonctions de lecture/�criture de config
//-----------------------------------------------------------------------------

#include "stdafx.h"

// globales globales ;-)
char gszCfgFile[_SW_MAX_PATH+1];
char gszCfgVersion[5+1];
BOOL gbSessionLock=TRUE;     // 0.63B4 : true si verrouillage sur suspension de session windows
char gszCfgPortal[_MAX_PATH+1];
BOOL gbInternetCheckVersion=FALSE;		// 0.80 : autorise swSSO � se connecter � internet pour v�rifier la version
BOOL gbInternetCheckBeta=FALSE;			// 0.80 : y compris pour les beta
BOOL gbInternetGetConfig=FALSE;			// 0.80 : autorise swSSO � se connecter � internet pour r�cup�rer des configurations
//BOOL gbInternetPutConfig=FALSE;			// 0.80 : autorise swSSO � se connecter � internet pour uploader automatiquement les configurations
BOOL gbInternetManualPutConfig=FALSE;	// 0.89 : autorise swSSO � se connecter � internet pour uploader manuellement les configurations
BOOL gbInternetUseProxy=FALSE;			// 0.80 : utilise un proxy pour se connecter � internet
char gszProxyURL[LEN_PROXY_URL+1];		// 0.80 : URL proxy
char gszProxyUser[LEN_PROXY_USER+1];	// 0.80 : compte utilisateur pour authentification proxy
char gszProxyPwd[LEN_PROXY_PWD+1];		// 0.80 : mot de passe pour authentification proxy
BOOL gbTmpInternetUseProxy=FALSE;		// 0.80 : utilise un proxy pour se connecter � internet (pour bouton "j'ai de la chance")
char gszTmpProxyURL[LEN_PROXY_URL+1];	// 0.80 : URL proxy (pour bouton "j'ai de la chance")
char gszTmpProxyUser[LEN_PROXY_USER+1];	// 0.80 : compte utilisateur pour authentification proxy (pour bouton "j'ai de la chance")
char gszTmpProxyPwd[LEN_PROXY_PWD+1];	// 0.80 : mot de passe pour authentification proxy (pour bouton "j'ai de la chance")
BOOL gbParseWindowsOnStart=TRUE;		// 0.93B4 : parse / ne parse pas les fen�tres ouvertes au lancement de SSO
int  giNbExcludedHandles=0;
HWND gTabExcludedHandles[MAX_EXCLUDED_HANDLES];
int  giDomainId=1;						// 0.94B1 : gestion des domaines
char gszDomainLabel[LEN_DOMAIN+1]="";
BOOL gbDisplayChangeAppPwdDialog ; // ISSUE#107
char gszLastADPwdChange[14+1]="";					// 1.03 : date de dernier changement de mdp dans l'AD, format AAAAMMJJHHMMSS
char gszLastADPwdChange2[50+1]="";					// 1.12 : date de dernier changement de mdp dans l'AD, format hilong,lowlong (cf. ISSUE#281)
char gszEncryptedADPwd[LEN_ENCRYPTED_AES256+1]="";	// 1.03 : valeur du mot de passe AD (fourni par l'utilisateur) ou en 1.08 r�cup�r� en mode chain� Windows
BOOL gbSSOInternetExplorer=TRUE;		// ISSUE#176
BOOL gbSSOFirefox=TRUE;					// ISSUE#176
BOOL gbSSOChrome=TRUE;					// ISSUE#176
BOOL gbShowLaunchAppWithoutCtrl=FALSE;	// ISSUE#254
int giLanguage=0; // 0=langue de l'OS, 1=FR, 2=EN
wchar_t gwszDefaultLanguage[256]=L"";
HWND gwIdAndPwdDialogProc=NULL;

int gx,gy,gcx,gcy; 		// positionnement de la fen�tre sites et applications
int gx2,gy2,gcx2,gcy2,gbLaunchTopMost; 	// positionnement de lancement d'application
int gx3,gy3,gcx3,gcy3; 		// positionnement de la fen�tre publishto
int gx4,gy4,gcx4,gcy4;		// positionnement de la fen�tre de s�lection d'un compte existant
char gszLastConfigUpdate[14+1]; 		// 0.91 : date (AAAAMMJJHHMMSS) de derni�re mise � jour des configurations depuis le serveur
HWND gwPropertySheet=NULL;

// globales locales ;-)
static HWND gwChangeMasterPwd=NULL;
static char buf100[100];
static char buf2048[2048]; // 0.76 (pr�caution...)
static char gszEnumComputerNames[512]; // utilis�e pour l'�num�ration de computernames. Ne pas utiliser ailleurs !!!

BOOL gbDontAskId,gbDontAskPwd; // 0.80 pour fenetre de saisie id et mdp (ajout d'une application)
BOOL gbDontAskId2,gbDontAskId3,gbDontAskId4;

// pour l'�num�ration des computernames...
static char *gpNextComputerName=NULL;
static char *gpComputerNameContext=NULL;

char gcszK2[]="22222222";

static int giRefreshTimer=10;

char *gpszURLBeingAdded=NULL;
char *gpszTitleBeingAdded=NULL;

T_SALT gSalts; // sels pour le stockage du mdp primaire et la d�rivation de cl� de chiffrement des mdp secondaires
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
			
			if (gwPropertySheet==NULL) // n�cessaire pour ne pas le faire � chaque premier affichage de l'onglet
			{
				gwPropertySheet=GetParent(w);
				// icone ALT-TAB
				SendMessage(gwPropertySheet,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
				SendMessage(gwPropertySheet,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// 0.53 positionnement de la fen�tre !
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
			// 0.63BETA5 : d�tail nb sso popups
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

			if (giDomainId==-1)
				SetWindowText(GetDlgItem(w,TX_DOMAIN),"Tous");
			else if (giDomainId==1) 
				SetWindowText(GetDlgItem(w,TX_DOMAIN),"Commun"); 
			else 
				SetWindowText(GetDlgItem(w,TX_DOMAIN),gszDomainLabel);

			// remplit avec les valeurs de config (c'�tait dans PSN_SETACTIVE avant... ???)
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

			if (*gszCfgPortal==0) // pas de portail d�fini
			{
				if (gbEnableOption_Portal) // l'utilisateur a le droit de d�finir un portail
				{
					SetDlgItemText(w,TX_PORTAL,GetString(IDS_DEFAULT_PORTAL));
				}
				else // pas le droit de d�finir un portail, on masque tout
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
			// 0.90 positionnement s�parateurs (bug RESEDIT) -> alignement sur bouton licence
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
			// if (iDlgCtrlID==TX_URL || iDlgCtrlID==TX_MAILTO) 0.86 : suppression de l'@mail
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
		case WM_COMMAND: // 0.63BETA5 : ouverture du .ini depuis la fen�tre about.
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
					ofn.lpstrFilter="*.xml";
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
					ofn.lpstrDefExt="xml";
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
						// r�initialisation depuis le serveur
						RefreshRights(TRUE,TRUE);
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
// R�cup�re la langue courante
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
// PSPConfigurationProc()
// ----------------------------------------------------------------------------------
// Dialog proc de l'onglet Configuration
// ----------------------------------------------------------------------------------
static int CALLBACK PSPConfigurationProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			int cx;
			int cy;
			RECT rect;
			
			if (gwPropertySheet==NULL) // n�cessaire pour ne pas le faire � chaque premier affichage de l'onglet
			{
				gwPropertySheet=GetParent(w);
				// icone ALT-TAB
				SendMessage(gwPropertySheet,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
				SendMessage(gwPropertySheet,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// 0.53 positionnement de la fen�tre !
				cx = GetSystemMetrics( SM_CXSCREEN );
				cy = GetSystemMetrics( SM_CYSCREEN );
				TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
				GetWindowRect(gwPropertySheet,&rect);
				SetWindowPos(gwPropertySheet,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}	
			// limitations champs saisie
			//SendMessage(GetDlgItem(w,TB_PROXY_PWD),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			SendMessage(GetDlgItem(w,TB_PROXY_URL),EM_LIMITTEXT,LEN_PROXY_URL,0);
			SendMessage(GetDlgItem(w,TB_PROXY_USER),EM_LIMITTEXT,LEN_PROXY_USER,0);
			SendMessage(GetDlgItem(w,TB_PROXY_PWD),EM_LIMITTEXT,LEN_PROXY_PWD,0);
			// grise/cache en fonction de la politique de s�curit�
			if (HowManyComputerNames()<2) ShowWindow(GetDlgItem(w,PB_CHANCE),SW_HIDE);
			if (!gbEnableOption_SessionLock)	 EnableWindow(GetDlgItem(w,CK_LOCK),FALSE);
			if (!gbEnableOption_CheckVersion)  { EnableWindow(GetDlgItem(w,CK_CHECK_VERSION),FALSE); EnableWindow(GetDlgItem(w,CK_CHECK_BETA),FALSE); }
			if (!gbEnableOption_GetConfig)		 EnableWindow(GetDlgItem(w,CK_GET_CONFIG),FALSE);
			//if (!gbEnableOption_PutConfig)		 EnableWindow(GetDlgItem(w,CK_PUT_CONFIG),FALSE);
			if (!gbEnableOption_ManualPutConfig) EnableWindow(GetDlgItem(w,CK_MANUAL_PUT_CONFIG),FALSE);
			if (!gbEnableOption_Proxy)			
			{ 
				EnableWindow(GetDlgItem(w,CK_USE_PROXY),FALSE); 
				EnableWindow(GetDlgItem(w,TB_PROXY_URL),FALSE); 
				EnableWindow(GetDlgItem(w,TB_PROXY_USER),FALSE); 
				EnableWindow(GetDlgItem(w,TB_PROXY_PWD),FALSE); 
				EnableWindow(GetDlgItem(w,TX_PROXY_URL),FALSE); 
				EnableWindow(GetDlgItem(w,TX_PROXY_USER),FALSE); 
				EnableWindow(GetDlgItem(w,TX_PROXY_PWD),FALSE); 
				ShowWindow(GetDlgItem(w,PB_CHANCE),SW_HIDE);
			}
			// remplit avec les valeurs de config (c'�tait dans PSN_SETACTIVE avant... ???
			CheckDlgButton(w,CK_CHECK_VERSION,gbInternetCheckVersion?BST_CHECKED:BST_UNCHECKED);
			// ISSUE #107
			CheckDlgButton(w,CK_DISPLAY_MSG,gbDisplayChangeAppPwdDialog?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_CHECK_BETA,gbInternetCheckBeta?BST_CHECKED:BST_UNCHECKED);
			if (IsDlgButtonChecked(w,CK_CHECK_VERSION)==BST_UNCHECKED)
			{
				CheckDlgButton(w,CK_CHECK_BETA,BST_UNCHECKED);
			}
			if (gbEnableOption_CheckVersion)
			{
				EnableWindow(GetDlgItem(w,CK_CHECK_BETA),(IsDlgButtonChecked(w,CK_CHECK_VERSION)==BST_CHECKED?TRUE:FALSE));
			}
			CheckDlgButton(w,CK_GET_CONFIG,gbInternetGetConfig?BST_CHECKED:BST_UNCHECKED);
			//CheckDlgButton(w,CK_PUT_CONFIG,gbInternetPutConfig?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_MANUAL_PUT_CONFIG,gbInternetManualPutConfig?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_LOCK,gbSessionLock?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_USE_PROXY,gbInternetUseProxy?BST_CHECKED:BST_UNCHECKED);
			SetDlgItemText(w,TB_PROXY_URL,gszProxyURL);
			SetDlgItemText(w,TB_PROXY_USER,gszProxyUser);
			SetDlgItemText(w,TB_PROXY_PWD,gszProxyPwd);
			if (gbEnableOption_Proxy)
			{
				EnableWindow(GetDlgItem(w,TB_PROXY_URL),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
				EnableWindow(GetDlgItem(w,TB_PROXY_USER),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
				EnableWindow(GetDlgItem(w,TB_PROXY_PWD),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
			}
			// 0.90 positionnement s�parateurs (bug RESEDIT) -> alignement sur champ de saisie proxy
			{ 
				RECT rectSeparator,rectProxyUrl;
				GetWindowRect(GetDlgItem(w,TB_PROXY_URL),&rectProxyUrl);
				
				GetWindowRect(GetDlgItem(w,IDC_SEP_SECURITE),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_SECURITE),NULL,0,0,rectProxyUrl.right-rectSeparator.left,2,SWP_NOMOVE);
				GetWindowRect(GetDlgItem(w,IDC_SEP_AUTORISATIONS),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_AUTORISATIONS),NULL,0,0,rectProxyUrl.right-rectSeparator.left,2,SWP_NOMOVE);
				GetWindowRect(GetDlgItem(w,IDC_SEP_PROXY),&rectSeparator);
				SetWindowPos(GetDlgItem(w,IDC_SEP_PROXY),NULL,0,0,rectProxyUrl.right-rectSeparator.left,2,SWP_NOMOVE);
			}
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_DEFAULT));
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)"Fran�ais");
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_ADDSTRING,0,(LPARAM)"English");
			SendMessage(GetDlgItem(w,CB_LANGUE),CB_SETCURSEL,giLanguage,0);

			// ISSUE#309
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
				case TX_INTERNET:
				case TX_PROXY:
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
						gbInternetCheckVersion=IsDlgButtonChecked(w,CK_CHECK_VERSION)==BST_CHECKED?TRUE:FALSE;
						gbDisplayChangeAppPwdDialog=IsDlgButtonChecked(w,CK_DISPLAY_MSG)==BST_CHECKED?TRUE:FALSE;
						gbInternetCheckBeta=IsDlgButtonChecked(w,CK_CHECK_BETA)==BST_CHECKED?TRUE:FALSE;
						gbInternetGetConfig=IsDlgButtonChecked(w,CK_GET_CONFIG)==BST_CHECKED?TRUE:FALSE;
						//gbInternetPutConfig=IsDlgButtonChecked(w,CK_PUT_CONFIG)==BST_CHECKED?TRUE:FALSE;
						gbInternetManualPutConfig=IsDlgButtonChecked(w,CK_MANUAL_PUT_CONFIG)==BST_CHECKED?TRUE:FALSE;
						gbInternetUseProxy=IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE;
						gbSessionLock=IsDlgButtonChecked(w,CK_LOCK)==BST_CHECKED?TRUE:FALSE;
						GetDlgItemText(w,TB_PROXY_URL,gszProxyURL,sizeof(gszProxyURL));
						GetDlgItemText(w,TB_PROXY_USER,gszProxyUser,sizeof(gszProxyUser));
						GetDlgItemText(w,TB_PROXY_PWD,gszProxyPwd,sizeof(gszProxyPwd));
						giLanguage=SendMessage(GetDlgItem(w,CB_LANGUE),CB_GETCURSEL,0,0);
						if (giLanguage==CB_ERR) giLanguage=0;
						SetLanguage();
						// ISSUE#309
						giMasterPwdExpiration=GetDlgItemInt(w,TB_MASTER_PWD_EXPIRATION,&bTranslated,FALSE);
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
				case CK_CHECK_BETA:
				case CK_LOCK:
				case CK_DISPLAY_MSG:
				case CK_GET_CONFIG:
				case CK_MANUAL_PUT_CONFIG:
					PropSheet_Changed(gwPropertySheet,w);
					break;
				case CK_USE_PROXY:
					if (gbEnableOption_Proxy)
					{
						EnableWindow(GetDlgItem(w,TB_PROXY_URL),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
						EnableWindow(GetDlgItem(w,TB_PROXY_USER),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
						EnableWindow(GetDlgItem(w,TB_PROXY_PWD),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
					}
					PropSheet_Changed(gwPropertySheet,w);
					break;
				case CK_CHECK_VERSION:
					if (gbEnableOption_CheckVersion)
					{
						if (IsDlgButtonChecked(w,CK_CHECK_VERSION)==BST_UNCHECKED)
							CheckDlgButton(w,CK_CHECK_BETA,BST_UNCHECKED);
						EnableWindow(GetDlgItem(w,CK_CHECK_BETA),(IsDlgButtonChecked(w,CK_CHECK_VERSION)==BST_CHECKED?TRUE:FALSE));
					}
					PropSheet_Changed(gwPropertySheet,w);
					break;
				case PB_TEST:
					InternetCheckProxyParams(w);
					break;
				case PB_CHANCE:
					if (gbEnableOption_Proxy)
					{
						GetProxyConfig(GetNextComputerName(),&gbTmpInternetUseProxy,gszTmpProxyURL,gszTmpProxyUser,gszTmpProxyPwd);
						CheckDlgButton(w,CK_USE_PROXY,gbTmpInternetUseProxy?BST_CHECKED:BST_UNCHECKED);
						SetDlgItemText(w,TB_PROXY_URL,gszTmpProxyURL);
						SetDlgItemText(w,TB_PROXY_USER,gszTmpProxyUser);
						SetDlgItemText(w,TB_PROXY_PWD,gszTmpProxyPwd);
						EnableWindow(GetDlgItem(w,TB_PROXY_URL),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
						EnableWindow(GetDlgItem(w,TB_PROXY_USER),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
						EnableWindow(GetDlgItem(w,TB_PROXY_PWD),(IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE));
					}
					break;
			}
			break;
	}
	return rc;
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
			
			if (gwPropertySheet==NULL) // n�cessaire pour ne pas le faire � chaque premier affichage de l'onglet
			{
				gwPropertySheet=GetParent(w);
				// icone ALT-TAB
				SendMessage(gwPropertySheet,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
				SendMessage(gwPropertySheet,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// 0.53 positionnement de la fen�tre !
				cx = GetSystemMetrics( SM_CXSCREEN );
				cy = GetSystemMetrics( SM_CYSCREEN );
				TRACE((TRACE_INFO,_F_,"SM_CXSCREEN=%d SM_CYSCREEN=%d",cx,cy));
				GetWindowRect(gwPropertySheet,&rect);
				SetWindowPos(gwPropertySheet,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}	
			// remplit avec les valeurs de config 
			CheckDlgButton(w,CK_IE,gbSSOInternetExplorer?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_FIREFOX,gbSSOFirefox?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(w,CK_CHROME,gbSSOChrome?BST_CHECKED:BST_UNCHECKED);
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
					gbSSOInternetExplorer=IsDlgButtonChecked(w,CK_IE)==BST_CHECKED?TRUE:FALSE;
					gbSSOFirefox=IsDlgButtonChecked(w,CK_FIREFOX)==BST_CHECKED?TRUE:FALSE;
					gbSSOChrome=IsDlgButtonChecked(w,CK_CHROME)==BST_CHECKED?TRUE:FALSE;
					SaveConfigHeader();
					PropSheet_UnChanged(gwPropertySheet,w);
					break;
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wp)==EN_CHANGE) PropSheet_Changed(gwPropertySheet,w);
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
// Dialogproc de la boite de saisie des id/mdp d'une appli r�cup�r�e dans config centrale
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
			// r�cup�ration de la structure T_IDANDPWDDIALOG pass�e en LPARAM
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
			// nom des champs compl�mentaires, si pr�sents
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
			// positionnement et dimensionnement de la fen�tre + replacement des boutons OK / Cancel
			int cx;
			int cy;
			RECT rect,clientRect,rectOK,rectCancel,rectLink;
			int iNbToHide=((gbDontAskId2?1:0)+(gbDontAskId3?1:0)+(gbDontAskId4?1:0));

			// dimensionnement de la fen�tre
			GetWindowRect(w,&rect);
			GetClientRect(w,&clientRect);
			SetWindowPos(w,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top-32*iNbToHide,SWP_NOMOVE | SWP_NOZORDER);

			// Correction BUG #109 en 0.90B1
			// positionnement bidon � 0,0 pour pouvoir faire les positionnements de bouton 
			// sinon par d�faut la fen�tre peut �ventuellement �tre d�cal�e en fonction des barres d'outils
			// ouvertes � gauche ou en haut !
			SetWindowPos(w,NULL,0,0,0,0,SWP_NOSIZE | SWP_NOZORDER);

			// replacement des boutons OK / Cancel
			GetWindowRect(GetDlgItem(w,IDOK),&rectOK);
			GetWindowRect(GetDlgItem(w,IDCANCEL),&rectCancel);
			TRACE((TRACE_DEBUG,_F_,"rectOK=(%d,%d,%d,%d)",rectOK.left,rectOK.right,rectOK.top,rectOK.bottom));
			// remarque : (rect.bottom-rect.top-clientRect.bottom) = taille de la barre de titre
			SetWindowPos(GetDlgItem(w,IDOK),NULL,rectOK.left,rectOK.top-32*iNbToHide-(rect.bottom-rect.top-clientRect.bottom),0,0,SWP_NOSIZE | SWP_NOZORDER);
			SetWindowPos(GetDlgItem(w,IDCANCEL),NULL,rectCancel.left,rectCancel.top-32*iNbToHide-(rect.bottom-rect.top-clientRect.bottom),0,0,SWP_NOSIZE | SWP_NOZORDER);

			// replacement du lien "je veux r�utiliser un identifiant existant"
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
			else // positionnement en bas � droite de l'�cran, pr�s de l'icone swSSO
			{
				SetWindowPos(w,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
			}
			// lien soulign�
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
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
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
							// NEW pour ISSUE#367 : replique le mot de passe sur toutes les configs du password group, uniquement si password group >= 20
							if (gptActions[params->iAction].iPwdGroup >= 20)
							{
								for (int i=0;i<giNbActions;i++)
								{
									if (i==params->iAction) continue; 
									if (gptActions[i].iPwdGroup==gptActions[params->iAction].iPwdGroup)
									{
										TRACE((TRACE_DEBUG,_F_,"Changement mot de passe appli %s induit par config %s",gptActions[i].szApplication,gptActions[params->iAction].szApplication));
										strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedPassword);
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
					// ISSUE#113 : refresh sur la fen�tre gestion des sites et applications
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
				if (iAction!=-1) // l'utilisateur a s�lectionn� une appli dans la liste
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
// Affichage de la fen�tre de config
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

	HPROPSHEETPAGE hpsp[3];
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
	if (gbEnableOption_ShowOptions)
	{
		psp.pszTemplate=MAKEINTRESOURCE(PSP_INTERNET);
		psp.pfnDlgProc=PSPConfigurationProc;
		psp.lParam=0;
		hpsp[iPage]=CreatePropertySheetPage(&psp);
		if (hpsp[iPage]==NULL) goto end;
		iPage++;
	}
	if (!gbAdmin && gbEnableOption_ShowBrowsers)
	{
		psp.pszTemplate=MAKEINTRESOURCE(PSP_BROWSER);
		psp.pfnDlgProc=PSPBrowserProc;
		psp.lParam=0;
		hpsp[iPage]=CreatePropertySheetPage(&psp);
		if (hpsp[iPage]==NULL) goto end;
		iPage++;
	}

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
// GetProxyConfig()
// ----------------------------------------------------------------------------------
// Lecture de la config proxy pour le computername pass� en param�tre
// ----------------------------------------------------------------------------------
int GetProxyConfig(const char *szComputerName, BOOL *pbInternetUseProxy, char *szProxyURL,char *szProxyUser,char *szProxyPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szItem[120+1]="";
	char szEncryptedProxyPwd[LEN_ENCRYPTED_AES256+1]; 
	int lenEncryptedProxyPwd;

	if (szComputerName==NULL || *szComputerName==0)
	{
		*pbInternetUseProxy=FALSE; *szProxyURL=0; *szProxyUser=0; *szProxyPwd=0;
	}
	else
	{
		sprintf_s(szItem,sizeof(szItem),"internetUseProxy-%s",szComputerName);
		*pbInternetUseProxy=GetConfigBoolValue("swSSO",szItem,FALSE,TRUE);		
		sprintf_s(szItem,sizeof(szItem),"ProxyURL-%s",szComputerName);
		GetPrivateProfileString("swSSO",szItem,"",szProxyURL,LEN_PROXY_URL+1,gszCfgFile);
		sprintf_s(szItem,sizeof(szItem),"ProxyUser-%s",szComputerName);
		GetPrivateProfileString("swSSO",szItem,"",szProxyUser,LEN_PROXY_USER+1,gszCfgFile);
		sprintf_s(szItem,sizeof(szItem),"ProxyPwd-%s",szComputerName);
		lenEncryptedProxyPwd=GetPrivateProfileString("swSSO",szItem,"",szEncryptedProxyPwd,sizeof(szEncryptedProxyPwd),gszCfgFile);
		if (lenEncryptedProxyPwd==0) // pas de mot de passe
		{
			*szProxyPwd=0;
		}
		else if (lenEncryptedProxyPwd==LEN_ENCRYPTED_3DES || lenEncryptedProxyPwd==LEN_ENCRYPTED_AES256)  // mot de passe chiffr� 3DES ou AES256
		{
			char *pszTmpDecryptedPassword=swCryptDecryptString(szEncryptedProxyPwd,ghKey1);
			if (pszTmpDecryptedPassword==NULL) // �chec d�chiffrement
			{
				*szProxyPwd=0;
			}
			else
			{
				//TRACE((TRACE_PWD,_F_,"pszTmpDecryptedPassword=%s",pszTmpDecryptedPassword));
				strcpy_s(szProxyPwd,LEN_PROXY_PWD+1,pszTmpDecryptedPassword);
				SecureZeroMemory(pszTmpDecryptedPassword,strlen(pszTmpDecryptedPassword));
				free(pszTmpDecryptedPassword);
			}
		}
		else // mot de passe non chiffr� (version 0.80)
		{	
			//TRACE((TRACE_PWD,_F_,"ProxyPwd-%s => mot de passe non chiffre (v0.80) : %s",szComputerName,szEncryptedProxyPwd));
			strcpy_s(szProxyPwd,LEN_PROXY_PWD+1,szEncryptedProxyPwd);
		}
		TRACE((TRACE_INFO,_F_,"ProxyURL-%s =%s",szComputerName,szProxyURL));
		TRACE((TRACE_INFO,_F_,"ProxyUser-%s=%s",szComputerName,szProxyUser));
		//TRACE((TRACE_PWD ,_F_,"ProxyPwd-%s =%s",szComputerName,szProxyPwd));
	}

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
	char szRegDomainLabel[LEN_DOMAIN+1];
	int iRegDomainId;
	BOOL bChangeIni=FALSE;
	
	// ISSUE#164 v�rifie l'int�grit� du fichier .ini. Il faut le faire avant de toucher au .ini
	// Si le fichier .ini n'existe pas (cas de la premi�re utilisation), on ne fait pas �videmment pas la v�rification...
	// HANDLE hf=INVALID_HANDLE_VALUE;
	// hf=CreateFile(gszCfgFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	// if (hf!=INVALID_HANDLE_VALUE) // le fichier existe
	// {	
	// 		CloseHandle(hf);
	//		if (CheckIniHash()!=0) goto end;
	// }
	// Il ne faut pas non plus v�rifier l'int�grit� lorsque swSSO n'a jamais �t� lanc� sinon impossible de fournir un .ini avec des options par d�faut...
	// Du coup on se base sur la pr�sence des sels : sels pr�sents on v�rifie l'int�grit�, sinon non.
	// Si l'utilisateur supprime les sels pour faire sauter la v�rification de l'int�grit�, swSSO ne se lancera pas (impossible de se connecter sans sels)
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
	// ISSUE#185 : les valeurs par d�faut sont d�sormais lues en base de registre dans swSSOPolicies
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
	giDomainId=GetPrivateProfileInt("swSSO","domainId",giDomainId_DefaultValue,gszCfgFile); // 0.94B1 : gestion des domaines
	GetPrivateProfileString("swSSO","domainLabel",gszDomainLabel_DefaultValue,gszDomainLabel,sizeof(gszDomainLabel),gszCfgFile);
	// ISSUE#317 : si domaine configur� en base de registre et qu'il est diff�rent de celui du .ini, le remplace
	if (ReadDomainLabel(szRegDomainLabel)==0)
	{
		if (_stricmp(szRegDomainLabel,gszDomainLabel)!=0) // libell� de domaine diff�rent, on remplace
		{
			TRACE((TRACE_INFO,"Reg domain=%s != ini domain=%s",szRegDomainLabel,gszDomainLabel));
			// commence d�j� par r�cup�rer l'id du domaine sur le serveur -- si �chec, on ne changera rien � ce qu'il y a dans le .ini
			if (GetDomainIdFromLabel(szRegDomainLabel,&iRegDomainId)==0)
			{
				char szItem[16+1];
				TRACE((TRACE_INFO,_F_,"Old domain ini values: %s [%d]",gszDomainLabel,giDomainId));
				giDomainId=iRegDomainId;
				strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),szRegDomainLabel);
				sprintf_s(szItem,sizeof(szItem),"%d",giDomainId);
				WritePrivateProfileString("swSSO","domainId",szItem,gszCfgFile); 
				WritePrivateProfileString("swSSO","domainLabel",gszDomainLabel,gszCfgFile);
				TRACE((TRACE_INFO,_F_,"New domain ini values: %s [%d]",gszDomainLabel,giDomainId));
			}
		}
	}

	gbDisplayChangeAppPwdDialog=GetConfigBoolValue("swSSO","displayChangeAppPwdDialog",gbDisplayChangeAppPwdDialog_DefaultValue,TRUE); // ISSUE#107
	gbSSOInternetExplorer=GetConfigBoolValue("swSSO","InternetExplorer",gbSSOInternetExplorer_DefaultValue,TRUE); // ISSUE#176
	gbSSOFirefox=GetConfigBoolValue("swSSO","Firefox",gbSSOFirefox_DefaultValue,TRUE); // ISSUE#176
	gbSSOChrome=GetConfigBoolValue("swSSO","Chrome",gbSSOChrome_DefaultValue,TRUE); // ISSUE#176
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
	
	// lecture des recovery infos
	GetPrivateProfileString("swSSO","recoveryInfos","",gszRecoveryInfos,sizeof(gszRecoveryInfos),gszCfgFile);
	if (*gszRecoveryInfos!=0)
	{
		char buf[5];
		TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gszRecoveryInfos,strlen(gszRecoveryInfos),"recoveryInfos :"));
		memcpy(buf,gszRecoveryInfos,4);
		buf[4]=0;
		giRecoveryInfosKeyId=atoi(buf);
		TRACE((TRACE_DEBUG,_F_,"recoveryInfosKeyId=%04d",giRecoveryInfosKeyId));
	}

	// 1.03 : lecture de la date de dernier changement mot de passe AD
	if (gbUseADPasswordForAppLogin)
	{
		// 1.12 (ISSUE#281) changement de format de stockage de la date de dernier changement (pb de changement d'heure)
		GetPrivateProfileString("swSSO","lastADPwdChange2","",gszLastADPwdChange2,sizeof(gszLastADPwdChange2),gszCfgFile);
		if (*gszLastADPwdChange2==0) // non trouv� : on la redemande � l'AD et on supprime l'ancienne cl� si pr�sente (migration vers v1.12)
		{
			if (GetLastADPwdChange2(gszLastADPwdChange2,sizeof(gszLastADPwdChange2))==0)
			{
				// �crit la nouvelle valeur
				WritePrivateProfileString("swSSO","lastADPwdChange2",gszLastADPwdChange2,gszCfgFile);
				// on regarde si l'ancienne valeur est pr�sente et on la supprime uniquement si la nouvelle est bien valoris�e,
				// c'est pour �a que je mets le code dans ce if.
				GetPrivateProfileString("swSSO","lastADPwdChange","",gszLastADPwdChange,sizeof(gszLastADPwdChange),gszCfgFile);
				if (*gszLastADPwdChange!=0) // ancienne valeur pr�sente, on la supprime !
				{
					*gszLastADPwdChange=0; // n�cessaire pour que le SaveConfigHeader ne r��crive pas cette ancienne valeur
					WritePrivateProfileString("swSSO","lastADPwdChange",NULL,gszCfgFile); // suppression de la cl�
				}
			}
		}
		GetPrivateProfileString("swSSO","ADPwd","",gszEncryptedADPwd,sizeof(gszEncryptedADPwd),gszCfgFile);
	}

	gx4=GetPrivateProfileInt("swSSO","x4",-1,gszCfgFile);
	gy4=GetPrivateProfileInt("swSSO","y4",-1,gszCfgFile);
	gcx4=GetPrivateProfileInt("swSSO","cx4",-1,gszCfgFile);
	gcy4=GetPrivateProfileInt("swSSO","cy4",-1,gszCfgFile);

	giMasterPwdExpiration=GetPrivateProfileInt("swSSO","MasterPwdExpiration",-1,gszCfgFile);
	if (giMasterPwdExpiration==-1) // non d�fini
	{
		if (giMasterPwdMaxExpiration==-1) // pas de policy d�finie
			giMasterPwdExpiration=5;
		else // on applique la policy d�finie
			giMasterPwdExpiration=giMasterPwdMaxExpiration;
	}
	else // d�fini par l'utilisateur, on borne � la valeur max de la policy, si d�finie
	{
		if (giMasterPwdMaxExpiration!=-1 && giMasterPwdExpiration>giMasterPwdMaxExpiration) giMasterPwdExpiration=giMasterPwdMaxExpiration;
	}
	
	// ISSUE#322 - change les param�tres si d�finis dans la cl� ChangeIniValues
	if (gbSessionLock_ChangeValue!=-1 && gbSessionLock!=gbSessionLock_ChangeValue)				
	{
		gbSessionLock=gbSessionLock_ChangeValue; 
		WritePrivateProfileString("swSSO","sessionLock",gbSessionLock?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbInternetCheckVersion_ChangeValue!=-1 && gbInternetCheckVersion!=gbInternetCheckVersion_ChangeValue)		
	{
		gbInternetCheckVersion=gbInternetCheckVersion_ChangeValue;
		WritePrivateProfileString("swSSO","internetCheckVersion",gbInternetCheckVersion?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbInternetCheckBeta_ChangeValue!=-1 && gbInternetCheckBeta!=gbInternetCheckBeta_ChangeValue)		
	{
		gbInternetCheckBeta=gbInternetCheckBeta_ChangeValue;
		WritePrivateProfileString("swSSO","internetCheckBeta",gbInternetCheckBeta?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbInternetGetConfig_ChangeValue!=-1 && gbInternetGetConfig!=gbInternetGetConfig_ChangeValue)		
	{
		gbInternetGetConfig=gbInternetGetConfig_ChangeValue;
		WritePrivateProfileString("swSSO","internetGetConfig",gbInternetGetConfig?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbInternetManualPutConfig_ChangeValue!=-1 && gbInternetManualPutConfig!=gbInternetManualPutConfig_ChangeValue)	
	{
		gbInternetManualPutConfig=gbInternetManualPutConfig_ChangeValue;
		WritePrivateProfileString("swSSO","internetManualPutConfig",gbInternetManualPutConfig?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gszCfgPortal_ChangeValue[0]!='*' && strcmp(gszCfgPortal,gszCfgPortal_ChangeValue)!=0)			
	{
		strcpy_s(gszCfgPortal,sizeof(gszCfgPortal),gszCfgPortal_ChangeValue);
		WritePrivateProfileString("swSSO","Portal",gszCfgPortal,gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbLaunchTopMost_ChangeValue!=-1 && gbLaunchTopMost!=gbLaunchTopMost_ChangeValue)			
	{
		gbLaunchTopMost=gbLaunchTopMost_ChangeValue;
		WritePrivateProfileString("swSSO","LaunchTopMost",gbLaunchTopMost?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbParseWindowsOnStart_ChangeValue!=-1 && gbParseWindowsOnStart!=gbParseWindowsOnStart_ChangeValue)		
	{
		gbParseWindowsOnStart=gbParseWindowsOnStart_ChangeValue;
		WritePrivateProfileString("swSSO","parseWindowsOnStart",gbParseWindowsOnStart?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (giDomainId_ChangeValue!=-1 && giDomainId!=giDomainId_ChangeValue) 				
	{
		char szItem[16+1];
		giDomainId=giDomainId_ChangeValue;	
		sprintf_s(szItem,sizeof(szItem),"%d",giDomainId);
		WritePrivateProfileString("swSSO","domainId",szItem,gszCfgFile); 
		bChangeIni=TRUE;
	}
	if (gszDomainLabel_ChangeValue[0]!='*' && strcmp(gszDomainLabel,gszDomainLabel_ChangeValue)!=0)			
	{
		strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),gszDomainLabel_ChangeValue);
		WritePrivateProfileString("swSSO","domainLabel",gszDomainLabel,gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbDisplayChangeAppPwdDialog_ChangeValue!=-1 && gbDisplayChangeAppPwdDialog!=gbDisplayChangeAppPwdDialog_ChangeValue) 
	{
		gbDisplayChangeAppPwdDialog=gbDisplayChangeAppPwdDialog_ChangeValue;
		WritePrivateProfileString("swSSO","displayChangeAppPwdDialog",gbDisplayChangeAppPwdDialog?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbSSOInternetExplorer_ChangeValue!=-1 && gbSSOInternetExplorer!=gbSSOInternetExplorer_ChangeValue)		
	{
		gbSSOInternetExplorer=gbSSOInternetExplorer_ChangeValue;
		WritePrivateProfileString("swSSO","InternetExplorer",gbSSOInternetExplorer?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbSSOFirefox_ChangeValue!=-1 && gbSSOFirefox!=gbSSOFirefox_ChangeValue)				
	{
		gbSSOFirefox=gbSSOFirefox_ChangeValue;	
		WritePrivateProfileString("swSSO","Firefox",gbSSOFirefox?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbSSOChrome_ChangeValue!=-1 && gbSSOChrome!=gbSSOChrome_ChangeValue)				
	{
		gbSSOChrome=gbSSOChrome_ChangeValue;
		WritePrivateProfileString("swSSO","Chrome",gbSSOChrome?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (gbShowLaunchAppWithoutCtrl_ChangeValue!=-1 && gbShowLaunchAppWithoutCtrl!=gbShowLaunchAppWithoutCtrl_ChangeValue) 
	{
		gbShowLaunchAppWithoutCtrl=gbShowLaunchAppWithoutCtrl_ChangeValue;
		WritePrivateProfileString("swSSO","ShowLaunchAppWithoutCtrl",gbShowLaunchAppWithoutCtrl?"YES":"NO",gszCfgFile);
		bChangeIni=TRUE;
	}
	if (giRecoveryInfosKeyId!=-1 && giRecoveryInfosKeyId==giRecoveryKeyId_ChangeValue) 
	{
		// dans ce cas, l'utilisateur est probablement bloqu� car en d�calage sur le keyid
		// il ne peut donc faire de recouvrement, il faut forcer nouveau mot de passe + recalcul du recovery infos
		TRACE((TRACE_INFO,_F_,"KeyId du RecoveryInfos=%04d KeyId de ChangeIniValues=%04d KeyId � appliquer=%04d",giRecoveryInfosKeyId,giRecoveryKeyId_ChangeValue,giRecoveryKeyId));
		WritePrivateProfileString("swSSO","recoveryInfos",NULL,gszCfgFile);		
		WritePrivateProfileString("swSSO","pwdValue",NULL,gszCfgFile);		
		WritePrivateProfileString("swSSO","CheckSynchro",NULL,gszCfgFile);		
		giPwdProtection=0;	
		bChangeIni=TRUE;
	}
	if (bChangeIni) StoreIniEncryptedHash(); 

	// REMARQUE : la config proxy est lue plus loin dans le d�marrage du main, sinon la cl� n'est pas disponible
	//            pour d�chiffrer le mot de passe proxy !
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// AddComputerName()
// ----------------------------------------------------------------------------------
// Ajoute szComputer dans la liste Computers de la section swSSO, sauf si d�j� pr�sent.
// ----------------------------------------------------------------------------------
int AddComputerName(const char *szComputer)
{
	TRACE((TRACE_ENTER,_F_, "szComputer=%s",szComputer));
	int rc=-1;
	char *p=NULL;
	char *pContexte=NULL;

	// on commence par v�rifier qu'il n'est pas d�j� l�...
	GetPrivateProfileString("swSSO","Computers","",buf2048,sizeof(buf2048),gszCfgFile);
	p=strtok_s(buf2048,":",&pContexte);
	while (p!=NULL)
	{
		TRACE((TRACE_DEBUG,_F_,"Already present: %s",p));
		if (strcmp(p,szComputer)==0) goto end; // d�j� pr�sent, on arr�te
		p=strtok_s(NULL,":",&pContexte);
	}
	// pas trouv�, on l'ajoute
	// mais attention, strtok a p�t� le buf2048, donc il faut le relire dans la config...
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

	// pour l'�num�ration des computernames...
	if (gpNextComputerName!=NULL) // c'est bon, on en a encore un en stock
	{
		pRet=gpNextComputerName;
		gpNextComputerName=strtok_s(NULL,":",&gpComputerNameContext);
	}
	else // on n'a pas encore �num�r� ou alors on a �puis� le stock
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
// Sauvegarde la date de derni�re mise � jour des configs depuis le serveur
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

	sprintf_s(szItem,sizeof(szItem),"%d",giDomainId);
	WritePrivateProfileString("swSSO","domainId",szItem,gszCfgFile); 
	WritePrivateProfileString("swSSO","domainLabel",gszDomainLabel,gszCfgFile);

	WritePrivateProfileString("swSSO","sessionLock",gbSessionLock?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetCheckVersion",gbInternetCheckVersion?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetCheckBeta",gbInternetCheckBeta?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetGetConfig",gbInternetGetConfig?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","internetPutConfig",NULL,gszCfgFile); // 0.90B1
	WritePrivateProfileString("swSSO","internetManualPutConfig",gbInternetManualPutConfig?"YES":"NO",gszCfgFile);

	// 0.80B7 : sauvegarde de la config proxy li�e au computername
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

	// 0.80B9 : m�morise le nom de l'ordinateur (pour bouton "j'ai de la chance" dans config proxy)
	AddComputerName(gszComputerName);

	// 0.93B4 ISSUE#50 (?)
	WritePrivateProfileString("swSSO","parseWindowsOnStart",gbParseWindowsOnStart?"YES":"NO",gszCfgFile);

	// ISSUE#107
	WritePrivateProfileString("swSSO","displayChangeAppPwdDialog",gbDisplayChangeAppPwdDialog?"YES":"NO",gszCfgFile);

	// 1.03 : date de dernier changement mot de passe AD
	if (gbUseADPasswordForAppLogin)
	{
		// ISSUE#281 : n'�crit la valeur que si non vide
		// WritePrivateProfileString("swSSO","lastADPwdChange",gszLastADPwdChange,gszCfgFile);
		if (*gszLastADPwdChange!=0) WritePrivateProfileString("swSSO","lastADPwdChange",gszLastADPwdChange,gszCfgFile);
		if (*gszLastADPwdChange2!=0) WritePrivateProfileString("swSSO","lastADPwdChange2",gszLastADPwdChange2,gszCfgFile);
		WritePrivateProfileString("swSSO","ADPwd",gszEncryptedADPwd,gszCfgFile);
	}
	// ISSUE#176
	WritePrivateProfileString("swSSO","InternetExplorer",gbSSOInternetExplorer?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","Firefox",gbSSOFirefox?"YES":"NO",gszCfgFile);
	WritePrivateProfileString("swSSO","Chrome",gbSSOChrome?"YES":"NO",gszCfgFile);
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
// ATTENTION : le sel doit avoir �t� tir� avant l'appel � cette fonction
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

	// (le sel doit avoir �t� tir� avant l'appel � StoreMasterPwd !)
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
// Nouveau en v0.76 -> stockage du mdp maitre s�curis� par DPAPI
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
	
	// encodage base64 et �criture dans swsso.ini
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
// Nouveau en v0.76 -> lecture du mdp maitre s�curis� par DPAPI
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

	// lecture dans swsso.ini et d�codage base64 
	sprintf_s(szKey,sizeof(szKey),"pwdDAPIValue-%s@%s",gszUserName,gszComputerName);
	DataSalt.pbData=(BYTE*)szKey;
	DataSalt.cbData=strlen(szKey);
	GetPrivateProfileString("swSSO",szKey,"",szBase64,sizeof(szBase64),gszCfgFile);
	if (*szBase64==0) goto end;
	swCryptDecodeBase64(szBase64,DAPIData,sizeof(DAPIData));
	DataIn.pbData=(BYTE*)DAPIData;
	DataIn.cbData=strlen(szBase64)/2;

	// d�chiffrement du mot de passe 
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
// G�n�re et �crit une valeur checksyncho dans swsso.ini
//-----------------------------------------------------------------------------
int GenWriteCheckSynchroValue(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	BYTE bufSynchroValue[16+64+16]; // iv + donn�es utiles + padding
	char szSynchroValue[192+1]; // (16+64+16)*2+1 = 193
	
	// G�n�re un al�a pour l'iv et les donn�es � chiffrer
	brc=CryptGenRandom(ghProv,16+64,bufSynchroValue);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()=%ld",GetLastError())); goto end; }
	// Chiffre  avec la cl� ghKey1
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
// Lit et v�rifie la valeur checksynchro
// V�rifie que la cl� permet bien de d�chiffrer les mot de passe maitre
// En version SSO WINDOWS, c'est le moyen de contr�ler que le mot de passe Windows
// est bien synchronis� avec la cl� de chiffrement des mdp secondaires de swSSO
//-----------------------------------------------------------------------------
int ReadVerifyCheckSynchroValue(int iKeyId)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;
	
	BYTE bufSynchroValue[16+64+16]; // iv + donn�es utiles + padding
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
	// D�code le faux base 64
	swCryptDecodeBase64(szSynchroValue,(char*)bufSynchroValue,sizeof(bufSynchroValue));
	// Chiffre  avec la cl� ghKey1
	if (swCryptDecryptDataAES256(bufSynchroValue,bufSynchroValue+16,80,iKeyId)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur de d�chiffrement, cas de d�synchro !"));
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
// Appel�e lors du premier lancement pour un nouvel utilisateur (ini inexistant)
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
	
	// G�n�re les sels PSKS
	if (swGenPBKDF2Salt()!=0) goto end;

	// Envoie les sels � swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
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
	// TODO un jour : analyser la r�ponse
	// Ecrit les sels dans le .ini
	swCryptEncodeBase64(gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,szPBKDF2Salt);
	WritePrivateProfileString("swSSO","pwdSalt",szPBKDF2Salt,gszCfgFile);
	swCryptEncodeBase64(gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,szPBKDF2Salt);
	WritePrivateProfileString("swSSO","keySalt",szPBKDF2Salt,gszCfgFile);

	// Demande le keydata � swssosvc
	// Construit la requ�te � envoyer � swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
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
	// Cr�e la cl� de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	swStoreAESKey(AESKeyData,ghKey1);
	if (GenWriteCheckSynchroValue()!=0) goto end;
	StoreIniEncryptedHash(); // ISSUE#164
	RecoveryFirstUse(NULL,ghKey1); // ISSUE#194
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

	// Envoie les sels � swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
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
	// TODO un jour : analyser la r�ponse

	// Demande le keydata � swssosvc
	// Construit la requ�te � envoyer � swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
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
	// Cr�e la cl� de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	if (swStoreAESKey(AESKeyData,ghKey2)!=0) goto end;

	// Fait le transchiffrement du fichier .ini de ghKey1 vers ghKey2
	if (swTranscrypt()!=0) goto end; 
	
	// Copie la cl� dans ghKey1 qui est utilis�e pour chiffrer les mots de passe secondaires
	swStoreAESKey(AESKeyData,ghKey1);
	// Enregistrement des infos de recouvrement dans swSSO.ini (AESKeyData+UserId)Kpub
	if (RecoveryChangeAESKeyData(ghKey1)!=0) goto end;
	// enregistrement des actions, comme �a les identifiants sont rechiffr�s automatiquement avec la nouvelle cl�
	if(SaveApplications()!=0) goto end;
	
	// Supprime le pwdvalue, on n'en a plus besoin :-)
	WritePrivateProfileString("swSSO","pwdValue",NULL,gszCfgFile);
	WritePrivateProfileString("swSSO","pwdProtection","WINDOWS",gszCfgFile);
	// Supprime l'�ventuelle cl� pwdadpivalue
	sprintf_s(szKey,sizeof(szKey),"pwdDAPIValue-%s@%s",gszUserName,gszComputerName);
	WritePrivateProfileString("swSSO",szKey,NULL,gszCfgFile);
	// Et par contre on cr�e une nouvelle valeur CheckSynchro = al�a chiffr� puis jet�
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
// V�rifie le mot de passe Windows et cr�e la cl� de chiffrement des mdp
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

	// Envoie les sels � swSSOSVC : V03:PUTPSKS:domain(256octets)username(256octets)UPN(256octets)PwdSalt(64octets)KeySalt(64octets)
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
	// TODO un jour : analyser la r�ponse

	// Demande le keydata � swssosvc
	// Construit la requ�te � envoyer � swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
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
	// Cr�e la cl� de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	if (swStoreAESKey(AESKeyData,ghKey1)) goto end;
	
	// Teste que la cl� est OK, sinon essaie avec la pr�c�dente cl� si existante et 
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
		// Cr�e la cl� de chiffrement des mots de passe secondaires
		memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
		if (swStoreAESKey(AESKeyData,ghKey1)) goto end;
		if (ReadVerifyCheckSynchroValue(ghKey1)==0)
		{
			TRACE((TRACE_INFO,_F_,"Un transchiffrement va �tre effectu�"));
			*pbMigrationWindowsSSO=TRUE; // demande la migration (sera faite une fois les configs charg�es)
		}
		else
		{
			// Ni le mot de passe courant ni l'ancien ne permettent de d�chiffrer...
			// On est dans un cas de d�synchro, l'utilisateur doit faire appel 
			// au support pour resynchroniser son mot de passe.
			TRACE((TRACE_ERROR,_F_,"ReadVerifyCheckSynchroValue(CUR and OLD) failed..."));
			goto end;
		}
	}

	RecoveryFirstUse(NULL,ghKey1);
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
		else if (gpRecoveryKeyValue==NULL || *gszRecoveryInfos==0)
		{
			MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_LOGON),"swSSO",MB_OK | MB_ICONSTOP);
		}
		else // une cl� de recouvrement existe et que les recoveryInfos ont d�j� �t� stock�es
		{
			// nouveau en 1.08, un web service permet de faire la resynchro de mani�re transparente.
			// donc si le webservice est activ� et qu'on est en mode synchro mdp, n'affiche pas le message
			if (gbRecoveryWebserviceActive || MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_LOGON2),"swSSO",MB_OKCANCEL | MB_ICONEXCLAMATION)==IDOK)
			{
				// ISSUE#165
				if (RecoveryChallenge(NULL)==0) // 0=OK, -1=erreur, -2=l'utilisateur a annul�
					rc=-3;
				else 
					rc=-1;
			}
		}
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// CheckMasterPwd()
// ----------------------------------------------------------------------------------
// V�rification du mot de passe maitre
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
// DialogProc de la fen�tre de changement du mot de passe maitre
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
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
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
			
						// Password Policy
						if (!IsPasswordPolicyCompliant(szNewPwd1))
						{
							MessageBox(w,gszPwdPolicy_Message,"swSSO",MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							if (ChangeMasterPwd(szNewPwd1)!=0)
							{
								MessageBox(w,GetString(IDS_CHANGE_PWD_FAILED),"swSSO",MB_OK | MB_ICONEXCLAMATION);
							}
							else
							{
								// ISSUE#342 : r�percute le changement de mot de passe sur le serveur
								if (gbAdmin && !gbNoMasterPwd)
								{
									ServerAdminSetPassword(w,szNewPwd1);
								}
								else
								{
									MessageBox(w,GetString(IDS_CHANGE_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
								}
								EndDialog(w,IDOK);
							}
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
// DialogProc de la fen�tre de changement IMPOSE du mot de passe maitre
// Utilis�e aussi pour demander un nouveau mot de passe suite � un recovery
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
			// si recovery, titre diff�rent
			if (gbRecoveryRunning) SetDlgItemText(w,TX_FRAME,GetString(IDS_RECOVERY_NEW_PWD));
			MACRO_SET_SEPARATOR;
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
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
			
					// v�rifie que pas �gal au mot de passe actuel
					if (CheckMasterPwd(szNewPwd1)==0)
					{
						MessageBox(w,GetString(IDS_FORCE_CHANGE),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemText(w,TB_NEW_PWD1,"");
						SetDlgItemText(w,TB_NEW_PWD2,"");
						SetFocus(GetDlgItem(w,TB_NEW_PWD1));
					}
					else
					{
						// Password Policy 
						if (!IsPasswordPolicyCompliant(szNewPwd1))
						{
							MessageBox(w,gszPwdPolicy_Message,"swSSO",MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							if (ChangeMasterPwd(szNewPwd1)!=0)
							{
								if (!gbRecoveryRunning) MessageBox(w,GetString(IDS_CHANGE_PWD_FAILED),"swSSO",MB_OK | MB_ICONEXCLAMATION);
							}
							else
							{
								if (!gbRecoveryRunning) 
								{
									// ISSUE#342 : r�percute le changement de mot de passe sur le serveur
									if (gbAdmin && !gbNoMasterPwd)
									{
										ServerAdminSetPassword(w,szNewPwd1);
									}
									else
									{
										MessageBox(w,GetString(IDS_CHANGE_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
									}
								}
								EndDialog(w,IDOK);
							}
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
// Fen�tre de changement du mot de passe maitre
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
	if (bForced)
	{
		if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_FORCE_CHANGE_PWD),NULL,ForceChangeMasterPasswordDialogProc)==IDOK)
			rc=0;
	}
	else
	{
		// si changement non forc�, v�rifie que MinAge OK.
		BOOL bChangePwdAllowed=TRUE;
		if (giPwdPolicy_MinAge!=0)
		{
			time_t tLastChange,tNow;
			time(&tNow);
			tLastChange=GetMasterPwdLastChange();
			TRACE((TRACE_INFO,_F_,"tNow              =%ld",tNow));
			TRACE((TRACE_INFO,_F_,"tLastChange       =%ld",tLastChange));
			TRACE((TRACE_INFO,_F_,"diff              =%ld",tNow-tLastChange));
			TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinAge=%ld",giPwdPolicy_MinAge));
			if ((tNow-tLastChange) < (giPwdPolicy_MinAge*86400)) 
			{
				char szMsg[255+1];
				bChangePwdAllowed=FALSE;
				wsprintf(szMsg,GetString(IDS_PWD_CHANGED_RECENTLY),giPwdPolicy_MinAge);
				MessageBox(NULL,szMsg,"swSSO",MB_OK|MB_ICONEXCLAMATION);
			}
		}
		if (bChangePwdAllowed) DialogBox(ghInstance,MAKEINTRESOURCE(IDD_CHANGE_PWD),NULL,ChangeMasterPasswordDialogProc);
		rc=0;
	}
	gwChangeMasterPwd=NULL;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetMasterPwdLastChange()
// ----------------------------------------------------------------------------------
// R�cup�re la date du dernier changement du mot de passe maitre
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
		//on suppose que le vilain utilisateur a supprim� la ligne et on impose changement imm�diat ?
		// effet de bord : pour les utilisateurs actuels, changement impos� d�s activation de la politique.
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
// SaveMasterPwdLastChange()
// ----------------------------------------------------------------------------------
// Sauvegarde la date du dernier changement du mot de passe maitre
// ----------------------------------------------------------------------------------
int SaveMasterPwdLastChange(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	time_t t;
	char szTime[16]; 
	char *pszEncryptedTime=NULL; 

	if (giPwdPolicy_MaxAge==0 && giPwdPolicy_MinAge==0)
	{
		TRACE((TRACE_INFO,_F_,"Pas de politique de changement de mot de passe"));
		rc=0;
		goto end;
	}
	time(&t);
	wsprintf(szTime,"%ld",t);
	TRACE((TRACE_INFO,_F_,"szTime=%s",szTime));
	pszEncryptedTime=swCryptEncryptString(szTime,ghKey1);
	if (pszEncryptedTime==NULL) goto end;
	WritePrivateProfileString("swSSO","LastPwdChange",pszEncryptedTime,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	if (pszEncryptedTime!=NULL) free(pszEncryptedTime);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// ChangeWindowsPwd()
// ----------------------------------------------------------------------------------
// Le mot de passe Windows a chang�, r�percute le changement
// ----------------------------------------------------------------------------------
int ChangeWindowsPwd(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BYTE AESKeyData[AES256_KEY_LEN];
	char bufRequest[1024];
	char bufResponse[1024];
	DWORD dwLenResponse;
	
	// Demande le nouveau keydata � swssosvc
	// Construit la requ�te � envoyer � swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	// ISSUE#156 : pour y voir plus clair dans les traces
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)==0) 
	{
		if (dwLenResponse==PBKDF2_PWD_LEN+AES256_KEY_LEN)
		{
			// Cr�e la cl� de chiffrement des mots de passe secondaires
			memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
			swStoreAESKey(AESKeyData,ghKey2);
			// Fait le transchiffrement du fichier .ini de ghKey1 vers ghKey2
			if (swTranscrypt()!=0) goto end;
			// Copie la cl� dans ghKey1 qui est utilis�e pour chiffrer les mots de passe secondaires
			swStoreAESKey(AESKeyData,ghKey1);
			// Enregistrement des infos de recouvrement dans swSSO.ini (AESKeyData+UserId)Kpub
			if (RecoveryChangeAESKeyData(ghKey1)!=0) goto end;
			// enregistrement des actions, comme �a les identifiants sont rechiffr�s automatiquement avec la nouvelle cl�
			if(SaveApplications()!=0) goto end;
			// R�cup�re le nouveau mot de passe AD chiffr�
			if (GetADPassword()!=0) goto end;
			// ISSUE#342 : r�percute le changement de mot de passe sur le serveur
			if (gbAdmin)
			{
				char *pszADPassword=GetDecryptedPwd(gszEncryptedADPwd,TRUE);
				if (pszADPassword!=NULL) 
				{
					ServerAdminSetPassword(NULL,pszADPassword);
					SecureZeroMemory(pszADPassword,strlen(pszADPassword));
					free(pszADPassword);
				}
			}
			// Met � jour la valeur de checksynchro
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

	// 1.08 ISSUE#248 : si configur�, synchronise un groupe de mot de passe secondaires avec le mot de passe AD
	if (!gbAdmin && gbSyncSecondaryPasswordActive)
	{
		if (CheckUserInOU()) SyncSecondaryPasswordGroup();
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

	// g�n�re de nouveaux sels (ben oui, autant le changer en m�me temps que le mot de passe maitre)
	// qui seront pris en compte pour la d�rivation de la cl� AES et le stockage du mot de passe
	if (swGenPBKDF2Salt()!=0) goto end;

	// d�rivation de ghKey2 � partir du nouveau mot de passe
	if (swCryptDeriveKey(szNewPwd,ghKey2)!=0) goto end;

	// Transchiffrement
	if (swTranscrypt()!=0) goto end; 

	// d�rivation de ghKey1 � partir du nouveau mot de passe pour la suite
	if(swCryptDeriveKey(szNewPwd,ghKey1)!=0) goto end;
	
	// enregistrement du nouveau mot maitre dans swSSO.ini
	if(StoreMasterPwd(szNewPwd)!=0) goto end;

	// enregistrement des infos de recouvrement dans swSSO.ini (mdp maitre + code RH)Kpub
	if (RecoveryChangeAESKeyData(ghKey1)!=0) goto end;

	// inscrit la date de dernier changement de mot de passe dans le .ini
	// cette valeur est chiffr� par le (nouveau) mot de passe et �crite seulement si politique mdp d�finie
	SaveMasterPwdLastChange();

	// si rememberme, stockage prot�g� DAPI
	if (gbRememberOnThisComputer)
	{
		if (DPAPIStoreMasterPwd(szNewPwd)!=0) goto end;
	}

	// enregistrement des actions, comme �a les identifiants sont rechiffr�s automatiquement avec la nouvelle cl�
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
// Loggue les erreurs de transchiffrement dans un fichier portant le m�me nom
// et cr�� dans le m�me dossier que le fichier .ini, mais avec extension .err
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

	// �crit le message dans le fichier log
	hf=CreateFile(szFilename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFilename,GetLastError())); goto end; }
	// se positionne � la fin
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
		// d�chiffrement mot de passe
		pszDecryptedPwd=swCryptDecryptString(gptActions[i].szPwdEncryptedValue,ghKey1);
		if (pszDecryptedPwd==NULL)
		{
			// goto end; // ISSUE#276 : en cas d'erreur, g�n�re un log, marque l'entr�e comme invalide et continue
			*gptActions[i].szPwdEncryptedValue=0;
			sprintf_s(szMessage,sizeof(szMessage),"Erreur de tranchiffrement du mot de passe de la config #%d (%s)\r\n",i,gptActions[i].szApplication);
			LogTranscryptError(szMessage);
			if (!gptActions[i].bError) 
			{ 
				giNbTranscryptError++; // incr�ment seulement si pas d�j� marqu�e en erreur � cause de l'identifiant
				gptActions[i].bError=TRUE;
			}
			goto suite;
		}
		// rechiffrement avec la nouvelle cl�
		pszTranscryptedPwd=swCryptEncryptString(pszDecryptedPwd,ghKey2);
		SecureZeroMemory(pszDecryptedPwd,strlen(pszDecryptedPwd));
		if (pszTranscryptedPwd==NULL)
		{
			// goto end; // ISSUE#276 : en cas d'erreur, g�n�re un log, marque l'entr�e comme invalide et continue
			*gptActions[i].szPwdEncryptedValue=0;
			sprintf_s(szMessage,sizeof(szMessage),"Erreur de tranchiffrement du mot de passe de la config #%d (%s)\r\n",i,gptActions[i].szApplication);
			LogTranscryptError(szMessage);
			if (!gptActions[i].bError) 
			{ 
				giNbTranscryptError++; // incr�ment seulement si pas d�j� marqu�e en erreur � cause de l'identifiant
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
		// r�cup du prochain computer name
		pszComputerName=GetNextComputerName();
		// lecture de la valeur du mdp proxy pour ce computername
		sprintf_s(szItem,sizeof(szItem),"ProxyPwd-%s",pszComputerName);
		lenEncryptedProxyPwd=GetPrivateProfileString("swSSO",szItem,"",szEncryptedProxyPwd,sizeof(szEncryptedProxyPwd),gszCfgFile);
		TRACE((TRACE_INFO,_F_,"ProxyPwd-%s : len=%d",pszComputerName,lenEncryptedProxyPwd));
		if (lenEncryptedProxyPwd==LEN_ENCRYPTED_AES256) // mot de passe chiffr�. Dans tous les autres cas (pas de mot de passe ou non chiffr�, on ne fait rien !)
		{
			pszDecryptedPwd=swCryptDecryptString(szEncryptedProxyPwd,ghKey1);
			if (pszDecryptedPwd==NULL) goto suitead;
			//TRACE((TRACE_PWD,_F_,"pszDecryptedPwd=%s",pszDecryptedPwd));
			// d�chiffrement OK avec ghKey1, on rechiffre avec ghKey2
			pszTranscryptedPwd=swCryptEncryptString(pszDecryptedPwd,ghKey2);
			// 0.85B9 : remplacement de memset(pszDecryptedPwd,0,strlen(pszDecryptedPwd));
			SecureZeroMemory(pszDecryptedPwd,strlen(pszDecryptedPwd));
			if (pszTranscryptedPwd==NULL) goto suitead;
			// �criture dans le fichier ini
			WritePrivateProfileString("swSSO",szItem,pszTranscryptedPwd,gszCfgFile);
			StoreIniEncryptedHash(); // ISSUE#164
		}
		if (pszDecryptedPwd!=NULL) { free(pszDecryptedPwd); pszDecryptedPwd=NULL; }
		if (pszTranscryptedPwd!=NULL) { free(pszTranscryptedPwd); pszTranscryptedPwd=NULL; }
	}
suitead:
	// 1.03 - transchiffre aussi le mot de passe AD, si pr�sent
	if (gbUseADPasswordForAppLogin)
	{
		if (*gszEncryptedADPwd!=0)
		{
			// d�chiffrement mot de passe
			pszDecryptedPwd=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
			if (pszDecryptedPwd==NULL) goto bientotlafin;
			// rechiffrement avec la nouvelle cl�
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
// DialogProc de la fen�tre de changement de mot de passe d'application
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
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
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

//-----------------------------------------------------------------------------
// ChangeApplicationPassword()
//-----------------------------------------------------------------------------
// Propose une fen�tre de changement de mot de passe pour une application
// (affich�e suite � d�tection d'une r�apparition rapide d'une fenetre d'authent)
//-----------------------------------------------------------------------------
int ChangeApplicationPassword(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, "%d",iAction));
	int rc=-1;
	
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_FORCE_CHANGE_PWD),w,ChangeApplicationPasswordDialogProc,iAction)==IDOK) rc=0;

	gwChangeApplicationPassword=NULL;

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// SavePortal()
// ----------------------------------------------------------------------------------
// G�n�ration du fichier XML portail (0.78)
// ----------------------------------------------------------------------------------
void SavePortal()
{
	TRACE((TRACE_ENTER,_F_,"%s",gszCfgPortal));
	FILE *hf=NULL;
	char szTmpURL[LEN_URL+1]; // ISSUE#375
	char szCategoryIndex[8+1];
	int iCategory;
	int lenURL;
	int i,j;
	int iNbActionsInConfig;
	int iCategoryPortalIndex;

	if (*gszCfgPortal==0) goto end;

	errno_t err=fopen_s(&hf,gszCfgPortal,"w");
	if (err!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Cannot open file for writing : '%s'",gszCfgPortal));
		goto end;
	}
	
	fputs("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n",hf);
	fputs("<?xml-stylesheet type=\"text/xsl\" href=\"./swsso.xsl\"?>\n",hf); 
	fputs("<swsso>\n",hf);

	iCategoryPortalIndex=0;
	for (j=0;j<giNbCategories;j++)
	{
		TRACE((TRACE_DEBUG,_F_,"Categorie n�%d Identifiant %d",j,gptCategories[j].id));
		iNbActionsInConfig=0;
		for (i=0;i<giNbActions;i++)
		{
			// appli de cette cat�gorie ?
			if (gptActions[i].iCategoryId!=gptCategories[j].id) goto nextAction;
			// appli affichage dans le portail ?
			if (!gptActions[i].bActive || strstr(gptActions[i].szURL,"http")==NULL) goto nextAction;
			// ajout dans le portail
			iNbActionsInConfig++;
			fputs("\t<app>\n",hf);
			iCategory=GetCategoryIndex(gptActions[i].iCategoryId);
			if (iCategory!=-1)
			{
				fputs("\t\t<categoryLabel>",hf); fputs(gptCategories[iCategory].szLabel,hf); fputs("</categoryLabel>\n",hf);
				wsprintf(szCategoryIndex,"%d",iCategoryPortalIndex);
				fputs("\t\t<categoryIndex>",hf); fputs(szCategoryIndex,hf); fputs("</categoryIndex>\n",hf);
				wsprintf(szCategoryIndex,"%d",gptActions[i].iCategoryId);
				fputs("\t\t<categoryId>",hf); fputs(szCategoryIndex,hf); fputs("</categoryId>\n",hf);
			}
			fputs("\t\t<label>",hf); fputs(gptActions[i].szApplication,hf); fputs("</label>\n",hf);
			fputs("\t\t<url><![CDATA[",hf); // supprimer l'* en fin d'URL si pr�sente
			lenURL=strlen(gptActions[i].szURL);
			memcpy(szTmpURL,gptActions[i].szURL,lenURL+1);
			if (szTmpURL[lenURL-1]=='*') szTmpURL[lenURL-1]=0;
			fputs(szTmpURL,hf); 
			fputs("]]></url>\n",hf);
			fputs("\t\t<title><![CDATA[",hf); fputs(gptActions[i].szTitle,hf); fputs("]]></title>\n",hf);
			fputs("\t</app>\n",hf);
			TRACE((TRACE_DEBUG,_F_,"fputs(application '%s')",gptActions[i].szApplication));
nextAction:
			; // ligne vide volontaire
		}
		if (iNbActionsInConfig!=0) iCategoryPortalIndex++;
	}

#if 0
	for (i=0;i<giNbActions;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"Application '%s'",gptActions[i].szApplication));
		if (gptActions[i].bActive && strstr(gptActions[i].szURL,"http")!=NULL)
		{
			fputs("\t<app>\n",hf);
			iCategory=GetCategoryIndex(gptActions[i].iCategoryId);
			if (iCategory!=-1)
			{
				fputs("\t\t<categoryLabel>",hf); fputs(gptCategories[iCategory].szLabel,hf); fputs("</categoryLabel>\n",hf);
				wsprintf(szCategoryIndex,"%d",iCategory);
				fputs("\t\t<categoryIndex>",hf); fputs(szCategoryIndex,hf); fputs("</categoryIndex>\n",hf);
			}
			fputs("\t\t<label>",hf); fputs(gptActions[i].szApplication,hf); fputs("</label>\n",hf);
			fputs("\t\t<url><![CDATA[",hf); // supprimer l'* en fin d'URL si pr�sente
			lenURL=strlen(gptActions[i].szURL);
			memcpy(szTmpURL,gptActions[i].szURL,lenURL+1);
			if (szTmpURL[lenURL-1]=='*') szTmpURL[lenURL-1]=0;
			fputs(szTmpURL,hf); 
			fputs("]]></url>\n",hf);
			fputs("\t\t<title>",hf); fputs(gptActions[i].szTitle,hf); fputs("</title>\n",hf);
			fputs("\t</app>\n",hf);
			TRACE((TRACE_DEBUG,_F_,"fputs(application '%s')",gptActions[i].szApplication));
		}
	}
#endif
	fputs("</swsso>\n",hf);
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// StoreNodeValue()
// ----------------------------------------------------------------------------------
// Remplit la sz pass�e en param�tre avec la valeur du textenode
// ----------------------------------------------------------------------------------
// [rc] Taille de la valeur renseign�e ou -1 si erreur
// ----------------------------------------------------------------------------------
int StoreNodeValue(char *buf,int bufsize,IXMLDOMNode *pNode)
{
	TRACE((TRACE_ENTER,_F_, "bufsize=%d",bufsize));
	
	int rc=-1;
	HRESULT hr;
	IXMLDOMNode *pText=NULL;
	VARIANT vNodeValue;
	vNodeValue.vt=VT_EMPTY;

	hr=pNode->get_firstChild(&pText);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild()")); goto end; }
	if (pText==NULL) // champ vide
	{ 
		TRACE((TRACE_DEBUG,_F_,"pNode->get_firstChild()=NULL"));
		*buf=0; rc=0; 
	}
	else
	{
		hr=pText->get_nodeValue(&vNodeValue);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pText->get_nodeValue()")); goto end; }
		if (vNodeValue.vt!=VT_BSTR) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeValue() is not a BSTR !?")); goto end; }
		TRACE((TRACE_DEBUG,_F_,"vNodeValue.bstrVal='%S'",vNodeValue.bstrVal));
		rc=sprintf_s(buf,bufsize,"%S",vNodeValue.bstrVal);
	}	
end:
	if (vNodeValue.vt!=VT_EMPTY) VariantClear(&vNodeValue);
	if (pText!=NULL) pText->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// StoreNodeValueIfNotEmpty() -- pour ISSUE#212
// ----------------------------------------------------------------------------------
// Remplit la sz pass�e en param�tre avec la valeur du textenode
// ----------------------------------------------------------------------------------
// [rc] Taille de la valeur renseign�e ou -1 si erreur
// ----------------------------------------------------------------------------------
int StoreNodeValueIfNotEmpty(char *buf,int bufsize,IXMLDOMNode *pNode)
{
	TRACE((TRACE_ENTER,_F_, "bufsize=%d",bufsize));
	
	int rc=-1;
	HRESULT hr;
	IXMLDOMNode *pText=NULL;
	VARIANT vNodeValue;
	vNodeValue.vt=VT_EMPTY;

	hr=pNode->get_firstChild(&pText);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild()")); goto end; }
	if (pText==NULL) // champ vide
	{ 
		TRACE((TRACE_DEBUG,_F_,"pNode->get_firstChild()=NULL"));
		rc=0; 
	}
	else
	{
		hr=pText->get_nodeValue(&vNodeValue);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pText->get_nodeValue()")); goto end; }
		if (vNodeValue.vt!=VT_BSTR) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeValue() is not a BSTR !?")); goto end; }
		TRACE((TRACE_DEBUG,_F_,"vNodeValue.bstrVal='%S'",vNodeValue.bstrVal));
		if (SysStringLen(vNodeValue.bstrVal)==0) 
			rc=0;
		else
			rc=sprintf_s(buf,bufsize,"%S",vNodeValue.bstrVal);
	}	
end:
	if (vNodeValue.vt!=VT_EMPTY) VariantClear(&vNodeValue);
	if (pText!=NULL) pText->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
// ----------------------------------------------------------------------------------
// LookForConfig
// ----------------------------------------------------------------------------------
// Recherche de la configuration d'un SSO sur le serveur
// ----------------------------------------------------------------------------------
static BSTR LookForConfig(const char *szTitle, const char *szURL, const char *szIds,
						  char *szDate, BOOL bNew,BOOL bMod,BOOL bOld,int iType,int iAutoPublish)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BSTR bstrXML=NULL;
	//ISSUE#149
	//char szRequest[4096+1]; // attention, contient la liste des ids (500 configs * 4 octets => 2Ko au moins)
	char *pszParams=NULL;
	int sizeofParams;
	char *pszResult=NULL;
	char *pszEncodedURL=NULL;
	char *pszEncodedTitle=NULL;
	char *pszEncodedIds=NULL;
	char szType[3+1];
	char szVersion[3+1];
	DWORD dwStatusCode;
	
	if (strcmp(gcszCurrentBeta,"0000")==0) // pas de beta en cours
	{
		memcpy(szVersion,gcszCurrentVersion,3);
		szVersion[3]=0;
	}
	else
	{
		memcpy(szVersion,gcszCurrentBeta,3);
		szVersion[3]=0;
	}

	//0.87 : encodage des param�tre URL et titre (BUG#60)
	pszEncodedURL=HTTPEncodeParam((char*)szURL); if (pszEncodedURL==NULL) goto end;
	pszEncodedTitle=HTTPEncodeParam((char*)szTitle); if (pszEncodedTitle==NULL) goto end;
	pszEncodedIds=HTTPEncodeParam((char*)szIds); if (pszEncodedIds==NULL) goto end;

	// ISSUE#149 : calcul dynamique taille de la requ�te
	sizeofParams=strlen(pszEncodedIds)+4096; // 2 Ko + taille des ids
	pszParams=(char*)malloc(sizeofParams);
	if (pszParams==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeofParams)); goto end;};
	switch (iType)
	{
		case WINSSO : strcpy_s(szType,sizeof(szType),"WIN"); break;
		case XINSSO : strcpy_s(szType,sizeof(szType),"XIN"); break;
		case WEBSSO : strcpy_s(szType,sizeof(szType),"WEB"); break;
		case XEBSSO : strcpy_s(szType,sizeof(szType),"XEB"); break;
		case POPSSO : strcpy_s(szType,sizeof(szType),"POP"); break;
		default: *szType=0; 
	}

	//&debug=1
	if (iAutoPublish)
	{
		sprintf_s(pszParams,sizeofParams,"?action=getconfigautopublish&title=%s&url=%s&ids=%s&date=%s&new=%d&mod=%d&old=%d&type=%s&domainId=%d&version=%s",
				pszEncodedTitle,
				pszEncodedURL,
				pszEncodedIds,
				*szDate==0?"20000101000000":szDate,
				bNew,bMod,bOld,szType,giDomainId,szVersion);
	}
	else
	{
		sprintf_s(pszParams,sizeofParams,"?action=getconfig&title=%s&url=%s&ids=%s&date=%s&new=%d&mod=%d&old=%d&type=%s&domainId=%d&version=%s",
				pszEncodedTitle,
				pszEncodedURL,
				pszEncodedIds,
				*szDate==0?"20000101000000":szDate,
				bNew,bMod,bOld,szType,giDomainId,szVersion);
	}
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",pszParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  pszParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",pszParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",pszParams)); goto end; }

	if (*pszResult==0) // cas o� la m�thode est inconnue donc retour vide du webservice
	{
		bstrXML=SysAllocString(L"<app>NOT FOUND</app>");
		if (bstrXML==NULL) goto end;
	}
	else
	{
		bstrXML=GetBSTRFromSZ(pszResult);
		if (bstrXML==NULL) goto end;
	}

end:
	if (pszEncodedURL!=NULL) free(pszEncodedURL);
	if (pszEncodedTitle!=NULL) free(pszEncodedTitle);
	if (pszEncodedIds!=NULL) free(pszEncodedIds);
	if (pszResult!=NULL) free(pszResult);
	if (pszParams!=NULL) free(pszParams);
	TRACE((TRACE_LEAVE,_F_, ""));
	return bstrXML;
}

// ----------------------------------------------------------------------------------
// PutConfigOnServer
// ----------------------------------------------------------------------------------
// D�pose la configuration de l'action iAction sur le serveur
// ----------------------------------------------------------------------------------
// Retour : 0=OK, -1=erreur, -2=configuration ignor�e
// ----------------------------------------------------------------------------------
int PutConfigOnServer(int iAction,int *piNewCategoryId,char *pszDomainIds,char *pszDomainAutoPublish)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));

	char szType[3+1];
	int rc=-1;
	char szParams[2048+1]; // augment� en 1.02 pour ISSUE#160 (taille champ URL 128 -> 256)
	char *pszResult=NULL;
	char szId2Type[5+1]="";
	char szId3Type[5+1]="";
	char szId4Type[5+1]="";
	char *pszEncodedURL=NULL;
	char *pszEncodedTitle=NULL;
	int iCategoryIndex=-1;
	int iNewCategoryId;
	char szCategId[10+1];
	char szCategLabel[LEN_CATEGORY_LABEL+1];
	char szLastModified[14+1]; // AAAAMMJJHHMMSS
	SYSTEMTIME localTime;
	char *p;
	int i;
	char *pszDecryptedPwd=NULL;
	char szWithIdPwd[20+1]="";
	int iWithIdPwd=0;
	DWORD dwStatusCode;
	
	TRACE((TRACE_ENTER,_F_, "pszDomainIds=%s",pszDomainIds));
	TRACE((TRACE_ENTER,_F_, "pszDomainAutoPublish=%s",pszDomainAutoPublish));

	*piNewCategoryId=-1;

	if (*gptActions[iAction].szTitle==0) // on ne remonte surtout pas les configs � titre vide car matche avec tout !
	{
		TRACE((TRACE_INFO,_F_,"Titre vide, on ne remonte pas cette config !")); rc=-2; goto end; 
	}
	if ((gptActions[iAction].iType==WEBSSO || gptActions[iAction].iType==XEBSSO) && *gptActions[iAction].szURL==0) // on ne remonte surtout pas les configs WEB � url vide
	{
		TRACE((TRACE_INFO,_F_,"URL vide, on ne remonte pas cette config !")); rc=-2; goto end; 
	}
	//TODO: il ne faudrait pas non plus remonter les configs de popup firefox sans URL... mais comment les reconnaitre ???
	if (gptActions[iAction].iType==WINSSO)		strcpy_s(szType,sizeof(szType),"WIN");
	else if (gptActions[iAction].iType==XINSSO)	strcpy_s(szType,sizeof(szType),"XIN");
	else if (gptActions[iAction].iType==WEBSSO)	strcpy_s(szType,sizeof(szType),"WEB");
	else if (gptActions[iAction].iType==XEBSSO)	strcpy_s(szType,sizeof(szType),"XEB");
	else if (gptActions[iAction].iType==POPSSO)	strcpy_s(szType,sizeof(szType),"POP");
	else strcpy_s(szType,sizeof(szType),"UNK");

	if (*gptActions[iAction].szId2Name!=0)
	{
		switch (gptActions[iAction].id2Type)
		{
			case EDIT:		  strcpy_s(szId2Type,sizeof(szId2Type),"EDIT"); break;
			case COMBO:		  strcpy_s(szId2Type,sizeof(szId2Type),"COMBO"); break;
			default:		  strcpy_s(szId2Type,sizeof(szId2Type),"UNK");
		}
	}
	if (*gptActions[iAction].szId3Name!=0)
	{
		switch (gptActions[iAction].id3Type)
		{
			case EDIT:		  strcpy_s(szId3Type,sizeof(szId3Type),"EDIT"); break;
			case COMBO:		  strcpy_s(szId3Type,sizeof(szId3Type),"COMBO"); break;
			default:		  strcpy_s(szId3Type,sizeof(szId3Type),"UNK");
		}
	}
	if (*gptActions[iAction].szId4Name!=0)
	{
		switch (gptActions[iAction].id4Type)
		{
			case EDIT:		  strcpy_s(szId4Type,sizeof(szId4Type),"EDIT"); break;
			case COMBO:		  strcpy_s(szId4Type,sizeof(szId4Type),"COMBO"); break;
			case CHECK_LABEL: strcpy_s(szId4Type,sizeof(szId4Type),"CHECK"); break;
			default:		  strcpy_s(szId4Type,sizeof(szId4Type),"UNK");
		}
	}

	//0.87 : encodage des param�tres URL et titre (BUG#60)
	pszEncodedURL=HTTPEncodeParam(gptActions[iAction].szURL);
	if (pszEncodedURL==NULL) goto end;
	pszEncodedTitle=HTTPEncodeParam((char*)gptActions[iAction].szTitle);
	if (pszEncodedTitle==NULL) goto end;

	if (gbCategoryManagement)
	{
		// szCategId
		sprintf_s(szCategId,sizeof(szCategId),"%d",gptActions[iAction].iCategoryId);
		// szCategLabel
		iCategoryIndex=GetCategoryIndex(gptActions[iAction].iCategoryId);
		if (iCategoryIndex==-1) goto end;
		strcpy_s(szCategLabel,sizeof(szCategLabel),gptCategories[iCategoryIndex].szLabel);
		TRACE((TRACE_DEBUG,_F_,"szCategId=%s | szCategLabel=%s",szCategId,szCategLabel));
	}
	else
	{
		*szCategId=0;
		*szCategLabel=0;
	}
	
	GetLocalTime(&localTime);
	sprintf_s(szLastModified,sizeof(szLastModified),"%04d%02d%02d%02d%02d%02d",
														localTime.wYear,localTime.wMonth,localTime.wDay,
														localTime.wHour,localTime.wMinute,localTime.wSecond);
	TRACE((TRACE_DEBUG,_F_,"szLastModified=%s",szLastModified));

	sprintf_s(szParams,sizeof(szParams),"?action=putconfig&configId=%d&title=%s&url=%s&typeapp=%s&id1Name=%s&id1Type=EDIT&pwdName=%s&validateName=%s&id2Name=%s&id2Type=%s&id3Name=%s&id3Type=%s&id4Name=%s&id4Type=%s&bKBSim=%d&szKBSim=%s&szName=%s&categId=%s&categLabel=%s&szFullPathName=%s&lastModified=%s&domainId=%s&pwdGroup=%d&autoLock=%d&domainAutoPublish=%s",
				gptActions[iAction].iConfigId,
				pszEncodedTitle,
				pszEncodedURL,
				szType,
				gptActions[iAction].szId1Name,
				gptActions[iAction].szPwdName,
				gptActions[iAction].szValidateName,
				gptActions[iAction].szId2Name,
				szId2Type,
				gptActions[iAction].szId3Name,
				szId3Type,
				gptActions[iAction].szId4Name,
				szId4Type,
				gptActions[iAction].bKBSim,
				gptActions[iAction].szKBSim,
				gptActions[iAction].szApplication,
				szCategId,
				szCategLabel,
				gptActions[iAction].szFullPathName,
				szLastModified,
				pszDomainIds,
				gptActions[iAction].iPwdGroup,
				gptActions[iAction].bAutoLock,
				pszDomainAutoPublish); 
	if (gbShowMenu_UploadWithIdPwd) 
	{
		iWithIdPwd=0;
		if (*gptActions[iAction].szId1Value!=0) { iWithIdPwd|=CONFIG_WITH_ID1; strcat_s(szParams,sizeof(szParams),"&id1Value="); strcat_s(szParams,sizeof(szParams),gptActions[iAction].szId1Value); }
		if (*gptActions[iAction].szId2Value!=0) { iWithIdPwd|=CONFIG_WITH_ID2; strcat_s(szParams,sizeof(szParams),"&id2Value="); strcat_s(szParams,sizeof(szParams),gptActions[iAction].szId2Value); }
		if (*gptActions[iAction].szId3Value!=0) { iWithIdPwd|=CONFIG_WITH_ID3; strcat_s(szParams,sizeof(szParams),"&id3Value="); strcat_s(szParams,sizeof(szParams),gptActions[iAction].szId3Value); }
		if (*gptActions[iAction].szId4Value!=0) { iWithIdPwd|=CONFIG_WITH_ID4; strcat_s(szParams,sizeof(szParams),"&id4Value="); strcat_s(szParams,sizeof(szParams),gptActions[iAction].szId4Value); }

		pszDecryptedPwd=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
		if (pszDecryptedPwd!=NULL)
		{
			if (*pszDecryptedPwd!=0) { iWithIdPwd|=CONFIG_WITH_PWD; strcat_s(szParams,sizeof(szParams),"&pwdValue="); strcat_s(szParams,sizeof(szParams),pszDecryptedPwd); }
		}
		sprintf_s(szWithIdPwd,sizeof(szWithIdPwd),"&withIdPwd=%d",iWithIdPwd);
		strcat_s(szParams,sizeof(szParams),szWithIdPwd);
	}
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,gwcszAdminCookie,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	TRACE((TRACE_INFO,_F_,"Result : %s",pszResult));

	// 0.91 : v�rifie que la requete a bien retourn� OK, sinon on sort en erreur
	// Si ok, le format de la r�ponse est "OK:configId:categId"
	if (pszResult[0]!='O' || pszResult[1]!='K') { TRACE((TRACE_ERROR,_F_,"Result=%s",pszResult)); goto end; }
	p=strchr(pszResult+3,':'); if (p==NULL) { TRACE((TRACE_ERROR,_F_,"Result=%s",pszResult)); goto end; }
	*p=0;
	// lecture du configId g�n�r� en base de donn�es et remplissage du champ iConfigId
	gptActions[iAction].iConfigId=atoi(pszResult+3);
	TRACE((TRACE_INFO,_F_,"gptActions[%d].iConfigId=%d",iAction,gptActions[iAction].iConfigId));
	if (gbCategoryManagement)
	{
		// lecture du categId attribu� par le serveur : si diff�rent du categId initial, c'est que la cat�gorie
		// vient d'�tre remont�e pour la 1�re fois sur le serveur, il faut donc modifier l'id de la cat�gorie
		// avec cette nouvelle valeur et corriger toutes les applications concern�es.
		p++;
		iNewCategoryId=atoi(p);
		*piNewCategoryId=iNewCategoryId;
		TRACE((TRACE_INFO,_F_,"iNewCategoryId=%d gptActions[%d].iCategoryId=%d",iNewCategoryId,iAction,gptActions[iAction].iCategoryId));
		if (iNewCategoryId!=gptActions[iAction].iCategoryId)
		{
			// retrouve toutes les applications de cette cat�gorie et modifie le lien
			for (i=0;i<giNbActions;i++)
			{
				if (gptActions[i].iCategoryId==gptCategories[iCategoryIndex].id) 
					gptActions[i].iCategoryId=iNewCategoryId;
			}
			// retrouve la cat�gorie et modifie son id
			gptCategories[iCategoryIndex].id=iNewCategoryId;
		}
	}	
	gptActions[iAction].bConfigSent=TRUE;
	strcpy_s(gptActions[iAction].szLastUpload,sizeof(gptActions[iAction].szLastUpload),szLastModified);

	// 0.90B1 : on ne sauvegarde plus (risque d'�crire dans une section renomm�e)
	// WritePrivateProfileString(gptActions[iAction].szApplication,"configSent","YES",gszCfgFile);

	// 0.91 : par contre une sauvegarde globale est faite dans la fonction appelante (UploadConfig())

	rc=0;
end:
	if (pszDecryptedPwd!=NULL) free(pszDecryptedPwd);
	if (pszEncodedTitle!=NULL) free(pszEncodedTitle);
	if (pszEncodedURL!=NULL) free(pszEncodedURL);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, ""));
	return rc;
}

// ----------------------------------------------------------------------------------
// AddApplicationFromXML()
// ----------------------------------------------------------------------------------
// Parse le fichier XML et renseigne les attributs dans la nouvelle action
// ----------------------------------------------------------------------------------
// bGetAll : TRUE = nouveau en 0.91 = r�cup�re TOUTES les configs actives sur le serveur
//           FALSE = idem versions pr�c�dente : ne r�cup�re que la config qui matche
//           REMARQUE : m�me avec FALSE, il peut y avoir plusieur APP dans le cas des 
//                      popups IE et des fenetres WINDOWS. Dans ce cas, il faut appeler 
//                      CheckURL pour v�rifier la config qui matche.
//						Un autre crit�re de choix est de prendre prioritairement les 
//						configurations qui ont un id!=0 ! Il n'y a rien � faire dans le code
//						ici, c'est simplement getconfig c�t� serveur qui retourne les configs
//						tri�es dans le bon sens !
// ----------------------------------------------------------------------------------
// rc : 0 OK, -1 erreur, -2 nb max configs atteint
// ----------------------------------------------------------------------------------
/* Format du fichier XML retourn� par le serveur (param�tre bstrXML): 
<app num=NNN>
	<configId></configId>
	<active></active>
	<lastModified></lastModified>
	<type></type>
	<title></title>
	<url></url>
	<id1Name></id1Name>
	<id1Type></id1Type>
	<id2Name></id2Name>
	<id2Type></id2Type>
	<id3Name></id3Name>
	<id3Type></id3Type>
	<id4Name></id4Name>
	<id4Type></id4Type>
	<id5Name></id5Name>
	<id5Type></id5Type>
	<pwdName></pwdName>
	<validateName></validateName>
	<bKBSim></bKBSim>
	<szKBSIM></szKBSIM>
	<szName></szName>
	<szFullPathName></szFullPathName>
	<categId></categId>
	<categLabel></categLabel>
	<domainId></domainId>
	<id1Value></domainId>
	<id2Value></domainId>
	<id3Value></domainId>
	<id4Value></domainId>
	<pwdValue></domainId>
</app>
*/
static int AddApplicationFromXML(HWND w,BSTR bstrXML,BOOL bGetAll)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int rc=-1;
	HRESULT hr;
	IXMLDOMDocument *pDoc=NULL;
	IXMLDOMNode		*pRoot=NULL;
	IXMLDOMNode		*pNode=NULL;
	IXMLDOMNode		*pChildApp=NULL;
	IXMLDOMNode		*pChildElement=NULL;
	IXMLDOMNode		*pNextChildApp=NULL;
	IXMLDOMNode		*pNextChildElement=NULL;

	VARIANT_BOOL vbXMLLoaded=VARIANT_FALSE;
	BSTR bstrNodeName=NULL;
	BOOL bApplicationFound=FALSE;
	int iReplaceExistingConfig=0;
	BOOL bReplaceExistingConfig=FALSE;

	char *pszEncodedURL=NULL;
	char *pszDecodedURL=NULL;

	int iConfigId;
	int iAction=-1;
	int *ptiActions=NULL;

	gbDontAskId=TRUE;
	gbDontAskId2=TRUE;
	gbDontAskId3=TRUE;
	gbDontAskId4=TRUE;
	gbDontAskPwd=TRUE;

	int iCurWithIdPwd=0;

	hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument,(void**)&pDoc);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IXMLDOMDocument)=0x%08lx",hr)); goto end; }

	hr = pDoc->loadXML(bstrXML,&vbXMLLoaded);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML()=0x%08lx",hr)); goto end; }
	if (vbXMLLoaded==VARIANT_FALSE) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML() returned FALSE")); goto end; }
	
	hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void **)&pRoot);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pXMLDoc->QueryInterface(IID_IXMLDOMNode)=0x%08lx",hr)); goto end;	}

	hr=pRoot->get_firstChild(&pNode);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pRoot->get_firstChild(&pNode)")); goto end; }

	hr=pNode->get_firstChild(&pChildApp);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildApp)")); goto end; }

	while (pChildApp!=NULL && !bApplicationFound) // parcours de tous les noeuds app
	{
		int i,j;
		iReplaceExistingConfig=0; 
		bReplaceExistingConfig=FALSE;
		iCurWithIdPwd=0;

		TRACE((TRACE_DEBUG,_F_,"<app>"));
		hr=pChildApp->get_firstChild(&pChildElement);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildElement)")); goto end; }

		while (pChildElement!=NULL) // parcours de tous les childs fils de <app>
		{
			hr=pChildElement->get_nodeName(&bstrNodeName);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeName()")); goto end; }
			TRACE((TRACE_DEBUG,_F_,"<%S>",bstrNodeName));

			if (CompareBSTRtoSZ(bstrNodeName,"configId")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					iConfigId=atoi(tmp);
					// le configId doit �tre recherch� pour voir si la configuration existe d�j�.
					// sauf si bGetAll=FALSE auquel cas on ne se pose pas de question et on ajoute !
					if (bGetAll)
					{
						if (iConfigId==0) // je ne sais pas trop quand �a peut arriver mais bon, dans ce cas, on ajoute � la fin
						{
							iAction=giNbActions;
						}
						else // v�rification de l'existence du configid
						{
							for (i=0;i<giNbActions;i++)
							{
								if (gptActions[i].iConfigId==iConfigId) iReplaceExistingConfig++;
								// ISSUE#24 : le probl�me, c'est qu'il peut y en avoir plusieurs, on r�f�rence les indices dans un tableau
							}
							if (iReplaceExistingConfig==0) // pas trouv�, on ajoute � la fin
							{
								iAction=giNbActions;
							}
							else
							{
								bReplaceExistingConfig=TRUE;
								ptiActions=(int*)malloc(sizeof(int)*iReplaceExistingConfig);
								if (ptiActions==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(int)*iReplaceExistingConfig)); goto end; }
								j=0;
								for (i=0;i<giNbActions;i++)
								{
									if (gptActions[i].iConfigId==iConfigId) { ptiActions[j]=i; j++; }
								}
							}
						}
					}
					else // bgetAll=FALSE => on ajoute la config � la fin
					{
						iAction=giNbActions;
					}
					// � ce stade, si configId trouv�, alors le tableau ptiActions contient la liste des iActions 
					// portant cet configId. Sinon iAction=giNbActions
					TRACE((TRACE_DEBUG,_F_,"iConfigId             =%d",iConfigId));
					TRACE((TRACE_DEBUG,_F_,"iAction               =%d",iAction));
					TRACE((TRACE_DEBUG,_F_,"iReplaceExistingConfig=%d",iReplaceExistingConfig));
#ifdef TRACES_ACTIVEES
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						TRACE((TRACE_DEBUG,_F_,"ptiActions[%d]=%d",i,ptiActions[i]));
					}
#endif
					if (iReplaceExistingConfig==0) // nouvelle action, quelques initialisations
					{
						// ISSUE#149
						if (giNbActions>=giMaxConfigs) { TRACE((TRACE_ERROR,_F_,"giNbActions=%d > giMaxConfigs=%d",giNbActions,giMaxConfigs)); rc=-2; goto end; }
						ZeroMemory(&gptActions[iAction],sizeof(T_ACTION));
						//gptActions[iAction].wLastDetect=NULL;
						//gptActions[iAction].tLastDetect=-1;
						gptActions[iAction].tLastSSO=-1;
						gptActions[iAction].wLastSSO=NULL;
						gptActions[iAction].iWaitFor=giWaitBeforeNewSSO;
						gptActions[iAction].bActive=gbActivateNewConfigs;
						gptActions[iAction].bAutoLock=TRUE;
						//gptActions[iAction].bAutoPublish=FALSE;
						gptActions[iAction].bConfigSent=TRUE;
						gptActions[iAction].bSaved=FALSE; // 0.93B6 ISSUE#55
						gptActions[iAction].iConfigId=iConfigId;
						gptActions[iAction].iWithIdPwd=0;
						gptActions[iAction].iPwdGroup=-1;
						TRACE((TRACE_DEBUG,_F_,"gptActions[iAction].iConfigId=%d",gptActions[iAction].iConfigId));
						if (bGetAll) giNbActions++; // sinon � ne pas faire, sera fait dans AddAppplicationFromCurrentWindow !
					}
					// Pour simplifier les corrections li�es � #ISSUE24, on constitue le m�me tableau ptiActions
					// quand on ajoute 1 config que quand on en replace 1 � N
					if (iReplaceExistingConfig==0)
					{
						ptiActions=(int*)malloc(sizeof(int));
						if (ptiActions==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(int))); goto end; }
						ptiActions[0]=iAction;
						iReplaceExistingConfig=1;
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"active")) 
			{
				char tmp[1+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc!=0) 
				{
					if (tmp[0]=='0' && gbDisableArchivedConfigsAtStart) // config archiv�e -> on la d�sactive !
					{
						// ISSUE#53 : si on a ajout� cette config, c'est con, elle est d�sactiv�e ! => Il faut la retirer !
						if (!bReplaceExistingConfig) { giNbActions--; goto nextChildApp; }
						// En plus de d�sactiver la config, on remet son Id � 0 : ainsi, l'utilisateur pourra la supprimer.
						for (i=0;i<iReplaceExistingConfig;i++)
						{
							TRACE((TRACE_INFO,_F_,"D�sactivation et raz du ConfigId de la config %d (id=%d,name=%s)",ptiActions[i],gptActions[ptiActions[i]].iConfigId,gptActions[ptiActions[i]].szApplication));
							gptActions[ptiActions[i]].bActive=FALSE;
							gptActions[ptiActions[i]].iConfigId=0;
							gtConfigSync.iNbConfigsDisabled++;
						}
					}
					else
					{
						if (bReplaceExistingConfig) gtConfigSync.iNbConfigsModified+=iReplaceExistingConfig; else gtConfigSync.iNbConfigsAdded++;
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"lastModified")) 
			{
				// v�rifie la date locale de la config : ne l'update que si date diff�rente.
				// (si la date est identique, c'est que c'est l'uploader qui vient de r�cup�rer
				// la config, c'est donc totalement inutile de la traiter)
				char szXMLLastModified[19+1]; // FORMAT retourn� dans xml : AAAA-MM-JJ HH:MM:SS
				char szIniLastModified[14+1]; // FORMAT stockage .ini : AAAAMMJJHHMMSS
				rc=StoreNodeValue(szXMLLastModified,sizeof(szXMLLastModified),pChildElement);
				if (rc>0) 
				{
					szIniLastModified[0]=szXMLLastModified[0];
					szIniLastModified[1]=szXMLLastModified[1];
					szIniLastModified[2]=szXMLLastModified[2];
					szIniLastModified[3]=szXMLLastModified[3];
					szIniLastModified[4]=szXMLLastModified[5];
					szIniLastModified[5]=szXMLLastModified[6];
					szIniLastModified[6]=szXMLLastModified[8];
					szIniLastModified[7]=szXMLLastModified[9];
					szIniLastModified[8]=szXMLLastModified[11];
					szIniLastModified[9]=szXMLLastModified[12];
					szIniLastModified[10]=szXMLLastModified[14];
					szIniLastModified[11]=szXMLLastModified[15];
					szIniLastModified[12]=szXMLLastModified[17];
					szIniLastModified[13]=szXMLLastModified[18];
					szIniLastModified[14]=0;
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						TRACE((TRACE_DEBUG,_F_,"szIniLastModified=%s gptActions[iAction].szLastUpload=%s",szIniLastModified,gptActions[ptiActions[i]].szLastUpload));
						if (strcmp(szIniLastModified,gptActions[ptiActions[i]].szLastUpload)==0)
						{
							if (bReplaceExistingConfig) gtConfigSync.iNbConfigsModified--; // compense le ++ fait lors du test sur valeur 'active'
							// Suite des impacts de ISSUE#24 : ne passe pas � la config suivante car il faut malgr� tout
							// r�percuter la modification sur l'ensemble des configs qui ont ce configId
							// goto nextChildApp;
						}
						else
						{
							strcpy_s(gptActions[ptiActions[i]].szLastUpload,sizeof(gptActions[ptiActions[i]].szLastUpload),szIniLastModified);
						}
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"type")) 
			{
				// 0.92B3 : correction du bug signal� par SB (#) 
				// <type> est le premier champ retourn� par webservice2.php
				// si iAction=-1 c'est qu'on n'a pas r�cup�r� le champ configid, donc il faut affecter iAction
				// iAction=giNbActions; ==> la correction 0.92B3 ci-dessus a introduit une r�gression 
				//                          je corrige en 0.92B4 on n'initialise iAction que si ==-1 !!
				// 0.92B8 : adaptation � la correction de ISSUE#24, c'est le tableau qu'on initialise et non plus iAction
				//if (iAction==-1) iAction=giNbActions;
				if (ptiActions==NULL) 
				{
					ptiActions=(int*)malloc(sizeof(int));
					if (ptiActions==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(int))); goto end; }
					ptiActions[0]=giNbActions;
					iReplaceExistingConfig=1;
				}
				char tmp[3+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc==0) goto end;
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					if (strcmp(tmp,"WIN")==0) gptActions[ptiActions[i]].iType=WINSSO;
					else if (strcmp(tmp,"XIN")==0) gptActions[ptiActions[i]].iType=XINSSO;
					else if (strcmp(tmp,"WEB")==0) gptActions[ptiActions[i]].iType=WEBSSO;
					else if (strcmp(tmp,"XEB")==0) gptActions[ptiActions[i]].iType=XEBSSO;
					else if (strcmp(tmp,"POP")==0) 
					{
						gptActions[ptiActions[i]].iType=POPSSO;
						// 0.91 : correction de ce qui devait �tre un bug si 1003 et 1005 n'�taient pas dans la base
						//        => les champs id et mdp �taient alors gris�s !
						gbDontAskId=FALSE;
						gbDontAskPwd=FALSE;
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"title"))
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szTitle,sizeof(gptActions[ptiActions[i]].szTitle),pChildElement);
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"url")) 
			{
				// 0.87 : d�codage du param�tre URL (BUG#60)
				// NON, finalement c'est inutile car l'URL remont�e est d�cod�e par PHP
				// donc ce qui arrive dans le xml du getconfig n'est pas encod� !
				// ISSUE#283 : en fait si, il faut faire quelque chose car le & est encod� en %26 et revient tel quel...
				pszEncodedURL=(char*)malloc(LEN_URL+1);
				if (pszEncodedURL==NULL) { TRACE((TRACE_DEBUG,_F_,"malloc(%d)",LEN_URL+1)); goto end; }
				rc=StoreNodeValue(pszEncodedURL,LEN_URL+1,pChildElement);
				pszDecodedURL=HTTPDecodeParam(pszEncodedURL);
				if (pszDecodedURL==NULL) goto end;
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					strcpy_s(gptActions[ptiActions[i]].szURL,LEN_URL+1,pszDecodedURL);
					//rc=StoreNodeValue(gptActions[ptiActions[i]].szURL,LEN_URL+1,pChildElement);
				}
				free(pszEncodedURL); pszEncodedURL=NULL;
				free(pszDecodedURL); pszDecodedURL=NULL;
				// le champ URL peut �ventuellement �tre non renseign�, 
				// donc rc=0 ne doit pas �tre une erreur et il faut consid�rer que le champ a �t� trouv�
				// si on est sur une fenetre windows et que l'URL retourn�e n'est pas vide,
				// on regarde si elle matche. Si oui, c'est elle, on arr�te, sinon on continue.
				if (!bGetAll)
				{
					if (gptActions[ptiActions[0]].iType==WINSSO && *gptActions[ptiActions[0]].szURL!=0)
					{
						TRACE((TRACE_INFO,_F_,"C'est une appli WINDOWS et URL non vide, il faut verifier si l'URL matche"));
						if (CheckURL(w,ptiActions[0])) // cool, on a trouv� !
						{
							TRACE((TRACE_INFO,_F_,"Cool, application trouvee dans la liste retournee !"));
							// il faut laisser finir la lecture de tous les �l�ments
							// et quand ce sera fait, on pourra quitter
							bApplicationFound=TRUE;
						}
						else
						{
							TRACE((TRACE_INFO,_F_,"Ce n'est pas celle la, on passe direct � la suivante"));
							goto nextChildApp;
						}
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id1Name"))
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szId1Name,sizeof(gptActions[ptiActions[i]].szId1Name),pChildElement);
				}
				if (rc>0) { gbDontAskId=FALSE; }
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"pwdName")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szPwdName,sizeof(gptActions[ptiActions[i]].szPwdName),pChildElement);
				}
				if (rc>0) { gbDontAskPwd=FALSE; }
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id2Name")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szId2Name,sizeof(gptActions[ptiActions[i]].szId2Name),pChildElement);
					TRACE((TRACE_DEBUG,_F_, "id2Name=%s",gptActions[ptiActions[i]].szId2Name));
				}
				if (rc>0) { gbDontAskId2=FALSE; }
				
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id2Type")) 
			{
				char tmp[5+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc!=0)
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						if (strcmp(tmp,"EDIT")==0) gptActions[ptiActions[i]].id2Type=EDIT;
						else if (strcmp(tmp,"COMBO")==0) gptActions[ptiActions[i]].id2Type=COMBO;
						else gptActions[ptiActions[i]].id2Type=0;
						TRACE((TRACE_DEBUG,_F_, "id2Type=%d",gptActions[ptiActions[i]].id2Type));
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id3Name")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szId3Name,sizeof(gptActions[ptiActions[i]].szId3Name),pChildElement);
					TRACE((TRACE_DEBUG,_F_, "id3Name=%s",gptActions[ptiActions[i]].szId3Name));
				}
				if (rc>0) { gbDontAskId3=FALSE; }
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id3Type")) 
			{
				char tmp[5+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc!=0)
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						if (strcmp(tmp,"EDIT")==0) gptActions[ptiActions[i]].id3Type=EDIT;
						else if (strcmp(tmp,"COMBO")==0) gptActions[ptiActions[i]].id3Type=COMBO;
						else gptActions[ptiActions[i]].id3Type=0;
						TRACE((TRACE_DEBUG,_F_, "id3Type=%d",gptActions[ptiActions[i]].id3Type));
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id4Name")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szId4Name,sizeof(gptActions[ptiActions[i]].szId4Name),pChildElement);
					TRACE((TRACE_DEBUG,_F_, "id4Name=%s",gptActions[ptiActions[i]].szId4Name));
				}
				if (rc>0) { gbDontAskId4=FALSE; }
				
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id4Type")) 
			{
				char tmp[5+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc!=0)
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						if (strcmp(tmp,"EDIT")==0) gptActions[ptiActions[i]].id4Type=EDIT;
						else if (strcmp(tmp,"COMBO")==0) gptActions[ptiActions[i]].id4Type=COMBO;
						else if (strcmp(tmp,"CHECK")==0) gptActions[ptiActions[i]].id4Type=CHECK_LABEL;
						else gptActions[ptiActions[i]].id4Type=0;
						TRACE((TRACE_DEBUG,_F_, "id4Type=%d",gptActions[ptiActions[i]].id4Type));
						if (gptActions[ptiActions[i]].id4Type==CHECK_LABEL) gbDontAskId4=TRUE;
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"validateName")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szValidateName,sizeof(gptActions[ptiActions[i]].szValidateName),pChildElement);
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"szName")) 
			{
				// si on remplace une configuration existante et qu'il n'y en a qu'une, on utilise le nom fourni par le serveur
				// sinon, comme c'est l'utilisateur lui-m�me qui a dupliqu� les configs, on ne casse pas son nommage.
				if (iReplaceExistingConfig==1)
				{
					char szTmpApplication[LEN_APPLICATION_NAME+1];
					// ISSUE#150 : si on est dans le cas d'un ajout au d�marrage (bGetAll=TRUE) et que le nom est vide, il faut mettre le titre � la place
					//             si on est dans le cas d'un ajout manuel, c'est trait� plus loin car on ne sait pas ce que va faire l'utilisateur et dans
					//             certains cas il ne faut pas prendre en compte le nom fourni par le serveur
					rc=StoreNodeValue(szTmpApplication,sizeof(szTmpApplication),pChildElement);
					if (bGetAll)
					{
						if (*szTmpApplication==0)
						{
							strncpy_s(szTmpApplication,sizeof(szTmpApplication),gptActions[ptiActions[0]].szTitle,LEN_APPLICATION_NAME-5);
							int len=strlen(szTmpApplication);
							if (len>0) { if (szTmpApplication[len-1]=='*') szTmpApplication[len-1]=0; }
						}
					}
					strcpy_s(gptActions[ptiActions[0]].szApplication,sizeof(gptActions[ptiActions[0]].szApplication),szTmpApplication);
					/* En fait � ce stade on ne sait pas si l'utilisateur va ajouter ou remplacer ! Donc � faire plus tard.
					if (rc>0) 
					{
						GenerateApplicationName(ptiActions[0],gptActions[ptiActions[0]].szApplication);
					}*/
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"szKBSim")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szKBSim,sizeof(gptActions[ptiActions[i]].szKBSim),pChildElement);
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"bKBSim")) 
			{
				char tmp[1+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement); 
				if (rc!=0) 
				{
					for (i=0;i<iReplaceExistingConfig;i++) gptActions[ptiActions[i]].bKBSim=(tmp[0]=='1');
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"szFullPathName")) 
			{
				for (i=0;i<iReplaceExistingConfig;i++)
				{
					rc=StoreNodeValue(gptActions[ptiActions[i]].szFullPathName,sizeof(gptActions[ptiActions[i]].szFullPathName),pChildElement);
				}
			}
			else if (gbCategoryManagement && CompareBSTRtoSZ(bstrNodeName,"categId")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						gptActions[ptiActions[i]].iCategoryId=atoi(tmp);
						TRACE((TRACE_DEBUG,_F_,"gptActions[%d].iCategoryId=%d",ptiActions[i],gptActions[ptiActions[i]].iCategoryId));
					}
				}
			}
			else if (gbCategoryManagement && CompareBSTRtoSZ(bstrNodeName,"categLabel")) 
			{
				char tmpLabel[LEN_CATEGORY_LABEL+1];
				rc=StoreNodeValue(tmpLabel,sizeof(tmpLabel),pChildElement);
				if (rc>0) 
				{
					int iCategIndex;
					iCategIndex=GetCategoryIndex(gptActions[ptiActions[0]].iCategoryId);
					if (iCategIndex==-1) // la cat�gorie n'existe pas, il faut la cr�er
					{
						ZeroMemory(&gptCategories[giNbCategories],sizeof(T_CATEGORY));
						gptCategories[giNbCategories].id=gptActions[ptiActions[0]].iCategoryId;
						GenerateCategoryName(giNbCategories,tmpLabel);
						gptCategories[giNbCategories].bExpanded=TRUE;
						giNbCategories++;
						//  ISSUE#59 : if (giNextCategId==gptActions[ptiActions[0]].iCategoryId) giNextCategId++;
						TRACE((TRACE_DEBUG,_F_,"Cr�ation    : gptCategories[%d].szLabel=%s",giNbCategories,gptCategories[giNbCategories].szLabel));				
					}
					else // la cat�gorie existe, mise � jour de son libell�
					{
						strcpy_s(gptCategories[iCategIndex].szLabel,sizeof(gptCategories[iCategIndex].szLabel),tmpLabel);
						TRACE((TRACE_DEBUG,_F_,"Mise � jour : gptCategories[%d].szLabel=%s",iCategIndex,gptCategories[iCategIndex].szLabel));
					}
					// sauvegarde des cat�gories
					SaveCategories();
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"withIdPwd")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					iCurWithIdPwd=atoi(tmp);
					TRACE((TRACE_DEBUG,_F_,"iCurWithIdPwd=%d",iCurWithIdPwd));
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id1Value")) // nouveau v1.03
			{
				if (iCurWithIdPwd & CONFIG_WITH_ID1) // identifiant impos� par l'admin
				{
					rc=StoreNodeValue(gptActions[ptiActions[0]].szId1Value,sizeof(gptActions[ptiActions[0]].szId1Value),pChildElement);
				}
				else
				{
					if (gptActions[ptiActions[0]].iWithIdPwd & CONFIG_WITH_ID1) // la config avait un ID impos� mais maintenant l'admin l'a enlev� , il faut le supprimer
						*gptActions[ptiActions[0]].szId1Value=0;
					else
						rc=StoreNodeValueIfNotEmpty(gptActions[ptiActions[0]].szId1Value,sizeof(gptActions[ptiActions[0]].szId1Value),pChildElement); // ISSUE#212
				}
				TRACE((TRACE_DEBUG,_F_, "id1Value=%s",gptActions[ptiActions[0]].szId1Value));
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id2Value")) // nouveau v1.03
			{
				if (iCurWithIdPwd & CONFIG_WITH_ID2) // identifiant impos� par l'admin
				{
					rc=StoreNodeValue(gptActions[ptiActions[0]].szId2Value,sizeof(gptActions[ptiActions[0]].szId2Value),pChildElement);
				}
				else
				{
					if (gptActions[ptiActions[0]].iWithIdPwd & CONFIG_WITH_ID2) // la config avait un ID2 impos� mais maintenant l'admin l'a enlev� , il faut le supprimer
						*gptActions[ptiActions[0]].szId2Value=0;
					else
						rc=StoreNodeValueIfNotEmpty(gptActions[ptiActions[0]].szId2Value,sizeof(gptActions[ptiActions[0]].szId2Value),pChildElement); // ISSUE#212
				}
				TRACE((TRACE_DEBUG,_F_, "id2Value=%s",gptActions[ptiActions[0]].szId2Value));
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id3Value")) // nouveau v1.03
			{
				if (iCurWithIdPwd & CONFIG_WITH_ID3) // identifiant impos� par l'admin
				{
					rc=StoreNodeValue(gptActions[ptiActions[0]].szId3Value,sizeof(gptActions[ptiActions[0]].szId3Value),pChildElement);
				}
				else
				{
					if (gptActions[ptiActions[0]].iWithIdPwd & CONFIG_WITH_ID3) // la config avait un ID3 impos� mais maintenant l'admin l'a enlev� , il faut le supprimer
						*gptActions[ptiActions[0]].szId3Value=0;
					else
						rc=StoreNodeValueIfNotEmpty(gptActions[ptiActions[0]].szId3Value,sizeof(gptActions[ptiActions[0]].szId3Value),pChildElement); // ISSUE#212
				}
				TRACE((TRACE_DEBUG,_F_, "id3Value=%s",gptActions[ptiActions[0]].szId3Value));
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"id4Value")) // nouveau v1.03
			{
				if (iCurWithIdPwd & CONFIG_WITH_ID4) // identifiant impos� par l'admin
				{
					rc=StoreNodeValue(gptActions[ptiActions[0]].szId4Value,sizeof(gptActions[ptiActions[0]].szId4Value),pChildElement);
				}
				else
				{
					if (gptActions[ptiActions[0]].iWithIdPwd & CONFIG_WITH_ID4) // la config avait un ID4 impos� mais maintenant l'admin l'a enlev� , il faut le supprimer
						*gptActions[ptiActions[0]].szId4Value=0;
					else
						rc=StoreNodeValueIfNotEmpty(gptActions[ptiActions[0]].szId4Value,sizeof(gptActions[ptiActions[0]].szId4Value),pChildElement); // ISSUE#212
				}
				TRACE((TRACE_DEBUG,_F_, "id4Value=%s",gptActions[ptiActions[0]].szId4Value));
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"pwdValue")) // nouveau v1.03
			{
				char tmpPwd[LEN_PWD+1]="";
				rc=StoreNodeValue(tmpPwd,sizeof(tmpPwd),pChildElement);
				//TRACE((TRACE_PWD,_F_, "pwdValue=%s",tmpPwd));
				if ((iCurWithIdPwd & CONFIG_WITH_PWD) || rc>0)
				{
					char *pszEncryptedPassword=NULL;
					// chiffre le mot de passe
					pszEncryptedPassword=swCryptEncryptString(tmpPwd,ghKey1);
					SecureZeroMemory(tmpPwd,LEN_PWD+1);
					if (pszEncryptedPassword!=NULL) 
					{
						strcpy_s(gptActions[ptiActions[0]].szPwdEncryptedValue,sizeof(gptActions[ptiActions[0]].szPwdEncryptedValue),pszEncryptedPassword);
					}
				}
				else if (gptActions[ptiActions[0]].iWithIdPwd & CONFIG_WITH_PWD) // la config avait un PWD impos� mais maintenant l'admin l'a enlev� , il faut le supprimer
				{
					*gptActions[ptiActions[0]].szPwdEncryptedValue=0;
				}
				// On a fini de traiter tous les id et mdp impos�s, on peut stocker le withIdPwd
				gptActions[ptiActions[0]].iWithIdPwd=iCurWithIdPwd;
				TRACE((TRACE_DEBUG,_F_,"gptActions[%d].iWithIdPwd=%d",ptiActions[0],gptActions[ptiActions[0]].iWithIdPwd));
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"pwdGroup")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						gptActions[ptiActions[i]].iPwdGroup=atoi(tmp);
						TRACE((TRACE_DEBUG,_F_,"gptActions[%d].iPwdGroup=%d",ptiActions[i],gptActions[ptiActions[i]].iPwdGroup));
					}
				}
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"autoLock")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						gptActions[ptiActions[i]].bAutoLock=atoi(tmp);
						TRACE((TRACE_DEBUG,_F_,"gptActions[%d].bAutoLock=%d",ptiActions[i],gptActions[ptiActions[i]].bAutoLock));
					}
				}
			}
			/*else if (CompareBSTRtoSZ(bstrNodeName,"autoPublish")) 
			{
				char tmp[10+1];
				rc=StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				if (rc>0) 
				{
					for (i=0;i<iReplaceExistingConfig;i++)
					{
						gptActions[ptiActions[i]].bAutoPublish=atoi(tmp);
						TRACE((TRACE_DEBUG,_F_,"gptActions[%d].bAutoLock=%d",ptiActions[i],gptActions[ptiActions[i]].bAutoPublish));
					}
				}
			}*/
			TRACE((TRACE_DEBUG,_F_,"/<%S>",bstrNodeName));
			if (bstrNodeName!=NULL) { SysFreeString(bstrNodeName); bstrNodeName=NULL; }
			if (rc==-1) goto end; // erreur
			// rechercher ses fr�res et soeurs
			pChildElement->get_nextSibling(&pNextChildElement);
			pChildElement->Release();
			pChildElement=pNextChildElement;
		} // while(pChild!=NULL)
nextChildApp:
		// rechercher ses fr�res et soeurs
		if (ptiActions!=NULL) { free(ptiActions); ptiActions=NULL; }
		pChildApp->get_nextSibling(&pNextChildApp);
		pChildApp->Release();
		pChildApp=pNextChildApp;
		TRACE((TRACE_DEBUG,_F_,"</app>"));
	} // while(pNode!=NULL)
	// 0.91:ce test ne sert plus � rien...
	// if (iNbChampsTrouves<4) { TRACE((TRACE_ERROR,_F_,"Nombre de champs trouves : %d",iNbChampsTrouves)); goto end; }

	// aucun fils, sans doute <app>APP NOT FOUND</app>
	// if (iAction==-1 ) goto end;
	// ne marche plus du fait de correction ISSUE#24
	if (!bGetAll && iAction==-1) goto end;

	if (!bGetAll && gptActions[iAction].bKBSim) // c'est une simulation de frappe
	{
		// les �ventuels champs trouv�s ne comptent pas, ils faut analyser
		// le champ szKBSim pour savoir ce qu'il faut demander � l'utilisateur
		gbDontAskId=TRUE;
		gbDontAskId2=TRUE;
		gbDontAskId3=TRUE;
		gbDontAskId4=TRUE;
		gbDontAskPwd=TRUE;
		if (strstr(gptActions[iAction].szKBSim,"[ID]")!=NULL) gbDontAskId=FALSE;
		if (strstr(gptActions[iAction].szKBSim,"[ID2]")!=NULL) 
		{
			gbDontAskId2=FALSE;
			if (*gptActions[iAction].szId2Name==0)
				strcpy_s(gptActions[iAction].szId2Name,LEN_ID+1,GetString(IDS_ID2));
		}
		if (strstr(gptActions[iAction].szKBSim,"[ID3]")!=NULL) 
		{
			gbDontAskId3=FALSE;
			if (*gptActions[iAction].szId3Name==0)
				strcpy_s(gptActions[iAction].szId3Name,LEN_ID+1,GetString(IDS_ID3));
		}
		if (strstr(gptActions[iAction].szKBSim,"[ID4]")!=NULL) 
		{
			gbDontAskId4=FALSE;
			if (*gptActions[iAction].szId4Name==0)
				strcpy_s(gptActions[iAction].szId4Name,LEN_ID+1,GetString(IDS_ID4));
		}
		if (strstr(gptActions[iAction].szKBSim,"[PWD]")!=NULL) gbDontAskPwd=FALSE;
	}
	rc=0;
#ifdef TRACES_ACTIVEES
	if (bGetAll)
	{
		TRACE((TRACE_DEBUG,_F_,"gtConfigSync.iNbConfigsAdded   =%d",gtConfigSync.iNbConfigsAdded));
		TRACE((TRACE_DEBUG,_F_,"gtConfigSync.iNbConfigsModified=%d",gtConfigSync.iNbConfigsModified));
		TRACE((TRACE_DEBUG,_F_,"gtConfigSync.iNbConfigsDisabled=%d",gtConfigSync.iNbConfigsDisabled));
	}
#endif
end:
	if (ptiActions!=NULL) { free(ptiActions); ptiActions=NULL; }
	if (pszEncodedURL!=NULL) free(pszEncodedURL);
	if (pszDecodedURL!=NULL) free(pszDecodedURL);
	if (bstrNodeName!=NULL) SysFreeString(bstrNodeName);
	if (pDoc!=NULL) pDoc->Release();
	if (pNode!=NULL)  pNode->Release();
	if (pRoot!=NULL)  pRoot->Release();
	if (pChildApp!=NULL) pChildApp->Release();
	if (pChildElement!=NULL) pChildElement->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetAllConfigsFromServer()
// ----------------------------------------------------------------------------------
// R�cup�re la liste compl�te des configurations disponibles sur le serveur
// Cette fonction est appel�e au premier lancement si l'utilisateur demande � 
// rappatrier toutes les configurations (cette question n'est pos�e que si la cl�
// de registre gbGetAllConfigsAtFirstStart est � 1 -> ce n'est pas le cas par d�faut).
// ATTENTION : le but de cette fonction est uniquement d'initialiser
// un nouveau compte, MAIS PAS de mettre � jour des configurations d�j� existantes
// localement � partir du serveur 
// ----------------------------------------------------------------------------------
int GetAllConfigsFromServer(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
						
	int rc=-1;
	BSTR bstrXML=NULL;

	// r�cup�re toutes les configurations sur le serveur
	bstrXML=LookForConfig("","","","",TRUE,FALSE,FALSE,UNK,FALSE);
	if (bstrXML==NULL) // la requete n'a pas abouti
	{
		// 0.92 / ISSUE#26 : n'affiche pas les messages d'erreur si gbDisplayConfigsNotifications=FALSE
		if (gbDisplayConfigsNotifications) MessageBox(NULL,gszErrorServerNotAvailable,"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	if (CompareBSTRtoSZ(bstrXML,"<app>NOT FOUND</app>")) // rien trouv�, pas grave la base est vide, il faut bien commencer un jour ;-)
	{
		// 0.92 / ISSUE#26 : n'affiche pas les messages d'erreur si gbDisplayConfigsNotifications=FALSE
		// if (gbDisplayConfigsNotifications) MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	// OK, on a le r�sultat de la requ�te HTTP : parsing du XML et remplissage des configs
	rc=AddApplicationFromXML(NULL,bstrXML,TRUE);
	if (rc==-1)
	{
		// 0.92 / ISSUE#26 : n'affiche pas les messages d'erreur si gbDisplayConfigsNotifications=FALSE
		if (gbDisplayConfigsNotifications) MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	else if (rc==-2) // ISSUE#149
	{
		MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP);
		goto end;
	}
	SaveApplications();
	SavePortal();
	SaveLastConfigUpdate();
	rc=0;
end:
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetAutoPublishConfigsFromServer()
// ----------------------------------------------------------------------------------
// R�cup�re les configurations marqu�es autoPublish et correspondant au domaine
// de rattachement de l'utilisateur, � condition qu'elle n'existe pas d�j� c�t� client.
// Si elle existe d�j� c�t� client, elle sera mise � jour comme pr�c�demment.
// ----------------------------------------------------------------------------------
int GetAutoPublishConfigsFromServer(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
						
	int rc=-1;
	BSTR bstrXML=NULL;
	char *pszIds=NULL;
	int sizeofIdsList;
	int i;
	int pos;

	if (giNbActions==0) // cas du premier lancement
	{
		bstrXML=LookForConfig("","","","",FALSE,FALSE,FALSE,UNK,TRUE);
	}
	else // il y a d�j� des configurations, on fournit la liste au serveur pour ne r�cup�rer que celles qu'on n'a pas d�j�
	{
		// construction de la liste des configurations connues localement
		sizeofIdsList=6*giNbActions; // on prend un peu de marge avec 5 octets par Id + 1 pour s�parateur 
		pszIds=(char*)malloc(sizeofIdsList);
		if (pszIds==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeofIdsList)); goto end; }
		pos=0;
		for (i=0;i<giNbActions;i++)
		{
			pos+=wsprintf(pszIds+pos,"%d,",gptActions[i].iConfigId);
		}
		*(pszIds+pos-1)=0; // supprime la virgule
		bstrXML=LookForConfig("","",pszIds,"",FALSE,FALSE,FALSE,UNK,TRUE);
	}

	if (bstrXML==NULL) // la requete n'a pas abouti
	{
		// 1.14FIX1 : on n'affiche pas de message d'erreur, sinon risque de doublon
		// if (gbDisplayConfigsNotifications) MessageBox(NULL,gszErrorServerNotAvailable,"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	if (CompareBSTRtoSZ(bstrXML,"<app>NOT FOUND</app>")) // rien trouv�
	{
		goto end;
	}
	// OK, on a le r�sultat de la requ�te HTTP : parsing du XML et remplissage des configs
	rc=AddApplicationFromXML(NULL,bstrXML,TRUE);
	if (rc==-1)
	{
		if (gbDisplayConfigsNotifications) MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	else if (rc==-2) // ISSUE#149
	{
		MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP);
		goto end;
	}
	SaveApplications();
	SavePortal();
	SaveLastConfigUpdate();
	rc=0;
end:
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetNewOrModifiedConfigsFromServer()
// ----------------------------------------------------------------------------------
// R�cup�re la liste des configurations ajout�es et/ou modifi�es depuis la derni�re
// v�rification, ainsi que les configurations archiv�es pour d�sactivation
// ----------------------------------------------------------------------------------
int GetNewOrModifiedConfigsFromServer(BOOL bForced)
{
	TRACE((TRACE_ENTER,_F_, ""));
						
	int rc=-1;
	BSTR bstrXML=NULL;
	char *pszIds=NULL;
	int sizeofIdsList;
	int i;
	int pos;

	// r�cup�re toutes les configurations sur le serveur
	if (gbGetNewConfigsAtStart && (gbGetModifiedConfigsAtStart | gbDisableArchivedConfigsAtStart))
	{
		// inutile de construire la liste des configurations connues localement,
		// il faut de toute fa�on r�cup�rer toutes les configs ajout�es et (modifi�es ou archiv�es)
		// depuis la derni�re v�rification puis traiter au cas par cas les configs
		sizeofIdsList=0;
		bstrXML=LookForConfig("","","",bForced?"20000101000000":gszLastConfigUpdate,
							gbGetNewConfigsAtStart,gbGetModifiedConfigsAtStart,gbDisableArchivedConfigsAtStart,UNK,FALSE);
	}
	else
	{
		if (giNbActions==0)
		{
			// ISSUE #69 : 0.94B4
			// si aucune configuration connue localement ET gbGetNewConfigsAtStart=0, �a veut dire qu'on ne veut
			// pas r�cup�rer les configurations existantes sur le serveur. Dans ce cas le mieux est de ne pas faire 
			// la requ�te ! (d'autant que le serveur retourne les configs existantes !)
			if (!gbGetNewConfigsAtStart) { rc=0; goto end; }
			bstrXML=LookForConfig("","","",bForced?"20000101000000":gszLastConfigUpdate,
								gbGetNewConfigsAtStart,gbGetModifiedConfigsAtStart,gbDisableArchivedConfigsAtStart,UNK,FALSE);
		}
		else
		{
			// construction de la liste des configurations connues localement
			sizeofIdsList=6*giNbActions; // on prend un peu de marge avec 5 octets par Id + 1 pour s�parateur 
			pszIds=(char*)malloc(sizeofIdsList);
			if (pszIds==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeofIdsList)); goto end; }
			pos=0;
			for (i=0;i<giNbActions;i++)
			{
				pos+=wsprintf(pszIds+pos,"%d,",gptActions[i].iConfigId);
			}
			*(pszIds+pos-1)=0; // supprime la virgule
			bstrXML=LookForConfig("","",pszIds,bForced?"20000101000000":gszLastConfigUpdate,
								gbGetNewConfigsAtStart,gbGetModifiedConfigsAtStart,gbDisableArchivedConfigsAtStart,UNK,FALSE);
		}
	}
	if (bstrXML==NULL) // la requete n'a pas abouti, pb serveur
	{
		// 0.92 / ISSUE#26 : n'affiche pas la demande si gbDisplayConfigsNotifications=FALSE
		if (gbDisplayConfigsNotifications) MessageBox(NULL,gszErrorServerNotAvailable,"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	if (CompareBSTRtoSZ(bstrXML,"<app>NOT FOUND</app>")) // rien trouv�, rien � faire !
	{
		rc=0;
		goto end;
	}
	// OK, on a le r�sultat de la requ�te HTTP : parsing du XML et remplissage des configs
	rc=AddApplicationFromXML(NULL,bstrXML,TRUE);
	if (rc==-1)
	{
		// 0.92 / ISSUE#26 : n'affiche pas la demande si gbDisplayConfigsNotifications=FALSE
		// 1.07 : les erreurs sont affich�es + tard
		// if (gbDisplayConfigsNotifications) MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
		goto end;
	}
	else if (rc==-2) // ISSUE#149
	{
		MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP);
		goto end;
	}
	SaveApplications();
	SavePortal();

	rc=0;
end:
	if (rc==0) // mise � jour OK, on note la date courante
	{
		SaveLastConfigUpdate();
	}
	if (pszIds!=NULL) free(pszIds);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DeleteConfigsNotOnServer()
// ----------------------------------------------------------------------------------
// R�cup�re la liste des configurations autoris�e pour l'utilisateur sur le serveur
// Supprime dans le .ini toutes celles qui ne sont pas sur le serveur
// ----------------------------------------------------------------------------------
int DeleteConfigsNotOnServer(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szParams[512+1];
	char *pszResult=NULL;
	char *p;
	int *pitabConfigsOnServer=NULL;
	int iNbConfigsOnServer;
	int iConfigId;
	int i,j,k;
	char *strtokContext=NULL;
	DWORD dwStatusCode;
	
	// appel webservice
	sprintf_s(szParams,sizeof(szParams),"?action=getdomainconfigs&domainId=%d",giDomainId);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	if (pszResult[0]=='N' && pszResult[1]=='O' && pszResult[2]=='N' && pszResult[3]=='E') 
	{
		// l'utilisateur n'a plus le droit � rien... c'est louche...
		// ISSUE#232 : c'est peut-�tre louche, mais il faut supprimer les configs du coup...
		gtConfigSync.iNbConfigsDeleted=giNbActions;
		giNbActions=0;
		giNbCategories=0;
	}
	else // r�ponse au format id,id,id,
	{
		pitabConfigsOnServer=(int*)malloc(giMaxConfigs*sizeof(int));
		if (pitabConfigsOnServer==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",giMaxConfigs)); goto end; }
		// construit une table d'id de configs pr�sentes sur le serveur
		iNbConfigsOnServer=0;
		p=strtok_s(pszResult,",",&strtokContext);
		while (p!=NULL)
		{
			iConfigId=atoi(p);
			if (iConfigId!=0) 
			{
				pitabConfigsOnServer[iNbConfigsOnServer]=iConfigId;
				iNbConfigsOnServer++;
			}
			p=strtok_s(NULL,",",&strtokContext);
		}
#if TRACES_ACTIVEES
		{
			for (i=0;i<iNbConfigsOnServer;i++)
			{
				TRACE((TRACE_DEBUG,_F_,"pitabConfigsOnServer[%d]=%d",i,pitabConfigsOnServer[i]));
			}
		}
#endif
		// parcourt la table des configs de l'utilisateur et supprime celles qui ne sont pas dans la liste du serveur
		i=0;
		while (i<giNbActions)
		{
			if (gptActions[i].iConfigId==0) goto config_suivante;
			for (j=0;j<iNbConfigsOnServer;j++)
			{
				if (gptActions[i].iConfigId==pitabConfigsOnServer[j]) 
				{
					TRACE((TRACE_DEBUG,_F_,"ConfigId %d trouve --> on conserve",gptActions[i].iConfigId));
					goto config_suivante;
				}
			}
			TRACE((TRACE_DEBUG,_F_,"ConfigId %d non trouv�e --> on supprime !",gptActions[i].iConfigId));
			for (k=i;k<giNbActions-1;k++)
			{
				TRACE((TRACE_DEBUG,_F_,"Copie %d sur %d",k+1,k));
				memcpy(&gptActions[k],&gptActions[k+1],sizeof(T_ACTION));
			}
			gtConfigSync.iNbConfigsDeleted++;
			giNbActions--;
			i--;
config_suivante:
			i++;
		}
	}
	if (gtConfigSync.iNbConfigsDeleted!=0) // il y a eu au moins une suppression
	{
		SaveApplications();
		SavePortal();
		SaveLastConfigUpdate();
	}
	rc=0;

end:
	if (pitabConfigsOnServer!=NULL) free(pitabConfigsOnServer);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DeleteConfigOnServer()
// ----------------------------------------------------------------------------------
// Supprime la configuration pass�e en param�tre sur le serveur
// ----------------------------------------------------------------------------------
int DeleteConfigOnServer(int iAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));
	int rc=-1;
	char szParams[512+1];
	char *pszResult=NULL;
	DWORD dwStatusCode;

	if (iAction==-1) goto end;
	// appel webservice
	sprintf_s(szParams,sizeof(szParams),"?action=deleteconfig&configId=%d",gptActions[iAction].iConfigId);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,gwcszAdminCookie,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	if (pszResult[0]=='O' && pszResult[1]=='K') rc=0;

end:		
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DeleteCategOnServer()
// ----------------------------------------------------------------------------------
// Supprime la cat�gorie pass�e en param�tre sur le serveur
// ----------------------------------------------------------------------------------
int DeleteCategOnServer(int iCategory)
{
	TRACE((TRACE_ENTER,_F_, "iCategory=%d",iCategory));
	int rc=-1;
	char szParams[512+1];
	char *pszResult=NULL;
	DWORD dwStatusCode;

	if (iCategory==-1) goto end;
	// appel webservice
	sprintf_s(szParams,sizeof(szParams),"?action=deletecateg&categId=%d",gptCategories[iCategory].id);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,gwcszAdminCookie,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	if (pszResult[0]=='O' && pszResult[1]=='K') rc=0;

end:		
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ReportConfigSync()
//-----------------------------------------------------------------------------
// Si iErrorMessage=0 : pas d'erreur, on affiche le d�tail de la synchro
// sinon affiche le message d'erreur fourni en apram�tre
//-----------------------------------------------------------------------------
void ReportConfigSync(int iErrorMessage,BOOL bShowMessage, BOOL bShowIfZero)
{
	TRACE((TRACE_ENTER,_F_, ""));

	if (iErrorMessage==0)
	{
		if (bShowIfZero || gtConfigSync.iNbConfigsAdded!=0 || gtConfigSync.iNbConfigsModified!=0 || gtConfigSync.iNbConfigsDisabled!=0 || gtConfigSync.iNbConfigsDeleted!=0)
		{
			char szNbConfigsAdded[5];
			char szNbConfigsModified[5];
			char szNbConfigsDisabled[5];
			char szNbConfigsDeleted[5];
			wsprintf(szNbConfigsAdded,"%d",gtConfigSync.iNbConfigsAdded);
			wsprintf(szNbConfigsModified,"%d",gtConfigSync.iNbConfigsModified);
			wsprintf(szNbConfigsDisabled,"%d",gtConfigSync.iNbConfigsDisabled);
			wsprintf(szNbConfigsDeleted,"%d",gtConfigSync.iNbConfigsDeleted);
			swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_CONFIG_UPDATE,szNbConfigsAdded,szNbConfigsModified,szNbConfigsDisabled,szNbConfigsDeleted,0);
			if (bShowMessage) 
			{
				// wsprintf(buf2048,GetString(IDS_GETCONFIGS_RESULT),gtConfigSync.iNbConfigsAdded,gtConfigSync.iNbConfigsModified,gtConfigSync.iNbConfigsDisabled,gtConfigSync.iNbConfigsDeleted);
				// MessageBox(NULL,buf2048,"swSSO",MB_ICONINFORMATION | MB_OK);
				NOTIFYICONDATA nid;
				ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
				nid.cbSize=sizeof(NOTIFYICONDATA);
				nid.hWnd=gwMain;
				nid.uID=0; 
				//nid.hIcon=;
				nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
				nid.uTimeout=2000;
				strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Mise � jour r�ussie");
				sprintf_s(nid.szInfo,sizeof(nid.szInfo),GetString(IDS_GETCONFIGS_RESULT),gtConfigSync.iNbConfigsAdded,gtConfigSync.iNbConfigsModified,gtConfigSync.iNbConfigsDisabled,gtConfigSync.iNbConfigsDeleted);
				Shell_NotifyIcon(NIM_MODIFY, &nid); 
			}
		}
	}
	else // message d'erreur
	{
		NOTIFYICONDATA nid;
		ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
		nid.cbSize=sizeof(NOTIFYICONDATA);
		nid.hWnd=gwMain;
		nid.uID=0; 
		//nid.hIcon=;
		nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
		nid.uTimeout=2000;
		strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Echec de la mise � jour");
		strcpy_s(nid.szInfo,sizeof(nid.szInfo),GetString(iErrorMessage));
		Shell_NotifyIcon(NIM_MODIFY, &nid); 
	}
	TRACE((TRACE_LEAVE,_F_, ""));
	return;
}
// ----------------------------------------------------------------------------------
// ReactivateApplicationFromCurrentWindow()
// ----------------------------------------------------------------------------------
// R�initialise les flags divers de l'application actuellement affich�e � l'�cran
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

	if (swGetTopWindow(&w,szTitle,sizeof(szTitle))!=0) { TRACE((TRACE_ERROR,_F_,"Top Window non trouvee !")); goto end; }

	GetClassName(w,szClassName,sizeof(szClassName));
    TRACE((TRACE_DEBUG,_F_,"szClassName          =%s",szClassName));
	TRACE((TRACE_DEBUG,_F_,"szTitle              =%s",szTitle));

	for (i=0;i<giNbActions;i++)
	{
		// ISSUE#215 : nouvelle gestion des popups pour Chrome 36+ (c'�tait merdique avant je ne conserve pas la compatibilit�)	
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
			// 0.92B3 : �volution de la reconnaissance des titres, prise en compte des * 
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
// G�n�re une proposition de configuration et l'affiche � l'utilisateur dans la 
// fen�tre de gestion des sites et applications
// ----------------------------------------------------------------------------------
void GenerateConfigAndOpenAppNsites(int iType, int iBrowser, char *pszTitle, char *pszURL)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszIEWindowTitle=NULL;
	char *p=NULL;
	
	ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));
	//gptActions[giNbActions].wLastDetect=NULL;
	//gptActions[giNbActions].tLastDetect=-1;
	gptActions[giNbActions].tLastSSO=-1;
	gptActions[giNbActions].wLastSSO=NULL;
	gptActions[giNbActions].iWaitFor=giWaitBeforeNewSSO;
	gptActions[giNbActions].bActive=TRUE; //0.93B6
	gptActions[giNbActions].bAutoLock=TRUE;
	//gptActions[giNbActions].bAutoPublish=FALSE;
	gptActions[giNbActions].bConfigSent=FALSE;
	gptActions[giNbActions].bSaved=FALSE; // 0.93B6 ISSUE#55
	// gptActions[giNbActions].iDomainId=1; //  1.00 ISSUE#112
	gptActions[giNbActions].iPwdGroup=-1;
	if (iType==UNK)
	{
		// le UNK �tait utile dans la requete WEB pour r�cup�rer configs WEB et XEB, maintenant il 
		// s'agit de proposer une config par d�faut la plus aboutie possible
		// ISSUE#162
		// if (iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4) iType=XEBSSO;
		if (iBrowser==BROWSER_CHROME || iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4) iType=XEBSSO;
	}
	gptActions[giNbActions].iType=iType;

	if (iBrowser==BROWSER_CHROME || iBrowser==BROWSER_IE || iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4 || iType==XINSSO)
	{
		strcpy_s(gptActions[giNbActions].szId1Name,sizeof(gptActions[giNbActions].szId1Name),"-1");
		strcpy_s(gptActions[giNbActions].szPwdName,sizeof(gptActions[giNbActions].szPwdName),"1");
		strcpy_s(gptActions[giNbActions].szValidateName,sizeof(gptActions[giNbActions].szValidateName),"[ENTER]");
	}
	// construction du titre
	if (iType==WEBSSO || iType==XEBSSO) // tronque la fin du titre qui contient le nom du navigateur
	{
		// firefox
		p=strstr(pszTitle," - Mozilla Firefox");
		if (p!=NULL) *p=0;
		// chrome
		p=strstr(pszTitle," - Google Chrome");
		if (p!=NULL) *p=0;
		// ie
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
		}
		// maxthon
		p=strstr(pszTitle," - Maxthon");
		if (p!=NULL) *p=0;
	}
	strncpy_s(gptActions[giNbActions].szTitle,sizeof(gptActions[giNbActions].szTitle),pszTitle,LEN_TITLE-1);
	// 0.92B4 : ajoute une * � la fin du titre sugg�r�
	// 0.92B6 : sauf si c'est une popup dans ce cas le titre est toujours complet ?
	if (iType!=POPSSO) strcat_s(gptActions[giNbActions].szTitle,sizeof(gptActions[giNbActions].szTitle),"*"); 
	// construction du application name
	strncpy_s(gptActions[giNbActions].szApplication,sizeof(gptActions[giNbActions].szApplication),pszTitle,LEN_APPLICATION_NAME-5);
	GenerateApplicationName(giNbActions,gptActions[giNbActions].szApplication);
	// construction URL
	if (pszURL!=NULL)
	{
		// ISSUE#305 : avec Chrome, si l'URL ne commence pas par http://, on l'ajoute
		if (iBrowser==BROWSER_CHROME && _strnicmp(pszURL,"http://",7)!=0 && _strnicmp(pszURL,"https://",8)!=0 && _strnicmp(pszURL,"file://",7)!=0)
		{
			strcpy_s(gptActions[giNbActions].szURL,sizeof(gptActions[giNbActions].szURL),"http://");
			strncat_s(gptActions[giNbActions].szURL,sizeof(gptActions[giNbActions].szURL),pszURL,LEN_URL-1);
		}
		else
		{
			strncpy_s(gptActions[giNbActions].szURL,sizeof(gptActions[giNbActions].szURL),pszURL,LEN_URL-1);
		}
	}

	TRACE((TRACE_DEBUG,_F_,"giNbActions=%d iType=%d szURL=%s",giNbActions,gptActions[giNbActions].iType,gptActions[giNbActions].szURL));
	// et hop, c'est fait (la sauvegarde sera faite ou pas par l'utilisateur dans la fen�tre appNsites)
	// si la fen�tre appnsites est ouverte, il ne faut pas faire le backup car il a �t� fait au moment de l'ouverture de la fen�tre 
	// et on veut que l'annulation annule tout ce qu'il a fait depuis. Sinon on fait le backup avant d'ouvrir la fen�tre.
	if (gwAppNsites==NULL) BackupAppsNcategs(); // il faut bien le faire avant le ++
	giNbActions++; 
	if (gwAppNsites!=NULL) 
	{
		EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),TRUE); // ISSUE#114
	}
	ShowAppNsites(giNbActions-1,FALSE);

	if (pszIEWindowTitle!=NULL) free(pszIEWindowTitle);
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// AddApplicationFromCurrentWindow()
// ----------------------------------------------------------------------------------
// Ajoute l'application actuellement affich�e sur le dessus
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
	int i;
	BOOL bServerAvailable=FALSE;
	int iBrowser=BROWSER_NONE;
	int iNbConfigWithThisId;
	int iOneOfReplacedConfigs=-1;

	if (swGetTopWindow(&w,szTitle,sizeof(szTitle))!=0) { TRACE((TRACE_ERROR,_F_,"Top Window non trouvee !")); goto end; }

	// R�cup�rer l'URL pour IE, Firefox et popup Firefox (ne pas faire pour fen�tres Windows et popup IE)
	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"szClassName=%s",szClassName));
	if (strcmp(szClassName,gcszMozillaDialogClassName)==0) // POPUP FIREFOX
	{
		pszURL=GetFirefoxPopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup firefox non trouvee")); goto end; }
		iType=POPSSO;
	}
	else if (strcmp(szClassName,"IEFrame")==0 || // IE 
			 strcmp(szClassName,"rctrl_renwnd32")==0 || // Outlook 97 � 2003 (au moins, � v�rifier pour 2007)
			 strcmp(szClassName,"OpusApp")==0 || // Word 97 � 2003 (au moins, � v�rifier pour 2007)
			 strcmp(szClassName,"ExploreWClass")==0 || 
			 strcmp(szClassName,"CabinetWClass")==0) // Explorateur Windows
			 // Ne surtout pas ajouter #32770 sinon on embarque les popup IE et c'est rat� !
	{
		pszURL=GetIEURL(w,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL IE non trouvee")); goto end; }
		iType=UNK; // permet de r�cup�rer les configs WEB ou XEB 
		iBrowser=BROWSER_IE;
	}
	else if (strcmp(szClassName,"#32770")==0 && 
			(strcmp(szTitle,"S�curit� de Windows")==0 || strcmp(szTitle,"Windows Security")==0)) // POPUP IE8 SUR W7 [ISSUE#5] (FR et US uniquement... pas beau)
	{
		pszURL=GetW7PopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL W7 popup non trouvee")); goto end; }
		iType=POPSSO;
	}
	else if (strcmp(szClassName,"#32770")==0 && 
		    (strncmp(szTitle,"Connexion �",strlen("Connexion �"))==0 
			 || strncmp(szTitle,"Connect to",strlen("Connect to"))==0)) // POPUP IE (ISSUE #8)
	{
		iType=POPSSO;
	}
	else if (strcmp(szClassName,gcszMozillaUIClassName)==0) // FIREFOX3
	{
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX3,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox3 non trouvee")); goto end; }
		iType=UNK; // permet de r�cup�rer les configs WEB ou XEB 
		iBrowser=BROWSER_FIREFOX3;
	}
	else if (strcmp(szClassName,gcszMozillaClassName)==0) // FIREFOX4
	{
		SetForegroundWindow(w); // n�cessaire pour ISSUE#371
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX4,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox4+ non trouvee")); goto end; }
		iType=UNK; // permet de r�cup�rer les configs WEB ou XEB 
		iBrowser=BROWSER_FIREFOX4;
	}
	else if (strcmp(szClassName,"Maxthon2_Frame")==0) // Maxthon
	{
		pszURL=GetMaxthonURL();
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Maxthon non trouvee")); goto end; }
		iType=WEBSSO; // pas UNK car XEBSSO pas encore support� pour Maxthon
		iBrowser=BROWSER_MAXTHON;
	}
	else if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0) // ISSUE#77 : Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	{
		// ISSUE#215 : nouvelle gestion des popups pour Chrome 36+ (c'�tait merdique avant je ne conserve pas la compatibilit�)	
		pszURL=GetChromePopupURL(w);
		if (pszURL==NULL) // pas de popup trouv�e, c'est du contenu chrome Web
		{
			iType=UNK; // permet de r�cup�rer les configs WEB ou XEB 
			pszURL=GetChromeURL(w);
			if (pszURL==NULL) pszURL=GetChromeURL51(w); // ISSUE#282
			if (pszURL==NULL) pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); // ISSUE#314
			if (gpAccessibleChromeURL!=NULL) { gpAccessibleChromeURL->Release(); gpAccessibleChromeURL=NULL; }
			// ISSUE#142 : si pszURL=NULL, mieux vaut s'arr�ter m�me si en fait �a ne crashe pas car bien g�r� partout
			// ISSUE#155
			// if (pszURL == NULL) { TRACE((TRACE_ERROR, _F_, "URL Chrome non trouvee")); goto suite; }
			if (pszURL == NULL) { TRACE((TRACE_ERROR, _F_, "URL Chrome non trouvee")); goto end; }
			iBrowser=BROWSER_CHROME;
		}
		else // trouv� une popup Chrome, lecture du titre de la popup et de l'URL du site
		{
			iType=POPSSO;
		}
	}
	// ISSUE#287 : prise en compte de EDGE avec Windows 10 anniversaire (1607)
	else if (strcmp(szClassName,"ApplicationFrameWindow")==0) // EDGE -- se comporte ensuite comme IE, pour les configs simplifi�es et les configs normales : MERCI MICROSOFT !! :-)
	{
		// ISSUE#287
		pszURL=GetIEURL(w,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL EDGE non trouvee")); goto end; }
		iType=UNK; // permet de r�cup�rer les configs WEB ou XEB 
		iBrowser=BROWSER_IE;
	}
	// ISSUE#297 : prise en compte des popups de s�curit� de Windows 10 anniversaire (dans IE, EDGE, etc.)
	else if (strcmp(szClassName,"Credential Dialog Xaml Host")==0 &&  
		    (strcmp(szTitle,"S�curit� Windows")==0 || strcmp(szTitle,"Windows Security")==0)) // POPUP W10 anniversaire... IE, EDGE, partages r�seau, etc.
	{
		pszURL=GetW10PopupURL(w);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W10 non trouvee")); goto end; }
		iType=POPSSO;
	}
	else iType=XINSSO;

	if (gbInternetGetConfig)
	{
		// recherche de la configuration sur le serveur
		bstrXML=LookForConfig(szTitle,pszURL==NULL?"":pszURL,"","",FALSE,FALSE,FALSE,iType,FALSE);
		if (bstrXML==NULL) // la requete n'a pas abouti
		{
			bServerAvailable=FALSE;
			MessageBox(NULL,gszErrorServerNotAvailable,"swSSO",MB_OK | MB_ICONEXCLAMATION);
			// on laisse continuer pour demander plus bas � l'utilisateur
			// s'il veut faire la config manuellement
		}
		else
		{
			// OK, on a le r�sultat de la requ�te HTTP
			bServerAvailable=TRUE;
			// Parsing du XML et remplissage de la config de l'appli
			rc=AddApplicationFromXML(w,bstrXML,FALSE);
			if (rc==0) 
				bConfigFound=TRUE;
		}
	}
	// le ShowAppNsites fait beaucoup plus tard va recharger la treeview, il faut faire un GetApplicationDeltails 
	// tout de suite sinon on perdra une �ventuelle config commenc�e par l'utilisateur
	// ISSUE#152 : tant pis, on ne fait plus, �a g�n�re trop de cas foireux car le giLastApplicationConfig ne varie
	//             pas seulement en fonction de la config s�lectionn�e dans la liste mais aussi en fonction des applis
	//             d�tect�es (pour justement pouvoir mettre le focus sur la config dans appnsites)
	// if (gwAppNsites!=NULL) GetApplicationDetails(gwAppNsites,giLastApplicationConfig);

	if (!bConfigFound) // config non trouv�e
	{
		if (!gbInternetGetConfig) // c'est normal qu'on ne trouve pas la config (interdit d'aller sur le serveur)
		{
			GenerateConfigAndOpenAppNsites(iType,iBrowser,szTitle,pszURL); // fait une proposition de config, l'utilisateur compl�tera
		}
		else
		{
			if (gbErrorServerConfigNotFoundDefaultMessage) // message par d�faut : demande � l'utilisateur s'il veut faire la config manuellement
			{
				if (MessageBox(NULL,gszErrorServerConfigNotFound,"swSSO",MB_YESNO | MB_ICONQUESTION)==IDYES)
				{
					GenerateConfigAndOpenAppNsites(iType,iBrowser,szTitle,pszURL); // fait une proposition de config, l'utilisateur compl�tera
				}
			}
			else // message personnalis�
			{
				char szSubTitle[256];
				T_MESSAGEBOX3B_PARAMS params;
				int reponse;
				params.szIcone=IDI_EXCLAMATION;
				if (bJustDisplayTheMessage)
				{
					params.iB1String=IDS_FERMER; // fermer
					params.iB2String=-1; // rien 
					params.iB3String=-1; // rien
					params.szMessage=gszAskThisAppMessage;
				}
				else
				{
					params.iB1String=IDS_AJOUTER; // ajouter
					params.iB2String=IDS_FERMER; // fermer
					params.iB3String=-1; // rien
					params.szMessage=gszErrorServerConfigNotFound;
				}
				params.wParent=w;
				params.iTitleString=IDS_MESSAGEBOX_TITLE;
				params.bVCenterSubTitle=TRUE;
				strcpy_s(szSubTitle,sizeof(szSubTitle),gszErrorServerTitleConfigNotFound);
				params.szSubTitle=szSubTitle;
				params.szMailTo=gszConfigNotFoundMailTo;
				gpszURLBeingAdded=pszURL;
				gpszTitleBeingAdded=szTitle;
				gpszURLBeingAdded=HTTPEncodeURL(pszURL);
				gpszTitleBeingAdded=HTTPEncodeURL(szTitle);
				reponse=MessageBox3B(&params);
				if (gpszURLBeingAdded!=NULL) { free (gpszURLBeingAdded); gpszURLBeingAdded=NULL; }
				if (gpszTitleBeingAdded!=NULL) { free (gpszTitleBeingAdded); gpszTitleBeingAdded=NULL; }
				if (!bJustDisplayTheMessage && (reponse==B1)) GenerateConfigAndOpenAppNsites(iType,iBrowser,szTitle,pszURL);
			}
		}
		goto end;
	}
	
	// 0.85 : v�rifie que cette configuration n'existe pas d�j�, sinon propose de la remplacer
	// ISSUE#126 : grosse modification apport�e : ne pas se limiter � une seule configuration, mais mettre toutes les configurations 
	//             portant cet ID � jour (s'il y en a plusieurs, c'est que l'utilisateur s'est ajout� des comptes sur une application,
	//             dans ce cas il ne vaut mieux pas mettre � jour le titre -- c'est la strat�gie d�j� appliqu�e lors d'une mise � jour 
	//             de configuration automatique depuis le serveur lorsque GetModifiedConfigsAtStart=1 par exemple)
	
	// On proc�de en 2 temps : on regarde combien de config locales avec cet id et on demande � l'utilisateur ce qu'il faut faire
	iNbConfigWithThisId=0;
	for (i=0;i<giNbActions;i++)
	{
		if (gptActions[giNbActions].iConfigId==gptActions[i].iConfigId) iNbConfigWithThisId++;
	}
	if (iNbConfigWithThisId!=0) // Au moins une configuration a cet id, on demande � l'utilisateur s'il faut remplacer ou en ajouter une nouvelle
	{
		T_MESSAGEBOX3B_PARAMS params;
		char szMsg[512];
		char szSubTitle[128];
		int reponse;
		params.szIcone=IDI_EXCLAMATION;
		params.iB1String=IDS_CONFIG_EXISTS_B1;
		params.iB2String=IDS_CONFIG_EXISTS_B2;
		params.iB3String=IDS_CONFIG_EXISTS_B3;
		params.wParent=HWND_DESKTOP;
		params.iTitleString=IDS_MESSAGEBOX_TITLE;
		params.bVCenterSubTitle=FALSE;
		strcpy_s(szSubTitle,sizeof(szSubTitle),GetString(IDS_CONFIG_EXISTS_SUBTITLE));
		params.szSubTitle=szSubTitle;
		strcpy_s(szMsg,sizeof(szMsg),GetString(IDS_CONFIG_EXISTS_MESSAGE));
		params.szMessage=szMsg;
		params.szMailTo=NULL;
		reponse=MessageBox3B(&params);
		if (reponse==B3) // IDCANCEL : conservation de l'ancienne config, pas d'ajout de la nouvelle
			goto end;
		else if (reponse==B1) // B1 = remplacer : remplacement de la ou des anciennes configs par la nouvelle
			bReplaceOldConfig=TRUE;
		else if (reponse==B2) // B2 = ajouter : d�sactivation de la ou des anciennes configs et ajout de la nouvelle
			bReplaceOldConfig=FALSE;
	}
	for (i=0;i<giNbActions;i++)
	{
		if (gptActions[giNbActions].iConfigId==gptActions[i].iConfigId)
		{
			if (bReplaceOldConfig) // remplacement de cette config par la nouvelle
			{
				iOneOfReplacedConfigs=i;
				// ISSUE#126 : on r�cup�re de l'ancienne config : idS + mot de passe + bActive + CategId si pas g�r� par le serveur (CategoryManagement=0)
				//             + nom (uniquement s'il y a plusieurs config qui matchent)
				if (!gbCategoryManagement) gptActions[giNbActions].iCategoryId=gptActions[i].iCategoryId;
				// ISSUE#150 : si le nom de config r�cup�r� sur le serveur est vide, on remet le nom de config qui existait en local
				// if (iNbConfigWithThisId!=1) strcpy_s(gptActions[giNbActions].szApplication,sizeof(gptActions[giNbActions].szApplication),gptActions[i].szApplication);
				if (iNbConfigWithThisId!=1 || *gptActions[giNbActions].szApplication==0) strcpy_s(gptActions[giNbActions].szApplication,sizeof(gptActions[giNbActions].szApplication),gptActions[i].szApplication);
				gptActions[giNbActions].bActive=gptActions[i].bActive;
				memcpy(gptActions[giNbActions].szId1Value,gptActions[i].szId1Value,sizeof(gptActions[giNbActions].szId1Value));
				memcpy(gptActions[giNbActions].szId2Value,gptActions[i].szId2Value,sizeof(gptActions[giNbActions].szId2Value));
				memcpy(gptActions[giNbActions].szId3Value,gptActions[i].szId3Value,sizeof(gptActions[giNbActions].szId3Value));
				memcpy(gptActions[giNbActions].szId4Value,gptActions[i].szId4Value,sizeof(gptActions[giNbActions].szId4Value));
				memcpy(gptActions[giNbActions].szPwdEncryptedValue,gptActions[i].szPwdEncryptedValue,sizeof(gptActions[giNbActions].szPwdEncryptedValue));
				// copie la nouvelle � l'ancienne position
				memcpy(&gptActions[i],&gptActions[giNbActions],sizeof(T_ACTION));
				bReplaceOldConfig=TRUE;
				gptActions[i].bSaved=FALSE; // 0.93B6 ISSUE#55
				gptActions[i].bConfigSent=TRUE; // on vient de la r�cup�rer sur le serveur, pas la peine de la remonter !
			}
			else // d�sactivation de cette config
			{
				GenerateApplicationName(i,gptActions[i].szApplication);
				gptActions[i].bActive=FALSE;
				gptActions[i].bSaved=FALSE; // 0.93B6 ISSUE#55
			}
		}
	}
//suite:
	if (!bReplaceOldConfig)
	{
		// Demande � l'utilisateur de saisir son id et son mdp pour cette application
		T_IDANDPWDDIALOG params;
		params.bCenter=FALSE;
		params.iAction=giNbActions;
		params.iTitle=IDS_IDANDPWDTITLE_NEW;
		// ISSUE#37 0.92B8 : d�plac� plus bas pour que le szApplication soit toujours bien renseign�.
		//wsprintf(params.szText,GetString(IDS_IDANDPWDTEXT_NEW),gptActions[giNbActions].szApplication);
		//if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_ID_AND_PWD),NULL,IdAndPwdDialogProc,(LPARAM)&params)!=IDOK) {rc=-2; goto end; }
		// si pas de nom trouv� sur le serveur, associe par d�faut le titre
		// ISSUE#37 0.92B8 en supprimant l'*
		if (*gptActions[giNbActions].szApplication==0)
		{
			strncpy_s(gptActions[giNbActions].szApplication,sizeof(gptActions[giNbActions].szApplication),gptActions[giNbActions].szTitle,LEN_APPLICATION_NAME-5);
			int len=strlen(gptActions[giNbActions].szApplication);
			if (len>0) { if (gptActions[giNbActions].szApplication[len-1]=='*') gptActions[giNbActions].szApplication[len-1]=0; }
		}
		wsprintf(params.szText,GetString(IDS_IDANDPWDTEXT_NEW),gptActions[giNbActions].szApplication);
		// ISSUE#334
		//if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_ID_AND_PWD),NULL,IdAndPwdDialogProc,(LPARAM)&params)!=IDOK) {rc=-2; goto end; }
		if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_ID_AND_PWD),w,IdAndPwdDialogProc,(LPARAM)&params)!=IDOK) 
		{
			gwIdAndPwdDialogProc=NULL;
			rc=-2; 
			goto end; 
		}
		gwIdAndPwdDialogProc=NULL;
		// rend le nom unique
		GenerateApplicationName(giNbActions,gptActions[giNbActions].szApplication);
		gptActions[giNbActions].bActive=TRUE;
		gptActions[giNbActions].bSaved=FALSE; // 0.93B6 ISSUE#55
		// gptActions[giNbActions].bConfigOK=FALSE; // 0.90B1 : on ne g�re plus l'�tat OK car plus de remont�e auto
		gptActions[giNbActions].bConfigSent=TRUE; // on vient de la r�cup�rer sur le serveur, pas la peine de la remonter !
		giNbActions++;
	}
	// Compl�ment ISSUE#117 : si fen�tre ouverte, ne fait pas la sauvegarde mais affiche la fen�tre
	if (gwAppNsites!=NULL)
	{
		EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),TRUE); // ISSUE#114
		ShowAppNsites(bReplaceOldConfig?iOneOfReplacedConfigs:giNbActions-1, FALSE);
	}
	else
	{
		SaveApplications();
		SavePortal();
	}

	rc=0;
end:
	if (pszURL!=NULL) free(pszURL);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// InternetCheckProxyParams
// ----------------------------------------------------------------------------------
// V�rification des param�tres proxy saisis par l'utilisateur
// On utilise une requ�te getversion.
// ----------------------------------------------------------------------------------
int InternetCheckProxyParams(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szParams[255+1];
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	T_PROXYPARAMS ProxyParams;
	DWORD dwStatusCode;

	hCursorOld=SetCursor(ghCursorWait);
	
	// r�cup�ration des param�tres proxy dans l'IHM
	ProxyParams.bInternetUseProxy=IsDlgButtonChecked(w,CK_USE_PROXY)==BST_CHECKED?TRUE:FALSE;
	GetDlgItemText(w,TB_PROXY_URL ,ProxyParams.szProxyURL ,LEN_PROXY_URL);
	GetDlgItemText(w,TB_PROXY_USER,ProxyParams.szProxyUser,LEN_PROXY_USER);
	GetDlgItemText(w,TB_PROXY_PWD ,ProxyParams.szProxyPwd ,LEN_PROXY_PWD);

	// effectue la requ�te getversion avec ces param�tres
	strcpy_s(szParams,sizeof(szParams),"?action=getversion");
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,5,&ProxyParams,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	rc=0;
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (rc==0)
		MessageBox(w,GetString(IDS_PROXY_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
	else 
		MessageBox(w,GetString(IDS_PROXY_NOK),"swSSO",MB_OK | MB_ICONEXCLAMATION);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// InternetCheckVersion
// ----------------------------------------------------------------------------------
// V�rification des mises � jour sur internet
// Format de la chaine retourn�e : 079-0801 (079-0000 si pas de beta en cours)
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
	
	if (strcmp(pReleaseVersion,"000")==0) goto end;// ISSUE#90 : dans ce cas on ne v�rifie plus la version.

	if (!gbInternetCheckBeta) // ne v�rifie pas la pr�sence de version beta
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
	else // v�rifie la pr�sence de beta
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

	// r�cup�re le mot de passe AD
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
			free(pszEncryptedPassword); // forc�ment pas NULL sinon on ne serait pas l�
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

