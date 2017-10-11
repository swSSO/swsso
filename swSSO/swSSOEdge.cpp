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
// swSSOFirefox.cpp
//-----------------------------------------------------------------------------
// SSO Firefox et Mozilla Seamonkey (pages Web seulement, les Popup sont
// traitées dans swSSOWin.cpp. Eh oui...
//-----------------------------------------------------------------------------

#include "stdafx.h"

// ----------------------------------------------------------------------------------
// SearchEdgeWebDocument() [attention, fonction récursive]
// ----------------------------------------------------------------------------------
// Parcours récursif de la fenêtre et de ses objets "accessibles". On s'arrête sur le 
// premier champ document pour obtenir l'URL -- ajoutée pour ISSUE#358
// ----------------------------------------------------------------------------------
// [in] w 
// [in] pAccessible 
// ----------------------------------------------------------------------------------
#ifdef TRACES_ACTIVEES
void SearchEdgeWebDocument(char *paramszTab,IAccessible *pAccessible,T_SEARCH_DOC *ptSearchDoc)
#else
void SearchEdgeWebDocument(IAccessible *pAccessible,T_SEARCH_DOC *ptSearchDoc)
#endif
{
	TRACE((TRACE_ENTER,_F_, ""));
	HRESULT hr;
	long lCount;
	VARIANT* pArray = NULL;
	IAccessible *pChild=NULL;
	VARIANT vtChild;
	VARIANT vtState,vtRole;
	long returnCount,l;

#ifdef TRACES_ACTIVEES
	char szTab[200];
	strcpy_s(szTab,sizeof(szTab),paramszTab);
	if (strlen(szTab)<sizeof(szTab)-5) strcat_s(szTab,sizeof(szTab),"  ");
#endif
	
	if (ptSearchDoc->pContent!=NULL)  { TRACE((TRACE_DEBUG,_F_,"%sOn remonte...",szTab)); goto end; }

	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr)) goto end;
	TRACE((TRACE_DEBUG,_F_,"%sget_accChildCount()==%ld",szTab,lCount));

	if (lCount==0) { TRACE((TRACE_INFO,_F_,"%sPas de fils",szTab)); goto end; }

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
			vtChild.vt=VT_I4;
			vtChild.lVal=CHILDID_SELF;
			pChild=NULL;

			if (ptSearchDoc->pContent!=NULL)  { TRACE((TRACE_DEBUG,_F_,"%sOn sort de la boucle...",szTab)); goto end; }

			TRACE((TRACE_DEBUG,_F_,"%s---- LEVEL=%d l=%ld vt=%d lVal=0x%08lx ----",szTab,ptSearchDoc->iLevel,l,pVarCurrent->vt,pVarCurrent->lVal));
			if (pVarCurrent->vt!=VT_DISPATCH) goto suivant;
			if (pVarCurrent->lVal==NULL) goto suivant; // ISSUE#80 0.96B2 
			hr=((IDispatch*)(pVarCurrent->lVal))->QueryInterface(IID_IAccessible, (void**) &pChild);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx",szTab,hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx -> pChild=0x%08lx",szTab,hr,pChild));
			
			hr=pChild->get_accRole(vtChild,&vtRole);
			if (FAILED(hr)) 
				{ TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); }
			else
				{ TRACE((TRACE_DEBUG,_F_,"%sget_accRole() vtRole.lVal=0x%08lx",szTab,vtRole.lVal)); }

			hr=pChild->get_accState(vtChild,&vtState);
			if (FAILED(hr)) 
				{ TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); }
			else
				{ TRACE((TRACE_DEBUG,_F_,"%sget_accState() vtState.lVal=0x%08lx",szTab,vtState.lVal)); }

			{
				BSTR bstrPoub=NULL;
				hr=pChild->get_accName(vtChild,&bstrPoub);
				if (SUCCEEDED(hr)) { TRACE((TRACE_DEBUG,_F_,"get_accName()=%s",GetSZFromBSTR(bstrPoub))); }
				else { TRACE((TRACE_ERROR,_F_,"get_accName()=0x%08lx",hr)); }
				SysFreeString(bstrPoub);
			}
			{
				BSTR bstrPoub=NULL;
				hr=pChild->get_accValue(vtChild,&bstrPoub);
				if (SUCCEEDED(hr)) { TRACE((TRACE_DEBUG,_F_,"get_accValue()=%s",GetSZFromBSTR(bstrPoub))); }
				else { TRACE((TRACE_ERROR,_F_,"get_accValue()=0x%08lx",hr)); }
				SysFreeString(bstrPoub);
			}

			/*if (vtRole.lVal == ROLE_SYSTEM_PANE)
			{
				//if (!(vtState.lVal & STATE_SYSTEM_INVISIBLE)) // trouvé ! 1.17 FIX 1 (élimine les onglets autres que celui visible !)
				{
					TRACE((TRACE_DEBUG,_F_,"%sDOCUMENT TROUVE",szTab)); 
					ptSearchDoc->pContent=pChild;
					// ptSearchDoc->pContent->AddRef(); pas besoin de AddRef puisqu'il ne sera pas releasé grace au goto end
					goto end;
				}
			}
			else // if (ptSearchDoc->iLevel!=0 || vtRole.lVal == ROLE_SYSTEM_GROUPING) // 1.17 FIX 1 : optimisation, on ne cherche au niveau d'en dessous que dans le cas d'un élément groupé
			{
			}*/
			ptSearchDoc->iLevel++;
			SearchEdgeWebDocument(szTab,pChild,ptSearchDoc);
suivant:
			if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		} // for
	}
end:
	ptSearchDoc->iLevel--;
	TRACE((TRACE_LEAVE,_F_, ""));
}

/*static int CALLBACK TestProc(HWND w, LPARAM lp)
{
	int rc=true;
	char szClassName[128+1];
	HWND *pwChild;
	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"w=0x%08lx class=%s",w,szClassName));
	if (strcmp(szClassName,"Windows.UI.Core.CoreWindow")==0) { 
		rc=false; 
		pwChild=(HWND*)lp;
		*pwChild=w;
	}
	return rc;
}*/

// ----------------------------------------------------------------------------------
// GetEdgeURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fenêtre Edge
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [in] pInAccessible = si l'appelant a un pAccessible, il le passe ici
// [in] bGetAccessible = indique si on veut le pointeur pAccessible en retour
// [out] ppOutAccessible = pointeur pAccessible si demandé.
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
char *GetEdgeURL(HWND w,IAccessible *pInAccessible,BOOL bGetAccessible,IAccessible **ppOutAccessible,int iBrowser,BOOL bWaitReady)
{
	TRACE((TRACE_ENTER,_F_, ""));

	HRESULT hr;
	IDispatch *pIDispatch=NULL;
	IAccessible *pAccessible=NULL;
	ISimpleDOMDocument *pSimpleDOMDocument = NULL;
	IServiceProvider *pServiceProvider=NULL;
	BSTR bstrURL=NULL;
	char *pszURL=NULL;
	IAccessible *pContent=NULL;
	VARIANT vtMe;
	vtMe.vt=VT_I4;
	vtMe.lVal=CHILDID_SELF;
	VARIANT vtResult;
	//HWND wChild;

	T_SEARCH_DOC tSearchDoc;
		
	UNREFERENCED_PARAMETER(iBrowser);

	//EnumChildWindows(w,TestProc,(LPARAM)&wChild);
	
	if (pInAccessible!=NULL) // cool, l'appelant a fourni le pAccessible en entrée, on va gagner du temps
	{
		pContent=pInAccessible;
		pContent->AddRef(); // astuce : nécessaire sinon on va libérer dans le end le pointeur passé par l'appelant
	}
	else // l'appelant n'a pas fourni le pAccessible sur le contenu de la page
	{
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	
		tSearchDoc.pContent=NULL;
		tSearchDoc.iLevel=0;
#ifdef TRACES_ACTIVEES
		SearchEdgeWebDocument("",pAccessible,&tSearchDoc);
#else
		SearchEdgeWebDocument(pAccessible,&tSearchDoc);
#endif		
		if (tSearchDoc.pContent==NULL)
		{
			TRACE((TRACE_ERROR,_F_,"SearchWebDocument n'a pas trouvé l'objet document...")); goto end;
		}
		pContent=tSearchDoc.pContent;
	}
	if (bWaitReady)
	{
		hr=pContent->get_accState(vtMe,&vtResult);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accState()=0x%08lx",hr)); goto end; }
		if (vtResult.lVal & STATE_SYSTEM_BUSY) { TRACE((TRACE_ERROR,_F_,"get_accState() : STATE_SYSTEM_BUSY, on verra plus tard !")); goto end; }
	}

	hr=pContent->get_accName(vtMe,&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accValue()=0x%08lx",hr)); goto end; }

	pszURL=GetSZFromBSTR(bstrURL);
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",pszURL));
	
	if (bGetAccessible) // l'appelant veut récupérer le pAccessible en sortie pour usage futur, charge à lui de le libérer ensuite
	{
		*ppOutAccessible=pContent;
		pContent->AddRef(); // astuce : nécessaire pour que le ppAccessible retourné reste valide malgré le pContent->Release() du end
	}
end:
	if (bstrURL!=NULL) SysFreeString(bstrURL);
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pContent!=NULL) pContent->Release();
	if (pServiceProvider!=NULL) pServiceProvider->Release();
	if (pSimpleDOMDocument!=NULL) pSimpleDOMDocument->Release();
	if (pAccessible!=NULL) pAccessible->Release();
	
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}
