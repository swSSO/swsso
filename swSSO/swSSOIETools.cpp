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
// swSSOIETools.cpp
//-----------------------------------------------------------------------------
// Fonctions utilitaires sp�cifiques IE
//-----------------------------------------------------------------------------

#include "stdafx.h"

static HWND gwW7PopupChild;
char gcszK4[]="44444444";

//***********************************************************************************
//                             FONCTIONS PRIVEES
//***********************************************************************************

//-----------------------------------------------------------------------------
// CheckIEFrameURL() [attention, fonction r�cursive]
//-----------------------------------------------------------------------------
// IHTMLDocument2 <=================================
// -> get_URL									   |
// -> QueryInterface(IID_IOleContainer)            |
//		-> EnumObjects, puis sur chaque objet :	   |
// 			-> QueryInterface(IID_IWebBrowser2)    |
// 				-> get_LocationURL                 |
// 				-> get_Document ====================
//-----------------------------------------------------------------------------
static int CheckIEFrameURL(IHTMLDocument2 *pHTMLDocument2,T_CHECKURL *ptCheckURL)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=true; // true=continuer l'�num�ration
	HRESULT hr;
	BSTR bstrURL=NULL;
	IOleContainer *pIOleContainer=NULL;
	IEnumUnknown *pIEnum=NULL;
	IUnknown *pIUnknown=NULL;
	IWebBrowser2 *pIWebBrowser2=NULL;
	IDispatch *pIDispatch=NULL;

	// termine la r�cursivit� d�s qu'une URL qui match a �t� trouv�e
	if (ptCheckURL->pszURL!=NULL) 
	{ 
		TRACE((TRACE_DEBUG,_F_,"URL trouv�e, on remonte !")); 
		rc=false;
		goto end; 
	}

	// �num�ration des frames de la page
	hr=pHTMLDocument2->QueryInterface(IID_IOleContainer,(void**)&pIOleContainer);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pHTMLDocument2->QueryInterface(IID_IOleContainer)=0x%08lx",hr)); goto end; }
	hr=pIOleContainer->EnumObjects(OLECONTF_EMBEDDINGS,&pIEnum);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pIOleContainer->EnumObjects(OLECONTF_EMBEDDINGS)=0x%08lx",hr)); goto end; }
	while (!FAILED(pIEnum->Next(1,&pIUnknown,NULL)))
	{
		TRACE((TRACE_DEBUG,_F_,"pIEnum->Next() OK, pIUnknown=0x%08lx",pIUnknown));
		if (pIUnknown==NULL) goto end; // �num�ration termin�e
		hr=pIUnknown->QueryInterface(IID_IWebBrowser2,(void**)&pIWebBrowser2);
		if (SUCCEEDED(hr)) 
		{
			// lit l'URL du document
			hr=pIWebBrowser2->get_LocationURL(&bstrURL);
			if (hr==S_OK && bstrURL!=NULL) 
			{ 
				TRACE((TRACE_DEBUG,_F_,"bstrURL=%S",bstrURL));
				ptCheckURL->pszURL=GetSZFromBSTR(bstrURL);
				if (ptCheckURL->pszURL==NULL) goto end;
				TRACE((TRACE_DEBUG,_F_,"pszURL =%s",ptCheckURL->pszURL));
				// v�rifie si elle matche avec la config iAction
				if (swURLMatch(ptCheckURL->pszURL,gptActions[ptCheckURL->iAction].szURL))
				{
					rc=false; // c'est bon, c'est elle, on arr�te l'�num�ration, l'appelant lib�rera ptCheckURL->pszURL
					goto end;
				}
				// ne matche pas, on lib�re et on continue...
				free(ptCheckURL->pszURL); ptCheckURL->pszURL=NULL;
				SysFreeString(bstrURL); bstrURL=NULL;
			}
			// parcours les �ventuelles frames du document
			hr=pIWebBrowser2->get_Document(&pIDispatch);
			TRACE((TRACE_DEBUG,_F_,"pIWebBrowser2->get_Document()=0x%08lx",hr)); 
			if (hr==S_OK)
			{ 
				IHTMLDocument2 *pHTMLDocument2NextLevel=NULL;
				hr=pIDispatch->QueryInterface(IID_IHTMLDocument2,(void**)&pHTMLDocument2NextLevel);
				TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IHTMLDocument2)=0x%08lx",hr));
				if (hr==S_OK) 
				{
					rc=CheckIEFrameURL(pHTMLDocument2NextLevel,ptCheckURL);
					pHTMLDocument2NextLevel->Release(); pHTMLDocument2NextLevel=NULL;
					if (rc==false) goto end;
				}
			}
		}
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pIWebBrowser2!=NULL) { pIWebBrowser2->Release(); pIWebBrowser2=NULL; }
		if (pIUnknown!=NULL) { pIUnknown->Release(); pIUnknown=NULL; }
	} // while
end:
	SysFreeString(bstrURL); 
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	if (pIWebBrowser2!=NULL) pIWebBrowser2->Release();
	if (pIUnknown!=NULL) pIUnknown->Release(); 
	if (pIEnum!=NULL) pIEnum->Release();
	if (pIOleContainer!=NULL) pIOleContainer->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// CheckIEURLEnumChildProc()
//-----------------------------------------------------------------------------
// Enum�ration des fils de la fen�tre navigateur � la recherche de l'URL 
// de la page web et des iframes de la page. S'arr�te quand une URL matche
// avec l'action pass�e en param�tre dans ptCheckURL->iAction
//-----------------------------------------------------------------------------
static int CALLBACK CheckIEURLEnumChildProc(HWND w, LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	int rc=true; // true=continuer l'�num�ration
	char szClassName[128+1];
	DWORD dw;
	HRESULT hr;
	IHTMLDocument2 *pHTMLDocument2=NULL;
	BSTR bstrURL=NULL;
	BSTR bstrState=NULL;

	T_CHECKURL *ptCheckURL=(T_CHECKURL *)lp;

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"w=0x%08lx class=%s",w,szClassName));
	if (strcmp(szClassName,"Internet Explorer_Server")!=0) goto end;

	// r�cup�ration pointeur sur le document HTML (interface IHTMLDocument2)
	SendMessageTimeout(w,guiHTMLGetObjectMsg,0L,0L,SMTO_ABORTIFHUNG,1000,&dw);
	hr=ObjectFromLresult(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"ObjectFromLresult(%d,IID_IHTMLDocument2)=0x%08lx",dw,hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"ObjectFromLresult(IID_IHTMLDocument2)=%08lx pHTMLDocument2=%08lx",hr,pHTMLDocument2));

	// v�rifie si la page est charg�e
	if (ptCheckURL->bWaitReady)
	{
		hr=pHTMLDocument2->get_readyState(&bstrState);
		if (FAILED(hr)) 
		{ 
			// ca n'a pas march�, pas grave, on continue quand m�me
			TRACE((TRACE_ERROR,_F_,"get_readyState=0x%08lx",hr)); 
		}
		else
		{
			TRACE((TRACE_INFO,_F_,"readyState=%S",bstrState)); 
			if (!CompareBSTRtoSZ(bstrState,"complete")) { rc=false; goto end; } // pas fini de charger, on arr�te
		}
	}
	// r�cup�re l'URL du document charg�
	hr=pHTMLDocument2->get_URL(&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_URL()=0x%08lx",hr)); goto end; }
	ptCheckURL->pszURL=GetSZFromBSTR(bstrURL);
	if (ptCheckURL->pszURL==NULL) goto end;
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",ptCheckURL->pszURL));
	// On �limine les URLs inutiles
	if (_strnicmp(ptCheckURL->pszURL,"res://",6)==0 || _strnicmp(ptCheckURL->pszURL,"about:",6)==0)
	{
		TRACE((TRACE_DEBUG,_F_,"%s : on passe � une autre child window !",ptCheckURL->pszURL));
		free(ptCheckURL->pszURL); ptCheckURL->pszURL=NULL;
		goto end;
	}
	// v�rifie si elle matche avec la config iAction
	if (swURLMatch(ptCheckURL->pszURL,gptActions[ptCheckURL->iAction].szURL))
	{
		rc=false; // c'est bon, c'est elle, on arr�te l'�num�ration, l'appelant lib�rera ptCheckURL->pszURL
		goto end;
	}
	// On lib�re l'URL trouv�e puisqu'elle ne matche avec rien
	free(ptCheckURL->pszURL); ptCheckURL->pszURL=NULL;
	// Inception, on parse les frames
	rc=CheckIEFrameURL(pHTMLDocument2,ptCheckURL);

end:
	if (pHTMLDocument2!=NULL) pHTMLDocument2->Release();
	SysFreeString(bstrState);
	SysFreeString(bstrURL);
	return rc;
}

//-----------------------------------------------------------------------------
// GetIEURLEnumChildProc()
//-----------------------------------------------------------------------------
// Enum�ration des fils de la fen�tre navigateur � la recherche 
// de la barre d'adresse pour v�rifier l'URL -> chang� en 0.83, voir dans swSSOWeb.cpp
//-----------------------------------------------------------------------------
// [in] lp = &psz
//-----------------------------------------------------------------------------
static int CALLBACK GetIEURLEnumChildProc(HWND w, LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	int rc=true; // true=continuer l'�num�ration
	char szClassName[128+1];
	DWORD dw;
	HRESULT hr;
	IHTMLDocument2 *pHTMLDocument2=NULL;
	BSTR bstrURL=NULL;
	BSTR bstrState=NULL;

	T_GETURL *ptGetURL=(T_GETURL *)lp;

	// 0.85B6 : je supprime la comparaison du titre car ne fonctionne pas quand le titre est trop long !
	//          (exemple copaindavant -> titre fen�tre windows tronqu� vs titre html entier, incomparable donc)

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"w=0x%08lx class=%s",w,szClassName));
	if (strcmp(szClassName,"Internet Explorer_Server")!=0) goto end;

	// r�cup�ration pointeur sur le document HTML (interface IHTMLDocument2)
	SendMessageTimeout(w,guiHTMLGetObjectMsg,0L,0L,SMTO_ABORTIFHUNG,1000,&dw);
	// hr=(*gpfObjectFromLresult)(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2); // 1.12B3-TI-TIE4
	hr=ObjectFromLresult(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2);
	if (FAILED(hr)) 
	{
		TRACE((TRACE_ERROR,_F_,"ObjectFromLresult(%d,IID_IHTMLDocument2)=0x%08lx",dw,hr));
		goto end;
	}
   	TRACE((TRACE_DEBUG,_F_,"ObjectFromLresult(IID_IHTMLDocument2)=%08lx pHTMLDocument2=%08lx",hr,pHTMLDocument2));

	// 0.90 : ne commence pas tant que le document n'est pas charg�
	//        (uniquement dans le cas d'une simulation de frappe clavier ?)
	// 0.97 : on ne le fait que si bWaitReady (et notamment on ne le fait pas dans le cas des popups cf. ISSUE#87)
	if (ptGetURL->bWaitReady)
	{
		hr=pHTMLDocument2->get_readyState(&bstrState);
		if (FAILED(hr)) 
		{ 
			TRACE((TRACE_ERROR,_F_,"get_readyState=0x%08lx",hr)); 
			// ca n'a pas march�, pas grave, on continue quand m�me
		}
		else
		{
			TRACE((TRACE_INFO,_F_,"readyState=%S",bstrState)); 
			if (!CompareBSTRtoSZ(bstrState,"complete")) { rc=false; goto end; } // pas fini de charger, on arr�te
		}
	}
	hr=pHTMLDocument2->get_URL(&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_URL()=0x%08lx",hr)); goto end; }
	
	ptGetURL->pszURL=GetSZFromBSTR(bstrURL);
	if (ptGetURL->pszURL==NULL) goto end;
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",ptGetURL->pszURL));

	// ISSUE#312 : si la console debug F12 est ouverte, elle apparait en premier dans l'�num�ration des fen�tres.
	//             Il faut l'ignorer et continuer l'�num�ration
	if (_strnicmp(ptGetURL->pszURL,"res://",6)==0 || _strnicmp(ptGetURL->pszURL,"about:",6)==0)
	{
		TRACE((TRACE_DEBUG,_F_,"%s : on passe � une autre child window !",ptGetURL->pszURL));
		free(ptGetURL->pszURL); ptGetURL->pszURL=NULL;
		goto end;
	}
	rc=false; // trouv� l'URL, on arrete l'enum

end:
	if (bstrState!=NULL) SysFreeString(bstrState);
	if (pHTMLDocument2!=NULL) pHTMLDocument2->Release();
	if (bstrURL!=NULL) SysFreeString(bstrURL);
	return rc;
}

//***********************************************************************************
//                             FONCTIONS PUBLIQUES
//***********************************************************************************

// ----------------------------------------------------------------------------------
// CheckIEURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL qui matche avec la configuration pass�e en param�tre
// ----------------------------------------------------------------------------------
// [out] pszURL (� lib�rer par l'appelant) ou NULL si erreur
// ----------------------------------------------------------------------------------

char *CheckIEURL(HWND w, BOOL bWaitReady,int iAction)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	T_CHECKURL tCheckURL;
	tCheckURL.pszURL=NULL;
	tCheckURL.bWaitReady=bWaitReady;
	tCheckURL.iAction=iAction;

	EnumChildWindows(w,CheckIEURLEnumChildProc,(LPARAM)&tCheckURL);

	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",tCheckURL.pszURL));
	return tCheckURL.pszURL;
}
// ----------------------------------------------------------------------------------
// GetIEURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fen�tre IE
// ----------------------------------------------------------------------------------
// [out] pszURL (� lib�rer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------

char *GetIEURL(HWND w, BOOL bWaitReady)
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	T_GETURL tGetURL;
	tGetURL.pszURL=NULL;
	tGetURL.bWaitReady=bWaitReady;

	EnumChildWindows(w,GetIEURLEnumChildProc,(LPARAM)&tGetURL);

	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",tGetURL.pszURL));
	return tGetURL.pszURL;
}

// ----------------------------------------------------------------------------------
// GetMaxthonURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fen�tre MAXTHON
// ----------------------------------------------------------------------------------
// [out] pszURL (� lib�rer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
char *GetMaxthonURL(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HWND wMaxthonView;
	char *pszURL=NULL;
	
	// c'est un peu compliqu�, parce que la fen�tre de navigation (Internet Explorer_Server)
	// n'est pas fille de la fen�tre ppale de maxthon ! Il faut donc commencer trouver
	// la fen�tre m�re (facile elle a la classe Maxthon_View) puis appeler GetIEURL() avec 
	// le handle de cette fen�tre au lieu de celle re�ue en param�tre.
	wMaxthonView=FindWindow("Maxthon2_View",NULL);
	if (wMaxthonView==NULL)
	{
		TRACE((TRACE_ERROR,_F_,"Fen�tre de classe Maxthon2_View non trouv�e"));
		goto end;
	}
	pszURL=GetIEURL(wMaxthonView,TRUE);
end:
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

// ----------------------------------------------------------------------------------
// GetIEWindowTitle()
// ----------------------------------------------------------------------------------
// Retourne le Window Title de IE (lecture en base de registre)
// La valeur retourn�e commence par espace-tiret-espace
// ----------------------------------------------------------------------------------
// NULL si non trouv�.
// ----------------------------------------------------------------------------------

char *GetIEWindowTitle(void)
{
	TRACE((TRACE_ENTER,_F_, ""));

	char *pszTitle=NULL;
	HKEY hKey=NULL;
	int rc;
	DWORD dwValueSize,dwValueType;

	rc=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Internet Explorer\\Main",0,KEY_READ,&hKey);
	if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegOpenKeyEx(UKCU)")); goto end;}

	pszTitle=(char*)malloc(256);
	if (pszTitle==NULL) goto end;
	pszTitle[0]=' ';
	pszTitle[1]='-';
	pszTitle[2]=' ';
	dwValueType=REG_SZ;
	dwValueSize=250;
	rc=RegQueryValueEx(hKey,"Window Title",NULL,&dwValueType,(LPBYTE)pszTitle+3,&dwValueSize);
	if (rc!=ERROR_SUCCESS) { free(pszTitle) ; pszTitle=NULL; goto end; }
end:
	if (hKey!=NULL) RegCloseKey(hKey);
	TRACE((TRACE_LEAVE,_F_,"pszTitle=0x%08lx",pszTitle));
	return pszTitle;
}

// ----------------------------------------------------------------------------------
// W7PopupEnumChildProc()
// ----------------------------------------------------------------------------------
// Enum�ration des fils de la popup W7 � la recherche de la fen�tre de classe
// DirectUIHWND qui contient les champs accessibles avec IAccessible
// ----------------------------------------------------------------------------------
static int CALLBACK W7PopupEnumChildProc(HWND w, LPARAM lp) 
{
	UNREFERENCED_PARAMETER(lp);
	TRACE((TRACE_ENTER,_F_, ""));

	char szClassName[128+1];
	int rc=TRUE;
	
	// v�rification de la classe. Si c'est pas lui, on continue l'enum
	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"Classe='%s'",szClassName));
	if (strcmp(szClassName,"DirectUIHWND")!=0) goto end;
	gwW7PopupChild=w;
	// si trouv�, pas la peine de continuer l'enum
	rc=FALSE;
end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return rc; 
}

//-----------------------------------------------------------------------------
// GetW7PopupIAccessible()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IAccessible *GetW7PopupIAccessible(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	HRESULT hr;
	IAccessible *pAccessible=NULL;

	gwW7PopupChild=NULL;
	EnumChildWindows(w,W7PopupEnumChildProc,0);
	if (gwW7PopupChild==NULL) goto end;
	TRACE((TRACE_DEBUG,_F_,"w=0x%08lx gwW7PopupChild=0x%08lx",w,gwW7PopupChild));

	// r�cup�re pointeur IAccessible
	hr=AccessibleObjectFromWindow(gwW7PopupChild,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow()=0x%08lx",hr));
		goto end;
	}
end:
	TRACE((TRACE_LEAVE,_F_, "pAccessible=0x%08lx",pAccessible));
	return pAccessible;
}

//-----------------------------------------------------------------------------
// GetW10PopupIAccessible()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IAccessible *GetW10PopupIAccessible(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	HRESULT hr;
	IAccessible *pAccessible=NULL;

	// r�cup�re pointeur IAccessible
	hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow()=0x%08lx",hr));
		goto end;
	}
end:
	TRACE((TRACE_LEAVE,_F_, "pAccessible=0x%08lx",pAccessible));
	return pAccessible;
}

// ----------------------------------------------------------------------------------
// GetW7PopupURL()
// ----------------------------------------------------------------------------------
// lecture de l'URL dans les popups IE dans W7
// ----------------------------------------------------------------------------------
// [rc] pointeur vers la chaine, � lib�rer par l'appelant. NULL si erreur
// ----------------------------------------------------------------------------------
char *GetW7PopupURL(HWND w) 
{
	TRACE((TRACE_ENTER,_F_, ""));

	HRESULT hr;
	IAccessible *pAccessible=NULL;
	VARIANT vtSelf,vtChild;
	VARIANT vtRole;
	BSTR bstrName=NULL;
	char *pszURL=NULL;
	IAccessible *pChild=NULL;
	IDispatch *pIDispatch=NULL;
	long lCount;
	int i;
	
	// r�cup pAccessible
	pAccessible=GetW7PopupIAccessible(w);
	if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup")); goto end; }

	// compte les childs
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"get_accChildCount()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_INFO,_F_,"get_accChildCount()=%ld",lCount));

	// �num�re les childs et r�cup�re le libell� du dernier child de type texte
	// suite � ISSUE#78 et ISSUE#228 cela semble �tre la meilleure solution
	// Suite � ISSUE#356, il ne faut pas rechercher le dernier champ statique car si caps lock est activ�e, 
	// c'est le warning affich� dans la fen�tre qui remonte et non pas l'URL. 
	// Il faut donc trouver le dernier champ statique AVANT la liste des comptes / champs de login/mdp (=ROLE_SYSTEM_LISTITEM)
	vtSelf.vt=VT_I4;
	vtSelf.lVal=CHILDID_SELF;
	for (i=1;i<=lCount;i++) // inutile de commencer � 0, c'est CHILDID_SELF
	{
		// r�cup du i�me child
		vtChild.vt=VT_I4;
		vtChild.lVal=i;
		
		hr=pAccessible->get_accChild(vtChild,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"pAccessible->get_accChild(%ld)=0x%08lx",vtChild.lVal,hr));
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChild(%ld)=0x%08lx",vtChild.lVal,hr)); goto end; }
		
		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pChild);
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr));
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }

		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }

		// Lecture du r�le du child � la recherche de ROLE_SYSTEM_STATICTEXT
		hr=pChild->get_accRole(vtSelf,&vtRole);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole(%d)=0x%08lx",vtSelf.lVal,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"get_accRole(%d) vtRole.lVal=0x%08lx",vtSelf.lVal,vtRole.lVal));
		
		if (vtRole.lVal==ROLE_SYSTEM_STATICTEXT) // trouv�, on lit le libell�
		{
			SysFreeString(bstrName); bstrName=NULL; // efface le libell� du pr�c�dent child
			// Lecture du texte
			hr=pChild->get_accName(vtSelf,&bstrName);
			if (hr!=S_OK) 
			{
				TRACE((TRACE_ERROR,_F_,"pChild->get_accName(%ld)=0x%08lx",vtSelf.lVal,hr));
				goto end;
			}
		}
		else if (vtRole.lVal==ROLE_SYSTEM_LISTITEM) // trouv� la liste des comptes, on sort, l'URL �tait avant (ISSUE#356)
		{
			TRACE((TRACE_INFO,_F_,"vtRole.lVal=ROLE_SYSTEM_LISTITEM, on arr�te la recherche de l'URL"));
			goto suite; // oui, j'aime bien le goto suite, c'est pratique (OK, je sais, c'est mal, mais j'ai pas le temps)
		}
		if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
	}
suite:
	if (bstrName!=NULL)
	{
		pszURL=GetSZFromBSTR(bstrName);
		if (pszURL==NULL) goto end;
		TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));
	}
end:
	SysFreeString(bstrName);
	if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
	if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

// ----------------------------------------------------------------------------------
// GetW10PopupURL()
// ----------------------------------------------------------------------------------
// lecture de l'URL dans les popups IE dans W10 (anniversaire et >)
// Pour l'instant ne fonctionne pas, la navigation dans les popup semble cass�e dans 
// cette version de Windows (06/08/2016). Je laisse le code pour �volution future
// ----------------------------------------------------------------------------------
// [rc] pointeur vers la chaine, � lib�rer par l'appelant. NULL si erreur
// ----------------------------------------------------------------------------------
char *GetW10PopupURL(HWND w) 
{
	TRACE((TRACE_ENTER,_F_, ""));

	HRESULT hr;
	IAccessible *pAccessible=NULL;
	VARIANT vtSelf,vtChild;
	VARIANT vtRole;
	BSTR bstrName=NULL;
	char *pszURL=NULL;
	IAccessible *pChild=NULL;
	IAccessible *pChildLevelOne=NULL;
	IDispatch *pIDispatch=NULL;
	long lCount;
	int i;
	long returnCount;
	VARIANT* pArray = NULL;
	BOOL bChampsLoginMotDePasseTrouves=FALSE;

	vtSelf.vt=VT_I4;
	vtSelf.lVal=CHILDID_SELF;

	// r�cup pAccessible
	pAccessible=GetW10PopupIAccessible(w);
	if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup")); goto end; }

	// r�cup childs de 1er niveau, c'est le 1er qui nous int�resse, mais on est oblig� de faire comme �a
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pAccessible->get_accChildCount()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_INFO,_F_,"get_accChildCount()=%ld",lCount));
	pArray = new VARIANT[lCount];
	hr = AccessibleChildren(pAccessible, 0L, lCount, pArray, &returnCount);
	if (FAILED(hr)) { TRACE((TRACE_DEBUG,_F_,"AccessibleChildren()=0x%08lx",hr)); goto end; }
	
	// on prend le 2�me child
	VARIANT *pVarChildLevelOne = &pArray[2];
	pChildLevelOne=NULL;
	if (pVarChildLevelOne->vt!=VT_DISPATCH) { TRACE((TRACE_DEBUG,_F_,"pVarChildLevelOne->vt=%d (!=VT_DISPATCH)",pVarChildLevelOne->vt)); goto end; }
	if (pVarChildLevelOne->lVal==NULL) {TRACE((TRACE_DEBUG,_F_,"pVarChildLevelOne->lVal=NULL")); goto end; }
	((IDispatch*)(pVarChildLevelOne->lVal))->QueryInterface(IID_IAccessible, (void**) &pChildLevelOne);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx -> pChild=0x%08lx",hr,pChildLevelOne));

	// compte les childs
	hr=pChildLevelOne->get_accChildCount(&lCount);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pChildLevelOne->get_accChildCount()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_INFO,_F_,"get_accChildCount()=%ld",lCount));
	pArray = new VARIANT[lCount];
	hr = AccessibleChildren(pChildLevelOne, 0L, lCount, pArray, &returnCount);
	if (FAILED(hr)) { TRACE((TRACE_DEBUG,_F_,"AccessibleChildren()=0x%08lx",hr)); goto end; }
	
	TRACE((TRACE_DEBUG,_F_,"AccessibleChildren() returnCount=%d",returnCount));
	for (i=0;i<lCount;i++)
	{
		VARIANT *pVarCurrent = &pArray[i];
		VariantInit(&vtRole);
		pChild=NULL;

		if (pVarCurrent->vt!=VT_DISPATCH) goto suivant;
		if (pVarCurrent->lVal==NULL) goto suivant;  
		((IDispatch*)(pVarCurrent->lVal))->QueryInterface(IID_IAccessible, (void**) &pChild);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto suivant; }
		TRACE((TRACE_DEBUG,_F_,"QueryInterface(IID_IAccessible)=0x%08lx -> pChild=0x%08lx",hr,pChild));
			
		vtChild.vt=VT_I4;
		vtChild.lVal=CHILDID_SELF;
			
		// Lecture du r�le du child � la recherche de ROLE_SYSTEM_STATICTEXT
		// il peut y en avoir plusieurs : dans ce cas on garde le dernier trouv�
		// avant le champ mot de passe, ce qu'il y a en dessous ne nous int�resse pas.
		// (cas de la fen�tre mstsc qui a un champ domaine sous le login/pwd)
		hr=pChild->get_accRole(vtChild,&vtRole);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); goto suivant; }
		TRACE((TRACE_DEBUG,_F_,"get_accRole() vtRole.lVal=0x%08lx",vtRole.lVal));
		if (vtRole.lVal==ROLE_SYSTEM_STATICTEXT && !bChampsLoginMotDePasseTrouves) // on lit le libell�
		{
			SysFreeString(bstrName); bstrName=NULL; // efface le libell� du pr�c�dent child
			// Lecture du texte
			hr=pChild->get_accName(vtSelf,&bstrName);
			if (hr!=S_OK) 
			{
				TRACE((TRACE_ERROR,_F_,"pChild->get_accName(%ld)=0x%08lx",vtSelf.lVal,hr));
				goto end;
			}
		}
		else if (vtRole.lVal==ROLE_SYSTEM_TEXT)
		{
			bChampsLoginMotDePasseTrouves=TRUE;
		}
suivant:
		if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
	}

	if (bstrName!=NULL)
	{
		pszURL=GetSZFromBSTR(bstrName);
		if (pszURL==NULL) goto end;
		TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));
	}
end:
	SysFreeString(bstrName);
	if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
	if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
	if (pChildLevelOne!=NULL) { pChildLevelOne->Release(); pChildLevelOne=NULL; }
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}

