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
// trait�es dans swSSOWin.cpp. Eh oui...
//-----------------------------------------------------------------------------

#include "stdafx.h"

// totalement bidon pour param�tre inutilis� de QueryService
const GUID refguid = {0x0c539790, 0x12e4, 0x11cf, 0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8};


typedef struct 
{
	HWND w;
	int iAction; // id action
	int iSuivi;  // suivi action 
	int iErreur; // le positionner � -1 pour sortir de la r�cursivit� de DoAccessible
	int iLevel;  // niveau de r�cursivit�
	bool bId2Done; // true d�s que le champ id2 a �t� rempli 
	bool bId3Done; // true d�s que le champ id3 a �t� rempli 
	bool bId4Done; // true d�s que le champ id3 a �t� rempli 
	IAccessible *pPwdAccessible; // pour m�moriser le champ pwd pour la frappe ENTREE
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
// R�cup�re dans le DOM le nom du tag HTML correspondant au pAccessible pass� 
// en param�tre
// ----------------------------------------------------------------------------------
// [in]  pAccessible
// [out] szNodeTagName (allou� par l'appelant taille 100+1)
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
	
	// seuls les ELEMENT (type=1) de nom INPUT ou html:input nous int�ressent (0.66 + select pour le 2nd identifiant)
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

	// lecture des attributs � la recherche de l'attribut name qui contient enfin le nom du tag
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
					// 0.71c : quelle horreur, ce sizeof retourne �videmment la taille du pointeur
					// et pas la taille du buffer... symptome = trap d�bordement de buffer
					// sur la page d'accueil du site caisse d'�pargne
					//sprintf_s(szNodeTagName,sizeof(szNodeTagName),"%S",bstrAttribValues[i]);
					sprintf_s(szNodeTagName,100+1,"%S",bstrAttribValues[i]); // pas tr�s beau, mais bon...
					rc=0;
					// attention, il faut continuer la boucle jusqu'au bout pour
					// lib�rer tous les BSTR !
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
// DoDOMAccessible() [attention, fonction r�cursive]
// ----------------------------------------------------------------------------------
// Parcours r�cursif de la fen�tre et de ses objets "accessibles". Pour chacun,
// lecture du tagname HTML pour comparer avec les valeurs attendues � renseigner.
// ----------------------------------------------------------------------------------
// [in] w : non utilis� 
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
	char szNodeTagName[100+1]; // attention � changer le sprintf_s() dans GetNodeTagName
	int rc;
	long l,lCount;

#ifdef TRACES_ACTIVEES
	char szTab[200];
	strcpy_s(szTab,sizeof(szTab),paramszTab);
#endif

	// si fini ou erreur, on termine la r�cursivit�
	if (ptSuivi->iErreur!=0) goto end;
	if (ptSuivi->iSuivi==0) goto end;
	
	// pr�paration de la lecture de l'�l�ment courant
	index.vt=VT_I4;
	index.lVal=CHILDID_SELF;
	
	// lecture du tagname de l'element
#ifdef TRACES_ACTIVEES
	rc=GetNodeTagName(szTab,pAccessible,szNodeTagName);
#else
	rc=GetNodeTagName(pAccessible,szNodeTagName);
#endif
	if (rc==0) // reussi � lire le tagname, on regarde s'il faut le remplir
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
				char *pszPassword=GetDecryptedPwd(gptActions[ptSuivi->iAction].szPwdEncryptedValue,TRUE);
				if (pszPassword!=NULL) 
				{
					KBSim(ptSuivi->w,TRUE,200,pszPassword,TRUE);				
					// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
					SecureZeroMemory(pszPassword,strlen(pszPassword));
					free(pszPassword);
				}
			}
			// m�morise les infos permettant de faire la simulation de frappe clavier sur le
			// champ mot de passe quand tout aura �t� rempli.
			pAccessible->AddRef();
			ptSuivi->pPwdAccessible=pAccessible;
			TRACE((TRACE_INFO,_F_,"ptSuivi->pPwdAccessible=0x%08lx",ptSuivi->pPwdAccessible));
			ptSuivi->iSuivi--;
			TRACE((TRACE_INFO,_F_,"ptSuivi->iSuivi=%d",ptSuivi->iSuivi));
		}
	}
	SysFreeString(bstrNodeName); bstrNodeName=NULL;
	// 0.80 : si il ne reste plus qu'une chose � faire et que c'est la validation du formulaire,
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
	// Bug al�atoire. Signal� � Aaron le 9 janvier 2005
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
		// TODO : BUG signal� � Aaron le 9 janvier 2005. Sur certains �l�ments
		// get_accChild retourne une erreur injustifi�e. 
		// Cas de test envoy� � Aaron le 22 janvier 2005 (accchildcount.zip)
		// BUG d�sormais enregistr� chez Mozilla sous la r�f�rence [Bug 278872]
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
		// 0.70 lib�ration de pIDispatch et pChild, �a peut pas faire de mal !
		// C'�tait peut-�tre � la source des consos m�moire Firefox...
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		if (ptSuivi->iSuivi==0) goto end; // on arr�te si on a trouv� tout ce qu'on veut ! (0.80)
	}

end:
	SysFreeString(bstrNodeName); bstrNodeName=NULL;
	SysFreeString(bstrValue); bstrValue=NULL;
	TRACE((TRACE_LEAVE,_F_, ""));
}

#if 0 
// Supprim�e en 0.92B8 : utilisation de NAVRELATION_EMBEDS pour rechercher le document
//                       comme pr�conis� par Mozilla (et seule solution fonctionnant � partir de FF4)
// ----------------------------------------------------------------------------------
// FirefoxEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils � la recherche de MozillaContentWindowClass pour FF3
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


// ----------------------------------------------------------------------------------
// SearchWebDocument() [attention, fonction r�cursive]
// ----------------------------------------------------------------------------------
// Parcours r�cursif de la fen�tre et de ses objets "accessibles". On s'arr�te sur le 
// premier champ document pour obtenir l'URL -- ajout�e pour ISSUE#358
// ----------------------------------------------------------------------------------
// [in] w 
// [in] pAccessible 
// ----------------------------------------------------------------------------------
#ifdef TRACES_ACTIVEES
void SearchWebDocument(char *paramszTab,IAccessible *pAccessible,T_SEARCH_DOC *ptSearchDoc)
#else
void SearchWebDocument(IAccessible *pAccessible,T_SEARCH_DOC *ptSearchDoc)
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
			pChild=NULL;

			if (ptSearchDoc->pContent!=NULL)  { TRACE((TRACE_DEBUG,_F_,"%sOn sort de la boucle...",szTab)); goto end; }

			TRACE((TRACE_DEBUG,_F_,"%s ---------------------LEVEL=%d l=%ld vt=%d lVal=0x%08lx",szTab,ptSearchDoc->iLevel,l,pVarCurrent->vt,pVarCurrent->lVal));
			if (pVarCurrent->vt!=VT_DISPATCH) goto suivant;
			if (pVarCurrent->lVal==NULL) goto suivant; // ISSUE#80 0.96B2 
			((IDispatch*)(pVarCurrent->lVal))->QueryInterface(IID_IAccessible, (void**) &pChild);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx",szTab,hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sQueryInterface(IID_IAccessible)=0x%08lx -> pChild=0x%08lx",szTab,hr,pChild));
			
			vtChild.vt=VT_I4;
			vtChild.lVal=CHILDID_SELF;
			hr=pChild->get_accRole(vtChild,&vtRole);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accRole()=0x%08lx",hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sget_accRole() vtRole.lVal=0x%08lx",szTab,vtRole.lVal));

			hr=pChild->get_accState(vtChild,&vtState);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); goto suivant; }
			TRACE((TRACE_DEBUG,_F_,"%sget_accState() vtState.lVal=0x%08lx",szTab,vtState.lVal));
			
			if (vtRole.lVal == ROLE_SYSTEM_DOCUMENT)
			{
				if (vtState.lVal & STATE_SYSTEM_INVISIBLE)
				{
					TRACE((TRACE_DEBUG,_F_,"%STATE_SYSTEM_INVISIBLE => pas le bon onglet, on passe",szTab)); 
				}
				else if (!(vtState.lVal & STATE_SYSTEM_FOCUSED)) // nouveau en 1.19B1 pour ISSUE#371
				{
					TRACE((TRACE_DEBUG,_F_,"%!STATE_SYSTEM_FOCUSED => pas le bon onglet, on passe",szTab)); 
				}
				else // trouv� ! (�limine les onglets autres que celui visible !)
				{
					TRACE((TRACE_DEBUG,_F_,"%sDOCUMENT TROUVE",szTab)); 
					ptSearchDoc->pContent=pChild;
					// ptSearchDoc->pContent->AddRef(); pas besoin de AddRef puisqu'il ne sera pas releas� grace au goto end
					goto end;
				}
			}
			else if (ptSearchDoc->iLevel!=0 || vtRole.lVal == ROLE_SYSTEM_GROUPING) // 1.17 FIX 1 : optimisation, on ne cherche au niveau d'en dessous que dans le cas d'un �l�ment group�
			{
				ptSearchDoc->iLevel++;
				SearchWebDocument(szTab,pChild,ptSearchDoc);
			}
suivant:
			if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		} // for
	}
end:
	ptSearchDoc->iLevel--;
	TRACE((TRACE_LEAVE,_F_, ""));
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// GetFirefoxURL()
// ----------------------------------------------------------------------------------
// Retourne l'URL courante de la fen�tre Firefox
// ----------------------------------------------------------------------------------
// [in] w = handle de la fen�tre
// [in] pInAccessible = si l'appelant a un pAccessible, il le passe ici
// [in] bGetAccessible = indique si on veut le pointeur pAccessible en retour
// [out] ppOutAccessible = pointeur pAccessible si demand�.
// [rc] pszURL (� lib�rer par l'appelant) ou NULL si erreur 
// ----------------------------------------------------------------------------------
char *GetFirefoxURL(HWND w,IAccessible *pInAccessible,BOOL bGetAccessible,IAccessible **ppOutAccessible,int iBrowser,BOOL bWaitReady)
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

	T_SEARCH_DOC tSearchDoc;
		
	UNREFERENCED_PARAMETER(iBrowser);

	if (pInAccessible!=NULL) // cool, l'appelant a fourni le pAccessible en entr�e, on va gagner du temps
	{
		pContent=pInAccessible;
		pContent->AddRef(); // astuce : n�cessaire sinon on va lib�rer dans le end le pointeur pass� par l'appelant
	}
	else // l'appelant n'a pas fourni le pAccessible sur le contenu de la page
	{
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	
		hr=pAccessible->accNavigate(0x1009,vtMe,&vtResult); // NAVRELATION_EMBEDS = 0x1009
		TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS)=0x%08lx",hr));
		if (hr==S_OK) // ISSUE#358 -- cette m�thode ne fonctionne plus � partir de Firefox 56, mais on la conserve pour les anciennes versions
		{
			TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS) vtEnd=0x%08lx",vtResult.lVal));
			if (vtResult.vt!=VT_DISPATCH) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) is not VT_DISPATCH")); goto end; }
			pIDispatch=(IDispatch*)vtResult.lVal;
			if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) pIDispatch=NULL")); goto end; }

			hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pContent);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }
	
			hr=pContent->get_accRole(vtMe,&vtResult);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accRole()=0x%08lx",hr)); goto end; }
			if (vtResult.lVal!=ROLE_SYSTEM_DOCUMENT) { TRACE((TRACE_ERROR,_F_,"get_accRole() : vtResult.lVal=%ld",vtResult.lVal)); goto end; }

		}
		else // ISSUE#358
		{
			// Parcourt l'ensemble de la page � la recherche de l'objet document
			tSearchDoc.pContent=NULL;
			tSearchDoc.iLevel=0;
#ifdef TRACES_ACTIVEES
			SearchWebDocument("",pAccessible,&tSearchDoc);
#else
			SearchWebDocument(pAccessible,&tSearchDoc);
#endif		
			if (tSearchDoc.pContent==NULL)
			{
				TRACE((TRACE_ERROR,_F_,"SearchWebDocument n'a pas trouv� l'objet document...")); goto end;
			}
			pContent=tSearchDoc.pContent;
		}
	}
	// ISSUE#72 (0.95) : maintenant que Firefox indique bien que le chargement de la page n'est pas termin�,
	//                   on attend patiemment avant de lancer le SSO.
	// 0.97 : on ne le fait que si bWaitReady (et notamment on ne le fait pas dans le cas des popups cf. ISSUE#87)
	if (bWaitReady)
	{
		hr=pContent->get_accState(vtMe,&vtResult);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accState()=0x%08lx",hr)); goto end; }
		if (vtResult.lVal & STATE_SYSTEM_BUSY) { TRACE((TRACE_ERROR,_F_,"get_accState() : STATE_SYSTEM_BUSY, on verra plus tard !")); goto end; }
	}

	hr=pContent->get_accValue(vtMe,&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pContent->get_accValue()=0x%08lx",hr)); goto end; }

	pszURL=GetSZFromBSTR(bstrURL);
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",pszURL));
	
	if (bGetAccessible) // l'appelant veut r�cup�rer le pAccessible en sortie pour usage futur, charge � lui de le lib�rer ensuite
	{
		*ppOutAccessible=pContent;
		pContent->AddRef(); // astuce : n�cessaire pour que le ppAccessible retourn� reste valide malgr� le pContent->Release() du end
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
// enum des fils � la recherche de la fen�tre de rendu du navigateur
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
// [out] 0=OK, -1=pas r�ussi (champs non trouv�s ou autre erreur), -2=pas la bonne URL
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
		pszURL=GetFirefoxURL(w,NULL,TRUE,&pAccessible,iBrowser,TRUE);
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
	if (*(gptActions[iAction].szId3Name)!=0) tSuivi.iSuivi++; // 0.80 gestion 3�me identifiant
	if (*(gptActions[iAction].szId4Name)!=0) tSuivi.iSuivi++; // 0.80 gestion 4�me identifiant
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

