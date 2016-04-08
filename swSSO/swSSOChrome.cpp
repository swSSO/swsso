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
// swSSOChrome.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
IAccessible *gpAccessibleChromeURL=NULL;

//-----------------------------------------------------------------------------
// GetChromePopupURL()
//-----------------------------------------------------------------------------
// Retourne le message dans la popup Chrome qui contient le texte permettant
// d'identifier l'appli : "Le serveur http://blabla demande..."
// Cette fonction est appelée systématiquement pour vérifier si la fenêtre Chrome
// en cours d'analyse est une popup ou pas, il faut donc retourner dès qu'on voit
// que ce n'est pas une popup
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
// Nouvelle fonction en 1.07 / ISSUE#215 pour popup Chrome 36+ (version approximative)
// Architecture :
// - Niveau 0 : fenêtre principale de Chrome (w)
// - Niveau 1 : fils#2
// - Niveau 2 : fils#1
// - Niveau 3 : fils#2
// - Niveau 4 : fils#1
// - Niveau 5 : fils#1 --> c'est lui qui contient le message (validé en Chrome 39 et 41)
// ----------------------------------------------------------------------------------
char *GetChromePopupURL(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HRESULT hr;
	IAccessible *pNiveau0=NULL,*pChildNiveau1=NULL,*pChildNiveau2=NULL,*pChildNiveau3=NULL,*pChildNiveau4=NULL,*pChildNiveau5=NULL;
	VARIANT vtChild;
	IDispatch *pIDispatch=NULL;
	char *pszURL=NULL;
	BSTR bstrURL=NULL;
	//BSTR bstrName=NULL;
	long lCount;

	// Récupère le niveau 0
	hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pNiveau0);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	// Vérifie qu'il y a bien au moins 2 fils
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pNiveau0->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (lCount<2) { TRACE((TRACE_DEBUG,_F_,"Niveau 0 : %d fils, on en attendait au moins 2 --> ce n'est pas une popup",lCount)); goto end; }

	// Récupère le niveau 1 (fils n°2 du niveau 0)
	vtChild.vt=VT_I4;
	vtChild.lVal=2;
	hr=pNiveau0->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau1);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Vérifie qu'il y a bien au moins 1 fils
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pChildNiveau1->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (lCount<1) { TRACE((TRACE_ERROR,_F_,"Niveau 1 : %d fils, on en attendait au moins 1",lCount)); goto end; }

	// Récupère le niveau 2 (fils n°1 du niveau 1)
	vtChild.vt=VT_I4;
	vtChild.lVal=1;
	hr=pChildNiveau1->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau2);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Vérifie qu'il y a bien au moins 2 fils
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pChildNiveau2->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (lCount<2) { TRACE((TRACE_ERROR,_F_,"Niveau 2 : %d fils, on en attendait au moins 2",lCount)); goto end; }

	// Récupère le niveau 3 (fils n°2 du niveau 2)
	vtChild.vt=VT_I4;
	vtChild.lVal=2;
	hr=pChildNiveau2->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau3);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Vérifie qu'il y a bien au moins 1 fils
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pChildNiveau3->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (lCount<1) { TRACE((TRACE_ERROR,_F_,"Niveau 3 : %d fils, on en attendait au moins 1",lCount)); goto end; }

	// Récupère le niveau 4 (fils n°1 du niveau 3)
	vtChild.vt=VT_I4;
	vtChild.lVal=1;
	hr=pChildNiveau3->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau4);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Vérifie qu'il y a bien au moins 1 fils
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pChildNiveau4->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (lCount<1) { TRACE((TRACE_ERROR,_F_,"Niveau 4 : %d fils, on en attendait au moins 1",lCount)); goto end; }

	//  Récupère le niveau 5 (fils n°1 du niveau 4)
	vtChild.vt=VT_I4;
	vtChild.lVal=1;
	hr=pChildNiveau4->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau5);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Lecture du contenu du champ : c'est notre URL
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	hr=pChildNiveau5->get_accName(vtChild,&bstrURL);
	TRACE((TRACE_DEBUG,_F_,"get_accName() hr=0x%08lx bstrURL=%S",hr,bstrURL));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accName() hr=0x%08lx",hr)); goto end; }
	pszURL=GetSZFromBSTR(bstrURL);
	SysFreeString(bstrURL);

end:
	SysFreeString(bstrURL);
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pChildNiveau1!=NULL) pChildNiveau1->Release();
	if (pChildNiveau2!=NULL) pChildNiveau2->Release();
	if (pChildNiveau3!=NULL) pChildNiveau3->Release();
	if (pChildNiveau4!=NULL) pChildNiveau4->Release();
	if (pChildNiveau5!=NULL) pChildNiveau5->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}


//-----------------------------------------------------------------------------
// GetChromeURL()
//-----------------------------------------------------------------------------
// Retourne l'URL courante de la fenêtre Chrome (pour versions inférieures à 51)
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
// Gros changement en Chrome 30, il faut maintenant naviguer avec IAccessible... --> ISSUE#93
// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
// Le child de niveau 1 a 2 childs, il faut prendre le 2eme.
// Le child de niveau 2 a 4 childs, il faut prendre le 3eme.
// Le child de niveau 3 a 3 childs, il faut prendre le 2eme.
// Le child de niveau 4 a 7 childs, il faut prendre le 5eme.
// Le child de niveau 5 a 32 childs, il faut prendre le 3eme (à partir de chrome 49 c'est le 2ème...)
// ----------------------------------------------------------------------------------
char *GetChromeURL(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HWND wURL=NULL;
	char *pszURL=NULL;
	int len;
	BSTR bstrURL=NULL;
	HRESULT hr;
	IDispatch *pIDispatch=NULL;
	IAccessible *pAccessible=NULL;
	IAccessible *pChildNiveau1=NULL, *pChildNiveau2=NULL, *pChildNiveau3=NULL, *pChildNiveau4=NULL, *pChildNiveau5=NULL, *pChildNiveau6=NULL;
	VARIANT vtChild;
	int  bstrLen;
	VARIANT vtRole;
	//long lCount;

	// recherche la fenêtre fille de classe "Chrome_AutocompleteEditView"
	// son texte contient l'URL de l'onglet en cours
	wURL=FindWindowEx(w,NULL,"Chrome_AutocompleteEditView",NULL);
	// 0.94 pour Chrome 13+, la fenêtre est de classe Chrome_OmniboxView !!
	// if (wURL==NULL) { TRACE((TRACE_ERROR,_F_,"FindWindowEx(Chrome_AutocompleteEditView)=NULL")); goto end; }
	if (wURL==NULL) 
	{ 
		TRACE((TRACE_INFO,_F_,"FindWindowEx(Chrome_AutocompleteEditView)=NULL"));
		wURL=FindWindowEx(w,NULL,"Chrome_OmniboxView",NULL);
		if (wURL==NULL) 
		{ 
			TRACE((TRACE_INFO,_F_,"FindWindowEx(Chrome_OmniboxView)=NULL")); 
			// ISSUE#93 / 0.98 : pour Chome 30+, il faut rechercher l'URL avec des IAccessible car l'URL n'est plus un champ standard Windows...
			hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
			// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
			vtChild.vt=VT_I4;
			vtChild.lVal=1;
			hr=pAccessible->get_accChild(vtChild,&pIDispatch);
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
			// Le child de niveau 2 a 4 childs, il faut prendre le 3eme.
			vtChild.vt=VT_I4;
			vtChild.lVal=3;
			hr=pChildNiveau2->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau2->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau3);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			// Le child de niveau 3 a 3 childs, il faut prendre le 2eme.
			vtChild.vt=VT_I4;
			vtChild.lVal=2;
			hr=pChildNiveau3->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau3->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau4);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			// Le child de niveau 4 a 7 childs, il faut prendre le 5eme.
			vtChild.vt=VT_I4;
			vtChild.lVal=5;
			hr=pChildNiveau4->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau4->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau5);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			// Le child de niveau 5 a 32 childs, il faut prendre le 3eme 
			// ISSUE#272 : à partir de chrome 49 c'est le 2ème. Du coup on commence par regarder le 2ème, 
			//			   si c'est bien un texte editable c'est que c'est la barre d'URL, sinon on regarde le 3ème
			vtChild.vt=VT_I4;
			vtChild.lVal=2;
			hr=pChildNiveau5->get_accChild(vtChild,&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau5->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau6);
			TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
			pIDispatch->Release(); pIDispatch=NULL;
			vtChild.vt=VT_I4;
			vtChild.lVal=0; // cette fois, 0, oui, car c'est le nom de l'objet lui-même que l'on cherche.
			// ISSUE#272: vérification du rôle 
			hr=pChildNiveau6->get_accRole(vtChild,&vtRole);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChildNiveau6(2)->get_accRole()=0x%08lx",hr)); goto end; }
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau6(2)->get_accRole()=0x%08lx",vtRole.lVal));
			if (vtRole.lVal != ROLE_SYSTEM_TEXT) // si pas rôle texte, on essaie le suivant
			{
				pChildNiveau6->Release();
				vtChild.lVal=3;
				hr=pChildNiveau5->get_accChild(vtChild,&pIDispatch);
				TRACE((TRACE_DEBUG,_F_,"pChildNiveau5->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
				hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau6);
				TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
				pIDispatch->Release(); pIDispatch=NULL;
				vtChild.vt=VT_I4;
				vtChild.lVal=0; // cette fois, 0, oui, car c'est le nom de l'objet lui-même que l'on cherche.
			}
			hr=pChildNiveau6->get_accValue(vtChild,&bstrURL);
			if (hr!=S_OK) { TRACE((TRACE_ERROR,_F_,"pChildNiveau6->get_accValue(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
			TRACE((TRACE_DEBUG,_F_,"pChildNiveau6->get_accValue(%ld)='%S'",vtChild.lVal,bstrURL));
			bstrLen=SysStringLen(bstrURL);
			pszURL=(char*)malloc(bstrLen+1);
			if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",bstrLen+1)); goto end; }
			sprintf_s(pszURL,bstrLen+1,"%S",bstrURL);
			TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));
			gpAccessibleChromeURL=pChildNiveau6;
			pChildNiveau6->AddRef();
		}
	}
	if (wURL!=NULL)
	{
		TRACE((TRACE_DEBUG,_F_,"wURL=0x%08lx",wURL));
		// lecture taille URL (=longueur texte de la fenêtre trouvée)
		len=SendMessage(wURL,WM_GETTEXTLENGTH,0,0);
		TRACE((TRACE_DEBUG,_F_,"lenURL=%d",len));
		if (len==0) { TRACE((TRACE_ERROR,_F_,"URL vide")); goto end; }
		// allocation pszURL
		pszURL=(char*)malloc(len+1);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",len+1)); goto end; }
		// lecture texte fenêtre
		SendMessage(wURL,WM_GETTEXT,len+1,(LPARAM)pszURL);
		TRACE((TRACE_DEBUG,_F_,"URL=%s",pszURL));
	}
end:
	SysFreeString(bstrURL);
	if (pAccessible!=NULL) pAccessible->Release();
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pChildNiveau1!=NULL) pChildNiveau1->Release();
	if (pChildNiveau2!=NULL) pChildNiveau2->Release();
	if (pChildNiveau3!=NULL) pChildNiveau3->Release();
	if (pChildNiveau4!=NULL) pChildNiveau4->Release();
	if (pChildNiveau5!=NULL) pChildNiveau5->Release();
	if (pChildNiveau6!=NULL) pChildNiveau6->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

//-----------------------------------------------------------------------------
// GetChromeURL51()
//-----------------------------------------------------------------------------
// Retourne l'URL courante de la fenêtre Chrome (pour versions 51+)
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
// Le child de niveau 1 a 2 childs, il faut prendre le 2eme.
// Le child de niveau 2 a 5 childs, il faut prendre le 2eme.
// Le child de niveau 3 a 3 childs, il faut prendre le 3eme.
// Le child de niveau 4 a 7 childs, il faut prendre le 5eme.
// Le child de niveau 5 a 32 childs, il faut prendre le 2eme 
// 
// ----------------------------------------------------------------------------------
char *GetChromeURL51(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszURL=NULL;
	BSTR bstrURL=NULL;
	HRESULT hr;
	IDispatch *pIDispatch=NULL;
	IAccessible *pAccessible=NULL;
	IAccessible *pChildNiveau1=NULL, *pChildNiveau2=NULL, *pChildNiveau3=NULL, *pChildNiveau4=NULL, *pChildNiveau5=NULL, *pChildNiveau6=NULL;
	VARIANT vtChild;
	int  bstrLen;

	hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	// La fenêtre principale a 1 child de niveau 1, il faut prendre le 1er.
	vtChild.vt=VT_I4;
	vtChild.lVal=1;
	hr=pAccessible->get_accChild(vtChild,&pIDispatch);
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
	// Le child de niveau 2 a 5 childs, il faut prendre le 2eme.
	vtChild.vt=VT_I4;
	vtChild.lVal=2;
	hr=pChildNiveau2->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pChildNiveau2->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau3);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Le child de niveau 3 a 3 childs, il faut prendre le 3eme.
	vtChild.vt=VT_I4;
	vtChild.lVal=3;
	hr=pChildNiveau3->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pChildNiveau3->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau4);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Le child de niveau 4 a 7 childs, il faut prendre le 5eme.
	vtChild.vt=VT_I4;
	vtChild.lVal=5;
	hr=pChildNiveau4->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pChildNiveau4->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau5);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	// Le child de niveau 5 a 32 childs, il faut prendre le 3eme 
	// ISSUE#272 : à partir de chrome 49 c'est le 2ème. Du coup on commence par regarder le 2ème, 
	//			   si c'est bien un texte editable c'est que c'est la barre d'URL, sinon on regarde le 3ème
	vtChild.vt=VT_I4;
	vtChild.lVal=2;
	hr=pChildNiveau5->get_accChild(vtChild,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pChildNiveau5->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChildNiveau6);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	pIDispatch->Release(); pIDispatch=NULL;
	vtChild.vt=VT_I4;
	vtChild.lVal=0; // cette fois, 0, oui, car c'est le nom de l'objet lui-même que l'on cherche.
	
	hr=pChildNiveau6->get_accValue(vtChild,&bstrURL);
	if (hr!=S_OK) { TRACE((TRACE_ERROR,_F_,"pChildNiveau6->get_accValue(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"pChildNiveau6->get_accValue(%ld)='%S'",vtChild.lVal,bstrURL));
	
	bstrLen=SysStringLen(bstrURL);
	pszURL=(char*)malloc(bstrLen+1);
	if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",bstrLen+1)); goto end; }
	sprintf_s(pszURL,bstrLen+1,"%S",bstrURL);
	TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));
	gpAccessibleChromeURL=pChildNiveau6;
	pChildNiveau6->AddRef();

end:
	SysFreeString(bstrURL);
	if (pAccessible!=NULL) pAccessible->Release();
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pChildNiveau1!=NULL) pChildNiveau1->Release();
	if (pChildNiveau2!=NULL) pChildNiveau2->Release();
	if (pChildNiveau3!=NULL) pChildNiveau3->Release();
	if (pChildNiveau4!=NULL) pChildNiveau4->Release();
	if (pChildNiveau5!=NULL) pChildNiveau5->Release();
	if (pChildNiveau6!=NULL) pChildNiveau6->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

typedef struct 
{
	HWND w;
	char szClassName[128+1]; // classe de fenêtre recherchée
	char szExclude[128+1]; // classe de fenêtre à exclure : si trouvée, on arrête l'énum avec retour null
} T_SUIVI_NEW_CHROME_URL;

// ----------------------------------------------------------------------------------
// NewChromeURLEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils à la recherche de la fenêtre de rendu de Chrome pour lire l'URL (ISSUE#273)
// ----------------------------------------------------------------------------------
static int CALLBACK NewChromeURLEnumChildProc(HWND w, LPARAM lp)
{
	int rc=TRUE;
	char szClassName[50+1];

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
	if (strcmp(szClassName,((T_SUIVI_NEW_CHROME_URL*)lp)->szExclude)==0)
	{
		((T_SUIVI_NEW_CHROME_URL*)lp)->w=NULL;
		TRACE((TRACE_INFO,_F_,"Fenetre classe=%s w=0x%08lx --> stoppe l'enumeration, retourne NULL",szClassName,w));
		rc=FALSE;
	}
	else if (strcmp(szClassName,((T_SUIVI_NEW_CHROME_URL*)lp)->szClassName)==0)
	{
		((T_SUIVI_NEW_CHROME_URL*)lp)->w=w;
		TRACE((TRACE_INFO,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
		rc=FALSE;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// NewGetChromeURL()
// ----------------------------------------------------------------------------------
// Nouvelle fonction de lecture d'URL Chrome (ISSUE#273)
// N'est pas encore utilisée (en 1.10) car rend inopérante la bidouille nécessaire
// au contournement du bug empechant de mettre le focus sur un champ de la page web
// lorsque le focus est dans la barre d'URL
// ----------------------------------------------------------------------------------
char *NewGetChromeURL(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	T_SUIVI_NEW_CHROME_URL tSuivi;
	char *pszURL=NULL;
	HRESULT hr;
	IAccessible *pAccessible=NULL;
	BSTR bstrURL=NULL;
	
	// recherche le document 
	strcpy_s(tSuivi.szExclude,sizeof(tSuivi.szExclude),"Static");
	strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"Chrome_RenderWidgetHostHWND");
	EnumChildWindows(w,NewChromeURLEnumChildProc,(LPARAM)&tSuivi);
	if (tSuivi.w==NULL) { TRACE((TRACE_ERROR,_F_,"Fenetre Chrome_RenderWidgetHostHWND non trouvee")); goto end; }
	// Obtient un IAccessible
	hr=AccessibleObjectFromWindow(tSuivi.w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	// vérifie que c'est bien le ROLE_SYSTEM_DOCUMENT
	VARIANT vtMe,vtRole;
	vtMe.vt=VT_I4;
	vtMe.lVal=CHILDID_SELF;
	hr=pAccessible->get_accRole(vtMe,&vtRole);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_accRole() vtRole.lVal=0x%08lx",vtRole.lVal));
	if (vtRole.lVal!=ROLE_SYSTEM_DOCUMENT) { TRACE((TRACE_ERROR,_F_,"get_accRole()!=ROLE_SYSTEM_DOCUMENT")); goto end; }
	// si OK, récupère la value qui contient l'URL
	hr=pAccessible->get_accValue(vtMe,&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accValue() hr=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_accValue() hr=0x%08lx bstrURL=%S",hr,bstrURL));
	pszURL=GetSZFromBSTR(bstrURL);

end:
	SysFreeString(bstrURL);
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

// conservé au cas où, mais plus utilisé depuis 1.07 B4 (cf. ISSUE#215)
#if 0
typedef struct 
{
	int iAction;
	HWND w;
} T_CHROMEFINDPOPUP;

//-----------------------------------------------------------------------------
// ChromeFindPopupProc()
//-----------------------------------------------------------------------------
// Enumération des fils de la fenêtre principale de chrome à la recherche
// d'une popup d'authentification
//-----------------------------------------------------------------------------
static int CALLBACK ChromeFindPopupProc(HWND w, LPARAM lp)
{
	int rc=TRUE;
	char szClassName[128+1]; 
	char szTitle[512+1];
	T_CHROMEFINDPOPUP *ptChromeFindPopup;

	ptChromeFindPopup=(T_CHROMEFINDPOPUP *)lp;

	GetClassName(w,szClassName,sizeof(szClassName));
	// ISSUE#77 : Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	if (strncmp(szClassName,"Chrome_WidgetWin_",17)==0)
	{
		GetWindowText(w,szTitle,sizeof(szTitle));
		TRACE((TRACE_DEBUG,_F_,"szTitle=%s iAction=%d",szTitle,ptChromeFindPopup->iAction));
		if (ptChromeFindPopup->iAction==-1) 
		{
			if (strcmp(szTitle,"Authentification requise")==0) rc=FALSE; // trouvé, on arrête l'énum
		}
		else
		{
			if (swStringMatch(szTitle,gptActions[ptChromeFindPopup->iAction].szTitle)) rc=FALSE; // trouvé, on arrête l'énum
		}
		if (!rc) 
		{ 
			TRACE((TRACE_DEBUG,_F_,"Popup trouvee w=0x%08lx",w));
			ptChromeFindPopup->w=w;
		}
	}
	return rc;
}

//-----------------------------------------------------------------------------
// GetChromePopupHandle()
//-----------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [in] iAction = config SSO concernée, si -1, utilise les titres de popup en dur
// [rc] TRUE si la fenêtre héberge une popup d'authentification Chrome 
//-----------------------------------------------------------------------------
HWND GetChromePopupHandle(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d",w,iAction));
	HWND rc=NULL;

	T_CHROMEFINDPOPUP tChromeFindPopup;
	tChromeFindPopup.iAction=iAction;
	tChromeFindPopup.w=NULL;
	
	EnumChildWindows(w,ChromeFindPopupProc,(LPARAM)&tChromeFindPopup);

	rc=tChromeFindPopup.w;

	TRACE((TRACE_LEAVE,_F_, "rc=0x%08lx",rc));
	return rc;
}	

#endif