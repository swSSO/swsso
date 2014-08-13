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
// swSSOAD.cpp
//-----------------------------------------------------------------------------
// Tout ce qui a un rapport avec l'AD...
//-----------------------------------------------------------------------------

#include "stdafx.h"
static int giRefreshTimer=10;

//-----------------------------------------------------------------------------
// GetUserDomainAndComputer() 
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetUserDomainAndComputer(void)
{
	TRACE((TRACE_ENTER,_F_,""));

	DWORD cbRDN,cbSid;
	SID_NAME_USE eUse;
	DWORD lenComputerName;
	DWORD lenUserName;
	int rc=-1;

	// ComputerName
	lenComputerName=sizeof(gszComputerName); 
	if (!GetComputerName(gszComputerName,&lenComputerName))
	{
		TRACE((TRACE_ERROR,_F_,"GetComputerName(%d)",GetLastError())); goto end;
	}

	// UserName
	lenUserName=sizeof(gszUserName); 
	if (!GetUserName(gszUserName,&lenUserName))
	{
		TRACE((TRACE_ERROR,_F_,"GetUserName(%d)",GetLastError())); goto end;
	}

	// détermine le SID de l'utilisateur courant et récupère le nom de domaine
	cbSid=0;
	cbRDN=0;
	LookupAccountName(NULL,gszUserName,NULL,&cbSid,NULL,&cbRDN,&eUse); // pas de test d'erreur, car la fonction échoue forcément
	if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[1](%s)=%d",gszUserName,GetLastError()));
		goto end;
	}
	gpSid=(SID *)malloc(cbSid); if (gpSid==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbSid)); goto end; }
	gpszRDN=(char *)malloc(cbRDN); if (gpszRDN==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbRDN)); goto end; }
	if(!LookupAccountName(NULL,gszUserName,gpSid,&cbSid,gpszRDN,&cbRDN,&eUse))
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[2](%s)=%d",gszUserName,GetLastError()));
		goto end;
	}
	TRACE((TRACE_INFO,_F_,"LookupAccountName(%s) pszRDN=%s",gszUserName,gpszRDN));
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;

}

//-----------------------------------------------------------------------------
// GetLastADPwdChange()
//-----------------------------------------------------------------------------
// Récupère la date de dernier changement de mot de passe dans l'AD
//-----------------------------------------------------------------------------
int GetLastADPwdChange(char *pszLastADPwdChange)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	HRESULT hr;
	IADsUser *pIAdsUser=NULL;
	IADsADSystemInfo *pIADsADSystemInfo=NULL;
	char *pszUserDN=NULL;
	BSTR bstrUserDN=NULL;
	char *pszUserRequest=NULL;
	BSTR bstrUserRequest=NULL;
	int sizeUserRequest;
	
	DATE dateLastChange;
	SYSTEMTIME stLastChange;

	// récupération du DN de l'utilisateur
	hr=CoCreateInstance(CLSID_ADSystemInfo,NULL,CLSCTX_INPROC_SERVER,IID_IADsADSystemInfo,(void**)&pIADsADSystemInfo);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IADsADSystemInfo) hr=0x%08lx",hr)); goto end; }
	hr=pIADsADSystemInfo->get_UserName(&bstrUserDN);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pIADsADSystemInfo->get_UserName() hr=0x%08lx",hr)); goto end; }
	pszUserDN=GetSZFromBSTR(bstrUserDN); if (pszUserDN==NULL) goto end;
	TRACE((TRACE_ERROR,_F_,"pIADsADSystemInfo->get_UserName()=%s",pszUserDN));
	
	// récupération de l'objet User dans l'AD
	sizeUserRequest=10+strlen(pszUserDN);
	pszUserRequest=(char*)malloc(sizeUserRequest); 
	if (pszUserRequest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeUserRequest)); }
	sprintf_s(pszUserRequest,sizeUserRequest,"LDAP://%s",pszUserDN);
	bstrUserRequest=GetBSTRFromSZ(pszUserRequest); if (bstrUserRequest==NULL) goto end;
	hr=ADsGetObject(bstrUserRequest,IID_IADsUser,(LPVOID*)&pIAdsUser);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"ADsGetObject(%S,IID_IADsUser) hr=0x%08lx\n",bstrUserRequest,hr)); goto end; }
	
	// lecture de la date de dernier changement de mot de passe
	hr=pIAdsUser->get_PasswordLastChanged(&dateLastChange);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pUser->get_PasswordLastChanged() hr=0x%08lx\n",hr)); goto end; }

	// conversion pour stockage et comparaison format AAAAMMJJHHMMSS ex:20140730182514
	VariantTimeToSystemTime(dateLastChange, &stLastChange);
	sprintf_s(pszLastADPwdChange,14,"%04d%02d%02d%02d%02d%02d",
		(int)stLastChange.wYear,(int)stLastChange.wMonth,(int)stLastChange.wDay,
		(int)stLastChange.wHour,(int)stLastChange.wMinute,(int)stLastChange.wSecond);
	TRACE((TRACE_INFO,_F_,"LastAdPwdChange=%s",pszLastADPwdChange));

	rc=0;
end:
	// BOUCHON TEST **************************************************
	rc=0;
	strcpy_s(pszLastADPwdChange,14+1,"20140501120000");
	// BOUCHON TEST **************************************************
	if (bstrUserDN!=NULL) SysFreeString(bstrUserDN);
	if (bstrUserRequest!=NULL) SysFreeString(bstrUserRequest);
	if (pszUserDN!=NULL) free(pszUserDN);
	if (pszUserRequest!=NULL) free(pszUserRequest);
	if (pIADsADSystemInfo!=NULL) pIADsADSystemInfo->Release();
	if (pIAdsUser!=NULL) pIAdsUser->Release();
	
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// AskADPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fenêtre de saisie du mot de passe AD
//-----------------------------------------------------------------------------
static int CALLBACK AskADPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);

	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			MACRO_SET_SEPARATOR;
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
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
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					char szNewPwd1[LEN_PWD+1];
					char *gpszEncryptedADPwd=NULL;
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					// chiffrement du mot de passe AD
					gpszEncryptedADPwd=swCryptEncryptString(szNewPwd1,ghKey1);
					if (gpszEncryptedADPwd!=NULL)
					{
						strcpy_s(gszEncryptedADPwd,sizeof(gszEncryptedADPwd),gpszEncryptedADPwd);
						free(gpszEncryptedADPwd);
					}
					SecureZeroMemory(szNewPwd1,strlen(szNewPwd1));
					// Remarque : l'enregistrement dans le .ini en retour de la fonction avec SaveConfigHeader()
					EndDialog(w,IDOK);
					break;
				}
				case IDCANCEL:
					// demande à l'utilisateur de confirmer qu'il veut vraiment annuler, car s'il ne fournit pas son
					// nouveau mdp AD, le SSO sur les applications utilisant ce mdp ne fonctionnera pas !
					if (MessageBox(NULL,GetString(IDS_WARNING_AD_PWD),"swSSO",MB_YESNO | MB_ICONEXCLAMATION)==IDYES) EndDialog(w,IDCANCEL);
					break;
				case TB_NEW_PWD1:
				case TB_NEW_PWD2:
				{
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					int lenNewPwd1,lenNewPwd2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						BOOL bOK;
						lenNewPwd1=GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
						lenNewPwd2=GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
						bOK=FALSE;
						if (lenNewPwd1!=0 && lenNewPwd1==lenNewPwd2)
						{
							bOK=((strcmp(szNewPwd1,szNewPwd2)==0) ? TRUE : FALSE);
						}
						EnableWindow(GetDlgItem(w,IDOK),bOK);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
	return rc;
}


//-----------------------------------------------------------------------------
// AskADPwd()
//-----------------------------------------------------------------------------
// Demande à l'utilisateur de saisir son mot de passe AD
//-----------------------------------------------------------------------------
int AskADPwd(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_ASK_AD_PWD),NULL,AskADPwdDialogProc)==IDOK) rc=0;

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// CheckADPwdChange()
//-----------------------------------------------------------------------------
// Vérifie la date de dernier changement du mot de passe AD et si nécessaire
// le demande à l'utilisateur pour le stocker dans le .ini
//-----------------------------------------------------------------------------
int CheckADPwdChange(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szLastADPwdChange[14+1];
	BOOL bAskADPwd=FALSE;
	
	// récupère la date de dernier changement dans l'AD
	if (GetLastADPwdChange(szLastADPwdChange)!=0) goto end;

	if (*gszLastADPwdChange==0) // pas de date de changement de mdp dans le .ini, il faut demander le mdp AD
	{
		TRACE((TRACE_INFO,_F_,"Pas lastADPwdChange dans le .ini, demande le mdp à l'utilisateur"));
		bAskADPwd=TRUE;
	}
	else // date de changement de mdp dans le .ini, compare avec la date de dernier changement dans l'AD
	{
		TRACE((TRACE_DEBUG,_F_,"lastADPwdChange dans le .ini : %s",gszLastADPwdChange));
		TRACE((TRACE_DEBUG,_F_,"lastADPwdChange dans l'AD    : %s",szLastADPwdChange));
		if (strcmp(gszLastADPwdChange,szLastADPwdChange)<0) 
		{
			TRACE((TRACE_INFO,_F_,"Mot de passe changé dans l'AD, demande le mdp à l'utilisateur"));
			bAskADPwd=TRUE;// date de changement .ini antérieure à date changement AD 
		}
	}
	if (bAskADPwd) // demande le mot de passe AD à l'utilisateur et le stocke dans le .ini
	{
		if (AskADPwd()!=0) goto end;
		strcpy_s(gszLastADPwdChange,sizeof(gszLastADPwdChange),szLastADPwdChange);
		SaveConfigHeader();
	}

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetDecryptedPwd()
//-----------------------------------------------------------------------------
// Déchiffre le mot de passe. Si valeur=%ADPASSWORD%, renvoie le mdp AD déchiffré
//-----------------------------------------------------------------------------
char *GetDecryptedPwd(char *szPwdEncryptedValue)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *ret=NULL;
	char *pszPassword=NULL;
	char *pszADPassword=NULL;
	
	pszPassword=swCryptDecryptString(szPwdEncryptedValue,ghKey1);
	if (pszPassword!=NULL) 
	{
		if (strcmp(pszPassword,"%ADPASSWORD%")==0)
		{
			TRACE((TRACE_DEBUG,_F_,"%%ADPASSWORD%%"));
			pszADPassword=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
			free(pszPassword); pszPassword=NULL;
			ret=pszADPassword;
		}
		else
		{
			ret=pszPassword;
		}
	}
	
	TRACE((TRACE_LEAVE,_F_, ""));
	return ret;
}

