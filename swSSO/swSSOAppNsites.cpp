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
// swSSOAppNsites.cpp
//-----------------------------------------------------------------------------
// Fenêtre de config des applications et sites
//-----------------------------------------------------------------------------

#if 0
//-----------------------------------------------------------------------------
// ()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int XXX(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void XXX(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	TRACE((TRACE_LEAVE,_F_, ""));
}

#endif

#include "stdafx.h"

// globales globales
T_CATEGORY *gptCategories=NULL;
int giNbCategories=0;
int giNextCategId=1; // ISSUE#184
HWND gwAppNsites=NULL;

// globales locales
static BOOL gbTVApplicationsLabelEditing=FALSE;
static BOOL gbTVDomainsLabelEditing=FALSE;
// static BOOL gbAtLeastOneAppRenamed=FALSE; // 0.90B1 : renommage direct, flag inutile
static BOOL gbEffacementEnCours=FALSE;
static BOOL gbShowPwd;
static BOOL gbShowGeneratedPwd;
static char buf2048[2048]; // gros buffer pas beau
static HBRUSH ghTabBrush=NULL;

char gcszUsername[]="%USERNAME%";
char gcszADPassword[]="%ADPASSWORD%";

#define TB_PWD_SUBCLASS_ID 1
#define TB_PWD_CLEAR_SUBCLASS_ID 2
static BOOL gbPwdSubClass=FALSE;
static BOOL gbPwdClearSubClass=FALSE;

BOOL gbIsChanging=FALSE;

typedef struct {
	int iNbModified;
	char szId1Value[LEN_ID+1];	
	char szId2Value[LEN_ID+1];	
	char szId3Value[LEN_ID+1];	
	char szId4Value[LEN_ID+1];	
	char szPwdClearValue[LEN_PWD+1];
	char szPwdEncryptedValue[LEN_ENCRYPTED_AES256+1];
	BOOL bId1Modified;
	BOOL bId2Modified;
	BOOL bId3Modified;
	BOOL bId4Modified;
	BOOL bPwdModified;
} T_IDSNPWD;
T_IDSNPWD gIds; // identifiants et mot de passe (clearvalue effacée immédiatement après chiffrement)

typedef struct
{
	int iSelected;
	BOOL bFromSystray;
} T_APPNSITES;

// 0.89B2 (backup/restore mémoire des catégories et applications)
static T_CATEGORY *gptBackupCategories=NULL;
static T_ACTION   *gptBackupActions=NULL;
static int		   giBackupNbCategories=0;
static int		   giBackupNbActions=0;

static int giRefreshTimer=10;
static int giSetForegroundTimer=20;

// id menu popup listbox applis
#define MENU_ACTIVER	50001
#define MENU_DESACTIVER 50002
#define MENU_RENOMMER	50003
#define MENU_DEPLACER	50004
#define MENU_SUPPRIMER  50005
#define MENU_AJOUTER_APPLI	50006
#define MENU_AJOUTER_CATEG	50007
#define MENU_PUBLISH   50008
#define MENU_DUPLIQUER   50010
#define MENU_IMPORTER   50011
#define MENU_EXPORTER   50012
#define MENU_CHANGER_IDS 50013
#define MENU_LANCER_APPLI 50014
#define MENU_AJOUTER_COMPTE 50015
#define MENU_PUBLISH_TO   50016
#define SUBMENU_CATEG			50100
#define MENU_RENOMMER_DOMAINE	50017
#define MENU_SUPPRIMER_DOMAINE	50018
#define MENU_AJOUTER_DOMAINE 50019

#define TYPE_APPLICATION	1
#define TYPE_CATEGORY		2

#define ACTIVATE_YES		1
#define ACTIVATE_NO			2
#define ACTIVATE_TOGGLE		3


void TVItemSetLParam(HWND w,HTREEITEM hItem,LPARAM lp);
int TVItemGetText(HWND w,HTREEITEM hItem,char *pszText,int sizeofText);
HTREEITEM TVAddApplication(HWND w,int iAction,char *pszApplication,BOOL bWithId);
HTREEITEM TVAddCategory(HWND w,int iCategory);
void ClearApplicationDetails(HWND w);
void ShowApplicationDetails(HWND w,int iAction);
static void MoveControls(HWND w, HWND wToRefresh);

// pour l'énumération des collpased...
static char *gpNextCollapsedCategory=NULL;
static char *gpCollapsedCategoryContext=NULL;
static char gszEnumCollapsedCategories[1024];

static BOOL gbAddingDomain=FALSE;

// ISSUE#111 - tooltips
static HWND gwTip=NULL;
typedef struct
{
	int iTip;
	int idString;
} T_TIP;
T_TIP gtip[44];

typedef struct
{
	BOOL bCateg;
	int iCategoryId;
	int iAction;
} T_PUBLISH_STRUCT;
T_PUBLISH_STRUCT gtPublish;


// ----------------------------------------------------------------------------------
// InitTooltip()
// ----------------------------------------------------------------------------------
// ISSUE#111 : le grand retour des tooltips !
// ----------------------------------------------------------------------------------
static void InitTooltip(HWND w)
{
    TOOLINFO ti;
	int i;

	gtip[0].iTip=IMG_ID;			gtip[0].idString=IDS_TIP_TB_ID;
	gtip[1].iTip=TB_ID;				gtip[1].idString=IDS_TIP_TB_ID;
	gtip[2].iTip=IMG_PWD;			gtip[2].idString=IDS_TIP_TB_PWD;
	gtip[3].iTip=TB_PWD;			gtip[3].idString=IDS_TIP_TB_PWD;
	gtip[4].iTip=IMG_ID2;			gtip[4].idString=IDS_TIP_TB_ID2;
	gtip[5].iTip=TB_ID2;			gtip[5].idString=IDS_TIP_TB_ID2;
	gtip[6].iTip=IMG_ID3;			gtip[6].idString=IDS_TIP_TB_ID3;
	gtip[7].iTip=TB_ID3;			gtip[7].idString=IDS_TIP_TB_ID3;
	gtip[8].iTip=IMG_ID4;			gtip[8].idString=IDS_TIP_TB_ID4;
	gtip[9].iTip=TB_ID4;			gtip[9].idString=IDS_TIP_TB_ID4;
	gtip[10].iTip=IMG_TYPE;			gtip[10].idString=IDS_TIP_CB_TYPE;
	gtip[11].iTip=CB_TYPE;			gtip[11].idString=IDS_TIP_CB_TYPE;
	gtip[12].iTip=IMG_TITRE;			gtip[12].idString=IDS_TIP_TB_TITRE;
	gtip[13].iTip=TB_TITRE;			gtip[13].idString=IDS_TIP_TB_TITRE;
	gtip[14].iTip=IMG_URL;			gtip[14].idString=IDS_TIP_TB_URL;
	gtip[15].iTip=TB_URL;			gtip[15].idString=IDS_TIP_TB_URL;
	gtip[16].iTip=IMG_ID_ID;			gtip[16].idString=IDS_TIP_TB_ID_ID;
	gtip[17].iTip=TB_ID_ID;      	gtip[17].idString=IDS_TIP_TB_ID_ID;
	gtip[18].iTip=IMG_PWD_ID;     	gtip[18].idString=IDS_TIP_TB_PWD_ID;
	gtip[19].iTip=TB_PWD_ID;     	gtip[19].idString=IDS_TIP_TB_PWD_ID;
	gtip[20].iTip=IMG_VALIDATION; 	gtip[20].idString=IDS_TIP_TB_VALIDATION;
	gtip[21].iTip=TB_VALIDATION; 	gtip[21].idString=IDS_TIP_TB_VALIDATION;
	gtip[22].iTip=IMG_LANCEMENT;  	gtip[22].idString=IDS_TIP_TB_LANCEMENT;
	gtip[23].iTip=TB_LANCEMENT;  	gtip[23].idString=IDS_TIP_TB_LANCEMENT;
	gtip[24].iTip=IMG_ID2_TYPE;      gtip[24].idString=IDS_TIP_CB_ID2_TYPE;
	gtip[25].iTip=CB_ID2_TYPE;      gtip[25].idString=IDS_TIP_CB_ID2_TYPE;
	gtip[26].iTip=IMG_ID2_ID;      	gtip[26].idString=IDS_TIP_TB_ID2_ID;
	gtip[27].iTip=TB_ID2_ID;      	gtip[27].idString=IDS_TIP_TB_ID2_ID;
	gtip[28].iTip=IMG_ID3_TYPE;      gtip[28].idString=IDS_TIP_CB_ID3_TYPE;
	gtip[29].iTip=CB_ID3_TYPE;      gtip[29].idString=IDS_TIP_CB_ID3_TYPE;
	gtip[30].iTip=IMG_ID3_ID;      	gtip[30].idString=IDS_TIP_TB_ID3_ID;
	gtip[31].iTip=TB_ID3_ID;      	gtip[31].idString=IDS_TIP_TB_ID3_ID;
	gtip[32].iTip=IMG_ID4_TYPE;      gtip[32].idString=IDS_TIP_CB_ID4_TYPE;
	gtip[33].iTip=CB_ID4_TYPE;      gtip[33].idString=IDS_TIP_CB_ID4_TYPE;
	gtip[34].iTip=IMG_ID4_ID;      	gtip[34].idString=IDS_TIP_TB_ID4_ID;
	gtip[35].iTip=TB_ID4_ID;      	gtip[35].idString=IDS_TIP_TB_ID4_ID;
	gtip[36].iTip=CK_KBSIM;      	gtip[36].idString=IDS_TIP_TB_KBSIM;
	gtip[37].iTip=TB_KBSIM;      	gtip[37].idString=IDS_TIP_TB_KBSIM;
	gtip[38].iTip=IMG_LOUPE;      	gtip[38].idString=IDS_TIP_LOUPE;
	gtip[39].iTip=IMG_KBSIM;      	gtip[39].idString=IDS_TIP_TB_KBSIM;
    gtip[40].iTip=IMG_PWD_GROUP;    gtip[40].idString=IDS_TIP_PWD_GROUP;
    gtip[41].iTip=TB_PWD_GROUP;     gtip[41].idString=IDS_TIP_PWD_GROUP;
    gtip[42].iTip=CK_AUTO_LOCK;     gtip[42].idString=IDS_TIP_AUTO_LOCK;
    gtip[43].iTip=IMG_AUTO_LOCK;    gtip[43].idString=IDS_TIP_AUTO_LOCK;
	gwTip = CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
							WS_POPUP | TTS_ALWAYSTIP /*| TTS_BALLOON*/,	
							CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
							w,NULL,ghInstance,NULL);
    SetWindowPos(gwTip,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SendMessage(gwTip,TTM_SETMAXTIPWIDTH,0,250);
	ZeroMemory(&ti,sizeof(ti));
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND ;
    ti.hinst = ghInstance;

	for (i=0;i<44;i++)
	{
		ti.hwnd = w;
	    ti.lpszText=GetString(gtip[i].idString);
	    ti.uId = (UINT_PTR)GetDlgItem(w,gtip[i].iTip);
	    SendMessage(gwTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
		SendMessage(gwTip, TTM_SETDELAYTIME, TTDT_AUTOPOP,MAKELPARAM(20000,0)); 
	}
} 

// ----------------------------------------------------------------------------------
// TermTooltip()
// ----------------------------------------------------------------------------------
// ISSUE#111 : le grand retour des tooltips !
// ----------------------------------------------------------------------------------
static void TermTooltip(void)
{
	if (gwTip!=NULL)
	{
		DestroyWindow(gwTip);
		gwTip=NULL;
	}
}

//-----------------------------------------------------------------------------
// PublishToMoveControls()
//-----------------------------------------------------------------------------
// Repositionne les contrôles suite à redimensionnement de la fenêtre
//-----------------------------------------------------------------------------
static void PublishToMoveControls(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	RECT rect;
	GetClientRect(w,&rect);

	SetWindowPos(GetDlgItem(w,IDOK),NULL,rect.right-84,rect.top+10,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,IDCANCEL),NULL,rect.right-84,rect.top+40,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,PB_TOUT),NULL,rect.right-84,rect.bottom-60,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,PB_RIEN),NULL,rect.right-84,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,LV_DOMAINS),NULL,0,0,rect.right-104,rect.bottom-18,SWP_NOMOVE | SWP_NOZORDER);

	//InvalidateRect(w,NULL,FALSE);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// PublishToSavePosition()
//-----------------------------------------------------------------------------
// Sauvegarde position/taille de la fenêtre publishTo
//-----------------------------------------------------------------------------
static void PublishToSavePosition(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	RECT rect;
	char s[8+1];

	if (IsIconic(w)) goto end;
	GetWindowRect(w,&rect);
	gx3=rect.left;
	gy3=rect.top;
	gcx3=rect.right-rect.left;
	gcy3=rect.bottom-rect.top;
	wsprintf(s,"%d",gx3);  WritePrivateProfileString("swSSO","x3",s,gszCfgFile);
	wsprintf(s,"%d",gy3);  WritePrivateProfileString("swSSO","y3",s,gszCfgFile);
	wsprintf(s,"%d",gcx3); WritePrivateProfileString("swSSO","cx3",s,gszCfgFile);
	wsprintf(s,"%d",gcy3); WritePrivateProfileString("swSSO","cy3",s,gszCfgFile);

end:
	StoreIniEncryptedHash(); // ISSUE#164
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// PublishToOnInitDialog()
//-----------------------------------------------------------------------------
// WM_INIT_DIALOG de la fenêtre de publication vers les domaines
//-----------------------------------------------------------------------------
void PublishToOnInitDialog(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	LVITEM lvi;
	int i,j;
	T_DOMAIN tabConfigDomains[MAX_DOMAINS];
	int iNbDomains;
	HCURSOR hCursorOld=NULL;
	hCursorOld=SetCursor(ghCursorWait);

	// icone ALT-TAB
	SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
	SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 

	// Positionnement et dimensionnement de la fenêtre (si shift enfoncée à l'ouverture, retaillage et repositionnement par défaut)
	if ((gx3!=-1 && gy3!=-1 && gcx3!=-1 && gcy3!=-1) && HIBYTE(GetAsyncKeyState(VK_SHIFT))==0) 
	{
		SetWindowPos(w,NULL,gx3,gy3,gcx3,gcy3,SWP_NOZORDER);
	}
	else // position par défaut (centrée sur fenêtre appNsites)
	{
		SetWindowPos(w,NULL,gx+100,gy+100,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	PublishToMoveControls(w);
	
	// affichage de la liste des domaines
	ListView_SetExtendedListViewStyle(GetDlgItem(w,LV_DOMAINS),LVS_EX_CHECKBOXES);
		
	// récupère la liste des domaines auxquels la config est rattachée (si c'est une categ on ne coche rien)
	if (!gtPublish.bCateg)
	{
		iNbDomains=GetDomains(FALSE,gptActions[gtPublish.iAction].iConfigId,tabConfigDomains); 
	}
	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	for (i=0;i<giNbDomains;i++)
	{
		lvi.iItem=i;
		lvi.iSubItem=0;
		lvi.pszText=gtabDomains[i].szDomainLabel;
		lvi.lParam=gtabDomains[i].iDomainId;
		ListView_InsertItem(GetDlgItem(w,LV_DOMAINS),&lvi);
		if (!gtPublish.bCateg)
		{
			for (j=0;j<iNbDomains;j++)
			{
				if (gtabDomains[i].iDomainId==tabConfigDomains[j].iDomainId) 
				{
					ListView_SetCheckState(GetDlgItem(w,LV_DOMAINS),i,TRUE);
				}
			}
		}
	}
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// LVCheckAllDomains()
//-----------------------------------------------------------------------------
// Coche / décoche tous les domaines
//-----------------------------------------------------------------------------
void LVCheckAllDomains(HWND w,BOOL bChecked,BOOL bCommun)
{
	TRACE((TRACE_ENTER,_F_, "bChecked=%d",bChecked));
	int i;
	if (bCommun) ListView_SetCheckState(GetDlgItem(w,LV_DOMAINS),0,FALSE);
	for (i=1;i<ListView_GetItemCount(GetDlgItem(w,LV_DOMAINS));i++)
	{
		ListView_SetCheckState(GetDlgItem(w,LV_DOMAINS),i,bChecked);
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// PublishConfigToDomains()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int PublishConfigToDomains(HWND w)
{
	UNREFERENCED_PARAMETER(w);
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szDomainIds[1024];
	char szDomainId[10];
	int i;
	int iNbDomains;
	LVITEM lvitem;

	*szDomainIds=0;

	// récupère la liste des domaines cochés
	iNbDomains=ListView_GetItemCount(GetDlgItem(w,LV_DOMAINS));
	for (i=0;i<iNbDomains;i++)
	{
		if (ListView_GetCheckState(GetDlgItem(w,LV_DOMAINS),i))
		{
			lvitem.mask=LVIF_PARAM;
			lvitem.iItem=i;
			lvitem.iSubItem=0;
			if (ListView_GetItem(GetDlgItem(w,LV_DOMAINS),&lvitem))
			{
				sprintf_s(szDomainId,sizeof(szDomainId),"%d,",lvitem.lParam);
				strcat_s(szDomainIds,sizeof(szDomainIds),szDomainId);
			}
		}
	}
	rc=UploadConfig(gwAppNsites,szDomainIds);

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// PublishToDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de publication vers les domaines
//-----------------------------------------------------------------------------
static int CALLBACK PublishToDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			PublishToOnInitDialog(w);
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
					if (PublishConfigToDomains(w)==0) 
					{
						PublishToSavePosition(w);
						EndDialog(w,IDOK);
					}
					break;
				case IDCANCEL:
					PublishToSavePosition(w);
					EndDialog(w,IDCANCEL);
					break;
				case PB_TOUT:
					LVCheckAllDomains(w,TRUE,TRUE);
					break;
				case PB_RIEN:
					LVCheckAllDomains(w,FALSE,TRUE);
					break;
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_SIZE:			
			PublishToMoveControls(w);
			break;
		case WM_SIZING:
			{
				RECT *pRectNewSize=(RECT*)lp;
				TRACE((TRACE_DEBUG,_F_,"RectNewSize=%d,%d,%d,%d",pRectNewSize->top,pRectNewSize->left,pRectNewSize->bottom,pRectNewSize->right));
				if (pRectNewSize->right-pRectNewSize->left < 379)  pRectNewSize->right=pRectNewSize->left+379;
				if (pRectNewSize->bottom-pRectNewSize->top < 284)  pRectNewSize->bottom=pRectNewSize->top+284;
				rc=TRUE;
			}
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case LVN_ITEMCHANGED: 
					{
						LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lp; 
						TRACE((TRACE_DEBUG,_F_,"LVN_ITEMCHANGED: item=%d uchanged=%d unewstate=%d",pnmv->iItem,pnmv->uChanged,pnmv->uNewState));
					}
					break;
				case NM_CLICK:
					{
						LPNMITEMACTIVATE pnmv = (LPNMITEMACTIVATE)lp;
						TRACE((TRACE_DEBUG,_F_,"NM_CLICK: item=%d uchanged=%d unewstate=%d",pnmv->iItem,pnmv->uChanged,pnmv->uNewState));
						
						UINT itemState=ListView_GetItemState(GetDlgItem(w,LV_DOMAINS),pnmv->iItem,LVIS_SELECTED);
						BOOL b=ListView_GetCheckState(GetDlgItem(w,LV_DOMAINS),pnmv->iItem);
						if (itemState & LVIS_SELECTED)
						{
							ListView_SetItemState(GetDlgItem(w,LV_DOMAINS),pnmv->iItem,0,LVIS_SELECTED);
							ListView_SetCheckState(GetDlgItem(w,LV_DOMAINS),pnmv->iItem,!b);
						}
						if (pnmv->iItem==0) // domaine commun --> si coché décoche tout le reste
						{
							if (!b) LVCheckAllDomains(w,FALSE,FALSE);
						}
						else // autre domaine coché, décoche commun
						{
							if (!b) ListView_SetCheckState(GetDlgItem(w,LV_DOMAINS),0,FALSE);
						}
					}
					break;
			}
			break;
	}
	return rc;
}
//-----------------------------------------------------------------------------
// PublishTo()
//-----------------------------------------------------------------------------
// Affiche la fenêtre de publication vers les domaines et fait l'upload
//-----------------------------------------------------------------------------
int PublishTo(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HTREEITEM hItem,hParentItem;
	
	// Récupère l'élément sélectionné et regarde si c'est une config ou une catégorie
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie 
	{
		gtPublish.bCateg=TRUE;
		gtPublish.iCategoryId=TVItemGetLParam(w,hItem);
		if (gtPublish.iCategoryId==-1) goto end;
	}
	else // c'est une config
	{
		gtPublish.bCateg=FALSE;
		gtPublish.iAction=TVItemGetLParam(w,hItem); 
		if (gtPublish.iAction==-1 || gtPublish.iAction>=giNbActions) goto end;	
	}

	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_PUBLISH_TO),w,PublishToDialogProc)!=IDOK) goto end;

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// BackupAppsNcategs()
//-----------------------------------------------------------------------------
// Sauvegarde (en mémoire) la table des catégories et des applications pour 
// pouvoir la restaurer lorsque l'utilisateur annule ses modifications
// dans la fenêtre AppNSites (remarque : c'était fait par rechargement complet
// depuis le fichier swsso.ini dans les versions <= 0.89B1
// Remarque : si échec de backup ou restore mémoire, retour à l'ancienne méthode
//-----------------------------------------------------------------------------
int BackupAppsNcategs(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	// on est dans ce cas si l'utilisateur fait "appliquer"
	if (gptBackupCategories!=NULL) { free(gptBackupCategories); gptBackupCategories=NULL; }
	if (gptBackupActions!=NULL) { free(gptBackupActions); gptBackupActions=NULL; }
	giBackupNbCategories=0;
	giBackupNbActions=0;
	
	// catégories
	gptBackupCategories=(T_CATEGORY *)malloc(sizeof(T_CATEGORY)*giNbCategories);
	if (gptBackupCategories==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(T_CATEGORY)*giNbCategories)); goto end; }
    giBackupNbCategories=giNbCategories;
	memcpy(gptBackupCategories,gptCategories,sizeof(T_CATEGORY)*giNbCategories);
	
	// actions
	gptBackupActions=(T_ACTION *)malloc(sizeof(T_ACTION)*giNbActions);
	if (gptBackupActions==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(T_ACTION)*giNbActions)); goto end; }
    giBackupNbActions=giNbActions;
	memcpy(gptBackupActions,gptActions,sizeof(T_ACTION)*giNbActions);
	
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RestoreAppsNcategs()
//-----------------------------------------------------------------------------
// cf. BackupAppsNcategs
//-----------------------------------------------------------------------------
int RestoreAppsNcategs(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	if (gptBackupCategories==NULL || gptBackupActions==NULL) goto end;
	
	// catégories
    giNbCategories=giBackupNbCategories;
	memcpy(gptCategories,gptBackupCategories,sizeof(T_CATEGORY)*giBackupNbCategories);

	// actions
    giNbActions=giBackupNbActions;
	memcpy(gptActions,gptBackupActions,sizeof(T_ACTION)*giBackupNbActions);

	rc=0;
end:
	if (gptBackupCategories!=NULL) { free(gptBackupCategories); gptBackupCategories=NULL; }
	if (gptBackupActions!=NULL) { free(gptBackupActions); gptBackupActions=NULL; }

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// HideConfigControls()
//-----------------------------------------------------------------------------
void HideConfigControls(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	if (!gbEnableOption_ViewAppConfig)
	{
		ShowWindow(GetDlgItem(w,TB_TITRE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_URL),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID2_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID3_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID4_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD_GROUP),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID2_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID3_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID4_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_TITRE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_URL),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID2_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID3_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID4_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_PWD_GROUP),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_AUTO_LOCK),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_PWD_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID2_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID3_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID4_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TAB_CONFIG),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_KBSIM),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_KBSIM),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_LANCEMENT),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_LANCEMENT),SW_HIDE);
		ShowWindow(GetDlgItem(w,PB_PARCOURIR),SW_HIDE);
		// ISSUE#199
		ShowWindow(GetDlgItem(w,IMG_TYPE)	  ,SW_HIDE); 
		ShowWindow(GetDlgItem(w,IMG_TITRE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_URL)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_PWD_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_KBSIM)     ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_LANCEMENT) ,SW_HIDE);
	}	
	TRACE((TRACE_LEAVE,_F_, ""));
}
//-----------------------------------------------------------------------------
// DisableConfigControls()
//-----------------------------------------------------------------------------
void DisableConfigControls(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	if (!gbEnableOption_ModifyAppConfig)
	{
		EnableWindow(GetDlgItem(w,TB_TITRE),FALSE);
		EnableWindow(GetDlgItem(w,TB_URL),FALSE);
		EnableWindow(GetDlgItem(w,TB_ID_ID),FALSE);	
		EnableWindow(GetDlgItem(w,TB_ID2_ID),FALSE);
		EnableWindow(GetDlgItem(w,TB_ID3_ID),FALSE);
		EnableWindow(GetDlgItem(w,TB_ID4_ID),FALSE);
		EnableWindow(GetDlgItem(w,TB_PWD_GROUP),FALSE);
		EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),FALSE);
		EnableWindow(GetDlgItem(w,TB_PWD_ID),FALSE);
		EnableWindow(GetDlgItem(w,TB_VALIDATION),FALSE);
		EnableWindow(GetDlgItem(w,CB_TYPE),FALSE);
		EnableWindow(GetDlgItem(w,CB_ID2_TYPE),FALSE);
		EnableWindow(GetDlgItem(w,CB_ID3_TYPE),FALSE);
		EnableWindow(GetDlgItem(w,CB_ID4_TYPE),FALSE);
		EnableWindow(GetDlgItem(w,CK_KBSIM),FALSE);
		EnableWindow(GetDlgItem(w,TB_KBSIM),FALSE);
		EnableWindow(GetDlgItem(w,TB_LANCEMENT),FALSE);
		EnableWindow(GetDlgItem(w,PB_PARCOURIR),FALSE);
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// EnableControls()
//-----------------------------------------------------------------------------
// [in] iType=UNK | WINSSO | POPSSO | WEBSSO | XEBSSO
//-----------------------------------------------------------------------------
void EnableControls(HWND w,int iType,BOOL bEnable)
{
	TRACE((TRACE_ENTER,_F_, ""));

	if (!bEnable)
	{
		SetDlgItemText(w,TB_ID,"");			EnableWindow(GetDlgItem(w,TB_ID),FALSE);
		SetDlgItemText(w,TB_ID2,"");		EnableWindow(GetDlgItem(w,TB_ID2),FALSE);
		SetDlgItemText(w,TB_ID3,"");		EnableWindow(GetDlgItem(w,TB_ID3),FALSE);
		SetDlgItemText(w,TB_ID4,"");		EnableWindow(GetDlgItem(w,TB_ID4),FALSE);	
		SetDlgItemText(w,TB_PWD,"");		EnableWindow(GetDlgItem(w,TB_PWD),FALSE);
		SetDlgItemText(w,TB_PWD_CLEAR,"");		EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),FALSE);
		if (gbAdmin) 
		{
			EnableWindow(GetDlgItem(w,CK_AD_ID),FALSE);
			EnableWindow(GetDlgItem(w,CK_AD_PWD),FALSE);
		}
		EnableWindow(GetDlgItem(w,IMG_LOUPE),FALSE);
		SetDlgItemText(w,TB_TITRE,"");		EnableWindow(GetDlgItem(w,TB_TITRE),FALSE);
		SetDlgItemText(w,TB_URL,"");		EnableWindow(GetDlgItem(w,TB_URL),FALSE);
		SetDlgItemText(w,TB_ID_ID,"");		EnableWindow(GetDlgItem(w,TB_ID_ID),FALSE);	
		SetDlgItemText(w,TB_ID2_ID,"");		EnableWindow(GetDlgItem(w,TB_ID2_ID),FALSE);
		SetDlgItemText(w,TB_ID3_ID,"");		EnableWindow(GetDlgItem(w,TB_ID3_ID),FALSE);
		SetDlgItemText(w,TB_ID4_ID,"");		EnableWindow(GetDlgItem(w,TB_ID4_ID),FALSE);
		SetDlgItemText(w,TB_PWD_GROUP,"");	EnableWindow(GetDlgItem(w,TB_PWD_GROUP),FALSE);
		EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),FALSE);
		SetDlgItemText(w,TB_PWD_ID,"");		EnableWindow(GetDlgItem(w,TB_PWD_ID),FALSE);
		SetDlgItemText(w,TB_VALIDATION,"");	EnableWindow(GetDlgItem(w,TB_VALIDATION),FALSE);
		SendMessage(GetDlgItem(w,CB_TYPE),CB_SETCURSEL,0,0);		EnableWindow(GetDlgItem(w,CB_TYPE),FALSE);
		SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_SETCURSEL,0,0);	EnableWindow(GetDlgItem(w,CB_ID2_TYPE),FALSE);
		SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_SETCURSEL,0,0);	EnableWindow(GetDlgItem(w,CB_ID3_TYPE),FALSE);
		SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_SETCURSEL,0,0);	EnableWindow(GetDlgItem(w,CB_ID4_TYPE),FALSE);
		EnableWindow(GetDlgItem(w,CK_KBSIM),FALSE);
		SetDlgItemText(w,TB_KBSIM,"");		EnableWindow(GetDlgItem(w,TB_KBSIM),FALSE);
		SetDlgItemText(w,TB_LANCEMENT,"");	EnableWindow(GetDlgItem(w,TB_LANCEMENT),FALSE);
		EnableWindow(GetDlgItem(w,PB_PARCOURIR),FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(w,TB_ID),TRUE);
		EnableWindow(GetDlgItem(w,TB_PWD),TRUE);
		EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),TRUE);
		EnableWindow(GetDlgItem(w,IMG_LOUPE),TRUE);
		if (gbAdmin) 
		{
			EnableWindow(GetDlgItem(w,CK_AD_ID),TRUE);
			EnableWindow(GetDlgItem(w,CK_AD_PWD),TRUE);
		}
		EnableWindow(GetDlgItem(w,CB_TYPE),TRUE);
		EnableWindow(GetDlgItem(w,CK_KBSIM),TRUE);
		EnableWindow(GetDlgItem(w,TB_KBSIM),TRUE); //////// TODO : NE PAS DEGRISER SI SUR L'AUTRE ONGLET
		EnableWindow(GetDlgItem(w,PB_PARCOURIR),TRUE);
		EnableWindow(GetDlgItem(w,TB_LANCEMENT),TRUE);
		switch (iType)
		{
			case UNK:
				EnableWindow(GetDlgItem(w,TB_ID2),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID3),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID4),FALSE);	
				EnableWindow(GetDlgItem(w,TB_TITRE),FALSE);
				EnableWindow(GetDlgItem(w,TB_URL),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID_ID),FALSE);	
				EnableWindow(GetDlgItem(w,TB_PWD_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_VALIDATION),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID2_TYPE),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID3_TYPE),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID4_TYPE),FALSE);	
				EnableWindow(GetDlgItem(w,TB_ID2_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID3_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID4_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_PWD_GROUP),FALSE);
				EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),FALSE);
				EnableWindow(GetDlgItem(w,CK_KBSIM),FALSE);
				EnableWindow(GetDlgItem(w,TB_KBSIM),FALSE);
				EnableWindow(GetDlgItem(w,PB_PARCOURIR),FALSE);
				EnableWindow(GetDlgItem(w,TB_LANCEMENT),FALSE);
				break;
			case POPSSO:
				EnableWindow(GetDlgItem(w,TB_ID2),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID3),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID4),TRUE);	
				EnableWindow(GetDlgItem(w,TB_TITRE),TRUE);
				EnableWindow(GetDlgItem(w,TB_URL),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID_ID),FALSE);	
				EnableWindow(GetDlgItem(w,TB_PWD_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_VALIDATION),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID2_TYPE),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID3_TYPE),FALSE);
				EnableWindow(GetDlgItem(w,CB_ID4_TYPE),FALSE);	
				EnableWindow(GetDlgItem(w,TB_ID2_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID3_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_ID4_ID),FALSE);
				EnableWindow(GetDlgItem(w,TB_PWD_GROUP),TRUE);
				EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),TRUE);
				break;
			case WINSSO:
				EnableWindow(GetDlgItem(w,TB_ID2),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID3),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID4),TRUE);	
				EnableWindow(GetDlgItem(w,TB_TITRE),TRUE);
				EnableWindow(GetDlgItem(w,TB_URL),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID_ID),TRUE);	
				EnableWindow(GetDlgItem(w,TB_PWD_ID),TRUE);
				EnableWindow(GetDlgItem(w,TB_VALIDATION),TRUE);
				EnableWindow(GetDlgItem(w,CB_ID2_TYPE),TRUE); // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,CB_ID3_TYPE),TRUE); // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,CB_ID4_TYPE),TRUE); // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,TB_ID2_ID),TRUE);   // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,TB_ID3_ID),TRUE);   // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,TB_ID4_ID),TRUE);   // 0.90B1 : FALSE -> TRUE
				EnableWindow(GetDlgItem(w,TB_PWD_GROUP),TRUE);
				EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),TRUE);
				break;
			case WEBSSO:
			case XEBSSO:
				EnableWindow(GetDlgItem(w,TB_ID2),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID3),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID4),TRUE);	
				EnableWindow(GetDlgItem(w,TB_TITRE),TRUE);
				EnableWindow(GetDlgItem(w,TB_URL),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID_ID),TRUE);	
				EnableWindow(GetDlgItem(w,TB_PWD_ID),TRUE);
				EnableWindow(GetDlgItem(w,TB_VALIDATION),TRUE);
				EnableWindow(GetDlgItem(w,CB_ID2_TYPE),TRUE);
				EnableWindow(GetDlgItem(w,CB_ID3_TYPE),TRUE);
				EnableWindow(GetDlgItem(w,CB_ID4_TYPE),TRUE);	
				EnableWindow(GetDlgItem(w,TB_ID2_ID),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID3_ID),TRUE);
				EnableWindow(GetDlgItem(w,TB_ID4_ID),TRUE);
				EnableWindow(GetDlgItem(w,TB_PWD_GROUP),TRUE);
				EnableWindow(GetDlgItem(w,CK_AUTO_LOCK),TRUE);
				break;
		}
	}
	DisableConfigControls(w);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// SaveWindowPos()
//-----------------------------------------------------------------------------
// Sauvegarde position/taille/collapse de la fenêtre appNsites
//-----------------------------------------------------------------------------
static void SaveWindowPos(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	RECT rect;
	char s[8+1];
	HTREEITEM hItem;
	UINT uiItemState;
	int iCategId,iCategIndex;

	if (IsIconic(w)) goto end;
	GetWindowRect(w,&rect);
	gx=rect.left;
	gy=rect.top;
	gcx=rect.right-rect.left;
	gcy=rect.bottom-rect.top;
	wsprintf(s,"%d",gx);  WritePrivateProfileString("swSSO","x",s,gszCfgFile);
	wsprintf(s,"%d",gy);  WritePrivateProfileString("swSSO","y",s,gszCfgFile);
	wsprintf(s,"%d",gcx); WritePrivateProfileString("swSSO","cx",s,gszCfgFile);
	wsprintf(s,"%d",gcy); WritePrivateProfileString("swSSO","cy",s,gszCfgFile);

	// 0.92B4 : sauvegarde aussi l'état du collapse de la liste
	*buf2048=0;
	hItem=TreeView_GetRoot(GetDlgItem(w,TV_APPLICATIONS));
	while (hItem!=NULL)
	{
		iCategId=TVItemGetLParam(w,hItem);
		TRACE((TRACE_DEBUG,_F_,"TVItemGetLParam=%d",iCategId));
		if (iCategId!=-1)
		{
			uiItemState=TreeView_GetItemState(GetDlgItem(w,TV_APPLICATIONS),hItem,TVIF_STATE);
			TRACE((TRACE_DEBUG,_F_,"Categ %d : item state=%d (%s)",iCategId,uiItemState,(uiItemState & TVIS_EXPANDED)?"Expanded":"Not expanded"));
			iCategIndex=GetCategoryIndex(iCategId);
			if (iCategIndex!=-1) gptCategories[iCategIndex].bExpanded=(uiItemState & TVIS_EXPANDED);
			if (!(uiItemState & TVIS_EXPANDED))
			{ 
				wsprintf(s,"%d",iCategId); 
				strcat_s(buf2048,sizeof(buf2048),s); 
				strcat_s(buf2048,sizeof(buf2048),":");
			}
   		}
		hItem=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hItem);
	}
	WritePrivateProfileString("swSSO","CollapsedCategs",buf2048,gszCfgFile);

end:
	StoreIniEncryptedHash(); // ISSUE#164
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// GetCategoryIndex()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetCategoryIndex(int id)
{
	TRACE((TRACE_ENTER,_F_, "id=%d",id));
	int rc=-1;
	int i;

	for (i=0;i<giNbCategories;i++)
	{
		if (gptCategories[i].id==id) { rc=i; goto end;}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetDomainIndex()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetDomainIndex(int id)
{
	TRACE((TRACE_ENTER,_F_, "id=%d",id));
	int rc=-1;
	int i;

	for (i=0;i<giNbDomains;i++)
	{
		if (gtabDomains[i].iDomainId==id) { rc=i; goto end;}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// TVSelectItemFromLParam()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void TVSelectItemFromLParam(HWND w,int iAppOrCateg,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "lp=%ld",lp));
	if (lp==-1) goto end;
	HTREEITEM hNextCateg,hNextApp;
	hNextCateg=TreeView_GetRoot(GetDlgItem(w,TV_APPLICATIONS));
	while (hNextCateg!=NULL)
	{
		if (iAppOrCateg==TYPE_CATEGORY)
		{
			if (TVItemGetLParam(w,hNextCateg)==lp)
			{
				TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNextCateg);
				goto end;
			}
		}
		else
		{
			hNextApp=TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hNextCateg);
			while(hNextApp!=NULL)
			{
				if (TVItemGetLParam(w,hNextApp)==lp)
				{
					TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNextApp);
					goto end;
				}
				hNextApp=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextApp);
			}
		}
		hNextCateg=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextCateg);
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// TogglePasswordField()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void TogglePasswordField(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szPwd[LEN_PWD+1];

	if (giLastApplicationConfig!=-1)
	{
		// 1.03 : loupe interdite sur mot de passe imposé
		// 1.05 : sauf pour l'admin !
		if ((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_PWD) && !gbInternetManualPutConfig) goto end; 
	}

	if (!gbShowPwd) // 0.96
	{
		if (AskPwd(w,FALSE)!=0) goto end;
	}
	gbShowPwd=!gbShowPwd;
	ShowWindow(GetDlgItem(w,TB_PWD),gbShowPwd?SW_HIDE:SW_SHOW);
	ShowWindow(GetDlgItem(w,TB_PWD_CLEAR),gbShowPwd?SW_SHOW:SW_HIDE);
	GetDlgItemText(w,gbShowPwd?TB_PWD:TB_PWD_CLEAR,szPwd,sizeof(szPwd));
	SetDlgItemText(w,gbShowPwd?TB_PWD_CLEAR:TB_PWD,szPwd);
	SecureZeroMemory(szPwd,sizeof(szPwd));

	// un ptit coup de refresh sur les contrôles
	RECT rect;
	GetClientRect(GetDlgItem(w,TB_PWD),&rect);
	InvalidateRect(GetDlgItem(w,TB_PWD),&rect,FALSE);
	GetClientRect(GetDlgItem(w,TB_PWD_CLEAR),&rect);
	InvalidateRect(GetDlgItem(w,TB_PWD_CLEAR),&rect,FALSE);
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// GenerateApplicationName()
// ----------------------------------------------------------------------------------
// Génère un nom pour cette config (vérifie si existe, ajoute un id unique)
// ----------------------------------------------------------------------------------
void GenerateApplicationName(int iAction,char *pszProposition)
{
	TRACE((TRACE_ENTER,_F_, ""));

	char szApplication[LEN_APPLICATION_NAME+1];
	int i;
	BOOL bUniqueNameFound=FALSE;
	int iUniqueId=0;

	strcpy_s(szApplication,sizeof(szApplication),pszProposition);
	// parcourt la liste pour voir si ce titre existe déjà. Si oui, ajoute (1) et rééssaie en incrémentant à chaque fois
	while (!bUniqueNameFound)
	{
		bUniqueNameFound=TRUE;
		TRACE((TRACE_DEBUG,_F_,"Looking for %s AppName unicity",szApplication));
		for (i=0;i<giNbActions;i++)
		{
			//TRACE((TRACE_DEBUG,_F_,"Comparing with : %s (%d)",gptActions[i].szApplication,i));
			if (i!=iAction && _stricmp(szApplication,gptActions[i].szApplication)==0) 
			{
				TRACE((TRACE_INFO,_F_,"AppName %s already exists",szApplication));
				bUniqueNameFound=FALSE;
				iUniqueId++;
				pszProposition[LEN_APPLICATION_NAME-7]=0; // laisse de la place pour id unique ! (bug #116)
				wsprintf(szApplication,"%s (%d)",pszProposition,iUniqueId);
				goto suite;
			}
		}
suite:;
	}
	strcpy_s(gptActions[iAction].szApplication,sizeof(gptActions[iAction].szApplication),szApplication);
	TRACE((TRACE_LEAVE,_F_, "out:%s",gptActions[iAction].szApplication));
}

// ----------------------------------------------------------------------------------
// GenerateCategoryName()
// ----------------------------------------------------------------------------------
// Génère un nom pour nouvelle catégorie (vérifie si existe, ajoute un id unique)
// ----------------------------------------------------------------------------------
void GenerateCategoryName(int iCategory,char *pszProposition)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	BOOL bUniqueNameFound=FALSE;
	int iUniqueId=0;
	char szCategory[LEN_CATEGORY_LABEL+1];

	strcpy_s(szCategory,sizeof(szCategory),pszProposition);
	// parcourt la liste pour voir si ce titre existe déjà. Si oui, ajoute (1) et rééssaie en incrémentant à chaque fois
	while (!bUniqueNameFound)
	{
		bUniqueNameFound=TRUE;
		TRACE((TRACE_DEBUG,_F_,"Looking for %s CategName unicity",szCategory));
		for (i=0;i<giNbCategories;i++)
		{
			if (_stricmp(szCategory,gptCategories[i].szLabel)==0) 
			{
				TRACE((TRACE_INFO,_F_,"CategName %s already exists",szCategory));
				bUniqueNameFound=FALSE;
				iUniqueId++;
				wsprintf(szCategory,"%s (%d)",pszProposition,iUniqueId);
				goto suite;
			}
		}
suite:;
	}
	strcpy_s(gptCategories[iCategory].szLabel,sizeof(gptCategories[iCategory].szLabel),szCategory);
	TRACE((TRACE_LEAVE,_F_, "out:%s",gptCategories[iCategory].szLabel));
}

// ----------------------------------------------------------------------------------
// GenerateDomainName()
// ----------------------------------------------------------------------------------
// Génère un nom pour ce domaine (vérifie si existe, ajoute un id unique)
// ----------------------------------------------------------------------------------
void GenerateDomainName(int iDomain,char *pszProposition)
{
	TRACE((TRACE_ENTER,_F_, ""));

	char szDomain[LEN_DOMAIN+1];
	int i;
	BOOL bUniqueNameFound=FALSE;
	int iUniqueId=0;

	strcpy_s(szDomain,sizeof(szDomain),pszProposition);
	// parcourt la liste pour voir si ce titre existe déjà. Si oui, ajoute (1) et rééssaie en incrémentant à chaque fois
	while (!bUniqueNameFound)
	{
		bUniqueNameFound=TRUE;
		TRACE((TRACE_DEBUG,_F_,"Looking for %s DomainLabel unicity",szDomain));
		for (i=0;i<giNbDomains;i++)
		{
			//TRACE((TRACE_DEBUG,_F_,"Comparing with : %s (%d)",gptActions[i].szApplication,i));
			if (i!=iDomain && _stricmp(szDomain,gtabDomains[i].szDomainLabel)==0) 
			{
				TRACE((TRACE_INFO,_F_,"DomainLabel %s already exists",szDomain));
				bUniqueNameFound=FALSE;
				iUniqueId++;
				pszProposition[LEN_DOMAIN-7]=0; // laisse de la place pour id unique ! (bug #116)
				wsprintf(szDomain,"%s (%d)",pszProposition,iUniqueId);
				goto suite;
			}
		}
suite:;
	}
	strcpy_s(gtabDomains[iDomain].szDomainLabel,sizeof(gtabDomains[iDomain].szDomainLabel),szDomain);
	TRACE((TRACE_LEAVE,_F_, "out:%s",gtabDomains[iDomain].szDomainLabel));
}

// ----------------------------------------------------------------------------------
// IsDomainNameUnique()
// ----------------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------------
BOOL IsDomainNameUnique(int iDomain,char *pszDomainLabel)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	int rc=TRUE;
	for (i=0;i<giNbDomains;i++)
	{
		if (i==iDomain) continue;
		if (_stricmp(pszDomainLabel,gtabDomains[i].szDomainLabel)==0) 
		{
			rc=FALSE;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// IsCategoryNameUnique()
// ----------------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------------
BOOL IsCategoryNameUnique(int iCategory,char *pszCategory)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	int rc=TRUE;
	for (i=0;i<giNbCategories;i++)
	{
		if (i==iCategory) continue;
		if (_stricmp(pszCategory,gptCategories[i].szLabel)==0) 
		{
			rc=FALSE;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// IsApplicationNameUnique()
// ----------------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------------
BOOL IsApplicationNameUnique(int iAction,char *pszApplication)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	int rc=TRUE;
	
	if (_stricmp(pszApplication,"swsso")==0) { rc=FALSE; goto end; } // 0.88
	if (_stricmp(pszApplication,"swsso-categories")==0) { rc=FALSE; goto end; } // 0.88

	for (i=0;i<giNbActions;i++)
	{
		if (i==iAction) continue;
		if (_stricmp(pszApplication,gptActions[i].szApplication)==0) 
		{
			rc=FALSE;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
//-----------------------------------------------------------------------------
// NewApplication()
//-----------------------------------------------------------------------------
// Ajout d'une nouvelle application (suite menu clic-droit ajouter)
//-----------------------------------------------------------------------------
int NewApplication(HWND w,char *szAppName,BOOL bActive)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HTREEITEM hParentItem,hNewItem,hSelectedItem;

	// ISSUE#149
	if (giNbActions>=giMaxConfigs) { MessageBox(NULL,GetString(IDS_MSG_MAX_CONFIGS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }

	ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));
	//gptActions[giNbActions].wLastDetect=NULL;
	//gptActions[giNbActions].tLastDetect=-1;
	gptActions[giNbActions].tLastSSO=-1;
	gptActions[giNbActions].wLastSSO=NULL;
	gptActions[giNbActions].iWaitFor=WAIT_IF_SSO_OK;
	gptActions[giNbActions].bActive=bActive; // 0.93B6 (avant c'était FALSE)
	gptActions[giNbActions].bAutoLock=TRUE;
	// gptActions[giNbActions].bConfigOK=FALSE; // 0.90B1 : on ne gère plus l'état OK car plus de remontée auto
	gptActions[giNbActions].bConfigSent=FALSE;
	// gptActions[giNbActions].iDomainId=1;
	gptActions[giNbActions].iPwdGroup=-1;

	// si le clic-droit est fait sur une catégorie, on ajoute
	// l'application dans la catégorie sélectionnée, sinon dans non classé
	// récupère catégorie de l'appli sélectionnée ou catégorie sélectionnée
	hSelectedItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	TRACE((TRACE_DEBUG,_F_,"hSelectedItem=0x%08lx",hSelectedItem));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hSelectedItem);
	TRACE((TRACE_DEBUG,_F_,"hParentItem=0x%08lx",hParentItem));
	if (hParentItem==NULL) // c'est une catégorie
		gptActions[giNbActions].iCategoryId=TVItemGetLParam(w,hSelectedItem);
	else // c'est une application
		gptActions[giNbActions].iCategoryId=TVItemGetLParam(w,hParentItem);
	// génère le nom de l'application
	GenerateApplicationName(giNbActions,szAppName);
	
	hNewItem=TVAddApplication(w,giNbActions,NULL,FALSE);
	if (hNewItem!=NULL) 
	{
		TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNewItem);
		TreeView_EditLabel(GetDlgItem(w,TV_APPLICATIONS),hNewItem);
	}
	ShowApplicationDetails(w,giNbActions);
	giNbActions++;
	//ClearApplicationDetails(w);
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// NewCategory()
//-----------------------------------------------------------------------------
// Ajout d'une nouvelle catégorie (suite menu clic-droit ajouter)
//-----------------------------------------------------------------------------
int NewCategory(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	HTREEITEM hNewItem;
	ZeroMemory(&gptCategories[giNbCategories],sizeof(T_CATEGORY));
	gptCategories[giNbCategories].id=giNextCategId;
	GenerateCategoryName(giNbCategories,GetString(IDS_NEW_CATEG));
	hNewItem=TVAddCategory(w,giNbCategories);
	if (hNewItem!=NULL) 
	{
		TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNewItem);
		TreeView_EditLabel(GetDlgItem(w,TV_APPLICATIONS),hNewItem);
	}
	giNbCategories++;
	giNextCategId++;

	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// TVGetNextOrPreviousItem()
//-----------------------------------------------------------------------------
// Appelé en cas de suppression pour déterminer l'item à sélectionner
// - Si l'item passé en paramètre est une catégorie, retourne dans l'ordre
//   1) La catégorie suivante
//   2) La catégorie précédente
//   3) NULL
// - Si l'item est une application, retourne dans l'ordre
//	 1) L'application suivante de même catégorie
//   2) L'application précédente de même catégorie
//   3) La catégorie à laquelle appartient l'application
//-----------------------------------------------------------------------------
HTREEITEM TVGetNextOrPreviousItem(HWND w,HTREEITEM hItem)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HTREEITEM hReturnedItem=NULL;
	HTREEITEM hParentItem;
	HTREEITEM hNextAppItem, hPrevAppItem;
	HTREEITEM hNextCategItem, hPrevCategItem;

	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie 
	{
		// 1) La catégorie suivante
		hNextCategItem=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hItem);
		if (hNextCategItem!=NULL) 
		{
			hReturnedItem=hNextCategItem;
			goto end;
		}
		// 2) La catégorie précédente
		hPrevCategItem=TreeView_GetPrevSibling(GetDlgItem(w,TV_APPLICATIONS),hItem);
		if (hPrevCategItem!=NULL) 
		{
			hReturnedItem=hPrevCategItem;
			goto end;
		}
		hReturnedItem=NULL;
	}
	else
	{
		// 1) L'application suivante de même catégorie
		hNextAppItem=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hItem);
		if (hNextAppItem!=NULL) 
		{
			hReturnedItem=hNextAppItem;
			goto end;
		}
		// 2) L'application précédente de même catégorie
		hPrevAppItem=TreeView_GetPrevSibling(GetDlgItem(w,TV_APPLICATIONS),hItem);
		if (hPrevAppItem!=NULL) 
		{
			hReturnedItem=hPrevAppItem;
			goto end;
		}
		// 3) La catégorie à laquelle appartient l'application
		hReturnedItem=hParentItem;
	}
end:
	TRACE((TRACE_LEAVE,_F_, "hReturnedItem=0x%08lx",hReturnedItem));
	return hReturnedItem;
}

//-----------------------------------------------------------------------------
// CategIdsCheckBoxEvents()
//-----------------------------------------------------------------------------
// Gestion de l'événement clic sur les check box de le fenêtre ChangeCategIds
//-----------------------------------------------------------------------------

void CategIdsCheckBoxEvents(HWND w,int iCheckBox,int iTextBox)
{
	EnableWindow(GetDlgItem(w,iTextBox),IsDlgButtonChecked(w,iCheckBox)==BST_CHECKED?TRUE:FALSE);
	EnableWindow(GetDlgItem(w,IDOK),(IsDlgButtonChecked(w,CK_ID1)==BST_CHECKED ||
									 IsDlgButtonChecked(w,CK_ID2)==BST_CHECKED ||
									 IsDlgButtonChecked(w,CK_ID3)==BST_CHECKED ||
									 IsDlgButtonChecked(w,CK_ID4)==BST_CHECKED ||
									 IsDlgButtonChecked(w,CK_PWD)==BST_CHECKED));
	SetFocus(GetDlgItem(w,iTextBox));
}
//-----------------------------------------------------------------------------
// ChangeCategIdsDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de saisie des identifiants / mot de passe à modifier
//-----------------------------------------------------------------------------
static int CALLBACK ChangeCategIdsDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// limitation des champs de saisie
			SendMessage(GetDlgItem(w,TB_ID1),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID2),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID3),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_ID4),EM_LIMITTEXT,LEN_ID,0);
			SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			// TODO: positionnement 
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
					char *pszEncryptedPassword=NULL;
					char szMsg[250];
					wsprintf(szMsg,GetString(IDS_CHANGER_IDS),gIds.iNbModified);
					if (MessageBox(w,szMsg,"swSSO",MB_YESNO | MB_ICONQUESTION)==IDYES)
					{
						gIds.bId1Modified=(IsDlgButtonChecked(w,CK_ID1)==BST_CHECKED);
						gIds.bId2Modified=(IsDlgButtonChecked(w,CK_ID2)==BST_CHECKED);
						gIds.bId3Modified=(IsDlgButtonChecked(w,CK_ID3)==BST_CHECKED);
						gIds.bId4Modified=(IsDlgButtonChecked(w,CK_ID4)==BST_CHECKED);
						gIds.bPwdModified=(IsDlgButtonChecked(w,CK_PWD)==BST_CHECKED);

						GetDlgItemText(w,TB_ID1,gIds.szId1Value,sizeof(gIds.szId1Value));
						GetDlgItemText(w,TB_ID2,gIds.szId2Value,sizeof(gIds.szId2Value));
						GetDlgItemText(w,TB_ID3,gIds.szId3Value,sizeof(gIds.szId3Value));
						GetDlgItemText(w,TB_ID4,gIds.szId4Value,sizeof(gIds.szId4Value));
						GetDlgItemText(w,TB_PWD,gIds.szPwdClearValue,sizeof(gIds.szPwdClearValue));
						if (*(gIds.szPwdClearValue)!=0) // TODO -> CODE A REVOIR PLUS TARD (PAS BEAU SUITE A ISSUE#83)
						{
							pszEncryptedPassword=swCryptEncryptString(gIds.szPwdClearValue,ghKey1);
							SecureZeroMemory(gIds.szPwdClearValue,sizeof(gIds.szPwdClearValue));
							if (pszEncryptedPassword!=NULL)
							{
								strcpy_s(gIds.szPwdEncryptedValue,sizeof(gIds.szPwdEncryptedValue),pszEncryptedPassword);
								free(pszEncryptedPassword); 
							}
							else 
								*(gIds.szPwdEncryptedValue)=0;
						}
						else
						{
							strcpy_s(gIds.szPwdEncryptedValue,sizeof(gIds.szPwdEncryptedValue),gIds.szPwdClearValue);
						}
						EndDialog(w,IDOK);
					}
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case CK_ID1:
					CategIdsCheckBoxEvents(w,CK_ID1,TB_ID1);
					break;
				case CK_ID2:
					CategIdsCheckBoxEvents(w,CK_ID2,TB_ID2);
					break;
				case CK_ID3:
					CategIdsCheckBoxEvents(w,CK_ID3,TB_ID3);
					break;
				case CK_ID4:
					CategIdsCheckBoxEvents(w,CK_ID4,TB_ID4);
					break;
				case CK_PWD:
					CategIdsCheckBoxEvents(w,CK_PWD,TB_PWD);
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
// LaunchSelectedApp()
//-----------------------------------------------------------------------------
// Lance l'application sélectionnée en utilisant la commande fournie dans le 
// champ szFullPathName. Si non présente et que c'est une appli web, on utilise
// directement l'URL (en enlevant * à la fin si présente !)
//-----------------------------------------------------------------------------
void LaunchSelectedApp(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HTREEITEM hItem,hParentItem;
	int iAction;
	char szCmd[LEN_FULLPATHNAME+1];
	char *pszParams;
	int len;
	HCURSOR hCursorOld=NULL;
	int rc;
	
	hCursorOld=SetCursor(ghCursorWait);
	
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) goto end; // c'est une catégorie 

	iAction=TVItemGetLParam(w,hItem); 
	if (iAction==-1) goto end;

	if ((gptActions[iAction].iType==WEBSSO || gptActions[iAction].iType==XEBSSO) && *gptActions[iAction].szFullPathName==0)
	{
		strcpy_s(szCmd,sizeof(szCmd),gptActions[iAction].szURL);
		len=strlen(szCmd);
		if (szCmd[len-1]=='*') szCmd[len-1]=0;
	}
	else
	{
		strcpy_s(szCmd,sizeof(szCmd),gptActions[iAction].szFullPathName);
	}	
	if (*szCmd==0) { MessageBox(w,GetString(IDS_NO_LAUNCH_PATH),"swSSO",MB_ICONEXCLAMATION); goto end; }

	// si la commande contient des paramètres, il faut les isoler pour passer la commande d'un côté et les param de l'autre
	// la recherche d'un espace n'est pas une bonne solution (il y a un espace dans program files, par exemple !!)
	// du coup l'idée est de chercher le .exe, .bat, .cmd ou .com : tout ce qui se trouve après est un (ou +) paramètres
	pszParams=strnistr(szCmd,".exe ",-1);
	if (pszParams==NULL) pszParams=strnistr(szCmd,".bat ",-1);
	if (pszParams==NULL) pszParams=strnistr(szCmd,".cmd ",-1);
	if (pszParams==NULL) pszParams=strnistr(szCmd,".com ",-1);
	
	if (pszParams!=NULL) { pszParams+=4; *pszParams=0; pszParams++; }
	TRACE((TRACE_DEBUG,_F_,"szCmd    =%s",szCmd));
	TRACE((TRACE_DEBUG,_F_,"pszParams=%s",pszParams==NULL?"null":pszParams));

	rc=(int)ShellExecute(NULL,"open",szCmd,pszParams,"",SW_SHOW);
	if (rc<=32) 
	{
		wsprintf(buf2048,GetString(IDS_LAUNCH_APP_ERROR),rc,szCmd,pszParams==NULL?"":pszParams);
		MessageBox(w,buf2048,"swSSO",MB_OK | MB_ICONSTOP); 
	}

	// mémorise l'action lancée pour proposer tout de suite le bon compte à l'utilisateur en cas
	// de multi-comptes !
	giLaunchedApp=iAction;
	TRACE((TRACE_DEBUG,_F_,"giLaunchedApp=%d",giLaunchedApp));

	// RAZ les paramètres d'attente de SSO pour cette application pour que le SSO se fasse dans la foulée 
	// même si la config avait été mise en attente suite à une connexion réussie précédemment
	//gptActions[iAction].tLastDetect=-1;
	//gptActions[iAction].wLastDetect=NULL;
	gptActions[iAction].tLastSSO=-1;
	gptActions[iAction].wLastSSO=NULL;
	gptActions[iAction].iWaitFor=WAIT_IF_SSO_OK;

end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, ""));
}
//-----------------------------------------------------------------------------
// ChangeCategIds()
//-----------------------------------------------------------------------------
// Fonction appelée lorsque l'utilisateur sélectionne le menu "changer identifiant
// et mot de passe" dans le menu clic-droit d'une catégorie.
// Affiche une fenêtre de saisie des identifiants et du mot de passe puis effectue
// la modification (en mémoire -> l'utilisateur doit valider par OK ou appliquer)
//-----------------------------------------------------------------------------
int ChangeCategIds(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HTREEITEM hItem;
	int iCategoryId,iCategoryIndex,i;
	
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	if (hItem==NULL) goto end;
	iCategoryId=TVItemGetLParam(w,hItem);
	if (iCategoryId==-1) goto end;
	iCategoryIndex=GetCategoryIndex(iCategoryId);
	if (iCategoryIndex==-1) goto end;

	TRACE((TRACE_INFO,_F_,"Modification ids et pwd de la catégorie %d (%s)",iCategoryId,gptCategories[iCategoryIndex].szLabel));

	memset(&gIds,0,sizeof(T_IDSNPWD));
	for (i=0;i<giNbActions;i++)
	{
		if (gptActions[i].iCategoryId==iCategoryId) gIds.iNbModified++;
	}

	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_CHANGE_IDS),w,ChangeCategIdsDialogProc)!=IDOK) goto end;
	
	TRACE((TRACE_DEBUG,_F_,"gIds.szId1Value=%s",gIds.szId1Value));
	TRACE((TRACE_DEBUG,_F_,"gIds.szId2Value=%s",gIds.szId2Value));
	TRACE((TRACE_DEBUG,_F_,"gIds.szId3Value=%s",gIds.szId3Value));
	TRACE((TRACE_DEBUG,_F_,"gIds.szId4Value=%s",gIds.szId4Value));
	
	for (i=0;i<giNbActions;i++)
	{
		if (gptActions[i].iCategoryId==iCategoryId)
		{
			TRACE((TRACE_DEBUG,_F_,"Modification action %d (%s)",i,gptActions[i].szApplication));
			if (gIds.bId1Modified) strcpy_s(gptActions[i].szId1Value,sizeof(gptActions[i].szId1Value),gIds.szId1Value);
			if (gIds.bId2Modified) strcpy_s(gptActions[i].szId2Value,sizeof(gptActions[i].szId2Value),gIds.szId2Value);
			if (gIds.bId3Modified) strcpy_s(gptActions[i].szId3Value,sizeof(gptActions[i].szId3Value),gIds.szId3Value);
			if (gIds.bId4Modified) strcpy_s(gptActions[i].szId4Value,sizeof(gptActions[i].szId4Value),gIds.szId4Value);
			if (gIds.bPwdModified) strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),gIds.szPwdEncryptedValue);
		}
	}
	TRACE((TRACE_INFO,_F_,"%d configs modifiées",gIds.iNbModified));
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// TVRemoveSelectedAppOrCateg()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int TVRemoveSelectedAppOrCateg(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	// char szMsg[250];
	//char szCategoryId[8+1];
	HTREEITEM hItem,hParentItem,hNextCateg,hNextApp,hNewSelectedItem;
	int iAction,iCategoryId,iCategory;
	int i;

	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie 
	{
		iCategoryId=TVItemGetLParam(w,hItem);
		if (iCategoryId==-1) goto end;
		if (iCategoryId==0)
		{
			MessageBox(w,GetString(IDS_CATEG_NOT_REMOVABLE),"swSSO",MB_ICONEXCLAMATION);
			goto end;
		}
		TRACE((TRACE_INFO,_F_,"SUPPRESSION iCategoryId=%d",iCategoryId));
		iCategory=GetCategoryIndex(iCategoryId);
		if (iCategory==-1) goto end;
		if (TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hItem)!=NULL)
		{
			MessageBox(w,GetString(IDS_CATEG_NOT_EMPTY),"swSSO",MB_ICONEXCLAMATION);
			goto end;
		}
		// ISSUE#223 : si gbAdminDeleteConfigsOnServer alors supprime aussi sur le serveur (avec confirmation)
		if (gbAdminDeleteConfigsOnServer && iCategoryId>=10000)
		{
			char szMsg[500];
			HCURSOR hCursorOld=NULL;
			hCursorOld=SetCursor(ghCursorWait);
			wsprintf(szMsg,GetString(IDS_DELETE_CATEG),gptCategories[iCategory].szLabel);
			if (MessageBox(w,szMsg,"swSSO",MB_YESNOCANCEL | MB_ICONQUESTION)!=IDYES) goto end;
			if (DeleteCategOnServer(iCategory)==0)
			{
				MessageBox(w,GetString(IDS_DELETE_CATEG_ON_SERVER_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				MessageBox(w,GetString(IDS_DELETE_CATEG_ON_SERVER_KO),"swSSO",MB_OK | MB_ICONEXCLAMATION);
				goto end;
			}
			if (hCursorOld!=NULL) SetCursor(hCursorOld);
		}
		// effacement dans le fichier : ne me semble plus utile depuis que le fichier est réécrit 
		// complètement à chaque sauvegarde => supprimé en 0.90B1
		// wsprintf(szCategoryId,"%d",iCategoryId);
		// WritePrivateProfileString("swSSO-Categories",szCategoryId,NULL,gszCfgFile);
		// effacement dans la table : en fait, décalage de toutes les actions > à celle à effacer
		for (i=iCategory;i<giNbCategories-1;i++)
		{
			TRACE((TRACE_DEBUG,_F_,"Copie %d sur %d",i+1,i));
			memcpy(&gptCategories[i],&gptCategories[i+1],sizeof(T_CATEGORY));
		}
		// avant d'effacer l'élément dans la liste, on définit quel item sera sélectionné après la suppression
		hNewSelectedItem=TVGetNextOrPreviousItem(w,hItem);
		// effacement dans la liste
		// ASTUCE : flag anti-actualisation de la liste sur notification
		//          de modification de l'élement qu'on supprime.
		gbEffacementEnCours=TRUE;
		TreeView_DeleteItem(GetDlgItem(w,TV_APPLICATIONS),hItem);
		giNbCategories--;
		TRACE((TRACE_DEBUG,_F_,"Nouveau giNbCategories=%d",giNbCategories));
		gbEffacementEnCours=FALSE;
		// efface la partie droite (détail) 
		if (hNewSelectedItem!=NULL) TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNewSelectedItem);
		ClearApplicationDetails(w);
		// ISSUE#223 suite : après effacement de la categ sur le serveur, on sauvegarde localement
		if (gbAdminDeleteConfigsOnServer && iCategoryId>=10000)
		{
			SaveApplications();
			BackupAppsNcategs();
			EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),FALSE);
		}
	}
	else // c'est une application
	{
		iAction=TVItemGetLParam(w,hItem); 
		if (iAction==-1) goto end;
		// 0.91 : interdiction de supprimer les configurations managées
		if (gptActions[iAction].iConfigId!=0 && !gbAllowManagedConfigsDeletion && !gptActions[iAction].bAddAccount)
		{
			MessageBox(w,GetString(IDS_DELETION_FORBIDDEN),"swSSO",MB_ICONEXCLAMATION);
			goto end;
		}
		// ISSUE#223 : si gbAdminDeleteConfigsOnServer alors supprime aussi sur le serveur (avec confirmation)
		if (gbAdminDeleteConfigsOnServer && gptActions[iAction].iConfigId!=0)
		{
			char szMsg[500];
			HCURSOR hCursorOld=NULL;
			hCursorOld=SetCursor(ghCursorWait);
			wsprintf(szMsg,GetString(IDS_DELETE),gptActions[iAction].szApplication);
			if (MessageBox(w,szMsg,"swSSO",MB_YESNOCANCEL | MB_ICONQUESTION)!=IDYES) goto end;
			if (DeleteConfigOnServer(iAction)==0)
			{
				MessageBox(w,GetString(IDS_DELETE_CONFIG_ON_SERVER_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				MessageBox(w,GetString(IDS_DELETE_CONFIG_ON_SERVER_KO),"swSSO",MB_OK | MB_ICONEXCLAMATION);
				goto end;
			}
			if (hCursorOld!=NULL) SetCursor(hCursorOld);
		}
		// ISSUE#159 : on ne demande plus de confirmation puisque la suppression est annulable
		// wsprintf(szMsg,GetString(IDS_DELETE),gptActions[iAction].szApplication);
		// if (MessageBox(w,szMsg,"swSSO",MB_YESNO | MB_ICONQUESTION)==IDNO) goto end;
		TRACE((TRACE_INFO,_F_,"SUPPRESSION application %ld",iAction));
		// effacement dans le fichier : ne me semble plus utile depuis que le fichier est réécrit 
		// complètement à chaque sauvegarde => supprimé en 0.90B1
		// WritePrivateProfileString(gptActions[iAction].szApplication,NULL,NULL,gszCfgFile);
		// effacement dans la table : en fait, décalage de toutes les actions > à celle à effacer
		for (i=iAction;i<giNbActions-1;i++)
		{
			TRACE((TRACE_DEBUG,_F_,"Copie %d sur %d",i+1,i));
			memcpy(&gptActions[i],&gptActions[i+1],sizeof(T_ACTION));
		}
		// avant d'effacer l'élément dans la liste, on définit quel item sera sélectionné après la suppression
		hNewSelectedItem=TVGetNextOrPreviousItem(w,hItem);
		// effacement dans la liste
		// ASTUCE : flag anti-actualisation de la liste sur notification
		//          de modification de l'élement qu'on supprime.
		gbEffacementEnCours=TRUE;
		TreeView_DeleteItem(GetDlgItem(w,TV_APPLICATIONS),hItem);
		giNbActions--;
		TRACE((TRACE_DEBUG,_F_,"Nouveau giNbActions=%d",giNbActions));
		gbEffacementEnCours=FALSE;
		// attention, il faut maintenant redécaler tous les lParam des items
		// qui se trouvaient après l'item supprimé, c'est à dire qui ont un 
		// lParam > à celui de l'item supprimé.
		hNextCateg=TreeView_GetRoot(GetDlgItem(w,TV_APPLICATIONS));
		while (hNextCateg!=NULL)
		{
			TRACE((TRACE_DEBUG,_F_,"CATEG"));
			hNextApp=TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hNextCateg);
			while(hNextApp!=NULL)
			{
				LPARAM lp=TVItemGetLParam(w,hNextApp);
				TRACE((TRACE_DEBUG,_F_," +APP(lp=%d)",lp));
				if (lp>=iAction) TVItemSetLParam(w,hNextApp,lp-1);
				hNextApp=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextApp);
			}
			hNextCateg=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextCateg);
		}
		// réaffiche la partie droite (détail) correspondant au nouvel élément sélectionné
		hParentItem=NULL;
		if (hNewSelectedItem!=NULL) hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hNewSelectedItem);
		if (giNbActions<=0 || hNewSelectedItem==NULL || hParentItem==NULL) 
		{
			ClearApplicationDetails(w);
		}
		else
		{
			TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNewSelectedItem);
			ShowApplicationDetails(w,TVItemGetLParam(w,hNewSelectedItem));
		}
		// Au final c'est bien compliqué cette suppression... Je me demande
		// s'il n'y avait pas plus simple... Qu'en pensez-vous ?
		// 0.92B3 effacement mémoire pour éviter l'affichage des champs dans la fenêtre ajouter cette application
		ZeroMemory(&gptActions[giNbActions],sizeof(T_ACTION));

		// ISSUE#223 suite : après effacement de la config sur le serveur, on sauvegarde localement
		if (gbAdminDeleteConfigsOnServer && gptActions[iAction].iConfigId!=0)
		{
			SaveApplications();
			BackupAppsNcategs();
			EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),FALSE);
		}
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}					

//-----------------------------------------------------------------------------
// TVDuplicateSelectedApp()
//-----------------------------------------------------------------------------
// Duplication de l'application sélectionnée
// bKeepId : TRUE dans le cas d'un ajout de compte, FALSE sinon
//-----------------------------------------------------------------------------
int TVDuplicateSelectedApp(HWND w,BOOL bKeepId)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	HTREEITEM hItem;
	char szAppName[LEN_APPLICATION_NAME+1];
	int iAction;

	// récupère le nom de l'application à dupliquer
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	TVItemGetText(w,hItem,szAppName,sizeof(szAppName));
	iAction=TVItemGetLParam(w,hItem);
	TRACE((TRACE_INFO,_F_,"Duplication application %s (%d)",szAppName,iAction));
		
	// création de la nouvelle application
	NewApplication(w,szAppName,TRUE);
	// 0.93B6
	TRACE((TRACE_INFO,_F_,"Nouvelle application créée %s (%d)",gptActions[giNbActions-1].szApplication,giNbActions-1));

	// recopie des champs de l'application source
	strcpy_s(gptActions[giNbActions-1].szTitle   ,sizeof(gptActions[giNbActions-1].szTitle)   ,gptActions[iAction].szTitle);
	strcpy_s(gptActions[giNbActions-1].szURL     ,sizeof(gptActions[giNbActions-1].szURL)     ,gptActions[iAction].szURL);
	strcpy_s(gptActions[giNbActions-1].szId1Name ,sizeof(gptActions[giNbActions-1].szId1Name) ,gptActions[iAction].szId1Name);
	strcpy_s(gptActions[giNbActions-1].szId2Name ,sizeof(gptActions[giNbActions-1].szId2Name) ,gptActions[iAction].szId2Name);
	strcpy_s(gptActions[giNbActions-1].szId3Name ,sizeof(gptActions[giNbActions-1].szId3Name) ,gptActions[iAction].szId3Name);
	strcpy_s(gptActions[giNbActions-1].szId4Name ,sizeof(gptActions[giNbActions-1].szId4Name) ,gptActions[iAction].szId4Name);
	strcpy_s(gptActions[giNbActions-1].szPwdName ,sizeof(gptActions[giNbActions-1].szPwdName) ,gptActions[iAction].szPwdName);
	if (!bKeepId) // l'utilisateur a choisi "ajouter un compte" -> dans ce cas on efface les informations de compte
	{
		strcpy_s(gptActions[giNbActions-1].szId1Value,sizeof(gptActions[giNbActions-1].szId1Value),gptActions[iAction].szId1Value);
		strcpy_s(gptActions[giNbActions-1].szId2Value,sizeof(gptActions[giNbActions-1].szId2Value),gptActions[iAction].szId2Value);
		strcpy_s(gptActions[giNbActions-1].szId3Value,sizeof(gptActions[giNbActions-1].szId3Value),gptActions[iAction].szId3Value);
		strcpy_s(gptActions[giNbActions-1].szId4Value,sizeof(gptActions[giNbActions-1].szId4Value),gptActions[iAction].szId4Value);
		strcpy_s(gptActions[giNbActions-1].szPwdEncryptedValue,sizeof(gptActions[giNbActions-1].szPwdEncryptedValue),gptActions[iAction].szPwdEncryptedValue);
	}
	strcpy_s(gptActions[giNbActions-1].szValidateName     ,sizeof(gptActions[giNbActions-1].szValidateName)     ,gptActions[iAction].szValidateName);
	gptActions[giNbActions-1].iType	 =gptActions[iAction].iType;
	gptActions[giNbActions-1].id2Type=gptActions[iAction].id2Type;
	gptActions[giNbActions-1].id3Type=gptActions[iAction].id3Type;
	gptActions[giNbActions-1].id4Type=gptActions[iAction].id4Type;
	gptActions[giNbActions-1].bKBSim =gptActions[iAction].bKBSim;
	strcpy_s(gptActions[giNbActions-1].szKBSim,sizeof(gptActions[giNbActions-1].szKBSim),gptActions[iAction].szKBSim);
	strcpy_s(gptActions[giNbActions-1].szFullPathName,sizeof(gptActions[giNbActions-1].szFullPathName),gptActions[iAction].szFullPathName);
	// ISSUE#24 : 0.92B8 : duplique aussi le config ID
	// ISSUE#52 : 0.93B5 : ne conserve le config id que si bKeepId=TRUE
	gptActions[giNbActions-1].iConfigId=bKeepId?gptActions[iAction].iConfigId:0;
	
	// ISSUE#57 : 0.93B7
	gptActions[giNbActions-1].iCategoryId=gptActions[iAction].iCategoryId;

	// ISSUE#96 : 0.97
	gptActions[giNbActions-1].bAddAccount=bKeepId;
	
	// 1.03
	gptActions[giNbActions-1].iWithIdPwd=bKeepId?0:gptActions[iAction].iWithIdPwd;
	gptActions[giNbActions-1].iPwdGroup=bKeepId?-1:gptActions[iAction].iPwdGroup;

	ShowApplicationDetails(w,giNbActions-1);
	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// TVUpdateItemState()
//-----------------------------------------------------------------------------
// Met à jour la bulle verte/rouge si changement d'état
//-----------------------------------------------------------------------------
void TVUpdateItemState(HWND w, HTREEITEM hItem,int iAction)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	TVITEMEX itemex;

	itemex.mask=TVIF_STATE;
	itemex.stateMask=TVIS_STATEIMAGEMASK;
	itemex.hItem=hItem;
	// voila le genre de truc complètement immaintenable dont je ne suis pas fier...
	// alors : il faut comprendre qu'on met la bulle :
	// - en rouge si l'action est désactivée
	// - en vert avec un ! si l'action n'est pas marquée comme envoyée au serveur ET 
	//   que l'utilisateur a autorisé la remontée des configs
	// - en vert tout cout si l'action est marquée comme envoyée au serveur OU
	//   que l'utilisateur n'a pas autorisé les remontées (ni manuelles ni automatiques)
	itemex.state=INDEXTOSTATEIMAGEMASK(gptActions[iAction].bActive?((gptActions[iAction].bConfigSent || !gbInternetManualPutConfig)?1:3):2);
	TreeView_SetItem(GetDlgItem(w,TV_APPLICATIONS),&itemex);

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// TVActivateAction()
//-----------------------------------------------------------------------------
// Active l'application dont le hItem est passé en paramètre
//-----------------------------------------------------------------------------
int TVActivateAction(HWND w, HTREEITEM hItem,int iActivate)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int iAction;
	
	iAction=TVItemGetLParam(w,hItem); 
	if (iAction==-1 || iAction>=giNbActions) goto end;

	TRACE((TRACE_INFO,_F_,"iAction=%d (%s) iActivate=%d",iAction,gptActions[iAction].szApplication,iActivate));

	switch(iActivate)
	{
		case ACTIVATE_YES:
			gptActions[iAction].bActive=TRUE;
			break;
		case ACTIVATE_NO:
			gptActions[iAction].bActive=FALSE;
			break;
		case ACTIVATE_TOGGLE:
			gptActions[iAction].bActive=!gptActions[iAction].bActive;
			break;
	}
	//gptActions[iAction].wLastDetect=NULL;
	//gptActions[iAction].tLastDetect=-1;
	gptActions[iAction].tLastSSO=-1;
	gptActions[iAction].wLastSSO=NULL;
	gptActions[iAction].iWaitFor=WAIT_IF_SSO_OK;
	TVUpdateItemState(w,hItem,iAction);

end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// TVActivateSelectedAppOrCateg()
//-----------------------------------------------------------------------------
// Active l'application sélectionnée ou les applications de la catégorie sélectionnée
//-----------------------------------------------------------------------------
int TVActivateSelectedAppOrCateg(HWND w, int iActivate)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HTREEITEM hItem,hParentItem,hNextApp;

	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie, il faut activer toutes ses applications
	{
		hNextApp=TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hItem);
		while(hNextApp!=NULL)
		{
			TVActivateAction(w,hNextApp,iActivate);
			hNextApp=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextApp);
		}
	}
	else // c'est une application
	{
		TVActivateAction(w,hItem,iActivate);
	}
	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// HowManyCollapsedCategories()
// ----------------------------------------------------------------------------------
static int HowManyCollapsedCategories(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;
	char *p;

	GetPrivateProfileString("swSSO","CollapsedCategs","",buf2048,sizeof(buf2048),gszCfgFile);
	p=strchr(buf2048,':');
	while (p!=NULL)
	{
		rc++;
		p=strchr(p+1,':');
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetNextCollapsedCategory()
// ----------------------------------------------------------------------------------
// Fournit la valeur suivante de computername pour le bouton "J'ai de la chance"
// ----------------------------------------------------------------------------------
char *GetNextCollapsedCategory(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pRet=NULL;

	// pour l'énumération des computernames...
	if (gpNextCollapsedCategory!=NULL) // c'est bon, on en a encore un en stock
	{
		pRet=gpNextCollapsedCategory;
		gpNextCollapsedCategory=strtok_s(NULL,":",&gpCollapsedCategoryContext);
	}
	else // on n'a pas encore énuméré ou alors on a épuisé le stock
	{
		GetPrivateProfileString("swSSO","CollapsedCategs","",gszEnumCollapsedCategories,sizeof(gszEnumCollapsedCategories),gszCfgFile);
		gpNextCollapsedCategory=strtok_s(gszEnumCollapsedCategories,":",&gpCollapsedCategoryContext);
		pRet=gpNextCollapsedCategory;
		gpNextCollapsedCategory=strtok_s(NULL,":",&gpCollapsedCategoryContext);
	}

	if (pRet==NULL) { TRACE((TRACE_LEAVE,_F_, "pRet=NULL")); }
	else			{ TRACE((TRACE_LEAVE,_F_, "pRet=%s",pRet)); }
	return pRet;
}

//-----------------------------------------------------------------------------
// LoadCategories()
//-----------------------------------------------------------------------------
// Charge la liste des catégories du .ini, section [swSSO]
//-----------------------------------------------------------------------------
int LoadCategories(void)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int rc=0; // retourne toujours OK
	int i;
	DWORD dw;
	char *p;
	char szCategoryIds[8*NB_MAX_CATEGORIES+2];
	char *pszCategId;
	int iCategId,iCategIndex;
	int iNbCollapsedCategories;

	if (gptCategories!=NULL) { free(gptCategories); gptCategories=NULL; }
	gptCategories=(T_CATEGORY*)malloc(sizeof(T_CATEGORY)*NB_MAX_CATEGORIES);
	// énumère les id de categ présents dans la section swSSO-Categories
	dw=GetPrivateProfileString("swSSO-Categories",NULL,NULL,szCategoryIds,sizeof(szCategoryIds),gszCfgFile);
	if (dw==0) { rc=0; goto end; } // aucune catégorie, pas grave.
	// premier tour pour compter le nombre de sections
	p=szCategoryIds;
	giNbCategories=0;
	giNextCategId=1;
	i=0;
	while (*p!=0)
	{
		TRACE((TRACE_DEBUG,_F_,"Lecture categ '%s'",p));
		gptCategories[i].id=atoi(p);
		// ISSUE#59 if (gptCategories[i].id>=giNextCategId) giNextCategId=gptCategories[i].id+1;
		if (gptCategories[i].id>=giNextCategId && gptCategories[i].id<10000) giNextCategId=gptCategories[i].id+1;
		GetPrivateProfileString("swSSO-Categories",p,"",gptCategories[i].szLabel,sizeof(gptCategories[i].szLabel),gszCfgFile);
		TRACE((TRACE_DEBUG,_F_,"%d : %s",i,gptCategories[i].szLabel));
		gptCategories[i].bExpanded=TRUE;
		while(*p!=0) p++ ;
		i++;
		p++;
	}
	giNbCategories=i;
	// 0.92B4 : lit l'état collapsed / expanded dans "CollapsedCategs" dans section [swSSO]
	// 1.04 : optimisation (demande une seule fois HowManyCollapsedCategories)
	iNbCollapsedCategories=HowManyCollapsedCategories();
	for (i=0;i<iNbCollapsedCategories;i++)
	{
		// récup de la prochaine catégorie collapsée
		pszCategId=GetNextCollapsedCategory();
		// ISSUE#59 0.93B7 : plantage au lancement de swsso si catégories collapsées supprimées
		if (pszCategId!=NULL)
		{
			iCategId=atoi(pszCategId);
			iCategIndex=GetCategoryIndex(iCategId);
			if (iCategIndex==-1) // (ISSUE#59 0.93B7) la catégorie n'existe plus
			{
				TRACE((TRACE_ERROR,_F_,"GetCategoryIndex(%d)=-1",iCategId));
			}
			else gptCategories[iCategIndex].bExpanded=FALSE;
		}
	}

end:
	TRACE((TRACE_INFO,_F_,"giNbCategories=%d giNextCategId=%d",giNbCategories,giNextCategId));
	// ISSUE#59 : déplacé dans winmain pour ne pas l'exécuter si des catégories ont été récupérées depuis le serveur
	/*if (giNbCategories==0) // si aucune catégorie, crée la catégorie "non classé"
	{
		strcpy_s(gptCategories[0].szLabel,LEN_CATEGORY_LABEL+1,GetString(IDS_NON_CLASSE));
		gptCategories[0].id=0;
		gptCategories[0].bExpanded=TRUE;
		giNbCategories=1;
		giNextCategId=1;
		WritePrivateProfileString("swSSO-Categories","0",gptCategories[0].szLabel,gszCfgFile);
	}*/
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SaveCategories()
//-----------------------------------------------------------------------------
// Sauvegarde la liste des catégories du .ini, section [swSSO]
//-----------------------------------------------------------------------------
int SaveCategories(void)
{
	int rc=-1;
	int i=0;
	char szCategId[8+1]; 
	TRACE((TRACE_ENTER,_F_, "giNbCategories=%d",giNbCategories));

	// 0.90B1 : on commence par tout effacer
	WritePrivateProfileString("swSSO-Categories",NULL,NULL,gszCfgFile);

	for (i=0;i<giNbCategories;i++)
	{
		wsprintf(szCategId,"%d",gptCategories[i].id);
		TRACE((TRACE_INFO,_F_,"id=%s label=%s",szCategId,gptCategories[i].szLabel));
		WritePrivateProfileString("swSSO-Categories",szCategId,gptCategories[i].szLabel,gszCfgFile);
	}
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// TVShowContextMenu()
// ----------------------------------------------------------------------------------
// Affichage du menu contextuel sur clic-droit dans la treeview
// ----------------------------------------------------------------------------------
static void TVShowContextMenu(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=NULL;
	HMENU hSubMenuCateg=NULL;
	HMENU hSubMenuUpload=NULL;
	HMENU hSubMenuUploadIdPwd=NULL;
	HTREEITEM hItem,hParentItem;
	int iAction;
	int i;
	BOOL bAddSeparator=FALSE;

	hMenu=CreatePopupMenu(); if (hMenu==NULL) goto end;

	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	if (hItem==NULL) { TRACE((TRACE_INFO,_F_,"TreeView_GetSelection()->NULL"));goto end;}
	
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie 
	{
		if (gbShowMenu_EnableDisable && !gbAdmin)
		{
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_ACTIVER,GetString(IDS_MENU_ACTIVER));
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_DESACTIVER,GetString(IDS_MENU_DESACTIVER));
		}
		if (gbShowMenu_Rename) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_RENOMMER,GetString(IDS_MENU_RENOMMER));
		if (gbShowMenu_Delete) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_SUPPRIMER,GetString(IDS_MENU_SUPPRIMER));
		if (gbShowMenu_EnableDisable || gbShowMenu_Rename || gbShowMenu_Delete) bAddSeparator=TRUE;
		// le menu "Identifiant et mot de passe" n'est affiché que si la config en base de registre l'autorise
		// et si la catégorie contient au moins une config
		if (gbShowMenu_ChangeCategIds && TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hItem)!=NULL)
		{
			if (bAddSeparator) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_CHANGER_IDS,GetString(IDS_MENU_CHANGER_IDS));
			bAddSeparator=TRUE;
		}
		if (gbEnableOption_ManualPutConfig && gbInternetManualPutConfig && TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hItem)!=NULL)
		{
			if (bAddSeparator) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
			if (giNbDomains>1)
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_PUBLISH_TO,GetString(IDS_MENU_UPLOADER_VERS));
			else
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_PUBLISH,GetString(IDS_MENU_UPLOADER));
			bAddSeparator=TRUE;
		}
		if (bAddSeparator && (gbShowMenu_AddApp || gbShowMenu_AddCateg)) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
		if (gbShowMenu_AddApp) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_APPLI,GetString(IDS_MENU_AJOUTER_APPLI));
		if (gbShowMenu_AddCateg) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_CATEG,GetString(IDS_MENU_AJOUTER_CATEG));
	}
	else // c'est une appli
	{
		// Construction sous-menu catégories
		hSubMenuCateg=CreatePopupMenu(); if (hSubMenuCateg==NULL) goto end;
		for (i=0;i<giNbCategories;i++) InsertMenu(hSubMenuCateg, (UINT)-1, MF_BYPOSITION,SUBMENU_CATEG+i,gptCategories[i].szLabel);

		iAction=TVItemGetLParam(w,hItem);
		if (iAction==-1) goto end;
		// le menu "lancer cette application" n'est affiché que si config en base de registre et
		// si le fullpathname n'est pas vide (sauf pour applis web où à défaut on utilisera l'URL)
		if (gbShowMenu_LaunchApp && (gptActions[iAction].iType==WEBSSO || gptActions[iAction].iType==XEBSSO || *gptActions[iAction].szFullPathName!=0))
		{
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_LANCER_APPLI,GetString(IDS_LAUNCH_APP));
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
			bAddSeparator=TRUE;
		}
		if (gbShowMenu_EnableDisable && !gbAdmin)
		{
			if (gptActions[iAction].bActive)
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_DESACTIVER,GetString(IDS_MENU_DESACTIVER));
			else
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_ACTIVER,GetString(IDS_MENU_ACTIVER));
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
			bAddSeparator=TRUE;
		}
		if (gbShowMenu_Rename) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_RENOMMER,GetString(IDS_MENU_RENOMMER));
		if (gbShowMenu_Move) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION |MF_POPUP,(UINT_PTR)hSubMenuCateg,GetString(IDS_MENU_DEPLACER));
		if (gbShowMenu_Duplicate) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_DUPLIQUER,GetString(IDS_MENU_DUPLIQUER));
		if (gbShowMenu_AddAccount) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_COMPTE,GetString(IDS_MENU_AJOUTER_COMPTE));
		if (gbShowMenu_Delete) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_SUPPRIMER,GetString(IDS_MENU_SUPPRIMER));
		if (gbShowMenu_Rename||gbShowMenu_Move||gbShowMenu_Duplicate||gbShowMenu_AddAccount||gbShowMenu_Delete) bAddSeparator=TRUE;
		if (gbInternetManualPutConfig && gbEnableOption_ManualPutConfig)
		{
			InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
			if (giNbDomains>1)
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_PUBLISH_TO,GetString(IDS_MENU_UPLOADER_VERS));
			else
				InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_PUBLISH,GetString(IDS_MENU_UPLOADER));
			bAddSeparator=TRUE;
		}
		if (bAddSeparator && (gbShowMenu_AddApp || gbShowMenu_AddCateg)) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
		if (gbShowMenu_AddApp) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_APPLI,GetString(IDS_MENU_AJOUTER_APPLI));
		if (gbShowMenu_AddCateg) InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_CATEG,GetString(IDS_MENU_AJOUTER_CATEG));
	}
	SetForegroundWindow(w);
	TrackPopupMenu(hMenu, TPM_TOPALIGN,pt.x, pt.y, 0, w, NULL );
end:	
	if (hSubMenuCateg!=NULL) DestroyMenu(hSubMenuCateg);
	if (hSubMenuUpload!=NULL) DestroyMenu(hSubMenuUpload);
	if (hSubMenuUploadIdPwd!=NULL) DestroyMenu(hSubMenuUploadIdPwd);
	if (hMenu!=NULL) DestroyMenu(hMenu);
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// TVDomainsShowContextMenu()
// ----------------------------------------------------------------------------------
// Affichage du menu contextuel sur clic-droit dans la treeview des domaines
// ----------------------------------------------------------------------------------
static void TVDomainsShowContextMenu(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=NULL;
	HTREEITEM hItem;
	//int iDomain;
	//int i;

	hMenu=CreatePopupMenu(); if (hMenu==NULL) goto end;
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_DOMAINS));
	if (hItem==NULL) { TRACE((TRACE_INFO,_F_,"TreeView_GetSelection()->NULL")); goto end; }
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_RENOMMER_DOMAINE,GetString(IDS_MENU_RENOMMER));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_SUPPRIMER_DOMAINE,GetString(IDS_MENU_SUPPRIMER));
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0,"");
	InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, MENU_AJOUTER_DOMAINE,GetString(IDS_MENU_ADD_DOMAIN));
	SetForegroundWindow(w);
	TrackPopupMenu(hMenu, TPM_TOPALIGN,pt.x, pt.y, 0, w, NULL );
end:	
	if (hMenu!=NULL) DestroyMenu(hMenu);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ShowApplicationDetails(HWND w,int iAction)
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void ShowApplicationDetails(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));
	char szTmp[10+1];
	gbIsChanging=TRUE;
	
	if (gbAdmin) 
	{
		CheckDlgButton(w,CK_AD_ID,BST_UNCHECKED);
		CheckDlgButton(w,CK_AD_PWD,BST_UNCHECKED);
	}
	if (iAction>giNbActions) { TRACE((TRACE_ERROR,_F_,"iAction=%d ! (giNbActions=%d)",iAction,giNbActions)); goto end; }
	giLastApplicationConfig=iAction;

	if (gbShowGeneratedPwd) { gbShowGeneratedPwd=FALSE; gbShowPwd=FALSE; }

	// 1.03 : ne pas dévoiler les mots de passe imposés par le serveur
	// 1.05 : sauf pour l'admin
	if ((gptActions[iAction].iWithIdPwd & CONFIG_WITH_PWD) && !gbInternetManualPutConfig) gbShowPwd=FALSE;

	EnableControls(w,gptActions[iAction].iType,TRUE);

	// déchiffrement mot de passe
	SetDlgItemText(w,TB_PWD,"");
	SetDlgItemText(w,TB_PWD_CLEAR,"");
	if (*gptActions[iAction].szPwdEncryptedValue!=0)
	{
		char *pszDecryptedValue=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
		if (pszDecryptedValue!=NULL) 
		{
			SetDlgItemText(w,gbShowPwd?TB_PWD_CLEAR:TB_PWD,pszDecryptedValue);
			if (gbAdmin) 
			{
				if (strcmp(pszDecryptedValue,gcszADPassword)==0) 
				{
					CheckDlgButton(w,CK_AD_PWD,BST_CHECKED);
					EnableWindow(GetDlgItem(w,TB_PWD),FALSE);
					EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),FALSE);
				}
				else
				{
					EnableWindow(GetDlgItem(w,TB_PWD),TRUE);
					EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),TRUE);
				}
			}
			SecureZeroMemory(pszDecryptedValue,strlen(pszDecryptedValue));
			free(pszDecryptedValue);
		}
	}
	if (gptActions[iAction].iType==POPSSO)
	{
		SetDlgItemText(w,TB_ID_ID,"");
		SetDlgItemText(w,TB_PWD_ID,"");
		SetDlgItemText(w,TB_VALIDATION,"");
	}
	else
	{
		SetDlgItemText(w,TB_ID_ID,gptActions[iAction].szId1Name);
		SetDlgItemText(w,TB_PWD_ID,gptActions[iAction].szPwdName);
		SetDlgItemText(w,TB_VALIDATION,gptActions[iAction].szValidateName);
	}

	CheckDlgButton(w,CK_KBSIM,gptActions[iAction].bKBSim?BST_CHECKED:BST_UNCHECKED);
	SetDlgItemText(w,TB_KBSIM,gptActions[iAction].szKBSim);
	// 0.93B7 ISSUE#10 : instruction déplacée en fin de fonction
	// MoveControls(w,GetDlgItem(w,TAB_CONFIG)); 

	SetDlgItemText(w,TB_ID, gptActions[iAction].szId1Value);
	if (gbAdmin) 
	{
		if (strcmp(gptActions[iAction].szId1Value,gcszUsername)==0) 
		{
			CheckDlgButton(w,CK_AD_ID,BST_CHECKED);
			EnableWindow(GetDlgItem(w,TB_ID),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(w,TB_ID),TRUE);
		}
	}
	
	SetDlgItemText(w,TB_ID2,gptActions[iAction].szId2Value);
	SetDlgItemText(w,TB_ID3,gptActions[iAction].szId3Value);
	SetDlgItemText(w,TB_ID4,gptActions[iAction].szId4Value);
	SetDlgItemText(w,TB_TITRE,gptActions[iAction].szTitle);
	SetDlgItemText(w,TB_URL,gptActions[iAction].szURL);
	SendMessage(GetDlgItem(w,CB_TYPE),CB_SETCURSEL,gptActions[iAction].iType,0);
	SetDlgItemText(w,TB_ID2_ID,gptActions[iAction].szId2Name);
	SetDlgItemText(w,TB_ID3_ID,gptActions[iAction].szId3Name);
	SetDlgItemText(w,TB_ID4_ID,gptActions[iAction].szId4Name);
	sprintf_s(szTmp,sizeof(szTmp),"%d",gptActions[iAction].iPwdGroup);SetDlgItemText(w,TB_PWD_GROUP,szTmp);
	SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_SETCURSEL,gptActions[iAction].id2Type,0);
	SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_SETCURSEL,gptActions[iAction].id3Type,0);
	SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_SETCURSEL,gptActions[iAction].id4Type,0);
	SetDlgItemText(w,TB_LANCEMENT,gptActions[iAction].szFullPathName);
	//EnableControls(w,gptActions[iAction].iType,TRUE);
	CheckDlgButton(w,CK_AUTO_LOCK,gptActions[iAction].bAutoLock?BST_CHECKED:BST_UNCHECKED);
	// 0.90 : affichage de l'application en cours de modification dans la barre de titre
	// 0.92B8 : affichage d'infos techniques dans la barre de titre si SHIFT enfoncée
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		time_t t;
		if (gptActions[iAction].tLastSSO==-1) t=-1;
		else t=time(NULL)-gptActions[iAction].tLastSSO;
		// wsprintf(buf2048,"swSSO [id=%d | i=%d | categ=%d | domain=%d | t=%ld]",gptActions[iAction].iConfigId,iAction,gptActions[iAction].iCategoryId,gptActions[iAction].iDomainId,t);
		wsprintf(buf2048,"swSSO [id=%d | i=%d | categ=%d | t=%ld]",gptActions[iAction].iConfigId,iAction,gptActions[iAction].iCategoryId,t);
	}
	else
	{
		wsprintf(buf2048,"%s [%s]",GetString(IDS_TITRE_APPNSITES),gptActions[iAction].szApplication);		
	}
	SetWindowText(w,buf2048);
	// 0.93B7 ISSUE#10
	MoveControls(w,GetDlgItem(w,TAB_CONFIG));

	// 1.03 : empêche la modification ids et mdp si valeurs forcées par le serveur (sauf pour l'admin !)
	if (!gbInternetManualPutConfig)
	{
		EnableWindow(GetDlgItem(w,TB_ID), ((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_ID1)==0));
		EnableWindow(GetDlgItem(w,TB_ID2),((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_ID2)==0));
		EnableWindow(GetDlgItem(w,TB_ID3),((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_ID3)==0));
		EnableWindow(GetDlgItem(w,TB_ID4),((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_ID4)==0));
		EnableWindow(GetDlgItem(w,TB_PWD),((gptActions[giLastApplicationConfig].iWithIdPwd & CONFIG_WITH_PWD)==0));
	}

end:
	gbIsChanging=FALSE;
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// GetApplicationDetails()
// ----------------------------------------------------------------------------------
// Récupération dans la partie droite du détail de l'application sélectionnée
// ----------------------------------------------------------------------------------
// [in] iAction = indice de l'application sélectionnée dans la liste
// ----------------------------------------------------------------------------------
void GetApplicationDetails(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, "iAction=%d",iAction));

	char szPassword[50+1];
	char *pszEncryptedPassword=NULL;

	// ISSUE#142 : si iAction=-1, on sort direct
	if (iAction==-1) goto end;

	// évalue si la configuration a changé et s'il faut donc la renvoyer sur le serveur
	// stocke en même temps les éventuelles nouvelles valeurs de config
	BOOL bChanged=FALSE;
	int iTmpType;
	int iTmpPwdGroup;
	BOOL bTmpChecked;

	iTmpType=SendMessage(GetDlgItem(w,CB_TYPE),CB_GETCURSEL,0,0); 
	if (iTmpType!=gptActions[iAction].iType)
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (iType : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].iType,iTmpType));
		bChanged=TRUE;
		gptActions[iAction].iType=iTmpType;
	}
	iTmpType=SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_GETCURSEL,0,0); 
	if (iTmpType!=gptActions[iAction].id2Type)
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (id2Type : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].id2Type,iTmpType));
		bChanged=TRUE;
		gptActions[iAction].id2Type=iTmpType;
	}
	iTmpType=SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_GETCURSEL,0,0); 
	if (iTmpType!=gptActions[iAction].id3Type)
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (id3Type : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].id3Type,iTmpType));
		bChanged=TRUE;
		gptActions[iAction].id3Type=iTmpType;
	}
	iTmpType=SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_GETCURSEL,0,0); 
	if (iTmpType!=gptActions[iAction].id4Type)
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (id4Type : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].id4Type,iTmpType));
		bChanged=TRUE;
		gptActions[iAction].id4Type=iTmpType;
	}
	GetDlgItemText(w,TB_TITRE,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szTitle)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szTitle : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szTitle,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szTitle,sizeof(gptActions[iAction].szTitle),buf2048);
	}
	GetDlgItemText(w,TB_URL,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szURL)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szURL : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szURL,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szURL,sizeof(gptActions[iAction].szURL),buf2048);
	}
	GetDlgItemText(w,TB_ID_ID,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szId1Name)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId1Name : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId1Name,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szId1Name,sizeof(gptActions[iAction].szId1Name),buf2048);
	}
	GetDlgItemText(w,TB_ID2_ID,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szId2Name)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId2Name : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId2Name,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szId2Name,sizeof(gptActions[iAction].szId2Name),buf2048);
	}
	GetDlgItemText(w,TB_ID3_ID,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szId3Name)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId3Name : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId3Name,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szId3Name,sizeof(gptActions[iAction].szId3Name),buf2048);
	}
	GetDlgItemText(w,TB_ID4_ID,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szId4Name)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId4Name : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId4Name,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szId4Name,sizeof(gptActions[iAction].szId4Name),buf2048);
	}
	GetDlgItemText(w,TB_PWD_GROUP,buf2048,sizeof(buf2048));
	iTmpPwdGroup=atoi(buf2048);
	if (iTmpPwdGroup!=gptActions[iAction].iPwdGroup) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (iPwdGroup : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].iPwdGroup,iTmpPwdGroup));
		bChanged=TRUE;
		gptActions[iAction].iPwdGroup=iTmpPwdGroup;
	}
	GetDlgItemText(w,TB_PWD_ID,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szPwdName)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szPwdName : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szPwdName,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szPwdName,sizeof(gptActions[iAction].szPwdName),buf2048);
	}
	GetDlgItemText(w,TB_VALIDATION,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szValidateName)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szValidateName : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szValidateName,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szValidateName,sizeof(gptActions[iAction].szValidateName),buf2048);
	}
	bTmpChecked=IsDlgButtonChecked(w,CK_KBSIM)==BST_CHECKED?TRUE:FALSE;
	if (bTmpChecked!=gptActions[iAction].bKBSim) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (bKBSim : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].bKBSim,bTmpChecked));
		bChanged=TRUE;
		gptActions[iAction].bKBSim=bTmpChecked;
	}
	bTmpChecked=IsDlgButtonChecked(w,CK_AUTO_LOCK)==BST_CHECKED?TRUE:FALSE;
	if (bTmpChecked!=gptActions[iAction].bAutoLock) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (bAutoLock : %d -> %d)",gptActions[iAction].szApplication,gptActions[iAction].bAutoLock,bTmpChecked));
		bChanged=TRUE;
		gptActions[iAction].bAutoLock=bTmpChecked;
	}
	GetDlgItemText(w,TB_KBSIM,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szKBSim)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szKBSim : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szKBSim,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szKBSim,sizeof(gptActions[iAction].szKBSim),buf2048);
	}
	GetDlgItemText(w,TB_LANCEMENT,buf2048,sizeof(buf2048));
	if (strcmp(buf2048,gptActions[iAction].szFullPathName)!=0) 
	{
		TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szFullPathName : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szFullPathName,buf2048));
		bChanged=TRUE;
		strcpy_s(gptActions[iAction].szFullPathName,sizeof(gptActions[iAction].szFullPathName),buf2048);
	}

	if (gbShowMenu_UploadWithIdPwd)
	{
		GetDlgItemText(w,TB_ID,buf2048,sizeof(buf2048));
		if (strcmp(buf2048,gptActions[iAction].szId1Value)!=0) 
		{
			TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId1Value : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId1Value,buf2048));
			bChanged=TRUE;
			strcpy_s(gptActions[iAction].szId1Value,sizeof(gptActions[iAction].szId1Value),buf2048);
		}
		GetDlgItemText(w,TB_ID2,buf2048,sizeof(buf2048));
		if (strcmp(buf2048,gptActions[iAction].szId2Value)!=0) 
		{
			TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId2Value : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId2Value,buf2048));
			bChanged=TRUE;
			strcpy_s(gptActions[iAction].szId2Value,sizeof(gptActions[iAction].szId2Value),buf2048);
		}
		GetDlgItemText(w,TB_ID3,buf2048,sizeof(buf2048));
		if (strcmp(buf2048,gptActions[iAction].szId3Value)!=0) 
		{
			TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId3Value : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId3Value,buf2048));
			bChanged=TRUE;
			strcpy_s(gptActions[iAction].szId3Value,sizeof(gptActions[iAction].szId3Value),buf2048);
		}
		GetDlgItemText(w,TB_ID4,buf2048,sizeof(buf2048));
		if (strcmp(buf2048,gptActions[iAction].szId4Value)!=0) 
		{
			TRACE((TRACE_DEBUG,_F_,"Chgt config %s (szId4Value : %s -> %s)",gptActions[iAction].szApplication,gptActions[iAction].szId4Value,buf2048));
			bChanged=TRUE;
			strcpy_s(gptActions[iAction].szId4Value,sizeof(gptActions[iAction].szId4Value),buf2048);
		}
		GetDlgItemText(w,gbShowPwd?TB_PWD_CLEAR:TB_PWD,buf2048,sizeof(buf2048));
		char *pszDecryptedValue=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
		if (pszDecryptedValue!=NULL) 
		{
			if (strcmp(buf2048,pszDecryptedValue)!=0) 
			{
				TRACE((TRACE_PWD,_F_,"Chgt config %s (Password : %s -> %s)",gptActions[iAction].szApplication,pszDecryptedValue,buf2048));
				bChanged=TRUE;
			}
			SecureZeroMemory(pszDecryptedValue,strlen(pszDecryptedValue));
			free(pszDecryptedValue);
		}
		SecureZeroMemory(buf2048,sizeof(buf2048));
	}
	else
	{
		GetDlgItemText(w,TB_ID, gptActions[iAction].szId1Value,sizeof(gptActions[iAction].szId1Value));
		GetDlgItemText(w,TB_ID2,gptActions[iAction].szId2Value,sizeof(gptActions[iAction].szId2Value));
		GetDlgItemText(w,TB_ID3,gptActions[iAction].szId3Value,sizeof(gptActions[iAction].szId3Value));
		GetDlgItemText(w,TB_ID4,gptActions[iAction].szId4Value,sizeof(gptActions[iAction].szId4Value));

	}
	GetDlgItemText(w,gbShowPwd?TB_PWD_CLEAR:TB_PWD,szPassword,sizeof(szPassword));
	if (*szPassword!=0)
	{
		pszEncryptedPassword=swCryptEncryptString(szPassword,ghKey1);
		if (pszEncryptedPassword==NULL) goto end;
		strcpy_s(gptActions[iAction].szPwdEncryptedValue,sizeof(gptActions[iAction].szPwdEncryptedValue),pszEncryptedPassword);
		
		// ISSUE#191 : changement des mots de passe de toutes les applications du groupe
		free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
		pszEncryptedPassword=NULL;
		if (gptActions[iAction].iPwdGroup!=-1) // change les autres applis
		{
			TRACE((TRACE_DEBUG,_F_,"Changement mot de passe groupé induit par appli %s",gptActions[iAction].szApplication));
			int i;
			for (i=0;i<giNbActions;i++)
			{
				if ((gptActions[i].iPwdGroup==gptActions[iAction].iPwdGroup) &&
					(*gptActions[i].szId1Value!=0) && (*gptActions[iAction].szId1Value!=0) && // nouvelle condition ISSUE#235
					(stricmp(gptActions[i].szId1Value,gptActions[iAction].szId1Value)==0))    // nouvelle condition ISSUE#235
				{
					TRACE((TRACE_DEBUG,_F_,"Changement mot de passe appli %s induit par appli %s",gptActions[i].szApplication,gptActions[iAction].szApplication));
					pszEncryptedPassword=swCryptEncryptString(szPassword,ghKey1);
					if (pszEncryptedPassword==NULL) goto end;
					strcpy_s(gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),pszEncryptedPassword);
					free(pszEncryptedPassword); // forcément pas NULL sinon on ne serait pas là
					pszEncryptedPassword=NULL;
				}
			}
		}
		SecureZeroMemory(szPassword,strlen(szPassword));
	}
	else
	{
		strcpy_s(gptActions[iAction].szPwdEncryptedValue,sizeof(gptActions[iAction].szPwdEncryptedValue),szPassword);
	}
	if (bChanged)
	{
		TRACE((TRACE_INFO,_F_,"Changement de config pour %s",gptActions[iAction].szApplication));
		// gptActions[iAction].bConfigOK=FALSE; // 0.90B1 : on ne gère plus l'état OK car plus de remontée auto
		gptActions[iAction].bConfigSent=FALSE;
		//gptActions[iAction].tLastDetect=-1;
		//gptActions[iAction].wLastDetect=NULL;
		gptActions[iAction].tLastSSO=-1;
		gptActions[iAction].wLastSSO=NULL;
		gptActions[iAction].iWaitFor=WAIT_IF_SSO_OK;
	}
end:
	if (pszEncryptedPassword!=NULL) free(pszEncryptedPassword);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// TVAddApplication(HWND w,int iAction)
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
HTREEITEM TVAddApplication(HWND w,int iAction,char *pszApplication,BOOL bWithId)
{
	TRACE((TRACE_ENTER,_F_, "Label=%s iCategoryId=%d bWithId=%d",gptActions[iAction].szApplication,gptActions[iAction].iCategoryId,bWithId));
	HTREEITEM hItem=NULL;
	TVINSERTSTRUCT tvis;
	int iCategory;
	char buf[LEN_APPLICATION_NAME+LEN_ID+10];

	iCategory=GetCategoryIndex(gptActions[iAction].iCategoryId);
	if (iCategory==-1) iCategory=0;
	tvis.hParent=gptCategories[iCategory].hItem;
	tvis.hInsertAfter=TVI_SORT;
	tvis.itemex.mask=TVIF_TEXT|TVIF_PARAM; //|TVIF_STATE;
	tvis.itemex.cchTextMax=0;
	if (pszApplication==NULL)
	{
		if (bWithId)
		{
			sprintf_s(buf,sizeof(buf),"%s (identifiant : %s)",gptActions[iAction].szApplication,gptActions[iAction].szId1Value);
			tvis.itemex.pszText=buf;
		}
		else
		{
			tvis.itemex.pszText=gptActions[iAction].szApplication;
		}
	}
	else
	{
		tvis.itemex.pszText=pszApplication;
	}
	tvis.itemex.lParam=iAction;
	//tvis.itemex.stateMask=TVIS_STATEIMAGEMASK;
	//tvis.itemex.state=INDEXTOSTATEIMAGEMASK(gptActions[iAction].bActive?1:2);
	//tvis.itemex.state=INDEXTOSTATEIMAGEMASK(gptActions[iAction].bActive?(gptActions[iAction].bConfigSent?1:3):2);

	hItem=TreeView_InsertItem(GetDlgItem(w,TV_APPLICATIONS),&tvis);
	if (hItem==NULL) { TRACE((TRACE_ERROR,_F_,"TreeView_InsertItem()")) ; goto end; }

	TVUpdateItemState(w,hItem,iAction);

end:
	TRACE((TRACE_LEAVE,_F_, "hItem=0x%08lx",hItem));
	return hItem;
}	

//-----------------------------------------------------------------------------
// TVAddCategory(HWND w,int iAction)
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
HTREEITEM TVAddCategory(HWND w,int iCategIndex)
{
	TRACE((TRACE_ENTER,_F_, ""));
	TVINSERTSTRUCT tvis;

	tvis.hParent=TVI_ROOT;
	tvis.hInsertAfter=TVI_SORT;
	tvis.itemex.mask=TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
	tvis.itemex.state=TVIS_BOLD ;
	if (gptCategories[iCategIndex].bExpanded) tvis.itemex.state|=TVIS_EXPANDED;
	tvis.itemex.stateMask=TVIS_EXPANDED|TVIS_BOLD ;
	tvis.itemex.cchTextMax=0;
	tvis.itemex.pszText=gptCategories[iCategIndex].szLabel;
	tvis.itemex.lParam=gptCategories[iCategIndex].id;
	gptCategories[iCategIndex].hItem=TreeView_InsertItem(GetDlgItem(w,TV_APPLICATIONS),&tvis);
	if (gptCategories[iCategIndex].hItem==NULL) { TRACE((TRACE_ERROR,_F_,"TreeView_InsertItem()==NULL")); goto end; }
end:
	TRACE((TRACE_LEAVE,_F_, "hItem=0x%08lx",gptCategories[iCategIndex].hItem));
	return gptCategories[iCategIndex].hItem;
}		

//-----------------------------------------------------------------------------
// FillTreeView()
//-----------------------------------------------------------------------------
// Remplit la TreeView TV_APPLICATIONS
//-----------------------------------------------------------------------------
void FillTreeView(HWND w,BOOL bWithId)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	HTREEITEM hItem=NULL;

	TreeView_DeleteAllItems(GetDlgItem(w,TV_APPLICATIONS));

	// Ajout des catégories
	for (i=0;i<giNbCategories;i++) 
	{
		hItem=TVAddCategory(w,i); if (hItem==NULL) goto end;
	}
	// Ajout des actions
	for (i=0;i<giNbActions;i++) 
	{
		if (bWithId && i==giActionIdPwdAsked) continue; // ISSUE#225
		if (gptActions[i].iType!=WEBPWD)
		{
			hItem=TVAddApplication(w,i,NULL,bWithId); if (hItem==NULL) goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

LRESULT CALLBACK PwdProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	TRACE((TRACE_ENTER,_F_, ""));
	UNREFERENCED_PARAMETER(uIdSubclass);
	UNREFERENCED_PARAMETER(dwRefData);
	LRESULT lrc=FALSE;
	POINT pt;
	HMENU hMenu=NULL;
	HMENU hPopupMenu=NULL;
	BOOL bTrackPopupMenu;
	char szPwd[20+1];
	int iPwdLen,iPwdType;
	switch (msg)
	{
		case WM_CONTEXTMENU:
		{
			hMenu=LoadMenu(ghInstance,MAKEINTRESOURCE(IDM_PWD_MENU));
			if (hMenu==NULL) { TRACE((TRACE_ERROR,_F_,"LoadMenu(IDM_PWD_MENU)")); goto end; }
			hPopupMenu=GetSubMenu(hMenu,0);
			if (hPopupMenu==NULL) { TRACE((TRACE_ERROR,_F_,"GetSubMenu()")); goto end; }
			GetCursorPos(&pt);
			bTrackPopupMenu=TrackPopupMenu(hPopupMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,pt.x, pt.y, 0, w, NULL);
			switch (bTrackPopupMenu)
			{
				case IDM_SWUNDO:
					SendMessage(w,EM_UNDO,0,0);
					break;
				case IDM_SWCUT:
					SendMessage(w,WM_CUT,0,0);
					break;
				case IDM_SWCOPY:
					SendMessage(w,WM_COPY,0,0);
					break;
				case IDM_SWPASTE:
					SendMessage(w, WM_PASTE,0,0);
					break;
				case IDM_SWCLEARSELECTION:
					SendMessage(w,WM_CLEAR,0,0);
					break;
				case IDM_SWSELECTALL:
					SendMessage(w,EM_SETSEL,0,-1);
					break;
				case IDM_GENERATE_ALPHANUM_10:
				case IDM_GENERATE_ALPHANUM_12:
				case IDM_GENERATE_ALPHANUM_15:
				case IDM_GENERATE_ALPHANUM_20:
				case IDM_GENERATE_SPECIALCHARS_10:
				case IDM_GENERATE_SPECIALCHARS_12:
				case IDM_GENERATE_SPECIALCHARS_15:
				case IDM_GENERATE_SPECIALCHARS_20:
					if (!gbShowPwd)
					{
						gbShowPwd=TRUE;
						gbShowGeneratedPwd=TRUE;
						ShowWindow(GetDlgItem(gwAppNsites,TB_PWD),SW_HIDE);
						ShowWindow(GetDlgItem(gwAppNsites,TB_PWD_CLEAR),SW_SHOW);
					}
					if (bTrackPopupMenu==IDM_GENERATE_ALPHANUM_10)			{ iPwdLen=10; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM; }
					else if (bTrackPopupMenu==IDM_GENERATE_ALPHANUM_12)		{ iPwdLen=12; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM; }
					else if (bTrackPopupMenu==IDM_GENERATE_ALPHANUM_15)		{ iPwdLen=15; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM; }
					else if (bTrackPopupMenu==IDM_GENERATE_ALPHANUM_20)		{ iPwdLen=20; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM; }
					else if (bTrackPopupMenu==IDM_GENERATE_SPECIALCHARS_10) { iPwdLen=10; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM|PWDTYPE_SPECIALCHARS; }
					else if (bTrackPopupMenu==IDM_GENERATE_SPECIALCHARS_12) { iPwdLen=12; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM|PWDTYPE_SPECIALCHARS; }
					else if (bTrackPopupMenu==IDM_GENERATE_SPECIALCHARS_15) { iPwdLen=15; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM|PWDTYPE_SPECIALCHARS; }
					else if (bTrackPopupMenu==IDM_GENERATE_SPECIALCHARS_20) { iPwdLen=20; iPwdType=PWDTYPE_ALPHA|PWDTYPE_NUM|PWDTYPE_SPECIALCHARS; }
					else goto end;
					swGenerateRandomPwd(szPwd,iPwdLen,iPwdType);
					SetDlgItemText(gwAppNsites,TB_PWD_CLEAR,szPwd);
					SecureZeroMemory(szPwd,sizeof(szPwd));
					break;
		   }
		   lrc=TRUE;
		}
		break;
		case WM_INITMENUPOPUP:
		{
			UINT uBegSel,uEndSel;
			if (!SendMessage(w, EM_CANUNDO, 0, 0)) EnableMenuItem( (HMENU) wp, IDM_SWUNDO, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
			SendMessage (w, EM_GETSEL, (UINT)&uBegSel, (UINT)&uEndSel);
			if (uBegSel == uEndSel)
			{
				EnableMenuItem( (HMENU) wp, IDM_SWCUT, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
				EnableMenuItem( (HMENU) wp, IDM_SWCOPY, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
				EnableMenuItem( (HMENU) wp, IDM_SWCLEARSELECTION, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
			}
			if (!IsClipboardFormatAvailable(CF_TEXT)) EnableMenuItem( (HMENU) wp, IDM_PASTE, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
			if (SendMessage(w, WM_GETTEXTLENGTH , (WPARAM)-1, 0) == (LRESULT)(uEndSel - uBegSel)) EnableMenuItem( (HMENU) wp, IDM_SELECTALL, MF_BYCOMMAND | MF_DISABLED| MF_GRAYED );
		}
		lrc=DefSubclassProc(w,msg,wp,lp);
		break;
	default:
		lrc=DefSubclassProc(w,msg,wp,lp);
	}
end:
   if (hMenu!=NULL) DestroyMenu(hMenu);
	TRACE((TRACE_LEAVE,_F_, "lrc=%ld",lrc));
	return lrc;
} 

//-----------------------------------------------------------------------------
// OnInitDialog()
//-----------------------------------------------------------------------------
void OnInitDialog(HWND w,T_APPNSITES *ptAppNsites)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
    TCITEM tcItem; 
	int cx;
	int cy;
	RECT rect;

	gbIsChanging=TRUE;
	gbTVApplicationsLabelEditing=FALSE;
	gbTVDomainsLabelEditing=FALSE;
	//gbAtLeastOneAppRenamed=FALSE; // 0.90B1 : renommage direct, flag inutile
	gbEffacementEnCours;
	gbShowPwd=FALSE;
	gbShowGeneratedPwd=FALSE;
	gwAppNsites=w;

	// Positionnement et dimensionnement de la fenêtre
	// ISSUE#1 : si Alt enfoncée à l'ouverture, retaillage et repositionnement par défaut
	if ((gx!=-1 && gy!=-1 && gcx!=-1 && gcy!=-1 && gcx>=560 && (gcy>=540+(gbAdmin?50:0))) && HIBYTE(GetAsyncKeyState(VK_MENU))==0) 
	{
		SetWindowPos(w,NULL,gx,gy,gcx,gcy,SWP_NOZORDER);
	}
	else // position par défaut
	{
		cx = GetSystemMetrics( SM_CXSCREEN );
		cy = GetSystemMetrics( SM_CYSCREEN );
		SetWindowPos(w,NULL,0,0,560,540+(gbAdmin?50:0),SWP_NOMOVE | SWP_NOZORDER);
		GetWindowRect(w,&rect);
		SetWindowPos(w,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}

	// icone ALT-TAB
	SendMessage(gwAppNsites,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
	SendMessage(gwAppNsites,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
	
	// Création des onglets
	tcItem.mask = TCIF_TEXT ; 
	tcItem.pszText = GetString(IDS_TAB_APPLICATIONS); // "Sites et applications"; 
	TabCtrl_InsertItem(GetDlgItem(w,TAB_APPLICATIONS),0,&tcItem);
	if (gbAdmin)
	{
		tcItem.pszText = GetString(IDS_TAB_DOMAINS); // "Domains"; 
		TabCtrl_InsertItem(GetDlgItem(w,TAB_APPLICATIONS),1,&tcItem);
	}
	tcItem.pszText = GetString(IDS_TAB_IDPWD1); // "Identifiant et mot de passe"; 
	TabCtrl_InsertItem(GetDlgItem(w,TAB_IDPWD),0,&tcItem);
	tcItem.pszText = GetString(IDS_TAB_IDPWD2); // "Identifiants complémentaires"; 
	TabCtrl_InsertItem(GetDlgItem(w,TAB_IDPWD),1,&tcItem);
	tcItem.pszText = GetString(IDS_TAB_CONFIG1); // "Configuration"; 
	TabCtrl_InsertItem(GetDlgItem(w,TAB_CONFIG),0,&tcItem);
	tcItem.pszText = GetString(IDS_TAB_CONFIG2); // "Champs complémentaires"; 
	TabCtrl_InsertItem(GetDlgItem(w,TAB_CONFIG),1,&tcItem);

	if (!gbShowMenu_LaunchApp)
	{
		ShowWindow(GetDlgItem(w,TX_LANCEMENT),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_LANCEMENT),SW_HIDE);
		ShowWindow(GetDlgItem(w,PB_PARCOURIR),SW_HIDE);
	}
		
	// Remplissage des combo
	SendMessage(GetDlgItem(w,CB_TYPE),CB_ADDSTRING,0,(LPARAM)"");
	SendMessage(GetDlgItem(w,CB_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOTYPE1));
	SendMessage(GetDlgItem(w,CB_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOTYPE2));
	SendMessage(GetDlgItem(w,CB_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOTYPE3));
	SendMessage(GetDlgItem(w,CB_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOTYPE4));
	SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_ADDSTRING,0,(LPARAM)"");
	SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE1));
	SendMessage(GetDlgItem(w,CB_ID2_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE2));
	SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_ADDSTRING,0,(LPARAM)"");
	SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE1));
	SendMessage(GetDlgItem(w,CB_ID3_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE2));
	SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_ADDSTRING,0,(LPARAM)"");
	SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE1));
	SendMessage(GetDlgItem(w,CB_ID4_TYPE),CB_ADDSTRING,0,(LPARAM)GetString(IDS_COMBOIDTYPE2));

	// Limitation de la longueur des champs de saisie
	SendMessage(GetDlgItem(w,TB_ID),EM_LIMITTEXT,LEN_ID,0);
	SendMessage(GetDlgItem(w,TB_ID2),EM_LIMITTEXT,LEN_ID,0);
	SendMessage(GetDlgItem(w,TB_ID3),EM_LIMITTEXT,LEN_ID,0);
	SendMessage(GetDlgItem(w,TB_ID4),EM_LIMITTEXT,LEN_ID,0);
	SendMessage(GetDlgItem(w,TB_PWD),EM_LIMITTEXT,LEN_PWD,0);
	SendMessage(GetDlgItem(w,TB_PWD_CLEAR),EM_LIMITTEXT,LEN_PWD,0);
	SendMessage(GetDlgItem(w,TB_TITRE),EM_LIMITTEXT,LEN_TITLE,0);
	SendMessage(GetDlgItem(w,TB_URL),EM_LIMITTEXT,LEN_URL,0);
	SendMessage(GetDlgItem(w,TB_ID_ID),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_ID2_ID),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_ID3_ID),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_ID4_ID),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_PWD_GROUP),EM_LIMITTEXT,2,0);
	SendMessage(GetDlgItem(w,TB_PWD_ID),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_VALIDATION),EM_LIMITTEXT,90,0);
	SendMessage(GetDlgItem(w,TB_KBSIM),EM_LIMITTEXT,LEN_KBSIM,0);
	SendMessage(GetDlgItem(w,TB_LANCEMENT),EM_LIMITTEXT,LEN_FULLPATHNAME,0);

	// loupe et help
	SendDlgItemMessage(w, IMG_LOUPE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconLoupe);
	SendDlgItemMessage(w, IMG_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_PWD, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID2, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID3, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID4, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_TYPE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_TITRE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_URL, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_PWD_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_VALIDATION, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_LANCEMENT, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID2_TYPE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID2_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID3_TYPE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID3_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID4_TYPE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_ID4_ID, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_KBSIM, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_PWD_GROUP, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	SendDlgItemMessage(w, IMG_AUTO_LOCK, STM_SETIMAGE, IMAGE_ICON, (LPARAM)ghIconHelp);
	
	// Chargement de l'image list
	TreeView_SetImageList(GetDlgItem(w,TV_APPLICATIONS),ghImageList,TVSIL_STATE);

	// Remplissage de la treeview
	FillTreeView(w,FALSE);
	if (gbAdmin) FillTreeViewDomains(w);

	// Sélectionne l'application
	if (ptAppNsites->iSelected==-1) 
		ClearApplicationDetails(w);
	else
		TVSelectItemFromLParam(w,TYPE_APPLICATION,ptAppNsites->iSelected);

	if (giSetForegroundTimer==giTimer) giSetForegroundTimer=21;
	SetTimer(w,giSetForegroundTimer,100,NULL);

	gbPwdSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID,NULL);
	gbPwdClearSubClass=SetWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID,NULL);

	InitTooltip(w); // ISSUE#111
	
	gbIsChanging=FALSE;
	EnableWindow(GetDlgItem(w,IDAPPLY),FALSE); // ISSUE#114

	if (gbEnableOption_ManualPutConfig && gbInternetManualPutConfig)
	{
		ShowWindow(GetDlgItem(w,TX_PWD_GROUP),SW_SHOW);
		ShowWindow(GetDlgItem(w,TB_PWD_GROUP),SW_SHOW);
		ShowWindow(GetDlgItem(w,IMG_PWD_GROUP),SW_SHOW);
	}
	// ISSUE#180
	ShowWindow(GetDlgItem(w,CK_AUTO_LOCK),gbShowAutoLockOption?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(w,IMG_AUTO_LOCK),gbShowAutoLockOption?SW_SHOW:SW_HIDE);

	if (gbAdmin) { SetTextBold(w,TX_MODE_ADMIN); ShowWindow(GetDlgItem(w,TX_MODE_ADMIN),SW_SHOW); }

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ClearApplicationDetails()
// ----------------------------------------------------------------------------------
// Cache/Masque la partie droite
// ----------------------------------------------------------------------------------
void ClearApplicationDetails(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	gbIsChanging=TRUE;
	EnableControls(w,UNK,FALSE);
	// 0.90 : affichage de l'application en cours de modification dans la barre de titre
	SetWindowText(w,GetString(IDS_TITRE_APPNSITES));
	gbIsChanging=FALSE;
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// DrawAdminBar()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void DrawAdminBar(HWND w)
{
	PAINTSTRUCT ps;
	RECT rect;
	HDC dc=NULL;
	if (GetClientRect(w,&rect))
	{
		dc=BeginPaint(w,&ps);
		if (dc!=NULL)
		{
			BitBlt(dc,10,10,rect.right-20,20,0,0,0,WHITENESS);
			if (dc!=NULL) EndPaint(w,&ps);
		}
	}
}

//-----------------------------------------------------------------------------
// MoveControls()
//-----------------------------------------------------------------------------
// Repositionne les contrôles suite à redimensionnement de la fenêtre
//-----------------------------------------------------------------------------
static void MoveControls(HWND w,HWND wToRefresh)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	RECT rect;
	RECT rectKBSim;
	GetClientRect(w,&rect);
	int yPosAutoLock;
	int yOffset=gbAdmin?30:0;
	int yOffsetPwd=gbAdmin?20:0;

	RECT rectTabConfig;
	rectTabConfig.left=rect.right*2/5+15;
	rectTabConfig.top=rect.bottom*1/3-5;
	rectTabConfig.right=rectTabConfig.left+rect.right*3/5-20;
	rectTabConfig.bottom=rectTabConfig.top+rect.bottom*2/3-30;

	if (gbAdmin) SetWindowPos(GetDlgItem(w,TX_MODE_ADMIN),NULL,10,5,rect.right-20,30,SWP_NOZORDER);
		
	// OK, Annuler et Appliquer
	SetWindowPos(GetDlgItem(w,IDOK),NULL,rect.right-242,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,IDCANCEL),NULL,rect.right-162,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(w,IDAPPLY),NULL,rect.right-82,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
	
	// TabControl de gauche (liste des applications)
	SetWindowPos(GetDlgItem(w,TAB_APPLICATIONS),NULL,10,10+yOffset,rect.right*2/5,rect.bottom-45,SWP_NOZORDER);
	if (TabCtrl_GetCurSel(GetDlgItem(w,TAB_APPLICATIONS))==0) // onglet sélectionné = sites et applications
	{
		ShowWindow(GetDlgItem(w,TV_DOMAINS),SW_HIDE);
		SetWindowPos(GetDlgItem(w,TV_APPLICATIONS),NULL,20,40+yOffset,rect.right*2/5-20,rect.bottom-85,SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	else // onglet sélectionné = domains
	{
		ShowWindow(GetDlgItem(w,TV_APPLICATIONS),SW_HIDE);
		SetWindowPos(GetDlgItem(w,TV_DOMAINS),NULL,20,40+yOffset,rect.right*2/5-20,rect.bottom-85,SWP_NOZORDER|SWP_SHOWWINDOW);
	}

	// TabControl haut droite (identifiants et mot de passe)
	SetWindowPos(GetDlgItem(w,TAB_IDPWD),NULL,rect.right*2/5+15,10+yOffset,rect.right*3/5-20,rect.bottom*1/3-20,SWP_NOZORDER);
	if (TabCtrl_GetCurSel(GetDlgItem(w,TAB_IDPWD))==0) // onglet sélectionné = identifiant et mot de passe
	{
		ShowWindow(GetDlgItem(w,TX_ID2),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID3),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID4),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID2),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID3),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID4),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID2),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID3),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID4),SW_HIDE);
		SetWindowPos(GetDlgItem(w,TX_ID)   ,NULL,rect.right*2/5+25,50+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_PWD)  ,NULL,rect.right*2/5+25,80+yOffset+yOffsetPwd,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID)   ,NULL,rect.right*2/5+25+80,47+yOffset,rect.right*3/5-140,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID)  ,NULL,rect.right-30,49+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		if (gbEnableOption_ShowPassword)
		{
			SetWindowPos(GetDlgItem(w,IMG_LOUPE),NULL,rect.right-52,79+yOffset+yOffsetPwd,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_PWD)      ,NULL,rect.right*2/5+25+80,77+yOffset+yOffsetPwd,rect.right*3/5-160,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_PWD_CLEAR),NULL,rect.right*2/5+25+80,77+yOffset+yOffsetPwd,rect.right*3/5-160,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_PWD) ,NULL,rect.right-30,79+yOffset+yOffsetPwd,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
		else
		{
			ShowWindow(GetDlgItem(w,IMG_LOUPE),SW_HIDE);
			SetWindowPos(GetDlgItem(w,TB_PWD)  ,NULL,rect.right*2/5+25+80,77+yOffset+yOffsetPwd,rect.right*3/5-140,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_PWD) ,NULL,rect.right-30,79+yOffset+yOffsetPwd,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
		ShowWindow(GetDlgItem(w,TB_PWD),gbShowPwd?SW_HIDE:SW_SHOW);
		ShowWindow(GetDlgItem(w,TB_PWD_CLEAR),gbShowPwd?SW_SHOW:SW_HIDE);
		if (gbAdmin)
		{
			SetWindowPos(GetDlgItem(w,CK_AD_ID),NULL,rect.right*2/5+25+80,47+yOffset+25,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,CK_AD_PWD),NULL,rect.right*2/5+25+80,79+yOffset+43,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
	}
	else // onglet sélectionné = identifiants complémentaires
	{
		ShowWindow(GetDlgItem(w,TX_ID)   ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_PWD)  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID)   ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD)  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD_CLEAR)  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID)   ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_PWD)  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_LOUPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_AD_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_AD_PWD),SW_HIDE);
		SetWindowPos(GetDlgItem(w,TX_ID2),NULL,rect.right*2/5+25,50+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID3),NULL,rect.right*2/5+25,80+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID4),NULL,rect.right*2/5+25,110+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID2),NULL,rect.right*2/5+25+80,47+yOffset,rect.right*3/5-140,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID3),NULL,rect.right*2/5+25+80,77+yOffset,rect.right*3/5-140,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID4),NULL,rect.right*2/5+25+80,107+yOffset,rect.right*3/5-140,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID2),NULL,rect.right-30,49+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID3),NULL,rect.right-30,79+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID4),NULL,rect.right-30,109+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);

	}

	// TabControl bas droite (config)
	SetWindowPos(GetDlgItem(w,TAB_CONFIG),NULL,rect.right*2/5+15,rect.bottom*1/3-5+yOffset,rect.right*3/5-20,rect.bottom*2/3-30-yOffset,SWP_NOZORDER);
	if (TabCtrl_GetCurSel(GetDlgItem(w,TAB_CONFIG))==0) // onglet sélectionné = configuration
	{
		ShowWindow(GetDlgItem(w,TX_ID2_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID3_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID4_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_PWD_GROUP),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_AUTO_LOCK),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID2_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID3_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID4_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD_GROUP),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID2_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID3_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID4_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID2_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID3_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_ID4_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID2_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID3_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID4_TYPE),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID2_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID3_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID4_ID),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_PWD_GROUP),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_AUTO_LOCK),SW_HIDE);
		SetWindowPos(GetDlgItem(w,TX_TYPE)		,NULL,rect.right*2/5+25,rect.bottom*1/3+35+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,CB_TYPE)		,NULL,rect.right*2/5+25+110,rect.bottom*1/3+35-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_TITRE)		,NULL,rect.right*2/5+25,rect.bottom*1/3+65+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_TITRE)		,NULL,rect.right*2/5+25+110,rect.bottom*1/3+65-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_URL)		,NULL,rect.right*2/5+25,rect.bottom*1/3+95+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_URL)		,NULL,rect.right*2/5+25+110,rect.bottom*1/3+95-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,CK_KBSIM)		,NULL,rect.right*2/5+25,rect.bottom*1/3+125+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_TYPE)		,NULL,rect.right-30,rect.bottom*1/3+35-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_TITRE)	,NULL,rect.right-30,rect.bottom*1/3+65-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_URL)		,NULL,rect.right-30,rect.bottom*1/3+95-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		GetClientRect(GetDlgItem(w,CK_KBSIM),&rectKBSim);
		SetWindowPos(GetDlgItem(w,IMG_KBSIM)	,NULL,rectTabConfig.left+rectKBSim.right+15,rect.bottom*1/3+125+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		if (IsDlgButtonChecked(w,CK_KBSIM)==BST_CHECKED)
		{
			ShowWindow(GetDlgItem(w,TX_ID_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,TX_PWD_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,TX_VALIDATION),SW_HIDE);
			ShowWindow(GetDlgItem(w,TB_ID_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,TB_PWD_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,TB_VALIDATION),SW_HIDE);
			ShowWindow(GetDlgItem(w,IMG_ID_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,IMG_PWD_ID)	  ,SW_HIDE);
			ShowWindow(GetDlgItem(w,IMG_VALIDATION),SW_HIDE);
			SetWindowPos(GetDlgItem(w,TB_KBSIM)		,NULL,rect.right*2/5+25,rect.bottom*1/3+155-3+yOffset,rect.right*3/5-40,80,SWP_NOZORDER|SWP_SHOWWINDOW);
		}
		else
		{
			SetWindowPos(GetDlgItem(w,TX_ID_ID)		,NULL,rect.right*2/5+25,rect.bottom*1/3+155+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TX_PWD_ID)    ,NULL,rect.right*2/5+25,rect.bottom*1/3+185+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TX_VALIDATION),NULL,rect.right*2/5+25,rect.bottom*1/3+215+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_ID_ID)		,NULL,rect.right*2/5+25+110,rect.bottom*1/3+155-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_ID_ID)	,NULL,rect.right-30,rect.bottom*1/3+155-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_PWD_ID)	,NULL,rect.right*2/5+25+110,rect.bottom*1/3+185-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_PWD_ID)	,NULL,rect.right-30,rect.bottom*1/3+185-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_VALIDATION),NULL,rect.right*2/5+25+110,rect.bottom*1/3+215-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_VALIDATION),NULL,rect.right-30,rect.bottom*1/3+215-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			ShowWindow(GetDlgItem(w,TB_KBSIM),SW_HIDE);
		}
		if (gbShowMenu_LaunchApp)
		{
			SetWindowPos(GetDlgItem(w,TX_LANCEMENT),NULL,rect.right*2/5+25,rect.bottom*1/3+245+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_LANCEMENT),NULL,rect.right*2/5+25+110,rect.bottom*1/3+245-3+yOffset,rect.right*3/5-170,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_LANCEMENT),NULL,rect.right-30,rect.bottom*1/3+245-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,PB_PARCOURIR),NULL,rect.right-92,rect.bottom*1/3+267+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
	}
	else // onglet sélectionné = champs complémentaires
	{
		ShowWindow(GetDlgItem(w,TX_TYPE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_TITRE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_URL)		  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_ID_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_PWD_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,CB_TYPE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_TITRE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_URL)		  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_ID_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_PWD_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,CK_KBSIM)     ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_KBSIM)     ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TX_LANCEMENT) ,SW_HIDE);
		ShowWindow(GetDlgItem(w,TB_LANCEMENT) ,SW_HIDE);
		ShowWindow(GetDlgItem(w,PB_PARCOURIR) ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_TYPE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_TITRE)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_URL)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_ID_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_PWD_ID)	  ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_VALIDATION),SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_KBSIM)     ,SW_HIDE);
		ShowWindow(GetDlgItem(w,IMG_LANCEMENT) ,SW_HIDE);
		SetWindowPos(GetDlgItem(w,TX_ID2_TYPE)	,NULL,rect.right*2/5+25,rect.bottom*1/3+35+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,CB_ID2_TYPE)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+35-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID2_TYPE)	,NULL,rect.right-30,rect.bottom*1/3+35-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID2_ID)	,NULL,rect.right*2/5+25,rect.bottom*1/3+65+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID2_ID)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+65-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID2_ID)	,NULL,rect.right-30,rect.bottom*1/3+65-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID3_TYPE)	,NULL,rect.right*2/5+25,rect.bottom*1/3+95+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,CB_ID3_TYPE)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+95-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID3_TYPE)	,NULL,rect.right-30,rect.bottom*1/3+95-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID3_ID)	,NULL,rect.right*2/5+25,rect.bottom*1/3+125+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID3_ID)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+125-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID3_ID)	,NULL,rect.right-30,rect.bottom*1/3+125-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID4_TYPE)	,NULL,rect.right*2/5+25,rect.bottom*1/3+155+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,CB_ID4_TYPE)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+155-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID4_TYPE)	,NULL,rect.right-30,rect.bottom*1/3+155-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TX_ID4_ID)	,NULL,rect.right*2/5+25,rect.bottom*1/3+185+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,TB_ID4_ID)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+185-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
		SetWindowPos(GetDlgItem(w,IMG_ID4_ID)	,NULL,rect.right-30,rect.bottom*1/3+185-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		yPosAutoLock=215;
		if (gbEnableOption_ManualPutConfig && gbInternetManualPutConfig)
		{
			SetWindowPos(GetDlgItem(w,TX_PWD_GROUP)	,NULL,rect.right*2/5+25,rect.bottom*1/3+215+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,TB_PWD_GROUP)	,NULL,rect.right*2/5+25+120,rect.bottom*1/3+215-3+yOffset,rect.right*3/5-180,20,SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_PWD_GROUP),NULL,rect.right-30,rect.bottom*1/3+215-1+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			yPosAutoLock=245;
		}
		if (gbShowAutoLockOption) // ISSUE#180
		{
			SetWindowPos(GetDlgItem(w,CK_AUTO_LOCK)	,NULL,rect.right*2/5+25,rect.bottom*1/3+yPosAutoLock+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			SetWindowPos(GetDlgItem(w,IMG_AUTO_LOCK),NULL,rect.right-30,rect.bottom*1/3+yPosAutoLock-2+yOffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
	}
	HideConfigControls(w);
	if (wToRefresh==NULL)
		InvalidateRect(w,NULL,FALSE);
	else
		InvalidateRect(w,&rectTabConfig,FALSE);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// TVItemGetLParam()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int TVItemGetLParam(HWND w,HTREEITEM hItem)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	TVITEMEX tvItem; 
	if (hItem==NULL) goto end;
	tvItem.mask=TVIF_HANDLE | TVIF_PARAM;
	tvItem.hItem=hItem;
	if (!TreeView_GetItem(GetDlgItem(w,TV_APPLICATIONS),&tvItem)) { TRACE((TRACE_ERROR,_F_,"TreeView_GetItem()")); goto end; }
	rc=tvItem.lParam;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// TVItemSetLParam()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void TVItemSetLParam(HWND w,HTREEITEM hItem,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "%d",lp));

	TVITEMEX tvItem; 
	if (hItem==NULL) goto end;
	tvItem.mask=TVIF_HANDLE | TVIF_PARAM;
	tvItem.hItem=hItem;
	tvItem.lParam=lp;
	if (!TreeView_SetItem(GetDlgItem(w,TV_APPLICATIONS),&tvItem)) { TRACE((TRACE_ERROR,_F_,"TreeView_SetItem()")); goto end; }
	
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// TVItemGetText()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int TVItemGetText(HWND w,HTREEITEM hItem,char *pszText,int sizeofText)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	TVITEMEX tvItem; 
	
	if (hItem==NULL) goto end;
	*pszText=0;
	tvItem.mask=TVIF_HANDLE | TVIF_TEXT;
	tvItem.hItem=hItem;
	tvItem.pszText=pszText;
	tvItem.cchTextMax=sizeofText;
	if (!TreeView_GetItem(GetDlgItem(w,TV_APPLICATIONS),&tvItem)) { TRACE((TRACE_ERROR,_F_,"TreeView_GetItem()")); goto end; }
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d pszText=%s",rc,pszText));
	return rc;
}


//-----------------------------------------------------------------------------
// MoveApp()
//-----------------------------------------------------------------------------
// Déplace une application vers une autre catégorie
//-----------------------------------------------------------------------------
void MoveApp(HWND w,HTREEITEM hItem,int iNewCategoryIndex)
{
	TRACE((TRACE_ENTER,_F_, "iNewCategoryIndex=%d",iNewCategoryIndex));

	TVITEMEX tvItem; 
	HTREEITEM hNewItem;
	char szApplication[LEN_APPLICATION_NAME+1];
	int iGnored=-1;

	tvItem.mask=TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
	tvItem.hItem=hItem;
	tvItem.pszText=szApplication;
	tvItem.cchTextMax=sizeof(szApplication);
	if (!TreeView_GetItem(GetDlgItem(w,TV_APPLICATIONS),&tvItem)) { TRACE((TRACE_ERROR,_F_,"TreeView_GetItem()")); goto end; }
	if (tvItem.lParam>=giNbActions) { TRACE((TRACE_ERROR,_F_,"tvItem.lParam=%ld ! (giNbActions=%ld)",tvItem.lParam,giNbActions)); goto end; }

	TRACE((TRACE_INFO,_F_,"Moving action %d to categ index %d (id=%d)",tvItem.lParam,iNewCategoryIndex,gptCategories[iNewCategoryIndex].id));
	// modification de la catégorie dans la table des actions
	gptActions[tvItem.lParam].iCategoryId=gptCategories[iNewCategoryIndex].id;
	// ajout de l'élément avec sa nouvelle catégorie mais en conservant le titre existant (correction #99)
	hNewItem=TVAddApplication(w,tvItem.lParam,szApplication,FALSE);
	// effacement de l'élément dans son ancienne catégorie
	TreeView_DeleteItem(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hNewItem!=NULL) TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hNewItem);
	
	// ISSUE#206 : Mise à jour automatique sur le serveur quand une application est déplacée d’une catégorie à une autre
	if (gbCategoryManagement && gbCategoryAutoUpdate)
	{
		PutConfigOnServer(tvItem.lParam,&iGnored,"DONTCHANGE");
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// UploadConfig()
//-----------------------------------------------------------------------------
// Uploade la configuration (ou les configurations de la catégorie) sélectionnée sur le serveur
// Nouveau en 0.91 : le menu upload est proposée aussi sur les catégories, 
// auquel cas l'ensemble des configurations de la catégories sont remontées 
// sur le serveur
//-----------------------------------------------------------------------------
int UploadConfig(HWND w, char *pszDomainIds)
{
	TRACE((TRACE_ENTER,_F_, "pszDomainIds=%s",pszDomainIds));

	HTREEITEM hItem,hParentItem,hNextApp;
	int iAction,iCategoryId;
	int rc=-1;
	HCURSOR hCursorOld=NULL;
	char szMsg[255];
	int iNbConfigUploaded=0;
	int iNbConfigIgnored=0;
	int iOldCategoryId=-1;
	int iNewCategoryId=-1;
	
	hCursorOld=SetCursor(ghCursorWait);
	hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
	hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
	if (hParentItem==NULL) // c'est une catégorie 
	{
		iCategoryId=TVItemGetLParam(w,hItem);
		if (iCategoryId==-1) goto end;
		iOldCategoryId=iCategoryId;
		hNextApp=TreeView_GetChild(GetDlgItem(w,TV_APPLICATIONS),hItem);
		while(hNextApp!=NULL)
		{
			iAction=TVItemGetLParam(w,hNextApp); 
			if (iAction==-1 || iAction>=giNbActions) { rc=-1; goto end; }
			TRACE((TRACE_INFO,_F_,"Upload config n°%d (%s)",iAction,gptActions[iAction].szApplication));
			rc=PutConfigOnServer(iAction,&iNewCategoryId,pszDomainIds);
			TVUpdateItemState(w,hNextApp,iAction);
			if (rc==0) iNbConfigUploaded++;
			else if (rc==-2) { iNbConfigIgnored++; rc=0; }
			else goto end;
			hNextApp=TreeView_GetNextSibling(GetDlgItem(w,TV_APPLICATIONS),hNextApp);
		}
		sprintf_s(szMsg,sizeof(szMsg),GetString(IDS_MULTI_UPLOAD_OK),iNbConfigUploaded,iNbConfigIgnored);
	}
	else // c'est une application
	{
		iAction=TVItemGetLParam(w,hItem); 
		if (iAction==-1 || iAction>=giNbActions) goto end;

		GetApplicationDetails(w,iAction); // ISSUE#213

		TRACE((TRACE_INFO,_F_,"Upload config n°%d (%s)",iAction,gptActions[iAction].szApplication));
		iOldCategoryId=gptActions[iAction].iCategoryId;
		rc=PutConfigOnServer(iAction,&iNewCategoryId,pszDomainIds);
		TVUpdateItemState(w,hItem,iAction);
		strcpy_s(szMsg,sizeof(szMsg),GetString(IDS_UPLOAD_OK));
	}
	
	// 0.91 : il faut écrire le configId dans le .ini, sinon si l'utilisateur annule on le perd...
	//        mais il ne faut pas écrire betement (risque d'écrire dans une section renommée, cf. swSSOCOnfig.cpp)
	//        Je n'ai pas de meilleure idée qu'une sauvegarde complète (il n'est pas possible de retrouver
	//        de manière certaine la section à modifier)
	//        Il faut aussi écrire le bConfigSent à TRUE
	//        Il faut aussi écrire le categId... et aussi iWithIdPwd... du coup la sauvegarde complète va bien !
	SaveApplications();
	BackupAppsNcategs();

	// si le categ id a changé lors de l'upload, il faut le détecter pour mettre à jour l'item dans la listview
	// (tout le reste est bon en mémoire et dans le fichier swsso.ini)
	if (iNewCategoryId!=-1 && iNewCategoryId!=iOldCategoryId)
	{
		HTREEITEM hItemCategory=hParentItem==NULL?hItem:hParentItem;
		TVItemSetLParam(w,hItemCategory,iNewCategoryId);
	}

end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (rc==0)
		MessageBox(w,szMsg,"swSSO",MB_OK | MB_ICONINFORMATION);
	else 
		MessageBox(w,GetString(IDS_UPLOAD_NOK),"swSSO",MB_OK | MB_ICONEXCLAMATION);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
// ----------------------------------------------------------------------------------
// LoadApplicationIdX()
// ----------------------------------------------------------------------------------
// Chargement de la config des identifiants complémentaires 2 à 5
// ----------------------------------------------------------------------------------
int LoadApplicationIdX(char *szSection,int x, char *szIdName,int sizeofIdName, 
					   char *szIdValue, int sizeofIdValue,int *idType)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szIdType[5+1];
	char szItem[20+1];
	char szIdEncryptedValue[LEN_ENCRYPTED_AES256+1];
	
	*szIdName=0;
	*szIdValue=0;
	*idType=0;
	
	sprintf_s(szItem,sizeof(szItem),"id%dValue",x); 
	GetPrivateProfileString(szSection,szItem,"",szIdEncryptedValue,sizeof(szIdEncryptedValue),gszCfgFile);
	// 0.89 : c'est mieux de tout charger et on peut se le permettre maintenant
	// 0.90 : non, ça rame trop s'essayer de tout charger, même si ce n'est qu'une fois seulement au lancement
	//        donc on ne charge que si value non vide (le test fait précédemment sur name n'est pas bon
	//        car en KBSim on n'a pas de name et type, mais une value)
	if (*szIdEncryptedValue!=0)
	{
		sprintf_s(szItem,sizeof(szItem),"id%dName",x); 	
		GetPrivateProfileString(szSection,szItem,"",szIdName,sizeofIdName,gszCfgFile);
		sprintf_s(szItem,sizeof(szItem),"id%dType",x);	
		GetPrivateProfileString(szSection,szItem,"",szIdType,sizeof(szIdType),gszCfgFile);
		if (strcmp(szIdType,"EDIT")==0)			*idType=EDIT;
		else if (strcmp(szIdType,"COMBO")==0)	*idType=COMBO;
		else if (*szIdType==0)					*idType=0;
		else { TRACE((TRACE_ERROR,_F_, "id%dType (%s) : %s",x,szSection,szIdType)); goto end; }

		// déchiffrement immédiat de l'identifiant (on ne fait pas comme avec le mot de passe qui n'est déchiffré qu'à l'utilisation)
		if (strlen(szIdEncryptedValue)==LEN_ENCRYPTED_3DES || strlen(szIdEncryptedValue)==LEN_ENCRYPTED_AES256)
		{
			char *pszDecryptedValue=swCryptDecryptString(szIdEncryptedValue,ghKey1);
			if (pszDecryptedValue!=NULL) 
			{
				strcpy_s(szIdValue,sizeofIdValue,pszDecryptedValue);
				free(pszDecryptedValue);
			}
		}
		else
		{
			strcpy_s(szIdValue,sizeofIdValue,szIdEncryptedValue);
		}
		TRACE((TRACE_DEBUG,_F_,"id%dName=%s | id%dValue=%s | id%dType=%d",x,szIdName,x,szIdValue,x,*idType));
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
// ----------------------------------------------------------------------------------
// SaveApplicationIdX()
// ----------------------------------------------------------------------------------
// Sauvegarde des identifiants complémentaires 2 à 4
// ----------------------------------------------------------------------------------
int SaveApplicationIdX(HANDLE hf,int x,const char *szIdName,const char *szIdValue,int idType)
{
	TRACE((TRACE_ENTER,_F_, "id%d : szIdName=%s szIdValue=%s",x,szIdName,szIdValue));
	int rc=-1;
	DWORD dw;
	char tmpBuf[2048];
	char szIdEncryptedValue[LEN_ENCRYPTED_AES256+1];
	char szIdType[10+1]; // NONE | EDIT | COMBO

	// chiffrement de l'identifiant sauf si vide
	if (*szIdValue==0)
	{
		strcpy_s(szIdEncryptedValue,sizeof(szIdEncryptedValue),szIdValue);
	}
	else
	{
		char *pszEncrypted=swCryptEncryptString(szIdValue,ghKey1);
		if (pszEncrypted==NULL) // si erreur, on le sauve en clair, c'est toujours mieux que de le perdre !
		{
			strcpy_s(szIdEncryptedValue,sizeof(szIdEncryptedValue),szIdValue);
		}
		else
		{
			strcpy_s(szIdEncryptedValue,sizeof(szIdEncryptedValue),pszEncrypted);
			free(pszEncrypted);
		}
	}

	if (*szIdName!=0 || *szIdValue!=0)
	{
		if (idType==EDIT) strcpy_s(szIdType,"EDIT");
		else if (idType==COMBO) strcpy_s(szIdType,"COMBO");
		else *szIdType=0;

		sprintf_s(tmpBuf,sizeof(tmpBuf),
			"id%dName=%s\r\nid%dValue=%s\r\nid%dType=%s\r\n",
			x,szIdName,
			x,szIdEncryptedValue,
			x,szIdType);
		if (!WriteFile(hf,tmpBuf,strlen(tmpBuf),&dw,NULL)) 
		{ 
			TRACE((TRACE_ERROR,_F_,"WriteFile(%s), len=%d",gszCfgFile,strlen(tmpBuf))); goto end;
		}
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// LoadApplications()
// ----------------------------------------------------------------------------------
// Remplissage du tableau des actions par lecture du .ini
// Appelé au lancement de swSSO, puis lorsque l'utilisateur
// clique sur annuler (on recharge la config depuis le fichier)
// ----------------------------------------------------------------------------------
int LoadApplications(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szType[3+1];
	char szConfigId[12+1];
	char szWithIdPwd[4+1];
	DWORD dw;
	char *p;
	char *pSectionNames=NULL;
	int i;
	char szId1EncryptedValue[LEN_ENCRYPTED_AES256+1];
	int lenTitle;

	pSectionNames=(char*)malloc(giMaxConfigs*(LEN_APPLICATION_NAME+1)); 
	if (pSectionNames==NULL) { TRACE((TRACE_ERROR,_F_,"malloc (%d)",giMaxConfigs*(LEN_APPLICATION_NAME+1))); goto end; }

	// énumération des sections du .ini passé en ligne de commande
	dw=GetPrivateProfileSectionNames(pSectionNames,giMaxConfigs*(LEN_APPLICATION_NAME+1),gszCfgFile);
	if (dw==0) // rien lu ! Fichier vide ou non trouvé
	{
		rc=-2;
		goto end;
	}
	// premier tour pour compter le nombre de sections
	p=pSectionNames;
	giNbActions=0;
	while (*p!=0)
	{
		while(*p!=0) p++ ;
		giNbActions++;
		p++;
	}
	if (giNbActions>giMaxConfigs+2) // +2 car 2 sections hors sujet : swSSO et swSSO-Categories
	{
		TRACE((TRACE_ERROR,_F_,"giNbActions=%d > giMaxConfigs=%d",giNbActions,giMaxConfigs));
		goto end;
	}

	// second tour pour remplir le tableau d'actions
	p=pSectionNames;
	i=0;
	while (*p!=0)
	{
		gptActions[i].szId1Value[0]=0;
		gptActions[i].szId2Value[0]=0;
		gptActions[i].szId3Value[0]=0;
		gptActions[i].szId4Value[0]=0;
		TRACE((TRACE_DEBUG,_F_,"Lecture cle '%s'",p));
		if (strcmp(p,"swSSO")==0) { giNbActions-- ; i--; goto suite;}
		if (strcmp(p,"swSSO-Categories")==0) { giNbActions-- ; i--; goto suite;}
		strcpy_s(gptActions[i].szApplication,sizeof(gptActions[i].szApplication),p);
		GetPrivateProfileString(p,"id","",szConfigId,sizeof(szConfigId),gszCfgFile);
		gptActions[i].iConfigId=atoi(szConfigId);
		// ISSUE#58 0.93B7 : pertes du dernier caractère du titre sizeof(gptActions[i].szTitle)-1 remplacé par sizeof(gptActions[i].szTitle)
		lenTitle=GetPrivateProfileString(p,"title","",gptActions[i].szTitle,sizeof(gptActions[i].szTitle),gszCfgFile);
		// 0.92B3 : pour maintenir la compatibilité sur la comparaison du titre maintenant que le matching
		// se fait explicitement avec les caractères joker *, ajout systématique d'une * sur tous les titres
		// dans les anciens fichiers de config.
		if (*gszCfgVersion==0 || atoi(gszCfgVersion)<92) // ancienne version // version de fichier != 0.92
		{
			// ISSUE#58 0.93B7 : pertes du dernier caractère du titre, suite : positionne l'* au bon endroit
			int iPos=lenTitle==LEN_TITLE?iPos=LEN_TITLE-1:iPos=lenTitle;
			gptActions[i].szTitle[iPos]='*';
			gptActions[i].szTitle[iPos+1]=0;
		}
		GetPrivateProfileString(p,"URL","",gptActions[i].szURL,sizeof(gptActions[i].szURL),gszCfgFile);
		GetPrivateProfileString(p,"idName","",gptActions[i].szId1Name,sizeof(gptActions[i].szId1Name),gszCfgFile);
		GetPrivateProfileString(p,"idValue","",szId1EncryptedValue,sizeof(szId1EncryptedValue),gszCfgFile);
		// déchiffrement immédiat de l'identifiant (on ne fait pas comme avec le mot de passe qui n'est déchiffré qu'à l'utilisation)
		if (*szId1EncryptedValue!=0) 
		{
			if (strlen(szId1EncryptedValue)==LEN_ENCRYPTED_3DES || strlen(szId1EncryptedValue)==LEN_ENCRYPTED_AES256)
			{
				char *pszDecryptedValue=swCryptDecryptString(szId1EncryptedValue,ghKey1);
				if (pszDecryptedValue!=NULL) 
				{
					strcpy_s(gptActions[i].szId1Value,sizeof(gptActions[i].szId1Value),pszDecryptedValue);
					free(pszDecryptedValue);
				}
			}
			else
			{
				strcpy_s(gptActions[i].szId1Value,sizeof(gptActions[i].szId1Value),szId1EncryptedValue);
			}
		}
		TRACE((TRACE_DEBUG,_F_,"szId1Value=%s",gptActions[i].szId1Value));
		GetPrivateProfileString(p,"pwdName","",gptActions[i].szPwdName,sizeof(gptActions[i].szPwdName),gszCfgFile);
		// 0.50
		// 0.65B3 on ne déchiffre plus qu'à l'utilisation !
		GetPrivateProfileString(p,"pwdValue","",gptActions[i].szPwdEncryptedValue,sizeof(gptActions[i].szPwdEncryptedValue),gszCfgFile);
		if (*gptActions[i].szPwdEncryptedValue==0) 
		{
			TRACE((TRACE_INFO,_F_,"Mot de passe vide pour appli %s",gptActions[i].szApplication));
		}
		GetPrivateProfileString(p,"validateName","",gptActions[i].szValidateName,sizeof(gptActions[i].szValidateName),gszCfgFile);
		// 0.80 : renommage de <SANSNOM> en [SANSNOM] pour compatibilité XML...
		if (strcmp(gptActions[i].szValidateName,gcszFormNoName1)==0) strcpy_s(gptActions[i].szValidateName,sizeof(gptActions[i].szValidateName),gcszFormNoName2);
		// nouvelle IHM : identifiants des catégories
		gptActions[i].iCategoryId=GetPrivateProfileInt(p,"categoryId",0,gszCfgFile);
		TRACE((TRACE_INFO,_F_,"Config[%d] => categ=%d",i,gptActions[i].iCategoryId));
		/*
		gptActions[i].iDomainId=GetPrivateProfileInt(p,"domainId",1,gszCfgFile);
		TRACE((TRACE_INFO,_F_,"Config[%d] => domain=%d",i,gptActions[i].iDomainId));
		*/

		gptActions[i].bActive=GetConfigBoolValue(p,"active",FALSE,TRUE);
		gptActions[i].bAutoLock=GetConfigBoolValue(p,"autoLock",FALSE,TRUE);
		// gptActions[i].bConfigOK=GetConfigBoolValue(p,"configOK",FALSE); // 0.90B1 : on ne gère plus l'état OK car plus de remontée auto
		gptActions[i].bConfigSent=GetConfigBoolValue(p,"configSent",FALSE,TRUE);
		gptActions[i].bKBSim=GetConfigBoolValue(p,"UseKBSim",FALSE,TRUE);
		GetPrivateProfileString(p,"KBSimValue","",gptActions[i].szKBSim,sizeof(gptActions[i].szKBSim),gszCfgFile);
		if (gptActions[i].bKBSim && _strnicmp(gptActions[i].szKBSim,"[WAIT]",strlen("[WAIT]"))==0) 
			gptActions[i].bWaitForUserAction=TRUE;
		else
			gptActions[i].bWaitForUserAction=FALSE;

		LoadApplicationIdX(p,2,gptActions[i].szId2Name,sizeof(gptActions[i].szId2Name),gptActions[i].szId2Value,sizeof(gptActions[i].szId2Value),&gptActions[i].id2Type);
		LoadApplicationIdX(p,3,gptActions[i].szId3Name,sizeof(gptActions[i].szId3Name),gptActions[i].szId3Value,sizeof(gptActions[i].szId3Value),&gptActions[i].id3Type);
		LoadApplicationIdX(p,4,gptActions[i].szId4Name,sizeof(gptActions[i].szId4Name),gptActions[i].szId4Value,sizeof(gptActions[i].szId4Value),&gptActions[i].id4Type);

		// lecture du type (WIN | WEB | POP | XEB)
		GetPrivateProfileString(p,"type","",szType,sizeof(szType),gszCfgFile);
		if (strcmp(szType,"WIN")==0)		gptActions[i].iType=WINSSO;
		else if (strcmp(szType,"WEB")==0)	gptActions[i].iType=WEBSSO;
		else if (strcmp(szType,"XEB")==0)	gptActions[i].iType=XEBSSO;
		else if (strcmp(szType,"POP")==0)	gptActions[i].iType=POPSSO;
		else if (strcmp(szType,"UNK")==0)	gptActions[i].iType=UNK;
		else 
		{ 
			// 0.88 : suite au bug #89, c'est une bonne idée de ne pas être trop strict sur la lecture
			//        de la config. Du coup, si type appli inconnu, on la zappe mais on continue quand même !
			//	      { TRACE((TRACE_ERROR,_F_, "type appli inconnu (%s) : %s"),gptActions[i].szApplication,szType));	rc=-1; goto end; }
			TRACE((TRACE_ERROR,_F_, "type appli inconnu (%s) : %s",gptActions[i].szApplication,szType));	
			giNbActions--;
			i--; 
			goto suite;
		}
		GetPrivateProfileString(p,"fullPathName","",gptActions[i].szFullPathName,sizeof(gptActions[i].szFullPathName),gszCfgFile);
		GetPrivateProfileString(p,"lastUpload","",gptActions[i].szLastUpload,sizeof(gptActions[i].szLastUpload),gszCfgFile);
		//gptActions[i].tLastDetect=-1;
		//gptActions[i].wLastDetect=NULL;
		gptActions[i].tLastSSO=-1;
		gptActions[i].wLastSSO=NULL;
		gptActions[i].iWaitFor=WAIT_IF_SSO_OK;
		gptActions[i].bSaved=TRUE; // 0.93B6 ISSUE#55
		gptActions[i].iNbEssais=0; // 0.93B7
		gptActions[i].bAddAccount=GetConfigBoolValue(p,"addAccount",FALSE,TRUE); // 0.97 ISSUE#86
		GetPrivateProfileString(p,"bWithIdPwd","",szWithIdPwd,sizeof(szWithIdPwd),gszCfgFile);
		gptActions[i].iWithIdPwd=atoi(szWithIdPwd);
		gptActions[i].iPwdGroup=GetPrivateProfileInt(p,"pwdGroup",-1,gszCfgFile); // 1.03
		gptActions[i].bPwdChangeInfos=GetConfigBoolValue(p,"pwdChange",FALSE,FALSE);
#if 0
		// 0.9X : gestion des changements de mot de passe sur les pages web
		if (gptActions[i].bPwdChangeInfos)
		{
			TRACE((TRACE_INFO,_F_,"Une page de changement de mot de passe est déclarée pour cette application :"));
			// PROTO : on crée une config supplémentaire qui ne contient que les éléments pour le changement de mot de passe
			i++;
			giNbActions++;
			ZeroMemory(&gptActions[i],sizeof(T_ACTION));
			gptActions[i].iType=WEBPWD;
			// TODO :trouver une bonne solution pour nommer la config supplémentaire (penser à l'ajout après coup... comment faire puisqu'elle ne sera jamais
			//        juste après...il vaudrait mieux avoir une espèce de pointeur.
			GetPrivateProfileString("_pwd_change","pwdTitle","",gptActions[i].szTitle,sizeof(gptActions[i].szTitle),gszCfgFile);
			GetPrivateProfileString("_pwd_change","pwdURL","",gptActions[i].szURL,sizeof(gptActions[i].szURL),gszCfgFile);
			GetPrivateProfileString("_pwd_change","pwdCurrent","",gptActions[i].szId1Name,sizeof(gptActions[i].szId1Name),gszCfgFile);
			GetPrivateProfileString("_pwd_change","pwdNew1","",gptActions[i].szId2Name,sizeof(gptActions[i].szId2Name),gszCfgFile);
			GetPrivateProfileString("_pwd_change","pwdNew2","",gptActions[i].szId3Name,sizeof(gptActions[i].szId3Name),gszCfgFile);
			gptActions[i].id2Type=EDIT;
			gptActions[i].id3Type=EDIT;
			GetPrivateProfileString("_pwd_change","pwdValidate","",gptActions[i].szValidateName,sizeof(gptActions[i].szValidateName),gszCfgFile);

			TRACE((TRACE_INFO,_F_,"pwdTitle   =%s",gptActions[i].szTitle));
			TRACE((TRACE_INFO,_F_,"pwdURL     =%s",gptActions[i].szURL));
			TRACE((TRACE_INFO,_F_,"pwdCurrent =%s",gptActions[i].szId1Name));
			TRACE((TRACE_INFO,_F_,"pwdNew1    =%s",gptActions[i].szId2Name));
			TRACE((TRACE_INFO,_F_,"pwdNew2    =%s",gptActions[i].szId3Name));
			TRACE((TRACE_INFO,_F_,"pwdValidate=%s",gptActions[i].szValidateName));

			gptActions[i].iPwdLen=10;

			// reprise des valeurs de la clé mère
			gptActions[i].bActive=gptActions[i-1].bActive;
			gptActions[i].bAutoLock=gptActions[i-1].bAutoLock;
			strcpy_s(gptActions[i].szApplication,sizeof(gptActions[i].szApplication),gptActions[i-1].szApplication);
			
			// autres initialisations...
			gptActions[i].bKBSim=FALSE;
			gptActions[i].bWaitForUserAction=FALSE;
			//gptActions[i].tLastDetect=-1;
			//gptActions[i].wLastDetect=NULL;
			gptActions[i].tLastSSO=-1;
			gptActions[i].wLastSSO=NULL;
			gptActions[i].iWaitFor=WAIT_IF_SSO_OK;
			gptActions[i].bSaved=TRUE;
			gptActions[i].bPwdChangeInfos=FALSE;
			gptActions[i].iNbEssais=0;
		}
#endif
suite:
		while(*p!=0) p++ ;
		i++;
		p++;
	}
	rc=0;
end:
	if (pSectionNames!=NULL) free(pSectionNames);

	// 0.92B3 (cf. plus haut ajout caractère joker dans le titre)
	if (*gszCfgVersion==0 || atoi(gszCfgVersion)<92) 
	{
		SaveConfigHeader();
		SaveApplications();
	}

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// SaveApplications()
// ----------------------------------------------------------------------------------
// Sauvegarde de la config des applications
// ----------------------------------------------------------------------------------
int SaveApplications(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szType[3+1];
	char szWithIdPwd[4+1];
	int i;
	DWORD dw;
	HANDLE hf=INVALID_HANDLE_VALUE;
	char *p;
	char *pszHeader=NULL;
	char tmpBuf[2048];
	char szId1EncryptedValue[LEN_ENCRYPTED_AES256+1];

	// ouvre le fichier en lecture pour récupérer le header complet (un peu bidouille mais bon...)
	hf=CreateFile(gszCfgFile,GENERIC_READ,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) 
	{
		TRACE((TRACE_ERROR,_F_,"CreateFile(OPEN_ALWAYS,%s)",gszCfgFile)); goto end;
	}
	pszHeader=(char*)malloc(16384); // 16Ko, ça devrait suffire...
	if (pszHeader==NULL) { TRACE((TRACE_ERROR,_F_,"malloc (16384)")); goto end; }

	if (!ReadFile(hf,pszHeader,16383,&dw,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%s)",gszCfgFile)); goto end;
	}
	TRACE((TRACE_INFO,_F_,"ReadFile(%s) : %ld octets lus",gszCfgFile,dw));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pszHeader,dw,"Lu :"));
	pszHeader[dw]=0;
	p=strchr(pszHeader+10,'[');
	if (p!=NULL) *(p-2)=0; // tronque le buffer lu à la taille du header (si p==NULL, y'a que le header, on garde tout)
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pszHeader,strlen(pszHeader),"Header (len=%d):",strlen(pszHeader)));

	// 0.85B6 : flush pour être sûr que le fichier est bien écrit sur le disque 
	//          avant de passer à la suite (peut-être une cause d'un bug vu 1 fois...
	//          où l'en-tête du fichier a été perdue...
	FlushFileBuffers(hf);

	CloseHandle(hf); hf=INVALID_HANDLE_VALUE;

	// ouvre le fichier en écriture et inscrit le header puis les applications
	hf=CreateFile(gszCfgFile,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) 
	{
		TRACE((TRACE_ERROR,_F_,"CreateFile(CREATE_ALWAYS,%s)",gszCfgFile)); goto end;
	}
	if (!WriteFile(hf,pszHeader,strlen(pszHeader),&dw,NULL)) 
	{ 
		TRACE((TRACE_ERROR,_F_,"WriteFile(%s), len=%d",gszCfgFile,strlen(pszHeader))); goto end;
	}
	TRACE((TRACE_INFO,_F_,"WriteFile(%s) : %ld octets ecrits",gszCfgFile,dw));
	// Applications
	for (i=0;i<giNbActions;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"Ecriture cle n°%d='%s'",i,gptActions[i].szApplication));

		// si l'action est de type WEBPWD, on ne devrait pas la voir puisqu'elle est 
		// écrite avec sa config mère (n-1). Si on la voit, ça veut donc dire que sa
		// config mère n'existe plus : le plus simple SEMBLE être de ne pas la sauvegarder
		// plutôt que d'essayer de la supprimer en mémoire. J'aime bien cette idée parce
		// que le code de TVRemoveSelectedAppOrCateg n'est vraiment pas simple et je pense
		// qu'il vaut mieux éviter d'essayer de le modifier.
		// Attention quand même, ça veut dire qu'une config fille peut exister en mémoire sans sa mère...
		// du coup si on veut s'en servir, on va chercher à récupérer le mot de passe
		// dans la config n-1 qui n'est plus la bonne... 
		// CONCLUSION : TODO = la supprimer dans TVRemoveSelectedAppOrCateg !
		if (gptActions[i].iType==WEBPWD) 
		{
			TRACE((TRACE_INFO,_F_,"Configuration mère supprimée, on ne sauvegarde pas la configuration fille"));
			continue;
		}
		if (gptActions[i].iType==WINSSO)
			strcpy_s(szType,sizeof(szType),"WIN");
		else if (gptActions[i].iType==WEBSSO)
			strcpy_s(szType,sizeof(szType),"WEB");
		else if (gptActions[i].iType==XEBSSO)
			strcpy_s(szType,sizeof(szType),"XEB");
		else if (gptActions[i].iType==POPSSO)
			strcpy_s(szType,sizeof(szType),"POP");
		else
			strcpy_s(szType,sizeof(szType),"UNK");

		if (gptActions[i].bKBSim && _strnicmp(gptActions[i].szKBSim,"[WAIT]",strlen("[WAIT]"))==0) 
			gptActions[i].bWaitForUserAction=TRUE;
		else
			gptActions[i].bWaitForUserAction=FALSE;

		// 0.80 : renommage de <SANSNOM> en [SANSNOM] pour compatibilité XML...
		if (strcmp(gptActions[i].szValidateName,gcszFormNoName1)==0) strcpy_s(gptActions[i].szValidateName,sizeof(gptActions[i].szValidateName),gcszFormNoName2);

		// chiffrement de l'identifiant sauf si vide
		if (*gptActions[i].szId1Value==0)
		{
			strcpy_s(szId1EncryptedValue,sizeof(szId1EncryptedValue),gptActions[i].szId1Value);
		}
		else
		{
			char *pszEncrypted=swCryptEncryptString(gptActions[i].szId1Value,ghKey1);
			if (pszEncrypted==NULL) // si erreur, on le sauve en clair, c'est toujours mieux que de le perdre !
			{
				strcpy_s(szId1EncryptedValue,sizeof(szId1EncryptedValue),gptActions[i].szId1Value);
			}
			else
			{
				strcpy_s(szId1EncryptedValue,sizeof(szId1EncryptedValue),pszEncrypted);
				free(pszEncrypted);
			}
		}
		// pour compatibilité : il faut avoir NO si 0, ensuite valeurs numériques OK
		if (gptActions[i].iWithIdPwd==0)
			strcpy_s(szWithIdPwd,sizeof(szWithIdPwd),"NO");
		else
			sprintf_s(szWithIdPwd,sizeof(szWithIdPwd),"%d",gptActions[i].iWithIdPwd);
		// le plus beau sprintf de ma carrière... en espérant que le buffer soit assez grand ;-(
		sprintf_s(tmpBuf,sizeof(tmpBuf),
			"\r\n[%s]\r\nId=%d\r\ncategoryId=%d\r\ntitle=%s\r\nURL=%s\r\nidName=%s\r\nidValue=%s\r\npwdName=%s\r\npwdValue=%s\r\nvalidateName=%s\r\ntype=%s\r\nactive=%s\r\nautoLock=%s\r\nconfigSent=%s\r\nuseKBSim=%s\r\nKBSimValue=%s\r\nfullPathName=%s\r\nlastUpload=%s\r\naddAccount=%s\r\nbWithIdPwd=%s\r\npwdGroup=%d\r\n", //pwdChange=%s\r\n",
			gptActions[i].szApplication,
			gptActions[i].iConfigId,
			gptActions[i].iCategoryId,
			// gptActions[i].iDomainId,
			gptActions[i].szTitle,
			gptActions[i].szURL,
			gptActions[i].szId1Name,
			szId1EncryptedValue,
			gptActions[i].szPwdName,
			gptActions[i].szPwdEncryptedValue,
			gptActions[i].szValidateName,
			szType,
			gptActions[i].bActive?"YES":"NO",
			gptActions[i].bAutoLock?"YES":"NO",
			// gptActions[i].bConfigOK?"YES":"NO", // 0.90B1 : on ne gère plus l'état OK car plus de remontée auto
			gptActions[i].bConfigSent?"YES":"NO",
			gptActions[i].bKBSim?"YES":"NO",
			gptActions[i].szKBSim,
			gptActions[i].szFullPathName,
			*(gptActions[i].szLastUpload)==0?"":gptActions[i].szLastUpload,
			gptActions[i].bAddAccount?"YES":"NO", // 0.97 ISSUE#86
			szWithIdPwd, // 1.03 + 1.05
			gptActions[i].iPwdGroup); // 1.03
			//,	gptActions[i].bPwdChangeInfos?"YES":"NO");
		if (!WriteFile(hf,tmpBuf,strlen(tmpBuf),&dw,NULL)) 
		{ 
			TRACE((TRACE_ERROR,_F_,"WriteFile(%s), len=%d",gszCfgFile,strlen(tmpBuf))); goto end;
		}
		// 080B9 : avaient été oubliés les id*Name, id*Value et id*Type...
		// 080B9 : + nouveau, on va jusqu'à 4
		SaveApplicationIdX(hf,2,gptActions[i].szId2Name,gptActions[i].szId2Value,gptActions[i].id2Type);
		SaveApplicationIdX(hf,3,gptActions[i].szId3Name,gptActions[i].szId3Value,gptActions[i].id3Type);
		SaveApplicationIdX(hf,4,gptActions[i].szId4Name,gptActions[i].szId4Value,gptActions[i].id4Type);
		gptActions[i].bSaved=TRUE; // 0.93B6 ISSUE#55 (puis en 0.94B3 cette ligne a été remontée, elle était après le if bPwdChangeInfos avant)
		// 0.9X : sauvegarde des infos pour changement de mot de passe web
		if (gptActions[i].bPwdChangeInfos) // la config N+1 contient les éléments pour le chgt de mdp
		{
			i++; // j'aime pas bien faire un i++ dans un for... mais bon, là je ne vois pas mieux.
			sprintf_s(tmpBuf,sizeof(tmpBuf),
				"pwdTitle=%s\r\npwdURL=%s\r\npwdCurrent=%s\r\npwdNew1=%s\r\npwdNew2=%s\r\npwdValidate=%s\r\npwdLen=%d\r\n",
				gptActions[i].szTitle,
				gptActions[i].szURL,
				gptActions[i].szId1Name,
				gptActions[i].szId2Name,
				gptActions[i].szId3Name,
				gptActions[i].szValidateName,
				gptActions[i].iPwdLen);
			if (!WriteFile(hf,tmpBuf,strlen(tmpBuf),&dw,NULL)) 
			{ 
				TRACE((TRACE_ERROR,_F_,"WriteFile(%s), len=%d",gszCfgFile,strlen(tmpBuf))); goto end;
			}
			gptActions[i].bSaved=TRUE;
		}
		
	}
	// 0.85B6 : flush pour être sûr que le fichier est bien écrit sur le disque 
	FlushFileBuffers(hf);
	CloseHandle(hf); hf=INVALID_HANDLE_VALUE;
	StoreIniEncryptedHash(); // ISSUE#164
	SaveCategories();
	rc=0;
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf);
	if (pszHeader!=NULL) free(pszHeader);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// AppNsitesDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de config des applications et sites 
//-----------------------------------------------------------------------------
static int CALLBACK AppNsitesDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	// TRACE((TRACE_DEBUG,_F_,"msg=0x%08lx LOWORD(wp)=0x%04x HIWORD(wp)=%d lp=%d",msg,LOWORD(wp),HIWORD(wp),lp));
	int rc=FALSE;
	switch (msg)
	{
		case WM_APP+1: // gestion du tri après renommage (v0.88-#58)
			HTREEITEM hParentItem;
			hParentItem=(HTREEITEM)lp;
			TreeView_SortChildren(GetDlgItem(w,TV_APPLICATIONS),hParentItem,FALSE);
			break;
		case WM_INITDIALOG:	// ---------------------------------------------------------------------- WM_INITDIALOG
			{
				T_APPNSITES *ptAppNsites=(T_APPNSITES*)lp;
				TRACE((TRACE_DEBUG,_F_,"iSelected=%d bFromSysTray=%d",ptAppNsites->iSelected,ptAppNsites->bFromSystray));
				OnInitDialog(w,ptAppNsites);
				MoveControls(w,NULL); // 0.90, avant il y avait (w,w), correction bug#102
				if (ptAppNsites->bFromSystray) BackupAppsNcategs(); // #118 => le Backup est maintenant fait plus tôt (avant création nouvelle application)
																	//         donc il ne faut pas le refaire ici (voir swSSOConfig.cpp)
			}
			break;
		case WM_DESTROY:
			if (gbPwdSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD),(SUBCLASSPROC)PwdProc,TB_PWD_SUBCLASS_ID);
			if (gbPwdClearSubClass) RemoveWindowSubclass(GetDlgItem(w,TB_PWD_CLEAR),(SUBCLASSPROC)PwdProc,TB_PWD_CLEAR_SUBCLASS_ID);
			if (ghTabBrush!=NULL) { DeleteObject(ghTabBrush); ghTabBrush=NULL; }
			TermTooltip(); // ISSUE#111
			break;
		case WM_HELP:
			Help();
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (SetForeground)"));
			if (giSetForegroundTimer==(int)wp) 
			{ 
				KillTimer(w,giSetForegroundTimer);
				SetForegroundWindow(w); 
				SetFocus(w); 
			}
			break;
		case WM_COMMAND:		// ------------------------------------------------------- WM_COMMAND
			TRACE((TRACE_DEBUG,_F_,"WM_COMMAND LOWORD(wp)=0x%04x HIWORD(wp)=%d lp=%d",LOWORD(wp),HIWORD(wp),lp));
			// ISSUE#114
			if (!IsWindowEnabled(GetDlgItem(w,IDAPPLY)) & !gbIsChanging)
			{
				if ((HIWORD(wp)==EN_CHANGE) || (HIWORD(wp)==CBN_SELCHANGE))
				{
					EnableWindow(GetDlgItem(w,IDAPPLY),TRUE);
				}
			}
			switch (LOWORD(wp))
			{
				case IDOK:
					if (HIWORD(wp)==0) // les notifications autres que "from a control" ne nous intéressent pas !
					{
						if (gbTVApplicationsLabelEditing)
						{
							TreeView_EndEditLabelNow(GetDlgItem(w,TV_APPLICATIONS),FALSE);
							rc=TRUE;
						}
						else if (gbTVDomainsLabelEditing)
						{
							TreeView_EndEditLabelNow(GetDlgItem(w,TV_DOMAINS),FALSE);
							rc=TRUE;
						}
						else
						{
							HTREEITEM hItem;
							HTREEITEM hParentItem=NULL;
							HCURSOR hCursorOld=SetCursor(ghCursorWait);
							hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
							if (hItem!=NULL) hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
							if (hParentItem!=NULL) 
							{
								LPARAM lp=TVItemGetLParam(w,hItem);
								if (lp!=-1) GetApplicationDetails(w,lp);
							}
							//if (gbAtLeastOneAppRenamed) UpdateActionsTitleFromTreeview(w);// 0.90B1 : renommage direct, flag inutile
							SaveWindowPos(w);
							SaveApplications();
							SavePortal();
							if (hCursorOld!=NULL) SetCursor(hCursorOld);
							EndDialog(w,IDOK);
						}
					}
					break;
				case IDAPPLY:
					if (HIWORD(wp)==0) // les notifications autres que "from a control" ne nous intéressent pas !
					{
						HTREEITEM hItem;
						HTREEITEM hParentItem=NULL;
						HCURSOR hCursorOld=SetCursor(ghCursorWait);
						hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
						if (hItem!=NULL) hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),hItem);
						if (hParentItem!=NULL) 
						{
							LPARAM lp=TVItemGetLParam(w,hItem);
							if (lp!=-1) GetApplicationDetails(w,lp);
							TVUpdateItemState(w,hItem,lp);
						}
						//if (gbAtLeastOneAppRenamed) UpdateActionsTitleFromTreeview(w);// 0.90B1 : renommage direct, flag inutile
						SaveApplications();
						SavePortal();
						BackupAppsNcategs();
						EnableWindow(GetDlgItem(w,IDAPPLY),FALSE); // ISSUE#114
						if (hCursorOld!=NULL) SetCursor(hCursorOld);
					}
					break;
				case IDCANCEL:
					if (HIWORD(wp)==0) // les notifications autres que "from a control" ne nous intéressent pas !
					{
						if (gbTVApplicationsLabelEditing)
						{
							TreeView_EndEditLabelNow(GetDlgItem(w,TV_APPLICATIONS),TRUE);
							rc=TRUE;
						}
						else if (gbTVDomainsLabelEditing)
						{
							TreeView_EndEditLabelNow(GetDlgItem(w,TV_DOMAINS),TRUE);
							rc=TRUE;
						}
						else
						{
							HCURSOR hCursorOld=SetCursor(ghCursorWait);
							if (RestoreAppsNcategs()!=0) // restauration mémoire
							{
								// si échec, rechargement depuis fichier
								LoadCategories();
								LoadApplications();
							}
							SaveWindowPos(w);
							if (hCursorOld!=NULL) SetCursor(hCursorOld);
							EndDialog(w,IDCANCEL);
						}
					}
					break;
				case MENU_ACTIVER:
					TVActivateSelectedAppOrCateg(w,ACTIVATE_YES);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_DESACTIVER:
					TVActivateSelectedAppOrCateg(w,ACTIVATE_NO);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_RENOMMER:
					{
						TRACE((TRACE_DEBUG,_F_,"MENU_RENOMMER"));
						HTREEITEM hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
						if (hItem!=NULL) TreeView_EditLabel(GetDlgItem(w,TV_APPLICATIONS),hItem);
					}
					break;
				case MENU_RENOMMER_DOMAINE:
					{
						TRACE((TRACE_DEBUG,_F_,"MENU_RENOMMER_DOMAINE"));
						HTREEITEM hItem=TreeView_GetSelection(GetDlgItem(w,TV_DOMAINS));
						if (hItem!=NULL) TreeView_EditLabel(GetDlgItem(w,TV_DOMAINS),hItem);
					}
					break;
				case MENU_SUPPRIMER:
					TVRemoveSelectedAppOrCateg(w);
					if (!gbIsChanging && !gbAdminDeleteConfigsOnServer) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_SUPPRIMER_DOMAINE:
					DeleteDomain(w);
					break;
				case MENU_DUPLIQUER:
					TVDuplicateSelectedApp(w,FALSE);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_AJOUTER_COMPTE:
					TVDuplicateSelectedApp(w,TRUE);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_AJOUTER_APPLI:
					NewApplication(w,GetString(IDS_NEW_APP),TRUE);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_AJOUTER_CATEG:
					NewCategory(w);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_AJOUTER_DOMAINE:
					AddDomain(w);
					break;
				case MENU_CHANGER_IDS:
					ChangeCategIds(w);
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case MENU_LANCER_APPLI:
					LaunchSelectedApp(w);
					break;
				case MENU_PUBLISH:
					UploadConfig(w,"1");
					EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),FALSE); // ISSUE#114 (upload sauvegarde donc il faut griser Apply)
					break;
				case MENU_PUBLISH_TO:
					if (PublishTo(gwAppNsites)==0) EnableWindow(GetDlgItem(gwAppNsites,IDAPPLY),FALSE); // ISSUE#114 (upload sauvegarde donc il faut griser Apply)
					break;
				case IMG_LOUPE:
					if (HIWORD(wp)==0) TogglePasswordField(w);
					break;
				case CB_TYPE: 
					if (HIWORD(wp)==CBN_SELCHANGE)
					{
						int iType=SendMessage(GetDlgItem(w,CB_TYPE),CB_GETCURSEL,0,0);
						EnableControls(w,iType,TRUE);
					}
					break;
				case CK_KBSIM:
					MoveControls(w,GetDlgItem(w,TAB_CONFIG)); // marche pas : GetDlgItem(w,TAB_CONFIG)
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					break;
				case CK_AUTO_LOCK:
					if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE);
					break;
				case CK_AD_ID:
					if (IsDlgButtonChecked(w,CK_AD_ID))
					{
						SetDlgItemText(w,TB_ID,gcszUsername);
						EnableWindow(GetDlgItem(w,TB_ID),FALSE);
					}
					else
					{	
						SetDlgItemText(w,TB_ID,"");
						EnableWindow(GetDlgItem(w,TB_ID),TRUE);
					}
					break;
				case CK_AD_PWD:
					if (IsDlgButtonChecked(w,CK_AD_PWD))
					{
						SetDlgItemText(w,TB_PWD,gcszADPassword);
						SetDlgItemText(w,TB_PWD_CLEAR,gcszADPassword);
						EnableWindow(GetDlgItem(w,TB_PWD),FALSE);
						EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),FALSE);
					}
					else
					{
						SetDlgItemText(w,TB_PWD,"");
						SetDlgItemText(w,TB_PWD_CLEAR,"");
						EnableWindow(GetDlgItem(w,TB_PWD),TRUE);
						EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),TRUE);
					}
					break;
				case TB_ID:
					if (gbAdmin)
					{
						char szId[LEN_ID+1];
						GetDlgItemText(w,TB_ID,szId,sizeof(szId));
						if (strcmp(szId,gcszUsername)==0)
						{
							CheckDlgButton(w,CK_AD_ID,BST_CHECKED);
							EnableWindow(GetDlgItem(w,TB_ID),FALSE);
						}
						else
						{
							CheckDlgButton(w,CK_AD_ID,BST_UNCHECKED);
							EnableWindow(GetDlgItem(w,TB_ID),TRUE);
						}
					}
					break;
				case TB_PWD:
					if (gbAdmin)
					{
						char szPwd[LEN_PWD+1];
						GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
						if (strcmp(szPwd,gcszADPassword)==0)
						{
							CheckDlgButton(w,CK_AD_PWD,BST_CHECKED);
							EnableWindow(GetDlgItem(w,TB_PWD),FALSE);
						}
						else
						{
							CheckDlgButton(w,CK_AD_PWD,BST_UNCHECKED);
							EnableWindow(GetDlgItem(w,TB_PWD),TRUE);
						}
					}
					break;
				case TB_PWD_CLEAR:
					if (gbAdmin)
					{
						char szPwd[LEN_PWD+1];
						GetDlgItemText(w,TB_PWD_CLEAR,szPwd,sizeof(szPwd));
						if (strcmp(szPwd,gcszADPassword)==0)
						{
							CheckDlgButton(w,CK_AD_PWD,BST_CHECKED);
							EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),FALSE);
						}
						else
						{
							CheckDlgButton(w,CK_AD_PWD,BST_UNCHECKED);
							EnableWindow(GetDlgItem(w,TB_PWD_CLEAR),TRUE);
						}
					}
					break;
				case PB_PARCOURIR:
					{
						OPENFILENAME ofn;
						char szFile[LEN_FULLPATHNAME+1];
						*szFile=0;
						ZeroMemory(&ofn,sizeof(OPENFILENAME));
						ofn.lStructSize=sizeof(OPENFILENAME);
						ofn.hwndOwner=w;
						ofn.hInstance=NULL;
						//ofn.lpstrFilter="*.*";
						ofn.lpstrCustomFilter=NULL;
						ofn.nMaxCustFilter=0;
						ofn.nFilterIndex=0;
						ofn.lpstrFile=szFile;
						ofn.nMaxFile=sizeof(szFile);
						ofn.lpstrFileTitle=NULL;
						ofn.nMaxFileTitle=NULL;
						ofn.lpstrInitialDir=NULL;
						ofn.lpstrTitle=NULL;
						ofn.Flags=OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
						//ofn.nFileOffset;
						//ofn.lpstrDefExt;
						if (GetOpenFileName(&ofn)) 
						{
							SetDlgItemText(w,TB_LANCEMENT,szFile);
							if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
						}
					}
					break;

				default:
					if (LOWORD(wp)>=SUBMENU_CATEG && LOWORD(wp)<SUBMENU_CATEG+100) 
					{
						TRACE((TRACE_DEBUG,_F_,"MENU_DEPLACER vers %d",LOWORD(wp)-SUBMENU_CATEG));
						HTREEITEM hItem=TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS));
						MoveApp(w,hItem,LOWORD(wp)-SUBMENU_CATEG);
						if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					}
					break;
			}
			break;
		case WM_SIZE:			// ------------------------------------------------------- WM_SIZE
			MoveControls(w,NULL); // 0.90, avant il y avait (w,w), correction bug#102
			break;
		case WM_SIZING: // ISSUE#116
			{
				RECT *pRectNewSize=(RECT*)lp;
				TRACE((TRACE_DEBUG,_F_,"RectNewSize=%d,%d,%d,%d",pRectNewSize->top,pRectNewSize->left,pRectNewSize->bottom,pRectNewSize->right));
				if (pRectNewSize->right-pRectNewSize->left < 560)  pRectNewSize->right=pRectNewSize->left+560;
				if (pRectNewSize->bottom-pRectNewSize->top < 540+(gbAdmin?50:0))  pRectNewSize->bottom=pRectNewSize->top+540+(gbAdmin?50:0);
				rc=TRUE;
			}
			break;
		case WM_CTLCOLORSTATIC:	// ------------------------------------------------------- WM_CTLCOLORSTATIC
			switch(GetDlgCtrlID((HWND)lp))
			{
				case TX_ID:
				case TX_PWD:
				case TX_TYPE:
				case TX_ID2:
				case TX_ID3:
				case TX_ID4:
				case TX_ID2_ID:
				case TX_ID3_ID:
				case TX_ID4_ID:
				case TX_PWD_GROUP:
				case TX_ID2_TYPE:
				case TX_ID3_TYPE:
				case TX_ID4_TYPE:
				case TX_TITRE:
				case TX_URL:
				case TX_ID_ID:
				case TX_PWD_ID:
				case TX_VALIDATION:
				case TX_LANCEMENT:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
				case TX_MODE_ADMIN: 
					if (gbAdmin)
					{
						SetTextColor((HDC)wp,RGB(255,255,255));
						SetBkMode((HDC)wp,TRANSPARENT);
						rc=(int)ghRedBrush;
					}
					break;
				case CK_AD_ID:
				case CK_AD_PWD:
				case CK_KBSIM:
				case CK_AUTO_LOCK:
				{
					// Si pas déjà fait, création de la BRUSH pour peinture arrière plan des CHECKBOX...
					// Comment ça marche : on récupère la couleur du pixel juste à côté de la check box,
					// qui correspond à la couleur du fond de l'onglet. Ca aurait été bcp plus simple si la
					// transparence des checkbox fonctionnait comme celle des statics (HOLLOW_BRUSH) !!!
					// La couleur sert à créer une BRUSH qu'on retourne en réponse à WM_CTLCOLORSTATIC
					// La BRUSH est libérée dans WM_DESTROY
					COLORREF color;
					if (ghTabBrush==NULL) 
					{
						RECT tabRect,ckRect;
						GetWindowRect(GetDlgItem(w,CK_KBSIM),&ckRect);
						GetWindowRect(GetDlgItem(w,TAB_CONFIG),&tabRect);
						color=GetPixel(GetDC(GetDlgItem(w,TAB_CONFIG)),ckRect.left-tabRect.left-1,ckRect.top-tabRect.top);
						// ISSUE#68 - 0.94B4 pour le cas où parfois la checkbox est toute noire -> correction OK seulement pour XP
						// ISSUE#71 - 0.95B1 -> correction mieux faite, devrait fonctionner avec tous les OS
						if (color==CLR_INVALID)
						{
							ghTabBrush=NULL;
							TRACE((TRACE_INFO,_F_,"Cas du pixel noir... évité :-)"));
						}
						else
						{
							ghTabBrush=CreateSolidBrush(color);
						}
					}
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)ghTabBrush;
					break;
				}
			}			
			break;
		case WM_NOTIFY:			// ------------------------------------------------------- WM_NOTIFY
			// TRACE((TRACE_DEBUG,_F_,"WM_NOTIFY : NMHDR.hwndFrom=0x%08lx NMHDR.idFrom=0x%08lx NMHDR.code=0x%08lx",((NMHDR*)lp)->hwndFrom,((NMHDR*)lp)->idFrom,((NMHDR*)lp)->code));
			switch (((NMHDR FAR *)lp)->code) 
			{
				case TCN_SELCHANGE:
					MoveControls(w,NULL); // 0.90, avant il y avait (w,w), correction bug#102
					break;
				case TVN_SELCHANGED:
					if (wp==TV_APPLICATIONS && !gbEffacementEnCours)
					{
						HTREEITEM hParentItem;
						LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lp;
						// récupération des modifs éventuelles de l'appli précédemment sélectionnée
						hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),pnmtv->itemOld.hItem);
						if (hParentItem!=NULL)
						{
							GetApplicationDetails(w,pnmtv->itemOld.lParam);
							TVUpdateItemState(w,pnmtv->itemOld.hItem,pnmtv->itemOld.lParam);
						}
						// affichage des infos de l'appli nouvellement sélectionnée
						hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),pnmtv->itemNew.hItem);
						if (hParentItem==NULL) // c'est une catégorie 
						{
							ClearApplicationDetails(w);
							giLastApplicationConfig=-1; // ISSUE#152
							TRACE((TRACE_DEBUG,_F_,"pnmtv->itemNew.lParam=%ld",pnmtv->itemNew.lParam));
							if (GetKeyState(VK_SHIFT) & 0x8000)
							{
								int iCategIndex=GetCategoryIndex(pnmtv->itemNew.lParam);
								if (iCategIndex!=-1)
								{
									wsprintf(buf2048,"swSSO [id=%ld | i=%ld]",gptCategories[iCategIndex].id,iCategIndex);
									SetWindowText(w,buf2048);
								}
							}
						}
						else
						{
							ShowApplicationDetails(w,pnmtv->itemNew.lParam);
						}
					}
					else if (wp==TV_DOMAINS)
					{
						if (GetKeyState(VK_SHIFT) & 0x8000)
						{
							HTREEITEM hItem;
							int iDomain;
							hItem=TreeView_GetSelection(GetDlgItem(w,TV_DOMAINS));
							iDomain=TVItemGetLParam(w,hItem);
							if (iDomain!=-1) 
							{
								wsprintf(buf2048,"swSSO [id=%d]",gtabDomains[iDomain].iDomainId);
								SetWindowText(w,buf2048);
							}
						}
						else
						{
							SetWindowText(w,GetString(IDS_TITRE_APPNSITES));
						}
					}
					break;
				case TVN_BEGINLABELEDIT:
					{
						TRACE((TRACE_DEBUG,_F_,"TVN_BEGINLABELEDIT"));
						if (wp==TV_APPLICATIONS)
						{
							HWND wEdit=TreeView_GetEditControl(GetDlgItem(w,TV_APPLICATIONS));
							if (wEdit!=NULL) SendMessage(wEdit,EM_LIMITTEXT,LEN_APPLICATION_NAME,0);
							gbTVApplicationsLabelEditing=TRUE;
						}
						else
						{
							HWND wEdit=TreeView_GetEditControl(GetDlgItem(w,TV_DOMAINS));
							if (wEdit!=NULL) SendMessage(wEdit,EM_LIMITTEXT,LEN_DOMAIN,0);
							gbTVDomainsLabelEditing=TRUE;
						}
					}
					break;
				case TVN_ENDLABELEDIT:
					{
						TRACE((TRACE_DEBUG,_F_,"TVN_ENDLABELEDIT"));
						if (wp==TV_APPLICATIONS)
						{
							gbTVApplicationsLabelEditing=FALSE;
							// gbAtLeastOneAppRenamed=TRUE; // 0.88 - #89 // 0.90B1 : renommage direct, flag inutile
							LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lp;
							HTREEITEM hParentItem;
							if (ptvdi->item.pszText!=NULL)
							{
								// ISSUE#151
								if (*(ptvdi->item.pszText)==0) { rc=FALSE; goto end; }
								hParentItem=TreeView_GetParent(GetDlgItem(w,TV_APPLICATIONS),ptvdi->item.hItem);
								if (hParentItem==NULL) // fin d'edition du label d'une catégorie
								{
									int iCategory,iCategoryId;
									iCategoryId=TVItemGetLParam(w,ptvdi->item.hItem);
									iCategory=GetCategoryIndex(iCategoryId);
									if (iCategory!=-1)
									{
										if (IsCategoryNameUnique(iCategory,ptvdi->item.pszText))
										{
											strcpy_s(gptCategories[iCategory].szLabel,LEN_CATEGORY_LABEL+1,ptvdi->item.pszText);
											SetWindowLong(w, DWL_MSGRESULT, TRUE);
											PostMessage(w,WM_APP+1,0,0);
										}
										else
										{
											SetWindowLong(w, DWL_MSGRESULT, FALSE);
											MessageBox(w,GetString(IDS_CATEGNAME_ALREADY_EXISTS),"swSSO",MB_OK | MB_ICONEXCLAMATION);
										}
									}
								}
								else // fin d'edition du label d'une application
								{
									int iAction;
									iAction=TVItemGetLParam(w,ptvdi->item.hItem);
									if (IsApplicationNameUnique(iAction,ptvdi->item.pszText))
									{
										// 0.88 : on ne recopie plus directement le nouveau nom dans la table gptActions
										//        ce sera fait plus tard au moment de la validation (OK ou appliquer)
										//        Correction du bug #89
										// 0.90B1 : on prend en compte tout de suite le renommage (la suppression des WriteProfileString le permet)
										strcpy_s(gptActions[iAction].szApplication,LEN_APPLICATION_NAME+1,ptvdi->item.pszText);
										// 0.90 : affichage de l'application en cours de modification dans la barre de titre
										wsprintf(buf2048,"%s [%s]",GetString(IDS_TITRE_APPNSITES),gptActions[iAction].szApplication);
										SetWindowText(w,buf2048);

										SetWindowLong(w, DWL_MSGRESULT, TRUE);
										PostMessage(w,WM_APP+1,0,(LPARAM)hParentItem);
									}
									else
									{
										SetWindowLong(w, DWL_MSGRESULT, FALSE);
										MessageBox(w,GetString(IDS_APPNAME_ALREADY_EXISTS),"swSSO",MB_OK | MB_ICONEXCLAMATION);
									}
								}
							}
						}
						else if (wp==TV_DOMAINS)
						{
							gbTVDomainsLabelEditing=FALSE;
							LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lp;
							if (ptvdi->item.pszText==NULL) // le nom n'a pas été changé : en renommage, on ne fait rien mais en ajout il faut ajouter
							{
								if (gbAddingDomain)
								{
									int iDomain;
									iDomain=TVItemGetLParam(w,ptvdi->item.hItem);
									if (UploadDomain(w,iDomain)==0)
										SetWindowLong(w, DWL_MSGRESULT, TRUE);
									else
									{
										HCURSOR hCursorOld=NULL;
										hCursorOld=SetCursor(ghCursorWait);
										giNbDomains=GetDomains(TRUE,0,gtabDomains);
										FillTreeViewDomains(w);
										if (hCursorOld!=NULL) SetCursor(hCursorOld);
										SetWindowLong(w, DWL_MSGRESULT, FALSE);
									}
								}
							}
							else // le nom a changé
							{
								if (*(ptvdi->item.pszText)==0) // nom vide interdit
								{
									SetWindowLong(w, DWL_MSGRESULT, FALSE);
								}
								else
								{
									int iDomain;
									iDomain=TVItemGetLParam(w,ptvdi->item.hItem);
									if (IsDomainNameUnique(iDomain,ptvdi->item.pszText))
									{
										strcpy_s(gtabDomains[iDomain].szDomainLabel,LEN_DOMAIN+1,ptvdi->item.pszText);
										if (UploadDomain(w,iDomain)==0)
											SetWindowLong(w, DWL_MSGRESULT, TRUE);
										else
										{
											HCURSOR hCursorOld=NULL;
											hCursorOld=SetCursor(ghCursorWait);
											giNbDomains=GetDomains(TRUE,0,gtabDomains);
											FillTreeViewDomains(w);
											if (hCursorOld!=NULL) SetCursor(hCursorOld);
											SetWindowLong(w, DWL_MSGRESULT, FALSE);
										}
									}
									else
									{
										if (gbAddingDomain)
										{
											HCURSOR hCursorOld=NULL;
											hCursorOld=SetCursor(ghCursorWait);
											giNbDomains=GetDomains(TRUE,0,gtabDomains);
											FillTreeViewDomains(w);
											if (hCursorOld!=NULL) SetCursor(hCursorOld);
										}
										MessageBox(w,GetString(IDS_DOMAIN_ALREADY_EXISTS),"swSSO",MB_OK | MB_ICONEXCLAMATION);								
										SetWindowLong(w, DWL_MSGRESULT, FALSE);
									}
								}
							}
							gbAddingDomain=FALSE;
						}
						rc=TRUE;
					}
					break;
				case TVN_KEYDOWN:
					{
						LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)lp;
						switch (ptvkd->wVKey)
						{
							case VK_F2:
								{
									if (wp==TV_APPLICATIONS)
									{
										HTREEITEM hItem= TreeView_GetSelection(GetDlgItem(w,TV_APPLICATIONS)); 
										TreeView_EditLabel(GetDlgItem(w,TV_APPLICATIONS),hItem); 
									}
									else if (wp==TV_DOMAINS)
									{
										HTREEITEM hItem= TreeView_GetSelection(GetDlgItem(w,TV_DOMAINS)); 
										TreeView_EditLabel(GetDlgItem(w,TV_DOMAINS),hItem); 
									}
								}
								break;
							case VK_DELETE:
								if (wp==TV_APPLICATIONS)
								{
									TVRemoveSelectedAppOrCateg(w);
									if (!gbIsChanging && !gbAdminDeleteConfigsOnServer) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
								}
								else if (wp==TV_DOMAINS)
								{
									DeleteDomain(w);
								}
								break;
							case VK_F5:
								if (wp==TV_DOMAINS)
								{
									HCURSOR hCursorOld=NULL;
									hCursorOld=SetCursor(ghCursorWait);
									giNbDomains=GetDomains(TRUE,0,gtabDomains);
									FillTreeViewDomains(w);
									if (hCursorOld!=NULL) SetCursor(hCursorOld);
								}
								break;						
						}
					}
					break;
				case NM_DBLCLK:
					if (wp==TV_APPLICATIONS && !gbAdmin) // 0.90 correction du bug #86
					{
						TVActivateSelectedAppOrCateg(w,ACTIVATE_TOGGLE);
						if (!gbIsChanging) EnableWindow(GetDlgItem(w,IDAPPLY),TRUE); // ISSUE#114
					}
					break;
				case NM_RCLICK: 
					// C'est un peu compliqué parce que le clic droit sur les treeview est merdique...
					// Il faut sélectionner à la main l'item cliqué-droit, sinon le menu
					// s'ouvre sur l'item précédemment sélectionné !
					if (wp==TV_APPLICATIONS) // 0.90 correction du bug #86
					{
						TVHITTESTINFO htInfo;
						HTREEITEM hItem;
						GetCursorPos(&htInfo.pt);
						TRACE((TRACE_DEBUG,_F_,"GetCursorPos()  ->x=%d y=%d",htInfo.pt.x,htInfo.pt.y));
						ScreenToClient(GetDlgItem(w,TV_APPLICATIONS),&htInfo.pt);
						TRACE((TRACE_DEBUG,_F_,"ScreenToClient()->x=%d y=%d",htInfo.pt.x,htInfo.pt.y));
						hItem=TreeView_HitTest(GetDlgItem(w,TV_APPLICATIONS),&htInfo);
						TRACE((TRACE_DEBUG,_F_,"TreeView_HitTest()=0x%08lx, flags=0x%08lx",hItem,htInfo.flags));
						if (hItem!=NULL && (htInfo.flags&TVHT_ONITEM))
						{
							TreeView_SelectItem(GetDlgItem(w,TV_APPLICATIONS),hItem);
						}
						TVShowContextMenu(w);
					}
					else if (wp==TV_DOMAINS)
					{
						TVHITTESTINFO htInfo;
						HTREEITEM hItem;
						GetCursorPos(&htInfo.pt);
						TRACE((TRACE_DEBUG,_F_,"GetCursorPos()  ->x=%d y=%d",htInfo.pt.x,htInfo.pt.y));
						ScreenToClient(GetDlgItem(w,TV_DOMAINS),&htInfo.pt);
						TRACE((TRACE_DEBUG,_F_,"ScreenToClient()->x=%d y=%d",htInfo.pt.x,htInfo.pt.y));
						hItem=TreeView_HitTest(GetDlgItem(w,TV_DOMAINS),&htInfo);
						TRACE((TRACE_DEBUG,_F_,"TreeView_HitTest()=0x%08lx, flags=0x%08lx",hItem,htInfo.flags));
						if (hItem!=NULL && (htInfo.flags&TVHT_ONITEM))
						{
							TreeView_SelectItem(GetDlgItem(w,TV_DOMAINS),hItem);
						}
						TVDomainsShowContextMenu(w);
					}
					break;

			}
			break;
	}
end:
	return rc;
}

// ----------------------------------------------------------------------------------
// ShowAppNsites()
// ----------------------------------------------------------------------------------
// Fenêtre de config des applications et sites
// ----------------------------------------------------------------------------------
int ShowAppNsites(int iSelected, BOOL bFromSystray)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=1;
	T_APPNSITES tAppNsites;
	int cx;
	int cy;
	RECT rect;

	tAppNsites.iSelected=iSelected;
	tAppNsites.bFromSystray=bFromSystray;

	// si fenêtre déjà affichée, la replace au premier plan
	if (gwAppNsites!=NULL)
	{
		ShowWindow(gwAppNsites,SW_SHOW);
		// ISSUE#1 : si Alt enfoncée à l'ouverture, repositionnement par défaut
		if (HIBYTE(GetAsyncKeyState(VK_MENU))!=0) 
		{
			cx = GetSystemMetrics( SM_CXSCREEN );
			cy = GetSystemMetrics( SM_CYSCREEN );
			GetWindowRect(gwAppNsites,&rect);
			SetWindowPos(gwAppNsites,NULL,cx-(rect.right-rect.left)-50,cy-(rect.bottom-rect.top)-70,0,0,SWP_NOSIZE | SWP_NOZORDER);
		}
		SetForegroundWindow(gwAppNsites);
		
		// ISSUE#117 --> Remplissage de la treeview et sélectionne l'application
		// SAUF si la demande de réaffichage vient du systray (dans ce cas on met juste au premier plan)
		if (!bFromSystray)
		{
			FillTreeView(gwAppNsites,FALSE);
			TVSelectItemFromLParam(gwAppNsites,TYPE_APPLICATION,iSelected);
		}	
		goto end;
	}

	DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_APPNSITES),HWND_DESKTOP,AppNsitesDialogProc,(LPARAM)&tAppNsites);
	gwAppNsites=NULL;
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// TVDomainsAddDomain(HWND w,int iDomain)
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
HTREEITEM TVDomainsAddDomain(HWND w,int iDomain)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HTREEITEM hItem=NULL;
	TVINSERTSTRUCT tvis;
	
	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_SORT;
	tvis.itemex.mask=TVIF_TEXT|TVIF_PARAM; //|TVIF_STATE;
	tvis.itemex.cchTextMax=0;
	tvis.itemex.pszText=gtabDomains[iDomain].szDomainLabel;
	tvis.itemex.lParam=iDomain;
	hItem=TreeView_InsertItem(GetDlgItem(w,TV_DOMAINS),&tvis);
	if (hItem==NULL) { TRACE((TRACE_ERROR,_F_,"TreeView_InsertItem()")) ; goto end; }

end:
	TRACE((TRACE_LEAVE,_F_, "hItem=0x%08lx",hItem));
	return hItem;
}	
//-----------------------------------------------------------------------------
// FillTreeViewDomains()
//-----------------------------------------------------------------------------
// Remplit la TreeView TV_DOMAINS
//-----------------------------------------------------------------------------
void FillTreeViewDomains(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int i;
	HTREEITEM hItem=NULL;

	TreeView_DeleteAllItems(GetDlgItem(w,TV_DOMAINS));

	// Ajout des domains
	for (i=0;i<giNbDomains;i++) 
	{
		hItem=TVDomainsAddDomain(w,i); 
		if (hItem==NULL) goto end;
	}
	// sélection première ligne
	hItem=TreeView_GetRoot(GetDlgItem(w,TV_DOMAINS));
	TreeView_SelectItem(GetDlgItem(w,TV_DOMAINS),hItem); 
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// UploadDomain()
//-----------------------------------------------------------------------------
// Publie un domaine sur le serveur
//-----------------------------------------------------------------------------
int UploadDomain(HWND w,int iDomain)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HCURSOR hCursorOld=NULL;
	hCursorOld=SetCursor(ghCursorWait);
	char szRequest[512+1];
	char *pszResult=NULL;
	BOOL bRename=FALSE;

	if (iDomain==giNbDomains) // c'est un ajout
	{
		// ajout dans la base et récupération de l'identifiant du domaine
		sprintf_s(szRequest,sizeof(szRequest),"%s?action=adddomain&domainLabel=%s",gszWebServiceAddress,gtabDomains[iDomain].szDomainLabel);
		TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szRequest));
		pszResult=HTTPRequest(szRequest,8,NULL);
		if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }

		// lecture du domainId généré en base de données et remplissage du champ iDomainId
		if (pszResult[0]!='O' || pszResult[1]!='K') { TRACE((TRACE_ERROR,_F_,"Result=%s",pszResult)); goto end; }
		gtabDomains[giNbDomains].iDomainId=atoi(pszResult+3);
		giNbDomains++;
	}
	else // c'est un renommage
	{
		bRename=TRUE;
		// renommage du domaine
		sprintf_s(szRequest,sizeof(szRequest),"%s?action=renamedomain&domainId=%d&domainLabel=%s",gszWebServiceAddress,gtabDomains[iDomain].iDomainId,gtabDomains[iDomain].szDomainLabel);
		TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szRequest));
		pszResult=HTTPRequest(szRequest,8,NULL);
		if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }
		if (pszResult[0]!='O' || pszResult[1]!='K') { TRACE((TRACE_ERROR,_F_,"Result=%s",pszResult)); goto end; }
	}
	rc=0;
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszResult!=NULL) free(pszResult);
	if (rc==0)
	{
		// MessageBox(w,GetString(bRename?IDS_RENAME_DOMAIN_OK:IDS_UPLOAD_DOMAIN_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(w,GetString(bRename?IDS_RENAME_DOMAIN_KO:IDS_UPLOAD_DOMAIN_KO),"swSSO",MB_OK | MB_ICONEXCLAMATION);
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// AddDomain()
//-----------------------------------------------------------------------------
// Ajoute un domaine
//-----------------------------------------------------------------------------
void AddDomain(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HTREEITEM hNewItem; //,hSelectedItem;

	if (giNbDomains>=MAX_DOMAINS) { MessageBox(NULL,GetString(IDS_MSG_MAX_DOMAINS),"swSSO",MB_OK | MB_ICONSTOP); goto end; }
	
	// 
	gtabDomains[giNbDomains].iDomainId=0; 
	strcpy_s(gtabDomains[giNbDomains].szDomainLabel,LEN_DOMAIN+1,GetString(IDS_NEW_DOMAIN));
	GenerateDomainName(giNbDomains,gtabDomains[giNbDomains].szDomainLabel);
	
	gbAddingDomain=TRUE;

	// ajout dans la treeview et renommage
	hNewItem=TVDomainsAddDomain(w,giNbDomains);
	if (hNewItem!=NULL) 
	{
		TreeView_SelectItem(GetDlgItem(w,TV_DOMAINS),hNewItem);
		TreeView_EditLabel(GetDlgItem(w,TV_DOMAINS),hNewItem);
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// DeleteDomain()
//-----------------------------------------------------------------------------
// Supprime le domaine sélectionné
//-----------------------------------------------------------------------------
void DeleteDomain(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HTREEITEM hItem;
	int iDomain;
	char szRequest[512+1];
	char *pszResult=NULL;
	char szMsg[500];
	HCURSOR hCursorOld=NULL;

	hItem=TreeView_GetSelection(GetDlgItem(w,TV_DOMAINS));
	iDomain=TVItemGetLParam(w,hItem);
	if (iDomain==-1) goto end;

	wsprintf(szMsg,GetString(IDS_DELETE_DOMAIN_CONFIRM),gtabDomains[iDomain].szDomainLabel);
	if (MessageBox(w,szMsg,"swSSO",MB_YESNOCANCEL | MB_ICONQUESTION)!=IDYES) goto end;

	hCursorOld=SetCursor(ghCursorWait);

	// appel webservice
	sprintf_s(szRequest,sizeof(szRequest),"%s?action=deletedomain&domainId=%d",gszWebServiceAddress,gtabDomains[iDomain].iDomainId);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szRequest));
	pszResult=HTTPRequest(szRequest,8,NULL);
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }
	if (pszResult[0]=='O' && pszResult[1]=='K')
	{
		giNbDomains=GetDomains(TRUE,0,gtabDomains);
		FillTreeViewDomains(w);
		MessageBox(w,GetString(IDS_DELETE_DOMAIN_OK),"swSSO",MB_OK|MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(w,GetString(IDS_DELETE_DOMAIN_KO),"swSSO",MB_OK|MB_ICONEXCLAMATION);
	}
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, ""));
}
