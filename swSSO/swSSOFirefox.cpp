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

// totalement bidon pour paramètre inutilisé de QueryService
const GUID refguid = {0x0c539790, 0x12e4, 0x11cf, 0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8};


typedef struct 
{
	HWND w;
	int iAction; // id action
	int iSuivi;  // suivi action 
	int iErreur; // le positionner à -1 pour sortir de la récursivité de DoAccessible
	int iLevel;  // niveau de récursivité
	bool bId2Done; // true dès que le champ id2 a été rempli 
	bool bId3Done; // true dès que le champ id3 a été rempli 
	bool bId4Done; // true dès que le champ id3 a été rempli 
	IAccessible *pPwdAccessible; // pour mémoriser le champ pwd pour la frappe ENTREE
	int iBrowser; // BROWSER_FIREFOX3 | BROWSER_FIREFOX4
} T_SUIVI_FIREFOX;

const char gcszMozillaClassName[]="MozillaWindowClass";
const char gcszMozillaUIClassName[]="MozillaUIWindowClass";
const char gcszMozillaContentClassName[]="MozillaContentWindowClass";
const char gcszMozillaDialogClassName[]="MozillaDialogClass";

int giaccChildCountErrors;
int giaccChildErrors;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// GetNodeTagName()
// ----------------------------------------------------------------------------------
// Récupère dans le DOM le nom du tag HTML correspondant au pAccessible passé 
// en paramètre
// ----------------------------------------------------------------------------------
// [in]  pAccessible
// [out] szNodeTagName (alloué par l'appelant taille 100+1)
// ----------------------------------------------------------------------------------
#ifdef TRACES_ACTIVEES
int GetNodeTagName(char *szTab,IAccessible *pAccessible,char *szNodeTagName)
#else
int GetNodeTagName(IAccessible *pAccessible,char *szNodeTagName)
#endif
{
	TRACE((TRACE_ENTER,_F_, ""));
	
	HRESULT hr;
	int rc=-1;
	ISimpleDOMNode *pNode=NULL;
	IServiceProvider *pServiceProvider=NULL;

	// pour get_nodeInfo
	BSTR bstrNodeName=NULL;
	BSTR bstrNodeValue=NULL;
	int lenNodeName;
	unsigned short usNodeType;
	short sNameSpaceID;
	unsigned int uiNumChildren;
	unsigned int uiUniqueID;
	char szNodeName[11+1];
	BOOL bNodeTypeOK;
	
	// pour get_attributes
	const short MAX_ATTRIBS = 20;
	unsigned short usNbAttribs; 
	short sNameSpaceIDs[MAX_ATTRIBS];
	BSTR  bstrAttribNames[MAX_ATTRIBS];
	BSTR  bstrAttribValues[MAX_ATTRIBS];
	unsigned short i;
	char szAttribName[4+1];

	// pServiceProvider => permet d'obtenir ensuite un pNode
	hr=pAccessible->QueryInterface(IID_IServiceProvider,(void**)&pServiceProvider);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"%sQueryInterface(IID_IServiceProvider)=0x%08lx",szTab,hr));
		goto end;
	}

	// ISimpleDOMNode
	hr=pServiceProvider->QueryService(refguid,IID_ISimpleDOMNode,(void**)&pNode);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"%sQueryService(IID_ISimpleDOMNode)=0x%08lx",szTab,hr));
		goto end;
	}
	//
	hr=pNode->get_nodeInfo(&bstrNodeName,&sNameSpaceID,&bstrNodeValue,&uiNumChildren,&uiUniqueID,&usNodeType);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"%sget_nodeInfo()=0x%08lx",szTab,hr));
		goto end;
	}
	TRACE((TRACE_DEBUG,_F_,"%sbstrNodeName=%S",szTab,bstrNodeName));
	TRACE((TRACE_DEBUG,_F_,"%susNodeType=%d",szTab,usNodeType));
	
	// seuls les ELEMENT (type=1) de nom INPUT ou html:input nous intéressent (0.66 + select pour le 2nd identifiant)
	bNodeTypeOK=FALSE;
	if (usNodeType!=1) goto end;
	lenNodeName=SysStringLen(bstrNodeName);
	if (lenNodeName==5) 
	{
		sprintf_s(szNodeName,sizeof(szNodeName),"%S",bstrNodeName);
		if (_stricmp(szNodeName,"INPUT")==0) bNodeTypeOK=TRUE; 
	} 
	else if (lenNodeName==10) 
	{
		sprintf_s(szNodeName,sizeof(szNodeName),"%S",bstrNodeName);
		if (_stricmp(szNodeName,"html:input")==0) bNodeTypeOK=TRUE; 
	}
	else if (lenNodeName==6) 
	{
		sprintf_s(szNodeName,sizeof(szNodeName),"%S",bstrNodeName);
		if (_stricmp(szNodeName,"SELECT")==0) bNodeTypeOK=TRUE; 
	}
	else if (lenNodeName==11) 
	{
		sprintf_s(szNodeName,sizeof(szNodeName),"%S",bstrNodeName);
		if (_stricmp(szNodeName,"html:select")==0) bNodeTypeOK=TRUE; 
	}
	if (!bNodeTypeOK) goto end;

	// lecture des attributs à la recherche de l'attribut name qui contient enfin le nom du tag
	hr=pNode->get_attributes(MAX_ATTRIBS,bstrAttribNames,sNameSpaceIDs,bstrAttribValues,&usNbAttribs);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR,_F_,"%spNode->get_attributes()=0x%08lx",szTab,hr));
		goto end;
	}
	for (i=0; i<usNbAttribs;i++) 
	{
		TRACE((TRACE_DEBUG,_F_,"%sattrib[%d].name =%S",szTab,i,bstrAttribNames[i]));
		TRACE((TRACE_DEBUG,_F_,"%sattrib[%d].value=%S",szTab,i,bstrAttribValues[i]));
		if (bstrAttribNames[i]!=NULL && bstrAttribValues[i]!=NULL)
		{
			if (SysStringLen(bstrAttribNames[i])==4 && SysStringLen(bstrAttribValues[i])<100)
			{
				sprintf_s(szAttribName,sizeof(szAttribName),"%S",bstrAttribNames[i]);
				if (strcmp(szAttribName,"name")==0)
				{
					// 0.71c : quelle horreur, ce sizeof retourne évidemment la taille du pointeur
					// et pas la taille du buffer... symptome = trap débordement de buffer
					// sur la page d'accueil du site caisse d'épargne
					//sprintf_s(szNodeTagName,sizeof(szNodeTagName),"%S",bstrAttribValues[i]);
					sprintf_s(szNodeTagName,100+1,"%S",bstrAttribValues[i]); // pas très beau, mais bon...
					rc=0;
					// attention, il faut continuer la boucle jusqu'au bout pour
					// libérer tous les BSTR !
				}
			}
		}
		SysFreeString(bstrAttribNames[i]);
		SysFreeString(bstrAttribValues[i]);
	}
// ERREUR NON?	rc=0;

end:
	SysFreeString(bstrNodeName);
	SysFreeString(bstrNodeValue);
	if (pNode!=NULL) pNode->Release();
	if (pServiceProvider!=NULL) pServiceProvider->Release();
	
	TRACE((TRACE_LEAVE,_F_, "%src=%d",szTab,rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// DoDOMAccessible() [attention, fonction récursive]
// ----------------------------------------------------------------------------------
// Parcours récursif de la fenêtre et de ses objets "accessibles". Pour chacun,
// lecture du tagname HTML pour comparer avec les valeurs attendues à renseigner.
// ----------------------------------------------------------------------------------
// [in] w : non utilisé 
// [in] pAccessible 
// [in] ptSuivi 
// ----------------------------------------------------------------------------------
#ifdef TRACES_ACTIVEES
static void DoDOMAccessible(char *paramszTab,HWND w,IAccessible *pAccessible,T_SUIVI_FIREFOX *ptSuivi)
#else
static void DoDOMAccessible(HWND w,IAccessible *pAccessible,T_SUIVI_FIREFOX *ptSuivi)
#endif
{
	TRACE((TRACE_ENTER,_F_, ""));

	UNREFERENCED_PARAMETER(w);
	
	HRESULT hr;
	IAccessible *pChild=NULL;
	IDispatch *pIDispatch=NULL;
	BSTR bstrValue=NULL;
	BSTR bstrNodeName=NULL;
	
	VARIANT index; 
	char szNodeTagName[100+1]; // attention à changer le sprintf_s() dans GetNodeTagName
	int rc;
	long l,lCount;

#ifdef TRACES_ACTIVEES
	char szTab[200];
	strcpy_s(szTab,sizeof(szTab),paramszTab);
#endif

	// si fini ou erreur, on termine la récursivité
	if (ptSuivi->iErreur!=0) goto end;
	if (ptSuivi->iSuivi==0) goto end;
	
	// préparation de la lecture de l'élément courant
	index.vt=VT_I4;
	index.lVal=CHILDID_SELF;
	
	// lecture du tagname de l'element
#ifdef TRACES_ACTIVEES
	rc=GetNodeTagName(szTab,pAccessible,szNodeTagName);
#else
	rc=GetNodeTagName(pAccessible,szNodeTagName);
#endif
	if (rc==0) // reussi à lire le tagname, on regarde s'il faut le remplir
	{
		if (strcmp(szNodeTagName,gptActions[ptSuivi->iAction].szId1Name)==0) 
		{
			TRACE((TRACE_INFO,_F_,"tag(Id1) trouve = '%s'",szNodeTagName));
			PutAccValue(ptSuivi->w,pAccessible,index,gptActions[ptSuivi->iAction].szId1Value);
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
		// 0.66 - gestion du 2nd identifiant
		else if (strcmp(szNodeTagName,gptActions[ptSuivi->iAction].szId2Name)==0 && !ptSuivi->bId2Done) 
		{
			TRACE((TRACE_INFO,_F_,"tag(Id2) trouve = '%s'",szNodeTagName));
			ptSuivi->bId2Done=TRUE;
			PutAccValue(ptSuivi->w,pAccessible,index,gptActions[ptSuivi->iAction].szId2Value);
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
		else if (strcmp(szNodeTagName,gptActions[ptSuivi->iAction].szId3Name)==0 && !ptSuivi->bId3Done) 
		{
			TRACE((TRACE_INFO,_F_,"tag(Id3) trouve = '%s'",szNodeTagName));
			ptSuivi->bId3Done=TRUE;
			PutAccValue(ptSuivi->w,pAccessible,index,gptActions[ptSuivi->iAction].szId3Value);
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
		else if (strcmp(szNodeTagName,gptActions[ptSuivi->iAction].szId4Name)==0 && !ptSuivi->bId4Done) 
		{
			TRACE((TRACE_INFO,_F_,"tag(Id4) trouve = '%s'",szNodeTagName));
			ptSuivi->bId4Done=TRUE;
			PutAccValue(ptSuivi->w,pAccessible,index,gptActions[ptSuivi->iAction].szId4Value);
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
		else if (strcmp(szNodeTagName,gptActions[ptSuivi->iAction].szPwdName)==0) 
		{
			TRACE((TRACE_INFO,_F_,"tag(pwd) trouve = '%s'",szNodeTagName));
			SetForegroundWindow(ptSuivi->w);
			hr=pAccessible->accSelect(SELFLAG_TAKEFOCUS,index);
			if ((*gptActions[ptSuivi->iAction].szPwdEncryptedValue!=0)) 
			{
				// char *pszPassword=swCryptDecryptString(gptActions[ptSuivi->iAction].szPwdEncryptedValue,ghKey1);
				char *pszPassword=GetDecryptedPwd(gptActions[ptSuivi->iAction].szPwdEncryptedValue);
				if (pszPassword!=NULL) 
				{
					KBSim(ptSuivi->w,TRUE,200,pszPassword,TRUE);				
					// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
					SecureZeroMemory(pszPassword,strlen(pszPassword));
					free(pszPassword);
				}
			}
			// mémorise les infos permettant de faire la simulation de frappe clavier sur le
			// champ mot de passe quand tout aura été rempli.
			pAccessible->AddRef();
			ptSuivi->pPwdAccessible=pAccessible;
			TRACE((TRACE_INFO,_F_,"ptSuivi->pPwdAccessible=0x%08lx",ptSuivi->pPwdAccessible));
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
	}
	SysFreeString(bstrNodeName); bstrNodeName=NULL;
	// 0.80 : si il ne reste plus qu'une chose à faire et que c'est la validation du formulaire,
	//        met le focus sur le champ mot de passe (s'il existe) et simule frappe touche ENTREE
	TRACE((TRACE_INFO,_F_,"ptSuivi->pPwdAccessible=0x%08lx",ptSuivi->pPwdAccessible));
	if (ptSuivi->iSuivi==1 && *(gptActions[ptSuivi->iAction].szValidateName)!=0)
	{
		SetForegroundWindow(ptSuivi->w);
		if (ptSuivi->pPwdAccessible!=NULL) 
		{
			index.vt=VT_I4;
			index.lVal=CHILDID_SELF;
			hr=ptSuivi->pPwdAccessible->accSelect(SELFLAG_TAKEFOCUS,index);
		}
		TRACE((TRACE_INFO,_F_,"Simulation frappe ENTREE"));
		Sleep(100);
		// 0.89 : si le champ validation commence par [, on interprete le contenu
		if (gptActions[ptSuivi->iAction].szValidateName[0]=='[')
		{
			KBSimEx(NULL,gptActions[ptSuivi->iAction].szValidateName,"","","","","");
		}
		else
		{
			keybd_event(VK_RETURN,0,0,0);
			keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
		}
		ptSuivi->iSuivi=0;
		guiNbWEBSSO++;
	}

	if (ptSuivi->iSuivi==0) goto end;
	
	// parcours de la suite : combien de fils ?
	hr=pAccessible->get_accChildCount(&lCount);
	if (FAILED(hr)) goto end;
	TRACE((TRACE_DEBUG,_F_,"%sget_accChildCount()==%ld",szTab,lCount));

	// Parfois on dirait que get_accChildCount pete les plombs : il retourne 65535
	// Bug aléatoire. Signalé à Aaron le 9 janvier 2005
	if (lCount==65535) { giaccChildCountErrors++;goto end;}
	// plus de fils, on termine
	if (lCount==0) goto end;

#ifdef TRACES_ACTIVEES
	if (strlen(szTab)<sizeof(szTab)-5) strcat_s(szTab,sizeof(szTab),"  ");
#endif
	
	for (l=1;l<=lCount;l++)
	{
		index.vt=VT_I4;
		index.lVal=l;

		TRACE((TRACE_DEBUG,_F_,"%s%ld ---------------------------------",szTab,l));

		hr=pAccessible->get_accChild(index,&pIDispatch);
		TRACE((TRACE_DEBUG,_F_,"%sget_accChild()=0x%08lx",szTab,hr));
		if (hr==0x80010105) giaccChildErrors++;
		// TODO : BUG signalé à Aaron le 9 janvier 2005. Sur certains éléments
		// get_accChild retourne une erreur injustifiée. 
		// Cas de test envoyé à Aaron le 22 janvier 2005 (accchildcount.zip)
		// BUG désormais enregistré chez Mozilla sous la référence [Bug 278872]
		if (FAILED(hr)) goto suite;
		
		hr =pIDispatch->QueryInterface(IID_IAccessible, (void**) &pChild);
		TRACE((TRACE_DEBUG,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx",szTab,hr));
		if (FAILED(hr)) goto suite;

#ifdef TRACES_ACTIVEES
		DoDOMAccessible(szTab,NULL,pChild,ptSuivi);
#else
		DoDOMAccessible(NULL,pChild,ptSuivi);
#endif
suite:
		// 0.70 libération de pIDispatch et pChild, ça peut pas faire de mal !
		// C'était peut-être à la source des consos mémoire Firefox...
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		if (ptSuivi->iSuivi==0) goto end; // on arrête si on a trouvé tout ce qu'on veut ! (0.80)
	}

end:
	SysFreeString(bstrNodeName); bstrNodeName=NULL;
	SysFreeString(bstrValue); bstrValue=NULL;
	TRACE((TRACE_LEAVE,_F_, ""));
}

#if 0 
// Supprimée en 0.92B8 : utilisation de NAVRELATION_EMBEDS pour rechercher le document
//                       comme préconisé par Mozilla (et seule solution fonctionnant à partir de FF4)
// ----------------------------------------------------------------------------------
// FirefoxEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils à la recherche de MozillaContentWindowClass pour FF3
// et MozillaWindowClass pour FF4
// ----------------------------------------------------------------------------------
static int CALLBACK FirefoxEnumChildProc(HWND w, LPARAM lp)
{
	int rc=TRUE;
	char szClassName[128+1];

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_INFO,_F_,"Classe=%s w=0x%08lx",szClassName,w));
	if (((T_SUIVI_FIREFOX*)lp)->iBrowser==BROWSER_FIREFOX3)
	{
		if (strcmp(szClassName,gcszMozillaContentClassName)==0) 
		{
			((T_SUIVI_FIREFOX*)lp)->w=w;
			TRACE((TRACE_INFO,_F_,"Firefox 3 classe=%s w=0x%08lx",szClassName,w));
			rc=FALSE;
		}
	}
	else if (((T_SUIVI_FIREFOX*)lp)->iBrowser==BROWSER_FIREFOX4)
	{
		if (strcmp(szClassName,gcszMozillaClassName)==0) 
		{
			((T_SUIVI_FIREFOX*)lp)->w=w;
			TRACE((TRACE_INFO,_F_,"Firefox 4 classe=%s w=0x%08lx",szClassName,w));
			rc=FALSE;
		}
	}
	return rc;
}
#endif

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// GetFirefoxURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fenêtre Firefox
// ----------------------------------------------------------------------------------
// [in] w = handle de la fenêtre
// [in] bGetAccessible = indique si on veut le pointeur pAccessible en retour
// [out] ppAccessible = pointeur pAccessible si demandé.
// [rc] pszURL (à libérer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------

char *GetFirefoxURL(HWND w,BOOL bGetAccessible,IAccessible **ppAccessible,int iBrowser,BOOL bWaitReady)
{
	TRACE((TRACE_ENTER,_F_, ""));

	T_SUIVI_FIREFOX tSuivi;
	HRESULT hr;
	IDispatch *pIDispatch=NULL;
	IAccessible *pAccessible=NULL;
	ISimpleDOMDocument *pSimpleDOMDocument = NULL;
	IServiceProvider *pServiceProvider=NULL;
	BSTR bstrURL=NULL;
	char *pszURL=NULL;
	IAccessible *pContent=NULL;

	UNREFERENCED_PARAMETER(iBrowser);
	UNREFERENCED_PARAMETER(tSuivi);
	
	hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
		
	VARIANT vtStart,vtResult;
	vtStart.vt=VT_I4;
	vtStart.lVal=CHILDID_SELF;
	hr=pAccessible->accNavigate(0x1009,vtStart,&vtResult); // NAVRELATION_EMBEDS = 0x1009
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS)=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS) vtEnd=0x%08lx",vtResult.lVal));

	if (vtResult.vt!=VT_DISPATCH) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) is not VT_DISPATCH")); goto end; }
	pIDispatch=(IDispatch*)vtResult.lVal;
	if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) pIDispatch=NULL")); goto end; }

	// ISSUE#60 : pServiceProvider->QueryService(refguid,IID_ISimpleDOMDocument) provoque un plantage avec FF4 sous WIN7
	//            J'utilise donc un nouveau moyen pour récupérer l'URL du document, décrit ici : http://www.mozilla.org/access/windows/at-apis
	// hr=pIDispatch->QueryInterface(IID_IServiceProvider, (void**)&pServiceProvider);
	// if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IServiceProvider)=0x%08lx",hr));	goto end; }	
	// TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface() -> OK"));
	// hr=pServiceProvider->QueryService(refguid,IID_ISimpleDOMDocument, (void**) &pSimpleDOMDocument);
	// if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryService(IID_ISimpleDOMDocument)=0x%08lx",hr));	goto end; }	
	// TRACE((TRACE_DEBUG,_F_,"pServiceProvider->QueryService(IID_ISimpleDOMDocument) -> OK"));
	// Lit l'URL
	// hr = pSimpleDOMDocument->get_URL(&bstrURL); 
	// if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pSimpleDOMDocument->get_URL()=0x%08lx",hr));	goto end; }

	hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pContent);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	
	hr=pContent->get_accRole(vtStart,&vtResult);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accRole()=0x%08lx",hr)); goto end; }
	if (vtResult.lVal!=ROLE_SYSTEM_DOCUMENT) { TRACE((TRACE_ERROR,_F_,"get_accRole() : vtResult.lVal=%ld",vtResult.lVal)); goto end; }

	// ISSUE#72 (0.95) : maintenant que Firefox indique bien que le chargement de la page n'est pas terminé,
	//                   on attend patiemment avant de lancer le SSO.
	// 0.97 : on ne le fait que si bWaitReady (et notamment on ne le fait pas dans le cas des popups cf. ISSUE#87)
	if (bWaitReady)
	{
		hr=pContent->get_accState(vtStart,&vtResult);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accState()=0x%08lx",hr)); goto end; }
		if (vtResult.lVal & STATE_SYSTEM_BUSY) { TRACE((TRACE_ERROR,_F_,"get_accState() : STATE_SYSTEM_BUSY, on verra plus tard !")); goto end; }
	}
	hr=pContent->get_accValue(vtStart,&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accValue()=0x%08lx",hr)); goto end; }
	
	// Alloue pszURL et le retourne
	// ISSUE#298 : ne plus utiliser le %S, j'ai fait une fonction pour ça !!
	/* TRACE((TRACE_INFO,_F_,"URL=%S",bstrURL));
	pszURL=(char*)malloc(SysStringLen(bstrURL)+1);
	if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",SysStringLen(bstrURL)+1)); goto end; }
	wsprintf(pszURL,"%S",bstrURL);*/
	
	pszURL=GetSZFromBSTR(bstrURL);
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",pszURL));
	
	// Si demandé, fourni le ppAccessible du document afin de réduire le parsing de la page Web (0.80)
	if (bGetAccessible)
	{
		*ppAccessible=pContent;
		pContent->AddRef(); // astuce : nécessaire pour que le ppAccessible retourné reste valide malgré le pContent->Release()
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

typedef struct 
{
	HWND w;
	char szClassName[128+1];
} T_SUIVI_GETACCESSIBLE;

// ----------------------------------------------------------------------------------
// WebEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils à la recherche de la fenêtre de rendu du navigateur
// ----------------------------------------------------------------------------------
static int CALLBACK WebEnumChildProc(HWND w, LPARAM lp)
{
	int rc=TRUE;
	char szClassName[50+1];

	GetClassName(w,szClassName,sizeof(szClassName));
	TRACE((TRACE_DEBUG,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
	if (strcmp(szClassName,((T_SUIVI_GETACCESSIBLE*)lp)->szClassName)==0)
	{
		((T_SUIVI_GETACCESSIBLE*)lp)->w=w;
		TRACE((TRACE_INFO,_F_,"Fenetre classe=%s w=0x%08lx",szClassName,w));
		rc=FALSE;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// SSOFirefox()
// ----------------------------------------------------------------------------------
// Principe : trouver la fenetre de contenu, ensuite, on demande un pAccessible et 
// on recurse.
// ----------------------------------------------------------------------------------
// [out] 0=OK, -1=pas réussi (champs non trouvés ou autre erreur), -2=pas la bonne URL
// ----------------------------------------------------------------------------------
int SSOFirefox(HWND w,int iAction,int iBrowser)
{
	TRACE((TRACE_ENTER,_F_, ""));

	T_SUIVI_FIREFOX tSuivi;
	T_SUIVI_GETACCESSIBLE tSuiviGetAccessible;
	int rc=-1;
	IAccessible *pAccessible=NULL;
	char *pszURL=NULL;
	HRESULT hr;

	tSuivi.pPwdAccessible=NULL;

	// On commence par rechercher l'URL, en demandant le pointeur IAccessible vers le DOM au passage
	if (iBrowser==BROWSER_FIREFOX3 || iBrowser==BROWSER_FIREFOX4)
	{
		pszURL=GetFirefoxURL(w,TRUE,&pAccessible,iBrowser,TRUE);
		if (pszURL==NULL) { TRACE((TRACE_INFO,_F_,"URL non trouvee")); rc=-2; goto end; }
		if (pAccessible==NULL) { TRACE((TRACE_ERROR,_F_,"pAccessible=NULL")); goto end; }
	}
	else if (iBrowser==BROWSER_CHROME)
	{
		tSuiviGetAccessible.w=NULL;
		strcpy_s(tSuiviGetAccessible.szClassName,sizeof(tSuiviGetAccessible.szClassName),"Chrome_RenderWidgetHostHWND");
		EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuiviGetAccessible);
		if (tSuiviGetAccessible.w==NULL) { TRACE((TRACE_ERROR,_F_,"EnumChildWindows() => pas trouve la fenetre de contenu")); goto end; }
		// Obtient un IAccessible
		hr=AccessibleObjectFromWindow(tSuiviGetAccessible.w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	}
	else goto end;
	
	tSuivi.w=w;
	tSuivi.iAction=iAction;
	tSuivi.iSuivi=0;
	tSuivi.iErreur=0;
	tSuivi.iLevel=0;
	tSuivi.bId2Done=FALSE;
	tSuivi.bId3Done=FALSE;
	tSuivi.bId4Done=FALSE;
	tSuivi.pPwdAccessible=NULL;
	if (*(gptActions[iAction].szId1Name)!=0) tSuivi.iSuivi++;
	if (*(gptActions[iAction].szId2Name)!=0) tSuivi.iSuivi++; // 0.66 gestion du 2nd identifiant
	if (*(gptActions[iAction].szId3Name)!=0) tSuivi.iSuivi++; // 0.80 gestion 3ème identifiant
	if (*(gptActions[iAction].szId4Name)!=0) tSuivi.iSuivi++; // 0.80 gestion 4ème identifiant
	if (*(gptActions[iAction].szPwdName)!=0) tSuivi.iSuivi++;
	if (*(gptActions[iAction].szValidateName)!=0) tSuivi.iSuivi++;

	TRACE((TRACE_INFO,_F_,"tSuivi.iSuivi=%d",tSuivi.iSuivi));
	
#ifdef TRACES_ACTIVEES
	DoDOMAccessible("",w,pAccessible,&tSuivi);
#else
	DoDOMAccessible(w,pAccessible,&tSuivi);
#endif
	
	TRACE((TRACE_INFO,_F_,"tSuivi.iErreur=%d tSuivi.iSuivi=%d",tSuivi.iErreur,tSuivi.iSuivi));
	if (tSuivi.iErreur==0 && tSuivi.iSuivi==0)
		rc=0;
end:
	if (tSuivi.pPwdAccessible!=NULL) tSuivi.pPwdAccessible->Release();
	if (pAccessible!=NULL) pAccessible->Release();
	if (pszURL!=NULL) free(pszURL); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

