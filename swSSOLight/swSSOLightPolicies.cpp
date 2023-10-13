//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
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
// ISSUE#306
BOOL gbShowMenu_Help=FALSE;
// ISSUE#309
int giMasterPwdMaxExpiration=60;	// 1.14 : valeur max pour l'expiration du master password
// ISSUE#319
BOOL gbShowMenu_AskThisApp=FALSE;
// ISSUE#320
BOOL gbShowMenu_PutInSafeBox=TRUE;
// ISSUE#326
BOOL gbEnableOption_ViewServerInfos=TRUE;
// ISSUE#337
BOOL gbShowSystrayIcon=TRUE;
// ISSUE#363
BOOL gbEnableOption_ShowAdditionalIds=TRUE;
// ISSUE#374
BOOL gbEnableOption_Reset=FALSE;
BOOL gbShowMenu_SignUp=TRUE;

// REGKEY_PASSWORD_POLICY
int giPwdPolicy_MinLength=8;		// 1.12B4 - TI-TIE1 : politique de mot de passe imposée par défaut
int giPwdPolicy_MinLetters=1;		// 1.12B4 - TI-TIE1 : politique de mot de passe imposée par défaut
int giPwdPolicy_MinUpperCase=1;		// 1.12B4 - TI-TIE1 : politique de mot de passe imposée par défaut
int giPwdPolicy_MinLowerCase=1;		// 1.12B4 - TI-TIE1 : politique de mot de passe imposée par défaut
int giPwdPolicy_MinNumbers=1;		// 1.12B4 - TI-TIE1 : politique de mot de passe imposée par défaut
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
char gszErrorServerTitleConfigNotFound[256+1]; // 1.14 : configuration demandée non trouvée
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
int  giMaxConfigs=1000;						// 1.01 : nb max de configurations - ISSUE#149
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
char gszServerAddress2[128+1];					// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
char gszWebServiceAddress2[256+1];				// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
BOOL gbServerHTTPS2=FALSE;						// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
int  giServerPort2=INTERNET_DEFAULT_HTTP_PORT;	// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
char gszDomainRegKey[256+1];					// 1.14 - ISSUE#317
char gszDomainRegValue[128+1];					// 1.14 - ISSUE#317
BOOL gbGetAutoPublishedConfigsAtStart;			// 1.14 - ISSUE#310
char gszAskThisAppMessage[1024+1];				// 1.14 - ISSUE#319
int	giWebServiceTimeout=8;						// 1.14 - ISSUE#329
int	giWebServiceTimeout2=8;						// 1.14 - ISSUE#329
BOOL gbUseSquareForManagedConfigs=TRUE;			// 1.16 - ISSUE#338
char gpszIniPathName[_SW_MAX_PATH+1];			// 1.18 - ISSUE#364
int gbExitIfNetworkUnavailable=FALSE;			// 1.18 - ISSUE#365
int giDetectionFrequency=500;					// 1.19 - ISSUE#379


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
BOOL gbDisplayChangeAppPwdDialog_DefaultValue=TRUE;	// 1.04
BOOL gbSSOInternetExplorer_DefaultValue=TRUE;		// 1.04
BOOL gbSSOFirefox_DefaultValue=TRUE;				// 1.04
BOOL gbSSOChrome_DefaultValue=TRUE;					// 1.04
BOOL gbSSOEdge_DefaultValue=TRUE;					// 1.20
BOOL gbShowLaunchAppWithoutCtrl_DefaultValue=FALSE;	// 1.08

/*
// REGKEY_CHANGEINIVALUES
BOOL gbSessionLock_ChangeValue=-1;					// 1.14
BOOL gbInternetCheckVersion_ChangeValue=-1;			// 1.14
BOOL gbInternetCheckBeta_ChangeValue=-1;			// 1.14
BOOL gbInternetGetConfig_ChangeValue=-1;			// 1.14
BOOL gbInternetManualPutConfig_ChangeValue=-1;		// 1.14
char gszCfgPortal_ChangeValue[_MAX_PATH+1]="*";		// 1.14
BOOL gbLaunchTopMost_ChangeValue=-1;				// 1.14
BOOL gbParseWindowsOnStart_ChangeValue=-1;			// 1.14
int  giDomainId_ChangeValue=-1;						// 1.14
BOOL gbDisplayChangeAppPwdDialog_ChangeValue=-1;	// 1.14
BOOL gbSSOInternetExplorer_ChangeValue=-1;			// 1.14
BOOL gbSSOFirefox_ChangeValue=-1;					// 1.14
BOOL gbSSOChrome_ChangeValue=-1;					// 1.14
BOOL gbSSOEdge_ChangeValue=-1;						// 1.20
BOOL gbShowLaunchAppWithoutCtrl_ChangeValue=-1;		// 1.14
int  giRecoveryKeyId_ChangeValue=-1;				// 1.14
*/

// REGKEY_REGKEY_PWDGROUP_COLORS
COLORREF gtabPwdGroupColors[MAX_COLORS];
int giNbPwdGroupColors;

char gszPastePwd_Text[100];


