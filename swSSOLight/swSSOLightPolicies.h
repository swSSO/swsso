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
// swSSOPolicies.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// GlobalPolicy
//-----------------------------------------------------------------------------
extern BOOL gbEnableOption_Portal;
extern BOOL gbEnableOption_ViewIni;
extern BOOL gbEnableOption_OpenIni;
extern BOOL gbEnableOption_ShowOptions;
extern BOOL gbEnableOption_SessionLock;
extern BOOL gbEnableOption_CheckVersion;
extern BOOL gbEnableOption_GetConfig;
extern BOOL gbEnableOption_ManualPutConfig;
extern BOOL gbEnableOption_Proxy;
extern BOOL gbEnableOption_SavePassword;
extern BOOL gbEnableOption_ShowPassword;
extern int  gbPasswordChoiceLevel;
extern BOOL gbEnableOption_ViewAppConfig;
extern BOOL gbEnableOption_ModifyAppConfig;
extern BOOL gbShowMenu_ChangeCategIds;
extern BOOL gbShowMenu_LaunchApp;
extern BOOL gbShowMenu_AddApp;
extern BOOL gbShowMenu_EnableDisable;
extern BOOL gbShowMenu_Rename;
extern BOOL gbShowMenu_Move;
extern BOOL gbShowMenu_Delete;
extern BOOL gbShowMenu_AddCateg;
extern BOOL gbShowMenu_Duplicate;
extern BOOL gbShowMenu_AddAccount;
extern BOOL gbShowMenu_AddThisApp;
extern BOOL gbShowMenu_AppPasswordMenu;
extern BOOL gbOldPwdAutoFill;
extern BOOL gbReactivateWithoutPwd;
extern BOOL gbShowMenu_UploadWithIdPwd;				// 1.03 - active le menu "Uploader avec identifiant et mot de passe"
extern BOOL gbCheckIniIntegrity;					// ISSUE#164 - vérifie l'intégrité du fichier .ini
extern BOOL gbNoMasterPwd;
extern BOOL gbShowAutoLockOption;
extern BOOL gbEnableOption_ShowBrowsers;
extern BOOL gbShowMenu_RefreshRights;
extern int giShowPasswordGroup;
extern BOOL gbShowMenu_Quit;
extern BOOL gbShowMenu_Help;
extern int giMasterPwdMaxExpiration;
extern BOOL gbShowMenu_AskThisApp;
extern BOOL gbShowMenu_PutInSafeBox;
extern BOOL gbEnableOption_ViewServerInfos;
extern BOOL gbShowSystrayIcon;
extern BOOL gbEnableOption_ShowAdditionalIds;
extern BOOL gbEnableOption_Reset;
extern BOOL gbShowMenu_SignUp;

//-----------------------------------------------------------------------------
// EnterpriseOptions
//-----------------------------------------------------------------------------
#define LOG_LEVEL_NONE			0 // pas de log
#define LOG_LEVEL_ERROR			1 // erreurs
#define LOG_LEVEL_WARNING		2 // warning
#define LOG_LEVEL_INFO_MANAGED	3 // infos pour configurations managées uniquement
#define LOG_LEVEL_INFO_ALL		4 // infos tout
extern char gszServerAddress[];			// 0.80 : adresse du serveur pour stockage configs ("ws.swsso.fr" par défaut)
extern char gszWebServiceAddress[];		// 0.80 : adresse du serveur pour stockage configs ("/webservice5.php" par défaut)
extern char gszErrorMessageIniFile[]; 	// 0.88 : message d'erreur en cas de corruption swsso.ini
extern char gszErrorServerNotAvailable[];   // 0.90 : serveur non joignable / pb config proxy
extern char gszErrorServerConfigNotFound[]; // 0.90 : configuration demandée non trouvée
extern char gszErrorServerTitleConfigNotFound[]; // 1.14 : configuration demandée non trouvée
extern BOOL gbErrorServerConfigNotFoundDefaultMessage;
extern BOOL gbCategoryManagement;  			// 0.91 : prise en compte des catégories dans putconfig et getconfig
extern BOOL gbGetAllConfigsAtFirstStart;	// 0.91 : propose à l'utilisateur de récupérer toutes les config au 1er lancement
extern BOOL gbGetNewConfigsAtStart;			// 0.91 : récupère les nouvelles configurations à chaque démarrage
extern BOOL gbGetModifiedConfigsAtStart;	// 0.91 : récupère les configurations modifiées à chaque démarrage
extern BOOL gbDisableArchivedConfigsAtStart;// 0.91 : désactive localement les configurations archivées en central à chaque démarrage
extern BOOL gbAllowManagedConfigsDeletion;  // 0.91 : autorise l'utilisateur à supprimer des configurations managées (cad. avec un id provenant du central)
extern BOOL gbActivateNewConfigs;			// 0.91 : active les configurations récupérées sur le serveur au démarrage
extern BOOL gbDisplayConfigsNotifications;	// 0.92 : affiche les messages de notification d’ajout / modification / suppression des configurations au démarrage
extern BOOL gbWindowsEventLog;				// 0.93 : log dans le journal d'événements de Windows
extern char gszLogFileName[];				// 0.93 : chemin complet du fichier de log
extern int  giLogLevel;						// 0.93 : niveau de log
extern int  giStat;							// 0.99 : statistiques - ISSUE#106 + ISSUE#244
extern char gszWelcomeMessage[];			// 1.01 : message de définition du mot de passe maitre dans la fenêtre bienvenue - ISSUE#146
extern int  giMaxConfigs;					// 1.01 : nb max de configurations - ISSUE#149
extern BOOL gbServerHTTPS;					// 1.03 - ISSUE#162
extern int  giServerPort;					// 1.03 - ISSUE#162
extern BOOL gbUseADPasswordForAppLogin;		// 1.03 - permet d'utiliser %ADPASSWORD% dans le champ mot de passe (n'utilise pas (encore) swSSOCM --> le mdp AD est demandé à l'utilisateur)
extern BOOL gbDisplayWindowsPasswordChange; // 1.05 - affiche / masque le message affiché lors du changement de mot de passe windows (en mode chaîné)
extern BOOL gbCategoryAutoUpdate;			// 1.06 - ISSUE#206 : met à jour la catégorie sur le serveur lorsqu'une application est déplacée dans l'IHM client
extern BOOL gbRemoveDeletedConfigsAtStart;	// 1.07 - ISSUE#214
extern BOOL gbAdminDeleteConfigsOnServer;	// 1.07 - ISSUE#223
extern int  giRefreshRightsFrequency;		// 1.07 - ISSUE#220
extern BOOL gbAllowManagedConfigsModification;  // 1.07 : ISSUE#238
extern BOOL gbRecoveryWebserviceActive;		// 1.08
extern char gszRecoveryWebserviceServer[];	// 1.08
extern char gszRecoveryWebserviceURL[];		// 1.08
extern int  giRecoveryWebservicePort;		// 1.08
extern int  giRecoveryWebserviceTimeout;	// 1.08
extern BOOL gbRecoveryWebserviceHTTPS;		// 1.08
extern BOOL gbRecoveryWebserviceManualBackup; // 1.08
extern BOOL gbSyncSecondaryPasswordActive;	// 1.08
extern int  giSyncSecondaryPasswordGroup;	// 1.08
extern char gszSyncSecondaryPasswordOU[];	// 1.08
extern BOOL gbCheckCertificates;			// 1.08 - ISSUE#252
extern char gszConfigNotFoundMailTo[];		// 1.08
extern char *gpszConfigNotFoundMailSubject;	// 1.08
extern char *gpszConfigNotFoundMailBody;	// 1.08
extern int	giWaitBeforeNewSSO;				// 1.08 - ISSUE#253
extern int  giRecoveryWebserviceNbTry;		// 1.10 - ISSUE#275
extern int  giRecoveryWebserviceWaitBeforeRetry;	// 1.10 - ISSUE#275
extern char gszServerAddress2[];			// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
extern char gszWebServiceAddress2[];		// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
extern BOOL gbServerHTTPS2;					// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
extern int  giServerPort2;					// 1.14 - ISSUE#309 : adresse de failover pour le web service de configuration
extern char gszDomainRegKey[];				// 1.14 - ISSUE#317
extern char gszDomainRegValue[];			// 1.14 - ISSUE#317
extern BOOL gbGetAutoPublishedConfigsAtStart;	// 1.14 - ISSUE#310
extern char gszAskThisAppMessage[];			// 1.14 - ISSUE#319
extern int	giWebServiceTimeout;			// 1.14 - ISSUE#329
extern int	giWebServiceTimeout2;			// 1.14 - ISSUE#329
extern BOOL gbUseSquareForManagedConfigs;	// 1.16 - ISSUE#338
extern char gpszIniPathName[];				// 1.18 - ISSUE#364
extern int	gbExitIfNetworkUnavailable;		// 1.18 - ISSUE#365
extern int  giDetectionFrequency;			// 1.19 - ISSUE#379

// DefaultIniValues
extern BOOL gbSessionLock_DefaultValue;					// 1.04
extern BOOL gbInternetCheckVersion_DefaultValue;		// 1.04
extern BOOL gbInternetCheckBeta_DefaultValue;			// 1.04
extern BOOL gbInternetGetConfig_DefaultValue;			// 1.04
extern BOOL gbInternetManualPutConfig_DefaultValue;		// 1.04
extern char gszCfgPortal_DefaultValue[];				// 1.04
extern BOOL gbLaunchTopMost_DefaultValue;				// 1.04
extern BOOL gbParseWindowsOnStart_DefaultValue;			// 1.04
extern int  giDomainId_DefaultValue;					// 1.04
extern char gszDomainLabel_DefaultValue[];				// 1.04
extern BOOL gbDisplayChangeAppPwdDialog_DefaultValue;	// 1.04
extern BOOL gbSSOInternetExplorer_DefaultValue;			// 1.04
extern BOOL gbSSOFirefox_DefaultValue;					// 1.04
extern BOOL gbSSOChrome_DefaultValue;					// 1.04
extern BOOL gbSSOEdge_DefaultValue;						// 1.20
extern BOOL gbShowLaunchAppWithoutCtrl_DefaultValue;	// 1.08
/*
extern BOOL gbSessionLock_ChangeValue;					// 1.14
extern BOOL gbInternetCheckVersion_ChangeValue;			// 1.14
extern BOOL gbInternetCheckBeta_ChangeValue;			// 1.14
extern BOOL gbInternetGetConfig_ChangeValue;			// 1.14
extern BOOL gbInternetManualPutConfig_ChangeValue;		// 1.14
extern char gszCfgPortal_ChangeValue[];					// 1.14
extern BOOL gbLaunchTopMost_ChangeValue;				// 1.14
extern BOOL gbParseWindowsOnStart_ChangeValue;			// 1.14
extern int  giDomainId_ChangeValue;						// 1.14
extern char gszDomainLabel_ChangeValue[];				// 1.14
extern BOOL gbDisplayChangeAppPwdDialog_ChangeValue;	// 1.14
extern BOOL gbSSOInternetExplorer_ChangeValue;			// 1.14
extern BOOL gbSSOFirefox_ChangeValue;					// 1.14
extern BOOL gbSSOChrome_ChangeValue;					// 1.14
extern BOOL gbSSOEdge_ChangeValue;						// 1.20
extern BOOL gbShowLaunchAppWithoutCtrl_ChangeValue;		// 1.14
extern int  giRecoveryKeyId_ChangeValue;				// 1.14
*/
//-----------------------------------------------------------------------------
#define REGKEY_HOTKEY "SOFTWARE\\swSSO\\HotKey"
#define REGKEY_HOTKEY_ADMIN "SOFTWARE\\swSSOAdmin\\HotKey"
//-----------------------------------------------------------------------------
#define REGVALUE_PASTEPWD_TEXT	"PastePwd_Text"
extern char gszPastePwd_Text[];

//-----------------------------------------------------------------------------
// PwdGroupColors"
//-----------------------------------------------------------------------------
#define MAX_COLORS			25   
extern COLORREF gtabPwdGroupColors[MAX_COLORS];
extern int		giNbPwdGroupColors;
