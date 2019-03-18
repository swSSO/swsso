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

typedef struct
{
	char c;
	BOOL bAlt;
	WORD w1;
	WORD w2;
	WORD w3;
	char c1;
	char c2;
} T_ASCII_TO_ALTCODE;

T_ASCII_TO_ALTCODE gTableAsciiToAltcode[]={
	// a/A
	{'ä',FALSE,0,0,0,'¨','a'},	
	{'â',FALSE,0,0,0,'^','a'},	
	{'ã',FALSE,0,0,0,'~','a'}, 
	{'Ä',FALSE,0,0,0,'¨','A'},	
	{'Â',FALSE,0,0,0,'^','A'},	
	{'Ã',FALSE,0,0,0,'~','A'},
	// e/E
	{'ë',FALSE,0,0,0,'¨','e'},	
	{'ê',FALSE,0,0,0,'^','e'},	
	{'Ë',FALSE,0,0,0,'¨','E'},	
	{'Ê',FALSE,0,0,0,'^','E'},	
	{'È',FALSE,0,0,0,'`','E'},	
	// i/I
	{'ï',FALSE,0,0,0,'¨','i'},	
	{'î',FALSE,0,0,0,'^','i'},	
	{'ì',FALSE,0,0,0,'`','i'},	
	{'Ï',FALSE,0,0,0,'¨','I'},	
	{'Î',FALSE,0,0,0,'^','I'},	
	{'Ì',FALSE,0,0,0,'`','I'},	
	// o/O
	{'ö',FALSE,0,0,0,'¨','o'},	
	{'ô',FALSE,0,0,0,'^','o'},	
	{'ò',FALSE,0,0,0,'`','o'},	
	{'õ',FALSE,0,0,0,'~','o'},	
	{'Ö',FALSE,0,0,0,'¨','O'},	
	{'Ô',FALSE,0,0,0,'^','O'},	
	{'Ò',FALSE,0,0,0,'`','O'},	
	{'Õ',FALSE,0,0,0,'~','O'},	
	// u/U
	{'ü',FALSE,0,0,0,'¨','u'},	
	{'û',FALSE,0,0,0,'^','u'},	
	{'Ü',FALSE,0,0,0,'¨','U'},	
	{'Û',FALSE,0,0,0,'^','U'},	
	// y/Y
	{'ÿ',FALSE,0,0,0,'¨','y'},	
	// n/N
	{'ñ',FALSE,0,0,0,'~','n'},	
	{'Ñ',FALSE,0,0,0,'~','N'},	
	// autres
	{'å',TRUE,VK_NUMPAD1,VK_NUMPAD3,VK_NUMPAD4,0,0},
	{'Å',TRUE,VK_NUMPAD1,VK_NUMPAD4,VK_NUMPAD3,0,0},
	{'É',TRUE,VK_NUMPAD1,VK_NUMPAD4,VK_NUMPAD4,0,0},
	{'æ',TRUE,VK_NUMPAD1,VK_NUMPAD4,VK_NUMPAD5,0,0},
	{'Æ',TRUE,VK_NUMPAD1,VK_NUMPAD4,VK_NUMPAD6,0,0},
	{'ƒ',TRUE,VK_NUMPAD1,VK_NUMPAD5,VK_NUMPAD9,0,0},
	{'á',TRUE,VK_NUMPAD1,VK_NUMPAD6,VK_NUMPAD0,0,0},
	{'í',TRUE,VK_NUMPAD1,VK_NUMPAD6,VK_NUMPAD1,0,0},
	{'ó',TRUE,VK_NUMPAD1,VK_NUMPAD6,VK_NUMPAD2,0,0},
	{'ú',TRUE,VK_NUMPAD1,VK_NUMPAD6,VK_NUMPAD3,0,0},
	{'¿',TRUE,VK_NUMPAD1,VK_NUMPAD6,VK_NUMPAD8,0,0},
	{'¡',TRUE,VK_NUMPAD1,VK_NUMPAD7,VK_NUMPAD3,0,0},
	{'«',TRUE,VK_NUMPAD1,VK_NUMPAD7,VK_NUMPAD4,0,0},
	{'»',TRUE,VK_NUMPAD1,VK_NUMPAD7,VK_NUMPAD5,0,0}
};

// ----------------------------------------------------------------------------------
// SendKey
// ----------------------------------------------------------------------------------
void SendKey(HWND w,BOOL bCapsLock,char c)
{
	TRACE((TRACE_ENTER,_F_, "%c",c));
	BYTE hiVk,loVk;
	WORD wKeyScan;

	wKeyScan=VkKeyScan(c);
	hiVk=HIBYTE(wKeyScan);
	loVk=LOBYTE(wKeyScan);

	// ISSUE#336 : si caps lock, on fait shift à l'envers puisque la désactivation de caps lock ne fonctionne pas (en citrix)
	// Attention, il ne faut pas le faire si la touche control est aussi enfoncée sinon ça fout la zone (saisie de @ par exemple)
	if (bCapsLock & !(hiVk & 2))
	{
		if (!(hiVk & 1)) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);
	}
	else
	{
		if (hiVk & 1) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);
	}
	if (hiVk & 2) { keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0); }
	if (hiVk & 4) { keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),0,0);  } 

	if (w!=NULL) SetForegroundWindow(w); // ISSUE#285 : remet la fenêtre au 1er plan avant chaque frappe
	keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),0,0);
	keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),KEYEVENTF_KEYUP,0);
		
	// ISSUE#336 : si caps lock, on fait shift à l'envers puisque la désactivation de caps lock ne fonctionne pas (en citrix)
	// Attention, il ne faut pas le faire si la touche control est aussi enfoncée sinon ça fout la zone (saisie de @ par exemple)
	if (bCapsLock & !(hiVk & 2))
	{
		if (!(hiVk & 1)) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0);
	}
	else
	{
		if (hiVk & 1) keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0);
	}
	// ISSUE#163 : inversion des lignes pour CONTROL et MENU il relacher les touches dans le même sens sinon la touche ALT
	//             reste enfoncée (uniquement constaté dans IE9, reproduit nulle par ailleurs)
	if (hiVk & 2) { keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0); } 
	if (hiVk & 4) { keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),KEYEVENTF_KEYUP,0); } 
	TRACE((TRACE_LEAVE,_F_, ""));
}	
	
// ----------------------------------------------------------------------------------
// SendDiatric
// ----------------------------------------------------------------------------------
// Traitement des caractères à saisir en 2 touches ou par Alt+num
// Liste / jeu de test : äâãÄÂÃëêËÊÈïîìÏÎÌöôòõÖÔÒÕüûÜÛÿñÑåÅÉæÆƒáíóú¿¡«»
// ----------------------------------------------------------------------------------
void SendDiatric(HWND w,BOOL bCapsLock,char c)
{
	TRACE((TRACE_ENTER,_F_, "%c",c));
	int i;
	INPUT inputKey[8];
	memset(&inputKey,0,sizeof(inputKey));
	
	for (i=0;i<sizeof(gTableAsciiToAltcode)/sizeof(T_ASCII_TO_ALTCODE);i++)
	{
		if (gTableAsciiToAltcode[i].c==c) break;
	}
	if (i==sizeof(gTableAsciiToAltcode)/sizeof(T_ASCII_TO_ALTCODE)) { TRACE((TRACE_ERROR,_F_,"Altcode du caractere %c non trouvé",c)); goto end; }

	if (gTableAsciiToAltcode[i].bAlt) // ALT+3 chiffres
	{
		inputKey[0].type=INPUT_KEYBOARD;
		inputKey[0].ki.wVk=VK_MENU;
		inputKey[0].ki.wScan=LOBYTE(MapVirtualKey(inputKey[0].ki.wVk, 0));
		
		inputKey[1].type=INPUT_KEYBOARD;
		inputKey[1].ki.wVk=gTableAsciiToAltcode[i].w1;
		inputKey[1].ki.wScan=LOBYTE(MapVirtualKey(inputKey[1].ki.wVk, 0));
		inputKey[2].type=INPUT_KEYBOARD;
		inputKey[2].ki.wVk=gTableAsciiToAltcode[i].w1;
		inputKey[2].ki.wScan=LOBYTE(MapVirtualKey(inputKey[2].ki.wVk, 0));
		inputKey[2].ki.dwFlags=KEYEVENTF_KEYUP;
		
		inputKey[3].type=INPUT_KEYBOARD;
		inputKey[3].ki.wVk=gTableAsciiToAltcode[i].w2;
		inputKey[3].ki.wScan=LOBYTE(MapVirtualKey(inputKey[3].ki.wVk, 0));
		inputKey[4].type=INPUT_KEYBOARD;
		inputKey[4].ki.wVk=gTableAsciiToAltcode[i].w2;
		inputKey[4].ki.wScan=LOBYTE(MapVirtualKey(inputKey[4].ki.wVk, 0));
		inputKey[4].ki.dwFlags=KEYEVENTF_KEYUP;

		inputKey[5].type=INPUT_KEYBOARD;
		inputKey[5].ki.wVk=gTableAsciiToAltcode[i].w3;
		inputKey[5].ki.wScan=LOBYTE(MapVirtualKey(inputKey[5].ki.wVk, 0));
		inputKey[5].type=INPUT_KEYBOARD;
		inputKey[6].ki.wVk=gTableAsciiToAltcode[i].w3;
		inputKey[6].ki.wScan=LOBYTE(MapVirtualKey(inputKey[6].ki.wVk, 0));
		inputKey[6].ki.dwFlags=KEYEVENTF_KEYUP;

		inputKey[7].type=INPUT_KEYBOARD;
		inputKey[7].ki.wVk=VK_MENU;
		inputKey[7].ki.wScan=LOBYTE(MapVirtualKey(inputKey[7].ki.wVk, 0));
		inputKey[7].ki.dwFlags=KEYEVENTF_KEYUP;
		SendInput(8,inputKey,sizeof(INPUT));
	}
	else // saisie de la séquence de caractères ^ ou ¨ ou ~ + lettre
	{
		SendKey(w,bCapsLock,gTableAsciiToAltcode[i].c1);
		SendKey(NULL,bCapsLock,gTableAsciiToAltcode[i].c2);
	}
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

// ----------------------------------------------------------------------------------
// Simulation frappe clavier
// ----------------------------------------------------------------------------------
// [in] bErase : TRUE s'il faut vider le contenu du champ avant de le remplir
// [in] sz : chaine à saisir
// ----------------------------------------------------------------------------------
void KBSim(HWND w,BOOL bErase,int iTempo,const char *sz,BOOL bPwd)
{
	UNREFERENCED_PARAMETER(bPwd); // pour ne pas avoir de warning en mode release
	TRACE((TRACE_ENTER,_F_, "bErase=%d iTempo=%d",bErase,iTempo));
	//TRACE((bPwd?TRACE_PWD:TRACE_INFO,_F_, "sz='%s'",sz));

	int i,len;
	len=strlen(sz);
	BYTE loVk;
	WORD wKeyScan;
	BOOL bCapsLock=FALSE;

	// en 1.09, déplacement du control du caps lock tout au début
	TRACE((TRACE_DEBUG,_F_,"GetKeyState(VK_CAPITAL)=%04x",GetKeyState(VK_CAPITAL)));
	if (LOBYTE(GetKeyState(VK_CAPITAL))==1) // 0.75 : caps lock
	{
		bCapsLock=TRUE;
	}
	
	// ISSUE#264 : changement de la technique d'effacement, on fait CTRL+A puis DEL, ça évite les changements de champs.
	if (bErase) // ISSUE#286 : refait comme avant, n'efface pas systématiquement sinon la config type "simulation de frappe" ne fonctionne plus !
	{
		Sleep(iTempo);
		if (w!=NULL) SetForegroundWindow(w); // ISSUE#285 : remet la fenêtre au 1er plan avant chaque frappe
		keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0);
		wKeyScan=VkKeyScan('a');
		loVk=LOBYTE(wKeyScan);
		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),0,0);
		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),KEYEVENTF_KEYUP,0);
		keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0);
		keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),0,0);
		keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),KEYEVENTF_KEYUP,0);
	}
	for (i=0;i<len;i++)
	{
		wKeyScan=VkKeyScan(sz[i]);
		if (wKeyScan==0xffff) // cas des trémas, tilde et accents circonflexes -- ISSUE#388
		{
			SendDiatric(w,bCapsLock,sz[i]);
		}
		else
		{
			SendKey(w,bCapsLock,sz[i]);
		}
	}
	Sleep(iTempo);

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// CheckIfURLStillOK() 
//-----------------------------------------------------------------------------
// ISSUE#313 : pour Chrome et Firefox, permet de vérifier qu'on est toujours
// sur le bon onglet avant de saisir les identifiants
//-----------------------------------------------------------------------------
BOOL CheckIfURLStillOK(HWND w,int iAction,int iBrowser,IAccessible *pInAccessible,BOOL bGetAccessible,IAccessible **ppOutAccessible)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx iAction=%d iBrowser",w,iAction,iBrowser));
	BOOL rc=TRUE;
	char *pszURL=NULL;
	
	if (iBrowser==BROWSER_CHROME)
	{
		pszURL=NewGetChromeURL(w,pInAccessible,bGetAccessible,ppOutAccessible);
	}
	else if ((iBrowser==BROWSER_FIREFOX3) || (iBrowser==BROWSER_FIREFOX4))
	{
		pszURL=GetFirefoxURL(w,pInAccessible,bGetAccessible,ppOutAccessible,iBrowser,FALSE);
	}
	if (pszURL!=NULL)
	{
		if (!swURLMatch(pszURL,gptActions[iAction].szURL))
		{
			TRACE((TRACE_INFO,_F_,"URL ne matche plus... changement d'onglet ?"));
			//MessageBox(w,"Changement d'onglet !","",MB_OK);
			rc=FALSE;
		}
		free(pszURL);
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// KBSimWeb
// ----------------------------------------------------------------------------------
int KBSimWeb(HWND w,BOOL bErase,int iTempo,const char *sz,BOOL bPwd,int iAction,int iBrowser,IAccessible *pTextField,VARIANT vtChild)
{
	UNREFERENCED_PARAMETER(bPwd); 
	TRACE((TRACE_ENTER,_F_, "bErase=%d iTempo=%d",bErase,iTempo));

	int i,len;
	len=strlen(sz);
	BYTE loVk;
	WORD wKeyScan;
	BOOL bCapsLock=FALSE;
	int rc=-1;
	IAccessible *pAccessible=NULL;

	// en 1.09, déplacement du control du caps lock tout au début
	TRACE((TRACE_DEBUG,_F_,"GetKeyState(VK_CAPITAL)=%04x",GetKeyState(VK_CAPITAL)));
	if (LOBYTE(GetKeyState(VK_CAPITAL))==1) // 0.75 : caps lock
	{
		bCapsLock=TRUE;
	}
	// ISSUE#264 : changement de la technique d'effacement, on fait CTRL+A puis DEL, ça évite les changements de champs.
	if (bErase) // ISSUE#286 : refait comme avant, n'efface pas systématiquement sinon la config type "simulation de frappe" ne fonctionne plus !
	{
		Sleep(iTempo);
		if (!CheckIfURLStillOK(w,iAction,iBrowser,pAccessible,(pAccessible==NULL),&pAccessible)) goto end;
		if (w!=NULL) SetForegroundWindow(w); // ISSUE#285 : remet la fenêtre au 1er plan avant chaque frappe
		keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0);
		wKeyScan=VkKeyScan('a');
		loVk=LOBYTE(wKeyScan);
		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),0,0);
		keybd_event(loVk,LOBYTE(MapVirtualKey(loVk,0)),KEYEVENTF_KEYUP,0);
		keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0);
		keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),0,0);
		keybd_event(VK_DELETE,LOBYTE(MapVirtualKey(VK_DELETE,0)),KEYEVENTF_KEYUP,0);
	}
	for (i=0;i<len;i++)
	{
		if (i%4==0) { if (!CheckIfURLStillOK(w,iAction,iBrowser,pAccessible,(pAccessible==NULL),&pAccessible)) goto end; }
		// if (bPwd && ptSuivi!=NULL) ptSuivi->pTextFields[ptSuivi->iPwdIndex]->accSelect(SELFLAG_TAKEFOCUS,vtChild);
		// ISSUE#353 : applique la méthode à tous les champs (pas seulement le mot de passe)
		if (w!=NULL) SetForegroundWindow(w); // ISSUE#285 : remet la fenêtre au 1er plan avant chaque frappe
		if (iBrowser==BROWSER_CHROME)
			ChromeAccSelect(w,pTextField);
		else
			pTextField->accSelect(SELFLAG_TAKEFOCUS,vtChild);
		
		wKeyScan=VkKeyScan(sz[i]);
		if (wKeyScan==0xffff) // cas des trémas, tilde et accents circonflexes -- ISSUE#388
		{
			SendDiatric(w,bCapsLock,sz[i]);
		}
		else
		{
			SendKey(w,bCapsLock,sz[i]);
		}
	}
	Sleep(iTempo);
	rc=0;
end:
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
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
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx pAccessible=0x%08lx szValue=%s",w,pAccessible,szValue));
	
	HRESULT hr;
	BSTR bstrValue=NULL;
	
	SetForegroundWindow(w);
	hr=pAccessible->accSelect(SELFLAG_TAKEFOCUS,index);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->accSelect(%d) : hr=0x%08lx",index.lVal,hr));

	// 1.09B2 : tente de faire put_accValue : si non implémenté, retour à la simulation de frappe clavier
	bstrValue=GetBSTRFromSZ(GetComputedValue(szValue));
	hr=S_OK;
	if (bstrValue!=NULL)
	{
		hr=pAccessible->put_accValue(index,bstrValue);
		TRACE((TRACE_INFO,_F_,"pAccessible->put_accValue() : hr=0x%08lx",hr));
	}
	if (bstrValue==NULL || FAILED(hr))
	{
		KBSim(w,TRUE,100,GetComputedValue(szValue),FALSE); // 1.09B1 : bErase à TRUE toujours
	}
	if (bstrValue!=NULL) SysFreeString(bstrValue);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// PutAccValueWeb()
//-----------------------------------------------------------------------------
// Remplissage d'un champ par simulation de frappe clavier puisque personne
// ne veut implémenter put_AccValue !!!
// utilisé pour tous les champs sauf mot de passe
//-----------------------------------------------------------------------------
int PutAccValueWeb(HWND w,IAccessible *pAccessible,VARIANT index,const char *szValue,int iAction,int iBrowser)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx pAccessible=0x%08lx szValue=%s",w,pAccessible,szValue));
	
	HRESULT hr;
	BSTR bstrValue=NULL;
	int rc=-1;
	VARIANT vtNone;

	vtNone.vt=VT_I4;
	vtNone.lVal=0;

	SetForegroundWindow(w);
	hr=pAccessible->accSelect(SELFLAG_TAKEFOCUS,index);
	TRACE((TRACE_DEBUG,_F_,"pAccessible->accSelect(%d) : hr=0x%08lx",index.lVal,hr));

	// 1.09B2 : tente de faire put_accValue : si non implémenté, retour à la simulation de frappe clavier
	bstrValue=GetBSTRFromSZ(GetComputedValue(szValue));
	hr=S_OK;
	if (bstrValue!=NULL)
	{
		hr=pAccessible->put_accValue(index,bstrValue);
		TRACE((TRACE_INFO,_F_,"pAccessible->put_accValue() : hr=0x%08lx",hr));
	}
	if (bstrValue==NULL || FAILED(hr))
	{
		if (KBSimWeb(w,TRUE,100,GetComputedValue(szValue),FALSE,iAction,iBrowser,pAccessible,vtNone)!=0) goto end; // 1.09B1 : bErase à TRUE toujours
	}
	rc=0;
end:
	if (bstrValue!=NULL) SysFreeString(bstrValue);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
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
// GetFirefox51PopupURL() 
// ----------------------------------------------------------------------------------
// ISSUE#303 : avec FF51, l'ancienne méthode GetFirefoxPopupURL() ne marche plus
// par contre on peut trouver la même chaine dans la description de la fenêtre ! 
// Beaucoup plus simple et marche déjà avec la version actuelle FF49. 
// Pour assurer la compatibilité avec de plus vieilles versions de FF, il vaut mieux
// appeler cet méthode APRES avoir tenté avec GetFirefoxPopupURL()
// ----------------------------------------------------------------------------------
// [rc] pointeur vers la chaine, à libérer par l'appelant. NULL si erreur
// ----------------------------------------------------------------------------------
char *GetFirefox51PopupURL(HWND w) 
{
	TRACE((TRACE_ENTER,_F_, ""));
	HRESULT hr;
	VARIANT vtSelf;
	IAccessible *pAccessible=NULL;
	BSTR bstrName=NULL;
	char *pszURL=NULL;

	pAccessible=GetFirefoxPopupIAccessible(w);
	if (pAccessible==NULL) 
	{
		TRACE((TRACE_ERROR,_F_,"Impossible de trouver un pointeur iAccessible sur cette popup"));
		goto end;
	}

	vtSelf.vt=VT_I4;
	vtSelf.lVal=CHILDID_SELF;
	hr=pAccessible->get_accDescription(vtSelf,&bstrName);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"get_accDescription()=0x%08lx",hr)); goto end; }
	pszURL=GetSZFromBSTR(bstrName);
	if (pszURL==NULL) goto end;
	TRACE((TRACE_DEBUG,_F_,"pszURL='%s'",pszURL));

end:
	SysFreeString(bstrName);
	if (pAccessible!=NULL) pAccessible->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
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
//
// ISSUE#303 : Avec FF51, tout ça ne marche plus, impossible d'énumérer les fils... 
// par contre on peut trouver la même chaine dans la description de la fenêtre ! 
// Voir nouvelle méthode GetFirefox51PopupURL()
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
	// ISSUE#303 : si ça n'a pas marché, on appelle la nouvelle fonction GetFirefox51PopupURL()
	if (pszURL==NULL) pszURL=GetFirefox51PopupURL(w);

	if (pIDispatch!=NULL) { pIDispatch->Release(); pIDispatch=NULL; }
	if (pChild2!=NULL) { pChild2->Release(); pChild2=NULL; }
	if (pChild!=NULL) { pChild->Release(); pChild=NULL; }

	SysFreeString(bstrName);
	if (pAccessible!=NULL) pAccessible->Release();
	if (pAccessibleFF4!=NULL) pAccessibleFF4->Release();
	TRACE((TRACE_LEAVE,_F_,"pszURL=0x%08lx",pszURL));
	return pszURL;
}
