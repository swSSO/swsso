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
// swSSODomains.cpp
//-----------------------------------------------------------------------------
// Gestion des domaines de configuration
//-----------------------------------------------------------------------------

#include "stdafx.h"

static int giRefreshTimer=10;

//-----------------------------------------------------------------------------
// SelectDomainDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de choix de domaine
//-----------------------------------------------------------------------------
static int CALLBACK SelectDomainDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// titre en gras
			SetTextBold(w,TX_FRAME);
			SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
			MACRO_SET_SEPARATOR;
			// remplissage combo
			T_DOMAIN *ptDomains=(T_DOMAIN*)lp;
			int i=1;
			while (ptDomains[i].iDomainId!=-1) 
			{ 
				int index=SendMessage(GetDlgItem(w,CB_DOMAINS),CB_ADDSTRING,0,(LPARAM)ptDomains[i].szDomainLabel); 
				SendMessage(GetDlgItem(w,CB_DOMAINS),CB_SETITEMDATA,index,(LPARAM)ptDomains[i].iDomainId); 
				i++; 
			}
			SendMessage(GetDlgItem(w,CB_DOMAINS),CB_SETCURSEL,0,0);
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		}
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
					int index=SendMessage(GetDlgItem(w,CB_DOMAINS),CB_GETCURSEL,0,0);
					giDomainId=SendMessage(GetDlgItem(w,CB_DOMAINS),CB_GETITEMDATA,index,0);
					SendMessage(GetDlgItem(w,CB_DOMAINS),CB_GETLBTEXT,index,(LPARAM)gszDomainLabel);
					EndDialog(w,IDOK);
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
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
// SelectDomain()
//-----------------------------------------------------------------------------
// Récupère la liste des domaines disponibles sur le serveur et s'il y en 
// a plus d'un propose le choix à l'utilisateur
// rc :  0 - OK, l'utilisateur a choisi, le domaine est renseigné dans giDomainId et gszDomainLabel
//    :  1 - Il n'y avait qu'un seul domaine, l'utilisateur n'a rien vu mais le domaine est bien renseigné
//    :  2 - L'utilisateur a annulé
//-----------------------------------------------------------------------------
int SelectDomain(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int ret;

	if (giNbDomains==0) // aucun domaine
	{
		giDomainId=1; *gszDomainLabel=0;
		rc=1; goto end;
	}
	else if (giNbDomains==1) // domaine commun -> renseigne le domaine commun
	{
		giDomainId=gtabDomains[0].iDomainId;
		strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),gtabDomains[0].szDomainLabel); // corrigé en 1.03 sizeof(gtabDomains) remplacé par sizeof(gszDomainLabel)
		rc=1; goto end;
	}
	else if (giNbDomains==2) // domaine commun + 1 domaine spécifique -> renseigne le domaine spécifique
	{
		giDomainId=gtabDomains[1].iDomainId;
		strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),gtabDomains[1].szDomainLabel);
		rc=1; goto end;
	}
	else // plus de 2 domaines, demande à l'utilisateur de choisir
	{
		ret=DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_SELECT_DOMAIN),NULL,SelectDomainDialogProc,(LPARAM)gtabDomains);
		if (ret==IDCANCEL) { rc=2; goto end; }
	}
	rc=0;
end:
	SaveConfigHeader();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetDomainLabel()
//-----------------------------------------------------------------------------
// Renseigne le libellé d'un domaine en fonction de son id (id si non trouvé)
//-----------------------------------------------------------------------------
void GetDomainLabel(int iDomainId)
{
	TRACE((TRACE_ENTER,_F_, "iDomainId=%d",iDomainId));
	int i;

	if (iDomainId==-1)
	{
		strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),"Tous");
	}
	else if (iDomainId==1)
	{
		strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),"Commun");
	}
	else
	{
		for (i=0;i<giNbDomains;i++)
		{
			if (gtabDomains[i].iDomainId==iDomainId) 
			{
				strcpy_s(gszDomainLabel,sizeof(gszDomainLabel),gtabDomains[i].szDomainLabel);
				goto end;
			}
		}
		// non trouvé, on met l'id en libellé
		sprintf_s(gszDomainLabel,sizeof(gszDomainLabel),"Domaine #%d",iDomainId);
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// GetAllDomains()
//-----------------------------------------------------------------------------
// Récupère la liste des domaines disponibles sur le serveur
//-----------------------------------------------------------------------------
// Retour : nb de domaines lus, 0 si aucun (y/c si erreur de lecture)
//-----------------------------------------------------------------------------
int GetAllDomains(T_DOMAIN *pgtabDomain)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;
	char szParams[512+1];
	char *pszResult=NULL;
	BSTR bstrXML=NULL;
	HRESULT hr;
	IXMLDOMDocument *pDoc=NULL;
	IXMLDOMNode		*pRoot=NULL;
	IXMLDOMNode		*pNode=NULL;
	IXMLDOMNode		*pChildApp=NULL;
	IXMLDOMNode		*pChildElement=NULL;
	IXMLDOMNode		*pNextChildApp=NULL;
	IXMLDOMNode		*pNextChildElement=NULL;
	VARIANT_BOOL	vbXMLLoaded=VARIANT_FALSE;
	DWORD dwStatusCode;
	BSTR bstrNodeName=NULL;
	char tmp[10];

	// requete le serveur pour obtenir la liste des domaines
	strcpy_s(szParams,sizeof(szParams),"?action=getdomains");
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	bstrXML=GetBSTRFromSZ(pszResult);
	if (bstrXML==NULL) goto end;

	// analyse le contenu XML retourné
	hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument,(void**)&pDoc);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IXMLDOMDocument)=0x%08lx",hr)); goto end; }
	hr = pDoc->loadXML(bstrXML,&vbXMLLoaded);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML()=0x%08lx",hr)); goto end; }
	if (vbXMLLoaded==VARIANT_FALSE) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML() returned FALSE")); goto end; }
	hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void **)&pRoot);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pXMLDoc->QueryInterface(IID_IXMLDOMNode)=0x%08lx",hr)); goto end;	}
	hr=pRoot->get_firstChild(&pNode);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pRoot->get_firstChild(&pNode)")); goto end; }
	hr=pNode->get_firstChild(&pChildApp);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildApp)")); goto end; }
	while (pChildApp!=NULL) 
	{
		TRACE((TRACE_DEBUG,_F_,"<domain>"));
		hr=pChildApp->get_firstChild(&pChildElement);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildElement)")); goto end; }
		while (pChildElement!=NULL) 
		{
			hr=pChildElement->get_nodeName(&bstrNodeName);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeName()")); goto end; }
			TRACE((TRACE_DEBUG,_F_,"<%S>",bstrNodeName));
			
			if (CompareBSTRtoSZ(bstrNodeName,"id")) 
			{
				StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				pgtabDomain[rc].iDomainId=atoi(tmp);
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"label")) 
			{
				StoreNodeValue(pgtabDomain[rc].szDomainLabel,sizeof(pgtabDomain[rc].szDomainLabel),pChildElement);
			}
			// rechercher ses frères et soeurs
			pChildElement->get_nextSibling(&pNextChildElement);
			pChildElement->Release();
			pChildElement=pNextChildElement;
		} // while(pChild!=NULL)
		// rechercher ses frères et soeurs
		pChildApp->get_nextSibling(&pNextChildApp);
		pChildApp->Release();
		pChildApp=pNextChildApp;
		TRACE((TRACE_DEBUG,_F_,"</domain>"));
		rc++;
	} // while(pNode!=NULL)
	pgtabDomain[rc].iDomainId=-1;

#ifdef TRACES_ACTIVEES
	int trace_i;
	for (trace_i=0;trace_i<rc-1;trace_i++)
	{
		TRACE((TRACE_INFO,_F_,"Domaine %d : id=%d label=%s",trace_i,pgtabDomain[trace_i].iDomainId,pgtabDomain[trace_i].szDomainLabel));
	}
#endif
end:
	if (pszResult!=NULL) free(pszResult);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	if (bstrNodeName!=NULL) SysFreeString(bstrNodeName);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetConfigDomains()
//-----------------------------------------------------------------------------
// Retourne les domaines auxquels la config iConfigId est attachée
//-----------------------------------------------------------------------------
// Retour : nb de domaines lus, 0 si aucun (y/c si erreur de lecture)
//-----------------------------------------------------------------------------
int GetConfigDomains(int iConfigId,T_CONFIGS_DOMAIN *pgtabDomain)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;
	char szParams[512+1];
	char *pszResult=NULL;
	BSTR bstrXML=NULL;
	HRESULT hr;
	IXMLDOMDocument *pDoc=NULL;
	IXMLDOMNode		*pRoot=NULL;
	IXMLDOMNode		*pNode=NULL;
	IXMLDOMNode		*pChildApp=NULL;
	IXMLDOMNode		*pChildElement=NULL;
	IXMLDOMNode		*pNextChildApp=NULL;
	IXMLDOMNode		*pNextChildElement=NULL;
	VARIANT_BOOL	vbXMLLoaded=VARIANT_FALSE;
	DWORD dwStatusCode;
	BSTR bstrNodeName=NULL;
	char tmp[10];

	// requete le serveur pour obtenir la liste des domaines
	sprintf_s(szParams,sizeof(szParams),"?action=getconfigdomains&configId=%d",iConfigId);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	bstrXML=GetBSTRFromSZ(pszResult);
	if (bstrXML==NULL) goto end;

	// analyse le contenu XML retourné
	hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument,(void**)&pDoc);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IXMLDOMDocument)=0x%08lx",hr)); goto end; }
	hr = pDoc->loadXML(bstrXML,&vbXMLLoaded);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML()=0x%08lx",hr)); goto end; }
	if (vbXMLLoaded==VARIANT_FALSE) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML() returned FALSE")); goto end; }
	hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void **)&pRoot);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pXMLDoc->QueryInterface(IID_IXMLDOMNode)=0x%08lx",hr)); goto end;	}
	hr=pRoot->get_firstChild(&pNode);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pRoot->get_firstChild(&pNode)")); goto end; }
	hr=pNode->get_firstChild(&pChildApp);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildApp)")); goto end; }
	while (pChildApp!=NULL) 
	{
		TRACE((TRACE_DEBUG,_F_,"<domain>"));
		hr=pChildApp->get_firstChild(&pChildElement);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildElement)")); goto end; }
		while (pChildElement!=NULL) 
		{
			hr=pChildElement->get_nodeName(&bstrNodeName);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeName()")); goto end; }
			TRACE((TRACE_DEBUG,_F_,"<%S>",bstrNodeName));
			
			if (CompareBSTRtoSZ(bstrNodeName,"id")) 
			{
				StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				pgtabDomain[rc].iDomainId=atoi(tmp);
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"label")) 
			{
				StoreNodeValue(pgtabDomain[rc].szDomainLabel,sizeof(pgtabDomain[rc].szDomainLabel),pChildElement);
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"domainAutoPublish")) // si absent, valorisé à FALSE puisque tableau des domaines initialisé à 0 avant l'appel
			{
				StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				pgtabDomain[rc].bAutoPublish=atoi(tmp);
			}
			// rechercher ses frères et soeurs
			pChildElement->get_nextSibling(&pNextChildElement);
			pChildElement->Release();
			pChildElement=pNextChildElement;
		} // while(pChild!=NULL)
		// rechercher ses frères et soeurs
		pChildApp->get_nextSibling(&pNextChildApp);
		pChildApp->Release();
		pChildApp=pNextChildApp;
		TRACE((TRACE_DEBUG,_F_,"</domain>"));
		rc++;
	} // while(pNode!=NULL)
	pgtabDomain[rc].iDomainId=-1;

#ifdef TRACES_ACTIVEES
	int trace_i;
	for (trace_i=0;trace_i<rc-1;trace_i++)
	{
		TRACE((TRACE_INFO,_F_,"Domaine %d : id=%d label=%s",trace_i,pgtabDomain[trace_i].iDomainId,pgtabDomain[trace_i].szDomainLabel));
	}
#endif
end:
	if (pszResult!=NULL) free(pszResult);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	if (bstrNodeName!=NULL) SysFreeString(bstrNodeName);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetDomainConfigsAutoPublish()
//-----------------------------------------------------------------------------
// Retourne les configurations rattachées au domaine, avec le statut
// d'autopublish pour ce domaine
//-----------------------------------------------------------------------------
// Retour : nb de configs lues, 0 si aucun (y/c si erreur de lecture)
//-----------------------------------------------------------------------------
int GetDomainConfigsAutoPublish(int iDomainId,T_DOMAIN_CONFIGS *pgtabConfig)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=0;
	char szParams[512+1];
	char *pszResult=NULL;
	BSTR bstrXML=NULL;
	HRESULT hr;
	IXMLDOMDocument *pDoc=NULL;
	IXMLDOMNode		*pRoot=NULL;
	IXMLDOMNode		*pNode=NULL;
	IXMLDOMNode		*pChildApp=NULL;
	IXMLDOMNode		*pChildElement=NULL;
	IXMLDOMNode		*pNextChildApp=NULL;
	IXMLDOMNode		*pNextChildElement=NULL;
	VARIANT_BOOL	vbXMLLoaded=VARIANT_FALSE;
	DWORD dwStatusCode;
	BSTR bstrNodeName=NULL;
	char tmp[10];

	// requete le serveur pour obtenir la liste des configs du domaine et leur statut d'autopublish
	sprintf_s(szParams,sizeof(szParams),"?action=getdomainconfigsautopublish&domainId=%d",iDomainId);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	bstrXML=GetBSTRFromSZ(pszResult);
	if (bstrXML==NULL) goto end;

	// analyse le contenu XML retourné
	hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument,(void**)&pDoc);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IXMLDOMDocument)=0x%08lx",hr)); goto end; }
	hr = pDoc->loadXML(bstrXML,&vbXMLLoaded);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML()=0x%08lx",hr)); goto end; }
	if (vbXMLLoaded==VARIANT_FALSE) { TRACE((TRACE_ERROR,_F_,"pXMLDoc->loadXML() returned FALSE")); goto end; }
	hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void **)&pRoot);
	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pXMLDoc->QueryInterface(IID_IXMLDOMNode)=0x%08lx",hr)); goto end;	}
	hr=pRoot->get_firstChild(&pNode);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pRoot->get_firstChild(&pNode)")); goto end; }
	hr=pNode->get_firstChild(&pChildApp);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildApp)")); goto end; }
	while (pChildApp!=NULL) 
	{
		TRACE((TRACE_DEBUG,_F_,"<config>"));
		hr=pChildApp->get_firstChild(&pChildElement);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pNode->get_firstChild(&pChildElement)")); goto end; }
		while (pChildElement!=NULL) 
		{
			hr=pChildElement->get_nodeName(&bstrNodeName);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pChild->get_nodeName()")); goto end; }
			TRACE((TRACE_DEBUG,_F_,"<%S>",bstrNodeName));
			
			if (CompareBSTRtoSZ(bstrNodeName,"id")) 
			{
				StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				pgtabConfig[rc].iConfigId=atoi(tmp);
			}
			else if (CompareBSTRtoSZ(bstrNodeName,"domainAutoPublish")) // si absent, valorisé à FALSE puisque tableau des domaines initialisé à 0 avant l'appel
			{
				StoreNodeValue(tmp,sizeof(tmp),pChildElement);
				pgtabConfig[rc].bAutoPublish=atoi(tmp);
			}
			// rechercher ses frères et soeurs
			pChildElement->get_nextSibling(&pNextChildElement);
			pChildElement->Release();
			pChildElement=pNextChildElement;
		} // while(pChild!=NULL)
		// rechercher ses frères et soeurs
		pChildApp->get_nextSibling(&pNextChildApp);
		pChildApp->Release();
		pChildApp=pNextChildApp;
		TRACE((TRACE_DEBUG,_F_,"</config>"));
		rc++;
	} // while(pNode!=NULL)
	pgtabConfig[rc].iConfigId=-1;

#ifdef TRACES_ACTIVEES
	int trace_i;
	for (trace_i=0;trace_i<rc-1;trace_i++)
	{
		TRACE((TRACE_INFO,_F_,"Config[%d] id=%d domainAutoPublish=%d",trace_i,pgtabConfig[trace_i].iConfigId,pgtabConfig[trace_i].bAutoPublish));
	}
#endif
end:
	if (pszResult!=NULL) free(pszResult);
	if (bstrXML!=NULL) SysFreeString(bstrXML);
	if (bstrNodeName!=NULL) SysFreeString(bstrNodeName);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ReadDomainLabel()
//-----------------------------------------------------------------------------
// Lit le libellé du domaine de l'utilisateur dans la clé de registre indiquée
// dans la configuration (EnterpriseOptions/DomainRegRootKey et DomainRegKey)
//-----------------------------------------------------------------------------
int ReadDomainLabel(char *pszDomainLabel)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HKEY hKey=NULL;
	HKEY hRootKey;
	DWORD dwValueType,dwValueSize;

	if (strlen(gszDomainRegKey)<10) { TRACE((TRACE_ERROR,_F_,"Bad reg key: %s",gszDomainRegKey)); goto end; }

	if (_strnicmp(gszDomainRegKey,"HKLM",4)==0)
		hRootKey=HKEY_LOCAL_MACHINE;
	else if (_strnicmp(gszDomainRegKey,"HKCU",4)==0)
		hRootKey=HKEY_CURRENT_USER;
	else { TRACE((TRACE_ERROR,_F_,"Unknown root key: %s",gszDomainRegKey)); goto end; }

	rc=RegOpenKeyEx(hRootKey,gszDomainRegKey+5,0,KEY_READ,&hKey);
	if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"Key not found: %s",gszDomainRegKey)); goto end; }

	dwValueType=REG_SZ;
	dwValueSize=LEN_DOMAIN+1;
	rc=RegQueryValueEx(hKey,gszDomainRegValue,NULL,&dwValueType,(LPBYTE)pszDomainLabel,&dwValueSize);
	if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"Value not found: %s",gszDomainRegValue)); goto end; }
	
	TRACE((TRACE_INFO,_F_,"pszDomainLabel=%s",pszDomainLabel));

	rc=0;
end:
	if (hKey!=NULL) RegCloseKey(hKey);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetDomainIdFromLabel()
//-----------------------------------------------------------------------------
// Interroge le serveur pour récupérer l'ID d'un domain à partir de son libellé
//-----------------------------------------------------------------------------
int GetDomainIdFromLabel(const char *cszDomainLabel,int *piDomainId)
{
	TRACE((TRACE_ENTER,_F_, "cszDomainLabel=%s",cszDomainLabel));
	int rc=-1;
	char szParams[512+1];
	char *pszResult=NULL;
	DWORD dwStatusCode;
	
	sprintf_s(szParams,sizeof(szParams),"?action=getdomainid&domainLabel=%s",cszDomainLabel);
	TRACE((TRACE_INFO,_F_,"Requete HTTP : %s",szParams));
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }
	
	*piDomainId=atoi(pszResult);
	if (*piDomainId==0) { TRACE((TRACE_ERROR,_F_,"domainId not found")); goto end; }

	TRACE((TRACE_INFO,_F_,"*piDomainId=%d",*piDomainId));

	rc=0;
end:
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}