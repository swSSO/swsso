
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
// Point d'entrée + boucle de recherche de fenêtre à SSOiser
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
bool gbSSOActif=TRUE;	 // Etat swSSO : actif / désactivé	
int giPwdProtection=PP_UNDEFINED; // Protection des mots de passe : PP_ENCRYPTED | PP_WINDOWS -- ISSUE#296 : ajout d'une valeur par défaut

T_ACTION *gptActions;  // tableau d'actions
int giNbActions;		// nb d'actions dans le tableau

int giBadPwdCount;		// nb de saisies erronées de mdp consécutives
HWND gwAskPwd=NULL ;       // anti ré-entrance fenêtre saisie pwd

// 1.12 : on ne conserve plus l'objet clé AES, mais seulement les données utiles dans 4 buffers différents
BOOL gAESKeyInitialized[2];
BYTE gAESProtectedKeyData[2][AES256_KEY_LEN];

// astuce pour limiter les modifs de code : ghKey1 et ghKey2 étaient les handle des 2 clés, ils deviennent les index pour le tableau des clés
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

// 0.91 : pour choix de config (fenêtre ChooseConfig)
typedef struct
{
	int iNbConfigs;
	int tabConfigs[500]; // ISSUE#149 : je laisse 500, c'est trop compliqué de faire dynamique avec giMaxConfigs et surtout inutile
	int iConfig;
} T_CHOOSE_CONFIG;

T_LAST_DETECT gTabLastDetect[MAX_NB_LAST_DETECT]; // 0.93 liste des fenêtres détectées sur cette action

HANDLE ghPwdChangeEvent=NULL; // 0.96

int giLastApplicationSSO=-1;		// dernière application sur laquelle le SSO a été réalisé
int giLastApplicationConfig=-1; // dernière application utilisée (soit SSO soit config)
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
// L'appel à cette fonction est déclenché toutes les 500 ms par le timer.
// C'est cette fonction qui lance l'énumération des fenêtres
//-----------------------------------------------------------------------------
static void CALLBACK TimerProc(HWND w,UINT msg,UINT idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(w);

	// TODO : à déplacer dans un autre timer pour le faire moins souvent ?
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
				strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Changement de mot de passe réussi");
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
		// 0.93 : avant de commencer l'énumération, détaggue toutes les fenêtres
		LastDetect_UntagAllWindows();
		// enumération des fenêtres
		EnumWindows(EnumWindowsProc,0);
		// 0.93 : après l'énumération, efface toutes les fenêtres non taggués
		//        cela permet de supprimer de la liste des derniers SSO réalisés les fenêtres 
		//        qui ne sont plus à l'écran
		LastDetect_RemoveUntaggedWindows();
		// 0.82 : réarmement du timer (si désarmé) une fois que l'énumération est terminée 
		//        (plutôt que dans la WindowsProc -> au moins on est sûr que c'est toujours fait)
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
			if (giNbRegisterSessionNotificationTries>20) // 20 fois 15 secondes = 5 minutes, on arrête, tant pis !
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
// Lance le timer si pas déjà lancé
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
// Réalisation du SSO de l'action passée en paramètre par simulation de frappe clavier
//-----------------------------------------------------------------------------
int KBSimSSO(HWND w, int iAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));
	int rc=-1;
	char szDecryptedPassword[LEN_PWD+1];

	// déchiffrement du champ mot de passe
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

	// analyse et exécution de la simulation de frappe clavier - on passe tous les paramètres, KBSimEx se débrouille.
	// nouveau en 0.91 : flag NOFOCUS permet de ne pas mettre le focus systématiquement sur la fenêtre
	// (nécessaire avec Terminal Server, sinon perte du focus sur les champs login/pwd)
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
// InitDialog DialogProc de la fenêtre de choix de config en cas de multi-comptes
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
	// listview - création colonnes 
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
		lvi.lParam=lpConfigs->tabConfigs[i];		// index de la config dans table générale gptActions
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
	// sélection par défaut
	SendMessage(GetDlgItem(w,CB_TYPE),CB_SETCURSEL,0,0);
	ListView_SetItemState(GetDlgItem(w,LV_CONFIGS),0,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
	
	// titre en gras
	SetTextBold(w,TX_FRAME);
	// centrage
	SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	MACRO_SET_SEPARATOR;
	// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
	// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
	if (giRefreshTimer==giTimer) giRefreshTimer=11;
	SetTimer(w,giRefreshTimer,200,NULL);

end:
	TRACE((TRACE_LEAVE,_F_, ""));
}
//-----------------------------------------------------------------------------
// ChooseConfigOnOK()
//-----------------------------------------------------------------------------
// Appelée lorsque l'utilisateur clique sur OK ou double-clique sur un item de la listview
//-----------------------------------------------------------------------------
int ChooseConfigOnOK(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int iSelectedItem=ListView_GetNextItem(GetDlgItem(w,LV_CONFIGS),-1,LVNI_SELECTED);

	if (iSelectedItem==-1) goto end;
	// Récupère le lparam de l'item sélectionné. 
	// Le lparam contient l'index de la config dans la table générale gptActions : lpConfigs->tabConfigs[i]
	LVITEM lvi;
	lvi.mask=LVIF_PARAM ;
	lvi.iItem=iSelectedItem;
	ListView_GetItem(GetDlgItem(w,LV_CONFIGS),&lvi);
	// Récupère le lparam de la fenêtre qui contient le pointeur vers la structure configs
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
// DialogProc de la fenêtre de choix de config en cas de multi-comptes
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
// Propose une fenêtre de choix de la configuration à utiliser si plusieurs
// matchent (=multi-comptes !)
//-----------------------------------------------------------------------------
int ChooseConfig(HWND w,int *piAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",*piAction));
	int rc=-1;
	int i,j;
	T_CHOOSE_CONFIG config;
	config.iNbConfigs=1;			// nombre de configs qui matchent
	config.tabConfigs[0]=*piAction;	// première config 
	config.iConfig=*piAction;		// première config

	// Cherche si d'autres configurations ACTIVES ont les mêmes caractéristiques :
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
	// si aucune config trouvée autre que celle initiale, on sort et on fait le SSO avec cette config
	if (config.iNbConfigs==1) { giLaunchedApp=-1; rc=0; goto end; }

	// avant d'afficher la fenêtre de choix des configs, on va regarder si l'une des configs trouvées
	// correspond à une application qui vient d'être lancée par LaunchSelectedApp(). 
	// si c'est le cas, inutile de proposer à l'utilisateur de choisir, on choisit pour lui ! (ça c'est vraiment génial)
	TRACE((TRACE_DEBUG,_F_,"giLaunchedApp=%d",giLaunchedApp));
	if (giLaunchedApp!=-1)
	{
		for (i=0;i<config.iNbConfigs;i++) 
		{
			if (config.tabConfigs[i]==giLaunchedApp) // trouvé, on utilisera celle-là, on sort.
			{
				TRACE((TRACE_INFO,_F_,"Lancé depuis LaunchPad action %d",config.tabConfigs[i]));
				*piAction=config.tabConfigs[i];
				giLaunchedApp=-1;
				rc=0;
				// repositionne tLastSSO et wLastSSO des actions qui ne seront pas traitées
				// l'action traitée sera mise à jour au moment du SSO
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
	// pas trouvé, on oublie, c'était une mauvaise piste
	giLaunchedApp=-1;

	// affiche la fenêtre de choix des configs
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_CHOOSE_CONFIG),w,ChooseConfigDialogProc,LPARAM(&config))!=IDOK) 
	{
		// l'utilisateur a annulé, on marque tout le monde en WAIT_ONE_MINUTE comme ça on ne fait pas le SSO tout de suite
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
	// retourne l'action qui a été choisie par l'utilisateur
	TRACE((TRACE_INFO,_F_,"Choix de l'utilisateur : action %d",config.iConfig));
	*piAction=config.iConfig;
	// repositionne tLastSSO et wLastSSO des actions qui ne seront pas traitées
	// l'action traitée sera mise à jour au moment du SSO
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
// Nouvelle fonction en 1.19 pour reprendre tout le code qui demande à l'utilisateur
// les informations identifiant(s) et mot de passe éventuellement manquantes
//-----------------------------------------------------------------------------
// i = index de l'action dans gptActions
//-----------------------------------------------------------------------------
int AskMissingValues(HWND w,int i,int iPopupType)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	//0.91 : vérifie que chaque champ identifiant et mot de passe déclaré a bien une valeur associée
	//       sinon demande les valeurs manquantes à l'utilisateur !
	//0.92 : correction ISSUE#7 : le cas des popup qui ont toujours id et pwd mais pas de chaque champ 
	//		 identifiant et mot de passe déclaré n'était pas traité !
	gbDontAskId=TRUE;
	gbDontAskId2=TRUE;
	gbDontAskId3=TRUE;
	gbDontAskId4=TRUE;
	gbDontAskPwd=TRUE;

	// 0.93 : logs
	// ISSUE#127 : déplacé + loin, une fois qu'on a réussi le SSO
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
			
	// s'il y a au moins un champ non renseigné, afficher la fenêtre de saisie
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
			// l'utilisateur a annulé, on marque la config en WAIT_ONE_MINUTE comme ça on ne fait pas 
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
// Callback d'énumération de fenêtres présentes sur le bureau et déclenchement
// du SSO le cas échéant
//-----------------------------------------------------------------------------
// [rc] : toujours TRUE (continuer l'énumération)
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
	char szClassName[128+1]; // pour stockage nom de classe de la fenêtre
	char szTitre[255+1];	  // pour stockage titre de fenêtre
	int rc;
	char szMsg[512+1];
	int lenTitle;
	int iBrowser=BROWSER_NONE;
	IUIAutomationElement* pDocument=NULL; // document navigateur
	
	guiNbWindows++;
	// 0.93B4 : fenêtres éventuellement exclues 
	if (IsExcluded(w)) goto end;
	// lecture du titre de la fenêtre
	GetWindowText(w,szTitre,sizeof(szTitre));
	if (*szTitre==0) goto end; // si fenêtre sans titre, on passe ! <optim>
	if (!IsWindowVisible(w)) goto end; // fenêtre cachée, on passe ! <optim+compteur>
	if (IsIconic(w)) goto end; // ISSUE#280 : ignore les fenêtres réduites dans la barre des taches pour éviter des pertes de focus inutiles
	guiNbVisibleWindows++;

	// 0.93 : marque la fenêtre comme toujours présente à l'écran dans liste des derniers SSO réalisés
	LastDetect_TagWindow(w); 

	// lecture de la classe de la fenêtre (pour reconnaitre IE et Firefox ensuite)
	GetClassName(w,szClassName,sizeof(szClassName));

	// nouveau en 1.07B6
	// si c'est une popup chrome, comme on ne peut pas discriminer sur le titre, il vaut mieux lire une seule fois l'URL ici
	// sinon on le fait autant de fois qu'il y a d'applications configurées, c'est inutile et trop consommateur
	if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0)
	{
		pszChromePopupURL=GetChromePopupURL(w);
	}
	
	TRACE((TRACE_DEBUG,_F_,"szTitre=%s",szTitre));

	// boucle dans la liste d'action pour voir si la fenêtre correspond à une config connue
    for (i=0;i<giNbActions;i++)
    {
    	if (gptActions[i].bSafe) goto suite;
		if (!gptActions[i].bActive) goto suite; // action désactivée
		if (!gptActions[i].bSaved) { TRACE((TRACE_INFO,_F_,"action %d non enregistrée => SSO non exécuté",i)); goto suite; } // 0.93B6 ISSUE#55
		if (gptActions[i].iType==UNK) goto suite; // 0.85 : ne traite pas si type inconnu
		
		// Avant de comparer le titre, vérifier si ce n'est pas une popup Chrome
		// En effet, avec Chrome, le titre n'est pas toujours représentatif (notamment sur popup proxy)
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
    		// 0.80 : on compare sur le début du titre et non plus sur une partie du titre 
			// if (strstr(szTitre,gptActions[i].szTitle)==NULL) goto suite; // fenêtre inconnue...
			// 0.92B3 : évolution de la comparaison du titre pour prendre en compte le joker *
			// if (_strnicmp(szTitre,gptActions[i].szTitle,lenTitle)!=0) goto suite; // fenêtre inconnue...
			if (!swStringMatch(szTitre,gptActions[i].szTitle)) goto suite;
		}
		// A ce stade, on a associé le titre à une action
		// (uniquement le titre, c'est à dire qu'on n'a pas encore vérifié que l'URL était correcte)
		TRACE((TRACE_INFO,_F_,"======= Fenêtre handle 0x%08lx titre connu (%s) classe (%s) action (%d) type (%d) à vérifier maintenant",w,szTitre,szClassName,i,gptActions[i].iType));
    	if (gptActions[i].iType==POPSSO) 
    	{
			if (strcmp(szClassName,gcszMozillaDialogClassName)==0) // POPUP FIREFOX
			{
				pszURL=GetFirefoxPopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup firefox non trouvee")); goto suite; }
				iPopupType=POPUP_FIREFOX;
			}
			else if (strcmp(szClassName,"#32770")==0 && (strcmp(szTitre,"Sécurité de Windows")==0 ||
					 strcmp(szTitre,"Windows Security")==0)) // POPUP IE8 SUR W7 [ISSUE#5] (FR et US uniquement... pas beau)
			{
				pszURL=GetW7PopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W7 non trouvee")); goto suite; }
				iPopupType=POPUP_W7;
			}
			else if (strcmp(szClassName,"Credential Dialog Xaml Host")==0 &&  
					(strcmp(szTitre,"Sécurité Windows")==0 || strcmp(szTitre,"Windows Security")==0)) // POPUP W10 anniversaire... IE, EDGE, partages réseau, etc.
			{
				pszURL=GetW10PopupURL(w);
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup W10 non trouvee")); goto end; }
				iPopupType=POPUP_W10;
			}
			else if (iPopupType==POPUP_CHROME)
			{
				pszURL=GetChromePopupURL(w); // oui c'est un peu con parce qu'on a l'info dans pszChromePopupURL mais pour simplifier la logique de libération des pointeurs c'est mieux comme ça
				if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL popup Chrome non trouvee")); goto suite; }
			}
			else
			{
				iPopupType=POPUP_XP;
			}
			// il faut vérifier que l'URL matche pour FIREFOX, W7 et CHROME car elles ont toutes le meme titre !
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
						TRACE((TRACE_INFO,_F_,"Impossible de vérifier l'URL de la barre d'URL... on ne fait pas le SSO !"));
						goto suite;// URL popup authentification inconnue
					}
				}
				// si p!=null c'est qu'on a trouvé un | et une URL derrière, il faut la vérifier
				// ce test ne change pas, sauf qu'il porte sur pszURL2 pour couvrir les 2 cas (avec ou sans |)
				// 0.92B6 : utilise le swStringMatch, permet donc d'utiliser * en début de chaîne
				if (!swStringMatch(pszURL,pszURL2))
				{
					TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
					goto suite;// URL popup authentification inconnue
				}
			}
		}
		else if (gptActions[i].iType==WINSSO && *(gptActions[i].szURL)!=0) // fenêtre Windows avec URL, il faut vérifier que l'URL matche
		{
			if (!CheckURL(w,i))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;// URL popup authentification inconnue
			}
		}
		else if (gptActions[i].iType==XINSSO && *(gptActions[i].szURL)!=0)
		{
			// ISSUE#400 : ajout du CheckURL pour qu'il s'applique également aux configurations Windows simplifiées
			if (!CheckURL(w,i))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;
			}
		}
		else if (gptActions[i].iType==WEBSSO || gptActions[i].iType==XEBSSO) // action WEB, il faut vérifier que l'URL matche
		{
			if (strcmp(szClassName,"IEFrame")==0 || // IE
				strcmp(szClassName,"#32770")==0 ||  // Network Connect
				strcmp(szClassName,"rctrl_renwnd32")==0 || // Outlook 97 à 2003 (au moins, à vérifier pour 2007)
				strcmp(szClassName,"OpusApp")==0 || // Word 97 à 2003 (au moins, à vérifier pour 2007)
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
				// ISSUE#381 : si, il faut le faire sinon ne fonctionne pas avec les sites lancés en mode application
				// ISSUE#382 : avec Chrome 69, GetChromeURL et  GetChromeURL51 ne fonctionnent pas, il faut NewGetChromeURL
				if (pszURL==NULL || *pszURL==0) pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); // ISSUE#314
				if (pszURL==NULL || *pszURL==0) { Sleep(100); pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); } // Suite à ISSUE#404, Chrome n'a pas eu le temps de monter l'interface IAccessible après l'appel à ForceChromeAccessibility
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

			// 0.92B6 : utilise le swStringMatch, permet donc d'utiliser * en début de chaîne
			// if (!swStringMatch(pszURL,gptActions[i].szURL))
			if (!swURLMatch(pszURL,gptActions[i].szURL))
			{
				TRACE((TRACE_DEBUG,_F_,"Titre connu, mais URL ne matche pas, on passe !"));
				goto suite;// URL popup authentification inconnue
			}
		}
		// ARRIVE ICI, ON SAIT QUE LA FENETRE EST BIEN ASSOCIEE A L'ACTION i ET QU'IL FAUT FAIRE LE SSO...
		// ... SAUF SI DEJA FAIT RECEMMENT !
		TRACE((TRACE_INFO,_F_,"======= Fenêtre vérifée OK, on vérifie si elle a été traitée récemment"));
		TRACE((TRACE_DEBUG,_F_,"Fenetre      ='%s'",szTitre));
		TRACE((TRACE_DEBUG,_F_,"Application  ='%s'",gptActions[i].szApplication));
		TRACE((TRACE_DEBUG,_F_,"Type         =%d",gptActions[i].iType));

		// ISSUE#107 : conserve l'id de la dernière application reconnue pour l'accompagnement du changement de mot de passe
		// ISSUE#108 : conserve l'id de la dernière application reconnue pour la sélectionner par défaut dans la fenêtre de gestion des sites
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

		// on n'exécute pas (une action utilisateur est nécessaire)
		if (gptActions[i].bWaitForUserAction) 
		{
			TRACE((TRACE_INFO,_F_,"SSO en attente d'action utilisateur"));
			goto suite;
		}
		
		// Détection d'un SSO déjà fait récemment sur la fenêtre afin de prévenir les essais multiples 
		// avec mauvais mots de passe qui pourraient bloquer le compte 
		tLastSSOOnThisWindow=LastDetect_GetTime(w); // date de dernier SSO sur cette fenêtre (toutes actions confondues)
		TRACE((TRACE_DEBUG,_F_,"tLastSSOOnThisWindow        =%ld (time du dernier SSO sur cette fenêtre)",tLastSSOOnThisWindow));
		TRACE((TRACE_DEBUG,_F_,"tNow-tLastSSOOnThisWindow	=%ld (nb secondes depuis dernier SSO sur cette fenêtre)",tNow-tLastSSOOnThisWindow));

		if (gptActions[i].iType==WEBSSO || gptActions[i].iType==XEBSSO || 
			(gptActions[i].iType==POPSSO && iPopupType==POPUP_CHROME)) // cas particulier : la popup chrome n'est pas une fenêtre comme les autres popup
		{
			// si tLastSSOOnThisWindow==1 => handle inconnu donc jamais traité => on fait le SSO, sinon :
			if (tLastSSOOnThisWindow!=-1) // on a déjà fait un SSO sur cette même fenetre (cette action ou une autre, peu importe, par exemple une autre action à cause du multi-comptes)
			{
				// c'est du Web, rien de choquant (le navigateur a toujours le meme handle quel que soit le site accédé !
				// Par contre, il faut dans les cas suivants ne pas réessayer immédiatement, mais laisser passer
				// un délai avant le prochain essai (précisé en face de chaque cas) :
				// - Eviter de griller un compte avec X saisies de mauvais mots de passe de suite (iWaitFor=giWaitBeforeNewSSO)
				// - Ne pas bouffer de CPU en cherchant tous les 500ms des champs qu'on n'a pas trouvé dans la page (iWaitFor=WAIT_IF_SSO_NOK)
				// - Ne pas bouffer de CPU quand l'URL ne correspond pas (alors que le titre correspond) (iWaitFor=WAIT_IF_BAD_URL)
				if ((tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor) 
				{
					TRACE((TRACE_INFO,_F_,"(tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor(%d)",gptActions[i].iWaitFor));
					bDoSSO=false;
				}
			}
			else // jamais fait de SSO sur cette fenêtre, on a envie de tenter, mais il ne faut juste pas le faire  
				 // si l'utilisateur a annulé dans la fenêtre de choix multi-comptes (ISSUE#133)
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
 				// fenêtre traitée précédemment par cette action OU PAR UNE AUTRE (cause multi-comptes)
				// elle est toujours là, donc c'est l'authentification qui rame, pas la peine de réessayer, 
				// elle disparaitra d'elle même au  bout d'un moment...
   				TRACE((TRACE_INFO,_F_,"Fenetre %s handle identique deja traitee il y a %d secondes, on ne fait rien",gptActions[i].szTitle,tNow-tLastSSOOnThisWindow));
				bDoSSO=false;
			}	
			else  
			{
				// fenêtre inconnue au bataillon
				TRACE((TRACE_INFO,_F_,"Fenetre %s handle différent",gptActions[i].szTitle));
				if (gptActions[i].iWaitFor==WAIT_ONE_MINUTE && (tNow-gptActions[i].tLastSSO)<gptActions[i].iWaitFor)
				{
					TRACE((TRACE_DEBUG,_F_,"WAIT_ONE_MINUTE"));
					bDoSSO=false;
				}
				else
				{
					if ((tNow-gptActions[i].tLastSSO)<5) // 0.86 : passage de 3 à 5 secondes pour applis qui rament...
					{
						TRACE((TRACE_DEBUG,_F_,"Le SSO sur cette action a été réalisé il y a moins de 5 secondes"));
						// le SSO sur cette action a été réalisé il y a moins de 5 secondes et cette fenêtre est nouvelle
						// => 2 possibilités :
						// 1) c'est une réapparition suite à un échec d'authentification
						// 2) c'est vraiment une nouvelle fenêtre ouverte dans les 5 secondes
						// pour différencier, il faut voir si la fenêtre sur laquelle le SSO a été fait précedemment
						// est toujours à l'écran ou pas. Si elle est toujours là, c'est bien une nouvelle fenêtre
						// Si elle n'est plus là, on est sans doute dans le cas de l'échec d'authent
						if(gptActions[i].wLastSSO!=NULL && IsWindow(gptActions[i].wLastSSO))
						{
							TRACE((TRACE_DEBUG,_F_,"La fenêtre précédemment SSOisée est toujours là, celle-ci est donc nouvelle"));
							// fenêtre toujours là ==> nouvelle fenêtre, on fait le SSO
							gptActions[i].iWaitFor=giWaitBeforeNewSSO;
    						bDoSSO=true;
						}
						else // fenêtre plus là, sans doute un échec d'authentification 
						{
							TRACE((TRACE_DEBUG,_F_,"La fenêtre précédemment SSOisée n'est plus là, sans doute un retour cause échec authentification"));
							// 0.85 : on suggère donc à l'utilisateur de changer son mot de passe.
							if (gptActions[i].bAutoLock) // 0.66 ne suspend pas si l'utilisateur a mis autoLock=NO (le SSO sera donc fait)
							{
								char szSubTitle[256];
								KillTimer(NULL,giTimer); giTimer=0;
								TRACE((TRACE_INFO,_F_,"Fenetre %s handle different deja traitee il y a %d secondes !",gptActions[i].szTitle,tNow-gptActions[i].tLastSSO));
								T_MESSAGEBOX3B_PARAMS params;
								params.szIcone=IDI_EXCLAMATION;
								params.iB1String=IDS_DESACTIVATE_B1; // réessayer
								params.iB2String=IDS_DESACTIVATE_B2; // changer le mdp
								params.iB3String=IDS_DESACTIVATE_B3; // désactiver
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
								if (reponse==B1) // réessayer
								{
									gptActions[i].iWaitFor=0;
									// rien à faire, ça va repartir tout seul :-)
								}
								else if (reponse==B2) // changer le mdp
								{
									ChangeApplicationPassword(w,i);
								}
								else // B3 : désactiver
								{
									bDoSSO=false;
									// 0.86 sauvegarde la désactivation dans le .INI !
									// 0.90A2 : on ne sauvegarde plus (risque d'écrire dans une section renommée)
									// WritePrivateProfileString(gptActions[i].szApplication,"active","NO",gszCfgFile);
									
									//gptActions[i].bActive=false;
									// 0.90B1 : finalement on ne désactive plus, on suspend pendant 1 minute (#107)
									gptActions[i].iWaitFor=WAIT_ONE_MINUTE;
								}
							}
    					}
					}
				}
			}
		}
		//------------------------------------------------------------------------------------------------------
		if (bDoSSO) // on a déterminé que le SSO doit être tenté, mais peut-être pas en fonction de la config (ISSUE#176)
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
		if (bDoSSO) // cette fois, c'est sûr, SSO doit être tenté
		//------------------------------------------------------------------------------------------------------
		{
			TRACE((TRACE_INFO,_F_,"======= Fenêtre vérifiée OK et pas traitée récemment, on lance le SSO !"));
			//0.80 : tue le timer le temps de faire le SSO, le réarme ensuite (cas des pages lourdes à parser avec Firefox...)
			KillTimer(NULL,giTimer); giTimer=0;

			//0.91 : fait choisir l'appli à l'utilisateur si plusieurs matchent (gestion du multicomptes)
			//1.19 : déplacement plus tard, une fois qu'on est sur qu'on est sur la bonne page (à cause du filtre textuel ISSUE#373)
			// if (ChooseConfig(w,&i)!=0) goto end;

			//1.19 : déplacement du code vers une nouvelle fonction, qui sera appelée plus tard, 
			// une fois qu'on est sur qu'on est sur la bonne page (à cause du filtre textuel ISSUE#373)
			// if (AskMissingValues(w,i,iPopupType)!=0) goto end;
						
			//0.89 : indépendamment du type (WIN, POP ou WEB), si c'est de la simulation de frappe clavier, on y va !
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
				// ISSUE#127 (le swLogEvent était fait trop tôt, cf. plus haut)
				swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_SECONDARY_LOGIN_SUCCESS,gptActions[i].szApplication,gptActions[i].szId1Value,NULL,NULL,i);
				goto end;
			}
			switch(gptActions[i].iType)
			{
				case WINSSO: 
				case POPSSO: 
					// TODO plus tard : déplacer ces 2 lignes dans SSOWindows
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
								// ISSUE#60 : en attendant d'avoir une réponse de Mozilla, on n'exécute pas les anciennes config 
								//            avec Firefox sous Windows 7 pour éviter le plantage !
								// ISSUE#60 modifié en 0.94B2 : Vista et 64 bits seulement
								if (giOSBits==OS_64) 
								{
									TRACE((TRACE_INFO,_F_,"Ancienne configuration Firefox + Windows 64 bits : on n'exécute pas !"));
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
									TRACE((TRACE_INFO,_F_,"Ancienne configuration Chrome + Windows 64 bits : on n'exécute pas !"));
									rc=0;
								}
								else rc=SSOFirefox(w,&i,iBrowser); // ISSUE#66 0.94 : chrome a implémenté ISimpleDOM comme Firefox !
							}
							break;
						case BROWSER_EDGE:
							rc=SSOWebUIA(w,&i,iBrowser,pDocument);
							break;
						default:
							rc=-1;
					}

					// repositionne tLastDetect et wLastDetect (écrasé par la suite dans un cas, cf. plus bas)
					//time(&gptActions[i].tLastDetect);
					//gptActions[i].wLastDetect=w;
					time(&gptActions[i].tLastSSO);
					gptActions[i].wLastSSO=w;
					LastDetect_AddOrUpdateWindow(w,iPopupType);

					if (rc==0) // SSO réussi
					{
						TRACE((TRACE_INFO,_F_,"SSO réussi, on ne le retente pas avant %d secondes",giWaitBeforeNewSSO));
						// on ne réessaie pas si le SSO est réussi mais on ne peut pas cramer définitivement
						// ce SSO sinon un utilisateur qui se déconnecte ne pourra pas se reconnecter
						// tant qu'il n'aura pas fermé / réouvert son navigateur (handle différent) !
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=giWaitBeforeNewSSO; 
						// ISSUE#127 (le swLogEvent était fait trop tôt, cf. plus haut)
						swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_SECONDARY_LOGIN_SUCCESS,gptActions[i].szApplication,gptActions[i].szId1Value,NULL,NULL,i);
					}
					else if (rc==-2) // SSO abandonné car l'URL ne correspond pas
					{
						// Si l'URL est différente, c'est probablement que le SSO a été fait précédemment
						// et que l'utilisateur est sur une autre page du site.
						// Il n'est pas utile de réessayer immédiatement, néanmoins il faut essayer régulièrement
						// pour que le SSO fonctionne quand la page attendue arrivera (par exemple si la page
						// visitée était une page précédant celle sur laquelle l'utilisateur va faire le SSO...)
						// Le ré-essai ne coute presque rien en CPU, on peut le faire toutes les 5 secondes
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=WAIT_IF_BAD_URL;
						TRACE((TRACE_INFO,_F_,"URL différente de celle attendue, prochain essai dans %d secondes",WAIT_IF_BAD_URL));
					}
					else if (rc==-3) // SSO abandonné car libelle szId4Value non trouvé dans la page
					{
						gptActions[i].iNbEssais=0;
						gptActions[i].iWaitFor=WAIT_IF_LABEL_NOT_FOUND;
						TRACE((TRACE_INFO,_F_,"Libelle %s non trouvé dans la page, prochain essai dans %d secondes",gptActions[i].szId4Value,WAIT_IF_LABEL_NOT_FOUND));
					}
					else if (rc==-4) // SSO annulé par l'utilisateur (dans fenetre choix de config ou saisie des valeurs manquantes)
					{
						// iWaitFor a déjà été mis à jour dans ChooseConfig ou AskMissingValues, on ne fait rien de plus
					}
					else // SSO raté, erreur inattendue ou plus probablement champs non trouvés
					{
						// Deux cas de figure... comment les différencier ???
						// 1) La page n'est pas encore complètement arrivée, il faut donc réessayer un petit peu plus tard
						// 2) Le titre et l'URL ne permettent pas de distinguer la page courante de la page de login...
						// Solution : on retente quelques fois relativement rapidement pour traiter le cas 1 et au bout de 
						// quelques essais on espace les tentatives pour traiter plutôt le cas 2 (car parsing complet gourmant
						// en CPU, on ne peut pas le faire toutes les 2 secondes indéfiniment)
						TRACE((TRACE_INFO,_F_,"Echec SSOWeb application %s (essai %d)",gptActions[i].szApplication,gptActions[i].iNbEssais));
						gptActions[i].iNbEssais++;
						if (gptActions[i].iNbEssais<21) // les 20 premières fois, on retente immédiatement
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
							// bon, ça fait un paquet de fois qu'on réessaie... c'est surement un cas 
							// ou titre+URL ne permettent pas de différencier la page de login des autres
							// Du coup, inutile de s'acharner, on retente + tard
							// gptActions[i].iNbEssais=0; // réarme le compteur pour la prochaine tentative
							// A VOIR : je pense qu'il vaut mieux ne pas réarmer le compteur
							//         comme ça on continue sur un rythme d'une fois tous les pas souvent
							gptActions[i].iWaitFor=WAIT_IF_SSO_NOK; 
							TRACE((TRACE_INFO,_F_,"Prochain essai dans %d secondes",WAIT_IF_SSO_NOK));
						}
					}
					break;
				default: ;
			} // switch
		}
		// 0.80 : update la config sur le serveur si autorisé par l'utilisateur
		// 0.82 : le timer doit plutôt être réarmé quand on a fini l'énumération des fenêtres -> voir TimerProc()
		//        en plus ici c'était dangereux : un goto suite ou end passait outre le réarmement !!!
		//        et je me demande si ça ne pouvait pas créer des cas de réentrance qui auraient eu pour conséquence
		//        que le timer ne soit jamais réarmé...
		// giTimer=SetTimer(NULL,0,500,TimerProc);
suite:
		// eh oui, il faut libérer pszURL... Sinon, vous croyez vraiment que 
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
// Déchargement de toutes les icones et pointeurs de souris
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
// DialogProc de la boite de choix simplifiée de stratégie de mot de passe
// (fenêtre ouverte lorsqu'aucun fichier de config n'est trouvé)
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
						// génère le sel qui sera pris en compte pour la dérivation de la clé AES et le stockage du mot de passe
						swGenPBKDF2Salt();
						swCryptDeriveKey(szPwd1,ghKey1);
						StoreMasterPwd(szPwd1);
						RecoveryChangeAESKeyData(ghKey1);
						// inscrit la date de dernier changement de mot de passe dans le .ini
						// cette valeur est chiffré par le (nouveau) mot de passe et écrite seulement si politique mdp définie
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
			// Complément ISSUE#136 : ne pas afficher le bouton mdp oublié si synchro mdp Windows activée
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
			// ISSUE#136 : nouveau bouton mdp oublié (masqué si pas de recouvrement défini ou si askpwd depuis loupe AppNsites)
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
					char szPwd[LEN_PWD+1];
					GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
					if (giPwdProtection==PP_WINDOWS)
					{
						if (gAESKeyInitialized[ghKey1]!=NULL) // Cas de la demande de mot de passe "loupe" (TogglePasswordField) ou du déverrouillage
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
						else // cas du découplage (ISSUE#181)
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
								// cette valeur est chiffré par le (nouveau) mot de passe et écrite seulement si politique mdp définie
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
						// 0.90 : si une clé de recouvrement existe et les infos de recouvrement n'ont pas encore
						//        été enregistrées dans le .ini (cas de la première utilisation après déploiement de la clé
						SetFocus(GetDlgItem(w,TB_PWD)); // ISSUE#182
						RecoveryFirstUse(w,ghKey1);
						EndDialog(w,IDOK);
					}
					else
					{
						SecureZeroMemory(szPwd,strlen(szPwd));
						// 0.93B1 : log authentification primaire échouée
						if (gbSSOActif)
							swLogEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_LOGIN_ERROR,NULL,NULL,NULL,NULL,0);
						else
							swLogEvent(EVENTLOG_WARNING_TYPE,MSG_UNLOCK_BAD_PWD,NULL,NULL,NULL,NULL,0);

						SendDlgItemMessage(w,TB_PWD,EM_SETSEL,0,-1);

						// ISSUE#136 : le message en cas d'erreur devient générique suite à l'ajout du bouton mdp oublié
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
// Demande et vérifie le mot de passe de l'utilisateur.
//-----------------------------------------------------------------------------
// [rc] : 0 si OK, -1 si mauvais mot de passe, -2 mot de passe oublié
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

	// 0.65 : anti ré-entrance
	if (gwAskPwd!=NULL) 
	{
		SetForegroundWindow(gwAskPwd);
		goto end;
	}

	// mode de fonctionnement sans mot de passe
	if (gbNoMasterPwd && gcszK1[0]!='1')
	{
		if (gbAdmin) // si défini, demande le mot de passe admin, sinon demande de le définir
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

				// 0.90 : si une clé de recouvrement existe et les infos de recouvrement n'ont pas encore
				//        été enregistrée dans le .ini (cas de la première utilisation après déploiement de la clé
				swCryptDeriveKey(szPwd,ghKey1);
				SecureZeroMemory(szPwd,strlen(szPwd));
				gbRememberOnThisComputer=TRUE;
				ret=0;
				goto end;
			}
		}
	}

	// ISSUE#309 : ne redemande pas le mot de passe s'il a été demandé récemment (selon config)
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
	time(&gtLastAskPwd); // ISSUE#309 : mot de passe OK, on mémorise l'heure
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
	int iError=0; // v0.88 : message d'erreur au démarrage
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
	// BOOL bForcePwdChangeNow=FALSE; // supprimé en 1.15, était toujours FALSE !
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
	gwAskPwd=NULL; // 0.65 anti ré-entrance fenêtre saisie pwd
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

	ZeroMemory(&gTabLastDetect,sizeof(T_LAST_DETECT)); // pourrait contribuer à la correction de ISSUE#229

	// ligne de commande
	if (strlen(lpCmdLine)>_MAX_PATH) { iError=-1; goto end; } 
	TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));

	// enregistrement des messages pour réception de paramètres en ligne de commande quand swSSO est déjà lancé
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
	
	// 0.92 : récupération version OS pour traitements spécifiques Vista et/ou Seven
	// Remarque : pas tout à fait juste, mais convient pour les postes de travail. A revoir pour serveurs.
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

	// 0.91 : si la ligne de commande contient le paramètre -launchapp, ouvre la fenêtre de lancement d'appli
	//        soit en postant à message à swsso si déjà lancé, soit par appel à ShowAppNsites en fin de WinMain()
	if (strnistr(lpCmdLine,"-launchapp",-1)!=NULL && guiLaunchAppMsg!=0) 
	{
		bLaunchApp=TRUE;
		// supprime le paramètre -launchapp de la ligne de commande pour traitement du path éventuellement spécifié pour le .ini
		lpCmdLine+=strlen("-launchapp");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	// 0.97 : si la ligne de commande contient le paramètre -connectapp, ouvre la fenêtre de lancement d'appli
	//        soit en postant à message à swsso si déjà lancé, soit par appel à ShowAppNsites en fin de WinMain()
	if (strnistr(lpCmdLine,"-connectapp",-1)!=NULL && guiConnectAppMsg!=0) 
	{
		bConnectApp=TRUE;
		// supprime le paramètre -connectapp de la ligne de commande 
		// pour traitement du path éventuellement spécifié pour le .ini
		lpCmdLine+=strlen("-connectapp");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	// ISSUE#205 : mode admin
	if (strnistr(lpCmdLine,"/admin",-1)!=NULL) 
	{
		gbAdmin=TRUE;
		// supprime le paramètre de la ligne de commande 
		// pour traitement du path éventuellement spécifié pour le .ini
		lpCmdLine+=strlen("/admin");
		if (*lpCmdLine==' ') lpCmdLine++;
		TRACE((TRACE_INFO,_F_,"lpCmdLine=%s",lpCmdLine));
	}
	else if (strnistr(lpCmdLine,"-admin",-1)!=NULL) 
	{
		gbAdmin=TRUE;
		// supprime le paramètre de la ligne de commande 
		// pour traitement du path éventuellement spécifié pour le .ini
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
	
	// 0.42 vérif pas déjà lancé
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
			TRACE((TRACE_INFO,_F_,"Demande à l'instance précédente d'ouvrir la fenetre de lancement d'applications"));
			PostMessage(HWND_BROADCAST,guiLaunchAppMsg,0,0);
		}
		if (bConnectApp)
		{
			TRACE((TRACE_INFO,_F_,"Demande à l'instance précédente de connecter l'application en avant plan"));
			PostMessage(HWND_BROADCAST,guiConnectAppMsg,0,0);
		}
		if (bQuit)
		{
			TRACE((TRACE_INFO,_F_,"Demande à l'instance précédente de s'arrêter gbAdmin=%d",gbAdmin));
			PostMessage(HWND_BROADCAST,gbAdmin?guiAdminQuitMsg:guiStandardQuitMsg,0,0);
			// attend 5 secondes et si toujours là on le shoote
			Sleep(iWaitBeforeKill);
			KillswSSO();
		}
		goto end;
	}

	// ISSUE#364 : lecture en priorité du chemin du .ini en base de registre
	LoadIniPathNamePolicy();
	if (*gpszIniPathName!=0)
	{
		ExpandFileName(gpszIniPathName,gszCfgFile,_SW_MAX_PATH+1);
	}
	else if (*lpCmdLine==0) // si pas de .ini passé en paramètre, on cherche dans le rép courant
	{
		len=GetCurrentDirectory(_SW_MAX_PATH-10,gszCfgFile);
		if (len==0) { iError=-1; goto end; }
		if (gszCfgFile[len-1]!='\\') { gszCfgFile[len]='\\'; len++; }
		strcpy_s(gszCfgFile+len,_SW_MAX_PATH+1,"swSSO.ini");
	}
	else // sinon utilie le chemin passé en paramètre
	{
		ExpandFileName(lpCmdLine,gszCfgFile,_SW_MAX_PATH+1); // ISSUE#104 et ISSUE#109
	}
	if (gbExitIfNetworkUnavailable)
	{
		// ISSUE#365 : teste le chemin du fichier .ini. Si erreur différente de "fichier non trouvé", 
		// on suppose que c'est un pb d'accès réseau et on sort
		HANDLE hfTest;
		hfTest=CreateFile(gszCfgFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if (hfTest==INVALID_HANDLE_VALUE)
		{
			DWORD dwError=GetLastError();
			TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",gszCfgFile,dwError));
			if (dwError!=ERROR_FILE_NOT_FOUND && dwError!=ERROR_PATH_NOT_FOUND)
			{
				TRACE((TRACE_ERROR,_F_,"Chemin d'accès au .ini introuvable (problème réseau ?), on quitte sans message"));
				iError=-9;
				goto end;
			}
		}
		else
		{
			CloseHandle(hfTest);
		}
	}
	// Pas simple de le faire plus tôt... ce veut dire que si des messages sont affichés avant 
	// ils seront dans la langue de l'OS et pas dans la langue choisie par l'utilisateur
	GetOSLanguage(); // récupère la langue de l'OS
	giLanguage=GetPrivateProfileInt("swSSO","Language",0,gszCfgFile);
	if (giLanguage!=0) SetLanguage(); // applique ce que l'utilisateur a défini

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
	// si failed, ne doit pas sortir, on est peut-être sur XP ou 2003...

	// récupère username, computername, SID et domaine
	if (GetUserDomainAndComputer()!=0) { iError=-1; goto end; }

	// chargement ressources
	if (LoadIcons()!=0) { iError=-1; goto end; }

	// initialisation du module crypto
	if (swCryptInit()!=0) { iError=-1; goto end; }
	if (swProtectMemoryInit()!=0) { iError=-1; goto end; }
	
	// chargement des policies (password, global et enterprise)
	LoadPolicies();

	// création fenêtre technique (réception des messages) -- ISSUE#175 (était plus bas avant, juste avant le WTSRegister)
	gwMain=CreateMainWindow();
	if (gwMain==NULL) { iError=-1; goto end; }

	// création icone systray -- ISSUE#175 (était plus bas avant, juste après le WTSRegister)
	rcSystray=CreateSystray(gwMain);

	// lecture du header de la config (=lecture section swSSO)
	if (GetConfigHeader()!=0) { iError=-2; goto end; }

	// ISSUE#318
	LoadGlobalOrDomainPolicies(gszDomainLabel);

	if (*gszCfgVersion==0 || giPwdProtection==0) // version <0.50 ou premier lancement... // ISSUE#295
	{
		// ISSUE#260 : crée le répertoire qui doit contenir le fichier .ini
		char *p=NULL;
		p=strrchr(gszCfgFile,'\\');
		if (p!=NULL)
		{
			*p=0;
			TRACE((TRACE_DEBUG,_F_,"Creation dossier pour le .ini : %s",gszCfgFile));
			if (!CreateDirectory(gszCfgFile,NULL))
			{
				TRACE((TRACE_ERROR,_F_,"CreateDirectory(%s)=%d",gszCfgFile,GetLastError()));
				// on continue quand même, sans doute il existe déjà, sinon pas grave ça merdera plus loin
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
		else if (!gbNoMasterPwd || gcszK1[0]=='1') // MODE mot de passe maitre : affichage message bienvenue avec définition du mot de passe maitre
		{
			if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_SIMPLE_PWD_CHOICE),NULL,SimplePwdChoiceDialogProc)!=IDOK) goto end;
		}
		else // mode sans mot de passe
		{
			char szTemp[LEN_PWD+1]; // 4 morceaux de clé x 8 octets + nom d'utilisateur tronqué à 16 octets = 48

			if (gbAdmin) // si défini, demande le mot de passe admin, sinon demande de le définir
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
			swGenPBKDF2Salt(); // génère le sel qui sera pris en compte pour la dérivation de la clé AES et le stockage du mot de passe
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
		// force la migration en SSO Windows si configuré en base de registre
		if (gbPasswordChoiceLevel==PP_WINDOWS)
		{
			// ISSUE#307 : vérifie si passage à nouvelle version (détecté via les actions de swSSOMigration à swSSOSVC, applicable au mode mot de passe Windows uniquement)
			CheckIfUpgraded();
			giPwdProtection=PP_WINDOWS;
		}
		else if (gbPasswordChoiceLevel==PP_ENCRYPTED) // ISSUE#181 (découplage)
		{
			giPwdProtection=PP_ENCRYPTED;
		}
askpwd:
		if (giPwdProtection==PP_ENCRYPTED) // MODE mot de passe maitre
		{
			// Regarde si l'utilisateur utilisait le couplage Windows précédemment
			char szSynchroValue[192+1]; // (16+64+16)*2+1 = 193
			int lenCheckSynchro;
			lenCheckSynchro =GetPrivateProfileString("swSSO","CheckSynchro","",szSynchroValue,sizeof(szSynchroValue),gszCfgFile);
			if (lenCheckSynchro !=0) // ISSUE#181 (découplage) : l'utilisateur était en PP_WINDOWS et passe PP_ENCRYPTED
			{
				// demande le mot de passe à l'utilisateur (pas le choix, pour hasher, il faut le connaitre)
				// bidouille pour se retrouver dans le même cas que le clic sur la loupe et vérifier le mot de passe windows facilement...
				giPwdProtection=PP_WINDOWS; // la remise à la bonne valeur se fait dans WM_COMMAND de PwdDialogProc
				// initialisation des sels
				if (swReadPBKDF2Salt()!=0) goto end;
				// demande du mot de passe
				rc=AskPwd(NULL,TRUE);
				if (rc!=0) goto end; // mauvais mot de passe
				// mise à jour du .ini sans oublier le sceau
				WritePrivateProfileString("swSSO","CheckSynchro",NULL,gszCfgFile); 
				WritePrivateProfileString("swSSO","pwdProtection","ENCRYPTED",gszCfgFile);
				StoreIniEncryptedHash(); // ISSUE#164
			}
			else // L'utilisateur est déjà en mode PP_ENCRYPTED
			{
				// regarde s'il y a une réinit de mdp en cours
				int ret=RecoveryResponse(NULL);
				if (ret==0) // il y a eu une réinit et ça a bien marché :-)
				{ 
					WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); // était plus bas avant (ISSUE#276)
					gbRecoveryRunning=TRUE; // transchiffrement plus tard une fois que les configs sont chargées en mémoire
				}
				else if (ret==-2)  // pas de réinit
				{
					rc=AskPwd(NULL,TRUE);
					if (rc==-1) goto end; // mauvais mot de passe
					else if (rc==-2) goto askpwd; // l'utilisateur a cliqué sur mot de passe oublié
				}
				else if (ret==-1 || ret==-3)  // erreur ou format de response incorrect
				{
					goto askpwd;
				}
				else if (ret==-5)  // l'utilisateur a demandé de regénérer le challenge (ISSUE#121)
				{
					RecoveryChallenge(NULL);
					goto askpwd; // je sais, c'est moche, mais c'est tellement plus simple...
				}
				else // il y a eu une réinit et ça n'a pas marché :-(
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
				SecureZeroMemory(szPwd,LEN_PWD+1); // pas besoin du mdp, c'était juste pour savoir si la clé était présente et valide
				if (ret!=0)
				{
					// N'affiche pas le message qui indique que le mot de passe maitre va être demandé une dernière fois
					// si jamais l'utilisateur avait enregistré son mot de passe
					MessageBox(NULL,GetString(IDS_INFO_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONINFORMATION);
				}
				giPwdProtection=PP_ENCRYPTED; // bidouille pour avoir le bon message dans la fenêtre AskPwd...
				if (AskPwd(NULL,TRUE)!=0) goto end;
				giPwdProtection=PP_WINDOWS;
				bMigrationWindowsSSO=TRUE; // On note de faire la migration (se fait plus tard une fois les configs chargées)
			}
			else if (lenConfigHashedPwd==0) // L'utilisateur est déjà en mode PP_WINDOWS
			{
				// regarde s'il y a une réinit de mdp en cours
				int ret=RecoveryResponse(NULL);
				if (ret==0) // il y a eu une réinit et ça a bien marché :-)
				{ 
					WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); // était plus bas avant (ISSUE#276)
					gbRecoveryRunning=TRUE; // transchiffrement plus tard une fois que les configs sont chargées en mémoire
				}
				else if (ret==-2) // pas de réinit
				{
					// ISSUE#165
					rc=CheckWindowsPwd(&bMigrationWindowsSSO);
					if (rc==-1) // erreur ou pas de recovery ou annulation de l'utilisateur dans le recovery
						goto end;
					else if(rc==-3) // l'utilisateur a cliqué sur continuer dans le recovery
						goto askpwd;
				}
				else if (ret==-1 || ret==-3)  // erreur ou format de response incorrect
				{
					goto askpwd;
				}
				else if (ret==-5)  // ISSUE#165 // l'utilisateur a demandé de regénérer le challenge (ISSUE#121)
				{
					if (RecoveryChallenge(NULL)==0)
						goto askpwd; // je sais, c'est moche, mais c'est tellement plus simple...
					else
						goto end;
				}
				else // il y a eu une réinit et ça n'a pas marché :-(
				{
					goto end;
				}
			}
			else if (lenConfigHashedPwd==296) // L'utilisateur a une vieille version de swSSO (ISSUE#172 : il faut migrer quand même)
			{
				char szPwd[LEN_PWD+1];
				int ret=DPAPIGetMasterPwd(szPwd);
				SecureZeroMemory(szPwd,LEN_PWD+1); // pas besoin du mdp, c'était juste pour savoir si la clé était présente et valide
				if (ret!=0)
				{
					// N'affiche pas le message qui indique que le mot de passe maitre va être demandé une dernière fois
					// si jamais l'utilisateur avait enregistré son mot de passe
					MessageBox(NULL,GetString(IDS_INFO_WINDOWS_SSO_MIGRATION),"swSSO",MB_OK | MB_ICONINFORMATION);
				}
				giPwdProtection=PP_ENCRYPTED; // bidouille
				if (AskPwd(NULL,TRUE)!=0) goto end;
				giPwdProtection=PP_WINDOWS;
				bMigrationWindowsSSO=TRUE; // On note de faire la migration (se fait plus tard une fois les configs chargées)
			}
			else // L'utilisateur a un problème avec son .ini...
			{
				TRACE((TRACE_ERROR,_F_,"lenConfigHashedPwd=%d", lenConfigHashedPwd));
				MessageBox(NULL,GetString(IDS_ERROR_WINDOWS_SSO_VER),"swSSO",MB_OK | MB_ICONSTOP);
				goto end;
			}
		}
		// 0.93B1 : log authentification primaire réussie
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_PRIMARY_LOGIN_SUCCESS,NULL,NULL,NULL,NULL,0);
	}

	// 0.80B9 : lit la config proxy pour ce poste de travail.
	// Remarque : n'est pas fait dans GetConfigHeader car on a besoin de la clé
	//			  dérivée du mot de passe maitre pour déchiffrement le mdp proxy
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
	// 0.92B5 : pour corriger bug catégories perdues en 0.92B3, LoadApplications passe APRES LoadCategories
	// lecture des catégories
	if (LoadCategories()!=0) { iError=-2; goto end; }
	// lecture des applications configurées
	if (LoadApplications()==-1) { iError=-2; goto end; }

	// vérifie la date de dernier changement de mot de passe
	// attention, comme il y a transchiffrement des id&pwd et des mdp proxy, il 
	// faut bien que ces infos aient été lues avant un éventuel changement de mot de passe imposé !
	// ISSUE#145 : si jamais on est en phase de migration, il faut le faire plus tard, après la migration
	if (giPwdProtection==PP_ENCRYPTED)
	{
		if (giPwdPolicy_MaxAge!=0 && !gbRecoveryRunning) // ISSUE#178 : ne pas demander le changement de mdp expiré si en cours de recouvrement !
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

	// 0.91 : propose à l'utilisateur de récupérer les configurations disponibles sur le serveur
	TRACE((TRACE_DEBUG,_F_,"giNbActions=%d gbGetAllConfigsAtFirstStart=%d giDomainId=%d",giNbActions,gbGetAllConfigsAtFirstStart,giDomainId));
	if (giNbActions==0 && gbGetAllConfigsAtFirstStart) // CAS DU PREMIER LANCEMENT (ou encore aucune config enregistrée)
	{
		// 1.03 : récupère la liste des domaines
		giNbDomains=GetAllDomains(gtabDomains);
		if (giNbDomains==0)
		{ 
			MessageBox(NULL,GetString(IDS_GET_ALL_CONFIGS_ERROR),"swSSO",MB_OK | MB_ICONEXCLAMATION);
			goto end;
		}
		if (gbAdmin || gbInternetManualPutConfig) // 1.05 : on ne demande pas à l'admin quel est son domaine, il doit pouvoir tous les gérer
		{
			giDomainId=-1;
			strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),"Tous");
			SaveConfigHeader();
			GetAllConfigsFromServer();
		}
		else
		{
			if (giDomainId==1 && giNbDomains!=0) // domaine non renseigné dans le .ini 
			{
				int ret= SelectDomain();
				// ret:  0 - OK, l'utilisateur a choisi, le domaine est renseigné dans giDomainId et gszDomainLabel
				//    :  1 - Il n'y avait qu'un seul domaine, l'utilisateur n'a rien vu mais le domaine est bien renseigné
				//    :  2 - L'utilisateur a annulé
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
	else // CAS DES LANCEMENTS ULTERIEURS (avec configurations déjà enregistrées)
	{
		BOOL bOK=TRUE;
		// 1.03 : récupère la liste des domaines (doit être fait dans tous les cas pour alimenter le menu Upload)
		// mais si échoue, ne doit pas être bloquant ni générer de message d'erreur (mode déconnecté)
		// Pour ne pas générer une requête inutile, on ne fait que pour les utilisateurs qui ont le droit d'utiliser le menu upload
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
		// ISSUE#310 : récupère les configurations en autopublish, si demandé
		if (gbGetAutoPublishedConfigsAtStart) GetAutoPublishConfigsFromServer();

		// 0.91 : si demandé, récupère les nouvelles configurations et/ou les configurations modifiées
		if (gbGetNewConfigsAtStart || gbGetModifiedConfigsAtStart)
		{
			if (GetNewOrModifiedConfigsFromServer(gbAdmin)!=0) bOK=FALSE;
		}
		// ISSUE#214
		if (gbRemoveDeletedConfigsAtStart) // réalise une synchro complète en supprimant les configs qui ne sont plus présentes sur le serveur
		{
			if (DeleteConfigsNotOnServer()!=0) bOK=FALSE;
		}
		if (bOK) ReportConfigSync(0,gbDisplayConfigsNotifications,gbAdmin);
	}
	
	// ISSUE#59 : ce code était avant dans LoadCategories().
	// Déplacé dans winmain pour ne pas l'exécuter si des catégories ont été récupérées depuis le serveur
	if (giNbCategories==0) // si aucune catégorie, crée la catégorie "non classé"
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
	
	// 1.03 : si configuré pour utiliser le mot de passe AD comme mot de passe secondaire (%ADPASSWORD%),
	//        vérifie que la date de dernier changement de mot de passe AD et le cas échéant demande à 
	//        l'utilisateur de le saisir
	if (!gbAdmin) // 1.07 : ne le demande pas en mode admin
	{
		if (gbUseADPasswordForAppLogin) CheckADPwdChange(); // ne doit pas être bloquant si échoue, car peut être lié à AD non joignable par ex.
	}
	
	// inscription pour réception des notifs de verrouillage de session
	gbRegisterSessionNotification=WTSRegisterSessionNotification(gwMain,NOTIFY_FOR_THIS_SESSION);
	TRACE((TRACE_DEBUG,_F_,"WTSRegisterSessionNotification() -> OK"));
	if (!gbRegisterSessionNotification)
	{
		// cause possible de l'échec : "If this function is called before the dependent services 
		// of Terminal Services have started, an RPC_S_INVALID_BINDING error code may be returned"
		// Du coup l'idée est de réessayer plus tard (1 minute) avec un timer
		TRACE((TRACE_ERROR,_F_,"WTSRegisterSessionNotification()=%ld [REESSAI DANS 15 SECONDES]",GetLastError()));
		giRegisterSessionNotificationTimer=SetTimer(NULL,0,15000,RegisterSessionNotificationTimerProc);
		giNbRegisterSessionNotificationTries++;
	}

	// 0.80 si demandé, vérification des mises à jour sur internet
	if (gbInternetCheckVersion) InternetCheckVersion();

	// 0.42 premiere utilisation (fichier vide) => affichage fenêtre config
	// 0.80 -> n'est plus nécessaire... on peut configurer sans ouvrir cette fenêtre ("ajouter cette application")
	// if (giNbActions==0) ShowConfig(0);

	// 0.93B4 : si gbParseWindowsOnStart=FALSE, ajoute toutes les fenêtres ouvertes et visibles dans la liste des fenêtres
	if (!gbParseWindowsOnStart) ExcludeOpenWindows();

	// ISSUE#145
	// supprimé en 1.15, était toujours FALSE !
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
		strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Synchronisation de mot de passe réussie");
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
		// WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); déplacé plus haut (ISSUE#276)
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
		// ISSUE#247 : passage du username en majuscule pour éviter les pb de différences de casse (vu avec POA Sophos)
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
		// ISSUE#169 : Demande le mot de passe à swSSOSVC et le stocke pour répondre aux demandes ultérieures traitées par GetDecryptedPwd() dans swSSOAD.cpp
		if (GetADPassword()!=0) { iError=-1; goto end; }
		// ISSUE#342 Si mode admin, login de l'admin sur le serveur (non bloquant + message d'erreur dans la fonction ServerAdminLogin)
		if (gbAdmin && !gbNoMasterPwd) ServerAdminLogin(NULL,gszUserName,NULL);

		// 1.08 ISSUE#248 : si configuré, synchronise un groupe de mot de passe secondaires avec le mot de passe AD
		if (!gbAdmin && gbSyncSecondaryPasswordActive)
		{
			if (CheckUserInOU()) SyncSecondaryPasswordGroup();
		}
	}

	// ISSUE#351 : le fait de ne pas activer le timer faisait que l'evt de changement de mdp windows n'était pas capté !
	// if (!gbAdmin)
	{
		if (LaunchTimer()!=0)
		{
			iError=-1;
			goto end;
		}
	}
	// Si -launchapp, ouvre la fenêtre ShowAppNsites
	if (bLaunchApp) ShowLaunchApp();

	// Ici on peut considérer que swSSO est bien démarré et que l'utilisateur est connecté
	// Prise de la date de login pour les stats
	GetLocalTime(&gLastLoginTime);

	// déclenchement du timer de refresh des droits, si demandé
	if (giRefreshRightsFrequency!=0)
	{
		giRefreshRightsTimer=SetTimer(NULL,0,giRefreshRightsFrequency*60*1000,RefreshRightsTimerProc);
	}

	// déclenchement du timer pour enumération de fenêtres toutes les 500ms
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
		// quitte sans mettre à jour les stats et sans message d'erreur
	}
	else
	{
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_QUIT,NULL,NULL,NULL,NULL,0);
		if (giStat!=0 && !gbAdmin) swStat(); // 0.99 - ISSUE#106 + ISSUE#244
	}
	if (gpIUIAutomation!=NULL) gpIUIAutomation->Release();
	// on libère tout avant de terminer
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
	if (gwMain!=NULL) DestroyWindow(gwMain); // 0.90 : par contre c'est bien de détruire gwMain (peut-être à l'origine du bug de non verrouillage JMA ?)
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
