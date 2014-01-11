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
// swSSOWin.cpp
//-----------------------------------------------------------------------------
// SSO fenêtres Windows (dont popup Internet Explorer)
// + SSO popup Firefox et Mozilla
//-----------------------------------------------------------------------------

#include "stdafx.h"

typedef struct 
{
	HWND w;
	int iAction; // id action
	int iSuivi;  // suivi action 
} T_SUIVI_ACTION;


typedef struct 
{
	HWND w;
	int iAction; 
	BOOL bFound; 
	int iCtrlURL;
	char *pszURL;
} T_CHECK_URL;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************


//-----------------------------------------------------------------------------
// WinFillControl()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static int WinFillControl(HWND w,int iCtrlId, const char *szValue,int iType)
{
	TRACE((TRACE_ENTER,_F_, "iCtrlId=%d szValue=%s iType=%d",iCtrlId,szValue,iType));

	int rc=-1;

	if (GetDlgCtrlID(w)!=iCtrlId) goto end;

	if (iType==EDIT)
	{
		TRACE((TRACE_INFO,_F_,"Saisie id : '%s'",szValue));
		SendMessage(w,WM_SETTEXT,0,(LPARAM)szValue);
	}
	else
	{
		TRACE((TRACE_INFO,_F_,"Sélection ligne : %d",atoi(szValue)));
		rc=SendMessage(w,CB_SETCURSEL,atoi(szValue),0);
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// WinEnumChildProc()
//-----------------------------------------------------------------------------
// Enumération des fils à la recherche des contrôles à remplir
// (SSO Windows et popup IE)
// Nouveau en 0.90B1 : on gère ID2, ID3 et ID4
//-----------------------------------------------------------------------------
static int CALLBACK WinEnumChildProc(HWND w, LPARAM lp)
{
	int iAction;
	int iPwdName;
	int rc=TRUE;
	BOOL bPwdKBSim=FALSE;
	
	iAction=((T_SUIVI_ACTION*)lp)->iAction;

	//0.85B6 (pour simulation de frappe clavier sur champ mot de passe)
	if (strlen(gptActions[iAction].szPwdName)>6 && memcmp(gptActions[iAction].szPwdName,"KBSIM:",6)==0)
	{
		iPwdName=atoi(gptActions[iAction].szPwdName+6);
		bPwdKBSim=TRUE;
	}
	else 
		iPwdName=atoi(gptActions[iAction].szPwdName);
#ifdef TRACES_ACTIVEES
	char szWindowText[200];
	GetWindowText(w,szWindowText,sizeof(szWindowText));
	TRACE((TRACE_DEBUG,_F_,"GetWindowText(0x%08lx)=%s",w,szWindowText));
	TRACE((TRACE_DEBUG,_F_,"GetDlgCtrlID=%d",GetDlgCtrlID(w)));
	TRACE((TRACE_DEBUG,_F_,"iPwdName    =%d",iPwdName));
#endif
	// champs identifiants
	if (*gptActions[iAction].szId1Name!=0)
	{
		if (WinFillControl(w,atoi(gptActions[iAction].szId1Name),GetComputedValue(gptActions[iAction].szId1Value),EDIT)==0)
			((T_SUIVI_ACTION*)lp)->iSuivi++;
	}
	if (*gptActions[iAction].szId2Name!=0) 
	{
		if (WinFillControl(w,atoi(gptActions[iAction].szId2Name),GetComputedValue(gptActions[iAction].szId2Value),gptActions[iAction].id2Type)==0)
			((T_SUIVI_ACTION*)lp)->iSuivi++;
	}
	if (*gptActions[iAction].szId3Name!=0) 
	{
		if (WinFillControl(w,atoi(gptActions[iAction].szId3Name),GetComputedValue(gptActions[iAction].szId3Value),gptActions[iAction].id3Type)==0)
			((T_SUIVI_ACTION*)lp)->iSuivi++;
	}
	if (*gptActions[iAction].szId4Name!=0) 
	{
		if (WinFillControl(w,atoi(gptActions[iAction].szId4Name),GetComputedValue(gptActions[iAction].szId4Value),gptActions[iAction].id4Type)==0)
			((T_SUIVI_ACTION*)lp)->iSuivi++;
	}

	// champ mot de passe
	if (GetDlgCtrlID(w)==iPwdName) 
	{
		TRACE((TRACE_DEBUG,_F_,"Saisie pwd"));
		if ((*gptActions[iAction].szPwdEncryptedValue!=0))
		{
			char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
			if (pszPassword!=NULL) 
			{
				// 0.85B6
				if (bPwdKBSim) 
				{
					SetForegroundWindow(((T_SUIVI_ACTION*)lp)->w);
					SetFocus(w);
					KBSim(FALSE,100,pszPassword,TRUE);
				}
				else
				{
					TRACE((TRACE_PWD,_F_,"SendMessage(WM_SETTEXT) pwd=%s",pszPassword));
					SendMessage(w,WM_SETTEXT,0,(LPARAM)pszPassword);
				}
				// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
				SecureZeroMemory(pszPassword,strlen(pszPassword));
				free(pszPassword);
			}
		}
		
		// 0.80 si l'utilisateur a demandé une simulation de frappe,
		//      on met le focus sur le champ mot de passe, ça peut aider
		if (*gptActions[iAction].szValidateName=='[')
		{
			SetFocus(w);
		}
   		Sleep(100); // tempo v0.11 contre erreurs basic auth
   		((T_SUIVI_ACTION*)lp)->iSuivi++;
   	}

   	// si on a rempli les 2 champs, on arrête l'énumération
	if (((T_SUIVI_ACTION*)lp)->iSuivi==0) rc=FALSE; 

	return rc;
}

//-----------------------------------------------------------------------------
// FillFirefoxPopupFields()
//-----------------------------------------------------------------------------
// Je fais la saisie dans le premier champ de saisie pour l'id et dans 
// le champ de saisie suivant pour le pwd
//-----------------------------------------------------------------------------
void FillFirefoxPopupFields(HWND w,int iAction,IAccessible *pAccessible)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d",w,iAction));

	HRESULT hr;
	IAccessible *pChild=NULL;
	IDispatch *pIDispatch=NULL;
	long l,lCount;
	BOOL bIdFound=FALSE,bPwdFound=FALSE;

	// comptage des childs
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr)) 
	{
		TRACE((TRACE_ERROR,_F_,"get_accChildCount()=0x%08lx",hr));
		goto end;
	}
	TRACE((TRACE_INFO,_F_,"get_accChildCount()=%ld",lCount));
	// parcours des childs
	for (l=1;l<=lCount;l++) // inutile de commencer à 0, c'est CHILDID_SELF
	{
		VARIANT vtChild,vtSelf,vtRole,vtState;
		vtChild.vt=VT_I4;
		vtChild.lVal=l;
		vtSelf.vt=VT_I4;
		vtSelf.lVal=CHILDID_SELF;

		hr=pAccessible->get_accChild(vtChild,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }

		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChild);
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }

		// lecture du role de l'élément
		hr=pChild->get_accRole(vtSelf,&vtRole);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole(%d)=0x%08lx",vtChild.lVal,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accRole(%d) vtRole.lVal=0x%08lx",vtChild.lVal,vtRole.lVal));

		// lecture du state de l'élément
		hr=pChild->get_accState(vtSelf,&vtState);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState(%d)=0x%08lx",vtChild.lVal,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accState(%d) vtState.lVal=0x%08lx",vtChild.lVal,vtState.lVal));
		
		if (!bIdFound) // pas encore trouvé le champ id
		{
			if (vtRole.lVal == ROLE_SYSTEM_TEXT && 
				((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)) &&
				!(vtState.lVal & STATE_SYSTEM_INVISIBLE))
			{
				SetForegroundWindow(w); 
				TRACE((TRACE_DEBUG,_F_,"Champ %d Saisie id  : '%s'",l,GetComputedValue(gptActions[iAction].szId1Value)));
				hr=pChild->accSelect(SELFLAG_TAKEFOCUS,vtSelf);
				KBSim(FALSE,100,GetComputedValue(gptActions[iAction].szId1Value),FALSE);
				//((T_SUIVI_ACTION*)lp)->iSuivi++;
				bIdFound=TRUE;
			}
		}
		if (bIdFound && !bPwdFound) // trouvé id mais pas encore pwd
		{
			if (vtRole.lVal == ROLE_SYSTEM_TEXT && 
				(vtState.lVal & STATE_SYSTEM_PROTECTED) &&
				((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)) &&
				!(vtState.lVal & STATE_SYSTEM_INVISIBLE))
			{
				SetForegroundWindow(w);
				hr=pChild->accSelect(SELFLAG_TAKEFOCUS,vtSelf);
				if ((*gptActions[iAction].szPwdEncryptedValue!=0))
				{
					char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
					if (pszPassword!=NULL) 
					{
						TRACE((TRACE_PWD,_F_,"Champ %d Saisie pwd : '%s'",l,pszPassword));
						KBSim(FALSE,100,pszPassword,TRUE);
						// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
						SecureZeroMemory(pszPassword,strlen(pszPassword));
						free(pszPassword);
					}
				}
				//((T_SUIVI_ACTION*)lp)->iSuivi++;
				bPwdFound=TRUE;
			}
		}
		if (bIdFound && bPwdFound) // trouvé id et pwd, ne manque plus que la validation bouton OK
		{
			if (vtRole.lVal == ROLE_SYSTEM_PUSHBUTTON && 
				((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)) &&
				!(vtState.lVal & STATE_SYSTEM_INVISIBLE))
			{
				TRACE((TRACE_DEBUG,_F_,"Validation (item #%d)",l));
				pChild->accDoDefaultAction(vtSelf);
	   			//((T_SUIVI_ACTION*)lp)->iSuivi++;
	   			goto end;
			}
		}
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
   	}
end:
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	if (pChild!=NULL) pChild->Release();
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// W7PopupSetTabOnField()
//-----------------------------------------------------------------------------
// Met le focus sur le champ demandé à coups de tab puisque Microsoft n'a
// pas bien implémenté le accSelect dans ses popups !
//-----------------------------------------------------------------------------
int W7PopupSetTabOnField(HWND w,IAccessible *pAccessible,long lIndex)
{
	TRACE((TRACE_ENTER,_F_, "lIndex=%ld",lIndex));
	int rc=-1;
	VARIANT vtChild;
	VARIANT vtSelf;
	VARIANT vtState;
	IDispatch *pIDispatch=NULL;
	IAccessible *pChild=NULL;
	HRESULT hr;
	int iAntiLoop=0;

	vtSelf.vt=VT_I4;
	vtSelf.lVal=CHILDID_SELF;
	vtChild.vt=VT_I4;
	vtChild.lVal=lIndex;
	// Récupère un pointeur IAccessible sur l'item demandé
	hr=pAccessible->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pChild);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	// Regarde s'il a le focus, si ce n'est pas le cas, tabule jusqu'à ce qu'il ait le focus
	hr=pChild->get_accState(vtSelf,&vtState);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_accState(CHILDID_SELF)=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"pChild->get_accState() vtState.lVal=0x%08lx",vtState.lVal));
	while ((!(vtState.lVal & STATE_SYSTEM_FOCUSED)) && iAntiLoop <10)
	{
		Sleep(20);
		KBSimEx(w,"[TAB]","","","","","");
		Sleep(20);
		hr=pChild->get_accState(vtSelf,&vtState);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_accState(CHILDID_SELF)=0x%08lx",hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"pChild->get_accState() vtState.lVal=0x%08lx",vtState.lVal));
		iAntiLoop++;
	}
	if (iAntiLoop==10) { TRACE((TRACE_ERROR,_F_,"Pas réussi à mettre le focus sur le champ %ld",lIndex)); goto end; }
	rc=0;
end:
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	if (pChild!=NULL) pChild->Release();

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// FillW7PopupFields() - revu ISSUE#81 pour W7 et W8
//-----------------------------------------------------------------------------
// Architecture de la popup IE sous W7 (basic auth)
// La fenêtre a 4 childs
// Identifiant et mot de passe se trouvent dans le 2nd child, qui a lui-même 5 childs de niveau 2
// Identifiant = 3ème child de niveau 2
// Mot de passe = 4ème child de niveau 2
//-----------------------------------------------------------------------------
// Architecture de la popup de partage réseau sous W7
// La fenêtre a 5 childs de niveau 1
// Identifiant et mot de passe se trouvent dans le 3ème child, qui a lui-même 6 childs de niveau 2
// Identifiant = 3ème child de niveau 2
// Mot de passe = 4ème child de niveau 2
//-----------------------------------------------------------------------------
// Architecture de la popup W8
// La fenêtre a 5 childs
// Identifiant et mot de passe se trouvent dans le 3ème child, qui lui-même 1 child
// qui a lui-même 4 childs :
// Identifiant = 2ème child du 3ème niveau 
// Mot de passe = 3ème child du 3ème niveau
// Bouton OK = 4ème child du 1er niveau
//-----------------------------------------------------------------------------
void FillW7PopupFields(HWND w,int iAction,IAccessible *pAccessible)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d",w,iAction));

	HRESULT hr;
	IAccessible *pLevel1Child=NULL;
	IAccessible *pLevel2Child=NULL;
	IDispatch *pIDispatch=NULL;
	long lFirstLevelChildCount, lSecondLevelChildCount;
	VARIANT index;
	int rc;
	VARIANT vtSelf;
	vtSelf.vt=VT_I4;
	vtSelf.lVal=CHILDID_SELF;

	// comptage des childs de 1er niveau
	hr=pAccessible->get_accChildCount(&lFirstLevelChildCount);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_INFO,_F_,"get_accChildCount()=%ld",lFirstLevelChildCount));
	if (lFirstLevelChildCount!=4 && lFirstLevelChildCount!=5) { TRACE((TRACE_ERROR,_F_,"Problème : on attendait 4 ou 5 childs de 1er niveau, il y en a %d !",lFirstLevelChildCount)); goto end; }

	// récupération du 2nd ou 3ème child de 1er niveau en fonction du nb de child au total (resp. 4 ou 5)
	index.vt=VT_I4;
	
	if (giOSVersion < OS_WINDOWS_8)
	{
		index.lVal=lFirstLevelChildCount-2; // 2eme child si nb total=4, 3ème child si nb total=5
		hr=pAccessible->get_accChild(index,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",index.lVal,hr));
		if (FAILED(hr)) goto end;
		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pLevel1Child);
		pIDispatch->Release(); pIDispatch=NULL;
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
		if (FAILED(hr)) goto end;
		hr=pLevel1Child->get_accChildCount(&lSecondLevelChildCount);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pLevel1Child->get_accChildCount()=0x%08lx",hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accChildCount()=%ld",lSecondLevelChildCount));
		if (lSecondLevelChildCount!=5 && lSecondLevelChildCount!=6) { TRACE((TRACE_ERROR,_F_,"Problème : on attendait 5 ou 6 childs de 2nd niveau, il y en a %d !",lSecondLevelChildCount)); goto end; }

		SetForegroundWindow(w); 
		
		// Identifiant = child de 2nd niveau n°3 
		rc=W7PopupSetTabOnField(w,pLevel1Child,3);
		TRACE((TRACE_DEBUG,_F_,"Saisie id  : '%s'",GetComputedValue(gptActions[iAction].szId1Value)));
		KBSim(FALSE,100,GetComputedValue(gptActions[iAction].szId1Value),FALSE);
		
		// Mot de passe = child de 2nd niveau n°4
		rc=W7PopupSetTabOnField(w,pLevel1Child,4);
		if ((*gptActions[iAction].szPwdEncryptedValue!=0))
		{
			char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
			if (pszPassword!=NULL) 
			{
				TRACE((TRACE_PWD,_F_,"Saisie pwd : '%s'",pszPassword));
				KBSim(FALSE,100,pszPassword,TRUE);
				// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
				SecureZeroMemory(pszPassword,strlen(pszPassword));
				free(pszPassword);
			}
		}
	}
	else // W8 
	{
		index.lVal=3; 
		hr=pAccessible->get_accChild(index,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",index.lVal,hr));
		if (FAILED(hr)) goto end;
		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pLevel1Child);
		pIDispatch->Release(); pIDispatch=NULL;
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
		if (FAILED(hr)) goto end;
		hr=pLevel1Child->get_accChildCount(&lSecondLevelChildCount);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pLevel1Child->get_accChildCount()=0x%08lx",hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accChildCount()=%ld",lSecondLevelChildCount));
		
		index.lVal=1; 
		pLevel1Child->get_accChild(index,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",index.lVal,hr));
		if (FAILED(hr)) goto end;
		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pLevel2Child);
		pIDispatch->Release(); pIDispatch=NULL;
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
		if (FAILED(hr)) goto end;
		
		SetForegroundWindow(w); 
		
		// Identifiant = child n°2 
		rc=W7PopupSetTabOnField(w,pLevel2Child,2);
		TRACE((TRACE_DEBUG,_F_,"Saisie id  : '%s'",GetComputedValue(gptActions[iAction].szId1Value)));
		KBSim(FALSE,100,GetComputedValue(gptActions[iAction].szId1Value),FALSE);
		
		// Mot de passe = child n°2
		rc=W7PopupSetTabOnField(w,pLevel2Child,3);
		if ((*gptActions[iAction].szPwdEncryptedValue!=0))
		{
			char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
			if (pszPassword!=NULL) 
			{
				TRACE((TRACE_PWD,_F_,"Saisie pwd : '%s'",pszPassword));
				KBSim(FALSE,100,pszPassword,TRUE);
				// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
				SecureZeroMemory(pszPassword,strlen(pszPassword));
				free(pszPassword);
			}
		}
	}

	Sleep(20);
	KBSimEx(w,"[ENTER]","","","","","");

end:
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	if (pLevel1Child!=NULL) pLevel1Child->Release();
	if (pLevel2Child!=NULL) pLevel2Child->Release();
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// CheckURLProc()
//-----------------------------------------------------------------------------
// ISSUE#XX : nouvelle fonction d'énumération des childs en remplacement
// du GetDlgItem qui ne trouvait pas le bouton "NOuveau mot de passe" 
// de la fenêtre de login de SAP
//-----------------------------------------------------------------------------
static int CALLBACK CheckURLProc(HWND w, LPARAM lp)
{
	int iAction;
	iAction=((T_CHECK_URL*)lp)->iAction;
	int rc=TRUE;
	char szCtrlURL[128+1];
	HRESULT hr;
	IAccessible *pAccessible=NULL;
	BSTR bstrName=NULL;

	TRACE((TRACE_DEBUG,_F_,"GetDlgCtrlID=%d",GetDlgCtrlID(w)));
	if (GetDlgCtrlID(w)==((T_CHECK_URL*)lp)->iCtrlURL) // champ trouvé, il faut encore vérifier son libellé
	{
		TRACE((TRACE_DEBUG,_F_,"Champ %d trouve, on verifie son libelle",((T_CHECK_URL*)lp)->iCtrlURL));
		GetWindowText(w,szCtrlURL,sizeof(szCtrlURL));
		TRACE((TRACE_DEBUG,_F_,"GetWindowText(0x%08lx)=%s",w,szCtrlURL));
		if (*szCtrlURL==0) // pas de libellé trouvé, peut-être qu'on est dans un cas type SAP...
		{
			TRACE((TRACE_DEBUG,_F_,"Libelle vide, recherche via API accessibilite"));
			hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
			VARIANT vtMe;
			vtMe.vt=VT_I4;
			vtMe.lVal=CHILDID_SELF;
			hr=pAccessible->get_accName(vtMe,&bstrName);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pAccessible->get_accName()=0x%08lx",hr)); goto end; }
			TRACE((TRACE_DEBUG,_F_,"Libelle=%S",bstrName));
			wsprintf(szCtrlURL,"%S",bstrName); // waouh le beau risque de buffer overflow :-)
		}
		if (*szCtrlURL!=0) // // libellé trouvé, on vérifie que c'est le bon 
		{
			if (swStringMatch(szCtrlURL,((T_CHECK_URL*)lp)->pszURL))
			{
				TRACE((TRACE_INFO,_F_,"Trouve le champ !"));
				((T_CHECK_URL*)lp)->bFound=TRUE;
			}
		}
		rc=FALSE; // libellé correct ou pas, on arrête l'énum car on a trouvé un champ avec le bon ID donc inutile d'espérer en trouver un autre
	}
end:
	if (bstrName!=NULL) SysFreeString(bstrName);
	if (pAccessible!=NULL) pAccessible->Release();
	return rc;
}


//-----------------------------------------------------------------------------
// CheckURL()
//-----------------------------------------------------------------------------
// Vérification "URL" des fenêtres Windows 
// ISSUE#XX : modification pour SAP : comme le champ URL n'était pas trouvable
// par GetDlgItem, j'ai remplacé par une énumération avec CheckURLProc()
//-----------------------------------------------------------------------------
BOOL CheckURL(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	int iCtrlURL=-1;
	char *pszURL=NULL;
	char szURL[128+1];
	T_CHECK_URL tCheckURL;

	TRACE((TRACE_INFO,_F_,"szURL    =%s",gptActions[iAction].szURL));
	strcpy_s(szURL,sizeof(szURL),gptActions[iAction].szURL);
	pszURL=strchr(szURL,':');
	if (pszURL!=NULL)
	{
		*pszURL=0;
		if ((pszURL+1)!=0)
		{
			iCtrlURL=atoi(szURL);
			pszURL++;
		}
	}
	TRACE((TRACE_DEBUG,_F_,"iCtrlURL =%d",iCtrlURL));
	if (iCtrlURL==-1) goto end; 
	TRACE((TRACE_DEBUG,_F_,"pszURL   =%s",pszURL)); 

	tCheckURL.iAction=iAction;
	tCheckURL.iCtrlURL=iCtrlURL;
	tCheckURL.bFound=FALSE;
	tCheckURL.pszURL=pszURL;
	EnumChildWindows(w,CheckURLProc,(LPARAM)&tCheckURL);
	rc=tCheckURL.bFound;
end:
	//if (pszURL!=NULL) free(pszURL); //0.85B6 => grave erreur corrigée en 0.92B1, il ne faut surtout pas libérer !
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// SSOWindows()
//-----------------------------------------------------------------------------
// SSO fenetre Windows (dont popup IE sauf W7 traitée dans swSSOMain.cpp) 
// + popup Firefox et Mozilla
// ----------------------------------------------------------------------------
// [in] w
// [in] iAction
// [in] iPopupType 
// ----------------------------------------------------------------------------
int SSOWindows(HWND w,int iAction,int iPopupType)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d iPopupType=%d",w,iAction,iPopupType));
	int rc=-1;
	
	T_SUIVI_ACTION tSuiviAction;
	IAccessible *pAccessible=NULL;
	tSuiviAction.w=w;
	tSuiviAction.iAction=iAction;
	tSuiviAction.iSuivi=5;

	if (gptActions[iAction].iType==POPSSO)
	{
		strcpy_s(gptActions[iAction].szId1Name,sizeof(gptActions[iAction].szId1Name),"1003");
		strcpy_s(gptActions[iAction].szPwdName,sizeof(gptActions[iAction].szPwdName),"1005");
		strcpy_s(gptActions[iAction].szValidateName,sizeof(gptActions[iAction].szValidateName),"1");
	}
	TRACE((TRACE_DEBUG,_F_,"szId1Name=%s",gptActions[iAction].szId1Name));
	TRACE((TRACE_DEBUG,_F_,"szId2Name=%s",gptActions[iAction].szId2Name));
	TRACE((TRACE_DEBUG,_F_,"szId3Name=%s",gptActions[iAction].szId3Name));
	TRACE((TRACE_DEBUG,_F_,"szId4Name=%s",gptActions[iAction].szId4Name));
	TRACE((TRACE_DEBUG,_F_,"szPwdName=%s",gptActions[iAction].szPwdName));
	
	if (*(gptActions[iAction].szId1Name)==0) tSuiviAction.iSuivi--;
	if (*(gptActions[iAction].szPwdName)==0) tSuiviAction.iSuivi--;
	if (*(gptActions[iAction].szId2Name)==0) tSuiviAction.iSuivi--;
	if (*(gptActions[iAction].szId3Name)==0) tSuiviAction.iSuivi--;
	if (*(gptActions[iAction].szId4Name)==0) tSuiviAction.iSuivi--;
	
	TRACE((TRACE_DEBUG,_F_,"tSuiviAction.iSuivi=%d",tSuiviAction.iSuivi));
	
	// 0.60+0.61 - traitement particulier des popup firefox
	if (iPopupType==POPUP_FIREFOX)
	{
		pAccessible=GetFirefoxPopupIAccessible(w);
		if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup")); goto end; }
		FillFirefoxPopupFields(w,iAction,pAccessible);
	}
	else if (iPopupType==POPUP_W7) // ISSUE#61 0.93
	{
		pAccessible=GetW7PopupIAccessible(w);
		if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup")); goto end; }
		FillW7PopupFields(w,iAction,pAccessible);
	}
	else // traitement des autres fenêtres (inchangé en 0.60)
	{
		// si pas d'identifiant ni mot de passe, on se contentera de cliquer
		if (tSuiviAction.iSuivi!=0)
		{
			// dans tous les autres cas, énumération des contrôles de la fenêtre
			EnumChildWindows(w,WinEnumChildProc,(LPARAM)&tSuiviAction);	    			
		}
		// au final, on clique	    		
		// 0.80 simulation frappe clavier touche ENTER si szValidateName=[ENTER]
		if (*gptActions[iAction].szValidateName=='[')
		{
			Sleep(50);
			SetForegroundWindow(w); //0.85B3
			Sleep(50);
			TRACE((TRACE_INFO,_F_,"Simulation frappe clavier"));
			KBSimEx(NULL,gptActions[iAction].szValidateName,"","","","","");
		}
		else // sinon, on poste le message de clic sur le bouton
		{
			TRACE((TRACE_INFO,_F_,"Clic sur le bouton id=%s",gptActions[iAction].szValidateName));
			PostMessage(w,WM_COMMAND,MAKEWPARAM(atoi(gptActions[iAction].szValidateName),BM_CLICK),0);
		}
	}
	if (gptActions[iAction].iType==POPSSO)
		guiNbPOPSSO++;
	else
		guiNbWINSSO++;
	rc=0;
end:
	// 0.91 : rétablit la config avec champs vide sinon ca risque de remonter sur le serveur et 
	//        ca génère des comportements non voulus surtout en multicomptes car la config
	//		  n'est plus reconnue
	if (gptActions[iAction].iType==POPSSO)
	{
		*(gptActions[iAction].szId1Name)=0;
		*(gptActions[iAction].szPwdName)=0;
		*(gptActions[iAction].szValidateName)=0;
	}
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_, "%d",rc));
	return rc; 
}
