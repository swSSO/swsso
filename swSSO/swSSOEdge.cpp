//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2018 - Sylvain WERDEFROY
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
// swSSOEdge.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"

// ----------------------------------------------------------------------------------
// GetEdgeURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fenêtre Edge
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
char *GetEdgeURL(HWND w,IUIAutomationElement **ppDocument)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	HRESULT hr;
	char *pszURL=NULL;
	BSTR bstrURL=NULL;
	VARIANT varProp;
	varProp.vt=VT_EMPTY;
	
	IUIAutomationCondition *pCondition = NULL;				
	IUIAutomationElement *pEdge = NULL;

	if (gpIUIAutomation==NULL) goto end;

	hr=gpIUIAutomation->ElementFromHandle(w,&pEdge);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"gpIUIAutomation->ElementFromHandle()=0x%08lx",hr)); goto end; }
	varProp.vt=VT_BSTR;
	varProp.bstrVal=SysAllocString(L"Internet Explorer_Server");
	if (varProp.bstrVal==NULL) { TRACE((TRACE_ERROR,_F_,"SysAllocString()")); goto end; }
	hr=gpIUIAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId,varProp,&pCondition);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"gpIUIAutomation->CreatePropertyCondition()=0x%08lx",hr)); goto end; }
	hr=pEdge->FindFirst(TreeScope_Descendants,pCondition,ppDocument);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pEdge->FindFirst()=0x%08lx",hr)); goto end; }
	// ISSUE#393 : ppDocument peut être NULL même si hr=S_OK.
	if (*ppDocument==NULL) { TRACE((TRACE_ERROR,_F_,"pEdge->FindFirst, rien trouve"));; goto end; }
	hr=(*ppDocument)->get_CurrentName(&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"ppDocument->ElementFromHandle()=0x%08lx",hr)); goto end; }
	pszURL=GetSZFromBSTR(bstrURL);
	if (pszURL==NULL) goto end;
	TRACE((TRACE_DEBUG,_F_,"URL=%s",pszURL));
end:
	if (varProp.vt!=VT_EMPTY) VariantClear(&varProp);
	if (pEdge != NULL) pEdge->Release();
	if (pCondition != NULL) pCondition->Release();
	SysFreeString(bstrURL);
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

// ----------------------------------------------------------------------------------
// SSOWebUIA()
// ----------------------------------------------------------------------------------
// SSO commun aux navigateurs implémentant l'interface UIAutomation
// Utilise forcément la méthode de configuration simplifiée
// ----------------------------------------------------------------------------------
// [out] rc :
//  0 = OK
// -1 = pas réussi (champs non trouvés ou autre erreur)
// -3 = libellé szId4Value non trouvé dans la page
// -4 = annulation utilisateur (dans la fenêtre de choix de compte)
// ----------------------------------------------------------------------------------
int SSOWebUIA(HWND w,int *piAction,int iBrowser,IUIAutomationElement *pDocument)
{
	UNREFERENCED_PARAMETER(w);
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	//IUIAutomationLegacyIAccessiblePattern *pIAccessiblePattern=NULL;
	HRESULT hr;
	UIA_HWND uiaw=NULL;

	//gpEdgeIAccessible=NULL;
	if (pDocument==NULL) { TRACE((TRACE_ERROR,_F_,"pDocument=NULL")); goto end; }

	/*
	hr=pDocument->GetCurrentPattern(UIA_LegacyIAccessiblePatternId,(IUnknown**)&pIAccessiblePattern);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"GetCurrentPattern(UIA_LegacyIAccessiblePatternId)=0x%08lx",hr)); goto end; }
	
	hr=pIAccessiblePattern->get_CurrentName(&bstr);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_CurrentName()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_CurrentName=%S",bstr));

	hr=pIAccessiblePattern->GetIAccessible(&gpEdgeIAccessible);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"GetIAccessible()=0x%08lx",hr)); goto end; }

	*/
	hr=pDocument->get_CurrentNativeWindowHandle(&uiaw);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_CurrentNativeWindowHandle()=0x%08lx",hr)); goto end; }
	
	rc=SSOWebAccessible((HWND)uiaw,piAction,iBrowser);

end:
	//if (pIAccessiblePattern!=NULL) pIAccessiblePattern->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
