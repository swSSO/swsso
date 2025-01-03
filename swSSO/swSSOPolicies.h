//-----------------------------------------------------------------------------
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
// swSSOPolicies.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#define REGKEY_GLOBAL_POLICY "SOFTWARE\\swSSO\\GlobalPolicy"
#define REGKEY_DOMAIN_POLICY "SOFTWARE\\swSSO\\DomainPolicy\\%s"
#define REGKEY_GLOBAL_POLICY_ADMIN "SOFTWARE\\swSSOAdmin\\GlobalPolicy"
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
#define REGVALUE_SHOWMENU_CHANGEAPPPASSWORD			"ShowChangeAppPwdMenu"
#define REGVALUE_OLD_PWD_AUTO_FILL					"OldPwdAutoFill"
// ISSUE#140
#define REGVALUE_REACTIVATE_WITHOUT_PWD				"ReactivateWithoutPwd"
#define REGVALUE_SHOWMENU_UPLOAD_WITH_ID_PWD		"ShowUploadWithIdPwdMenu"
// ISSUE#164
#define REGVALUE_CHECK_INI_INTEGRITY				"CheckIniIntegrity"
#define REGVALUE_NO_MASTER_PASSWORD					"NoMasterPwd"
// ISSUE#180
#define REGVALUE_SHOW_AUTO_LOCK_OPTION				"ShowAutoLockOption"
// ISSUE#183
#define REGVALUE_ENABLEOPTION_SHOWBROWSERS			"ShowBrowsers"
// ISSUE#204
#define REGVALUE_SHOWMENU_REFRESH_RIGHTS			"ShowRefreshRightsMenu"
// ISSUE#256
#define REGVALUE_SHOW_PASSWORD_GROUP				"ShowPasswordGroup"
// ISSUE#257
#define REGVALUE_SHOWMENU_QUIT						"ShowQuitMenu"
// ISSUE#306
#define REGVALUE_SHOWMENU_HELP						"ShowHelpMenu"
// ISSUE#309
#define REGVALUE_MASTER_PASSWORD_EXPIRATION			"MasterPwdExpiration"
// ISSUE#319
#define REGVALUE_SHOWMENU_ASKTHISAPP				"ShowAskThisAppMenu"
// ISSUE#320
#define REGVALUE_SHOWMENU_PUTINSAFEBOX				"ShowPutInSafeBoxMenu"
// ISSUE#326
#define REGVALUE_ENABLEOPTION_VIEWSERVERINFOS		"ViewServerInfos"
// ISSUE#337
#define REGVALUE_SHOW_SYSTRAY_ICON					"ShowSystrayIcon"
// ISSUE#363
#define REGVALUE_SHOW_ADDITIONAL_IDS				"ShowAdditionalIds"
// ISSUE#374
#define REGVALUE_ENABLEOPTION_RESET					"ShowResetButton"
#define REGVALUE_SHOWMENU_SIGNUP					"ShowSignUpMenu"

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
extern BOOL gbOldPwdAutoFill;
// ISSUE#140
extern BOOL gbReactivateWithoutPwd;
extern BOOL gbShowMenu_UploadWithIdPwd;				// 1.03 - active le menu "Uploader avec identifiant et mot de passe"
// ISSUE#164
extern BOOL gbCheckIniIntegrity;					// ISSUE#164 - v�rifie l'int�grit� du fichier .ini
extern BOOL gbNoMasterPwd;
// ISSUE#180
extern BOOL gbShowAutoLockOption;
// ISSUE#183
extern BOOL gbEnableOption_ShowBrowsers;
// ISSUE#204
extern BOOL gbShowMenu_RefreshRights;
// ISSUE#256
extern int giShowPasswordGroup;
// ISSUE#257
extern BOOL gbShowMenu_Quit;
// ISSUE#306
extern BOOL gbShowMenu_Help;
// ISSUE#309
extern int giMasterPwdMaxExpiration;
// ISSUE#319
extern BOOL gbShowMenu_AskThisApp;
// ISSUE#320
extern BOOL gbShowMenu_PutInSafeBox;
// ISSUE#326
extern BOOL gbEnableOption_ViewServerInfos;
// ISSUE#337
extern BOOL gbShowSystrayIcon;
// ISSUE#363
extern BOOL gbEnableOption_ShowAdditionalIds;
// ISSUE#374
extern BOOL gbEnableOption_Reset;
extern BOOL gbShowMenu_SignUp;

//-----------------------------------------------------------------------------
#define REGKEY_PASSWORD_POLICY "SOFTWARE\\swSSO\\PasswordPolicy"
#define REGKEY_PASSWORD_POLICY_ADMIN "SOFTWARE\\swSSOAdmin\\PasswordPolicy"
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
#define REGKEY_ENTERPRISE_OPTIONS_ADMIN "SOFTWARE\\swSSOAdmin\\EnterpriseOptions"
//-----------------------------------------------------------------------------
#define REGVALUE_SERVER_ADDRESS						"ServerAddress"
#define REGVALUE_WEBSERVICE_ADDRESS					"WebServiceAddress"
#define REGVALUE_ERROR_MESSAGE_INI_FILE				"ErrorMessageIniFile"
#define REGVALUE_ERROR_SERVER_NOT_AVAILABLE			"ErrorMessageServerNotAvailable"
#define REGVALUE_ERROR_TITLE_CONFIG_NOT_FOUND		"ErrorTitleConfigNotFound"
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
#define REGVALUE_STAT								"Stat"
#define REGVALUE_WELCOME_MESSAGE					"WelcomeMessage"
#define REGVALUE_MAX_CONFIGS						"MaxConfigs"
#define REGVALUE_SERVER_PORT						"ServerPort"
#define REGVALUE_SERVER_HTTPS						"ServerHTTPS"
#define REGVALUE_USE_AD_PASSWORD					"UseADPassword"
#define REGVALUE_DISPLAY_WINDOWS_PASSWORD_CHANGE	"DisplayWindowsPasswordChange"
#define REGVALUE_CATEGORY_AUTO_UPDATE				"CategoryAutoUpdate"
#define REGVALUE_REMOVE_DELETED_CONFIGS_AT_START	"RemoveDeletedConfigsAtStart"
#define REGVALUE_ADMIN_DELETE_CONFIGS_ON_SERVER		"AdminDeleteConfigsOnServer"
#define REGVALUE_REFRESH_RIGHTS_FREQUENCY			"RefreshRightsFrequency"
#define REGVALUE_ALLOW_MANAGED_CONFIGS_MODIFICATION "AllowManagedConfigsModification"
#define REGVALUE_RECOVERY_WEBSERVICE_ACTIVE			"RecoveryWebserviceActive"
#define REGVALUE_RECOVERY_WEBSERVICE_SERVER			"RecoveryWebserviceServer"
#define REGVALUE_RECOVERY_WEBSERVICE_ADDRESS		"RecoveryWebserviceURL"
#define REGVALUE_RECOVERY_WEBSERVICE_PORT			"RecoveryWebservicePort"
#define REGVALUE_RECOVERY_WEBSERVICE_TIMEOUT		"RecoveryWebserviceTimeout"
#define REGVALUE_RECOVERY_WEBSERVICE_HTTPS			"RecoveryWebserviceHTTPS"
#define REGVALUE_RECOVERY_WEBSERVICE_MANUAL_BACKUP	"RecoveryWebserviceManualBackup"
#define REGVALUE_RECOVERY_WEBSERVICE_NB_TRY			"RecoveryWebserviceNbTry"
#define REGVALUE_RECOVERY_WEBSERVICE_WAIT_BEFORE_RETRY	"RecoveryWebserviceWaitBeforeRetry"
#define REGVALUE_SYNC_SECONDARY_PASSWORD_ACTIVE		"SyncSecondaryPasswordActive"
#define REGVALUE_SYNC_SECONDARY_PASSWORD_GROUP		"SyncSecondaryPasswordGroup"
#define REGVALUE_SYNC_SECONDARY_PASSWORD_OU			"SyncSecondaryPasswordOU"
#define REGVALUE_CHECK_CERTIFICATES					"CheckCertificates"
#define REGVALUE_CONFIG_NOT_FOUND_MAILTO			"ConfigNotFoundMailTo"
#define REGVALUE_CONFIG_NOT_FOUND_MAILSUBJECT		"ConfigNotFoundMailSubject"
#define REGVALUE_CONFIG_NOT_FOUND_MAILBODY			"ConfigNotFoundMailBody"
#define REGVALUE_WAIT_BEFORE_NEW_SSO				"WaitBeforeNewSSO"
#define REGVALUE_SERVER_ADDRESS2					"ServerAddress2"
#define REGVALUE_WEBSERVICE_ADDRESS2				"WebServiceAddress2"
#define REGVALUE_SERVER_PORT2						"ServerPort2"
#define REGVALUE_SERVER_HTTPS2						"ServerHTTPS2"
#define REGVALUE_DOMAIN_REG_KEY						"DomainRegKey"
#define REGVALUE_DOMAIN_REG_VALUE					"DomainRegValue"
#define REGVALUE_GET_AUTO_PUBLISHED_CONFIGS_AT_START	"GetAutoPublishedConfigsAtStart"
#define REGVALUE_ASK_THIS_APP_MESSAGE				"AskThisAppMessage"
#define REGVALUE_WEBSERVICE_TIMEOUT					"WebServiceTimeout"
#define REGVALUE_WEBSERVICE_TIMEOUT2				"WebServiceTimeout2"
#define REGVALUE_USE_SQUARE_FOR_MANAGED_CONFIGS		"UseSquareForManagedConfigs"
#define REGVALUE_INI_PATHNAME						"IniPathName"
#define REGVALUE_EXIT_IF_NETWORK_UNAVAILABLE		"ExitIfNetworkUnavailable"
#define REGVALUE_DETECTION_FREQUENCY				"DetectionFrequency"

#define LOG_LEVEL_NONE			0 // pas de log
#define LOG_LEVEL_ERROR			1 // erreurs
#define LOG_LEVEL_WARNING		2 // warning
#define LOG_LEVEL_INFO_MANAGED	3 // infos pour configurations manag�es uniquement
#define LOG_LEVEL_INFO_ALL		4 // infos tout

extern char gszServerAddress[];			// 0.80 : adresse du serveur pour stockage configs ("swsso.free.nf" par d�faut)
extern char gszWebServiceAddress[];		// 0.80 : adresse du serveur pour stockage configs ("/webservice6.php" par d�faut)
extern char gszErrorMessageIniFile[]; 	// 0.88 : message d'erreur en cas de corruption swsso.ini
extern char gszErrorServerNotAvailable[];   // 0.90 : serveur non joignable / pb config proxy
extern char gszErrorServerConfigNotFound[]; // 0.90 : configuration demand�e non trouv�e
extern char gszErrorServerTitleConfigNotFound[]; // 1.14 : configuration demand�e non trouv�e
extern BOOL gbErrorServerConfigNotFoundDefaultMessage;
extern BOOL gbCategoryManagement;  			// 0.91 : prise en compte des cat�gories dans putconfig et getconfig
extern BOOL gbGetAllConfigsAtFirstStart;	// 0.91 : propose � l'utilisateur de r�cup�rer toutes les config au 1er lancement
extern BOOL gbGetNewConfigsAtStart;			// 0.91 : r�cup�re les nouvelles configurations � chaque d�marrage
extern BOOL gbGetModifiedConfigsAtStart;	// 0.91 : r�cup�re les configurations modifi�es � chaque d�marrage
extern BOOL gbDisableArchivedConfigsAtStart;// 0.91 : d�sactive localement les configurations archiv�es en central � chaque d�marrage
extern BOOL gbAllowManagedConfigsDeletion;  // 0.91 : autorise l'utilisateur � supprimer des configurations manag�es (cad. avec un id provenant du central)
extern BOOL gbActivateNewConfigs;			// 0.91 : active les configurations r�cup�r�es sur le serveur au d�marrage
extern BOOL gbDisplayConfigsNotifications;	// 0.92 : affiche les messages de notification d�ajout / modification / suppression des configurations au d�marrage
extern BOOL gbWindowsEventLog;				// 0.93 : log dans le journal d'�v�nements de Windows
extern char gszLogFileName[];				// 0.93 : chemin complet du fichier de log
extern int  giLogLevel;						// 0.93 : niveau de log
extern int  giStat;							// 0.99 : statistiques - ISSUE#106 + ISSUE#244
extern char gszWelcomeMessage[];			// 1.01 : message de d�finition du mot de passe maitre dans la fen�tre bienvenue - ISSUE#146
extern int  giMaxConfigs;					// 1.01 : nb max de configurations - ISSUE#149
extern BOOL gbServerHTTPS;					// 1.03 - ISSUE#162
extern int  giServerPort;					// 1.03 - ISSUE#162
extern BOOL gbUseADPasswordForAppLogin;		// 1.03 - permet d'utiliser %ADPASSWORD% dans le champ mot de passe (n'utilise pas (encore) swSSOCM --> le mdp AD est demand� � l'utilisateur)
extern BOOL gbDisplayWindowsPasswordChange; // 1.05 - affiche / masque le message affich� lors du changement de mot de passe windows (en mode cha�n�)
extern BOOL gbCategoryAutoUpdate;			// 1.06 - ISSUE#206 : met � jour la cat�gorie sur le serveur lorsqu'une application est d�plac�e dans l'IHM client
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

//-----------------------------------------------------------------------------
#define REGKEY_EXCLUDEDWINDOWS "SOFTWARE\\swSSO\\ExcludedWindows"
#define REGKEY_EXCLUDEDWINDOWS_ADMIN "SOFTWARE\\swSSOAdmin\\ExcludedWindows"
//-----------------------------------------------------------------------------
#define LEN_EXCLUDED_WINDOW_TITLE	200
#define MAX_EXCLUDED_WINDOWS		25   // ISSUE#154

extern char gtabszExcludedWindows[][LEN_EXCLUDED_WINDOW_TITLE+1];
extern int  giNbExcludedWindows;

//-----------------------------------------------------------------------------
#define REGKEY_DEFAULTINIVALUES "SOFTWARE\\swSSO\\DefaultIniValues"
#define REGKEY_DEFAULTINIVALUES_ADMIN "SOFTWARE\\swSSOAdmin\\DefaultIniValues"
//-----------------------------------------------------------------------------
#define REGKEY_CHANGEINIVALUES "SOFTWARE\\swSSO\\ChangeIniValues"
#define REGKEY_CHANGEINIVALUES_ADMIN "SOFTWARE\\swSSOAdmin\\ChangeIniValues"
//-----------------------------------------------------------------------------
#define REGVALUE_DEFAULT_SESSION_LOCK			"sessionLock"
#define REGVALUE_DEFAULT_CHECK_VERSION			"internetCheckVersion"
#define REGVALUE_DEFAULT_CHECK_BETA				"internetCheckBeta"
#define REGVALUE_DEFAULT_GET_CONFIG				"internetGetConfig"
#define REGVALUE_DEFAULT_PUT_CONFIG				"internetManualPutConfig"
#define REGVALUE_DEFAULT_PORTAL					"Portal"
#define REGVALUE_DEFAULT_LAUNCH_TOPMOST			"LaunchTopMost"
#define REGVALUE_DEFAULT_PARSE_ON_START			"parseWindowsOnStart"
#define REGVALUE_DEFAULT_DOMAIN_ID				"domainId"
#define REGVALUE_DEFAULT_DOMAIN_LABEL			"domainLabel"
#define REGVALUE_DEFAULT_DISPLAY_CHANGE_APP		"displayChangeAppPwdDialog"
#define REGVALUE_DEFAULT_INTERNET_EXPLORER		"InternetExplorer"
#define REGVALUE_DEFAULT_FIREFOX				"Firefox"
#define REGVALUE_DEFAULT_CHROME					"Chrome"
#define REGVALUE_DEFAULT_EDGE					"Edge"
#define REGVALUE_DEFAULT_SHOW_LAUNCHAPP_WITHOUT_CTRL	"ShowLaunchAppWithoutCtrl"
// uniquement dans les ChangeIniValues
#define REGVALUE_CHANGE_RECOVERY_KEY_ID			"RecoveryKeyId"	// ISSUE#323

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

//-----------------------------------------------------------------------------
#define REGKEY_HOTKEY "SOFTWARE\\swSSO\\HotKey"
#define REGKEY_HOTKEY_ADMIN "SOFTWARE\\swSSOAdmin\\HotKey"
//-----------------------------------------------------------------------------
#define REGVALUE_PASTEPWD_TEXT	"PastePwd_Text"
extern char gszPastePwd_Text[];

// FONCTIONS PUBLIQUES
void LoadIniPathNamePolicy(void); // doit �tre lu tr�s t�t, je cr�e cette fonction pour �viter d'�tudier le risque d'un d�placement de l'appel � LoadPolicies
void LoadPolicies(void);
void LoadGlobalOrDomainPolicies(char *pcszDomain);
void LoadNewPasswordPolicies(void);
BOOL IsPasswordPolicyCompliant(const char *szPwd);
#define SEARCHTYPE_LETTERS		1
#define SEARCHTYPE_UPPERCASE	2
#define SEARCHTYPE_LOWERCASE	3
#define SEARCHTYPE_NUMBERS		4
#define SEARCHTYPE_SPECIALCHARS	5
int GetNbCharsInString(const char *s,int iSearchType);
BOOL CheckCommonChars(const char *szPwd,const char *szUserName,int iPwdPolicy_IdMaxCommonChars);

//-----------------------------------------------------------------------------
#define REGKEY_PWDGROUP_COLORS "SOFTWARE\\swSSO\\PwdGroupColors"
#define REGKEY_PWDGROUP_COLORS_ADMIN "SOFTWARE\\swSSOAdmin\\PwdGroupColors"
//-----------------------------------------------------------------------------
#define LEN_COLOR_STRING	11
#define MAX_COLORS			25   
extern COLORREF gtabPwdGroupColors[MAX_COLORS];
extern int		giNbPwdGroupColors;

//-----------------------------------------------------------------------------
#define REGKEY_NEW_PASSWORD_POLICY "SOFTWARE\\swSSO\\NewPasswordPolicies\\%02d"
//-----------------------------------------------------------------------------
#define REGVALUE_NEW_PASSWORD_POLICY_MINLENGTH					"MinLength"
#define REGVALUE_NEW_PASSWORD_POLICY_MAXLENGTH					"MaxLength"
#define REGVALUE_NEW_PASSWORD_POLICY_MINUPPERCASE				"MinUpperCase"
#define REGVALUE_NEW_PASSWORD_POLICY_MINLOWERCASE				"MinLowerCase"
#define REGVALUE_NEW_PASSWORD_POLICY_MINNUMBERS					"MinNumbers"
#define REGVALUE_NEW_PASSWORD_POLICY_MINSPECIALCHARS			"MinSpecialChars"
#define REGVALUE_NEW_PASSWORD_POLICY_MAXCOMMONCHARS				"MaxCommonChars"
#define REGVALUE_NEW_PASSWORD_POLICY_MAXCONSECUTIVECOMMONCHARS	"MaxConsecutiveCommonChars"
#define REGVALUE_NEW_PASSWORD_POLICY_IDMAXCOMMONCHARS			"IdMaxCommonChars"

typedef struct
{
	BOOL isDefined;
	int MinLength;
	int MaxLength;
	int MinUpperCase;
	int MinLowerCase;
	int MinNumbers;
	int MinSpecialChars;
	int MaxCommonChars;
	int MaxConsecutiveCommonChars;
	int IdMaxCommonChars;

} T_NEW_PASSWORD_POLICY;

extern T_NEW_PASSWORD_POLICY gptNewPasswordPolicies[];
