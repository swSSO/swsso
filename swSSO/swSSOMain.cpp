
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2025 - Sylvain WERDEFROY
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
// swSSOMain.cpp
//-----------------------------------------------------------------------------
// Point d'entr�e + boucle de recherche de fen�tre � SSOiser
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "ISimpleDOMNode_i.c"
#include "ISimpleDOMDocument_i.c"

// Un peu de globales...
const char gcszCurrentVersion[]="125";	// 101 = 1.01
const char gcszCurrentBeta[]="0000";	// 1021 = 1.02 beta 1, 0000 pour indiquer qu'il n'y a pas de beta

HWND gwMain=NULL;
HWND gwChooseConfig=NULL;
HINSTANCE ghInstance;
HRESULT   ghrCoIni=E_FAIL;	 // code retour CoInitialize()
bool gbSSOActif=TRUE;	 // Etat swSSO : actif / d�sactiv�	
int giPwdProtection=PP_UNDEFINED; // Protection des mots de passe : PP_ENCRYPTED | PP_WINDOWS -- ISSUE#296 : ajout d'une valeur par d�faut

T_ACTION *gptActions;  // tableau d'actions
int giNbActions;		// nb d'actions dans le tableau

int giBadPwdCount;		// nb de saisies erron�es de mdp cons�cutives
HWND gwAskPwd=NULL ;       // anti r�-entrance fen�tre saisie pwd

// 1.12 : on ne conserve plus l'objet cl� AES, mais seulement les donn�es utiles dans 4 buffers diff�rents
BOOL gAESKeyInitialized[2];
BYTE gAESProtectedKeyData[2][AES256_KEY_LEN];

// astuce pour limiter les modifs de code : ghKey1 et ghKey2 �taient les handle des 2 cl�s, ils deviennent les index pour le tableau des cl�s
const int ghKey1=0; 
const int ghKey2=1;

// Icones et pointeur souris
HICON ghIconAltTab=NULL;
HICON ghIconSystrayActive=NULL;
HICON ghIconSystrayInactive=NULL;
HICON ghIconLoupe=NULL;
HICON ghIconHelp = NULL;
HANDLE ghLogo=NULL;
HANDLE ghLogoFondBlanc50=NULL;
HANDLE ghLogoFondBlanc90=NULL;
HANDLE ghLogoExclamation=NULL;
HANDLE ghLogoQuestion=NULL;
HCURSOR ghCursorHand=NULL; 
HCURSOR ghCursorWait=NULL;
HIMAGELIST ghImageList=NULL;

HFONT ghBoldFont=NULL;

// Compteurs pour les stats
UINT guiNbWEBSSO;
UINT guiNbWINSSO;
UINT guiNbPOPSSO;
UINT guiNbWindows;
UINT guiNbVisibleWindows;

// 0.76
BOOL gbRememberOnThisComputer=FALSE;
BOOL gbRecoveryRunning=FALSE;

BOOL gbRegisterSessionNotification=FALSE;
UINT guiLaunchAppMsg=0;
UINT guiConnectAppMsg=0;
UINT guiStandardQuitMsg=0;
UINT guiAdminQuitMsg=0;

int giTimer=0;
static int giRegisterSessionNotificationTimer=0;
static int giNbRegisterSessionNotificationTries=0;
static int giRefreshTimer=10;
int giRefreshRightsTimer=0;

int giOSVersion=OS_WINDOWS_OTHER;
int giOSBits=OS_32;

SID *gpSid=NULL;
char *gpszRDN=NULL;
char gszComputerName[MAX_COMPUTERNAME_LENGTH+1]="";
char gszUserName[UNLEN+1]="";
char gszUPN[UNLEN+1]="";

char gcszK1[]="11111111";

// 0.91 : pour choix de config (fen�tre ChooseConfig)
typedef struct
{
	int iNbConfigs;
	int tabConfigs[500]; // ISSUE#149 : je laisse 500, c'est trop compliqu� de faire dynamique avec giMaxConfigs et surtout inutile
	int iConfig;
} T_CHOOSE_CONFIG;

T_LAST_DETECT gTabLastDetect[MAX_NB_LAST_DETECT]; // 0.93 liste des fen�tres d�tect�es sur cette action

HANDLE ghPwdChangeEvent=NULL; // 0.96

int giLastApplicationSSO=-1;		// derni�re application sur laquelle le SSO a �t� r�alis�
int giLastApplicationConfig=-1; // derni�re application utilis�e (soit SSO soit config)
SYSTEMTIME gLastLoginTime; // ISSUE#106

T_DOMAIN gtabDomains[100];
int giNbDomains=0;

BOOL gbWillTerminate=FALSE;
BOOL gbAdmin=FALSE;

HBRUSH ghRedBrush=NULL;

int giNbTranscryptError=0;

time_t gtLastAskPwd=0;

IUIAutomation *gpIUIAutomation=NULL;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

static int CALLBACK EnumWindowsProc(HWND w, LPARAM lp);

//-----------------------------------------------------------------------------
// TimerProc()
//-----------------------------------------------------------------------------
// L'appel � cette fonction est d�clench� toutes les 500 ms par le timer.
// C'est cette fonction qui lance l'�num�ration des fen�tres
//-----------------------------------------------------------------------------
static void CALLBACK TimerProc(HWND w,UINT msg,UINT idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(w);

	// TODO : � d�placer dans un autre timer pour le faire moins souvent ?
	DWORD dw=WaitForSingleObject(ghPwdChangeEvent,0);
	TRACE((TRACE_DEBUG,_F_,"WaitForSingleObject=0x%08lx",dw));
	if (dw==WAIT_OBJECT_0)
	{
		TRACE((TRACE_INFO,_F_,"WaitForSingleObject : swsso-pwdchange event received"));
		if (ChangeWindowsPwd()==0)
		{
			if (gbDisplayWindowsPasswordChange)
			{
				// En 1.08, n'affiche plus de MessageBox mais une info bulle
				// MessageBox(w,GetString(IDS_CHANGE_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
				NOTIFYICONDATA nid;
				ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
				nid.cbSize=sizeof(NOTIFYICONDATA);
				nid.hWnd=gwMain;
				nid.uID=0; 
				//nid.hIcon=;
				nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
				nid.uTimeout=2000;
				strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Changement de mot de passe r�ussi");
				strcpy_s(nid.szInfo,sizeof(nid.szInfo),GetString(IDS_CHANGE_PWD_OK));
				Shell_NotifyIcon(NIM_MODIFY, &nid); 
			}
		}
		else
		{
			MessageBox(w,GetString(IDS_CHANGE_PWD_FAILED),"swSSO",MB_OK | MB_ICONEXCLAMATION);
		}
	}

	// ISSUE#351
	// if (gbSSOActif) 
	if (!gbAdmin && gbSSOActif) 
	{
		guiNbWindows=0;
		guiNbVisibleWindows=0;
		// 0.93 : avant de commencer l'�num�ration, d�taggue toutes les fen�tres
		LastDetect_UntagAllWindows();
		// enum�ration des fen�tres
		EnumWindows(EnumWindowsProc,0);
		// 0.93 : apr�s l'�num�ration, efface toutes les fen�tres non taggu�s
		//        cela permet de supprimer de la liste des derniers SSO r�alis�s les fen�tres 
		//        qui ne sont plus � l'�cran
		LastDetect_RemoveUntaggedWindows();
		// 0.82 : r�armement du timer (si d�sarm�) une fois que l'�num�ration est termin�e 
		//        (plut�t que dans la WindowsProc -> au moins on est s�r que c'est toujours fait)
		LaunchTimer();
	}
}

//-----------------------------------------------------------------------------
// RefreshRightsTimerProc()
//-----------------------------------------------------------------------------

static void CALLBACK RefreshRightsTimerProc(HWND w,UINT msg,UINT idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(w);

	TRACE((TRACE_ENTER,_F_, ""));

	RefreshRights(FALSE,FALSE);
	
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// RegisterSessionNotificationTimerProc()
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void CALLBACK RegisterSessionNotificationTimerProc(HWND w,UINT msg,UINT idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(w);
	
	if (!gbRegisterSessionNotification) 
	{
		gbRegisterSessionNotification=WTSRegisterSessionNotification(gwMain,NOTIFY_FOR_THIS_SESSION);
		if (gbRegisterSessionNotification) // c'est bon, enfin !
		{
			TRACE((TRACE_ERROR,_F_,"WTSRegisterSessionNotification() -> OK, au bout de %d tentatives !",giNbRegisterSessionNotificationTries));
			KillTimer(NULL,giRegisterSessionNotificationTimer);
			giRegisterSessionNotificationTimer=0;
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"WTSRegisterSessionNotification()=%ld [REESSAI DANS 15 SECONDES]",GetLastError()));
			giNbRegisterSessionNotificationTries++;
			if (giNbRegisterSessionNotificationTries>20) // 20 fois 15 secondes = 5 minutes, on arr�te, tant pis !
			{
				TRACE((TRACE_ERROR,_F_,"WTSRegisterSessionNotification n'a pas reussi : PLUS DE REESSAI"));
				KillTimer(NULL,giRegisterSessionNotificationTimer);
				giRegisterSessionNotificationTimer=0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// LauchTimer()
//-----------------------------------------------------------------------------
// Lance le timer si pas d�j� lanc�
//-----------------------------------------------------------------------------
int LaunchTimer(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	if (giTimer==0) 
	{
		giTimer=SetTimer(NULL,0,giDetectionFrequency,TimerProc); // ISSUE#379 - timer configurable
		if (giTimer==0) 
		{
			DWORD err=GetLastError();
			TRACE((TRACE_ERROR,_F_,"SetTimer() : %ld (0x%08lx)",err,err));
			goto end;
		}
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// KBSimSSO()
//-----------------------------------------------------------------------------
// R�alisation du SSO de l'action pass�e en param�tre par simulation de frappe clavier
//-----------------------------------------------------------------------------
int KBSimSSO(HWND w, int iAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));
	int rc=-1;
	char szDecryptedPassword[LEN_PWD+1];

	// d�chiffrement du champ mot de passe
	if ((*gptActions[iAction].szPwdEncryptedValue!=0)) // TODO -> CODE A REVOIR PLUS TARD (PAS BEAU SUITE A ISSUE#83)
	{
		// char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
		char *pszPassword=GetDecryptedPwd(gptActions[iAction].szPwdEncryptedValue,TRUE);
		if (pszPassword!=NULL) 
		{
			strcpy_s(szDecryptedPassword,sizeof(szDecryptedPassword),pszPassword);
			SecureZeroMemory(pszPassword,strlen(pszPassword));
			free(pszPassword);
		}
	}
	else
	{
		strcpy_s(szDecryptedPassword,sizeof(szDecryptedPassword),gptActions[iAction].szPwdEncryptedValue);
	}

	// analyse et ex�cution de la simulation de frappe clavier - on passe tous les param�tres, KBSimEx se d�brouille.
	// nouveau en 0.91 : flag NOFOCUS permet de ne pas mettre le focus syst�matiquement sur la fen�tre
	// (n�cessaire avec Terminal Server, sinon perte du focus sur les champs login/pwd)
	if (_strnicmp(gptActions[iAction].szKBSim,"[NOFOCUS]",strlen("[NOFOCUS]"))==0) w=NULL;
	KBSimEx(w,gptActions[iAction].szKBSim,gptActions[iAction].szId1Value,
										  gptActions[iAction].szId2Value,
										  gptActions[iAction].szId3Value,
										  gptActions[iAction].szId4Value,
										  szDecryptedPassword);

	SecureZeroMemory(szDecryptedPassword,sizeof(szDecryptedPassword));

	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ChooseConfigInitDialog()
//-----------------------------------------------------------------------------
// InitDialog DialogProc de la fen�tre de choix de config en cas de multi-comptes
//-----------------------------------------------------------------------------
void ChooseConfigInitDialog(HWND w,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, ""));

	T_CHOOSE_CONFIG *lpConfigs=(T_CHOOSE_CONFIG*)lp;
	if (lpConfigs==NULL) goto end;
	HWND wLV;
	LVCOLUMN lvc;
	LVITEM   lvi;
	int i,pos;

	gwChooseConfig=w;

	// conserve le lp pour la suite
	SetWindowLong(w,DWL_USER,lp);

	// icone ALT-TAB
	SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
	SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 

	// init de la listview
	// listview
	wLV=GetDlgItem(w,LV_CONFIGS);
	// listview - cr�ation colonnes 
	lvc.mask=LVCF_WIDTH | LVCF_TEXT ;
	lvc.cx=218;
	lvc.pszText="Application";
	ListView_InsertColumn(wLV,0,&lvc);

	lvc.mask=LVCF_WIDTH | LVCF_TEXT;
	lvc.cx=200;
	lvc.pszText="Identifiant";
	ListView_InsertColumn(wLV,1,&lvc);

	// listview - styles
	ListView_SetExtendedListViewStyle(wLV,ListView_GetExtendedListViewStyle(wLV)|LVS_EX_FULLROWSELECT);

	// listview - remplissage
	for (i=0;i<lpConfigs->iNbConfigs;i++)
	{
		lvi.mask=LVIF_TEXT | LVIF_PARAM;
		lvi.iItem=i;
		lvi.iSubItem=0;
		lvi.pszText=gptActions[lpConfigs->tabConfigs[i]].szApplication;
		lvi.lParam=lpConfigs->tabConfigs[i];		// index de la config dans table g�n�rale gptActions
		pos=ListView_InsertItem(GetDlgItem(w,LV_CONFIGS),&lvi);
		
		if (pos!=-1)
		{
			lvi.mask=LVIF_TEXT;
			lvi.iItem=pos;
			lvi.iSubItem=1;
			lvi.pszText=GetComputedValue(gptActions[lpConfigs->tabConfigs[i]].szId1Value);
			ListView_SetItem(GetDlgItem(w,LV_CONFIGS),&lvi);
		}
	}
	// s�lection par d�faut
	SendMessage(GetDlgItem(w,CB_TYPE),CB_SETCURSEL,0,0);
	ListView_SetItemState(GetDlgItem(w,LV_CONFIGS),0,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
	
	// titre en gras
	SetTextBold(w,TX_FRAME);
	// centrage
	SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	MACRO_SET_SEPARATOR;
	// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
	// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
	if (giRefreshTimer==giTimer) giRefreshTimer=11;
	SetTimer(w,giRefreshTimer,200,NULL);

end:
	TRACE((TRACE_LEAVE,_F_, ""));
}
//-----------------------------------------------------------------------------
// ChooseConfigOnOK()
//-----------------------------------------------------------------------------
// Appel�e lorsque l'utilisateur clique sur OK ou double-clique sur un item de la listview
//-----------------------------------------------------------------------------
int ChooseConfigOnOK(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int iSelectedItem=ListView_GetNextItem(GetDlgItem(w,LV_CONFIGS),-1,LVNI_SELECTED);

	if (iSelectedItem==-1) goto end;
	// R�cup�re le lparam de l'item s�lectionn�. 
	// Le lparam contient l'index de la config dans la table g�n�rale gptActions : lpConfigs->tabConfigs[i]
	LVITEM lvi;
	lvi.mask=LVIF_PARAM ;
	lvi.iItem=iSelectedItem;
	ListView_GetItem(GetDlgItem(w,LV_CONFIGS),&lvi);
	// R�cup�re le lparam de la fen�tre qui contient le pointeur vers la structure configs
	T_CHOOSE_CONFIG *lpConfigs=(T_CHOOSE_CONFIG *)GetWindowLong(w,DWL_USER);
	// Stocke l'index de la configuration choisie par l'utilisateur
	lpConfigs->iConfig=lvi.lParam;
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ChooseConfigDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fen�tre de choix de config en cas de multi-comptes
//-----------------------------------------------------------------------------
static int CALLBACK ChooseConfigDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			ChooseConfigInitDialog(w,lp);
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
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
					if (ChooseConfigOnOK(w)==0) EndDialog(w,IDOK);
					break;
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
			}
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case NM_DBLCLK: 
					if (ChooseConfigOnOK(w)==0) EndDialog(w,IDOK);
					break;
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
// ChooseConfig()
//-----------------------------------------------------------------------------
// Propose une fen�tre de choix de la configuration � utiliser si plusieurs
// matchent (=multi-comptes !)
//-----------------------------------------------------------------------------
int ChooseConfig(HWND w,int *piAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",*piAction));
	int rc=-1;
	int i,j;
	T_CHOOSE_CONFIG config;
	config.iNbConfigs=1;			// nombre de configs qui matchent
	config.tabConfigs[0]=*piAction;	// premi�re config 
	config.iConfig=*piAction;		// premi�re config

	// Cherche si d'autres configurations ACTIVES ont les m�mes caract�ristiques :
	// - iType
	// - szTitle
	// - szURL
	// - szId1Name...szId4Name
	// - id2Type...id4Type
	// - szPwdName
	for (i=0;i<giNbActions;i++)
	{
		if (i==*piAction) continue;
		if ((gptActions[i].bActive) &&
			(gptActions[*piAction].iType==gptActions[i].iType) &&
			(gptActions[*piAction].id2Type==gptActions[i].id2Type) &&
			(gptActions[*piAction].id3Type==gptActions[i].id3Type) &&
			(gptActions[*piAction].id4Type==gptActions[i].id4Type) &&
			(strcmp(gptActions[*piAction].szTitle,gptActions[i].szTitle)==0) &&
			(strcmp(gptActions[*piAction].szURL,gptActions[i].szURL)==0) &&
			(strcmp(gptActions[*piAction].szId1Name,gptActions[i].szId1Name)==0) &&
			(strcmp(gptActions[*piAction].szId2Name,gptActions[i].szId2Name)==0) &&
			(strcmp(gptActions[*piAction].szId3Name,gptActions[i].szId3Name)==0) &&
			(strcmp(gptActions[*piAction].szId4Name,gptActions[i].szId4Name)==0) &&
			(strcmp(gptActions[*piAction].szPwdName,gptActions[i].szPwdName)==0))
		{
			config.tabConfigs[config.iNbConfigs]=i;
			config.iNbConfigs++;
		}
	}
	TRACE((TRACE_INFO,_F_,"Liste des configurations possibles :"));
	for (i=0;i<config.iNbConfigs;i++) 
	{
		TRACE((TRACE_INFO,_F_,"%d : %d",i,config.tabConfigs[i]));
	}
	// si aucune config trouv�e autre que celle initiale, on sort et on fait le SSO avec cette config
	if (config.iNbConfigs==1) { giLaunchedApp=-1; rc=0; goto end; }

	// avant d'afficher la fen�tre de choix des configs, on va regarder si l'une des configs trouv�es
	// correspond � une application qui vient d'�tre lanc�e par LaunchSelectedApp(). 
	// si c'est le cas, inutile de proposer � l'utilisateur de choisir, on choisit pour lui ! (�a c'est vraiment g�nial)
	TRACE((TRACE_DEBUG,_F_,"giLaunchedApp=%d",giLaunchedApp));
	if (giLaunchedApp!=-1)
	{
		for (i=0;i<config.iNbConfigs;i++) 
		{
			if (config.tabConfigs[i]==giLaunchedApp) // trouv�, on utilisera celle-l�, on sort.
			{
				TRACE((TRACE_INFO,_F_,"Lanc� depuis LaunchPad action %d",config.tabConfigs[i]));
				*piAction=config.tabConfigs[i];
				giLaunchedApp=-1;
				rc=0;
				// repositionne tLastSSO et wLastSSO des actions qui ne seront pas trait�es
				// l'action trait�e sera mise � jour au moment du SSO
				for (j=0;j<config.iNbConfigs;j++)
				{
					if (config.tabConfigs[j]==*piAction) continue; // tout sauf celle choisie par l'utilisateur
					time(&gptActions[config.tabConfigs[j]].tLastSSO);
					gptActions[config.tabConfigs[j]].wLastSSO=w;
				}
				goto end;
			}
		}
	}
	// pas trouv�, on oublie, c'�tait une mauvaise piste
	giLaunchedApp=-1;

	// affiche la fen�tre de choix des configs
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_CHOOSE_CONFIG),w,ChooseConfigDialogProc,LPARAM(&config))!=IDOK) 
	{
		// l'utilisateur a annul�, on marque tout le monde en WAIT_ONE_MINUTE comme �a on ne fait pas le SSO tout de suite
		// repositionne tLastDetect et wLastDetect 
		for (i=0;i<config.iNbConfigs;i++) 
		{
			//gptActions[config.tabConfigs[i]].wLastDetect=w;
			//time(&gptActions[config.tabConfigs[i]].tLastDetect);
			time(&gptActions[config.tabConfigs[i]].tLastSSO);
			gptActions[config.tabConfigs[i]].wLastSSO=w;
			gptActions[config.tabConfigs[i]].iWaitFor=WAIT_ONE_MINUTE;
			TRACE((TRACE_DEBUG,_F_,"gptActions(%d).iWaitFor=WAIT_ONE_MINUTE",config.tabConfigs[i]));
		}
		goto end;
	}
	// retourne l'action qui a �t� choisie par l'utilisateur
	TRACE((TRACE_INFO,_F_,"Choix de l'utilisateur : action %d",config.iConfig));
	*piAction=config.iConfig;
	// repositionne tLastSSO et wLastSSO des actions qui ne seront pas trait�es
	// l'action trait�e sera mise � jour au moment du SSO
	for (i=0;i<config.iNbConfigs;i++)
	{
		if (config.tabConfigs[i]==*piAction) continue; // tout sauf celle choisie par l'utilisateur
		time(&gptActions[config.tabConfigs[i]].tLastSSO);
		gptActions[config.tabConfigs[i]].wLastSSO=w;
	}
	rc=0;
end:
	gwChooseConfig=NULL;
	TRACE((TRACE_LEAVE,_F_, "rc=%d iAction=%d",rc,*piAction));
	return rc;
}

//-----------------------------------------------------------------------------
// AskMissingValues()
//-----------------------------------------------------------------------------
// Nouvelle fonction en 1.19 pour reprendre tout le code qui demande � l'utilisateur
// les informations identifiant(s) et mot de passe �ventuellement manquantes
//-----------------------------------------------------------------------------
// i = index de l'action dans gptActions
//-----------------------------------------------------------------------------
int AskMissingValues(HWND w,int i,int iPopupType)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	//0.91 : v�rifie que chaque champ identifiant et mot de passe d�clar� a bien une valeur associ�e
	//       sinon demande les valeurs manquantes � l'utilisateur !
	//0.92 : correction ISSUE#7 : le cas des popup qui ont toujours id et pwd mais pas de chaque champ 
	//		 identifiant et mot de passe d�clar� n'�tait pas trait� !
	gbDontAskId=TRUE;
	gbDontAskId2=TRUE;
	gbDontAskId3=TRUE;
	gbDontAskId4=TRUE;
	gbDontAskPwd=TRUE;

	// 0.93 : logs
	// ISSUE#127 : d�plac� + loin, une fois qu'on a r�ussi le SSO
	// swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_SECONDARY_LOGIN_SUCCESS,gptActions[i].szApplication,gptActions[i].szId1Value,NULL,i);

	if (gptActions[i].bKBSim && gptActions[i].szKBSim[0]!=0) // simulation de frappe clavier
	{
		if (strnistr(gptActions[i].szKBSim,"[ID]",-1)!=NULL &&  *gptActions[i].szId1Value==0) gbDontAskId=FALSE;
		if (strnistr(gptActions[i].szKBSim,"[ID2]",-1)!=NULL && *gptActions[i].szId2Value==0) gbDontAskId2=FALSE;
		if (strnistr(gptActions[i].szKBSim,"[ID3]",-1)!=NULL && *gptActions[i].szId3Value==0) gbDontAskId3=FALSE;
		if (strnistr(gptActions[i].szKBSim,"[ID4]",-1)!=NULL && *gptActions[i].szId4Value==0) gbDontAskId4=FALSE;
		if (strnistr(gptActions[i].szKBSim,"[PWD]",-1)!=NULL && *gptActions[i].szPwdEncryptedValue==0) gbDontAskPwd=FALSE;
	}
	else // SSO normal
	{
		if (*gptActions[i].szId1Name!=0 && *gptActions[i].szId1Value==0) gbDontAskId=FALSE;
		if (*gptActions[i].szId2Name!=0 && *gptActions[i].szId2Value==0) gbDontAskId2=FALSE;
		if (*gptActions[i].szId3Name!=0 && *gptActions[i].szId3Value==0) gbDontAskId3=FALSE;
		if (*gptActions[i].szId4Name!=0 && gptActions[i].id4Type!=CHECK_LABEL && *gptActions[i].szId4Value==0) gbDontAskId4=FALSE;
		if (*gptActions[i].szPwdName!=0 && *gptActions[i].szPwdEncryptedValue==0) gbDontAskPwd=FALSE;
	}
	// cas des popups (0.92 - ISSUE#7)
	if (gptActions[i].iType==POPSSO) 
	{
		if (*gptActions[i].szId1Value==0) gbDontAskId=FALSE;
		if (*gptActions[i].szPwdEncryptedValue==0) gbDontAskPwd=FALSE;
	}
			
	// s'il y a au moins un champ non renseign�, afficher la fen�tre de saisie
	if (!gbDontAskId || !gbDontAskId2 || !gbDontAskId3 || !gbDontAskId4 || !gbDontAskPwd)
	{
		T_IDANDPWDDIALOG params;
		params.bCenter=TRUE;
		params.iAction=i;
		params.iTitle=IDS_IDANDPWDTITLE_MISSING;
		sprintf_s(params.szText,sizeof(params.szText),GetString(IDS_IDANDPWDTEXT_MISSING),gptActions[i].szApplication);
					
		// ISSUE#334
		// if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_ID_AND_PWD),HWND_DESKTOP,IdAndPwdDialogProc,(LPARAM)&params)==IDOK)
		if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_ID_AND_PWD),w,IdAndPwdDialogProc,(LPARAM)&params)==IDOK) 
		{
			gwIdAndPwdDialogProc=NULL;
			SaveApplications();
		}
		else
		{
			gwIdAndPwdDialogProc=NULL;
			// l'utilisateur a annul�, on marque la config en WAIT_ONE_MINUTE comme �a on ne fait pas 
			// le SSO tout de suite
			time(&gptActions[i].tLastSSO);
			gptActions[i].wLastSSO=w;
			LastDetect_AddOrUpdateWindow(w,iPopupType);
			gptActions[i].iWaitFor=WAIT_ONE_MINUTE;
			TRACE((TRACE_DEBUG,_F_,"gptActions(%d).iWaitFor=WAIT_ONE_MINUTE",i));
			goto end;
		}
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// EnumWindowsProc()
//-----------------------------------------------------------------------------
// Callback d'�num�ration de fen�tres pr�sentes sur le bureau et d�clenchement
// du SSO le cas �ch�ant
//-----------------------------------------------------------------------------
// [rc] : toujours TRUE (continuer l'�num�ration)
//-----------------------------------------------------------------------------
static int CALLBACK EnumWindowsProc(HWND w, LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	int i;
	time_t tNow,tLastSSOOnThisWindow;
	BOOL bDoSSO;
	char *pszChromePopupURL=NULL;
	char *pszURL=NULL;
	char *pszURL2=NULL;
	int iPopupType=POPUP_NONE;
	char szClassName[128+1]; // pour stockage nom de classe de la fen�tre
	char szTitre[255+1];	  // pour stockage titre de fen�tre
	int rc;
	char szMsg[512+1];
	int lenTitle;
	int iBrowser=BROWSER_NONE;
	IUIAutomationElement* pDocument=NULL; // document navigateur
	
	guiNbWindows++;
	// 0.93B4 : fen�tres �ventuellement exclues 
	if (IsExcluded(w)) goto end;
	// lecture du titre de la fen�tre
	GetWindowText(w,szTitre,sizeof(szTitre));
	if (*szTitre==0) goto end; // si fen�tre sans titre, on passe ! <optim>
	if (!IsWindowVisible(w)) goto end; // fen�tre cach�e, on passe ! <optim+compteur>
	if (IsIconic(w)) goto end; // ISSUE#280 : ignore les fen�tres r�duites dans la barre des taches pour �viter des pertes de focus inutiles
	guiNbVisibleWindows++;

	// 0.93 : marque la fen�tre comme toujours pr�sente � l'�cran dans liste des derniers SSO r�alis�s
	LastDetect_TagWindow(w); 

	// lecture de la classe de la fen�tre (pour reconnaitre IE et Firefox ensuite)
	GetClassName(w,szClassName,sizeof(szClassName));

	// nouveau en 1.07B6
	// si c'est une popup chrome, comme on ne peut pas discriminer sur le titre, il vaut mieux lire une seule fois l'URL ici
	// sinon on le fait autant de fois qu'il y a d'applications configur�es, c'est inutile et trop consommateur
	if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0)
	{
		pszChromePopupURL=GetChromePopupURL(w);
	}
	
	TRACE((TRACE_DEBUG,_F_,"szTitre=%s",szTitre));

	// boucle dans la liste d'action pour voir si la fen�tre correspond � une config connue
    for (i=0;i<giNbActions;i++)
    {
    	if (gptActions[i].bSafe) goto suite;
		if (!gptActions[i].bActive) goto suite; // action d�sactiv�e
		if (!gptActions[i].bSaved) { TRACE((TRACE_INFO,_F_,"action %d non enregistr�e => SSO non ex�cut�",i)); goto suite; } // 0.93B6 ISSUE#55
		if (gptActions[i].iType==UNK) goto suite; // 0.85 : ne traite pas si type inconnu
		
		// Avant de comparer le titre, v�rifier si ce n'est pas une popup Chrome
		// En effet, avec Chrome, le titre n'est pas toujours repr�sentatif (notamment sur popup proxy)
		// if (gptActions[i].iType==POPSSO && strncmp(szClassName,"Chrome_WidgetWin_",17)==0) pszURL=GetChromePopupURL(w);
		// if (pszURL!=NULL)
		if (gptActions[i].iType==POPSSO && pszChromePopupURL!=NULL)
		{
			iPopupType=POPUP_CHROME;
			TRACE((TRACE_DEBUG,_F_,"POPUP_CHROME, on ignore le titre"));
		}
		else
		{
			lenTitle=strlen(gptActions[i].szTitle);
			if (lenTitle==0) goto suite; // 0.85 : ne traite pas si pas de titre (sauf pour popup chrome)
    		// 0.80 : on compare sur le d�but du titre et non plus sur une partie du titre 
			// if (strstr(szTitre,gptActions[i].szTitle)==NULL) goto suite; // fen�tre inconnue...
			// 0.92B3 : �volution de la comparaison du titre pour prendre en compte le joker *
			// if (_strnicmp(szTitre,gptActions[i].szTitle,lenTitle)!=0) goto suite; // fen�tre inconnue...
			if (!swStringMatch(szTitre,gptActions[i].szTitle)) goto suite;
		}
		// A ce stade, on a associ� le titre � une action
		// (uniquement le titre, c'est � dire qu'on n'a pas encore v�rifi� que l'URL �tait correcte)
		TRACE((TRACE_INFO,_F_,"======= Fen�tre handle 0x%08lx titre connu (%s) classe (%s) action (%d) type (%d) � v�rifier maintenant",w,szTitre,szClassName,i,gptActions[i].iType));
    	if (gptActions[i].iType==POPSSO) 
    	{
			if (strcmp(szClassName,gcszMozillaDialogClassName)==0) // POPUP FIREFOX
			{
				pszURL=GetFirefoxPopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup firefox non trouvee")); goto suite; }
				iPopupType=POPUP_FIREFOX;
			}
			else if (strcmp(szClassName,"#32770")==0 && (strcmp(szTitre,"S�curit� de Windows")==0 ||
					 strcmp(szTitre,"Windows Security")==0)) // POPUP IE8 SUR W7 [ISSUE#5] (FR et US uniquement... pas beau)
			{
				pszURL=GetW7PopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W7 non trouvee")); goto suite; }
				iPopupType=POPUP_W7;
			}
			else if (strcmp(szClassName,"Credential Dialog Xaml Host")==0 &&  
					(strcmp(szTitre,"S�curit� Windows")==0 || strcmp(szTitre,"Windows Security")==0)) // POPUP W10 anniversaire... IE, EDGE, partages r�seau, etc.
			{
				pszURL=GetW10PopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W10 non trouvee")); goto end; }
				iPopupType=POPUP_W10;
			}
			else if (iPopupType==POPUP_CHROME)
			{
				pszURL=GetChromePopupURL(w); // oui c'est un peu con parce qu'on a l'info dans pszChromePopupURL mais pour simplifier la logique de lib�ration des pointeurs c'est mieux comme �a
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup Chrome non trouvee")); goto suite; }
			}
			else
			{
				iPopupType=POPUP_XP;
			}
			// il faut v�rifier que l'URL matche pour FIREFOX, W7 et CHROME car elles ont toutes le meme titre !
			// Popup IE sous XP, pas la peine, titre distinctif
			if (iPopupType==POPUP_FIREFOX || iPopupType==POPUP_W7 || iPopupType==POPUP_CHROME || iPopupType==POPUP_W10)
			{
				TRACE((TRACE_INFO,_F_,"URL trouvee  = %s",pszURL));
				TRACE((TRACE_INFO,_F_,"URL attendue = %s",gptActions[i].szURL));
				// ISSUE#87 : l'URL dans la config peut avoir la forme : le site www demande*|http://www
				int len=strlen(gptActions[i].szURL);
				pszURL2=(char*)malloc(len+1);
				if (pszURL2==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%ld)",len+1)); goto end; }
				memcpy(pszURL2,gptActions[i].szURL,len+1);
				char *p=strchr(pszURL2,'|');
				if (p!=NULL)
				{
					*p=0;
					p++;
					if (swCheckBrowserURL(iPopupType,p)!=0)
					{
						TRACE((TRACE_INFO,_F_,"Impossible de v�rifier l'URL de la barre d'URL... on ne fait pas le SSO !"));
						goto suite;// URL popup authentification inconnue
					}
				}
				// si p!=null c'est qu'on a trouv� un | et une URL derri�re, il faut la v�rifier
				// ce test ne change pas, sauf qu'il porte sur pszURL2 pour couvrir les 2 cas (avec ou sans |)
				// 0.92B6 : utilise le swStringMatch, permet donc d'utiliser * en d�but de cha�ne
				if (!swStringMatch(pszURL,pszURL2))
				{
					TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
					goto suite;// URL popup authentification inconnue
				}
			}
		}
		else if (gptActions[i].iType==WINSSO && *(gptActions[i].szURL)!=0) // fen�tre Windows avec URL, il faut v�rifier que l'URL matche
		{
			if (!CheckURL(w,i))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;// URL popup authentification inconnue
			}
		}
		else if (gptActions[i].iType==XINSSO && *(gptActions[i].szURL)!=0)
		{
			// ISSUE#400 : ajout du CheckURL pour qu'il s'applique �galement aux configurations Windows simplifi�es
			if (!CheckURL(w,i))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;
			}
		}
		else if (gptActions[i].iType==WEBSSO || gptActions[i].iType==XEBSSO) // action WEB, il faut v�rifier que l'URL matche
		{
			if (strcmp(szClassName,"IEFrame")==0 || // IE
				strcmp(szClassName,"#32770")==0 ||  // Network Connect
				strcmp(szClassName,"rctrl_renwnd32")==0 || // Outlook 97 � 2003 (au moins, � v�rifier pour 2007)
				strcmp(szClassName,"OpusApp")==0 || // Word 97 � 2003 (au moins, � v�rifier pour 2007)
				strcmp(szClassName,"ExploreWClass")==0 || strcmp(szClassName,"CabinetWClass")==0) // Explorateur Windows
			{
				iBrowser=BROWSER_IE;
				// ISSUE#376 : remplacement de GetIEURL (qui lit juste l'URL de la page) par CheckIEURL (qui parcours l'ensemble des iframes pour rechercher l'URL)
				//pszURL=GetIEURL(w,TRUE);
				pszURL=CheckIEURL(w,TRUE,i);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL IE non trouvee : on passe !")); goto suite; }
			}
			else if (strcmp(szClassName,gcszMozillaUIClassName)==0) // FF3
			{
				iBrowser=BROWSER_FIREFOX3;
				pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX3,TRUE);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 3- non trouvee : on passe !")); goto suite; }
			}
			else if (strcmp(szClassName,gcszMozillaClassName)==0) // FF4
			{
				iBrowser=BROWSER_FIREFOX4;
				pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX4,TRUE);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 4+ non trouvee : on passe !")); goto suite; }
			}
			else if (strcmp(szClassName,"Maxthon2_Frame")==0) // Maxthon
			{
				iBrowser=BROWSER_MAXTHON;
				if (gptActions[i].iType==XEBSSO) 
				{
					TRACE((TRACE_ERROR,_F_,"Nouvelle methode de configuration non supportee avec Maxthon"));
					goto suite;
				}
				pszURL=GetMaxthonURL();
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Maxthon non trouvee : on passe !")); goto suite; }
			}
			else if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0) // ISSUE#77 : Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
			{
				iBrowser=BROWSER_CHROME;
				/*if (gptActions[i].iType!=XEBSSO) 
				{
					TRACE((TRACE_ERROR,_F_,"Ancienne methode de configuration non supportee avec Chrome"));
					goto suite;
				}*/
				ForceChromeAccessibility(w);
				pszURL=GetChromeURL(w);
				if (pszURL==NULL) pszURL=GetChromeURL51(w); // ISSUE#282
				// finalement GetChromeURL51 marche bien, pas la peine de faire NewGetChromeURL qui casse ISSUE#266
				// ISSUE#381 : si, il faut le faire sinon ne fonctionne pas avec les sites lanc�s en mode application
				// ISSUE#382 : avec Chrome 69, GetChromeURL et  GetChromeURL51 ne fonctionnent pas, il faut NewGetChromeURL
				if (pszURL==NULL || *pszURL==0) pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); // ISSUE#314
				if (pszURL==NULL || *pszURL==0) { Sleep(100); pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); } // Suite � ISSUE#404, Chrome n'a pas eu le temps de monter l'interface IAccessible apr�s l'appel � ForceChromeAccessibility
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Chrome non trouvee : on passe !")); goto suite; }
			}
			// ISSUE#347 : prise en compte de EDGE avec UIA
			else if (strcmp(szClassName,"ApplicationFrameWindow")==0) 
			{
				iBrowser=BROWSER_EDGE;
				pszURL=GetEdgeURL(w,&pDocument);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Edge non trouvee : on passe !")); goto suite; }
			}
			else // autre ??
			{
				TRACE((TRACE_ERROR,_F_,"Unknown class : %s !",szClassName)); goto suite; 
			}
			TRACE((TRACE_INFO,_F_,"URL trouvee  = %s",pszURL));
			TRACE((TRACE_INFO,_F_,"URL attendue = %s",gptActions[i].szURL));

			// 0.92B6 : utilise le swStringMatch, permet donc d'utiliser * en d�but de cha�ne
			// if (!swStringMatch(pszURL,gptActions[i].szURL))
			if (!swURLMatch(pszURL,gptActions[i].szURL))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;// URL popup authentification inconnue
			}
		}
		// ARRIVE ICI, ON SAIT QUE LA FENETRE EST BIEN ASSOCIEE A L'ACTION i ET QU'IL FAUT FAIRE LE SSO...
		// ... SAUF SI DEJA FAIT RECEMMENT !
		TRACE((TRACE_INFO,_F_,"======= Fen�tre v�rif�e OK, on v�rifie si elle a �t� trait�e r�cemment"));
		TRACE((TRACE_DEBUG,_F_,"Fenetre      ='%s'",szTitre));
		TRACE((TRACE_DEBUG,_F_,"Application  ='%s'",gptActions[i].szApplication));
		TRACE((TRACE_DEBUG,_F_,"Type         =%d",gptActions[i].iType));

		// ISSUE#107 : conserve l'id de la derni�re application reconnue pour l'accompagnement du changement de mot de passe
		// ISSUE#108 : conserve l'id de la derni�re application reconnue pour la s�lectionner par d�faut dans la fen�tre de gestion des sites
		giLastApplicationSSO=i;
		giLastApplicationConfig=i;

		// 0.93
		//TRACE((TRACE_DEBUG,_F_,"now-tLastDetect=%ld",t-gptActions[i].tLastDetect));
		//TRACE((TRACE_DEBUG,_F_,"wLastDetect    =0x%08lx",gptActions[i].wLastDetect));
		time(&tNow);
		TRACE((TRACE_DEBUG,_F_,"tLastSSO     =%ld (time du dernier SSO sur cette action)",gptActions[i].tLastSSO));
		TRACE((TRACE_DEBUG,_F_,"tNow-tLastSSO=%ld (nb secondes depuis dernier SSO sur cette action)",tNow-gptActions[i].tLastSSO));
		TRACE((TRACE_DEBUG,_F_,"wLastSSO     =0x%08lx",gptActions[i].wLastSSO));

		bDoSSO=true;

		// on n'ex�cute pas (une action utilisateur est n�cessaire)
		if (gptActions[i].bWaitForUserAction) 
		{
			TRACE((TRACE_INFO,_F_,"SSO en attente d'action utilisateur"));
			goto suite;
		}
		
		// D�tection d'un SSO d�j� fait r�cemment sur la fen�tre afin de pr�venir les essais multiples 
		// avec mauvais mots de passe qui pourraient bloquer le compte 
		tLastSSOOnThisWindow=LastDetect_GetTime(w); // date de dernier SSO sur cette fen�tre (toutes actions confondues)
		TRACE((TRACE_DEBUG,_F_,"tLastSSOOnThisWindow        =%ld (time du dernier SSO sur cette fen�tre)",tLastSSOOnThisWindow));
		TRACE((TRACE_DEBUG,_F_,"tNow-tLastSSOOnThisWindow	=%ld (nb secondes depuis dernier SSO sur cette fen�tre)",tNow-tLastSSOOnThisWindow));

		if (gptActions[i].iType==WEBSSO || gptActions[i].iType==XEBSSO || 
			(gptActions[i].iType==POPSSO && iPopupType==POPUP_CHROME)) // cas particulier : la popup chrome n'est pas une fen�tre comme les autres popup
		{
			// si tLastSSOOnThisWindow==1 => handle inconnu donc jamais trait� => on fait le SSO, sinon :
			if (tLastSSOOnThisWindow!=-1) // on a d�j� fait un SSO sur cette m�me fenetre (cette action ou une autre, peu importe, par exemple une autre action � cause du multi-comptes)
			{
				// c'est du Web, rien de choquant (le navigateur a toujours le meme handle quel que soit le site acc�d� !
				// Par contre, il faut dans les cas suivants ne pas r�essayer imm�diatement, mais laisser passer
				// un d�lai avant le prochain essai (pr�cis� en face de chaque cas) :
				// - Eviter de griller un compte avec X saisies de mauvais mots de passe de suite (iWaitFor=giWaitBeforeNewSSO)
				// - Ne pas bouffer de CPU en cherchant tous les 500ms des champs qu'on n'a pas trouv� dans la page (iWaitFor=WAIT_IF_SSO_NOK)
				// - Ne pas bouffer de CPU quand l'URL ne correspond pas (alors que le titre correspond) (iWaitFor=WAIT_IF_BAD_URL)
				if ((tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor) 
				{
					TRACE((TRACE_INFO,_F_,"(tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor(%d)",gptActions[i].iWaitFor));
					bDoSSO=false;
				}
			}
			else // jamais fait de SSO sur cette fen�tre, on a envie de tenter, mais il ne faut juste pas le faire  
				 // si l'utilisateur a annul� dans la fen�tre de choix multi-comptes (ISSUE#133)
			{
				if (gptActions[i].iWaitFor==WAIT_ONE_MINUTE) 
				{
					TRACE((TRACE_INFO,_F_,"gptActions[i].iWaitFor==WAIT_ONE_MINUTE"));
					bDoSSO=FALSE;
				}
			}
		}
		else // WINSSO ou POPUP
		{
			if (tLastSSOOnThisWindow!=-1)
			{
 				// fen�tre trait�e pr�c�demment par cette action OU PAR UNE AUTRE (cause multi-comptes)
				// elle est toujours l�, donc c'est l'authentification qui rame, pas la peine de r�essayer, 
				// elle disparaitra d'elle m�me au  bout d'un moment...
   				TRACE((TRACE_INFO,_F_,"Fenetre %s handle identique deja traitee il y a %d secondes, on ne fait rien",gptActions[i].szTitle,tNow-tLastSSOOnThisWindow));
				bDoSSO=false;
			}	
			else  
			{
				// fen�tre inconnue au bataillon
				TRACE((TRACE_INFO,_F_,"Fenetre %s handle diff�rent",gptActions[i].szTitle));
				if (gptActions[i].iWaitFor==WAIT_ONE_MINUTE && (tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor)
				{
					TRACE((TRACE_DEBUG,_F_,"WAIT_ONE_MINUTE"));
					bDoSSO=false;
				}
				else
				{
					if ((tNow-gptActions[i].tLastSSO)<5) // 0.86 : passage de 3 � 5 secondes pour applis qui rament...
					{
						TRACE((TRACE_DEBUG,_F_,"Le SSO sur cette action a �t� r�alis� il y a moins de 5 secondes"));
						// le SSO sur cette action a �t� r�alis� il y a moins de 5 secondes et cette fen�tre est nouvelle
						// => 2 possibilit�s :
						// 1) c'est une r�apparition suite � un �chec d'authentification
						// 2) c'est vraiment une nouvelle fen�tre ouverte dans les 5 secondes
						// pour diff�rencier, il faut voir si la fen�tre sur laquelle le SSO a �t� fait pr�cedemment
						// est toujours � l'�cran ou pas. Si elle est toujours l�, c'est bien une nouvelle fen�tre
						// Si elle n'est plus l�, on est sans doute dans le cas de l'�chec d'authent
						if(gptActions[i].wLastSSO!=NULL && IsWindow(gptActions[i].wLastSSO))
						{
							TRACE((TRACE_DEBUG,_F_,"La fen�tre pr�c�demment SSOis�e est toujours l�, celle-ci est donc nouvelle"));
							// fen�tre toujours l� ==> nouvelle fen�tre, on fait le SSO
							gptActions[i].iWaitFor=giWaitBeforeNewSSO;
    						bDoSSO=true;
						}
						else // fen�tre plus l�, sans doute un �chec d'authentification 
						{
							TRACE((TRACE_DEBUG,_F_,"La fen�tre pr�c�demment SSOis�e n'est plus l�, sans doute un retour cause �chec authentification"));
							// 0.85 : on sugg�re donc � l'utilisateur de changer son mot de passe.
							if (gptActions[i].bAutoLock) // 0.66 ne suspend pas si l'utilisateur a mis autoLock=NO (le SSO sera donc fait)
							{
								char szSubTitle[256];
								KillTimer(NULL,giTimer); giTimer=0;
								TRACE((TRACE_INFO,_F_,"Fenetre %s handle different deja traitee il y a %d secondes !",gptActions[i].szTitle,tNow-gptActions[i].tLastSSO));
								T_MESSAGEBOX3B_PARAMS params;
								params.szIcone=IDI_EXCLAMATION;
								params.iB1String=IDS_DESACTIVATE_B1; // r�essayer
								params.iB2String=IDS_DESACTIVATE_B2; // changer le mdp
								params.iB3String=IDS_DESACTIVATE_B3; // d�sactiver
								params.wParent=w;
								params.iTitleString=IDS_MESSAGEBOX_TITLE;
								params.bVCenterSubTitle=FALSE;
								sprintf_s(szSubTitle,sizeof(szSubTitle),GetString(IDS_DESACTIVATE_SUBTITLE),gptActions[i].szApplication);
								params.szSubTitle=szSubTitle;
								strcpy_s(szMsg,sizeof(szMsg),GetString(IDS_DESACTIVATE_MESSAGE));
								params.szMessage=szMsg;
								params.szMailTo=NULL;
								//if (MessageBox(w,szMsg,"swSSO", MB_YESNO | MB_ICONQUESTION)==IDYES)
								int reponse=MessageBox3B(&params);
								if (reponse==B1) // r�essayer
								{
									gptActions[i].iWaitFor=0;
									// rien � faire, �a va repartir tout seul :-)
								}
								else if (reponse==B2) // changer le mdp
								{
									ChangeApplicationPassword(w,i);
								}
								else // B3 : d�sactiver
								{
									bDoSSO=false;
									// 0.86 sauvegarde la d�sactivation dans le .INI !
									// 0.90A2 : on ne sauvegarde plus (risque d'�crire dans une section renomm�e)
									// WritePrivateProfileString(gptActions[i].szApplication,"active","NO",gszCfgFile);
									
									//gptActions[i].bActive=false;
									// 0.90B1 : finalement on ne d�sactive plus, on suspend pendant 1 minute (#107)
									gptActions[i].iWaitFor=WAIT_ONE_MINUTE;
								}
							}
    					}
					}
				}
			}
		}
		//------------------------------------------------------------------------------------------------------
		if (bDoSSO) // on a d�termin� que le SSO doit �tre tent�, mais peut-�tre pas en fonction de la config (ISSUE#176)
		//------------------------------------------------------------------------------------------------------
		{
			if (gptActions[i].iType==POPSSO)
			{
				switch (iPopupType)
				{
					case POPUP_XP:
					case POPUP_W7:
					case POPUP_W10:
						bDoSSO=gbSSOInternetExplorer;
						break;
					case POPUP_FIREFOX:
						bDoSSO=gbSSOFirefox;
						break;
					case POPUP_CHROME:
						bDoSSO=gbSSOChrome;
						break;
				}
			}
			else if (gptActions[i].iType==WEBSSO || gptActions[i].iType==XEBSSO)
			{
				switch (iBrowser)
				{
					case BROWSER_IE:
					case BROWSER_MAXTHON:
						bDoSSO=gbSSOInternetExplorer;
						break;
					case BROWSER_FIREFOX3:
					case BROWSER_FIREFOX4:
						bDoSSO=gbSSOFirefox;
						break;
					case BROWSER_CHROME:
						bDoSSO=gbSSOChrome;
						break;
					case BROWSER_EDGE:
						bDoSSO=gbSSOEdge;
						break;
				}
			}
		}
		//------------------------------------------------------------------------------------------------------
		if (bDoSSO) // cette fois, c'est s�r, SSO doit �tre tent�
		//------------------------------------------------------------------------------------------------------
		{
			TRACE((TRACE_INFO,_F_,"======= Fen�tre v�rifi�e OK et pas trait�e r�cemment, on lance le SSO !"));
			//0.80 : tue le timer le temps de faire le SSO, le r�arme ensuite (cas des pages lourdes � parser avec Firefox...)
			KillTimer(NULL,giTimer); giTimer=0;

			//0.91 : fait choisir l'appli � l'utilisateur si plusieurs matchent (gestion du multicomptes)
			//1.19 : d�placement plus tard, une fois qu'on est sur qu'on est sur la bonne page (� cause du filtre textuel ISSUE#373)
			// if (ChooseConfig(w,&i)!=0) goto end;

			//1.19 : d�placement du code vers une nouvelle fonction, qui sera appel�e plus tard, 
			// une fois qu'on est sur qu'on est sur la bonne page (� cause du filtre textuel ISSUE#373)
			// if (AskMissingValues(w,i,iPopupType)!=0) goto end;
						
			//0.89 : ind�pendamment du type (WIN, POP ou WEB), si c'est de la simulation de frappe clavier, on y va !
			if (iPopupType==POPUP_CHROME)
			{
				gptActions[i].bKBSim=TRUE;
				strcpy_s(gptActions[i].szKBSim,LEN_KBSIM+1,"[40][ID][40][TAB][40][PWD][40][ENTER]");
			}
			if (gptActions[i].bKBSim && gptActions[i].szKBSim[0]!=0)
			{
				TRACE((TRACE_INFO,_F_,"SSO en mode simulation de frappe clavier"));
				if (ChooseConfig(w,&i)!=0) goto end;
				if (AskMissingValues(w,i,iPopupType)!=0) goto end;
				KBSimSSO(w,i);
				time(&gptActions[i].tLastSSO);
				gptActions[i].wLastSSO=w;
				LastDetect_AddOrUpdateWindow(w,iPopupType);
				if (_strnicmp(gptActions[i].szKBSim,"[WAIT]",strlen("[WAIT]"))==0) gptActions[i].bWaitForUserAction=TRUE;
				// ISSUE#61 / 0.93 : on ne traite plus les popup W7 en simulation de frappe, marche pas avec IE9 ou W7 SP1 ?
				// if (iPopupType==POPUP_W7 || iPopupType==POPUP_CHROME) { gptActions[i].bKBSim=FALSE; *(gptActions[i].szKBSim)=0; }
				if (iPopupType==POPUP_CHROME) { gptActions[i].bKBSim=FALSE; *(gptActions[i].szKBSim)=0; }
				switch (gptActions[i].iType)
				{
					case POPSSO: guiNbPOPSSO++; break;
					case WINSSO: guiNbWINSSO++; break;
					case XINSSO: guiNbWINSSO++; break;
					case WEBSSO: guiNbWEBSSO++; break;
					case XEBSSO: guiNbWEBSSO++; break;
				}
				// ISSUE#127 (le swLogEvent �tait fait trop t�t, cf. plus haut)
				swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_SECONDARY_LOGIN_SUCCESS,gptActions[i].szApplication,gptActions[i].szId1Value,NULL,NULL,i);
				goto end;
			}
			switch(gptActions[i].iType)
			{
				case WINSSO: 
				case POPSSO: 
					// TODO plus tard : d�placer ces 2 lignes dans SSOWindows
					if (ChooseConfig(w,&i)!=0) goto end;
					if (AskMissingValues(w,i,iPopupType)!=0) goto end;

					if (SSOWindows(w,i,iPopupType)==0) // ISSUE#188
					{
						time(&gptActions[i].tLastSSO);
						gptActions[i].wLastSSO=w;
						LastDetect_AddOrUpdateWindow(w,iPopupType);
					}
					break;
				case XINSSO: 
					if (SSOWebAccessible(w,&i,BROWSER_XIN)==0)
					{
						time(&gptActions[i].tLastSSO);
						gptActions[i].wLastSSO=w;
						LastDetect_AddOrUpdateWindow(w,iPopupType);
					}
					break;
				case WEBSSO: 
				case XEBSSO: 
					switch (iBrowser)
					{
						case BROWSER_IE:
							if (gptActions[i].iType==XEBSSO)
								rc=SSOWebAccessible(w,&i,BROWSER_IE);
							else
								rc=SSOWeb(w,&i,w); 
							break;
						case BROWSER_FIREFOX3:
						case BROWSER_FIREFOX4:
							if (gptActions[i].iType==XEBSSO)
								rc=SSOWebAccessible(w,&i,iBrowser);
							else
							{
								// ISSUE#60 : en attendant d'avoir une r�ponse de Mozilla, on n'ex�cute pas les anciennes config 
								//            avec Firefox sous Windows 7 pour �viter le plantage !
								// ISSUE#60 modifi� en 0.94B2 : Vista et 64 bits seulement
								if (giOSBits==OS_64) 
								{
									TRACE((TRACE_INFO,_F_,"Ancienne configuration Firefox + Windows 64 bits : on n'ex�cute pas !"));
									rc=0;
								}
								else rc=SSOFirefox(w,&i,iBrowser); 
							}
							break;
						case BROWSER_MAXTHON:
							rc=SSOMaxthon(w,&i); 
							break;
						case BROWSER_CHROME:
							if (gptActions[i].iType==XEBSSO)
								rc=SSOWebAccessible(w,&i,iBrowser);
							else
							{
								if (giOSBits==OS_64) 
								{
									TRACE((TRACE_INFO,_F_,"Ancienne configuration Chrome + Windows 64 bits : on n'ex�cute pas !"));
									rc=0;
								}
								else rc=SSOFirefox(w,&i,iBrowser); // ISSUE#66 0.94 : chrome a impl�ment� ISimpleDOM comme Firefox !
							}
							break;
						case BROWSER_EDGE:
							rc=SSOWebUIA(w,&i,iBrowser,pDocument);
							break;
						default:
							rc=-1;
					}

					// repositionne tLastDetect et wLastDetect (�cras� par la suite dans un cas, cf. plus bas)
					//time(&gptActions[i].tLastDetect);
					//gptActions[i].wLastDetect=w;
					time(&gptActions[i].tLastSSO);
					gptActions[i].wLastSSO=w;
					LastDetect_AddOrUpdateWindow(w,iPopupType);

					if (rc==0) // SSO r�ussi
					{
						TRACE((TRACE_INFO,_F_,"SSO r�ussi, on ne le retente pas avant %d secondes",giWaitBeforeNewSSO));
						// on ne r�essaie pas si le SSO est r�ussi mais on ne peut pas cramer d�finitivement
						// ce SSO sinon un utilisateur qui se d�connecte ne pourra pas se reconnecter
						// tant qu'il n'aura pas ferm� / r�ouvert son navigateur (handle diff�rent) !
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=giWaitBeforeNewSSO; 
						// ISSUE#127 (le swLogEvent �tait fait trop t�t, cf. plus haut)
						swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_SECONDARY_LOGIN_SUCCESS,gptActions[i].szApplication,gptActions[i].szId1Value,NULL,NULL,i);
					}
					else if (rc==-2) // SSO abandonn� car l'URL ne correspond pas
					{
						// Si l'URL est diff�rente, c'est probablement que le SSO a �t� fait pr�c�demment
						// et que l'utilisateur est sur une autre page du site.
						// Il n'est pas utile de r�essayer imm�diatement, n�anmoins il faut essayer r�guli�rement
						// pour que le SSO fonctionne quand la page attendue arrivera (par exemple si la page
						// visit�e �tait une page pr�c�dant celle sur laquelle l'utilisateur va faire le SSO...)
						// Le r�-essai ne coute presque rien en CPU, on peut le faire toutes les 5 secondes
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=WAIT_IF_BAD_URL;
						TRACE((TRACE_INFO,_F_,"URL diff�rente de celle attendue, prochain essai dans %d secondes",WAIT_IF_BAD_URL));
					}
					else if (rc==-3) // SSO abandonn� car libelle szId4Value non trouv� dans la page
					{
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=WAIT_IF_LABEL_NOT_FOUND;
						TRACE((TRACE_INFO,_F_,"Libelle %s non trouv� dans la page, prochain essai dans %d secondes",gptActions[i].szId4Value,WAIT_IF_LABEL_NOT_FOUND));
					}
					else if (rc==-4) // SSO annul� par l'utilisateur (dans fenetre choix de config ou saisie des valeurs manquantes)
					{
						// iWaitFor a d�j� �t� mis � jour dans ChooseConfig ou AskMissingValues, on ne fait rien de plus
					}
					else // SSO rat�, erreur inattendue ou plus probablement champs non trouv�s
					{
						// Deux cas de figure... comment les diff�rencier ???
						// 1) La page n'est pas encore compl�tement arriv�e, il faut donc r�essayer un petit peu plus tard
						// 2) Le titre et l'URL ne permettent pas de distinguer la page courante de la page de login...
						// Solution : on retente quelques fois relativement rapidement pour traiter le cas 1 et au bout de 
						// quelques essais on espace les tentatives pour traiter plut�t le cas 2 (car parsing complet gourmant
						// en CPU, on ne peut pas le faire toutes les 2 secondes ind�finiment)
						TRACE((TRACE_INFO,_F_,"Echec SSOWeb application %s (essai %d)",gptActions[i].szApplication,gptActions[i].iNbEssais));
						gptActions[i].iNbEssais++;
						if (gptActions[i].iNbEssais<21) // les 20 premi�res fois, on retente imm�diatement
						{
							//gptActions[i].wLastDetect=NULL;
							//gptActions[i].tLastDetect=-1;
							gptActions[i].tLastSSO=-1;
							gptActions[i].wLastSSO=NULL;
							TRACE((TRACE_INFO,_F_,"Essai %d immediat",gptActions[i].iNbEssais));
						}
						else if (gptActions[i].iNbEssais<121) // puis 100 fois toutes les 2 secondes
						{
							gptActions[i].iWaitFor=WAIT_IF_SSO_PAGE_NOT_COMPLETE; 
							TRACE((TRACE_INFO,_F_,"Essai %d dans %d secondes",gptActions[i].iNbEssais,WAIT_IF_SSO_PAGE_NOT_COMPLETE));
						}
						else 
						{
							// bon, �a fait un paquet de fois qu'on r�essaie... c'est surement un cas 
							// ou titre+URL ne permettent pas de diff�rencier la page de login des autres
							// Du coup, inutile de s'acharner, on retente + tard
							// gptActions[i].iNbEssais=0; // r�arme le compteur pour la prochaine tentative
							// A VOIR : je pense qu'il vaut mieux ne pas r�armer le compteur
							//         comme �a on continue sur un rythme d'une fois tous les pas souvent
							gptActions[i].iWaitFor=WAIT_IF_SSO_NOK; 
							TRACE((TRACE_INFO,_F_,"Prochain essai dans %d secondes",WAIT_IF_SSO_NOK));
						}
					}
					break;
				default: ;
			} // switch
		}
		// 0.80 : update la config sur le serveur si autoris� par l'utilisateur
		// 0.82 : le timer doit plut�t �tre r�arm� quand on a fini l'�num�ration des fen�tres -> voir TimerProc()
		//        en plus ici c'�tait dangereux : un goto suite ou end passait outre le r�armement !!!
		//        et je me demande si �a ne pouvait pas cr�er des cas de r�entrance qui auraient eu pour cons�quence
		//        que le timer ne soit jamais r�arm�...
		// giTimer=SetTimer(NULL,0,500,TimerProc);
suite:
		// eh oui, il faut lib�rer pszURL... Sinon, vous croyez vraiment que 
		// j'aurais fait ce "goto suite", alors que continue me tendait les bras ?
		if (pszURL!=NULL) { free(pszURL); pszURL=NULL; }
		if (pszURL2!=NULL) { free(pszURL2); pszURL2=NULL; }
		if (pDocument!= NULL) { pDocument->Release(); pDocument=NULL; }
	}
end:
	if (pszChromePopupURL!=NULL) free(pszChromePopupURL); 
	if (pszURL!=NULL) free(pszURL); 
	if (pszURL2!=NULL) free(pszURL2); // 1.12B2-AC-TIE4
	if (pDocument!= NULL) pDocument->Release();
	return TRUE;
}

//-----------------------------------------------------------------------------
// LoadIcons()
//-----------------------------------------------------------------------------
// Chargement de toutes les icones et pointeurs de souris pour l'IHM
//-----------------------------------------------------------------------------
// [rc] : 0 si OK
//-----------------------------------------------------------------------------
static int LoadIcons(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int len;
	char szLogoFilename[_MAX_PATH+1];
	ghIconAltTab=(HICON)LoadImage(ghInstance,MAKEINTRESOURCE(IDI_LOGO),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
	if (ghIconAltTab==NULL) goto end;
	ghIconSystrayActive=(HICON)LoadImage(ghInstance, gbAdmin ? MAKEINTRESOURCE(IDI_SYSTRAY_ADMIN_ACTIVE) : MAKEINTRESOURCE(IDI_SYSTRAY_ACTIVE),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	if (ghIconSystrayActive==NULL) goto end;
	ghIconSystrayInactive=(HICON)LoadImage(ghInstance, gbAdmin ? MAKEINTRESOURCE(IDI_SYSTRAY_ADMIN_INACTIVE) : MAKEINTRESOURCE(IDI_SYSTRAY_INACTIVE), IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	if (ghIconSystrayInactive==NULL) goto end;
	ghIconLoupe = (HICON)LoadImage(ghInstance,MAKEINTRESOURCE(IDI_LOUPE),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_LOADTRANSPARENT);
	if (ghIconLoupe == NULL) goto end;
	ghIconHelp = (HICON)LoadImage(ghInstance,MAKEINTRESOURCE(IDI_HELP),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_LOADTRANSPARENT);
	if (ghIconHelp == NULL) goto end;
	ghLogo = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_LOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (ghLogo == NULL) goto end;

	// ISSUE#219
	ghLogoFondBlanc50=NULL;
	ghLogoFondBlanc90=NULL;
	len=GetCurrentDirectory(_MAX_PATH-10,szLogoFilename);
	if (len!=0)
	{
		if (szLogoFilename[len-1]!='\\') { strcat_s(szLogoFilename,_MAX_PATH+1,"\\"); }
		strcat_s(szLogoFilename,_MAX_PATH+1,"swssologo50.bmp");
		ghLogoFondBlanc50 = (HICON)LoadImage(NULL, szLogoFilename, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	}
	len=GetCurrentDirectory(_MAX_PATH-10,szLogoFilename);
	if (len!=0)
	{
		if (szLogoFilename[len-1]!='\\') { strcat_s(szLogoFilename,_MAX_PATH+1,"\\"); }
		strcat_s(szLogoFilename,_MAX_PATH+1,"swssologo90.bmp");
		ghLogoFondBlanc90 = (HICON)LoadImage(NULL, szLogoFilename, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	}
	if (ghLogoFondBlanc50==NULL)
	{
		ghLogoFondBlanc50 = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_LOGO_FONDBLANC50), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		if (ghLogoFondBlanc50==NULL) goto end;
	}
	if (ghLogoFondBlanc90==NULL)
	{
		ghLogoFondBlanc90 = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_LOGO_FONDBLANC90), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		if (ghLogoFondBlanc90 == NULL) goto end;
	}

	ghLogoExclamation = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_EXCLAMATION), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (ghLogoExclamation == NULL) goto end;
	ghLogoQuestion = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDB_QUESTION), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (ghLogoQuestion == NULL) goto end;
	ghCursorHand = (HCURSOR)LoadImage(ghInstance,
					MAKEINTRESOURCE(IDC_CURSOR_HAND),
					IMAGE_CURSOR,
					0,
					0,
					LR_DEFAULTSIZE);
	if (ghCursorHand==NULL) goto end;
	ghCursorWait=LoadCursor(NULL, IDC_WAIT); 
	if (ghCursorWait==NULL) goto end;
	ghImageList=ImageList_LoadBitmap(ghInstance,MAKEINTRESOURCE(IDB_TVIMAGES),16,5,RGB(255,0,255));
	if (ghImageList==NULL) goto end;
	if (gbAdmin) ghRedBrush=CreateSolidBrush(RGB(255,0,0));

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// UnloadIcons()
//-----------------------------------------------------------------------------
// D�chargement de toutes les icones et pointeurs de souris
//-----------------------------------------------------------------------------
static void UnloadIcons(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	if (ghIconAltTab!=NULL) { DestroyIcon(ghIconAltTab); ghIconAltTab=NULL; }
	if (ghIconSystrayActive!=NULL) { DestroyIcon(ghIconSystrayActive); ghIconSystrayActive=NULL; }
	if (ghIconSystrayInactive!=NULL) { DestroyIcon(ghIconSystrayInactive); ghIconSystrayInactive=NULL; }
	if (ghIconLoupe != NULL) { DestroyIcon(ghIconLoupe); ghIconLoupe = NULL; }
	if (ghIconHelp != NULL) { DestroyIcon(ghIconHelp); ghIconHelp = NULL; }
	if (ghLogo != NULL) { DeleteObject(ghLogo); ghLogo = NULL; }
	if (ghLogoFondBlanc50!=NULL) { DeleteObject(ghLogoFondBlanc50); ghLogoFondBlanc50=NULL; }
	if (ghLogoFondBlanc90!=NULL) { DeleteObject(ghLogoFondBlanc90); ghLogoFondBlanc90=NULL; }
	if (ghLogoExclamation != NULL) { DeleteObject(ghLogoExclamation); ghLogoExclamation = NULL; }
	if (ghLogoQuestion != NULL) { DeleteObject(ghLogoQuestion); ghLogoQuestion = NULL; }
	if (ghCursorHand != NULL) { DestroyCursor(ghCursorHand); ghCursorHand = NULL; }
	if (ghCursorWait!=NULL) { DestroyCursor(ghCursorWait); ghCursorWait=NULL; }
	if (ghImageList!=NULL) ImageList_Destroy(ghImageList);
	if (ghRedBrush!=NULL) { DeleteObject(ghRedBrush); ghRedBrush=NULL; }
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// SimplePwdChoiceDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de choix simplifi�e de strat�gie de mot de passe
// (fen�tre ouverte lorsqu'aucun fichier de config n'est trouv�)
//-----------------------------------------------------------------------------
static int CALLBACK SimplePwdChoiceDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champs de saisie
			//SendMessage(GetDlgItem(w,TB_PWD1),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			//SendMessage(GetDlgItem(w,TB_PWD2),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			SendMessage(GetDlgItem(w,TB_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_PWD2),EM_LIMITTEXT,LEN_PWD,0);
			// titres en gras
			SetTextBold(w,TX_FRAME);
			// ISSUE#146
			if (*gszWelcomeMessage != 0) SetDlgItemText(w,TX_FRAME1,gszWelcomeMessage);
			// policies
			if (!gbEnableOption_SavePassword) EnableWindow(GetDlgItem(w,CK_SAVE),FALSE);
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
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					char szPwd1[LEN_PWD+1];
					GetDlgItemText(w,TB_PWD1,szPwd1,sizeof(szPwd1));
					// Password Policy
					if (!IsPasswordPolicyCompliant(szPwd1))
					{
						MessageBox(w,gszPwdPolicy_Message,"swSSO",MB_OK | MB_ICONEXCLAMATION);
					}
					else
					{
						giPwdProtection=PP_ENCRYPTED;
						// g�n�re le sel qui sera pris en compte pour la d�rivation de la cl� AES et le stockage du mot de passe
						swGenPBKDF2Salt();
						swCryptDeriveKey(szPwd1,ghKey1);
						StoreMasterPwd(szPwd1);
						RecoveryChangeAESKeyData(ghKey1);
						// inscrit la date de dernier changement de mot de passe dans le .ini
						// cette valeur est chiffr� par le (nouveau) mot de passe et �crite seulement si politique mdp d�finie
						SaveMasterPwdLastChange();
						if (IsDlgButtonChecked(w,CK_SAVE)==BST_CHECKED)
						{
							gbRememberOnThisComputer=TRUE;
							DPAPIStoreMasterPwd(szPwd1);
						}
						SecureZeroMemory(szPwd1,strlen(szPwd1));
						SaveConfigHeader();
						EndDialog(w,IDOK);
					}
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_PWD1:
				case TB_PWD2:
				{
					char szPwd1[LEN_PWD+1];
					char szPwd2[LEN_PWD+1];
					int len1,len2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						len1=GetDlgItemText(w,TB_PWD1,szPwd1,sizeof(szPwd1));
						len2=GetDlgItemText(w,TB_PWD2,szPwd2,sizeof(szPwd2));
						if (len1==len2 && len1!=0 && strcmp(szPwd1,szPwd2)==0)
							EnableWindow(GetDlgItem(w,IDOK),TRUE);
						else
							EnableWindow(GetDlgItem(w,IDOK),FALSE);
					}
					break;
				}
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
// PwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de saisie du mot de passe maitre
//-----------------------------------------------------------------------------
static int CALLBACK PwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			BOOL bUseDPAPI;
			// Modifie le texte pour demander le mot de passe WIndows
			if (giPwdProtection==PP_WINDOWS) SetDlgItemText(w,TX_FRAME,GetString(IDS_PLEASE_ENTER_WINDOWS_PASSWORD));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			gwAskPwd=w;
			// init champ de saisie
			//SendMessage(GetDlgItem(w,TB_PWD),EM_SETPASSWORDCHAR,(WPARAM)'*',0);
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			// policies
			bUseDPAPI=(BOOL)lp;
			if (!gbEnableOption_SavePassword || !bUseDPAPI || giPwdProtection==PP_WINDOWS) ShowWindow(GetDlgItem(w,CK_SAVE),SW_HIDE);
			// Compl�ment ISSUE#136 : ne pas afficher le bouton mdp oubli� si synchro mdp Windows activ�e
			if (giPwdProtection==PP_WINDOWS) ShowWindow(GetDlgItem(w,PB_MDP_OUBLIE),SW_HIDE);
			// 0.81 : centrage si parent!=NULL
			if (GetParent(w)!=NULL)
			{
				int cx;
				int cy;
				RECT rect,rectParent;
				cx = GetSystemMetrics( SM_CXSCREEN );
				cy = GetSystemMetrics( SM_CYSCREEN );
				GetWindowRect(w,&rect);
				GetWindowRect(GetParent(w),&rectParent);
				SetWindowPos(w,NULL,rectParent.left+((rectParent.right-rectParent.left)-(rect.right-rect.left))/2,
									rectParent.top+ ((rectParent.bottom-rectParent.top)-(rect.bottom-rect.top))/2,
									0,0,SWP_NOSIZE | SWP_NOZORDER);
			}
			else
			{
				SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
			}
			// ISSUE#136 : nouveau bouton mdp oubli� (masqu� si pas de recouvrement d�fini ou si askpwd depuis loupe AppNsites)
			if (gpRecoveryKeyValue==NULL || *gszRecoveryInfos==0 || !bUseDPAPI)
			{
				ShowWindow(GetDlgItem(w,PB_MDP_OUBLIE),SW_HIDE);
			}
			// ISSUE#181
			if (giPwdProtection==PP_WINDOWS && !gAESKeyInitialized[ghKey1]) 
			{
				SetWindowPos(GetDlgItem(w,TX_FRAME),NULL,60,10,330,30,SWP_NOZORDER);
				SetDlgItemText(w,TX_FRAME,GetString(IDS_DECOUPLAGE_WINDOWS));
			}
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
					char szPwd[LEN_PWD+1];
					GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
					if (giPwdProtection==PP_WINDOWS)
					{
						if (gAESKeyInitialized[ghKey1]!=NULL) // Cas de la demande de mot de passe "loupe" (TogglePasswordField) ou du d�verrouillage
						{
							swCryptDeriveKey(szPwd,ghKey2);
							SecureZeroMemory(szPwd,strlen(szPwd));
							if (ReadVerifyCheckSynchroValue(ghKey2)==0)
							{
								EndDialog(w,IDOK);
							}
							else
							{
								MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_OK | MB_ICONEXCLAMATION);
								if (giBadPwdCount>5) PostQuitMessage(-1);
								SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
							}
						}
						else // cas du d�couplage (ISSUE#181)
						{
							swCryptDeriveKey(szPwd,ghKey2);
							if (ReadVerifyCheckSynchroValue(ghKey2)==0)
							{
								giPwdProtection=PP_ENCRYPTED;
								StoreMasterPwd(szPwd);
								swCryptDeriveKey(szPwd,ghKey1);
								// enregistrement des infos de recouvrement dans swSSO.ini (mdp maitre + code RH)Kpub
								RecoveryChangeAESKeyData(ghKey1);
								// inscrit la date de dernier changement de mot de passe dans le .ini
								// cette valeur est chiffr� par le (nouveau) mot de passe et �crite seulement si politique mdp d�finie
								SaveMasterPwdLastChange();
								SecureZeroMemory(szPwd,strlen(szPwd));
								EndDialog(w,IDOK);
							}
							else
							{
								SecureZeroMemory(szPwd,strlen(szPwd));
								MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_OK | MB_ICONEXCLAMATION);
								SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
								if (giBadPwdCount>5) 
								{
									PostQuitMessage(-1);
									EndDialog(w,IDCANCEL);
								}
							}
						}
					}
					else if (CheckMasterPwd(szPwd)==0)
					{
						if (IsDlgButtonChecked(w,CK_SAVE)==BST_CHECKED)
						{
							gbRememberOnThisComputer=TRUE;
							DPAPIStoreMasterPwd(szPwd);
						}
						// ISSUE#342 Si mode admin, login de l'admin sur le serveur (non bloquant + message d'erreur dans la fonction ServerAdminLogin)
						if (gbAdmin && !gbNoMasterPwd) ServerAdminLogin(w,gszUserName,szPwd);

						swCryptDeriveKey(szPwd,ghKey1);
						SecureZeroMemory(szPwd,strlen(szPwd));
						// 0.90 : si une cl� de recouvrement existe et les infos de recouvrement n'ont pas encore
						//        �t� enregistr�es dans le .ini (cas de la premi�re utilisation apr�s d�ploiement de la cl�
						SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
						RecoveryFirstUse(w,ghKey1);
						EndDialog(w,IDOK);
					}
					else
					{
						SecureZeroMemory(szPwd,strlen(szPwd));
						// 0.93B1 : log authentification primaire �chou�e
						if (gbSSOActif)
							swLogEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_LOGIN_ERROR,NULL,NULL,NULL,NULL,0);
						else
							swLogEvent(EVENTLOG_WARNING_TYPE,MSG_UNLOCK_BAD_PWD,NULL,NULL,NULL,NULL,0);

						SendDlgItemMessage(w,TB_PWD,EM_SETSEL,0,-1);

						// ISSUE#136 : le message en cas d'erreur devient g�n�rique suite � l'ajout du bouton mdp oubli�
						MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_OK | MB_ICONEXCLAMATION);
						if (giBadPwdCount>5) PostQuitMessage(-1);
						SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
					}
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_PWD:
				{
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szPwd[LEN_PWD+1];
						int len;
						len=GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
						EnableWindow(GetDlgItem(w,IDOK),len==0 ? FALSE : TRUE);
					}
					break;
				}
				case PB_MDP_OUBLIE: // ISSUE#136
				{
					if (RecoveryChallenge(w)==0)
					{
						EndDialog(w,-2);
						//PostQuitMessage(-1); ISSUE#147
					}
					SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
					break;
				}
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


//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// AskPwd()
//-----------------------------------------------------------------------------
// Demande et v�rifie le mot de passe de l'utilisateur.
//-----------------------------------------------------------------------------
// [rc] : 0 si OK, -1 si mauvais mot de passe, -2 mot de passe oubli�
//-----------------------------------------------------------------------------
int AskPwd(HWND wParent,BOOL bUseDPAPI)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int ret=-1;
	int rc;
	char szPwd[LEN_PWD+1];
	time_t tNow;

	if (giBadPwdCount>5) 
	{
		PostQuitMessage(-1);
		goto end;
	}

	// 0.65 : anti r�-entrance
	if (gwAskPwd!=NULL) 
	{
		SetForegroundWindow(gwAskPwd);
		goto end;
	}

	// mode de fonctionnement sans mot de passe
	if (gbNoMasterPwd && gcszK1[0]!='1')
	{
		if (gbAdmin) // si d�fini, demande le mot de passe admin, sinon demande de le d�finir
		{
			if (IsAdminPwdSet())
			{
				if (AskAdminPwd()!=0) goto end;
			}
			else
			{
				if (SetAdminPwd()!=0) goto end;
			}
		}
		char szTemp[LEN_PWD+1];
		SecureZeroMemory(szTemp,LEN_PWD+1);
		memcpy(szTemp,gcszK1,8);
		memcpy(szTemp+8,gcszK2,8);
		memcpy(szTemp+16,gcszK3,8);
		memcpy(szTemp+24,gcszK4,8);

		// ISSUE#262 : force le username en minuscule
		//memcpy(szTemp+32,gszUserName,strlen(gszUserName)<16?strlen(gszUserName):16);
		int i,len;
		len=min(strlen(gszUserName),16);
		for (i=0;i<len;i++) szTemp[32+i]=(char)tolower(gszUserName[i]);
		//TRACE((TRACE_PWD,_F_,"sel=%s",szTemp));

		if (CheckMasterPwd(szTemp)==0)
		{
			swCryptDeriveKey(szTemp,ghKey1);
			ret=0;
		}
		else
		{
			swLogEvent(EVENTLOG_ERROR_TYPE,MSG_GENERIC_START_ERROR,NULL,NULL,NULL,NULL,0);
			char szErrMsg[1024+1];
			strcpy_s(szErrMsg,sizeof(szErrMsg),GetString(IDS_GENERIC_STARTING_ERROR));
			MessageBox(NULL,szErrMsg,GetString(IDS_MESSAGEBOX_TITLE),MB_OK | MB_ICONSTOP);
		}
		SecureZeroMemory(szTemp,LEN_PWD+1);
		goto end;
	}	

	//0.76 : DPAPI
	if (bUseDPAPI)
	{
		rc=DPAPIGetMasterPwd(szPwd);
		if (rc==0)
		{
			if (CheckMasterPwd(szPwd)==0)
			{
				// ISSUE#342 Si mode admin, login de l'admin sur le serveur (non bloquant + message d'erreur dans la fonction ServerAdminLogin)
				if (gbAdmin && !gbNoMasterPwd) ServerAdminLogin(wParent,gszUserName,szPwd);

				// 0.90 : si une cl� de recouvrement existe et les infos de recouvrement n'ont pas encore
				//        �t� enregistr�e dans le .ini (cas de la premi�re utilisation apr�s d�ploiement de la cl�
				swCryptDeriveKey(szPwd,ghKey1);
				SecureZeroMemory(szPwd,strlen(szPwd));
				gbRememberOnThisComputer=TRUE;
				ret=0;
				goto end;
			}
		}
	}

	// ISSUE#309 : ne redemande pas le mot de passe s'il a �t� demand� r�cemment (selon config)
	if (gtLastAskPwd!=0)
	{
		time(&tNow);
		if (tNow-gtLastAskPwd < (giMasterPwdExpiration*60))
		{ 
			TRACE((TRACE_INFO,_F_,"Ne redemande pas le mot de passe")); 
			ret=0; 
			goto end; 
		}
	}

	rc=DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_PWD),wParent,PwdDialogProc,(LPARAM)bUseDPAPI);
	gwAskPwd=NULL;
	if (rc==-2) { ret=-2; goto end; } // ISSUE#147
	if (rc!=IDOK) goto end;
	time(&gtLastAskPwd); // ISSUE#309 : mot de passe OK, on m�morise l'heure
	giBadPwdCount=0;
	ret=0;
end:
	TRACE((TRACE_LEAVE,_F_, "ret=%d",ret));
	return ret;
}

//-----------------------------------------------------------------------------
// CheckIfUpgraded()
//-----------------------------------------------------------------------------
void CheckIfUpgraded(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char bufRequest[1024];
	char bufResponse[64];
	DWORD dwLenResponse;

	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:NEWVERS",11);
	if (swPipeWrite(bufRequest,11,bufResponse,sizeof(bufResponse),&dwLenResponse)==0) 
	{
		if (memcmp(bufResponse,"YES",3)==0)
		{
			NOTIFYICONDATA nid;
			ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
			nid.cbSize=sizeof(NOTIFYICONDATA);
			nid.hWnd=gwMain;
			nid.uID=0; 
			nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
			nid.uTimeout=2000;
			strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),GetString(IDS_NOTIFY_TITLE_UPGRADED));
			if (strcmp(gcszCurrentBeta,"0000")==0)
			{
				sprintf_s(bufRequest,sizeof(bufRequest),GetString(IDS_NOTIFY_TEXT_UPGRADED),gcszCurrentVersion[0],gcszCurrentVersion[1],gcszCurrentVersion[2]);
			}
			else
			{
				sprintf_s(bufRequest,sizeof(bufRequest),GetString(IDS_NOTIFY_TEXT_UPGRADED_BETA),gcszCurrentBeta[0],gcszCurrentBeta[1],gcszCurrentBeta[2],gcszCurrentBeta[3]);
			}
			strcpy_s(nid.szInfo,sizeof(nid.szInfo),bufRequest);
			Shell_NotifyIcon(NIM_MODIFY, &nid); 
		}
	}
	else
	{ 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()"));
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}


//-----------------------------------------------------------------------------
// WinMain()
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInstance);
	
	int rc;
	int iError=0; // v0.88 : message d'erreur au d�marrage
	HRESULT hr;
	MSG msg;
	int len;
	int rcSystray=-1;
    HANDLE hMutex=NULL;
	BOOL bLaunchApp=FALSE;
	BOOL bConnectApp=FALSE;
	BOOL bAlreadyLaunched=FALSE;
	OSVERSIONINFO osvi;
	BOOL b64=false;
	BOOL bMigrationWindowsSSO=FALSE;
	// BOOL bForcePwdChangeNow=FALSE; // supprim� en 1.15, �tait toujours FALSE !
	BOOL bQuit=FALSE;
	int iWaitBeforeKill=5000; //en ms
	
	// init des traces
	TRACE_OPEN();
	TRACE((TRACE_ENTER,_F_, ""));
	// init de toutes les globales
	ghInstance=hInstance;
	gptActions=NULL;
	giBadPwdCount=0;
	gbSSOActif=true;
	ghIconSystrayActive=NULL;
	ghIconSystrayInactive=NULL; 
	ghIconLoupe=NULL;
	ghCursorHand=NULL;
	ghLogo=NULL;
	ghImageList=NULL;
	guiNbWEBSSO=0;
	guiNbWINSSO=0;
	guiNbPOPSSO=0;
	giaccChildCountErrors=0;
	giaccChildErrors=0;
	giBadPwdCount=0;
	gwAskPwd=NULL; // 0.65 anti r�-entrance fen�tre saisie pwd
	time_t tNow,tLastPwdChange;
	gbRecoveryRunning=FALSE;
	gpSid=NULL;
	gpszRDN=NULL;
	gSalts.bPBKDF2PwdSaltReady=FALSE;
	gSalts.bPBKDF2KeySaltReady=FALSE;
	gLastLoginTime.wYear=0; // ISSUE#171
	giNbTranscryptError=0;
	gAESKeyInitialized[0]=FALSE;
	gAESKeyInitialized[1]=FALSE;

	ZeroMemory(&gTabLastDetect,sizeof(T_LAST_DETECT)); // pourrait contribuer � la correction de ISSUE#229

	// ligne de commande
	if (strlen(lpCmdLine)>_MAX_PATH) { iError=-1; goto end; } 
	TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));

	// enregistrement des messages pour r�ception de param�tres en ligne de commande quand swSSO est d�j� lanc�
	guiLaunchAppMsg=RegisterWindowMessage("swsso-launchapp");
	if (guiLaunchAppMsg==0)	{ TRACE((TRACE_ERROR,_F_,"RegisterWindowMessage(swsso-launchapp)=%d",GetLastError())); }
	else { TRACE((TRACE_INFO,_F_,"RegisterWindowMessage(swsso-launchapp) OK -> msg=0x%08lx",guiLaunchAppMsg)); }
	guiConnectAppMsg=RegisterWindowMessage("swsso-connectapp");
	if (guiConnectAppMsg==0) { TRACE((TRACE_ERROR,_F_,"RegisterWindowMessage(swsso-connectapp)=%d",GetLastError())); }
	else { TRACE((TRACE_INFO,_F_,"RegisterWindowMessage(swsso-connectapp) OK -> msg=0x%08lx",guiConnectAppMsg)); }
	guiStandardQuitMsg=RegisterWindowMessage("swsso-quit");
	if (guiStandardQuitMsg==0) { TRACE((TRACE_ERROR,_F_,"RegisterWindowMessage(swsso-quit)=%d",GetLastError())); }
	else { TRACE((TRACE_INFO,_F_,"RegisterWindowMessage(swsso-quit) OK -> msg=0x%08lx",guiStandardQuitMsg)); }
	guiAdminQuitMsg=RegisterWindowMessage("swssoadmin-quit");
	if (guiAdminQuitMsg==0) { TRACE((TRACE_ERROR,_F_,"RegisterWindowMessage(swssoadmin-quit)=%d",GetLastError())); }
	else { TRACE((TRACE_INFO,_F_,"RegisterWindowMessage(swssoadmin-quit) OK -> msg=0x%08lx",guiAdminQuitMsg)); }
	
	// 0.92 : r�cup�ration version OS pour traitements sp�cifiques Vista et/ou Seven
	// Remarque : pas tout � fait juste, mais convient pour les postes de travail. A revoir pour serveurs.
	ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi))
	{
		TRACE((TRACE_DEBUG,_F_,"dwMajorVersion=%d dwMinorVersion=%d",osvi.dwMajorVersion,osvi.dwMinorVersion));
		if (osvi.dwMajorVersion==6 && osvi.dwMinorVersion==2) giOSVersion=OS_WINDOWS_8;
		else if (osvi.dwMajorVersion==6 && osvi.dwMinorVersion==1) giOSVersion=OS_WINDOWS_7;
		else if (osvi.dwMajorVersion==6 && osvi.dwMinorVersion==0) giOSVersion=OS_WINDOWS_VISTA;
		else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion==1) giOSVersion=OS_WINDOWS_XP;
	}
	IsWow64Process(GetCurrentProcess(),&b64);
	giOSBits=b64?OS_64:OS_32;
	TRACE((TRACE_INFO,_F_,"giOSVersion=%d giOSBits=%d",giOSVersion,giOSBits));

	// ISSUE#227 : quitter
	if (strnistr(lpCmdLine,"/quit:",-1)!=NULL || strnistr(lpCmdLine,"-quit:",-1)!=NULL)
	{
		char *p=strnistr(lpCmdLine,"quit:",-1);
		char szWaitBeforeKill[2+1]={0,0,0};
		if (p!=NULL)
		{
			if (*(p+5)!=0) 
			{
				szWaitBeforeKill[0]=*(p+5);
				if (*(p+6)!=0) szWaitBeforeKill[1]=*(p+6);
			}
		}
		bQuit=TRUE;
		iWaitBeforeKill=atoi(szWaitBeforeKill)*1000;
		TRACE((TRACE_INFO,_F_,"Quit iWaitBeforeKill=%d",iWaitBeforeKill));
	}
	else if (strnistr(lpCmdLine,"/quit",-1)!=NULL || strnistr(lpCmdLine,"-quit",-1)!=NULL) 
	{ 
		bQuit=TRUE;
		TRACE((TRACE_INFO,_F_,"Quit iWaitBeforeKill=%d",iWaitBeforeKill));
	}

	// 0.91 : si la ligne de commande contient le param�tre -launchapp, ouvre la fen�tre de lancement d'appli
	//        soit en postant � message � swsso si d�j� lanc�, soit par appel � ShowAppNsites en fin de WinMain()
	if (strnistr(lpCmdLine,"-launchapp",-1)!=NULL && guiLaunchAppMsg!=0) 
	{
		bLaunchApp=TRUE;
		// supprime le param�tre -launchapp de la ligne de commande pour traitement du path �ventuellement sp�cifi� pour le .ini
		lpCmdLine+=strlen("-launchapp");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	// 0.97 : si la ligne de commande contient le param�tre -connectapp, ouvre la fen�tre de lancement d'appli
	//        soit en postant � message � swsso si d�j� lanc�, soit par appel � ShowAppNsites en fin de WinMain()
	if (strnistr(lpCmdLine,"-connectapp",-1)!=NULL && guiConnectAppMsg!=0) 
	{
		bConnectApp=TRUE;
		// supprime le param�tre -connectapp de la ligne de commande 
		// pour traitement du path �ventuellement sp�cifi� pour le .ini
		lpCmdLine+=strlen("-connectapp");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	// ISSUE#205 : mode admin
	if (strnistr(lpCmdLine,"/admin",-1)!=NULL) 
	{
		gbAdmin=TRUE;
		// supprime le param�tre de la ligne de commande 
		// pour traitement du path �ventuellement sp�cifi� pour le .ini
		lpCmdLine+=strlen("/admin");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	else if (strnistr(lpCmdLine,"-admin",-1)!=NULL) 
	{
		gbAdmin=TRUE;
		// supprime le param�tre de la ligne de commande 
		// pour traitement du path �ventuellement sp�cifi� pour le .ini
		lpCmdLine+=strlen("-admin");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	
	// ISSUE#321 : si client d'admin, ferme la trace standard et ouvre la trace admin
	if (gbAdmin)
	{
		TRACE((TRACE_INFO,_F_,"Fermeture trace, ouverture trace admin"));
		TRACE_CLOSE();
		TRACE_OPEN();
	}
	
	// 0.42 v�rif pas d�j� lanc�
	if (gbAdmin)
		hMutex=CreateMutex(NULL,TRUE,"swSSO.exe[admin]");
	else
		hMutex=CreateMutex(NULL,TRUE,"swSSO.exe");
	bAlreadyLaunched=(GetLastError()==ERROR_ALREADY_EXISTS);

	if (bQuit && !bAlreadyLaunched)
	{
		TRACE((TRACE_INFO,_F_,"Demande d'arret mais pas demarre (gbAdmin=%d)",gbAdmin));
		goto end;
	}

	if (bAlreadyLaunched)
	{
		TRACE((TRACE_INFO,_F_,"Une instance est deja lancee"));
		if (bLaunchApp)
		{
			TRACE((TRACE_INFO,_F_,"Demande � l'instance pr�c�dente d'ouvrir la fenetre de lancement d'applications"));
			PostMessage(HWND_BROADCAST,guiLaunchAppMsg,0,0);
		}
		if (bConnectApp)
		{
			TRACE((TRACE_INFO,_F_,"Demande � l'instance pr�c�dente de connecter l'application en avant plan"));
			PostMessage(HWND_BROADCAST,guiConnectAppMsg,0,0);
		}
		if (bQuit)
		{
			TRACE((TRACE_INFO,_F_,"Demande � l'instance pr�c�dente de s'arr�ter gbAdmin=%d",gbAdmin));
			PostMessage(HWND_BROADCAST,gbAdmin?guiAdminQuitMsg:guiStandardQuitMsg,0,0);
			// attend 5 secondes et si toujours l� on le shoote
			Sleep(iWaitBeforeKill);
			KillswSSO();
		}
		goto end;
	}

	// ISSUE#364 : lecture en priorit� du chemin du .ini en base de registre
	LoadIniPathNamePolicy();
	if (*gpszIniPathName!=0)
	{
		ExpandFileName(gpszIniPathName,gszCfgFile,_SW_MAX_PATH+1);
	}
	else if (*lpCmdLine==0) // si pas de .ini pass� en param�tre, on cherche dans le r�p courant
	{
		len=GetCurrentDirectory(_SW_MAX_PATH-10,gszCfgFile);
		if (len==0) { iError=-1; goto end; }
		if (gszCfgFile[len-1]!='\\') { gszCfgFile[len]='\\'; len++; }
		strcpy_s(gszCfgFile+len,_SW_MAX_PATH+1,"swSSO.ini");
	}
	else // sinon utilie le chemin pass� en param�tre
	{
		ExpandFileName(lpCmdLine,gszCfgFile,_SW_MAX_PATH+1); // ISSUE#104 et ISSUE#109
	}
	if (gbExitIfNetworkUnavailable)
	{
		// ISSUE#365 : teste le chemin du fichier .ini. Si erreur diff�rente de "fichier non trouv�", 
		// on suppose que c'est un pb d'acc�s r�seau et on sort
		HANDLE hfTest;
		hfTest=CreateFile(gszCfgFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if (hfTest==INVALID_HANDLE_VALUE)
		{
			DWORD dwError=GetLastError();
			TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",gszCfgFile,dwError));
			if (dwError!=ERROR_FILE_NOT_FOUND && dwError!=ERROR_PATH_NOT_FOUND)
			{
				TRACE((TRACE_ERROR,_F_,"Chemin d'acc�s au .ini introuvable (probl�me r�seau ?), on quitte sans message"));
				iError=-9;
				goto end;
			}
		}
		else
		{
			CloseHandle(hfTest);
		}
	}
	// Pas simple de le faire plus t�t... ce veut dire que si des messages sont affich�s avant 
	// ils seront dans la langue de l'OS et pas dans la langue choisie par l'utilisateur
	GetOSLanguage(); // r�cup�re la langue de l'OS
	giLanguage=GetPrivateProfileInt("swSSO","Language",0,gszCfgFile);
	if (giLanguage!=0) SetLanguage(); // applique ce que l'utilisateur a d�fini

	// inits Window et COM
	InitCommonControls();
	ghrCoIni=CoInitialize(NULL);
	if (FAILED(ghrCoIni)) 
	{
		TRACE((TRACE_ERROR,_F_,"CoInitialize hr=0x%08lx",ghrCoIni));
		iError=-1;
		goto end;
	}

	hr=CoCreateInstance(CLSID_CUIAutomation, NULL,CLSCTX_INPROC_SERVER, IID_IUIAutomation,(LPVOID*)&gpIUIAutomation);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(CLSID_CUIAutomation)=0x%08lx",hr)); gpIUIAutomation=NULL; }
	// si failed, ne doit pas sortir, on est peut-�tre sur XP ou 2003...

	// r�cup�re username, computername, SID et domaine
	if (GetUserDomainAndComputer()!=0) { iError=-1; goto end; }

	// chargement ressources
	if (LoadIcons()!=0) { iError=-1; goto end; }

	// initialisation du module crypto
	if (swCryptInit()!=0) { iError=-1; goto end; }
	if (swProtectMemoryInit()!=0) { iError=-1; goto end; }
	
	// chargement des policies (password, global et enterprise)
	LoadPolicies();

	// cr�ation fen�tre technique (r�ception des messages) -- ISSUE#175 (�tait plus bas avant, juste avant le WTSRegister)
	gwMain=CreateMainWindow();
	if (gwMain==NULL) { iError=-1; goto end; }

	// cr�ation icone systray -- ISSUE#175 (�tait plus bas avant, juste apr�s le WTSRegister)
	rcSystray=CreateSystray(gwMain);

	// lecture du header de la config (=lecture section swSSO)
	if (GetConfigHeader()!=0) { iError=-2; goto end; }

	// ISSUE#318
	LoadGlobalOrDomainPolicies(gszDomainLabel);

	if (*gszCfgVersion==0 || giPwdProtection==0) // version <0.50 ou premier lancement... // ISSUE#295
	{
		// ISSUE#260 : cr�e le r�pertoire qui doit contenir le fichier .ini
		char *p=NULL;
		p=strrchr(gszCfgFile,'\\');
		if (p!=NULL)
		{
			*p=0;
			TRACE((TRACE_DEBUG,_F_,"Creation dossier pour le .ini : %s",gszCfgFile));
			if (!CreateDirectory(gszCfgFile,NULL))
			{
				TRACE((TRACE_ERROR,_F_,"CreateDirectory(%s)=%d",gszCfgFile,GetLastError()));
				// on continue quand m�me, sans doute il existe d�j�, sinon pas grave �a merdera plus loin
			}
			*p='\\';
			TRACE((TRACE_DEBUG,_F_,"Chemin complet du .ini : %s",gszCfgFile));
		}

		strcpy_s(gszCfgVersion,4,gcszCfgVersion);
		if (gbPasswordChoiceLevel==PP_WINDOWS) // MODE chainage mot de passe Windows, transparent pour l'utilisateur
		{
			if (InitWindowsSSO()) goto end;
			giPwdProtection=PP_WINDOWS;
			SaveConfigHeader();
		}
		else if (!gbNoMasterPwd || gcszK1[0]=='1') // MODE mot de passe maitre : affichage message bienvenue avec d�finition du mot de passe maitre
		{
			if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_SIMPLE_PWD_CHOICE),NULL,SimplePwdChoiceDialogProc)!=IDOK) goto end;
		}
		else // mode sans mot de passe
		{
			char szTemp[LEN_PWD+1]; // 4 morceaux de cl� x 8 octets + nom d'utilisateur tronqu� � 16 octets = 48

			if (gbAdmin) // si d�fini, demande le mot de passe admin, sinon demande de le d�finir
			{
				if (IsAdminPwdSet())
				{
					if (AskAdminPwd()!=0) goto end;
				}
				else
				{
					if (SetAdminPwd()!=0) goto end;
				}
			}

			giPwdProtection=PP_ENCRYPTED;
			swGenPBKDF2Salt(); // g�n�re le sel qui sera pris en compte pour la d�rivation de la cl� AES et le stockage du mot de passe
			SecureZeroMemory(szTemp,LEN_PWD+1);
			memcpy(szTemp,gcszK1,8);
			memcpy(szTemp+8,gcszK2,8);
			memcpy(szTemp+16,gcszK3,8);
			memcpy(szTemp+24,gcszK4,8);

			// ISSUE#262 : force le username en minuscule
			//memcpy(szTemp+32,gszUserName,strlen(gszUserName)<16?strlen(gszUserName):16);
			int i,lenUserName;
			lenUserName=min(strlen(gszUserName),16);
			for (i=0;i<lenUserName;i++) szTemp[32+i]=(char)tolower(gszUserName[i]);
			//TRACE((TRACE_PWD,_F_,"sel=%s",szTemp));

			swCryptDeriveKey(szTemp,ghKey1);
			StoreMasterPwd(szTemp);
			SecureZeroMemory(szTemp,LEN_PWD+1);
			SaveConfigHeader();
		}
	}
	else // ce n'est pas le premier lancement
	{
		// force la migration en SSO Windows si configur� en base de registre
		if (gbPasswordChoiceLevel==PP_WINDOWS)
		{
			// ISSUE#307 : v�rifie si passage � nouvelle version (d�tect� via les actions de swSSOMigration � swSSOSVC, applicable au mode mot de passe Windows uniquement)
			CheckIfUpgraded();
			giPwdProtection=PP_WINDOWS;
		}
		else if (gbPasswordChoiceLevel==PP_ENCRYPTED) // ISSUE#181 (d�couplage)
		{
			giPwdProtection=PP_ENCRYPTED;
		}
askpwd:
		if (giPwdProtection==PP_ENCRYPTED) // MODE mot de passe maitre
		{
			// Regarde si l'utilisateur utilisait le couplage Windows pr�c�demment
			char szSynchroValue[192+1]; // (16+64+16)*2+1 = 193
			int lenCheckSynchro;
			lenCheckSynchro =GetPrivateProfileString("swSSO","CheckSynchro","",szSynchroValue,sizeof(szSynchroValue),gszCfgFile);
			if (lenCheckSynchro !=0) // ISSUE#181 (d�couplage) : l'utilisateur �tait en PP_WINDOWS et passe PP_ENCRYPTED
			{
				// demande le mot de passe � l'utilisateur (pas le choix, pour hasher, il faut le connaitre)
				// bidouille pour se retrouver dans le m�me cas que le clic sur la loupe et v�rifier le mot de passe windows facilement...
				giPwdProtection=PP_WINDOWS; // la remise � la bonne valeur se fait dans WM_COMMAND de PwdDialogProc
				// initialisation des sels
				if (swReadPBKDF2Salt()!=0) goto end;
				// demande du mot de passe
				rc=AskPwd(NULL,TRUE);
				if (rc!=0) goto end; // mauvais mot de passe
				// mise � jour du .ini sans oublier le sceau
				WritePrivateProfileString("swSSO","CheckSynchro",NULL,gszCfgFile); 
				WritePrivateProfileString("swSSO","pwdProtection","ENCRYPTED",gszCfgFile);
				StoreIniEncryptedHash(); // ISSUE#164
			}
			else // L'utilisateur est d�j� en mode PP_ENCRYPTED
			{
				// regarde s'il y a une r�init de mdp en cours
				int ret=RecoveryResponse(NULL);
				if (ret==0) // il y a eu une r�init et �a a bien march� :-)
				{ 
					WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); // �tait plus bas avant (ISSUE#276)
					gbRecoveryRunning=TRUE; // transchiffrement plus tard une fois que les configs sont charg�es en m�moire
				}
				else if (ret==-2)  // pas de r�init
				{
					rc=AskPwd(NULL,TRUE);
					if (rc==-1) goto end; // mauvais mot de passe
					else if (rc==-2) goto askpwd; // l'utilisateur a cliqu� sur mot de passe oubli�
				}
				else if (ret==-1 || ret==-3)  // erreur ou format de response incorrect
				{
					goto askpwd;
				}
				else if (ret==-5)  // l'utilisateur a demand� de reg�n�rer le challenge (ISSUE#121)
				{
					RecoveryChallenge(NULL);
					goto askpwd; // je sais, c'est moche, mais c'est tellement plus simple...
				}
				else // il y a eu une r�init et �a n'a pas march� :-(
				{
					goto end;
				}
			}
		}
		else if (giPwdProtection==PP_WINDOWS) // MODE mot de passe Windows
		{
			char szConfigHashedPwd[SALT_LEN*2+HASH_LEN*2+1];
			int lenConfigHashedPwd;
			
			// Regarde si l'utilisateur utilisait un mot de passe maitre avant de demander le couplage Windows
			GetPrivateProfileString("swSSO","pwdValue","",szConfigHashedPwd,sizeof(szConfigHashedPwd),gszCfgFile);
			TRACE((TRACE_DEBUG,_F_,"pwdValue=%s",szConfigHashedPwd));
			lenConfigHashedPwd=strlen(szConfigHashedPwd);
			if (lenConfigHashedPwd==PBKDF2_PWD_LEN*2) // L'utilisateur a encore un mot de passe maitre mais doit passer en couplage Windows
			{
				char szPwd[LEN_PWD+1];
				int ret=DPAPIGetMasterPwd(szPwd);
				SecureZeroMemory(szPwd,LEN_PWD+1); // pas besoin du mdp, c'�tait juste pour savoir si la cl� �tait pr�sente et valide
				if (ret!=0)
				{
					// N'affiche pas le message qui indique que le mot de passe maitre va �tre demand� une derni�re fois
					// si jamais l'utilisateur avait enregistr� son mot de passe
					MessageBox(NULL,GetString(IDS_INFO_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONINFORMATION);
				}
				giPwdProtection=PP_ENCRYPTED; // bidouille pour avoir le bon message dans la fen�tre AskPwd...
				if (AskPwd(NULL,TRUE)!=0) goto end;
				giPwdProtection=PP_WINDOWS;
				bMigrationWindowsSSO=TRUE; // On note de faire la migration (se fait plus tard une fois les configs charg�es)
			}
			else if (lenConfigHashedPwd==0) // L'utilisateur est d�j� en mode PP_WINDOWS
			{
				// regarde s'il y a une r�init de mdp en cours
				int ret=RecoveryResponse(NULL);
				if (ret==0) // il y a eu une r�init et �a a bien march� :-)
				{ 
					WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); // �tait plus bas avant (ISSUE#276)
					gbRecoveryRunning=TRUE; // transchiffrement plus tard une fois que les configs sont charg�es en m�moire
				}
				else if (ret==-2) // pas de r�init
				{
					// ISSUE#165
					rc=CheckWindowsPwd(&bMigrationWindowsSSO);
					if (rc==-1) // erreur ou pas de recovery ou annulation de l'utilisateur dans le recovery
						goto end;
					else if(rc==-3) // l'utilisateur a cliqu� sur continuer dans le recovery
						goto askpwd;
				}
				else if (ret==-1 || ret==-3)  // erreur ou format de response incorrect
				{
					goto askpwd;
				}
				else if (ret==-5)  // ISSUE#165 // l'utilisateur a demand� de reg�n�rer le challenge (ISSUE#121)
				{
					if (RecoveryChallenge(NULL)==0)
						goto askpwd; // je sais, c'est moche, mais c'est tellement plus simple...
					else
						goto end;
				}
				else // il y a eu une r�init et �a n'a pas march� :-(
				{
					goto end;
				}
			}
			else if (lenConfigHashedPwd==296) // L'utilisateur a une vieille version de swSSO (ISSUE#172 : il faut migrer quand m�me)
			{
				char szPwd[LEN_PWD+1];
				int ret=DPAPIGetMasterPwd(szPwd);
				SecureZeroMemory(szPwd,LEN_PWD+1); // pas besoin du mdp, c'�tait juste pour savoir si la cl� �tait pr�sente et valide
				if (ret!=0)
				{
					// N'affiche pas le message qui indique que le mot de passe maitre va �tre demand� une derni�re fois
					// si jamais l'utilisateur avait enregistr� son mot de passe
					MessageBox(NULL,GetString(IDS_INFO_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONINFORMATION);
				}
				giPwdProtection=PP_ENCRYPTED; // bidouille
				if (AskPwd(NULL,TRUE)!=0) goto end;
				giPwdProtection=PP_WINDOWS;
				bMigrationWindowsSSO=TRUE; // On note de faire la migration (se fait plus tard une fois les configs charg�es)
			}
			else // L'utilisateur a un probl�me avec son .ini...
			{
				TRACE((TRACE_ERROR,_F_,"lenConfigHashedPwd=%d", lenConfigHashedPwd));
				MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_VER),"swSSO",MB_OK | MB_ICONSTOP);
				goto end;
			}
		}
		// 0.93B1 : log authentification primaire r�ussie
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_PRIMARY_LOGIN_SUCCESS,NULL,NULL,NULL,NULL,0);
	}

	// 0.80B9 : lit la config proxy pour ce poste de travail.
	// Remarque : n'est pas fait dans GetConfigHeader car on a besoin de la cl�
	//			  d�riv�e du mot de passe maitre pour d�chiffrement le mdp proxy
	GetProxyConfig(gszComputerName,&gbInternetUseProxy,gszProxyURL,gszProxyUser,gszProxyPwd);

	// allocation du tableau d'actions
	gptActions=(T_ACTION*)malloc(giMaxConfigs*sizeof(T_ACTION));
	TRACE((TRACE_DEBUG,_F_,"malloc (%d)",giMaxConfigs*sizeof(T_ACTION)));
	if (gptActions==NULL)
	{
		TRACE((TRACE_ERROR,_F_,"malloc (%d)",giMaxConfigs*sizeof(T_ACTION)));
		iError=-1;
		goto end;
	}
	// 0.92B5 : pour corriger bug cat�gories perdues en 0.92B3, LoadApplications passe APRES LoadCategories
	// lecture des cat�gories
	if (LoadCategories()!=0) { iError=-2; goto end; }
	// lecture des applications configur�es
	if (LoadApplications()==-1) { iError=-2; goto end; }

	// v�rifie la date de dernier changement de mot de passe
	// attention, comme il y a transchiffrement des id&pwd et des mdp proxy, il 
	// faut bien que ces infos aient �t� lues avant un �ventuel changement de mot de passe impos� !
	// ISSUE#145 : si jamais on est en phase de migration, il faut le faire plus tard, apr�s la migration
	if (giPwdProtection==PP_ENCRYPTED)
	{
		if (giPwdPolicy_MaxAge!=0 && !gbRecoveryRunning) // ISSUE#178 : ne pas demander le changement de mdp expir� si en cours de recouvrement !
		{
			time(&tNow);
			tLastPwdChange=GetMasterPwdLastChange();
			TRACE((TRACE_INFO,_F_,"tNow              =%ld",tNow));
			TRACE((TRACE_INFO,_F_,"tLastPwdChange    =%ld",tLastPwdChange));
			TRACE((TRACE_INFO,_F_,"diff              =%ld",tNow-tLastPwdChange));
			TRACE((TRACE_INFO,_F_,"giPwdPolicy_MaxAge=%ld",giPwdPolicy_MaxAge));
			if ((tNow-tLastPwdChange)>(giPwdPolicy_MaxAge*86400))
			{
				// impose le changement de mot de passe
				if (WindowChangeMasterPwd(TRUE)!=0) goto end;
			}
		}
	}

	gtConfigSync.iNbConfigsAdded=0;
	gtConfigSync.iNbConfigsDeleted=0;
	gtConfigSync.iNbConfigsDisabled=0;
	gtConfigSync.iNbConfigsModified=0;

	// 0.91 : propose � l'utilisateur de r�cup�rer les configurations disponibles sur le serveur
	TRACE((TRACE_DEBUG,_F_,"giNbActions=%d gbGetAllConfigsAtFirstStart=%d giDomainId=%d",giNbActions,gbGetAllConfigsAtFirstStart,giDomainId));
	if (giNbActions==0 && gbGetAllConfigsAtFirstStart) // CAS DU PREMIER LANCEMENT (ou encore aucune config enregistr�e)
	{
		// 1.03 : r�cup�re la liste des domaines
		giNbDomains=GetAllDomains(gtabDomains);
		if (giNbDomains==0)
		{ 
			MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
			goto end;
		}
		if (gbAdmin || gbInternetManualPutConfig) // 1.05 : on ne demande pas � l'admin quel est son domaine, il doit pouvoir tous les g�rer
		{
			giDomainId=-1;
			strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),"Tous");
			SaveConfigHeader();
			GetAllConfigsFromServer();
		}
		else
		{
			if (giDomainId==1 && giNbDomains!=0) // domaine non renseign� dans le .ini 
			{
				int ret= SelectDomain();
				// ret:  0 - OK, l'utilisateur a choisi, le domaine est renseign� dans giDomainId et gszDomainLabel
				//    :  1 - Il n'y avait qu'un seul domaine, l'utilisateur n'a rien vu mais le domaine est bien renseign�
				//    :  2 - L'utilisateur a annul�
				//    : -1 - Erreur (serveur non disponible, ...)
				if (ret==0 || ret==1) GetAllConfigsFromServer();
				else if (ret==2) goto end;
				else if (ret==-1) { MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION); goto end; }
			}
			// 0.92 / ISSUE#26 : n'affiche pas la demande si gbDisplayConfigsNotifications=FALSE
			else if (!gbDisplayConfigsNotifications || MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS),"swSSO",MB_YESNO | MB_ICONQUESTION)==IDYES) 
			{
				// 1.07 : renseigne le domaine label correspondant au domain id du .ini
				if (*gszDomainLabel==0) { GetDomainLabel(giDomainId); SaveConfigHeader(); }
				GetAllConfigsFromServer();
			}
		}
		ReportConfigSync(0,gbDisplayConfigsNotifications,gbAdmin);
	}
	else // CAS DES LANCEMENTS ULTERIEURS (avec configurations d�j� enregistr�es)
	{
		BOOL bOK=TRUE;
		// 1.03 : r�cup�re la liste des domaines (doit �tre fait dans tous les cas pour alimenter le menu Upload)
		// mais si �choue, ne doit pas �tre bloquant ni g�n�rer de message d'erreur (mode d�connect�)
		// Pour ne pas g�n�rer une requ�te inutile, on ne fait que pour les utilisateurs qui ont le droit d'utiliser le menu upload
		if (gbInternetManualPutConfig) 
		{
			giNbDomains=GetAllDomains(gtabDomains);
		}
		else
		{
			// Il faut aussi r�cup�rer la liste des domaines pour renseigner le label du domaine de l'utilisateur
			// (s'il est vide et que le domaineId est diff�rent de -1=tous ou 1=commun)
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
		// ISSUE#310 : r�cup�re les configurations en autopublish, si demand�
		if (gbGetAutoPublishedConfigsAtStart) GetAutoPublishConfigsFromServer();

		// 0.91 : si demand�, r�cup�re les nouvelles configurations et/ou les configurations modifi�es
		if (gbGetNewConfigsAtStart || gbGetModifiedConfigsAtStart)
		{
			if (GetNewOrModifiedConfigsFromServer(gbAdmin)!=0) bOK=FALSE;
		}
		// ISSUE#214
		if (gbRemoveDeletedConfigsAtStart) // r�alise une synchro compl�te en supprimant les configs qui ne sont plus pr�sentes sur le serveur
		{
			if (DeleteConfigsNotOnServer()!=0) bOK=FALSE;
		}
		if (bOK) ReportConfigSync(0,gbDisplayConfigsNotifications,gbAdmin);
	}
	
	// ISSUE#59 : ce code �tait avant dans LoadCategories().
	// D�plac� dans winmain pour ne pas l'ex�cuter si des cat�gories ont �t� r�cup�r�es depuis le serveur
	if (giNbCategories==0) // si aucune cat�gorie, cr�e la cat�gorie "non class�"
	{
		strcpy_s(gptCategories[0].szLabel,LEN_CATEGORY_LABEL+1,GetString(IDS_NON_CLASSE));
		gptCategories[0].id=0;
		gptCategories[0].bExpanded=TRUE;
		giNbCategories=1;
		giNextCategId=1;
		WritePrivateProfileString("swSSO-Categories","0",gptCategories[0].szLabel,gszCfgFile);
		StoreIniEncryptedHash(); // ISSUE#164
	}

	// initialisation SSO Web (IE)
	// if (SSOWebInit()!=0) { iError=-1; goto end; } // 1.12B3-TI-TIE4
	guiHTMLGetObjectMsg=RegisterWindowMessage("WM_HTML_GETOBJECT"); // 1.12B3-TI-TIE4
	
	// 1.03 : si configur� pour utiliser le mot de passe AD comme mot de passe secondaire (%ADPASSWORD%),
	//        v�rifie que la date de dernier changement de mot de passe AD et le cas �ch�ant demande � 
	//        l'utilisateur de le saisir
	if (!gbAdmin) // 1.07 : ne le demande pas en mode admin
	{
		if (gbUseADPasswordForAppLogin) CheckADPwdChange(); // ne doit pas �tre bloquant si �choue, car peut �tre li� � AD non joignable par ex.
	}
	
	// inscription pour r�ception des notifs de verrouillage de session
	gbRegisterSessionNotification=WTSRegisterSessionNotification(gwMain,NOTIFY_FOR_THIS_SESSION);
	TRACE((TRACE_DEBUG,_F_,"WTSRegisterSessionNotification() -> OK"));
	if (!gbRegisterSessionNotification)
	{
		// cause possible de l'�chec : "If this function is called before the dependent services 
		// of Terminal Services have started, an RPC_S_INVALID_BINDING error code may be returned"
		// Du coup l'id�e est de r�essayer plus tard (1 minute) avec un timer
		TRACE((TRACE_ERROR,_F_,"WTSRegisterSessionNotification()=%ld [REESSAI DANS 15 SECONDES]",GetLastError()));
		giRegisterSessionNotificationTimer=SetTimer(NULL,0,15000,RegisterSessionNotificationTimerProc);
		giNbRegisterSessionNotificationTries++;
	}

	// 0.80 si demand�, v�rification des mises � jour sur internet
	if (gbInternetCheckVersion) InternetCheckVersion();

	// 0.42 premiere utilisation (fichier vide) => affichage fen�tre config
	// 0.80 -> n'est plus n�cessaire... on peut configurer sans ouvrir cette fen�tre ("ajouter cette application")
	// if (giNbActions==0) ShowConfig(0);

	// 0.93B4 : si gbParseWindowsOnStart=FALSE, ajoute toutes les fen�tres ouvertes et visibles dans la liste des fen�tres
	if (!gbParseWindowsOnStart) ExcludeOpenWindows();

	// ISSUE#145
	// supprim� en 1.15, �tait toujours FALSE !
	// if (bForcePwdChangeNow) 
	// {
	// 	if (WindowChangeMasterPwd(TRUE)!=0) goto end;
	// }

	if (bMigrationWindowsSSO)
	{
		rc=MigrationWindowsSSO();
		if (rc!=0) goto end;
		// En 1.08, n'affiche plus de MessageBox mais une info bulle
		// MessageBox(NULL,GetString(IDS_SYNCHRO_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
		NOTIFYICONDATA nid;
		ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
		nid.cbSize=sizeof(NOTIFYICONDATA);
		nid.hWnd=gwMain;
		nid.uID=0; 
		//nid.hIcon=;
		nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
		nid.uTimeout=2000;
		strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Synchronisation de mot de passe r�ussie");
		strcpy_s(nid.szInfo,sizeof(nid.szInfo),GetString(IDS_SYNCHRO_PWD_OK));
		Shell_NotifyIcon(NIM_MODIFY, &nid); 
	}

	if (gbRecoveryRunning)
	{
		// demande le nouveau mot de passe
		if (giPwdProtection==PP_ENCRYPTED)
		{
			//ChangeMasterPwd("new");
			//RecoverySetNewMasterPwd();
			if (WindowChangeMasterPwd(TRUE)!=0) goto end;
		}
		else // PP_WINDOWS
		{
			if (ChangeWindowsPwd()!=0) goto end;
		}
		gbRecoveryRunning=FALSE;
		// supprime le recovery running
		// WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); d�plac� plus haut (ISSUE#276)
		StoreIniEncryptedHash(); // ISSUE#164
		if (giPwdProtection==PP_ENCRYPTED)
		{
			MessageBox(NULL,GetString(IDS_RECOVERY_ENCRYPTED_OK),"swSSO",MB_ICONINFORMATION | MB_OK);
		}
		else 
		{
			if (!gbRecoveryWebserviceActive) MessageBox(NULL,GetString(IDS_RECOVERY_WINDOWS_OK),"swSSO",MB_ICONINFORMATION | MB_OK);
		}
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_RECOVERY_SUCCESS,NULL,NULL,NULL,NULL,0);
	}

	if (giPwdProtection==PP_WINDOWS)
	{
		char szEventName[1024];
		// ISSUE#247 : passage du username en majuscule pour �viter les pb de diff�rences de casse (vu avec POA Sophos)
		char szUpperUserName[UNLEN+1];
		strcpy_s(szUpperUserName,sizeof(szUpperUserName),gszUserName);
		CharUpper(szUpperUserName);
		// ISSUE#346 - sprintf_s(szEventName,"Global\\swsso-pwdchange-%s-%s",gpszRDN,szUpperUserName);
		sprintf_s(szEventName,"Global\\swsso-pwdchange-%s",szUpperUserName);
		ghPwdChangeEvent=CreateEvent(NULL,FALSE,FALSE,szEventName);
		if (ghPwdChangeEvent==NULL)
		{
			TRACE((TRACE_ERROR,_F_,"CreateEvent(swsso-pwdchange)=%d",GetLastError()));
			iError=-1;
			goto end;
		}
		// ISSUE#169 : Demande le mot de passe � swSSOSVC et le stocke pour r�pondre aux demandes ult�rieures trait�es par GetDecryptedPwd() dans swSSOAD.cpp
		if (GetADPassword()!=0) { iError=-1; goto end; }
		// ISSUE#342 Si mode admin, login de l'admin sur le serveur (non bloquant + message d'erreur dans la fonction ServerAdminLogin)
		if (gbAdmin && !gbNoMasterPwd) ServerAdminLogin(NULL,gszUserName,NULL);

		// 1.08 ISSUE#248 : si configur�, synchronise un groupe de mot de passe secondaires avec le mot de passe AD
		if (!gbAdmin && gbSyncSecondaryPasswordActive)
		{
			if (CheckUserInOU()) SyncSecondaryPasswordGroup();
		}
	}

	// ISSUE#351 : le fait de ne pas activer le timer faisait que l'evt de changement de mdp windows n'�tait pas capt� !
	// if (!gbAdmin)
	{
		if (LaunchTimer()!=0)
		{
			iError=-1;
			goto end;
		}
	}
	// Si -launchapp, ouvre la fen�tre ShowAppNsites
	if (bLaunchApp) ShowLaunchApp();

	// Ici on peut consid�rer que swSSO est bien d�marr� et que l'utilisateur est connect�
	// Prise de la date de login pour les stats
	GetLocalTime(&gLastLoginTime);

	// d�clenchement du timer de refresh des droits, si demand�
	if (giRefreshRightsFrequency!=0)
	{
		giRefreshRightsTimer=SetTimer(NULL,0,giRefreshRightsFrequency*60*1000,RefreshRightsTimerProc);
	}

	// d�clenchement du timer pour enum�ration de fen�tres toutes les 500ms
	// boucle de message, dont on ne sortira que par un PostQuitMessage()
	while((rc=GetMessage(&msg,NULL,0,0))!=0)
    { 
		if (rc!=-1)
	    {
			if (msg.message==guiLaunchAppMsg) 
			{
				TRACE((TRACE_INFO,_F_,"Message recu : swsso-launchapp (0x%08lx)",guiLaunchAppMsg));
				PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_LAUNCH_APP,0),0);
			}
			else if (msg.message==guiConnectAppMsg) 
			{
				TRACE((TRACE_INFO,_F_,"Message recu : swsso-connectapp (0x%08lx)",guiConnectAppMsg));
				PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_SSO_NOW,0),0);
			}
			else if (msg.message==guiStandardQuitMsg && !gbAdmin) // ISSUE#227
			{
				TRACE((TRACE_INFO,_F_,"Message recu : swsso-quit (0x%08lx)",guiStandardQuitMsg));
				PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_QUITTER,0),0);
			}
			else if (msg.message==guiAdminQuitMsg && gbAdmin) // ISSUE#239
			{
				TRACE((TRACE_INFO,_F_,"Message recu : swssoadmin-quit (0x%08lx)",guiAdminQuitMsg));
				PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_QUITTER,0),0);
			}
			else
			{
		        TranslateMessage(&msg); 
		        DispatchMessage(&msg); 
			}
	    }
	}
	iError=0;
end:
	if (gbAdmin && !gbNoMasterPwd) ServerAdminLogout(); // ISSUE#342

	if (iError==-1)
	{
		swLogEvent(EVENTLOG_ERROR_TYPE,MSG_GENERIC_START_ERROR,NULL,NULL,NULL,NULL,0);
		char szErrMsg[1024+1];
		strcpy_s(szErrMsg,sizeof(szErrMsg),GetString(IDS_GENERIC_STARTING_ERROR));
		MessageBox(NULL,szErrMsg,GetString(IDS_MESSAGEBOX_TITLE),MB_OK | MB_ICONSTOP);
	}
	else if (iError==-2) 
	{
		swLogEvent(EVENTLOG_ERROR_TYPE,MSG_SWSSO_INI_CORRUPTED,gszCfgFile,NULL,NULL,NULL,0);
		MessageBox(NULL,gszErrorMessageIniFile,GetString(IDS_MESSAGEBOX_TITLE),MB_OK | MB_ICONSTOP);
	}
	else if (iError==-9) // ISSUE#365
	{
		// quitte sans mettre � jour les stats et sans message d'erreur
	}
	else
	{
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_QUIT,NULL,NULL,NULL,NULL,0);
		if (giStat!=0 && !gbAdmin) swStat(); // 0.99 - ISSUE#106 + ISSUE#244
	}
	if (gpIUIAutomation!=NULL) gpIUIAutomation->Release();
	// on lib�re tout avant de terminer
	swProtectMemoryTerm();
	swCryptTerm();
	//SSOWebTerm(); // 1.12B3-TI-TIE4
	UnloadIcons();
	if (giTimer!=0) KillTimer(NULL,giTimer);
	if (ghPwdChangeEvent!=NULL) CloseHandle(ghPwdChangeEvent);
	if (giRegisterSessionNotificationTimer!=0) KillTimer(NULL,giRegisterSessionNotificationTimer);
	if (gbRegisterSessionNotification) WTSUnRegisterSessionNotification(gwMain);
	if (giRefreshRightsTimer!=0) KillTimer(NULL,giRefreshRightsTimer);
	if (rcSystray==0) DestroySystray(gwMain);
	if (ghBoldFont!=NULL) DeleteObject(ghBoldFont);
	// 0.65 : suppression : UnregisterClass("swSSOClass",ghInstance);
	// Inutile (source MSDN) :
	// "All window classes that an application registers are unregistered when it terminates."
	if (gwMain!=NULL) DestroyWindow(gwMain); // 0.90 : par contre c'est bien de d�truire gwMain (peut-�tre � l'origine du bug de non verrouillage JMA ?)
	if (gptActions!=NULL) free(gptActions);
	if (gptCategories!=NULL) free(gptCategories); 
	if (ghrCoIni==S_OK) CoUninitialize();
	if (hMutex!=NULL) ReleaseMutex(hMutex);
	if (gpRecoveryKeyValue!=NULL) free(gpRecoveryKeyValue);
	if (gpSid!=NULL) free(gpSid);
	if (gpszRDN!=NULL) free(gpszRDN);
	if (gpszConfigNotFoundMailSubject!=NULL) free(gpszConfigNotFoundMailSubject) ;
	if (gpszConfigNotFoundMailBody!=NULL) free(gpszConfigNotFoundMailBody) ;

	TRACE((TRACE_LEAVE,_F_, ""));
	TRACE_CLOSE();
	return 0; 
}
