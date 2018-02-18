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
// swSSOConfig.h
//-----------------------------------------------------------------------------

#define _SW_MAX_PATH 1024 // 1.18

int GetConfigHeader();
int ShowConfig(void);
int SaveConfigHeader();
int StoreMasterPwd(const char *szPwd);
int DPAPIStoreMasterPwd(const char *szPwd); //0.76
int DPAPIGetMasterPwd(char *pszPwd); //0.76
int CheckMasterPwd(const char *szPwd);
int swTranscrypt(void);			// 0.96
int ChangeWindowsPwd(void);		// 0.96
int CheckWindowsPwd(BOOL *pbMigrationWindowsSSO);		// 0.96
int MigrationWindowsSSO(void);	// 0.96
int GenWriteCheckSynchroValue(void);	// 0.96
int ReadVerifyCheckSynchroValue(int iKeyId);	// 0.96 // 1.12
int InitWindowsSSO(void);	// 0.96

int ChangeMasterPwd(const char *szNewPwd);
int WindowChangeMasterPwd(BOOL bForced);
void SavePortal();
int AddApplicationFromCurrentWindow(BOOL bJustDisplayTheMessage);
void ReactivateApplicationFromCurrentWindow(void);
int PutConfigOnServer(int iAction,int *piNewCategoryId,char *pszDomainIds,char *pszDomainAutoPublish);
int InternetCheckProxyParams(HWND w);
void InternetCheckVersion();
int GetProxyConfig(const char *szComputerName, BOOL *pbInternetUseProxy, char *szProxyURL,char *szProxyUser,char *szProxyPwd);
char *GetNextComputerName(void);

void GenerateCategoryName(int iCategory,char *pszProposition);
void GenerateApplicationName(int iAction,char *pszProposition);
BOOL IsCategoryNameUnique(int iCategory,char *pszCategory);
BOOL IsApplicationNameUnique(int iAction,char *pszApplication);

int SaveMasterPwdLastChange(void);
long GetMasterPwdLastChange(void);
int ChangeApplicationPassword(HWND w,int iAction);
int GetAllConfigsFromServer(void);
int GetAutoPublishConfigsFromServer(void);
int GetNewOrModifiedConfigsFromServer(BOOL bForced);
int DeleteConfigsNotOnServer(void);
int DeleteConfigOnServer(int iAction);
int DeleteCategOnServer(int iAction);

int StoreNodeValue(char *buf,int bufsize,IXMLDOMNode *pNode);

extern BOOL gbDontAskId,gbDontAskPwd; 
extern BOOL gbDontAskId2,gbDontAskId3,gbDontAskId4;
int CALLBACK IdAndPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp);

int LogTranscryptError(char *szLogMessage);

extern const char gcszCfgVersion[];

extern char gszCfgFile[];
extern char gszCfgVersion[];
extern char gszCfgPortal[];
extern BOOL gbSessionLock;     // 0.63B4 : true si verrouillage sur suspension de session windows
extern BOOL gbInternetCheckVersion; // 0.80 : autorise swSSO à se connecter à internet pour vérifier la version
extern BOOL gbInternetGetConfig;	// 0.80 : autorise swSSO à se connecter à internet pour récupérer des configurations
//extern BOOL gbInternetPutConfig;	// 0.80 : autorise swSSO à se connecter à internet pour uploader des configurations
extern BOOL gbInternetManualPutConfig;	// 0.89 : autorise swSSO à se connecter à internet pour uploader manuellement les configurations
extern BOOL gbInternetUseProxy;		// 0.80 : utilise un proxy pour se connecter à internet
extern char gszProxyURL[];			// 0.80 : URL du proxy
extern char gszProxyUser[];			// 0.80 : compte utilisateur pour authentification proxy
extern char gszProxyPwd[];			// 0.80 : mot de passe pour authentification proxy
extern char gszServerAddress[];		// 0.80 : adresse du serveur pour stockage configs ("ws.swsso.fr" par défaut)
extern char gszWebServiceAddress[];	// 0.80 : adresse du serveur pour stockage configs ("/webservice5.php" par défaut)
extern HWND gwPropertySheet;		// 0.83 : handle de la fenetre de config (pour masquage sur désactivation)
extern int gx,gy,gcx,gcy;			// 0.85 : positionnement de la fenêtre sites et applications
extern int gx2,gy2,gcx2,gcy2,gbLaunchTopMost; // 0.91 : positionnement de la fenêtre de lancement d'application
extern int gx3,gy3,gcx3,gcy3;		// 1.05 : positionnement de la fenêtre publishto
extern int gx4,gy4,gcx4,gcy4;		// 1.06 : positionnement de la fenêtre de sélection d'un compte existant

extern BOOL gbDisplayChangeAppPwdDialog ;		// ISSUE#107
extern BOOL gbSSOInternetExplorer;				// ISSUE#176
extern BOOL gbSSOFirefox;						// ISSUE#176
extern BOOL gbSSOChrome;						// ISSUE#176
extern BOOL gbSSOEdge;							// 1.20
extern BOOL gbShowLaunchAppWithoutCtrl;	// ISSUE#254

extern int giActionIdPwdAsked;
extern char *gpszURLBeingAdded;
extern char *gpszTitleBeingAdded;

extern int giLanguage;
extern int giMasterPwdExpiration;

extern BOOL gbParseWindowsOnStart;  // 0.93B4 : parse / ne parse pas les fenêtres ouvertes au lancement de SSO
#define MAX_EXCLUDED_HANDLES 500

extern int  giNbExcludedHandles;
extern HWND gTabExcludedHandles[];

extern HWND gwIdAndPwdDialogProc;

#define LEN_PROXY_USER 50
#define LEN_PROXY_PWD 50
#define LEN_PROXY_URL 512

typedef struct 
{
	BOOL bInternetUseProxy;			
	char szProxyURL[LEN_PROXY_URL+1];		
	char szProxyUser[LEN_PROXY_USER+1];	
	char szProxyPwd[LEN_PROXY_PWD+1];		
}
T_PROXYPARAMS;

typedef struct 
{
	BOOL bCenter;
	int iAction;
	int iTitle;
	char szText[256+1];
} T_IDANDPWDDIALOG;

typedef struct 
{
	BOOL bPBKDF2PwdSaltReady;
	BYTE bufPBKDF2PwdSalt[PBKDF2_SALT_LEN]; // 0.93B6 ISSUE#56 : sel pour le stockage du mot de passe
	BOOL bPBKDF2KeySaltReady;
	BYTE bufPBKDF2KeySalt[PBKDF2_SALT_LEN]; // 0.93B6 ISSUE#56 : sel pour la dérivation de la clé de chiffrement des mots de passe secondaires
} T_SALT;

extern T_SALT gSalts;

#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite à moi...

extern char gszLastADPwdChange[14+1];					// 1.03 : date de dernier changement de mdp dans l'AD, format AAAAMMJJHHMMSS
extern char gszLastADPwdChange2[50+1];					// 1.12 : date de dernier changement de mdp dans l'AD, format HiLong,LoLong
extern char gszEncryptedADPwd[LEN_ENCRYPTED_AES256+1];	// 1.03 : valeur du mot de passe AD (fourni par l'utilisateur)


typedef struct
{
	int iNbConfigsAdded;
	int iNbConfigsModified;
	int iNbConfigsDisabled;
	int iNbConfigsDeleted;
} T_CONFIG_SYNC;

extern T_CONFIG_SYNC gtConfigSync;
void ReportConfigSync(int iErrorMessage,BOOL bShowMessage,BOOL bShowIfZero);
int SyncSecondaryPasswordGroup(void);
void SetLanguage(void);
void GetOSLanguage(void);

extern HWND gwChangeApplicationPassword;