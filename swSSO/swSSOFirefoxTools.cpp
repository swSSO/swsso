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
// swSSOFirefoxTools.cpp
//-----------------------------------------------------------------------------
// Fonctions utilitaires spécifiques Firefox et Mozilla
//-----------------------------------------------------------------------------

#include "stdafx.h"

static HWND gwFirefoxPopupChild; // handle fils fenetre popup Firefox/Mozilla

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// FirefoxToolsEnumPopupChildProc()
// ----------------------------------------------------------------------------------
// Enumération des fils d'une fenêtre de classe Mozilla à la recherche 
// du fils de classe "MozillaWindowClass" pour parcours de son contenu ensuite) : 
// conserve son handle ans la globale gwFirefoxPopupChild
// ----------------------------------------------------------------------------------
static int CALLBACK FirefoxToolsEnumPopupChildProc(HWND w, LPARAM lp) 
{
	UNREFERENCED_PARAMETER(lp);
	TRACE((TRACE_ENTER,_F_, ""));

	char szClassName[128+1];
	int rc=TRUE;

	// vérification de la classe. Si c'est pas lui, on continue l'enum
	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"Classe='%s'",szClassName));
	if (strcmp(szClassName,gcszMozillaClassName)!=0) goto end;

	gwFirefoxPopupChild=w;
	// si trouvé, pas la peine de continuer l'enum
	rc=FALSE;
end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return rc; 
}


//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// 0.60 - Simulation entrée clavier
// ----------------------------------------------------------------------------------
// Utilisé uniquement pour firefox/mozilla.
// J'aurais préféré utiliser put_accValue !
// Demande d'implémentation de put_accValue faite à Aaron le 12 janvier 2005
// ----------------------------------------------------------------------------------
// [in] bErase : TRUE s'il faut vider le contenu du champ avant de le remplir
// [in] sz : chaine à saisir
// ----------------------------------------------------------------------------------
void KBSim(BOOL bErase,int iTempo,const char *sz,BOOL bPwd)
{
	UNREFERENCED_PARAMETER(bPwd); // pour ne pas avoir de warning en mode release
	TRACE((TRACE_ENTER,_F_, "bErase=%d iTempo=%d",bErase,iTempo));
	TRACE((bPwd?TRACE_PWD:TRACE_INFO,_F_, "sz='%s'",sz));

	int i,len;
	len=strlen(sz);
	BYTE hiVk,loVk;
	WORD wKeyScan;
	BOOL bCapsLock=FALSE;

	// en 1.09, déplacement du control du caps lock tout au début
	if (LOBYTE(GetKeyState(VK_CAPITAL))==1) // 0.75 : caps lock
	{
		bCapsLock=TRUE;
		keybd_event(VK_CAPITAL,LOBYTE(MapVirtualKey(VK_CAPITAL,0)),KEYEVENTF_EXTENDEDKEY | 0,0);
		keybd_event(VK_CAPITAL,LOBYTE(MapVirtualKey(VK_CAPITAL,0)),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
	}


	/*if (bErase) // pour effacer, je fais tab et shift-tab, ce qui a pour effet
		        // de sélectionner le champ complet.
	{
		Sleep(iTempo);
		keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),0,0);
		keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),KEYEVENTF_KEYUP,0);
		Sleep(iTempo);
		keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);
		keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),0,0);
		keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),KEYEVENTF_KEYUP,0);
		keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0);
	}*/
	// ISSUE#264 : changement de la technique d'effacement, on fait CTRL+A puis DEL, ça évite les changements de champs.
	keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0);
	wKeyScan=VkKeyScan('a');
	loVk=LOBYTE(wKeyScan);
	keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),0,0);
	keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),KEYEVENTF_KEYUP,0);
	keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0);
	keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),0,0);
	keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),KEYEVENTF_KEYUP,0);

	Sleep(iTempo);

	for (i=0;i<len;i++)
	{
		wKeyScan=VkKeyScan(sz[i]);
		hiVk=HIBYTE(wKeyScan);
		loVk=LOBYTE(wKeyScan);
		
		if (hiVk & 1) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);
		if (hiVk & 2) { keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0); }
		if (hiVk & 4) { keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),0,0);  } 

		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),0,0);
		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),KEYEVENTF_KEYUP,0);
		
		// ISSUE#163 : inversion des lignes pour CONTROL et MENU il relacher les touches dans le même sens sinon la touche ALT
		//             reste enfoncée (uniquement constaté dans IE9, reproduit nulle par ailleurs)
		if (hiVk & 1) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0);
		if (hiVk & 2) { keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0); } 
		if (hiVk & 4) { keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),KEYEVENTF_KEYUP,0); } 
	}

	if (bCapsLock) // 0.75 : on remet caps lock
	{
		keybd_event(VK_CAPITAL,LOBYTE(MapVirtualKey(VK_CAPITAL,0)),KEYEVENTF_EXTENDEDKEY | 0,0);
		keybd_event(VK_CAPITAL,LOBYTE(MapVirtualKey(VK_CAPITAL,0)),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
	}
	Sleep(iTempo);

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// PutAccValue()
//-----------------------------------------------------------------------------
// Remplissage d'un champ par simulation de frappe clavier puisque personne
// ne veut implémenter put_AccValue !!!
// utilisé pour tous les champs sauf mot de passe
//-----------------------------------------------------------------------------
void PutAccValue(HWND w,IAccessible *pAccessible,VARIANT index,const char *szValue)
{
	TRACE((TRACE_ENTER,_F_, "pAccessible=0x%08lx szValue=%s",pAccessible,szValue));
	
	HRESULT hr;
	BSTR bstrPreviousValue=NULL;
	BOOL bErase=FALSE;

	hr=pAccessible->get_accValue(index,&bstrPreviousValue);
	TRACE((TRACE_INFO,_F_,"pAccessible->get_accValue() : hr=0x%08lx value='%S'",hr,bstrPreviousValue));
	if (FAILED(hr))
		bErase=TRUE; // pas réussi à lire le champ, on l'efface
	else
		bErase=(SysStringLen(bstrPreviousValue)!=0); // efface si champ non vide
	SetForegroundWindow(w);
	hr=pAccessible->accSelect(SELFLAG_TAKEFOCUS,index);

	// bidouille sur 0.95 pour crédit du nord
	// 1.09B1 : a priori plus nécessaire avec le nouveau mécanisme de suppression CTRL+A+DEL, c'était le tab qui provoquait le pb
	/*if (SysStringLen(bstrPreviousValue)==17) // votre identifiant ou code confidentiel
	{
		bErase=FALSE;
		TRACE((TRACE_INFO,_F_,"bidouille credit du nord"));
	}*/
	/*if (bErase) 
	{
		KBSim(bErase,0,"",FALSE);
		hr=pAccessible->accSelect(SELFLAG_TAKEFOCUS,index);
	}*/
	
	// 0.93B1 : si %xxx%, saisie de la valeur de la variable d'environnement
	KBSim(bErase,150,GetComputedValue(szValue),FALSE);

	if (bstrPreviousValue!=NULL) SysFreeString(bstrPreviousValue);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// FirefoxGetPopupIAccessible()
//-----------------------------------------------------------------------------
// Retourne un IAccessible sur une popup firefox (3 et 4)
//-----------------------------------------------------------------------------
IAccessible *GetFirefoxPopupIAccessible(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	HRESULT hr;
	IAccessible *pAccessible=NULL;
	
	gwFirefoxPopupChild=NULL;
	EnumChildWindows(w,FirefoxToolsEnumPopupChildProc,0);
	
	// 0.93 : à partir de FF4, plus de fenêtre fille, tout doit être parcouru avec IAccessible...
	// Ne pas trouver de fille n'est donc pas une erreur, il suffit de partir de la popup elle-même
	//if (gwFirefoxPopupChild==NULL) goto end;
	if (gwFirefoxPopupChild==NULL) gwFirefoxPopupChild=w;

	// récupère pointeur IAccessible
	hr=AccessibleObjectFromWindow(gwFirefoxPopupChild,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr))
	{
		pAccessible=NULL;
		TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow()=0x%08lx",hr));
		goto end;
	}
end:
	TRACE((TRACE_LEAVE,_F_, "pAccessible=0x%08lx",pAccessible));
	return pAccessible;
}

// ----------------------------------------------------------------------------------
// GetFirefoxPopupURL() 
// ----------------------------------------------------------------------------------
// Lecture de l'URL dans les popups Firefox
// Architecture des popups (mis à jour dans 0.92B8 suite à sortie FF4B6)
// FF3 : 
// - Fenêtre détectée au niveau de Windows = MozillaDialogClass
// - Sous-fenêtre = MozillaWindowClass. Cette fenêtre a 9 ou 10 fils
// - Le champ qui contient l'URL est le 1er fils 3ème élément de la fenêtre
// FF4 : 
// - Fenêtre détectée au niveau de Windows = MozillaDialogClass
// - Sous-fenêtre = MozillaDialogClass. Cette fenêtre a 9 ou 10 fils
// - Le champ qui contient l'URL est le 1er fils du 2ème élément de la fenêtre
// 
// Du coup pour faire générique, je repère le 1er fils du 1er élément trouvé dans la
// sous-fenêtre répondant aux critères suivants :
// Role=ROLE_SYSTEM_TEXT & State est STATE_SYSTEM_FOCUSED | STATE_SYSTEM_FOCUSABLE mais
// pas STATE_SYSTEM_INVISIBLE
// ----------------------------------------------------------------------------------
// [rc] pointeur vers la chaine, à libérer par l'appelant. NULL si erreur
// ----------------------------------------------------------------------------------
char *GetFirefoxPopupURL(HWND w) 
{
	TRACE((TRACE_ENTER,_F_, ""));

	HRESULT hr;
	IAccessible *pAccessible=NULL;
	IAccessible *pAccessibleFF4=NULL;
	BSTR bstrName=NULL;
	//int  bstrLen;
	char *pszURL=NULL;
	IAccessible *pChild=NULL;
	IAccessible *pChild2=NULL;
	IDispatch *pIDispatch=NULL;
	int i;
	VARIANT vtChild2;
	BOOL bFound;
	long lCount;

	pAccessible=GetFirefoxPopupIAccessible(w);
	if (pAccessible==NULL) 
	{
		TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup"));
		goto end;
	}
	
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr)) 
	{
		TRACE((TRACE_ERROR,_F_,"pAccessible->get_accChildCount()=0x%08lx",hr));
		goto end;
	}
	TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChildCount()=%ld",lCount));
	if (lCount!=9 && lCount!=10) goto end;

	long lCount2=0;

	// Récup pointeur sur nième child répondant aux propriétés role et state attendues (cf. cartouche fonction)
	// Remarque : la lecture des roles et states des childs ne marche pas si on le fait depuis pAccessible,
	//			  il faut récupérer les pChild et faire la demande sur CHILDID_SELF... bug de FF, sans doute.
	bFound=FALSE;
	for (i=1;i<lCount && !bFound;i++) // inutile de commencer à 0, c'est CHILDID_SELF
	{
		VARIANT vtChild,vtSelf,vtRole,vtState;
		vtChild.vt=VT_I4;
		vtChild.lVal=i;
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
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole(%d)=0x%08lx",i,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accRole(%d) vtRole.lVal=0x%08lx",i,vtRole.lVal));

		if (vtRole.lVal != ROLE_SYSTEM_STATICTEXT) goto suite;

		// lecture du state de l'élément
		hr=pChild->get_accState(vtSelf,&vtState);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState(%d)=0x%08lx",i,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accState(%d) vtState.lVal=0x%08lx",i,vtState.lVal));
		
		if (vtState.lVal & STATE_SYSTEM_INVISIBLE) goto suite;
		if ((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)) // c'est lui !
		{			
			TRACE((TRACE_INFO,_F_,"Champ URL trouvé !"));
			bFound=TRUE;
		}
suite:
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (!bFound && pChild!=NULL) { pChild->Release(); pChild=NULL; }
	}

	if (pChild==NULL || !bFound) // pas trouvé, on sort, tant pis
	{
		TRACE((TRACE_ERROR,_F_,"Champ URL non trouve"));
		goto end;
	}
	hr=pChild->get_accChildCount(&lCount2);
	if (FAILED(hr)) 
	{
		TRACE((TRACE_ERROR,_F_,"pChild->get_accChildCount()=0x%08lx",hr));
	}
	TRACE((TRACE_DEBUG,_F_,"pChild->get_accChildCount()=%ld",lCount2));
	if (lCount2!=1) goto end;

	// Récup pointeur sur le 1er child du 2nd child (celui récupéré précédemment)
	vtChild2.vt=VT_I4;
	vtChild2.lVal=1; // et oui, 1 et pas 0, car le 1er est 1 (le 0 veut dire "self") !
	hr=pChild->get_accChild(vtChild2,&pIDispatch);
	TRACE((TRACE_DEBUG,_F_,"pChild->get_accChild(%ld)=0x%08lx",vtChild2.lVal,hr));
	if (FAILED(hr)) goto end;
	hr =pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChild2);
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
	if (FAILED(hr)) goto end;

	// 0.77 : lecture du 1er child du 2nd child récupéré au-dessus
	vtChild2.vt=VT_I4;
	vtChild2.lVal=0; // cette fois, 0, oui, car c'est le nom de l'objet lui-même que l'on cherche.
	hr=pChild2->get_accName(vtChild2,&bstrName);
	if (hr!=S_OK) 
	{
		TRACE((TRACE_ERROR,_F_,"pChild2->get_accName(%ld)=0x%08lx",vtChild2.lVal,hr));
		goto end;
	}
	TRACE((TRACE_DEBUG,_F_,"pChild2->get_accName(%ld)='%S'",vtChild2.lVal,bstrName));

	/* modif conformément à ISSUE#122 constatée sur IE11, explique peut-être le non fonctionnement sur certaines popup firefox
	bstrLen=SysStringLen(bstrName);
	pszURL=(char*)malloc(bstrLen+1);
	if (pszURL==NULL) 
	{
		TRACE((TRACE_ERROR,_F_,"malloc(%ld)",bstrLen+1));
		goto end; 
	}
	sprintf_s(pszURL,bstrLen+1,"%S",bstrName);
	*/
	pszURL=GetSZFromBSTR(bstrName);
	if (pszURL==NULL) goto end;

	TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));
		
end:
	if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
	if (pChild2!=NULL) { pChild2->Release(); pChild2=NULL; }
	if (pChild!=NULL) { pChild->Release(); pChild=NULL; }

	SysFreeString(bstrName);
	if (pAccessible!=NULL) pAccessible->Release();
	if (pAccessibleFF4!=NULL) pAccessibleFF4->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}
