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
// swSSOAppNsites.h
//-----------------------------------------------------------------------------

#define NB_MAX_CATEGORIES	50
#define LEN_CATEGORY_LABEL	 50
#define LEN_APPLICATION_NAME 50

#define LEN_PWD 50
#define LEN_ID  50
// Longueur nécessaire pour chiffrement de 50 octets utiles
// Avec 3DES = (8+56+8)*2+1   = 145
// Avec AES  = (16+64+16)*2+1 = 193
#define LEN_ENCRYPTED_3DES  144
#define LEN_ENCRYPTED_AES256  192
#define LEN_URL 256 // ISSUE#160
#define LEN_TITLE 90
#define LEN_KBSIM 400
#define LEN_FULLPATHNAME 255

#define UNK	   0
#define WEBSSO 1
#define POPSSO 2
#define WINSSO 3
#define XEBSSO 4
#define XINSSO 5
#define WEBPWD 6

#define EDIT   1
#define COMBO  2

#define WAIT_IF_SSO_OK 120
#define WAIT_IF_BAD_URL 5
#define WAIT_IF_SSO_PAGE_NOT_COMPLETE 2
#define WAIT_IF_SSO_NOK 120
#define WAIT_ONE_MINUTE 61 // ATTENTION, CETTE VALEUR DOIT TOUJOURS ETRE DIFFERENTE DE WAIT_IF_SSO_OK !
#define WAIT_IF_LABEL_NOT_FOUND 1

// flags pour iWithIdPwd
#define CONFIG_WITH_ID1 1
#define CONFIG_WITH_ID2 2
#define CONFIG_WITH_ID3 4
#define CONFIG_WITH_ID4 8
#define CONFIG_WITH_PWD 16

typedef struct 
{
	int  iConfigId; // 0.91 : identifiant de la config (synchro avec le serveur)
	char szApplication[LEN_APPLICATION_NAME+1]; // nom de l'application (titre de la section de config)
	char szTitle[LEN_TITLE+1];	// titre (ou partie du titre) de la fenêtre 
	char szURL[LEN_URL+1];	// début de l'URL 
	char szId1Name[90+1];	// nom du contrôle identifiant
	char szId1Value[LEN_ID+1];	// valeur de l'identifiant déchiffré
	char szId2Name[90+1];	// nom du contrôle identifiant
	char szId2Value[LEN_ID+1];	// valeur de l'identifiant déchiffré
	char szId3Name[90+1];	// nom du contrôle identifiant
	char szId3Value[LEN_ID+1];	// valeur de l'identifiant déchiffré
	char szId4Name[90+1];	// nom du contrôle identifiant
	char szId4Value[LEN_ID+1];	// valeur de l'identifiant déchiffré
	char szPwdName[90+1];	// nom du contrôle mot de passe
	char szPwdEncryptedValue[LEN_ENCRYPTED_AES256+1];	// 0.65B3 mot de passe chiffré et encodé en faux base64 
	char szValidateName[90+1];   // id du bouton valider ou nom du formulaire Web
	int  iCategoryId;
	// int  iDomainId; // supprimé en 1.05 : on ne conserve plus localement l'appartenance de la config à un domaine
	int  iType;		// WINSSO | WEBSSO | POPSSO | WEBPWD | XEBSSO | XINSSO
	int  id2Type;	// EDIT | COMBO
	int  id3Type;	// EDIT | COMBO
	int  id4Type;	// EDIT | COMBO
	//time_t tLastDetect;	// derniere détection de cette fenetre 
	//HWND   wLastDetect;	// handle de cette fenetre déjà détectée
	time_t tLastSSO; // 0.93 date dernier SSO réalisé sur cette action
	HWND   wLastSSO; // 0.93 handle fenêtre dernier SSO réalisé sur cette action
	int  iWaitFor;		//
	int  iNbEssais;		//
	BOOL bActive;		// action active
	BOOL bAutoLock;		// 0.66 : désactivation autorisée ou non (oui par défaut)
	// BOOL bAutoPublish;	// 1.14 : configuration publiée automatiquement	- supprimé en 1.15, lié au domaine maintenant ISSUE#349
	//BOOL bConfigOK;		// FALSE au début, TRUE dès qu'on a détecté qu'un SSO a été fait correctement avec cette config
	BOOL bConfigSent;	// TRUE si la config a été envoyée au serveur
	BOOL bKBSim;				// 0.89
	char szKBSim[LEN_KBSIM+1];	// 0.89
	BOOL bWaitForUserAction;	// 0.89
	BOOL bPwdChangeInfos;		// 0.9X : la config contient des éléments pour changement de mot de passe qui sont dans la config N+1
	int  iPwdLen;				// 0.9X : longueur du mot de passe à générer
	char szFullPathName[LEN_FULLPATHNAME+1]; // 0.91 : full path name de la commande à lancer
	char szLastUpload[14+1]; 		//0.91 : date de dernier upload (AAAAMMJJHHMMSS)
	BOOL bSaved;				// 0.93B6 ISSUE#55 : indique si la config a été "appliquée" depuis son ajout. Une config non bSaved apparait dans la fenêtre mais n'est pas exécutée
	BOOL bAddAccount;			// 0.97B1 ISSUE#86 : permet de savoir que la config a été créée par ajout de compte
	BOOL iWithIdPwd;			// 1.03 modifé 1.05 (BOOL -> int) : permet de savoir que la config a été récupérée du serveur avec ids et/ou mdp (donc interdit de voir le mdp et de modifier les ids et mdp)
	int  iPwdGroup;				// 1.03 : groupement des configurations pour changement mot de passe
	BOOL bError;				// 1.11 : config en erreur (par exemple suite à pb de tranchiffrement)
	BOOL bSafe;					// 1.14 : TRUE si c'est une configuration de type mise en coffre, FALSE si c'est une configuration de SSO
} T_ACTION;

typedef struct 
{
	int id;
	HTREEITEM hItem;
	HTREEITEM hItemSelectAccount;
	char szLabel[LEN_CATEGORY_LABEL+1];
	BOOL bExpanded;
}
T_CATEGORY;

extern HWND gwAppNsites;
extern T_CATEGORY *gptCategories;
extern int giNbCategories;
extern int giNextCategId;

int LoadApplications(void);
int SaveApplications(void);
int LoadCategories(void);
int SaveCategories(void);
int ShowAppNsites(int iSelected, BOOL bFromSystray);
BOOL IsApplicationNameUnique(int iAction,char *pszApplication);
BOOL IsCategoryNameUnique(int iCategory,char *pszCategory);
void GenerateCategoryName(int iCategory,char *pszProposition);
void GenerateApplicationName(int iAction,char *pszProposition);
int GetConfigIndex(int id);
int GetCategoryIndex(int id);
int BackupAppsNcategs(void);
void ShowApplicationDetails(HWND w,int iAction);
void GetApplicationDetails(HWND w,int iAction);
int TVItemGetLParam(HWND w,HTREEITEM hItem);
int UploadConfig(HWND w, char *pszDomainIds,char *pszDomainAutoPublish);

// ATTENTION, ces deux fonctions sont utilisées par swSSOLaunchApp.cpp, cela suppose
// que l'identifiant du treeview TV_APPLICATIONS soit bien identique dans les 2 fenêtres !
void FillTreeView(HWND w);
void LaunchSelectedApp(HWND w);

void FillTreeViewDomains(HWND w);
void AddDomain(HWND w);
void DeleteDomain(HWND w);
int GetDomainIndex(int id);
int UploadDomain(HWND w,int iDomain);

extern BOOL gbAjoutDeCompteEnCours;