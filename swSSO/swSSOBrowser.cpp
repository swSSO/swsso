//-----------------------------------------------------------------------------
//                                  swSSO
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//                Copyright (C) 2004-2025 - Sylvain WERDEFROY
//
//                       sylvain.werdefroy@gmail.com
//-----------------------------------------------------------------------------
//  This file is part of swSSO.
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------
#include "stdafx.h"

BSTR gbstrPwd=NULL; 
// OK, vous allez me dire que c'est moche, mais je préfère une globale c'est plus facile
// à nettoyer que d'embarquer le pwd dans la fonction récursive et de risquer de rater
// un SecureZeroMemory

// ----------------------------------------------------------------------------------
// WebEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils à la recherche de la fenêtre de rendu du navigateur
// ----------------------------------------------------------------------------------
static int CALLBACK WebEnumChildProc(HWND w, LPARAM lp)
{
	int rc=TRUE;
	char szClassName[50+1];
	IHTMLDocument2 *pHTMLDocument2=NULL;
	BSTR bstrURL=NULL;
	char *pszURL=NULL;
	HRESULT hr;
	DWORD dw;

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
	if (strcmp(szClassName,((T_SUIVI_ACCESSIBLE*)lp)->szExclude)==0)
	{
		((T_SUIVI_ACCESSIBLE*)lp)->w=NULL;
		TRACE((TRACE_INFO,_F_,"Fenetre classe=%s w=0x%08lx --> stoppe l'enumeration, retourne NULL",szClassName,w));
		rc=FALSE;
	}
	else if (strcmp(szClassName,((T_SUIVI_ACCESSIBLE*)lp)->szClassName)==0)
	{
		((T_SUIVI_ACCESSIBLE*)lp)->w=w;
		TRACE((TRACE_INFO,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
		if (strcmp(szClassName,"Internet Explorer_Server")==0)
		{
			// ISSUE#312 : si la console debug F12 est ouverte, elle apparait en premier dans l'énumération des fenêtres.
			//             Il faut l'ignorer et continuer l'énumération
			// récupération pointeur sur le document HTML (interface IHTMLDocument2)
			SendMessageTimeout(w,guiHTMLGetObjectMsg,0L,0L,SMTO_ABORTIFHUNG,1000,&dw);
			hr=ObjectFromLresult(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"ObjectFromLresult(%d,IID_IHTMLDocument2)=0x%08lx",dw,hr)); goto end; }
   			TRACE((TRACE_DEBUG,_F_,"ObjectFromLresult(IID_IHTMLDocument2)=%08lx pHTMLDocument2=%08lx",hr,pHTMLDocument2));
			hr=pHTMLDocument2->get_URL(&bstrURL);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_URL()=0x%08lx",hr)); goto end; }
			pszURL=GetSZFromBSTR(bstrURL);
			TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",pszURL));
			if (_strnicmp(pszURL,"res://",6)==0)
			{
				TRACE((TRACE_DEBUG,_F_,"C'est la fenetre F12, on continue !"));
				goto end;
			}
		}
		rc=FALSE;
	}
end:
	if (pHTMLDocument2!=NULL) pHTMLDocument2->Release();
	if (bstrURL!=NULL) SysFreeString(bstrURL);
	if (pszURL!=NULL) free(pszURL);
	return rc;
}
// ----------------------------------------------------------------------------------
// BrowserGetType
// ----------------------------------------------------------------------------------
// Détermine le type de navigateur
// ----------------------------------------------------------------------------------
// [rc] BROWSER_NONE si erreur, sinon autre constante BROWSER_
// ----------------------------------------------------------------------------------
int BrowserGetType(HWND w)
{
	TRACE((TRACE_ENTER,_F_,""));

	int rc=BROWSER_NONE;
	char szClassName[128+1];

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"szClassName=%s",szClassName));

	if (strcmp(szClassName,"IEFrame")==0 || // IE
		strcmp(szClassName,"#32770")==0 ||  // Network Connect
		strcmp(szClassName,"rctrl_renwnd32")==0 || // Outlook 97 à 2003 (au moins, à vérifier pour 2007)
		strcmp(szClassName,"OpusApp")==0 || // Word 97 à 2003 (au moins, à vérifier pour 2007)
		strcmp(szClassName,"ExploreWClass")==0 || strcmp(szClassName,"CabinetWClass")==0) // Explorateur Windows
	{
		rc=BROWSER_IE;
	}
	else if (strcmp(szClassName,gcszMozillaUIClassName)==0) // FF3
	{
		rc=BROWSER_FIREFOX3;
	}
	else if (strcmp(szClassName,gcszMozillaClassName)==0) // FF4
	{
		rc=BROWSER_FIREFOX4;
	}
	else if (strcmp(szClassName,"Maxthon2_Frame")==0) // Maxthon
	{
		rc=BROWSER_MAXTHON;
	}
	else if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0) // ISSUE#77 : Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	{
		ForceChromeAccessibility(w);
		rc=BROWSER_CHROME;
	}
	else if (strcmp(szClassName,"ApplicationFrameWindow")==0) 
	{
		rc=BROWSER_EDGE;
	}
	else // autre ??
	{
		TRACE((TRACE_ERROR,_F_,"Unknown class : %s !",szClassName));
	}
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// BrowserGetIAccessible
// ----------------------------------------------------------------------------------
// Retourne un IAccessible
// ----------------------------------------------------------------------------------
// [rc] NULL si erreur
// ----------------------------------------------------------------------------------
IAccessible *BrowserGetIAccessible(HWND w,int iBrowser)
{
	TRACE((TRACE_ENTER,_F_,""));
	
	IAccessible *pAccessible=NULL;
	IAccessible *pTopAccessible=NULL;
	IAccessible *pNiveau0=NULL,*pChildNiveau1=NULL, *pChildNiveau2=NULL;
	T_SUIVI_ACCESSIBLE tSuivi;
	HRESULT hr;
	IDispatch *pIDispatch=NULL;
	VARIANT vtChild;
	T_SEARCH_DOC tSearchDoc;
	
	ZeroMemory(&tSuivi,sizeof(T_SUIVI_ACCESSIBLE));
	if (iBrowser==BROWSER_IE)
	{
		strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"Internet Explorer_Server");
		EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuivi);
		if (tSuivi.w!=NULL) { TRACE((TRACE_INFO,_F_,"Internet Explorer_Server = %d",IsWindowVisible(tSuivi.w))); }
		if (tSuivi.w==NULL) { TRACE((TRACE_ERROR,_F_,"EnumChildWindows() => pas trouve la fenetre de contenu")); goto end; }
		// Obtient un IAccessible
		hr=AccessibleObjectFromWindow(tSuivi.w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	}
	else if (iBrowser==BROWSER_CHROME)
	{
		// enum des fils à la recherche de la fenêtre de rendu Web
		strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"Chrome_RenderWidgetHostHWND");
		// ISSUE #98 . 0.99 : pour chrome 33 ou 34+, l'énumération refonctionne (cf ISSUE#95 plus bas) mais la fenêtre
		// qu'on récupère ne fonctionne pas. Elle a un vtRole.lVal=0x0000000a au lieu de vtRole.lVal=0x0000000f, et 0 childs !
		// La version 34 de Chrome se reconnait grace à la fenêtre de classe static qui est remontée juste avant celle 
		// qu'on cherche, du coup si je trouve fenêtre static, je rebranche sur la recherche sans énumération
		// 11/01-15:53:58:470 DEBUG WebEnumChildProc Fenetre classe=Static w=0x00030618
		// 11/01-15:53:58:470 DEBUG WebEnumChildProc Fenetre classe=Chrome_RenderWidgetHostHWND w=0x00080120
		strcpy_s(tSuivi.szExclude,sizeof(tSuivi.szExclude),"Static");
		EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuivi);
		if (tSuivi.w==NULL)
		{ 
			// ISSUE#95 / 0.98 : pour Chome 31 ou 32+, impossible de rechercher la fenêtre fille, on est obligé de passer par IAccessible :
			// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
			// Le child de niveau 1 a 2 childs, il faut prendre le 2eme.
			// Le child de niveau 2 a 4 childs, il faut prendre le 2eme --> c'est la fenêtre de contenu web !
			hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pNiveau0);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
			// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
			vtChild.vt=VT_I4;
			vtChild.lVal=1;
			hr=pNiveau0->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau1);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			// Le child de niveau 1 a 2 childs, il faut prendre le 2eme.
			vtChild.vt=VT_I4;
			vtChild.lVal=2;
			hr=pChildNiveau1->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau1->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau2);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			// Le child de niveau 2 a 4 childs, il faut prendre le 2eme.
			vtChild.vt=VT_I4;
			vtChild.lVal=2;
			hr=pChildNiveau2->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau2->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pAccessible);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
		}
		else
		{
			// Obtient un IAccessible
			hr=AccessibleObjectFromWindow(tSuivi.w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
		}
		VARIANT vtMe,vtRole;
		vtMe.vt=VT_I4;
		vtMe.lVal=CHILDID_SELF;
		hr=pAccessible->get_accRole(vtMe,&vtRole);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accRole() vtRole.lVal=0x%08lx",vtRole.lVal));
	}
	else if (iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4)
	{
		// accNavigate permet de trouver la fenêtre de rendu web
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pTopAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
		
		VARIANT vtMe,vtResult;
		vtMe.vt=VT_I4;
		vtMe.lVal=CHILDID_SELF;

		hr=pTopAccessible->accNavigate(0x1009,vtMe,&vtResult); // NAVRELATION_EMBEDS = 0x1009
		TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS)=0x%08lx",hr));
		if (hr!=S_OK) // ISSUE#371 : cf. échanges avec Jamie, un échec au 1er appel est normal, on retente juste 1 fois (OK avec Firefox 59)
		{
			Sleep(100); // Sleep(1) pas suffisant
			hr=pTopAccessible->accNavigate(0x1009,vtMe,&vtResult); // NAVRELATION_EMBEDS = 0x1009
			TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS)=0x%08lx",hr));
		}
		if (hr==S_OK) 
		{
			pTopAccessible->Release();pTopAccessible=NULL;
			TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS) vtEnd=%ld",vtResult.lVal));
			if (vtResult.vt!=VT_DISPATCH) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) is not VT_DISPATCH")); goto end; }
			pIDispatch=(IDispatch*)vtResult.lVal;
			if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) pIDispatch=NULL")); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pAccessible);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }	
		}
		else // Solution de contournement conservée au cas où accNavigate(NAVRELATION_EMBEDS) échoue (cf. ISSUE#358 et ISSUE#371)
		{
			// Parcourt l'ensemble de la page à la recherche de l'objet document
			tSearchDoc.pContent=NULL;
			tSearchDoc.iLevel=0;

#ifdef TRACES_ACTIVEES
			SearchWebDocument("",pTopAccessible,&tSearchDoc);
#else
			SearchWebDocument(pAccessible,&tSearchDoc);
#endif		
			pTopAccessible->Release();pTopAccessible=NULL;
			if (tSearchDoc.pContent==NULL)
			{
				TRACE((TRACE_ERROR,_F_,"SearchWebDocument n'a pas trouvé l'objet document...")); goto end;
			}
			pAccessible=tSearchDoc.pContent;
		}	
	}
	else if (iBrowser==BROWSER_XIN) // test XIN
	{
		// Obtient un IAccessible
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	}
	else if (iBrowser==BROWSER_EDGE)
	{
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"Unknown browser : %d",iBrowser)); goto end;
	}
end:
	if (pTopAccessible!=NULL) pTopAccessible->Release();
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pNiveau0!=NULL) pNiveau0->Release();
	if (pChildNiveau1!=NULL) pChildNiveau1->Release();
	if (pChildNiveau2!=NULL) pChildNiveau2->Release();
	TRACE((TRACE_LEAVE,_F_,"pAccessible=0x%08lx",pAccessible));
	return pAccessible;
}

// ----------------------------------------------------------------------------------
// BrowserRecurseFillPasswords() [attention, fonction récursive]
// ----------------------------------------------------------------------------------
static void BrowserRecurseFillPasswords(char *paramszTab,HWND w,IAccessible *pAccessible,T_SUIVI_ACCESSIBLE *ptSuivi)
{
	TRACE((TRACE_ENTER,_F_, ""));
	UNREFERENCED_PARAMETER(w);
	
	HRESULT hr;
	IAccessible *pChild=NULL;
	VARIANT vtChild;
	VARIANT vtState,vtRole;
	long l,lCount;
	long returnCount;
	VARIANT* pArray = NULL;
	BSTR bstrName=NULL;
	char *pszName=NULL;
	IDispatch *pTempIDispatch=NULL; // correction memory leak en 1.19b7
	
	char szTab[200];
	strcpy_s(szTab,sizeof(szTab),paramszTab);
	if (strlen(szTab)<sizeof(szTab)-5) strcat_s(szTab,sizeof(szTab),"  ");

	// si fini ou erreur, on termine la récursivité
	if (ptSuivi->iErreur!=0) goto end;

	// parcours de la suite : combien de fils ?
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr)) goto end;
	TRACE((TRACE_DEBUG,_F_,"%sget_accChildCount()==%ld",szTab,lCount));

	// plus de fils ou lu assez de champs, on termine !
	if (lCount==0)
	{
		TRACE((TRACE_INFO,_F_,"%sPas de fils",szTab));
		goto end;
	}
	if (ptSuivi->iTextFieldIndex>MAX_TEXT_FIELDS)
	{
		TRACE((TRACE_INFO,_F_,"Trop de champs, on arrête la recherche dans la page (lCount=%d ptSuivi->iTextFieldIndex=%d)",lCount,ptSuivi->iTextFieldIndex));
		goto end;
	}
	if (ptSuivi->iPwdIndex!=-1 && (ptSuivi->iTextFieldIndex - ptSuivi->iPwdIndex)>10) // optimisation : on ne lit pas plus de 10 champs après le mdp
	{
		TRACE((TRACE_INFO,_F_,"Fin de la recherche dans la page (ptSuivi->iTextFieldIndex=%d ptSuivi->iPwdIndex=%d)",ptSuivi->iTextFieldIndex,ptSuivi->iPwdIndex));
		goto end;
	}

	pArray = new VARIANT[lCount];
	hr = AccessibleChildren(pAccessible, 0L, lCount, pArray, &returnCount);
	if (FAILED(hr))
	{
		TRACE((TRACE_DEBUG,_F_,"%sAccessibleChildren()=0x%08lx",szTab,hr));
	}
	else
	{
		TRACE((TRACE_DEBUG,_F_,"%sAccessibleChildren() returnCount=%d",szTab,returnCount));
		for (l=0;l<lCount;l++)
		{
			VARIANT *pVarCurrent = &pArray[l];
			VariantInit(&vtRole);
			VariantInit(&vtState);
			pChild=NULL;
				
			TRACE((TRACE_DEBUG,_F_,"%s --------------------------------- l=%ld vt=%d lVal=0x%08lx",szTab,l,pVarCurrent->vt,pVarCurrent->lVal));
			if (pVarCurrent->vt!=VT_DISPATCH) goto suivant;
			if (pVarCurrent->lVal==NULL) goto suivant; // ISSUE#80 0.96B2 
			pTempIDispatch=((IDispatch*)(pVarCurrent->lVal)); // correction memory leak en 1.19b7
			pTempIDispatch->QueryInterface(IID_IAccessible, (void**) &pChild);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx",szTab,hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx -> pChild=0x%08lx",szTab,hr,pChild));
			
			vtChild.vt=VT_I4;
			vtChild.lVal=CHILDID_SELF;
			hr=pChild->get_accState(vtChild,&vtState);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sget_accState() vtState.lVal=0x%08lx",szTab,vtState.lVal));

			hr=pChild->get_accRole(vtChild,&vtRole);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sget_accRole() vtRole.lVal=0x%08lx",szTab,vtRole.lVal));
			
			// Reconnaissance du champ mot de passe :
			// - Role = ROLE_SYSTEM_TEXT
			// - State = (STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED) & STATE_SYSTEM_PROTECTED

			// ISSUE#279 : cas spécifique IE
			if (ptSuivi->iBrowser!=BROWSER_IE)
			{
				if ((vtRole.lVal == ROLE_SYSTEM_TEXT || vtRole.lVal==ROLE_SYSTEM_COMBOBOX) && 
					((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)))
				{
					// c'est un champ de saisie, s'il est protégé c'est le mdp
					if (vtState.lVal & STATE_SYSTEM_PROTECTED)
					{
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS* + STATE_SYSTEM_PROTECTED)"));
						// remplit le mot de passe
						hr=pChild->put_accValue(vtChild,gbstrPwd);
						if (SUCCEEDED(hr)) ptSuivi->iNbPwdFound++;
					}
				}
				else
				{
					BrowserRecurseFillPasswords(szTab,NULL,pChild,ptSuivi);
				}
			}
			else // code spécifique IE
			{
				if ((vtRole.lVal == ROLE_SYSTEM_TEXT) && !(vtState.lVal & STATE_SYSTEM_READONLY))
				{
					// c'est un champ de saisie, s'il est protégé c'est le mdp
					if (vtState.lVal & STATE_SYSTEM_PROTECTED)
					{
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS* + STATE_SYSTEM_PROTECTED)"));
						// remplit le mot de passe
						hr=pChild->put_accValue(vtChild,gbstrPwd);
						if (SUCCEEDED(hr)) ptSuivi->iNbPwdFound++;
					}
				}
				else
				{
					BrowserRecurseFillPasswords(szTab,NULL,pChild,ptSuivi);
				}
			}
suivant:
			if (pTempIDispatch!=NULL)  { pTempIDispatch->Release(); pTempIDispatch=NULL; } // correction memory leak en 1.19b7
			if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		} // for
	}
	
end:
	if (pTempIDispatch!=NULL)  { pTempIDispatch->Release(); pTempIDispatch=NULL; } // correction memory leak en 1.19b7
	if (pChild!=NULL) { pChild->Release(); pChild=NULL; } // ajouté en 1.19b7
	SysFreeString(bstrName);
	if (pszName!=NULL) free (pszName);
	if (pArray!=NULL) delete[] pArray;
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// BrowserFillPasswords()
// ----------------------------------------------------------------------------------
int BrowserFillPasswords(HWND w,char *pszPwd)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	IAccessible *pAccessible=NULL;
	VARIANT vtChild,vtState;
	HRESULT hr;
	int iNbTry;
	long lCount;
	T_SUIVI_ACCESSIBLE tSuivi;
	int rc=-1;

	ZeroMemory(&tSuivi,sizeof(T_SUIVI_ACCESSIBLE)); // déplacé plus haut, mais laissé ici aussi dans le doute ça coute pas cher.
	tSuivi.iBrowser=BrowserGetType(w);
	if (tSuivi.iBrowser==BROWSER_NONE) goto end;

	pAccessible=BrowserGetIAccessible(w,tSuivi.iBrowser);
	if (pAccessible==NULL) goto end;

	// regarde si le navigateur n'est pas busy
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	VariantInit(&vtState);
	hr=pAccessible->get_accState(vtChild,&vtState);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_accState(DOCUMENT PRINCIPAL) vtState.lVal=0x%08lx",vtState.lVal));
	iNbTry=1;
	while ((vtState.lVal & STATE_SYSTEM_BUSY) && iNbTry < 6)
	{
		TRACE((TRACE_DEBUG,_F_,"STATE_SYSTEM_BUSY -- wait 100ms before retry #%d (max 5)",iNbTry));
		Sleep(100);
		VariantInit(&vtState);
		hr=pAccessible->get_accState(vtChild,&vtState);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); }
		TRACE((TRACE_DEBUG,_F_,"get_accState(DOCUMENT PRINCIPAL) vtState.lVal=0x%08lx",vtState.lVal));
		iNbTry++;
	}
	// Prépare le mot de passe
	gbstrPwd=GetBSTRFromSZ(pszPwd);
	if (gbstrPwd==NULL) goto end;
	// OK, c'est parti
	lCount=0;
	hr=pAccessible->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (tSuivi.iBrowser==BROWSER_CHROME) // lCount=0 arrive parfois quelque fois après ouverture d'un nouvel onglet
	{
		iNbTry=0;
		while (lCount==0 && iNbTry<5)
		{
			Sleep(100);
			pAccessible->Release();
			pAccessible=NULL;
			// Obtient un IAccessible
			pAccessible=BrowserGetIAccessible(w,tSuivi.iBrowser);
			if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
			hr=pAccessible->get_accChildCount(&lCount);
			TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
			iNbTry++;
		}
		if (lCount==0) // ça n'a toujours pas marché, on abandonne...
		{
			TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end;
		}
	}

	tSuivi.w=w;
	tSuivi.iErreur=0;
	tSuivi.iLevel=0;
	BrowserRecurseFillPasswords("",w,pAccessible,&tSuivi);
	TRACE((TRACE_INFO,_F_,"tSuivi.iErreur=%d",tSuivi.iErreur));

	if (tSuivi.iNbPwdFound!=0) rc=0;

end:
	if (gbstrPwd!=NULL)
	{
		SecureZeroMemory(gbstrPwd,SysStringByteLen(gbstrPwd));
		SysFreeString(gbstrPwd);
	}
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

