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
// swSSOXeb.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

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
// DoWebAccessible() [attention, fonction récursive]
// ----------------------------------------------------------------------------------
// Parcours récursif de la fenêtre et de ses objets "accessibles". On s'arrête sur le 
// premier champ de saisie protégé : c'est le champ mot de passe à remplir.
// ----------------------------------------------------------------------------------
// [in] w 
// [in] pAccessible 
// [in] ptSuivi 
// ----------------------------------------------------------------------------------
#ifdef TRACES_ACTIVEES
static void DoWebAccessible(char *paramszTab,HWND w,IAccessible *pAccessible,T_SUIVI_ACCESSIBLE *ptSuivi)
#else
static void DoWebAccessible(HWND w,IAccessible *pAccessible,T_SUIVI_ACCESSIBLE *ptSuivi)
#endif
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
	
#ifdef TRACES_ACTIVEES
	char szTab[200];
	strcpy_s(szTab,sizeof(szTab),paramszTab);
	if (strlen(szTab)<sizeof(szTab)-5) strcat_s(szTab,sizeof(szTab),"  ");
#endif
	
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
			((IDispatch*)(pVarCurrent->lVal))->QueryInterface(IID_IAccessible, (void**) &pChild);
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
			
			//hr=pChild->get_accName(vtChild,&bstrName);
			//if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accName()=0x%08lx",hr)); goto suivant; }
			//TRACE((TRACE_DEBUG,_F_,"%sget_accName() name=%S",szTab,bstrName));
			
			// Reconnaissance du champ mot de passe : Nième champ ayant pour role et state les valeurs suivantes :
			// - Role = ROLE_SYSTEM_TEXT
			// - State = (STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED) & STATE_SYSTEM_PROTECTED

			// Reconnaissance du champ identifiant : Nième champ précédant le champ mot de passe et ayant pour role et state les valeurs suivantes :
			// - Role = ROLE_SYSTEM_TEXT
			// - State = (STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED)

			// ISSUE#279 : cas spécifique IE
			// TRACE((TRACE_DEBUG,_F_,"%sptSuivi->iBrowser=%d",szTab,ptSuivi->iBrowser));
			if (ptSuivi->iBrowser!=BROWSER_IE)
			{
				// ISSUE#340 : accepte aussi les listes saisissables
				// if ((vtRole.lVal == ROLE_SYSTEM_TEXT) && 
				if ((vtRole.lVal == ROLE_SYSTEM_TEXT || vtRole.lVal==ROLE_SYSTEM_COMBOBOX) && 
					((vtState.lVal & STATE_SYSTEM_FOCUSED) || (vtState.lVal & STATE_SYSTEM_FOCUSABLE)))
				{
					// c'est un champ de saisie, s'il est protégé c'est le mdp sinon c'est un id
					if (vtState.lVal & STATE_SYSTEM_PROTECTED)
					{
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS* + STATE_SYSTEM_PROTECTED)"));
						pChild->AddRef();
						ptSuivi->pTextFields[ptSuivi->iTextFieldIndex]=pChild;
						ptSuivi->iNbPwdFound++; 
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve : c'est le %dieme, on attendait le %d",ptSuivi->iNbPwdFound,atoi(gptActions[ptSuivi->iAction].szPwdName)));
						if (ptSuivi->iNbPwdFound==atoi(gptActions[ptSuivi->iAction].szPwdName)) 
						{
							ptSuivi->iPwdIndex=ptSuivi->iTextFieldIndex;
						}
						ptSuivi->iTextFieldIndex++;
					}
					else
					{
						TRACE((TRACE_INFO,_F_,"Un champ id trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS*)"));
						pChild->AddRef();
						ptSuivi->pTextFields[ptSuivi->iTextFieldIndex]=pChild;
						ptSuivi->iTextFieldIndex++;
					}
				}
				else
				{
	#ifdef TRACES_ACTIVEES
					DoWebAccessible(szTab,NULL,pChild,ptSuivi);
	#else
					DoWebAccessible(NULL,pChild,ptSuivi);
	#endif
				}
			}
			else // ISSUE#279 : IE
			{
				if ((vtRole.lVal == ROLE_SYSTEM_TEXT) && !(vtState.lVal & STATE_SYSTEM_READONLY))
				{
					// c'est un champ de saisie, s'il est protégé c'est le mdp sinon c'est un id
					if (vtState.lVal & STATE_SYSTEM_PROTECTED)
					{
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS* + STATE_SYSTEM_PROTECTED)"));
						pChild->AddRef();
						ptSuivi->pTextFields[ptSuivi->iTextFieldIndex]=pChild;
						ptSuivi->iNbPwdFound++; 
						TRACE((TRACE_INFO,_F_,"Champ mot de passe trouve : c'est le %dieme, on attendait le %d",ptSuivi->iNbPwdFound,atoi(gptActions[ptSuivi->iAction].szPwdName)));
						if (ptSuivi->iNbPwdFound==atoi(gptActions[ptSuivi->iAction].szPwdName)) 
						{
							ptSuivi->iPwdIndex=ptSuivi->iTextFieldIndex;
						}
						ptSuivi->iTextFieldIndex++;
					}
					else
					{
						TRACE((TRACE_INFO,_F_,"Un champ id trouve (ROLE_SYSTEM_TEXT + STATE_SYSTEM_FOCUS*)"));
						pChild->AddRef();
						ptSuivi->pTextFields[ptSuivi->iTextFieldIndex]=pChild;
						ptSuivi->iTextFieldIndex++;
					}
				}
				else
				{
	#ifdef TRACES_ACTIVEES
					DoWebAccessible(szTab,NULL,pChild,ptSuivi);
	#else
					DoWebAccessible(NULL,pChild,ptSuivi);
	#endif
				}

			}
suivant:
			if (pChild!=NULL) { pChild->Release(); pChild=NULL; }
		}
	}
	
end:
	if (pArray!=NULL) delete[] pArray;
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// GetAbsolutePos()
// ----------------------------------------------------------------------------------
// Retourne la position absolue du champ recherché dans la page
// ----------------------------------------------------------------------------------
// [out] -1 = champ recherché mais non trouvé => ERREUR
//       -2 = champ n'était pas recherché => PAS ERREUR
// ----------------------------------------------------------------------------------
static int GetAbsolutePos(const char *pszIdName,const char *pszPwdName,int iPwdIndex,T_SUIVI_ACCESSIBLE *ptSuivi)
{
	int rc=-1;
	int iRelativePos,iAbsolutePos;

	TRACE((TRACE_ENTER,_F_, "szIdName=%s szPwdName=%s iPwdIndex=%d",pszIdName,pszPwdName,iPwdIndex));
	
	if (*pszIdName==0) { rc=-2; goto end; }
	
	if (atoi(pszPwdName)==0) // pas de champ mot de passe recherché, donc position champ id exprimée en valeur absolue
	{
		iRelativePos=-1;
		iAbsolutePos=atoi(pszIdName)-1;
	}
	else // position relative par rapport à la position du champ mot de passe
	{
		iRelativePos=atoi(pszIdName);
		iAbsolutePos=iPwdIndex+iRelativePos;
	}
	TRACE((TRACE_INFO,_F_,"iPwdIndex=%d iRelativePos=%d iAbsolutePos=%d",ptSuivi->iPwdIndex,iRelativePos,iAbsolutePos));
	if (iAbsolutePos<0 || iAbsolutePos>MAX_TEXT_FIELDS) 
	{ 
		TRACE((TRACE_ERROR,_F_,"Erreur sur champ %s (iAbsolutePos=%d !) -> SSO NOK",pszIdName,iAbsolutePos)); 
		goto end; 
	}
	if (ptSuivi->pTextFields[iAbsolutePos]==NULL)
	{
		TRACE((TRACE_ERROR,_F_,"Erreur sur champ %s (pTextFields(iAbsolutePos)=NULL !) -> SSO NOK",pszIdName)); 
		goto end; 
	}
	rc=iAbsolutePos;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


// ----------------------------------------------------------------------------------
// SSOWebAccessible()
// ----------------------------------------------------------------------------------
// SSO commun aux navigateurs implémentant l'interface IAccessible
// Utilise forcément la méthode de configuration simplifiée
// ----------------------------------------------------------------------------------
// [out] 0=OK, -1=pas réussi (champs non trouvés ou autre erreur)
// ----------------------------------------------------------------------------------
int SSOWebAccessible(HWND w,int iAction,int iBrowser)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d iBrowser=%d",w,iAction,iBrowser));
	int rc=-1;
	IAccessible *pAccessible=NULL;
	IAccessible *pTopAccessible=NULL;
	IDispatch *pIDispatch=NULL;
	int i;	
	T_SUIVI_ACCESSIBLE tSuivi,*ptSuivi;
	HRESULT hr;
	long lCount;
	VARIANT vtChild,vtState;
	int iId1Index;
	int iId2Index;
	int iId3Index;
	int iId4Index;
	IAccessible *pNiveau0=NULL,*pChildNiveau1=NULL, *pChildNiveau2=NULL;
	int iNbTry;
	
	// ISSUE#39 : important, initialisation pour que le pointer iAccessible soit à NULL sinon le release provoque plantage !
	ZeroMemory(&tSuivi,sizeof(T_SUIVI_ACCESSIBLE));

	// 	BROWSER_MAXTHON: strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"Afx:400000:2008:10011:0:0");
	tSuivi.w=NULL;		
	if (iBrowser==BROWSER_IE)
	{
		// enum des fils à la recherche de la fenêtre de rendu Web
		// ISSUE#70 0.95 essaie aussi avec contenu flash (à faire en 1er)
		// pour tester le flash : http://itmj.homelinux.org:9090/workspace/Main.html?ap=1
		strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"MacromediaFlashPlayerActiveX"); 
		EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuivi);
		if (tSuivi.w!=NULL) { TRACE((TRACE_INFO,_F_,"MacromediaFlashPlayerActiveX. Visible = %d",IsWindowVisible(tSuivi.w))); }
		if (tSuivi.w==NULL || ((tSuivi.w!=NULL) && (!IsWindowVisible(tSuivi.w)))) // pas trouvé de flash ou trouvé flash mais non visible
		{
			strcpy_s(tSuivi.szClassName,sizeof(tSuivi.szClassName),"Internet Explorer_Server");
			EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuivi);
			if (tSuivi.w!=NULL) { TRACE((TRACE_INFO,_F_,"Internet Explorer_Server = %d",IsWindowVisible(tSuivi.w))); }
		}
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
		
		VARIANT vtStart,vtResult;
		vtStart.vt=VT_I4;
		vtStart.lVal=CHILDID_SELF;
		hr=pTopAccessible->accNavigate(0x1009,vtStart,&vtResult); // NAVRELATION_EMBEDS = 0x1009
		pTopAccessible->Release();pTopAccessible=NULL;
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS)=0x%08lx",hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"accNavigate(NAVRELATION_EMBEDS) vtEnd=%ld",vtResult.lVal));
		if (vtResult.vt!=VT_DISPATCH) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) is not VT_DISPATCH")); goto end; }
		pIDispatch=(IDispatch*)vtResult.lVal;
		if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"accNavigate(NAVRELATION_EMBEDS) pIDispatch=NULL")); goto end; }
		hr=pIDispatch->QueryInterface(IID_IAccessible, (void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"QueryInterface(IID_IAccessible)=0x%08lx",hr)); goto end; }	
	}
	else if (iBrowser==BROWSER_XIN) // test XIN
	{
		// Obtient un IAccessible
		hr=AccessibleObjectFromWindow(w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"Unknown browser : %d",iBrowser)); goto end;
	}
	// à ce stade, on a un pAccessible pour travailler, quel que soit le navigateur
	vtChild.vt=VT_I4;
	vtChild.lVal=CHILDID_SELF;
	VariantInit(&vtState);
	
	hr=pAccessible->get_accState(vtChild,&vtState);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_accState(DOCUMENT PRINCIPAL) vtState.lVal=0x%08lx",vtState.lVal));
	
	// ISSUE#163 : plutôt que d'attendre 1 fois 5 secondes, on attend 5 fois 100ms et ensuite on continue, 
	//             ça ne semble pas être bloquant surtout que Chrome et IE ont l'air de se mettre BUSY dès qu'ils n'ont pas le focus...
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
	
	lCount=0;
	hr=pAccessible->get_accChildCount(&lCount);
	TRACE((TRACE_DEBUG,_F_,"get_accChildCount() hr=0x%08lx lCount=%ld",hr,lCount));
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accChildCount() hr=0x%08lx",hr)); goto end; }
	if (iBrowser==BROWSER_CHROME) // lCount=0 arrive parfois quelque fois après ouverture d'un nouvel onglet
	{
		iNbTry=0;
		while (lCount==0 && iNbTry<10) // ajouté en 0.93B1 : 10 essais supplémentaires au lieu d'un seul
		{
			Sleep(150);
			pAccessible->Release();
			pAccessible=NULL;
			// Obtient un IAccessible
			hr=AccessibleObjectFromWindow(tSuivi.w,(DWORD)OBJID_CLIENT,IID_IAccessible,(void**)&pAccessible);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"AccessibleObjectFromWindow(IID_IAccessible)=0x%08lx",hr)); goto end; }
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

	ZeroMemory(&tSuivi,sizeof(T_SUIVI_ACCESSIBLE)); // déplacé plus haut, mais laissé ici aussi dans le doute ça coute pas cher.
	tSuivi.w=w;
	tSuivi.iAction=iAction;
	tSuivi.iErreur=0;
	tSuivi.iLevel=0;
	tSuivi.iPwdIndex=-1;
	tSuivi.iNbPwdFound=0;
	tSuivi.iBrowser=iBrowser; // ISSUE#279

#ifdef TRACES_ACTIVEES
	DoWebAccessible("",w,pAccessible,&tSuivi);
#else
	DoWebAccessible(w,pAccessible,&tSuivi);
#endif
	
	TRACE((TRACE_INFO,_F_,"tSuivi.iErreur=%d",tSuivi.iErreur));
	if (tSuivi.iErreur==0) 
	{
		ptSuivi=&tSuivi;
		vtChild.vt=VT_I4;
		vtChild.lVal=CHILDID_SELF;
		
		// 0.93B1 / ISSUE#40 : avant de démarrer les saisies, il faut vérifier qu'on a trouvé tous les champs attendus
		// En effet, comme on ne cherche plus les champs par leurs noms, on peut provoquer des mises au premier plan intempestives
		// de la fenêtre du navigateur si le titre et l'URL ne permettent pas de reconnaitre la page de login de manière certaine
		TRACE((TRACE_INFO,_F_,"Page analysee, verification (lCount=%d ptSuivi->iTextFieldIndex=%d)",lCount,ptSuivi->iTextFieldIndex));
		// 0.99B3 / ISSUE#103 : En mode configration simplifiée, la position du champ identifiant est considérée comme absolue 
		//                      si le champ de mot de passe est configuré à 0
		// if (*gptActions[ptSuivi->iAction].szPwdName!=0 && ptSuivi->iPwdIndex==-1)
		if (*gptActions[ptSuivi->iAction].szPwdName!=0 && atoi(gptActions[ptSuivi->iAction].szPwdName)!=0 && ptSuivi->iPwdIndex==-1)
		{
			TRACE((TRACE_ERROR,_F_,"Un champ mot de passe etait attendu mais n'a pas ete trouve => le SSO ne sera pas execute"));
			goto end;
		}
		iId1Index=GetAbsolutePos(gptActions[ptSuivi->iAction].szId1Name,gptActions[ptSuivi->iAction].szPwdName,ptSuivi->iPwdIndex,ptSuivi);
		iId2Index=GetAbsolutePos(gptActions[ptSuivi->iAction].szId2Name,gptActions[ptSuivi->iAction].szPwdName,ptSuivi->iPwdIndex,ptSuivi);
		iId3Index=GetAbsolutePos(gptActions[ptSuivi->iAction].szId3Name,gptActions[ptSuivi->iAction].szPwdName,ptSuivi->iPwdIndex,ptSuivi);
		iId4Index=GetAbsolutePos(gptActions[ptSuivi->iAction].szId4Name,gptActions[ptSuivi->iAction].szPwdName,ptSuivi->iPwdIndex,ptSuivi);
		if (iId1Index==-1 || iId2Index==-1 || iId3Index==-1 || iId4Index==-1)
		{
			TRACE((TRACE_ERROR,_F_,"Au moins un des champs n'a pas ete trouve => le SSO ne sera pas execute"));
			goto end;
		}
		
		// Vérification OK, on peut mettre la fenêtre au premier plan et démarrer les saisies 
		TRACE((TRACE_INFO,_F_,"Verifications OK, demarrage des saisies (lCount=%d ptSuivi->iTextFieldIndex=%d)",lCount,ptSuivi->iTextFieldIndex));
		SetForegroundWindow(ptSuivi->w);
		
		if (iId1Index>=0) PutAccValueWeb(ptSuivi->w,ptSuivi->pTextFields[iId1Index],vtChild,gptActions[ptSuivi->iAction].szId1Value,iAction,iBrowser);
		if (iId2Index>=0) PutAccValueWeb(ptSuivi->w,ptSuivi->pTextFields[iId2Index],vtChild,gptActions[ptSuivi->iAction].szId2Value,iAction,iBrowser);
		if (iId3Index>=0) PutAccValueWeb(ptSuivi->w,ptSuivi->pTextFields[iId3Index],vtChild,gptActions[ptSuivi->iAction].szId3Value,iAction,iBrowser);
		if (iId4Index>=0) PutAccValueWeb(ptSuivi->w,ptSuivi->pTextFields[iId4Index],vtChild,gptActions[ptSuivi->iAction].szId4Value,iAction,iBrowser);
		
		// Mdp
		if (ptSuivi->iPwdIndex!=-1)
		{
			// CODE A GARDER POUR BUGS OUVERTS CHEZ CHROME #75908 et #75911
			/*
			int iAntiLoop=0;
			hr=ptSuivi->pTextFields[ptSuivi->iPwdIndex]->accSelect(SELFLAG_TAKEFOCUS,vtChild);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"accSelect(SELFLAG_TAKEFOCUS)=0x%08lx",hr)); } 
			TRACE((TRACE_DEBUG,_F_,"accSelect(SELFLAG_TAKEFOCUS)=0x%08lx",hr));
			VARIANT vtSelf;
			VARIANT vtState;
			vtSelf.vt=VT_I4;
			vtSelf.lVal=CHILDID_SELF;
			hr=ptSuivi->pTextFields[ptSuivi->iPwdIndex]->get_accState(vtSelf,&vtState);
			TRACE((TRACE_DEBUG,_F_,"get_accState() vtState.lVal=0x%08lx",vtState.lVal));
			while (iAntiLoop <50)
			//while ((!(vtState.lVal & STATE_SYSTEM_FOCUSED)) && iAntiLoop <10)
			{
				Sleep(100);
				KBSimEx(w,"[TAB]","","","","","");
				Sleep(100);
				hr=ptSuivi->pTextFields[ptSuivi->iPwdIndex]->get_accState(vtSelf,&vtState);
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accState(CHILDID_SELF)=0x%08lx",hr)); }
				TRACE((TRACE_DEBUG,_F_,"get_accState() vtState.lVal=0x%08lx",vtState.lVal));
				iAntiLoop++;
			}
			*/
			Sleep(100); // ISSUE#163 (et autres problèmes, notamment identifiant saisi tronqué avec le reste dans le mot de passe)

			hr=ptSuivi->pTextFields[ptSuivi->iPwdIndex]->accSelect(SELFLAG_TAKEFOCUS,vtChild);
			if (FAILED(hr)) // ISSUE#251 : en cas d'erreur du accSelect (vu UNE fois), on fait TAB à l'aveugle, ça évite de taper le mdp dans le champ id
			{ 
				TRACE((TRACE_ERROR,_F_,"accSelect(SELFLAG_TAKEFOCUS)=0x%08lx",hr)); 
				// remarque : nombre de tab à faire = fonction de la position relative du champ id par rapport au champ mdp
				Sleep(100);
				for (i=0; i<abs(atoi(gptActions[ptSuivi->iAction].szId1Name));i++) { KBSimEx(w,"[TAB]","","","","",""); }
				Sleep(100);
			} 
			if ((*gptActions[ptSuivi->iAction].szPwdEncryptedValue!=0))
			{
				//char *pszPassword=swCryptDecryptString(gptActions[ptSuivi->iAction].szPwdEncryptedValue,ghKey1);
				char *pszPassword=GetDecryptedPwd(gptActions[ptSuivi->iAction].szPwdEncryptedValue);
				if (pszPassword!=NULL) 
				{
					// 1.09B2 : essaie de faire put_accValue avant de se rabattre sur la simulation de frappe
					BSTR bstrValue=GetBSTRFromSZ(pszPassword);
					hr=S_OK;
					if (bstrValue!=NULL)
					{
						hr=ptSuivi->pTextFields[ptSuivi->iPwdIndex]->put_accValue(vtChild,bstrValue);
						TRACE((TRACE_INFO,_F_,"pAccessible->put_accValue() : hr=0x%08lx",hr));
					}
					if (bstrValue==NULL || FAILED(hr))
					{
						KBSimWeb(ptSuivi->w,TRUE,100,pszPassword,TRUE,iAction,iBrowser,ptSuivi->pTextFields[ptSuivi->iPwdIndex],vtChild);
					}
					if (bstrValue!=NULL)
					{
						SecureZeroMemory(bstrValue,SysStringByteLen(bstrValue));
						SysFreeString(bstrValue); bstrValue=NULL;
					}
					SecureZeroMemory(pszPassword,strlen(pszPassword));
					free(pszPassword);
				}
			}
		}
		// Validation si demandée
		if (*gptActions[ptSuivi->iAction].szValidateName!=0)
		{
			Sleep(100);
			// ISSUE#101
			// KBSimEx(NULL,gptActions[ptSuivi->iAction].szValidateName,"","","","","");
			// ISSUE#101 suite : on autorise aussi le mot de passe sinon c'est naze...
			char szDecryptedPassword[LEN_PWD+1];
			// char *pszPassword=swCryptDecryptString(gptActions[ptSuivi->iAction].szPwdEncryptedValue,ghKey1);
			char *pszPassword=GetDecryptedPwd(gptActions[ptSuivi->iAction].szPwdEncryptedValue);
			if (pszPassword!=NULL) 
			{
				strcpy_s(szDecryptedPassword,sizeof(szDecryptedPassword),pszPassword);
				SecureZeroMemory(pszPassword,strlen(pszPassword));
				free(pszPassword);
			}
			else
			{
				*szDecryptedPassword=0;
			}
			if (!CheckIfURLStillOK(w,iAction,iBrowser,NULL,FALSE,NULL)) goto end;
			KBSimEx(NULL,gptActions[ptSuivi->iAction].szValidateName,
						 gptActions[iAction].szId1Value,
						 gptActions[iAction].szId2Value,
						 gptActions[iAction].szId3Value,
						 gptActions[iAction].szId4Value,szDecryptedPassword);
			SecureZeroMemory(szDecryptedPassword,sizeof(szDecryptedPassword));
		}
		guiNbWEBSSO++;
	}
	rc=0;
end:
	for (i=0;i<MAX_TEXT_FIELDS;i++)	if (tSuivi.pTextFields[i]!=NULL) tSuivi.pTextFields[i]->Release();
	if (pAccessible!=NULL) pAccessible->Release();
	if (pTopAccessible!=NULL) pTopAccessible->Release();
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pNiveau0!=NULL) pNiveau0->Release();
	if (pChildNiveau1!=NULL) pChildNiveau1->Release();
	if (pChildNiveau2!=NULL) pChildNiveau2->Release();
	if (gpAccessibleChromeURL!=NULL) { gpAccessibleChromeURL->Release(); gpAccessibleChromeURL=NULL; }

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
