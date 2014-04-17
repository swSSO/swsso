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
// swSSOPolicies.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#define REGKEY_GLOBAL_POLICY "SOFTWARE\\swSSO\\GlobalPolicy"
//-----------------------------------------------------------------------------
#define REGVALUE_ENABLEOPTION_PORTAL				"SavePortal"
#define REGVALUE_ENABLEOPTION_VIEWINI				"ViewConfigFilePath"
#define REGVALUE_ENABLEOPTION_OPENINI				"OpenConfigFile"
#define REGVALUE_ENABLEOPTION_SHOWOPTIONS			"ShowOptions"
#define REGVALUE_ENABLEOPTION_SESSIONLOCK			"SessionLockOption"
#define REGVALUE_ENABLEOPTION_CHECKVERSION			"CheckVersionOption"
#define REGVALUE_ENABLEOPTION_GETCONFIG				"GetConfigOption"
//#define REGVALUE_ENABLEOPTION_PUTCONFIG			"PutConfigOption"
#define REGVALUE_ENABLEOPTION_MANUALPUTCONFIG		"ManualPutConfigOption"
#define REGVALUE_ENABLEOPTION_PROXY					"ProxyOption"
#define REGVALUE_ENABLEOPTION_SAVEPASSWORD			"SavePasswordOption"
#define REGVALUE_ENABLEOPTION_SHOWPASSWORD			"ShowPasswordOption"
#define REGVALUE_PASSWORDCHOICELEVEL				"PasswordChoiceLevel"
#define REGVALUE_ENABLEOPTION_VIEWAPPCONFIG			"ViewApplicationConfig"
#define REGVALUE_ENABLEOPTION_MODIFYAPPCONFIG		"ModifyApplicationConfig"
#define REGVALUE_SHOWMENU_CHANGECATEGIDS			"ShowChangeCategIdsMenu"
#define REGVALUE_SHOWMENU_LAUNCHAPP					"ShowLaunchAppMenu"
#define REGVALUE_SHOWMENU_ADDAPP					"ShowAddAppMenu"
// ISSUE#84 : ajout de nouvelles options de restriction de l'IHM (items de menu categorie et appli)
#define REGVALUE_SHOWMENU_ENABLEDISABLE				"ShowEnableDisableMenu"
#define REGVALUE_SHOWMENU_RENAME					"ShowRenameMenu"
#define REGVALUE_SHOWMENU_MOVE						"ShowMoveMenu"
#define REGVALUE_SHOWMENU_DELETE					"ShowDeleteMenu"
#define REGVALUE_SHOWMENU_ADDCATEG					"ShowAddCategMenu"
#define REGVALUE_SHOWMENU_DUPLICATE					"ShowDuplicateMenu"
#define REGVALUE_SHOWMENU_ADDACCOUNT				"ShowAddAccountMenu"
// ISSUE#99 : ajout de gbShowMenu_AddThisApp pour dissocier gbShowMenu_AddApp
#define REGVALUE_SHOWMENU_ADDTHISAPP				"ShowAddThisAppMenu"
// ISSUE#107
#define REGVALUE_SHOWMENU_CHANGEAPPPASSWORD			"ShowChangeAppPasswordMenu"

extern BOOL gbEnableOption_Portal;
extern BOOL gbEnableOption_ViewIni;
extern BOOL gbEnableOption_OpenIni;
extern BOOL gbEnableOption_ShowOptions;
extern BOOL gbEnableOption_SessionLock;
extern BOOL gbEnableOption_CheckVersion;
extern BOOL gbEnableOption_GetConfig;
//extern BOOL gbEnableOption_PutConfig;
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
// ISSUE#84 : ajout de nouvelles options de restriction de l'IHM (items de menu categorie et appli)
extern BOOL gbShowMenu_EnableDisable;
extern BOOL gbShowMenu_Rename;
extern BOOL gbShowMenu_Move;
extern BOOL gbShowMenu_Delete;
extern BOOL gbShowMenu_AddCateg;
extern BOOL gbShowMenu_Duplicate;
extern BOOL gbShowMenu_AddAccount;
// ISSUE#99 : ajout de gbShowMenu_AddThisApp pour dissocier gbShowMenu_AddApp
extern BOOL gbShowMenu_AddThisApp;
// ISSUE#107
extern BOOL gbShowMenu_AppPasswordMenu;

//-----------------------------------------------------------------------------
#define REGKEY_PASSWORD_POLICY "SOFTWARE\\swSSO\\PasswordPolicy"
//-----------------------------------------------------------------------------
#define REGVALUE_PASSWORD_POLICY_MINLENGTH			"MinLength"
#define REGVALUE_PASSWORD_POLICY_MINLETTERS			"MinLetters"
#define REGVALUE_PASSWORD_POLICY_MINUPPERCASE		"MinUpperCase"
#define REGVALUE_PASSWORD_POLICY_MINLOWERCASE		"MinLowerCase"
#define REGVALUE_PASSWORD_POLICY_MINNUMBERS			"MinNumbers"
#define REGVALUE_PASSWORD_POLICY_MINSPECIALCHARS	"MinSpecialChars"
#define REGVALUE_PASSWORD_POLICY_MAXAGE				"MaxAge"
#define REGVALUE_PASSWORD_POLICY_MINAGE				"MinAge"
#define REGVALUE_PASSWORD_POLICY_IDMAXCOMMONCHARS	"IdMaxCommonChars"
#define REGVALUE_PASSWORD_POLICY_MESSAGE			"Message"
#define REGVALUE_PASSWORD_POLICY_MINRULES			"MinRules"

extern int giPwdPolicy_MinLength;
extern int giPwdPolicy_MinLetters;
extern int giPwdPolicy_MinUpperCase;
extern int giPwdPolicy_MinLowerCase;
extern int giPwdPolicy_MinNumbers;
extern int giPwdPolicy_MinSpecialsChars;
extern int giPwdPolicy_MaxAge;
extern int giPwdPolicy_MinAge;
extern int giPwdPolicy_IdMaxCommonChars;
extern char gszPwdPolicy_Message[];
extern int giPwdPolicy_MinRules;

//-----------------------------------------------------------------------------
#define REGKEY_ENTERPRISE_OPTIONS "SOFTWARE\\swSSO\\EnterpriseOptions"
//-----------------------------------------------------------------------------
#define REGVALUE_SERVER_ADDRESS						"ServerAddress"
#define REGVALUE_WEBSERVICE_ADDRESS					"WebServiceAddress"
#define REGVALUE_ERROR_MESSAGE_INI_FILE				"ErrorMessageIniFile"
#define REGVALUE_ERROR_SERVER_NOT_AVAILABLE			"ErrorMessageServerNotAvailable"
#define REGVALUE_ERROR_CONFIG_NOT_FOUND				"ErrorMessageConfigNotFound"
#define REGVALUE_RECOVERY_KEYID						"RecoveryKeyId"
#define REGVALUE_RECOVERY_KEYVALUE					"RecoveryKeyValue"
#define REGVALUE_CATEGORY_MANAGEMENT				"CategoryManagement"
#define REGVALUE_GET_ALL_CONFIGS_AT_FIRST_START		"GetAllConfigsAtFirstStart"
#define REGVALUE_GET_NEW_CONFIGS_AT_START			"GetNewConfigsAtStart"
#define REGVALUE_GET_MODIFIED_CONFIGS_AT_START		"GetModifiedConfigsAtStart"
#define REGVALUE_DISABLE_ARCHIVED_CONFIGS_AT_START	"DisableArchivedConfigsAtStart"
#define REGVALUE_ALLOW_MANAGED_CONFIGS_DELETION		"AllowManagedConfigsDeletion"
#define REGVALUE_ACTIVATE_NEW_CONFIGS				"ActivateNewConfigs"
#define REGVALUE_DISPLAY_CONFIGS_NOTIFICATIONS		"DisplayConfigsNotifications"
#define REGVALUE_WINDOWS_EVENT_LOG					"WindowsEventLog"
#define REGVALUE_LOG_FILE_NAME						"LogFileName"
#define REGVALUE_LOG_LEVEL							"LogLevel"						

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

//-----------------------------------------------------------------------------
#define REGKEY_EXCLUDEDWINDOWS "SOFTWARE\\swSSO\\ExcludedWindows"
//-----------------------------------------------------------------------------
#define LEN_EXCLUDED_WINDOW_TITLE	200
#define MAX_EXCLUDED_WINDOWS		10

extern char gtabszExcludedWindows[][LEN_EXCLUDED_WINDOW_TITLE+1];
extern int  giNbExcludedWindows;

// FONCTIONS PUBLIQUES
void LoadPolicies(void);
BOOL IsPasswordPolicyCompliant(const char *szPwd);
