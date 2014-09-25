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
BOOL gbStat=FALSE;							// 0.99 : statistiques - ISSUE#106
char gszWelcomeMessage[512+1];				// 1.01 : message de définition du mot de passe maitre dans la fenêtre bienvenue - ISSUE#146
int  giMaxConfigs=500;						// 1.01 : nb max de configurations - ISSUE#149
BOOL gbServerHTTPS=FALSE;						// 1.03 - ISSUE#162
int  giServerPort=INTERNET_DEFAULT_HTTP_PORT;	// 1.03 - ISSUE#162
BOOL gbUseADPasswordForAppLogin=FALSE;			// 1.03 - permet d'utiliser %ADPASSWORD% dans le champ mot de passe (n'utilise pas (encore) swSSOCM --> le mdp AD est demandé à l'utilisateur)
BOOL gbShowMenu_UploadWithIdPwd=FALSE;				// 1.03 - active le menu "Uploader avec identifiant et mot de passe"

// REGKEY_EXCLUDEDWINDOWS_OPTIONS (#110)
char gtabszExcludedWindows[MAX_EXCLUDED_WINDOWS][LEN_EXCLUDED_WINDOW_TITLE+1];
int  giNbExcludedWindows=0;

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

	//--------------------------------------------------------------
	// GLOBAL POLICY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_GLOBAL_POLICY,0,KEY_READ,&hKey);
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

		RegCloseKey(hKey);
	}

	//--------------------------------------------------------------
	// PASSWORD POLICY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_PASSWORD_POLICY,0,KEY_READ,&hKey);
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
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_ENTERPRISE_OPTIONS,0,KEY_READ,&hKey);
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
		if (rc==ERROR_SUCCESS) gbStat=(BOOL)dwValue; 
		
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

		RegCloseKey(hKey);
	}
	//--------------------------------------------------------------
	// EXCLUDED WINDOWS
	//--------------------------------------------------------------
	giNbExcludedWindows=0;
	ZeroMemory(gtabszExcludedWindows,sizeof(gtabszExcludedWindows));
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_EXCLUDEDWINDOWS,0,KEY_READ,&hKey);
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

#ifdef TRACES_ACTIVEES
	int i;
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
	TRACE((TRACE_INFO,_F_,"PASSWORD -------------------"));
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
	TRACE((TRACE_INFO,_F_,"gbStat=%d"							,gbStat));
	TRACE((TRACE_INFO,_F_,"giMaxConfigs=%d"						,giMaxConfigs));
	TRACE((TRACE_INFO,_F_,"gbUseADPasswordForAppLogin=%d"		,gbUseADPasswordForAppLogin));

	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gpRecoveryKeyValue,gdwRecoveryKeyLen,"gpRecoveryKeyValue :"));
	TRACE((TRACE_INFO,_F_,"EXCLUDED WINDOWS ---------"));
	for (i=0;i<giNbExcludedWindows;i++)
	{
		TRACE((TRACE_INFO,_F_,"%d=%s",i,gtabszExcludedWindows[i]));
	}
#endif

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