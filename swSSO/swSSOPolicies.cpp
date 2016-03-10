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
// swSSOPolicies.cpp
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#include "stdafx.h"

// REGKEY_GLOBAL_POLICY
BOOL gbEnableOption_Portal=TRUE;
BOOL gbEnableOption_ViewIni=TRUE;
BOOL gbEnableOption_OpenIni=TRUE;
BOOL gbEnableOption_ShowOptions=TRUE;
BOOL gbEnableOption_SessionLock=TRUE;
BOOL gbEnableOption_CheckVersion=TRUE;
BOOL gbEnableOption_GetConfig=TRUE;
//BOOL gbEnableOption_PutConfig=TRUE;
BOOL gbEnableOption_ManualPutConfig=TRUE;
BOOL gbEnableOption_Proxy=TRUE;
BOOL gbEnableOption_SavePassword=TRUE;
BOOL gbEnableOption_ShowPassword=TRUE;
int gbPasswordChoiceLevel=PP_ENCRYPTED;
BOOL gbEnableOption_ViewAppConfig=TRUE;
BOOL gbEnableOption_ModifyAppConfig=TRUE;
BOOL gbErrorServerConfigNotFoundDefaultMessage=TRUE;
BOOL gbShowMenu_ChangeCategIds=TRUE;
BOOL gbShowMenu_LaunchApp=TRUE;
BOOL gbShowMenu_AddApp=TRUE;
// ISSUE#84 : ajout de nouvelles options de restriction de l'IHM (items de menu categorie et appli)
BOOL gbShowMenu_EnableDisable=TRUE;
BOOL gbShowMenu_Rename=TRUE;
BOOL gbShowMenu_Move=TRUE;
BOOL gbShowMenu_Delete=TRUE;
BOOL gbShowMenu_AddCateg=TRUE;
BOOL gbShowMenu_Duplicate=TRUE;
BOOL gbShowMenu_AddAccount=TRUE;
// ISSUE#99 : ajout de gbShowMenu_AddThisApp pour dissocier gbShowMenu_AddApp
BOOL gbShowMenu_AddThisApp=TRUE;
// ISSUE#107
BOOL gbShowMenu_AppPasswordMenu=FALSE;
BOOL gbOldPwdAutoFill=FALSE;
// ISSUE#140
BOOL gbReactivateWithoutPwd=FALSE;
// ISSUE#164
BOOL gbCheckIniIntegrity=FALSE;					// ISSUE#164 - vérifie l'intégrité du fichier .ini
BOOL gbNoMasterPwd=FALSE; 
// ISSUE#180
BOOL gbShowAutoLockOption=TRUE;
// ISSUE#183
BOOL gbEnableOption_ShowBrowsers=TRUE;
BOOL gbShowMenu_UploadWithIdPwd=FALSE;			// 1.03 - active le menu "Uploader avec identifiant et mot de passe"
// ISSUE#204
BOOL gbShowMenu_RefreshRights=FALSE;
// ISSUE#256
int giShowPasswordGroup=2;
// ISSUE#257
BOOL gbShowMenu_Quit=TRUE;

// REGKEY_PASSWORD_POLICY
int giPwdPolicy_MinLength=0;
int giPwdPolicy_MinLetters=0;
int giPwdPolicy_MinUpperCase=0;
int giPwdPolicy_MinLowerCase=0;
int giPwdPolicy_MinNumbers=0;
int giPwdPolicy_MinSpecialsChars=0;
int giPwdPolicy_MaxAge=0;
int giPwdPolicy_MinAge=0;
int giPwdPolicy_IdMaxCommonChars=0;
char gszPwdPolicy_Message[1024+1];
int giPwdPolicy_MinRules=0;

// REGKEY_ENTERPRISE_OPTIONS
char gszServerAddress[128+1];
char gszWebServiceAddress[256+1];
char gszErrorMessageIniFile[1024+1];       	// 0.88 : message d'erreur en cas de corruption swsso.ini
char gszErrorServerNotAvailable[1024+1];   	// 0.90 : serveur non joignable / pb config proxy
char gszErrorServerConfigNotFound[1024+1]; 	// 0.90 : configuration demandée non trouvée
BOOL gbCategoryManagement=FALSE;  		   	// 0.91 : prise en compte des catégories dans putconfig et getconfig
BOOL gbGetAllConfigsAtFirstStart=FALSE;		// 0.91 : propose à l'utilisateur de récupérer toutes les config au 1er lancement
BOOL gbGetNewConfigsAtStart=FALSE;			// 0.91 : récupère les nouvelles configurations à chaque démarrage
BOOL gbGetModifiedConfigsAtStart=FALSE;		// 0.91 : récupère les configurations modifiées à chaque démarrage
BOOL gbDisableArchivedConfigsAtStart=FALSE;	// 0.91 : désactive localement les configurations archivées en central à chaque démarrage
BOOL gbAllowManagedConfigsDeletion=TRUE;   	// 0.91 : autorise l'utilisateur à supprimer des configurations managées (cad. avec un id provenant du central)
BOOL gbActivateNewConfigs=FALSE;			// 0.91 : active les configurations récupérées sur le serveur au démarrage
BOOL gbDisplayConfigsNotifications=TRUE;	// 0.92 : affiche les messages de notification d’ajout / modification / suppression des configurations au démarrage (ISSUE#26)
BOOL gbWindowsEventLog=FALSE;				// 0.93 : log dans le journal d'événements de Windows
char gszLogFileName[_MAX_PATH+1];			// 0.93 : chemin complet du fichier de log
int  giLogLevel=LOG_LEVEL_NONE;				// 0.93 : niveau de log
BOOL giStat=0;								// 0.99 : statistiques - ISSUE#106 + ISSUE#244
char gszWelcomeMessage[512+1];				// 1.01 : message de définition du mot de passe maitre dans la fenêtre bienvenue - ISSUE#146
int  giMaxConfigs=500;						// 1.01 : nb max de configurations - ISSUE#149
BOOL gbServerHTTPS=FALSE;						// 1.03 - ISSUE#162
int  giServerPort=INTERNET_DEFAULT_HTTP_PORT;	// 1.03 - ISSUE#162
BOOL gbUseADPasswordForAppLogin=FALSE;			// 1.03 - permet d'utiliser %ADPASSWORD% dans le champ mot de passe (n'utilise pas (encore) swSSOCM --> le mdp AD est demandé à l'utilisateur)
BOOL gbDisplayWindowsPasswordChange=TRUE;	// 1.05 - affiche / masque le message affiché lors du changement de mot de passe windows (en mode chaîné)
BOOL gbCategoryAutoUpdate=FALSE;			// 1.06 - ISSUE#206 : met à jour la catégorie sur le serveur lorsqu'une application est déplacée dans l'IHM client
BOOL gbRemoveDeletedConfigsAtStart=FALSE;	// 1.07 - ISSUE#214
BOOL gbAdminDeleteConfigsOnServer=FALSE;	// 1.07 - ISSUE#223
int giRefreshRightsFrequency=0;				// 1.07 - ISSUE#220
BOOL gbAllowManagedConfigsModification=TRUE;	// 1.07 : ISSUE#238
BOOL gbRecoveryWebserviceActive=FALSE;			// 1.08
char gszRecoveryWebserviceServer[128+1];		// 1.08
char gszRecoveryWebserviceURL[255+1];			// 1.08
int  giRecoveryWebservicePort=INTERNET_DEFAULT_HTTP_PORT;	// 1.08
int  giRecoveryWebserviceTimeout=10;			// 1.08
BOOL gbRecoveryWebserviceHTTPS=FALSE;			// 1.08
BOOL gbRecoveryWebserviceManualBackup=TRUE;		// 1.08
BOOL gbSyncSecondaryPasswordActive=FALSE;		// 1.08
int  giSyncSecondaryPasswordGroup=-1;			// 1.08
char gszSyncSecondaryPasswordOU[255+1];			// 1.08
BOOL gbCheckCertificates=TRUE;					// 1.08 - ISSUE#252
char gszConfigNotFoundMailTo[128+1];			// 1.08
char *gpszConfigNotFoundMailSubject=NULL;		// 1.08
char *gpszConfigNotFoundMailBody=NULL;			// 1.08
int	 giWaitBeforeNewSSO=WAIT_IF_SSO_OK;			// 1.08 - ISSUE#253
int  giRecoveryWebserviceNbTry=1;				// 1.10 - ISSUE#275
int  giRecoveryWebserviceWaitBeforeRetry=1000;	// 1.10 - ISSUE#275

// REGKEY_EXCLUDEDWINDOWS_OPTIONS (#110)
char gtabszExcludedWindows[MAX_EXCLUDED_WINDOWS][LEN_EXCLUDED_WINDOW_TITLE+1];
int  giNbExcludedWindows=0;

// REGKEY_DEFAULTINIVALUES
BOOL gbSessionLock_DefaultValue=FALSE;				// 1.04
BOOL gbInternetCheckVersion_DefaultValue=TRUE;		// 1.04
BOOL gbInternetCheckBeta_DefaultValue=FALSE;		// 1.04
BOOL gbInternetGetConfig_DefaultValue=FALSE;		// 1.04
BOOL gbInternetManualPutConfig_DefaultValue=FALSE;	// 1.04
char gszCfgPortal_DefaultValue[_MAX_PATH+1]="";		// 1.04
BOOL gbLaunchTopMost_DefaultValue=FALSE;			// 1.04
BOOL gbParseWindowsOnStart_DefaultValue=TRUE;		// 1.04
int  giDomainId_DefaultValue=1;						// 1.04
char gszDomainLabel_DefaultValue[LEN_DOMAIN+1]="";	// 1.04
BOOL gbDisplayChangeAppPwdDialog_DefaultValue=TRUE;	// 1.04
BOOL gbSSOInternetExplorer_DefaultValue=TRUE;		// 1.04
BOOL gbSSOFirefox_DefaultValue=TRUE;				// 1.04
BOOL gbSSOChrome_DefaultValue=TRUE;					// 1.04
BOOL gbShowLaunchAppWithoutCtrl_DefaultValue=FALSE;	// 1.08

// REGKEY_REGKEY_PWDGROUP_COLORS
COLORREF gtabPwdGroupColors[MAX_COLORS];
int giNbPwdGroupColors;

char gszPastePwd_Text[100];

//-----------------------------------------------------------------------------
// LoadPolicies()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void LoadPolicies(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc;
	HKEY hKey=NULL;
	char szValue[1024+1];
	DWORD dwValue,dwValueSize,dwValueType;

	// valeurs par défaut pour les chaines de caractères
	// les valeurs par défaut pour les DWORD sont initialisées dans la déclaration des variables globales
	strcpy_s(gszPwdPolicy_Message,sizeof(gszPwdPolicy_Message),GetString(IDS_PASSWORD_POLICY_MESSAGE));
	strcpy_s(gszServerAddress,sizeof(gszServerAddress),"ws.swsso.fr");
	strcpy_s(gszWebServiceAddress,sizeof(gszWebServiceAddress),"/webservice5.php"); 
	strcpy_s(gszErrorMessageIniFile,sizeof(gszErrorMessageIniFile),GetString(IDS_ERROR_MESSAGE_INI_FILE));
	strcpy_s(gszErrorServerNotAvailable,sizeof(gszErrorServerNotAvailable),GetString(IDS_CONFIG_PROXY));
	strcpy_s(gszErrorServerConfigNotFound,sizeof(gszErrorServerConfigNotFound),GetString(IDS_CONFIG_NOT_FOUND));
	*gszLogFileName=0;
	*gszWelcomeMessage=0;
	*gszPastePwd_Text=0;
	*gszRecoveryWebserviceServer=0;
	*gszRecoveryWebserviceURL=0;
	*gszSyncSecondaryPasswordOU=0;
	gpszConfigNotFoundMailSubject=NULL;
	gpszConfigNotFoundMailBody=NULL;

	//--------------------------------------------------------------
	// GLOBAL POLICY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_GLOBAL_POLICY_ADMIN:REGKEY_GLOBAL_POLICY,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_PORTAL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_Portal=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_VIEWINI,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ViewIni=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_OPENINI,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_OpenIni=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_SHOWOPTIONS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ShowOptions=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_SESSIONLOCK,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_SessionLock=(BOOL)dwValue; 
		
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_CHECKVERSION,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_CheckVersion=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_GETCONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_GetConfig=(BOOL)dwValue; 

		//dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		//rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_PUTCONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		//if (rc==ERROR_SUCCESS) gbEnableOption_PutConfig=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_MANUALPUTCONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ManualPutConfig=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_PROXY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_Proxy=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_SAVEPASSWORD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_SavePassword=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_SHOWPASSWORD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ShowPassword=(BOOL)dwValue; 

		// ISSUE#83 : on interdit les modes où le mot de passe maître n'est pas sécurisé
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORDCHOICELEVEL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbPasswordChoiceLevel=(BOOL)dwValue;
		if (gbPasswordChoiceLevel!=PP_WINDOWS && gbPasswordChoiceLevel!=PP_ENCRYPTED) gbPasswordChoiceLevel=PP_ENCRYPTED;

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_VIEWAPPCONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ViewAppConfig=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_MODIFYAPPCONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ModifyAppConfig=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_CHANGECATEGIDS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_ChangeCategIds=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_LAUNCHAPP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_LaunchApp=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_ADDAPP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_AddApp=(BOOL)dwValue; 

		// ISSUE#84 : ajout de nouvelles options de restriction de l'IHM (items de menu categorie et appli)
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_ENABLEDISABLE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_EnableDisable=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_RENAME,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_Rename=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_MOVE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_Move=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_DELETE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_Delete=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_ADDCATEG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_AddCateg=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_DUPLICATE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_Duplicate=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_ADDACCOUNT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_AddAccount=(BOOL)dwValue; 

		// ISSUE#99 : ajout de gbShowMenu_AddThisApp pour dissocier gbShowMenu_AddApp
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_ADDTHISAPP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_AddThisApp=(BOOL)dwValue; 

		// ISSUE#107
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_CHANGEAPPPASSWORD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_AppPasswordMenu=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_OLD_PWD_AUTO_FILL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbOldPwdAutoFill=(BOOL)dwValue; 

		// ISSUE#140
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_REACTIVATE_WITHOUT_PWD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbReactivateWithoutPwd=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_UPLOAD_WITH_ID_PWD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_UploadWithIdPwd=(BOOL)dwValue; 

		// ISSUE#164
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_CHECK_INI_INTEGRITY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbCheckIniIntegrity=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_NO_MASTER_PASSWORD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbNoMasterPwd=(BOOL)dwValue; 

		// ISSUE#180
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOW_AUTO_LOCK_OPTION,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowAutoLockOption=(BOOL)dwValue; 

		// ISSUE#183
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ENABLEOPTION_SHOWBROWSERS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbEnableOption_ShowBrowsers=(BOOL)dwValue; 

		// ISSUE#204
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_REFRESH_RIGHTS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_RefreshRights=(BOOL)dwValue; 

		// ISSUE#256
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOW_PASSWORD_GROUP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giShowPasswordGroup=dwValue; 

		// ISSUE#257
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SHOWMENU_QUIT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowMenu_Quit=(BOOL)dwValue; 

		RegCloseKey(hKey);
	}

	//--------------------------------------------------------------
	// PASSWORD POLICY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_PASSWORD_POLICY_ADMIN:REGKEY_PASSWORD_POLICY,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLENGTH,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLength=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLETTERS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLetters=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINUPPERCASE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinUpperCase=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLOWERCASE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLowerCase=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINNUMBERS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinNumbers=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINSPECIALCHARS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinSpecialsChars=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MAXAGE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MaxAge=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINAGE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinAge=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_IDMAXCOMMONCHARS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_IdMaxCommonChars=(int)dwValue+1; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MESSAGE,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszPwdPolicy_Message,sizeof(gszPwdPolicy_Message),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINRULES,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinRules=(int)dwValue; 

		RegCloseKey(hKey);
	}

	//--------------------------------------------------------------
	// ENTERPRISE OPTIONS
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_ENTERPRISE_OPTIONS_ADMIN:REGKEY_ENTERPRISE_OPTIONS,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SERVER_ADDRESS,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszServerAddress,sizeof(gszServerAddress),szValue);
			
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_WEBSERVICE_ADDRESS,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszWebServiceAddress,sizeof(gszWebServiceAddress),szValue);
		
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ERROR_MESSAGE_INI_FILE,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszErrorMessageIniFile,sizeof(gszErrorMessageIniFile),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_KEYID,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRecoveryKeyId=(int)dwValue; 

		dwValueType=REG_BINARY; dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_KEYVALUE,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
		{
			gdwRecoveryKeyLen=dwValueSize;
			gpRecoveryKeyValue=(BYTE*)malloc(gdwRecoveryKeyLen);
			if (gpRecoveryKeyValue==NULL) 
			{ 
				TRACE((TRACE_ERROR,_F_,"malloc(%d)",gdwRecoveryKeyLen));
			}
			else
			{
				rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_KEYVALUE,NULL,&dwValueType,gpRecoveryKeyValue,&dwValueSize);
				if (rc!=ERROR_SUCCESS) { free(gpRecoveryKeyValue) ; gpRecoveryKeyValue=NULL; }
			}
		}

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ERROR_SERVER_NOT_AVAILABLE,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszErrorServerNotAvailable,sizeof(gszErrorServerNotAvailable),szValue);

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ERROR_CONFIG_NOT_FOUND,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
		{
			strcpy_s(gszErrorServerConfigNotFound,sizeof(gszErrorServerConfigNotFound),szValue);
			gbErrorServerConfigNotFoundDefaultMessage=FALSE;
		}

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_CATEGORY_MANAGEMENT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbCategoryManagement=(BOOL)dwValue; 
		
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_GET_ALL_CONFIGS_AT_FIRST_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbGetAllConfigsAtFirstStart=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_GET_NEW_CONFIGS_AT_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbGetNewConfigsAtStart=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_GET_MODIFIED_CONFIGS_AT_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbGetModifiedConfigsAtStart=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DISABLE_ARCHIVED_CONFIGS_AT_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbDisableArchivedConfigsAtStart=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ALLOW_MANAGED_CONFIGS_DELETION,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbAllowManagedConfigsDeletion=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ALLOW_MANAGED_CONFIGS_MODIFICATION,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbAllowManagedConfigsModification=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ACTIVATE_NEW_CONFIGS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbActivateNewConfigs=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DISPLAY_CONFIGS_NOTIFICATIONS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbDisplayConfigsNotifications=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_WINDOWS_EVENT_LOG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbWindowsEventLog=(BOOL)dwValue; 
		
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_STAT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giStat=(int)dwValue; 
		
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_LOG_FILE_NAME,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
		{
			// ISSUE#104 et ISSUE#109
			// strcpy_s(gszLogFileName,sizeof(gszLogFileName),szValue);
			ExpandFileName(szValue,gszLogFileName,_MAX_PATH+1);
		}
	
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_LOG_LEVEL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giLogLevel=(int)dwValue; 

		// ISSUE#146
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_WELCOME_MESSAGE,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
		{
			strncpy_s(gszWelcomeMessage,sizeof(gszWelcomeMessage),szValue,sizeof(gszWelcomeMessage)-1);
		}

		// ISSUE#149
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_MAX_CONFIGS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giMaxConfigs=(dwValue<10 ? 10:(int)dwValue); // mini 10 

		// ISSUE#162
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SERVER_PORT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giServerPort=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SERVER_HTTPS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbServerHTTPS=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_USE_AD_PASSWORD,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbUseADPasswordForAppLogin=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DISPLAY_WINDOWS_PASSWORD_CHANGE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbDisplayWindowsPasswordChange=(BOOL)dwValue; 

		// ISSUE#206
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_CATEGORY_AUTO_UPDATE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbCategoryAutoUpdate=(BOOL)dwValue; 

		// ISSUE#214
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_REMOVE_DELETED_CONFIGS_AT_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbRemoveDeletedConfigsAtStart=(BOOL)dwValue; 

		// ISSUE#223
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_ADMIN_DELETE_CONFIGS_ON_SERVER,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbAdminDeleteConfigsOnServer=(BOOL)dwValue; 

		// ISSUE#220
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_REFRESH_RIGHTS_FREQUENCY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRefreshRightsFrequency=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_ACTIVE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbRecoveryWebserviceActive=(BOOL)dwValue; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_SERVER,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszRecoveryWebserviceServer,sizeof(gszRecoveryWebserviceServer),szValue);

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_ADDRESS,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszRecoveryWebserviceURL,sizeof(gszRecoveryWebserviceURL),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_PORT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRecoveryWebservicePort=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_TIMEOUT,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRecoveryWebserviceTimeout=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_HTTPS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbRecoveryWebserviceHTTPS=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_MANUAL_BACKUP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbRecoveryWebserviceManualBackup=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SYNC_SECONDARY_PASSWORD_ACTIVE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbSyncSecondaryPasswordActive=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SYNC_SECONDARY_PASSWORD_GROUP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giSyncSecondaryPasswordGroup=(int)dwValue; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_SYNC_SECONDARY_PASSWORD_OU,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszSyncSecondaryPasswordOU,sizeof(gszSyncSecondaryPasswordOU),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_CHECK_CERTIFICATES,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbCheckCertificates=(BOOL)dwValue; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_CONFIG_NOT_FOUND_MAILTO,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszConfigNotFoundMailTo,sizeof(gszConfigNotFoundMailTo),szValue);

		dwValueType=REG_SZ;
		dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_CONFIG_NOT_FOUND_MAILSUBJECT,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			gpszConfigNotFoundMailSubject=(char*)malloc(dwValueSize);
			if (gpszConfigNotFoundMailSubject==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwValueSize)); goto end; }
			rc=RegQueryValueEx(hKey,REGVALUE_CONFIG_NOT_FOUND_MAILSUBJECT,NULL,&dwValueType,(LPBYTE)gpszConfigNotFoundMailSubject,&dwValueSize);
			if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(MailObject)")); goto end; }
		}

		dwValueType=REG_SZ;
		dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_CONFIG_NOT_FOUND_MAILBODY,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			gpszConfigNotFoundMailBody=(char*)malloc(dwValueSize);
			if (gpszConfigNotFoundMailBody==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwValueSize)); goto end; }
			rc=RegQueryValueEx(hKey,REGVALUE_CONFIG_NOT_FOUND_MAILBODY,NULL,&dwValueType,(LPBYTE)gpszConfigNotFoundMailBody,&dwValueSize);
			if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(MailObject)")); goto end; }
		}

		if (gpszConfigNotFoundMailSubject==NULL)
		{
			gpszConfigNotFoundMailSubject=(char*)malloc(10);
			if (gpszConfigNotFoundMailSubject==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(10)")); goto end; }
			strcpy_s(gpszConfigNotFoundMailSubject,10,"[swSSO]");
		}
		if (gpszConfigNotFoundMailBody==NULL)
		{
			gpszConfigNotFoundMailBody=(char*)malloc(1);
			if (gpszConfigNotFoundMailBody==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(1)")); goto end; }
			*gpszConfigNotFoundMailBody=0;
		}
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_WAIT_BEFORE_NEW_SSO,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giWaitBeforeNewSSO=(int)dwValue; 
		if (giWaitBeforeNewSSO==WAIT_ONE_MINUTE) giWaitBeforeNewSSO++; // pour assurer que giWaitBeforeNewSSO est toujours différent de WAIT_ONE_MINUTE (cf. commentaire dans swSSOAppNsites.h)

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_NB_TRY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRecoveryWebserviceNbTry=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_RECOVERY_WEBSERVICE_WAIT_BEFORE_RETRY,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giRecoveryWebserviceWaitBeforeRetry=(int)dwValue; 


		RegCloseKey(hKey);
	}
	//--------------------------------------------------------------
	// EXCLUDED WINDOWS
	//--------------------------------------------------------------
	giNbExcludedWindows=0;
	ZeroMemory(gtabszExcludedWindows,sizeof(gtabszExcludedWindows));
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_EXCLUDEDWINDOWS_ADMIN:REGKEY_EXCLUDEDWINDOWS,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		char sz[2+1];
		rc=0;
		while (giNbExcludedWindows<MAX_EXCLUDED_WINDOWS && rc==0)
		{
			wsprintf(sz,"%d",giNbExcludedWindows);
			dwValueType=REG_SZ;
			dwValueSize=sizeof(szValue);
			rc=RegQueryValueEx(hKey,sz,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
			if (rc==ERROR_SUCCESS)
			{
				strcpy_s(gtabszExcludedWindows[giNbExcludedWindows],LEN_EXCLUDED_WINDOW_TITLE+1,szValue);
				giNbExcludedWindows++;
			}
		}
		RegCloseKey(hKey);
	}
	//--------------------------------------------------------------
	// DEFAULT INI VALUES
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_DEFAULTINIVALUES_ADMIN:REGKEY_DEFAULTINIVALUES,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_SESSION_LOCK,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbSessionLock_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_CHECK_VERSION,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbInternetCheckVersion_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_CHECK_BETA,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbInternetCheckBeta_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_GET_CONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbInternetGetConfig_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_PUT_CONFIG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbInternetManualPutConfig_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_PORTAL,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszCfgPortal_DefaultValue,sizeof(gszCfgPortal_DefaultValue),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_LAUNCH_TOPMOST,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbLaunchTopMost_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_PARSE_ON_START,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbParseWindowsOnStart_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_DOMAIN_ID,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giDomainId_DefaultValue=(int)dwValue; 
		
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_DOMAIN_LABEL,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszDomainLabel_DefaultValue,sizeof(gszDomainLabel_DefaultValue),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_DISPLAY_CHANGE_APP,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbDisplayChangeAppPwdDialog_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_INTERNET_EXPLORER,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbSSOInternetExplorer_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_FIREFOX,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbSSOFirefox_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_CHROME,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbSSOChrome_DefaultValue=(BOOL)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_DEFAULT_SHOW_LAUNCHAPP_WITHOUT_CTRL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbShowLaunchAppWithoutCtrl_DefaultValue=(BOOL)dwValue; 

		RegCloseKey(hKey);
	}
	//--------------------------------------------------------------
	// REGKEY_HOTKEY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_HOTKEY_ADMIN:REGKEY_HOTKEY,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
	
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASTEPWD_TEXT,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszPastePwd_Text,sizeof(gszPastePwd_Text),szValue);

		RegCloseKey(hKey);
	}
	//--------------------------------------------------------------
	// PWD GROUP COLORS
	//--------------------------------------------------------------
	ZeroMemory(gtabPwdGroupColors,sizeof(gtabPwdGroupColors));
	// initialisation avec 5 couleurs par défaut
	gtabPwdGroupColors[0]=RGB(255,255,128);
	gtabPwdGroupColors[1]=RGB(128,255,128);
	gtabPwdGroupColors[2]=RGB(128,128,255);
	gtabPwdGroupColors[3]=RGB(255,128,128);
	gtabPwdGroupColors[4]=RGB(128,192,192);
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_PWDGROUP_COLORS_ADMIN:REGKEY_PWDGROUP_COLORS,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		char szId[2+1];
		rc=0;
		char *pszR=NULL,*pszG=NULL,*pszB=NULL,*pContext=NULL;
		while (giNbPwdGroupColors<MAX_COLORS && rc==0)
		{
			wsprintf(szId,"%d",giNbPwdGroupColors);
			dwValueType=REG_SZ;
			dwValueSize=sizeof(szValue);
			rc=RegQueryValueEx(hKey,szId,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
			if (rc==ERROR_SUCCESS)
			{
				if (dwValueSize > LEN_COLOR_STRING+1){ TRACE((TRACE_ERROR,_F_,"Longueur PwdGroupColor #%d incorrecte (%d)",giNbPwdGroupColors,dwValueSize)) ; goto suite;}
				pszR=strtok_s(szValue,",;",&pContext);
				if (pszR==NULL) { TRACE((TRACE_ERROR,_F_,"PwdGroupColor #%d format incorrect (%s)",giNbPwdGroupColors,szValue)) ; goto suite;}
				pszG=strtok_s(NULL,",;",&pContext);
				if (pszG==NULL) { TRACE((TRACE_ERROR,_F_,"PwdGroupColor #%d format incorrect (%s)",giNbPwdGroupColors,szValue)) ; goto suite;}
				pszB=strtok_s(NULL,",;",&pContext);
				if (pszB==NULL) { TRACE((TRACE_ERROR,_F_,"PwdGroupColor #%d format incorrect (%s)",giNbPwdGroupColors,szValue)) ; goto suite;}
				gtabPwdGroupColors[giNbPwdGroupColors]=RGB(atoi(pszR),atoi(pszG),atoi(pszB));
				giNbPwdGroupColors++;
			}
		}
		RegCloseKey(hKey);
suite:;
	}
	giNbPwdGroupColors=max(giNbPwdGroupColors,5);
#ifdef TRACES_ACTIVEES
	int i;
	if (gbAdmin)
	{
		TRACE((TRACE_INFO,_F_,"Lecture dans les clés ADMIN --------------"));
	}
	TRACE((TRACE_INFO,_F_,"GLOBAL POLICY --------------"));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_Portal=%d"		,gbEnableOption_Portal));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ShowOptions=%d"	,gbEnableOption_ShowOptions));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_SessionLock=%d"	,gbEnableOption_SessionLock));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_CheckVersion=%d"	,gbEnableOption_CheckVersion));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_GetConfig=%d"		,gbEnableOption_GetConfig));
	//TRACE((TRACE_INFO,_F_,"gbEnableOption_PutConfig=%d"		,gbEnableOption_PutConfig));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ManualPutConfig=%d",gbEnableOption_ManualPutConfig));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_Proxy=%d"			,gbEnableOption_Proxy));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_SavePassword=%d"	,gbEnableOption_SavePassword));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ShowPassword=%d"	,gbEnableOption_ShowPassword));
	TRACE((TRACE_INFO,_F_,"gbPasswordChoiceLevel=%d"		,gbPasswordChoiceLevel));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ViewAppConfig=%d"	,gbEnableOption_ViewAppConfig));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ModifyAppConfig=%d",gbEnableOption_ModifyAppConfig));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_ChangeCategIds=%d"	,gbShowMenu_ChangeCategIds));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_LaunchApp=%d"			,gbShowMenu_LaunchApp));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_AddApp=%d"			,gbShowMenu_AddApp));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_EnableDisable=%d"		,gbShowMenu_EnableDisable));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_Rename=%d"			,gbShowMenu_Rename));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_Move=%d"				,gbShowMenu_Move));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_Delete=%d"			,gbShowMenu_Delete));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_AddCateg=%d"			,gbShowMenu_AddCateg));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_Duplicate=%d"			,gbShowMenu_Duplicate));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_AddAccount=%d"		,gbShowMenu_AddAccount));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_AddThisApp=%d"		,gbShowMenu_AddThisApp));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_AppPasswordMenu=%d"	,gbShowMenu_AppPasswordMenu));
	TRACE((TRACE_INFO,_F_,"gbOldPwdAutoFill=%d"		        ,gbOldPwdAutoFill));
	TRACE((TRACE_INFO,_F_,"gbReactivateWithoutPwd=%d"		,gbReactivateWithoutPwd));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_UploadWithIdPwd=%d"	,gbShowMenu_UploadWithIdPwd));
	TRACE((TRACE_INFO,_F_,"gbNoMasterPwd=%d"				,gbNoMasterPwd));
	TRACE((TRACE_INFO,_F_,"gbCheckIniIntegrity=%d"			,gbCheckIniIntegrity));
	TRACE((TRACE_INFO,_F_,"gbShowAutoLockOption=%d"			,gbShowAutoLockOption));
	TRACE((TRACE_INFO,_F_,"gbEnableOption_ShowBrowsers=%d"	,gbEnableOption_ShowBrowsers));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_RefreshRights=%d"		,gbShowMenu_RefreshRights));
	TRACE((TRACE_INFO,_F_,"giShowPasswordGroup=%d"			,giShowPasswordGroup));
	TRACE((TRACE_INFO,_F_,"gbShowMenu_Quit=%d"				,gbShowMenu_Quit));
	TRACE((TRACE_INFO,_F_,"PASSWORD POLICY-------------"));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLength=%d"		,giPwdPolicy_MinLength));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLetters=%d"		,giPwdPolicy_MinLetters));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinUpperCase=%d"		,giPwdPolicy_MinUpperCase));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLowerCase=%d"		,giPwdPolicy_MinLowerCase));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinNumbers=%d"		,giPwdPolicy_MinNumbers));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinSpecialsChars=%d"	,giPwdPolicy_MinSpecialsChars));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MaxAge=%d"			,giPwdPolicy_MaxAge));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinAge=%d"			,giPwdPolicy_MinAge));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_IdMaxCommonChars=%d"	,giPwdPolicy_IdMaxCommonChars));
	TRACE((TRACE_INFO,_F_,"ENTERPRISE OPTIONS ---------"));
	TRACE((TRACE_INFO,_F_,"gszServerAddress=%s"				,gszServerAddress));
	TRACE((TRACE_INFO,_F_,"gszServerPort=%d"				,giServerPort));
	TRACE((TRACE_INFO,_F_,"gszServerHTTPS=%d"				,gbServerHTTPS));
	TRACE((TRACE_INFO,_F_,"gszWebServiceAddress=%s"			,gszWebServiceAddress));
	TRACE((TRACE_INFO,_F_,"gszErrorMessageIniFile=%s"		,gszErrorMessageIniFile));
	TRACE((TRACE_INFO,_F_,"giRecoveryKeyId=%04d"			,giRecoveryKeyId));
	TRACE((TRACE_INFO,_F_,"gbGetAllConfigsAtFirstStart=%d"	,gbGetAllConfigsAtFirstStart));
	TRACE((TRACE_INFO,_F_,"gbGetNewConfigsAtStart=%d"		,gbGetNewConfigsAtStart));
	TRACE((TRACE_INFO,_F_,"gbGetModifiedConfigsAtStart=%d"	,gbGetModifiedConfigsAtStart));
	TRACE((TRACE_INFO,_F_,"gbDisableArchivedConfigsAtStart=%d"	,gbDisableArchivedConfigsAtStart));
	TRACE((TRACE_INFO,_F_,"gbAllowManagedConfigsDeletion=%d"	,gbAllowManagedConfigsDeletion));
	TRACE((TRACE_INFO,_F_,"gbActivateNewConfigs=%d"				,gbActivateNewConfigs));
	TRACE((TRACE_INFO,_F_,"gbDisplayConfigsNotifications=%d"	,gbDisplayConfigsNotifications));
	TRACE((TRACE_INFO,_F_,"giLogLevel=%d"						,giLogLevel));
	TRACE((TRACE_INFO,_F_,"gszLogFileName=%s"					,gszLogFileName));
	TRACE((TRACE_INFO,_F_,"gbWindowsEventLog=%d"				,gbWindowsEventLog));
	TRACE((TRACE_INFO,_F_,"giStat=%d"							,giStat));
	TRACE((TRACE_INFO,_F_,"giMaxConfigs=%d"						,giMaxConfigs));
	TRACE((TRACE_INFO,_F_,"gbUseADPasswordForAppLogin=%d"		,gbUseADPasswordForAppLogin));
	TRACE((TRACE_INFO,_F_,"gbDisplayWindowsPasswordChange=%d"	,gbDisplayWindowsPasswordChange));
	TRACE((TRACE_INFO,_F_,"gbCategoryAutoUpdate=%d"				,gbCategoryAutoUpdate));
	TRACE((TRACE_INFO,_F_,"gbRemoveDeletedConfigsAtStart=%d"	,gbRemoveDeletedConfigsAtStart));
	TRACE((TRACE_INFO,_F_,"gbAdminDeleteConfigsOnServer=%d"		,gbAdminDeleteConfigsOnServer));
	TRACE((TRACE_INFO,_F_,"giRefreshRightsFrequency=%d"			,giRefreshRightsFrequency));
	TRACE((TRACE_INFO,_F_,"gbAllowManagedConfigsModification=%d",gbAllowManagedConfigsModification));
	TRACE((TRACE_INFO,_F_,"gbRecoveryWebserviceActive=%d"		,gbRecoveryWebserviceActive));
	TRACE((TRACE_INFO,_F_,"gszRecoveryWebserviceServer=%s"		,gszRecoveryWebserviceServer));
	TRACE((TRACE_INFO,_F_,"gszRecoveryWebserviceURL=%s"			,gszRecoveryWebserviceURL));
	TRACE((TRACE_INFO,_F_,"giRecoveryWebserviceTimeout=%d"		,giRecoveryWebserviceTimeout));
	TRACE((TRACE_INFO,_F_,"gbRecoveryWebserviceHTTPS=%d"		,gbRecoveryWebserviceHTTPS));
	TRACE((TRACE_INFO,_F_,"gbRecoveryWebserviceManualBackup=%d"	,gbRecoveryWebserviceManualBackup));
	TRACE((TRACE_INFO,_F_,"giRecoveryWebserviceNbTry=%d"		,giRecoveryWebserviceNbTry));
	TRACE((TRACE_INFO,_F_,"giRecoveryWebserviceWaitBeforeRetry=%d",giRecoveryWebserviceWaitBeforeRetry));
	TRACE((TRACE_INFO,_F_,"gbSyncSecondaryPasswordActive=%d"	,gbSyncSecondaryPasswordActive));
	TRACE((TRACE_INFO,_F_,"giSyncSecondaryPasswordGroup=%d"		,giSyncSecondaryPasswordGroup));
	TRACE((TRACE_INFO,_F_,"gszSyncSecondaryPasswordOU=%s"		,gszSyncSecondaryPasswordOU));
	TRACE((TRACE_INFO,_F_,"gbCheckCertificates=%d"				,gbCheckCertificates));
	TRACE((TRACE_INFO,_F_,"gszErrorServerConfigNotFound=%s"		,gszErrorServerConfigNotFound));
	TRACE((TRACE_INFO,_F_,"gszConfigNotFoundMailTo=%s"			,gszConfigNotFoundMailTo));
	TRACE((TRACE_INFO,_F_,"gpszConfigNotFoundMailSubject=%s"	,gpszConfigNotFoundMailSubject));
	TRACE((TRACE_INFO,_F_,"gpszConfigNotFoundMailBody=%s"		,gpszConfigNotFoundMailBody));
	TRACE((TRACE_INFO,_F_,"giWaitBeforeNewSSO=%d"				,giWaitBeforeNewSSO));

	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gpRecoveryKeyValue,gdwRecoveryKeyLen,"gpRecoveryKeyValue :"));
	TRACE((TRACE_INFO,_F_,"EXCLUDED WINDOWS -----------"));
	for (i=0;i<giNbExcludedWindows;i++)
	{
		TRACE((TRACE_INFO,_F_,"%d=%s",i,gtabszExcludedWindows[i]));
	}
	TRACE((TRACE_INFO,_F_,"DEFAULT INI VALUES ---------"));
	TRACE((TRACE_INFO,_F_,"gbSessionLock_DefaultValue=%d",gbSessionLock_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbInternetCheckVersion_DefaultValue=%d",gbInternetCheckVersion_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbInternetCheckBeta_DefaultValue=%d",gbInternetCheckBeta_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbInternetGetConfig_DefaultValue=%d",gbInternetGetConfig_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbInternetManualPutConfig_DefaultValue=%d",gbInternetManualPutConfig_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gszCfgPortal_DefaultValue=%s",gszCfgPortal_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbLaunchTopMost_DefaultValue=%d",gbLaunchTopMost_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbParseWindowsOnStart_DefaultValue=%d",gbParseWindowsOnStart_DefaultValue));
	TRACE((TRACE_INFO,_F_,"giDomainId_DefaultValue=%d",giDomainId_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gszDomainLabel_DefaultValue=%s",gszDomainLabel_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbDisplayChangeAppPwdDialog_DefaultValue=%d",gbDisplayChangeAppPwdDialog_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbSSOInternetExplorer_DefaultValue=%d",gbSSOInternetExplorer_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbSSOFirefox_DefaultValue=%d",gbSSOFirefox_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbSSOChrome_DefaultValue=%d",gbSSOChrome_DefaultValue));
	TRACE((TRACE_INFO,_F_,"gbShowLaunchAppWithoutCtrl_DefaultValue=%d",gbShowLaunchAppWithoutCtrl_DefaultValue));
	TRACE((TRACE_INFO,_F_,"REGKEY_HOTKEY ---------"));
	TRACE((TRACE_INFO,_F_,"gszPastePwd_Text=%s",gszPastePwd_Text));
	for (i=0;i<giNbPwdGroupColors;i++)
	{
		TRACE((TRACE_INFO,_F_,"PwdGroupColor[%d]=0x%08lx",i,gtabPwdGroupColors[i]));
	}

#endif
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

#define SEARCHTYPE_LETTERS		1
#define SEARCHTYPE_UPPERCASE	2
#define SEARCHTYPE_LOWERCASE	3
#define SEARCHTYPE_NUMBERS		4
#define SEARCHTYPE_SPECIALCHARS	5

//-----------------------------------------------------------------------------
// GetNbCharsInString()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetNbCharsInString(const char *s,int iSearchType)
{
	TRACE((TRACE_ENTER,_F_, "iSearchType=%d",iSearchType));
	int count=0;
	unsigned int i;

	for (i=0;i<strlen(s);i++)
	{
		switch (iSearchType)
		{
			case SEARCHTYPE_LETTERS:
				if ((s[i]>='a' && s[i]<='z') || (s[i]>='A' && s[i]<='Z')) count++;
				break;
			case SEARCHTYPE_UPPERCASE:
				if (s[i]>='A' && s[i]<='Z') count++;
				break;
			case SEARCHTYPE_LOWERCASE:
				if (s[i]>='a' && s[i]<='z') count++;
				break;
			case SEARCHTYPE_NUMBERS:
				if (s[i]>='0' && s[i]<='9') count++;
				break;
			case SEARCHTYPE_SPECIALCHARS: // tout le reste !
				if (!(s[i]>='a' && s[i]<='z') && !(s[i]>='A' && s[i]<='Z') && !(s[i]>='0' && s[i]<='9')) count++;
				break;
		}
	}
	TRACE((TRACE_LEAVE,_F_, "count=%d",count));
	return count;
}

//-----------------------------------------------------------------------------
// CheckCommonChars()
//-----------------------------------------------------------------------------
// Vérifie que szPwd ne contient pas plus de giPwdPolicy_IdMaxCommonChars
// caractères consécutifs de la chaine szUserName
//-----------------------------------------------------------------------------
BOOL CheckCommonChars(const char *szPwd,const char *szUserName,int giPwdPolicy_IdMaxCommonChars)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=TRUE;
	char szExtract[50+1];
	unsigned int i;

	TRACE((TRACE_DEBUG,_F_,"szPwd=%s szUserName=%s giPwdPolicy_IdMaxCommonChars=%d",szPwd,szUserName,giPwdPolicy_IdMaxCommonChars));
	if (giPwdPolicy_IdMaxCommonChars>30) goto end;

	for (i=0;i<strlen(szUserName)-giPwdPolicy_IdMaxCommonChars+1;i++)
	{
		memcpy(szExtract,szUserName+i,giPwdPolicy_IdMaxCommonChars);
		szExtract[giPwdPolicy_IdMaxCommonChars]=0;
		TRACE((TRACE_DEBUG,_F_,"Look for szExtract=%s in pwd=%s",szExtract,szPwd));

		//if (strstr(szPwd,szExtract)!=NULL) { rc=FALSE; goto end; }
		//0.85B6 : comparaison non case-sensitive !
		if (strnistr(szPwd,szExtract,-1)!=NULL) { rc=FALSE; goto end; }

	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// IsPasswordPolicyCompliant()
//-----------------------------------------------------------------------------
// int giPwdPolicy_MinLength=0;
// int giPwdPolicy_MinLetters=0;
// int giPwdPolicy_MinUpperCase=0;
// int giPwdPolicy_MinLowerCase=0;
// int giPwdPolicy_MinNumbers=0;
// int giPwdPolicy_MinSpecialsChars=0;
// int giPwdPolicy_IdMaxCommonChars=0;
//-----------------------------------------------------------------------------
BOOL IsPasswordPolicyCompliant(const char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	int iNbNoCompliancy; // superbe nommage... qui veut dire : nombre d'entorses aux règles (permet de gérer le m parmi n).

	TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));

	// vérification longueur minimale 
	if (strlen(szPwd)<(unsigned)giPwdPolicy_MinLength) goto end;
	
	// vérification composition : comptage des caractères de chaque type puis vérif
	if (GetNbCharsInString(szPwd,SEARCHTYPE_LETTERS)<giPwdPolicy_MinLetters) goto end;
	iNbNoCompliancy=0;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_UPPERCASE)<giPwdPolicy_MinUpperCase) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_LOWERCASE)<giPwdPolicy_MinLowerCase) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_NUMBERS)<giPwdPolicy_MinNumbers) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_SPECIALCHARS)<giPwdPolicy_MinSpecialsChars) iNbNoCompliancy++;
	TRACE((TRACE_INFO,_F_,"iNbNoCompliancy=%d giPwdPolicy_MinRules=%d",iNbNoCompliancy,giPwdPolicy_MinRules));
	if (giPwdPolicy_MinRules==0 && iNbNoCompliancy!=0) goto end; // si MinRules=0 : aucune tolérance.
	if (iNbNoCompliancy>(4-giPwdPolicy_MinRules)) goto end;

	// vérification composition : Ne contenant pas plus de X caractères consécutifs de l'identifiant Windows
	if (giPwdPolicy_IdMaxCommonChars!=0)
	{
		if (!CheckCommonChars(szPwd,gszUserName,giPwdPolicy_IdMaxCommonChars)) goto end;
	}
	rc=TRUE; // yes!, pas facile de trouver un mot de passe, hein ? ;-)
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}