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
// swSSOWeb.cpp
//-----------------------------------------------------------------------------
// SSO pages Web Internet Explorer seulement 
// (Firefox et Mozilla dans swSSOFirefox.cpp)
//-----------------------------------------------------------------------------

#include "stdafx.h"

UINT guiHTMLGetObjectMsg;
//LPFNOBJECTFROMLRESULT gpfObjectFromLresult=NULL;

// globales locales
static HINSTANCE ghiOLEACCDLL=NULL;

const char gcszFormNoName1[]="<SANSNOM>";
const char gcszFormNoName2[]="[SANSNOM]";

// noms et valeurs du formulaire et des champs id et pwd � remplir.
static BSTR bstrFormName=NULL;
static BSTR bstrId1Name=NULL;
static BSTR bstrId1Value=NULL;
static BSTR bstrId2Name=NULL;
static BSTR bstrId2Value=NULL;
static BSTR bstrId3Name=NULL;
static BSTR bstrId3Value=NULL;
static BSTR bstrId4Name=NULL;
static BSTR bstrId4Value=NULL;
static BSTR bstrPwdName=NULL;
static BSTR bstrPwdValue=NULL;
static int  lenFormName=0;
static int  lenId1Name=0;
static int  lenId2Name=0;
static int  lenId3Name=0;
static int  lenId4Name=0;
static int  lenId1Value=0;
static int  lenId2Value=0;
static int  lenId3Value=0;
static int  lenId4Value=0;
static int  lenPwdName=0;
static int  lenPwdValue=0;
static int  id2Type=0;
static int  id3Type=0;
static int  id4Type=0;

static char szURL[LEN_URL+1]; // ISSUE#174 (remplacement de 128 par LEN_URL...)
static char szFormName[90+1]; // TODO moche (on a d�j� l'info dans la config)

typedef struct 
{
	HWND w;
	int iNbActions;
	int iAction; // 0.96 pour changement de mdp
} T_SUIVI_IE;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

//-----------------------------------------------------------------------------
// ParseFrame()
//-----------------------------------------------------------------------------
// Enum�ration des �l�ments d'une frame � la recherche des champs � remplir
//-----------------------------------------------------------------------------
// [in] pHTMLDocument2 : pointeur vers la frame
//-----------------------------------------------------------------------------
static void ParseFrame(IHTMLDocument2 *pHTMLDocument2,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "pHTMLDocument2=0x%08lx lp=0x%08lx",pHTMLDocument2,lp));

	HRESULT hr;

	IHTMLElementCollection *pItems=NULL;
	IDispatch *pIDispatch=NULL;
	IHTMLInputElement *pItem=NULL;
	IHTMLControlElement *pPwdField=NULL;
	IHTMLSelectElement *pCombo=NULL;
	IHTMLFormElement *pForm=NULL,*pTargetForm=NULL;
  	long lNbElements;
  	long l;
  	int len;
	BSTR bstr=NULL;	
	BSTR bstrForm=NULL;
	T_SUIVI_IE *ptSuivi=(T_SUIVI_IE*)lp;
	
   	// r�cup�ration liste des items composant le document HTML
	hr = pHTMLDocument2->get_all(&pItems);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pHTMLDocument2->get_all()=0x%08lx",hr)); goto end; }
  	TRACE((TRACE_DEBUG,_F_,"get_all hr=0x%08lx pItems=0x%08lx",hr,pItems));

	// r�cup�ration du nb d'items 
   	hr=pItems->get_length(&lNbElements);
   	if (FAILED(hr))	{ TRACE((TRACE_ERROR,_F_,"pItems->get_length()=0x%08lx",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"get_length hr=0x%08lx lNbElements=%d",hr,lNbElements));

	// �num�ration des items du document. On devrait y trouver :
	// - le formulaire
	// - le ou les champs identifiants
	// - le champ mot de passe
	for(l=0;l<lNbElements;l++)
	{
        VARIANT index; 
        index.vt=VT_I4;
        index.lVal=l;
		
		hr=pItems->item(index,index,&pIDispatch);
		if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pItems->item(%d)=0x%08lx",l,hr)); goto end; }
		TRACE((TRACE_DEBUG,_F_,"item[%d] pIDispatch=0x%08lx",l,pIDispatch));
		// ISSUE#229 : la m�thode "item" de IHTMLElementCollection retourne S_OK m�me si l'�l�ment n'est pas trouv� (si, si)
		// Du coup il faut imp�rativement v�rifier que le pointeur retourn� n'est pas NULL (c'est document� comme �a)
		if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"item[%d] pIDispatch=0x%08lx",l,pIDispatch)); goto end; }
		
		// si formulaire � valider (=non vide + pas ENTER), on cherche le formulaire dans la page HTML, sinon on passe � la suite
		//if (lenFormName!=0 && strcmp(szFormName,gcszEnter)!=0) : 0.89 : on accepte les simulations de frappe complexes
		// ISSUE#357 - 1.16B3 : si le nom du formulaire est [SANSNOM], il ne faut pas confondre avec une simulation de frappe
		if (lenFormName!=0 && (*szFormName!='[' || strcmp(szFormName,gcszFormNoName2)==0))
		{
			hr=pIDispatch->QueryInterface(IID_IHTMLFormElement,(void **)&pForm);
			TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLFormElement(%d)=0x%08lx",l,hr));
			if (SUCCEEDED(hr)) 
			{
				// l'item r�pond � l'interface IHTMLFormElement
				// on regarde si c'est le formulaire attendu
				hr=pForm->get_name(&bstrForm);
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pForm->get_name()=0x%08lx",hr)); goto end; }
				TRACE((TRACE_DEBUG,_F_,"pForm->get_name='%S'",bstrForm));
				
				len=SysStringByteLen(bstrForm);
				if (len==0) // 0.42 traitement des formulaire sans nom...
				{
					if (strcmp(szFormName,gcszFormNoName1)==0 || strcmp(szFormName,gcszFormNoName2)==0)
					{
						// c'est lui ! Euh... enfin si on a de la chance et 
						// qu'il n'y en a qu'un sans nom, sinon on est perdu
						pTargetForm=pForm;
						pForm=NULL;
						TRACE((TRACE_INFO,_F_,"pTargetForm=0x%08lx",pTargetForm));
						SysFreeString(bstrForm); bstrForm=NULL;
					}
				}
				else if (len==lenFormName && memcmp(bstrForm,bstrFormName,len)==0) 
				{
					TRACE((TRACE_INFO,_F_,"Formulaire trouve : '%S'",bstrForm));
					// on sauvegarde un pointeur vers cet item de mani�re
					// � soumettre le formulaire une fois que les champs
					// identifiant et mot de passe auront �t� remplis
					pTargetForm=pForm;
					// surtout ne pas lib�rer le pForm !!!
					// donc je le mets � NULL... la lib� du pTargetForm
					// plus tard r�soudra l'affaire !
					pForm=NULL;
					TRACE((TRACE_INFO,_F_,"pTargetForm=0x%08lx",pTargetForm));
					SysFreeString(bstrForm); bstrForm=NULL;
				}
				// pas la peine d'essayer le Query sur IHTMLInputElement donc :
				goto suite;
			}
		}
		// 0.63B4 : on ne cherche � remplir les champs que lorsque le formulaire a �t� trouv�.
		//          SAUF quand la validation du formulaire n'est pas demand�e !
		if (pTargetForm==NULL && lenFormName!=0 && *szFormName!='[') goto suite;

		hr=pIDispatch->QueryInterface(IID_IHTMLInputElement,(void **)&pItem);
		TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLInputElement(%d)=0x%08lx",l,hr));
		// l'item r�pond � l'interface IHTMLInputElement
		// on regarde si c'est le champ de saisie attendu
		if (SUCCEEDED(hr)) 
		{
			hr=pItem->get_name(&bstr);
			if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pForm->get_name()=0x%08lx",hr)); goto end; }
			TRACE((TRACE_DEBUG,_F_,"pItem->get_name='%S'",bstr));
	
			len=SysStringByteLen(bstr);
			if (lenId1Name!=0) // si 0, c'est qu'on n'a pas de champ id1 � remplir
			{
				if (len==lenId1Name && memcmp(bstr,bstrId1Name,lenId1Name)==0)
				{
					// c'est le champ id1, on le remplit
					SetForegroundWindow(ptSuivi->w);
					TRACE((TRACE_INFO,_F_,"Champ id1 trouv�   : '%S'",bstr));
					pItem->put_value(bstrId1Value);
					TRACE((TRACE_INFO,_F_,"Valeur id1 saisie  : '%S'",bstrId1Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes  : %d",ptSuivi->iNbActions));
				}
			}
			if (lenId2Name!=0 && id2Type==EDIT) // si 0, c'est qu'on n'a pas de champ id2 � remplir
			{
				if (len==lenId2Name && memcmp(bstr,bstrId2Name,lenId2Name)==0)
				{
					Sleep(50);
					// c'est le champ id2, on le remplit
					TRACE((TRACE_INFO,_F_,"Champ id2 trouv�   : '%S'",bstr));
					pItem->put_value(bstrId2Value);
					TRACE((TRACE_INFO,_F_,"Valeur id2 saisie  : '%S'",bstrId2Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes  : %d",ptSuivi->iNbActions));
					Sleep(50);
				}
			}
			if (lenId3Name!=0 && id3Type==EDIT) // si 0, c'est qu'on n'a pas de champ id3 � remplir
			{
				if (len==lenId3Name && memcmp(bstr,bstrId3Name,lenId3Name)==0)
				{
					Sleep(50);
					// focus obligatoire sur le champ nouveau mot de passe en mode changement de mot de passe
					if (gptActions[ptSuivi->iAction].iType==WEBPWD)
					{
						hr=pIDispatch->QueryInterface(IID_IHTMLControlElement,(void **)&pPwdField);
						TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLControlElement(pwd)=0x%08lx",hr));
						if (pPwdField!=NULL)
						{
							pPwdField->focus();
							pPwdField->Release();
							pPwdField=NULL;
						}
					}
					// c'est le champ id3, on le remplit
					TRACE((TRACE_INFO,_F_,"Champ id3 trouv�   : '%S'",bstr));
					pItem->put_value(bstrId3Value);
					TRACE((TRACE_INFO,_F_,"Valeur id3 saisie  : '%S'",bstrId3Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes  : %d",ptSuivi->iNbActions));
					Sleep(50);
				}
			}
			if (lenId4Name!=0 && id4Type==EDIT) // si 0, c'est qu'on n'a pas de champ id4 � remplir
			{
				if (len==lenId4Name && memcmp(bstr,bstrId4Name,lenId4Name)==0)
				{
					Sleep(50);
					// c'est le champ id4, on le remplit
					TRACE((TRACE_INFO,_F_,"Champ id4 trouv�   : '%S'",bstr));
					pItem->put_value(bstrId4Value);
					TRACE((TRACE_INFO,_F_,"Valeur id4 saisie  : '%S'",bstrId4Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes  : %d",ptSuivi->iNbActions));
					Sleep(50);
				}
			}
			if (lenPwdName!=0) // si 0, c'est qu'on n'a pas de champ pwd � remplir
			{
				if (len==lenPwdName && memcmp(bstr,bstrPwdName,lenPwdName)==0)
				{
					// c'est le champ PWD, on le remplit
					TRACE((TRACE_DEBUG,_F_,"len=%d lenPwdName=%d bstr=%S bstrPwdName=%S",len,lenPwdName,bstr,bstrPwdName));

					//if (strcmp(szFormName,gcszEnter)==0)
					if (*szFormName=='[')
					{
						// essaie de mettre le focus sur le champ mot de passe pour pr�parer la saisie de la touche
						// ENTREE + tard. N�cessaire de le faire avant de remplir le mot de passe pour traiter les 
						// cas tordus type ING Direct o� le champ est r�initialis� � chaque fois qu'il recoit le focus
						// (du coup si on saisit et qu'on met le focus ensuite, on perd ce qu'on a saisi)
						hr=pIDispatch->QueryInterface(IID_IHTMLControlElement,(void **)&pPwdField);
						TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLControlElement(pwd)=0x%08lx",hr));
						if (pPwdField!=NULL)
						{
							pPwdField->focus();
							pPwdField->Release();
							pPwdField=NULL;
						}
					}
					TRACE((TRACE_INFO,_F_,"Champ PWD trouv�  : '%S'",bstr));
					pItem->put_value(bstrPwdValue);
					//TRACE((TRACE_PWD,_F_,"Valeur PWD saisie : '%S'",bstrPwdValue));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes : %d",ptSuivi->iNbActions));
				}
				// pas la peine d'essayer le Query sur IHTMLSelectElement donc :
				goto suite;
			}
		}
		if (id2Type==COMBO && lenId2Name!=0)
		{
			hr=pIDispatch->QueryInterface(IID_IHTMLSelectElement,(void **)&pCombo);
			TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLSelectElement(%d)=0x%08lx",l,hr));
			// l'item r�pond � l'interface IHTMLSelectElement
			// on regarde si c'est le champ de saisie attendu
			if (SUCCEEDED(hr)) 
			{
				hr=pCombo->get_name(&bstr);
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pForm->get_name()=0x%08lx",hr)); goto end; }
				TRACE((TRACE_DEBUG,_F_,"pCombo->get_name='%S'",bstr));
		
				len=SysStringByteLen(bstr);
				if (len==lenId2Name && memcmp(bstr,bstrId2Name,lenId2Name)==0)
				{
					// c'est le champ id2, on le remplit
					TRACE((TRACE_INFO,_F_,"Combo '%S' trouv�",bstr));
					pCombo->put_value(bstrId2Value);
					TRACE((TRACE_INFO,_F_,"Valeur choisie : '%S'",bstrId2Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes : %d",ptSuivi->iNbActions));
				}
			}
		}
		if (id3Type==COMBO && lenId3Name!=0)
		{
			hr=pIDispatch->QueryInterface(IID_IHTMLSelectElement,(void **)&pCombo);
			TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLSelectElement(%d)=0x%08lx",l,hr));
			// l'item r�pond � l'interface IHTMLSelectElement
			// on regarde si c'est le champ de saisie attendu
			if (SUCCEEDED(hr)) 
			{
				hr=pCombo->get_name(&bstr);
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pForm->get_name()=0x%08lx",hr)); goto end; }
				TRACE((TRACE_DEBUG,_F_,"pCombo->get_name='%S'",bstr));
		
				len=SysStringByteLen(bstr);
				if (len==lenId3Name && memcmp(bstr,bstrId3Name,lenId3Name)==0)
				{
					// c'est le champ id3, on le remplit
					TRACE((TRACE_INFO,_F_,"Combo '%S' trouv�",bstr));
					pCombo->put_value(bstrId3Value);
					TRACE((TRACE_INFO,_F_,"Valeur choisie : '%S'",bstrId3Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes : %d",ptSuivi->iNbActions));
				}
			}
		}
		if (id4Type==COMBO && lenId4Name!=0)
		{
			hr=pIDispatch->QueryInterface(IID_IHTMLSelectElement,(void **)&pCombo);
			TRACE((TRACE_DEBUG,_F_,"pIDispatch->QueryInterface(IID_IHTMLSelectElement(%d)=0x%08lx",l,hr));
			// l'item r�pond � l'interface IHTMLSelectElement
			// on regarde si c'est le champ de saisie attendu
			if (SUCCEEDED(hr)) 
			{
				hr=pCombo->get_name(&bstr);
				if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pForm->get_name()=0x%08lx",hr)); goto end; }
				TRACE((TRACE_DEBUG,_F_,"pCombo->get_name='%S'",bstr));
		
				len=SysStringByteLen(bstr);
				if (len==lenId4Name && memcmp(bstr,bstrId4Name,lenId4Name)==0)
				{
					// c'est le champ id4, on le remplit
					TRACE((TRACE_INFO,_F_,"Combo '%S' trouv�",bstr));
					pCombo->put_value(bstrId4Value);
					TRACE((TRACE_INFO,_F_,"Valeur choisie : '%S'",bstrId4Value));
					(ptSuivi->iNbActions)--;
					TRACE((TRACE_INFO,_F_,"Actions restantes : %d",ptSuivi->iNbActions));
				}
			}
		}
suite:
		// tous les champs ont �t� remplis 
		if (ptSuivi->iNbActions<=0) 
		{
			TRACE((TRACE_DEBUG,_F_,"Tous les champs on �t� remplis (ptSuivi->iNbActions=%d), on s'occupe du formulaire",ptSuivi->iNbActions));
			TRACE((TRACE_DEBUG,_F_,"pTargetForm=0x%08lx",pTargetForm));
			TRACE((TRACE_DEBUG,_F_,"szFormName=%s",szFormName));
			if (pTargetForm!=NULL) // on a trouv� le formulaire, on le soumet et on sort
			{
				// 0.41 correction bug laposte.net, caisse d'�pargne...
				// il faut ex�cuter le onsubmit � la main, le submit ne le fait pas
				VARIANT onSubmit;
				hr=pTargetForm->get_onsubmit(&onSubmit);
				if (SUCCEEDED(hr) && onSubmit.pdispVal!=NULL)
				{
					// trouv� un onsubmit, on l'appelle
					TRACE((TRACE_DEBUG,_F_,"onSubmit.pdispVal=0x%08lx",onSubmit.pdispVal));
					DISPPARAMS params = {0};
					VARIANT varResult;
					VariantInit(&varResult);
					onSubmit.pdispVal->Invoke(DISPID_VALUE, IID_NULL,
						LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
						&params, &varResult, 0, 0);
				}
				TRACE((TRACE_INFO,_F_,"Soumission du formulaire pTargetForm=0x%08lx",pTargetForm));
				pTargetForm->submit();
				
				guiNbWEBSSO++;
				goto end;
			}
			else if (lenFormName==0) // formulaire non trouv� mais en fait on s'en fout car on ne le cherchait pas ;-) (formname=vide)
			{
				guiNbWEBSSO++;
				goto end;
			}
			else if(szFormName[0]=='[') // 0.89 on interpr�te le contenu du champ validation
			{
				Sleep(100);
				KBSimEx(NULL,szFormName,"","","","","");
				goto end;
			}
		}
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pItem!=NULL) {pItem->Release(); pItem = NULL;}
		if (pCombo!=NULL) {pCombo->Release(); pCombo = NULL;}
		if (pForm!=NULL) {pForm->Release(); pForm = NULL;}
		if (bstrForm!=NULL) { SysFreeString(bstrForm);bstrForm=NULL;}
	} // for(l=0;l<lNbElements;l++)
end:
	// Si formulaire non trouv�, on repositionne le nb d'actions � une valeur
	// bidon afin de faire �chouer le SSO vu du Main qui pourra le retenter plus tard
	// 0.41 : ouim�non pour les fenetres ou on veut juste remplir un champ sans valider
	// REMPLACEMENT de : if (pTargetForm==NULL) *piNbActions=2; PAR
	// + 0.80B7 nouveau tag [ENTER] pour simulation frappe clavier touche ENTREE

	if (pTargetForm==NULL && lenFormName!=0 && *szFormName!='[') 
		ptSuivi->iNbActions=2;

	if (bstr!=NULL) SysFreeString(bstr);
	if (bstrForm!=NULL) SysFreeString(bstrForm);
	if (pIDispatch!=NULL) pIDispatch->Release();
	if (pItem!=NULL) pItem->Release();
	if (pPwdField!=NULL) pPwdField->Release();
	if (pCombo!=NULL) pCombo->Release(); 
	if (pTargetForm!=NULL) { pTargetForm->Release(); pTargetForm=NULL; }
	if (pForm!=NULL) pForm->Release();
	if (pItems!=NULL) pItems->Release();

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ParseHTMLDoc2()
//-----------------------------------------------------------------------------
// R�cup�ration du document HTML contenu dans la fen�tre puis �num�ration
// de ses frames -> nouvelle version compatible iframe (0.90B1)
//-----------------------------------------------------------------------------
static void ParseHTMLDoc2(IHTMLDocument2 *pHTMLDocument2,LPARAM lp)
{
	TRACE((TRACE_ENTER,_F_, "lp=0x%08lx",lp));
	HRESULT hr;
	IOleContainer *pIOleContainer=NULL;
	IEnumUnknown *pIEnum=NULL;
	IUnknown *pIUnknown=NULL;
	IWebBrowser2 *pIWebBrowser2=NULL;
	IHTMLDocument2 *pFrame=NULL;
	IDispatch *pIDispatch=NULL;

	T_SUIVI_IE *ptSuivi=(T_SUIVI_IE*)lp;

 	// on commence par explorer directement le doc (comme avant)
	ParseFrame(pHTMLDocument2,lp);
	if (ptSuivi->iNbActions==0) goto end; // c'est bon, on a fini, on sort.

	// c'est ici que tout change dans la mani�re d'explorer le document
	// le code s'inspire de la pr�co Microsoft KB196340
	hr=pHTMLDocument2->QueryInterface(IID_IOleContainer,(void**)&pIOleContainer);
	if (FAILED(hr)) 
	{ 
		TRACE((TRACE_ERROR,_F_,"pHTMLDocument2->QueryInterface(IID_IOleContainer)=0x%08lx",hr));
		goto end;
	}
	hr=pIOleContainer->EnumObjects(OLECONTF_EMBEDDINGS,&pIEnum);
	if (FAILED(hr)) 
	{ 
		TRACE((TRACE_ERROR,_F_,"pIOleContainer->EnumObjects(OLECONTF_EMBEDDINGS)=0x%08lx",hr));
		goto end;
	}
	while (!FAILED(pIEnum->Next(1,&pIUnknown,NULL)))
	{
		TRACE((TRACE_DEBUG,_F_,"pIEnum->Next() OK, pIUnknown=0x%08lx",pIUnknown));
		if (pIUnknown==NULL) goto end; // �num�ration termin�e
		hr=pIUnknown->QueryInterface(IID_IWebBrowser2,(void**)&pIWebBrowser2);
		if (SUCCEEDED(hr)) 
		{
			hr=pIWebBrowser2->get_Document(&pIDispatch);
			if (SUCCEEDED(hr)) 
			{
				hr=pIDispatch->QueryInterface(IID_IHTMLDocument2,(void**)&pFrame);
				if (SUCCEEDED(hr))
				{
					ParseFrame(pFrame,lp);
					TRACE((TRACE_DEBUG,_F_,"ptSuivi->iNbActions=%d",ptSuivi->iNbActions));
					if (ptSuivi->iNbActions==0) goto end; // c'est bon, on a fini, on sort.
				}
				else
				{
					TRACE((TRACE_INFO,_F_,"pIDispatch->QueryInterface(IID_IHTMLDocument2)=0x%08lx",hr));
					// ca n'a pas march�, mais pas grave on continue � explorer le doc.
				}
			}
			else
			{
				TRACE((TRACE_INFO,_F_,"pIWebBrowser2->get_Document()=0x%08lx",hr));
				// ca n'a pas march�, mais pas grave on continue � explorer le doc.
			}
		}
		else 
		{
			TRACE((TRACE_INFO,_F_,"pIUnknown->QueryInterface(IID_IWebBrowser2)=0x%08lx",hr));
			// ca n'as pas march�, mais pas grave on continue � explorer le doc.
		}
		if (pIUnknown!=NULL) { pIUnknown->Release(); pIUnknown=NULL; }
		if (pIWebBrowser2!=NULL) { pIWebBrowser2->Release(); pIWebBrowser2=NULL; }
		if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
		if (pFrame!=NULL) { pFrame->Release(); pFrame=NULL; }
	}
end:
	if (pFrame!=NULL) pFrame->Release(); 
	if (pIWebBrowser2!=NULL) pIWebBrowser2->Release();
	if (pIUnknown!=NULL) pIUnknown->Release();
	if (pIEnum!=NULL) pIEnum->Release();
	if (pIOleContainer!=NULL) pIOleContainer->Release();
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// WebEnumChildProc()
//-----------------------------------------------------------------------------
// Nouvelle m�thode en 0.83 pour fonctionnement avec tabulations IE7
// qui ne fonctionnait plus correctement le 12/03/2009
// Plutot que de chercher le champ qui contient l'URL, on r�cup�re directement
// l'URL par appel � IHTMLDocument2->get_URL();
//-----------------------------------------------------------------------------
static int CALLBACK WebEnumChildProc(HWND w, LPARAM lp)
{
	char *pszURL=NULL;
	char szClassName[128+1];
	int rc=true; // true=continuer l'�num�ration
	DWORD dw;
	HRESULT hr;
	IHTMLDocument2 *pHTMLDocument2=NULL;
	BSTR bstrURL=NULL;

	GetClassName(w,szClassName,sizeof(szClassName));
	if (strcmp(szClassName,"Internet Explorer_Server")!=0) goto end;

	// r�cup�ration pointeur sur le document HTML (interface IHTMLDocument2)
	SendMessageTimeout(w,guiHTMLGetObjectMsg,0L,0L,SMTO_ABORTIFHUNG,1000,&dw);
	//hr=(*gpfObjectFromLresult)(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2); // 1.12B3-TI-TIE4
	hr=ObjectFromLresult(dw,IID_IHTMLDocument2,0,(void**)&pHTMLDocument2);
	if (FAILED(hr)) 
	{
		TRACE((TRACE_ERROR,_F_,"ObjectFromLresult(%d,IID_IHTMLDocument2)=0x%08lx",dw,hr));
		goto end;
	}
   	TRACE((TRACE_DEBUG,_F_,"ObjectFromLresult(IID_IHTMLDocument2)=%08lx pHTMLDocument2=%08lx",hr,pHTMLDocument2));

	ParseHTMLDoc2(pHTMLDocument2,lp);

	// ISSUE#312 : si la console debug F12 est ouverte, elle apparait en premier dans l'�num�ration des fen�tres.
	//             Il faut l'ignorer et continuer l'�num�ration
	hr=pHTMLDocument2->get_URL(&bstrURL);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_URL()=0x%08lx",hr)); goto end; }
	pszURL=GetSZFromBSTR(bstrURL);
	TRACE((TRACE_DEBUG,_F_,"get_URL()=%s",pszURL));
	if (_strnicmp(pszURL,"res://",6)==0)
	{
		TRACE((TRACE_DEBUG,_F_,"C'est la fenetre F12, on continue !"));
		goto end;
	}

	rc=false; // c'est fait, on arrete l'enum
end:
	if (pszURL!=NULL) free(pszURL);
	if (bstrURL!=NULL) SysFreeString(bstrURL);
	if (pHTMLDocument2!=NULL) pHTMLDocument2->Release();
	return rc;
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES 
//*****************************************************************************

//-----------------------------------------------------------------------------
// SSOWeb()
//-----------------------------------------------------------------------------
// SSO Web internet explorer
// [in] w=fen�tre qui contient la fen�tre de navigation
// [in] w2=fen�tre ppale
// avec IE: w=w2, avec Maxthon w!=w2
// ----------------------------------------------------------------------------------
// [out] 0=OK, -1=pas r�ussi (champs non trouv�s ou autre erreur), -2=pas la bonne URL
//-----------------------------------------------------------------------------
int SSOWeb(HWND w,int iAction,HWND w2)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d",w,iAction));

	T_SUIVI_IE tSuivi;
	tSuivi.w=w2;
	tSuivi.iNbActions=0;
	tSuivi.iAction=iAction;

	bstrFormName=NULL;
	bstrId1Name=NULL;
	bstrId1Value=NULL;
	bstrId2Name=NULL;
	bstrId2Value=NULL;
	bstrId3Name=NULL;
	bstrId3Value=NULL;
	bstrId4Name=NULL;
	bstrId4Value=NULL;
	bstrPwdName=NULL;
	bstrPwdValue=NULL;
	lenFormName=0;
	lenId1Name=0;
	lenId1Value=0;
	lenId2Name=0;
	lenId2Value=0;
	lenId3Name=0;
	lenId3Value=0;
	lenId4Name=0;
	lenId4Value=0;
	lenPwdName=0;
	lenPwdValue=0;
		
	int rc=-1;
	
	WCHAR wcTmp[255]; //0.80
	
	// recopie de l'URL : TODO pas beau
	strcpy_s(szURL,sizeof(szURL),gptActions[iAction].szURL);
	strcpy_s(szFormName,sizeof(szFormName),gptActions[iAction].szValidateName);

	// bstrisation du nom du formulaire
	bstrFormName=GetBSTRFromSZ(gptActions[iAction].szValidateName); if (bstrFormName==NULL) goto end;
	lenFormName= SysStringByteLen(bstrFormName);
	TRACE((TRACE_DEBUG,_F_,"bstrFormName='%S' lenFormName=%d",bstrFormName,lenFormName));

	if (*(gptActions[iAction].szId1Name)!=0) // un champ id1 � remplir
	{
		tSuivi.iNbActions++;
		// bstrisation du nom du champ id1 et valeur
		bstrId1Name=GetBSTRFromSZ(gptActions[iAction].szId1Name); if (bstrId1Name==NULL) goto end;
		lenId1Name= SysStringByteLen(bstrId1Name);
		if (gptActions[iAction].iType==WEBPWD)
		{
			bstrId1Value=GetBSTRFromSZ("aze"); if (bstrId1Value==NULL) goto end;
		}
		else
		{
			bstrId1Value=GetBSTRFromSZ(GetComputedValue(gptActions[iAction].szId1Value)); if (bstrId1Value==NULL) goto end;
		}
		lenId1Value=SysStringByteLen(bstrId1Value);
		TRACE((TRACE_DEBUG,_F_,"bstrId1Name=%S",bstrId1Name));
	}
	if (*(gptActions[iAction].szId2Name)!=0) // un champ id2 � remplir
	{
		tSuivi.iNbActions++;
		// bstrisation du nom du champ id2 et valeur
		bstrId2Name=GetBSTRFromSZ(gptActions[iAction].szId2Name); if (bstrId2Name==NULL) goto end;
		lenId2Name= SysStringByteLen(bstrId2Name);
		if (gptActions[iAction].iType==WEBPWD)
		{
			bstrId2Value=GetBSTRFromSZ("azeaze"); if (bstrId2Value==NULL) goto end;
		}
		else
		{
			bstrId2Value=GetBSTRFromSZ(gptActions[iAction].szId2Value); if (bstrId2Value==NULL) goto end;
		}
		lenId2Value=SysStringByteLen(bstrId2Value);
		id2Type=gptActions[iAction].id2Type;
		TRACE((TRACE_DEBUG,_F_,"bstrId2Name=%S",bstrId2Name));
	}
	if (*(gptActions[iAction].szId3Name)!=0) // un champ id3 � remplir
	{
		tSuivi.iNbActions++;
		// bstrisation du nom du champ id3 et valeur
		bstrId3Name=GetBSTRFromSZ(gptActions[iAction].szId3Name); if (bstrId3Name==NULL) goto end;
		lenId3Name= SysStringByteLen(bstrId3Name);
		if (gptActions[iAction].iType==WEBPWD)
		{
			bstrId3Value=GetBSTRFromSZ("azeaze"); if (bstrId3Value==NULL) goto end;
		}
		else
		{
			bstrId3Value=GetBSTRFromSZ(gptActions[iAction].szId3Value); if (bstrId3Value==NULL) goto end;
		}
		lenId3Value=SysStringByteLen(bstrId3Value);
		id3Type=gptActions[iAction].id3Type;
		TRACE((TRACE_DEBUG,_F_,"bstrId3Name=%S",bstrId3Name));
	}

	if (*(gptActions[iAction].szId4Name)!=0 && gptActions[iAction].id4Type!=CHECK_LABEL) // un champ id4 � remplir
	{
		tSuivi.iNbActions++;
		// bstrisation du nom du champ id4 et valeur
		bstrId4Name=GetBSTRFromSZ(gptActions[iAction].szId4Name); if (bstrId4Name==NULL) goto end;
		lenId4Name= SysStringByteLen(bstrId4Name);
		bstrId4Value=GetBSTRFromSZ(gptActions[iAction].szId4Value); if (bstrId4Value==NULL) goto end;
		lenId4Value=SysStringByteLen(bstrId4Value);
		id4Type=gptActions[iAction].id4Type;
		TRACE((TRACE_DEBUG,_F_,"bstrId4Name=%S",bstrId4Name));
	}
	if (*(gptActions[iAction].szPwdName)!=0) // un champ PWD � remplir
	{
		tSuivi.iNbActions++;
		// bstrisation du nom du champ PWD et valeur
		bstrPwdName=GetBSTRFromSZ(gptActions[iAction].szPwdName); if (bstrPwdName==NULL) goto end;
		lenPwdName= SysStringByteLen(bstrPwdName);

		TRACE((TRACE_DEBUG,_F_,"bstrPwdName=%S",bstrPwdName));
		
		if ((*gptActions[iAction].szPwdEncryptedValue!=0)) // TODO -> CODE A REVOIR PLUS TARD (PAS BEAU SUITE A ISSUE#83)
		{
			// char *pszPassword=swCryptDecryptString(gptActions[iAction].szPwdEncryptedValue,ghKey1);
			char *pszPassword=GetDecryptedPwd(gptActions[iAction].szPwdEncryptedValue,TRUE);
			if (pszPassword!=NULL) 
			{
				MultiByteToWideChar(CP_ACP,0,pszPassword,-1,wcTmp,sizeof(wcTmp));
				// 0.85B9 : remplacement de memset(pszPassword,0,strlen(pszPassword));
				SecureZeroMemory(pszPassword,strlen(pszPassword));
				free(pszPassword);
			}
		}
		else
		{
			MultiByteToWideChar(CP_ACP,0,gptActions[iAction].szPwdEncryptedValue,-1,wcTmp,sizeof(wcTmp));
		}
		bstrPwdValue=SysAllocString(wcTmp);
		if (bstrPwdValue==NULL) goto end;
		lenPwdValue=SysStringByteLen(bstrPwdValue);
	}

	// Enum�ration des fen�tres filles pour trouver :
	// 1) la barre d'adresse pour v�rifier l'URL -> non, d�j� fait dans le main.
	// 2) la zone de navigation pour trouver les champs � remplir
	EnumChildWindows(w,WebEnumChildProc,(LPARAM)&tSuivi);	
#if 0
	if (!bURLVerifiee) // on n'a pas trouv� l'URL
	{
		TRACE((TRACE_INFO,_F_,"URL NOK, SSO non fait"));
		rc=-2;
		goto end;
	}
#endif

	// si le nb d'actions � faire n'est pas tomb� � 0, c'est qu'on a pas trouv�
	// tout ce qu'on cherchait et que donc le SSO a tr�s probablement �chou�... donc on laisse le rc � -1
	if (tSuivi.iNbActions<=0) rc=0; //OK

end:
	if (bstrFormName!=NULL) { SysFreeString(bstrFormName); bstrFormName=NULL; }
	if (bstrId1Name!=NULL) 	{ SysFreeString(bstrId1Name);  bstrId1Name=NULL;}
	if (bstrId1Value!=NULL)	{ SysFreeString(bstrId1Value); bstrId1Value=NULL;}
	if (bstrId2Name!=NULL) 	{ SysFreeString(bstrId2Name);  bstrId2Name=NULL;}
	if (bstrId2Value!=NULL)	{ SysFreeString(bstrId2Value); bstrId2Value=NULL;}
	if (bstrId3Name!=NULL) 	{ SysFreeString(bstrId3Name);  bstrId3Name=NULL;}
	if (bstrId3Value!=NULL)	{ SysFreeString(bstrId3Value); bstrId3Value=NULL;}
	if (bstrId4Name!=NULL) 	{ SysFreeString(bstrId4Name);  bstrId4Name=NULL;}
	if (bstrId4Value!=NULL)	{ SysFreeString(bstrId4Value); bstrId4Value=NULL;}
	if (bstrPwdName!=NULL)	{ SysFreeString(bstrPwdName);  bstrPwdName=NULL;}
	if (bstrPwdValue!=NULL) { SysFreeString(bstrPwdValue); bstrPwdValue=NULL;}
	
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SSOMaxthon()
//-----------------------------------------------------------------------------
// R�alisation SSO avec Maxthon. Idem IE sauf que la fen�tre d�tect�e par 
// la boucle ppale du main n'est pas la bonne fen�tre puisque celle qui contient
// la zone de navigation n'est pas sa fille. Il faut donc retrouver la bonne
// m�re puis "simplement" appeler le SSO IE !
//-----------------------------------------------------------------------------
int SSOMaxthon(HWND w,int iAction)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d",w,iAction));

	int rc=-1;
	HWND wMaxthonView;
	
	wMaxthonView=FindWindow("Maxthon2_View",NULL);
	if (wMaxthonView==NULL)
	{
		TRACE((TRACE_ERROR,_F_,"Fen�tre de classe Maxthon2_View non trouv�e"));
		goto end;
	}
	rc=SSOWeb(wMaxthonView,iAction,w);
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
