//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2025 - Sylvain WERDEFROY
//
//
//                   
//                       sylvain.werdefroy@gmail.com
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
static BOOL gbWarningIfNoADPwd=FALSE;

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
	DWORD lenUPN;
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
	// UPN
	lenUPN=sizeof(gszUPN); 
	if (!GetUserNameEx(NameUserPrincipal,gszUPN,&lenUPN))
	{
		TRACE((TRACE_INFO,_F_,"GetUserNameEx(NameUserPrincipal)=%d",GetLastError())); 
		*gszUPN=0;
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
// GetLastADPwdChange2()
//-----------------------------------------------------------------------------
// Récupère la date de dernier changement de mot de passe dans l'AD
// Nouvelle méthode suite ISSUE#281 : récupère directement la valeur de l'attribut 
// pwdLastSet pour ne pas subir les pb de timezone et changements d'heure
//-----------------------------------------------------------------------------
int GetLastADPwdChange2(char *pszLastADPwdChange2,DWORD dwSizeofszLastADPwdChange2)
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
	VARIANT vtLastSet;
	IADsLargeInteger *pli=NULL;
	BSTR bstrProp=NULL;
	IDispatch *pIDispatch=NULL;
	long lHighDateLastSet,lLowDateLastSet;

	// récupération du DN de l'utilisateur
	hr=CoCreateInstance(CLSID_ADSystemInfo,NULL,CLSCTX_INPROC_SERVER,IID_IADsADSystemInfo,(void**)&pIADsADSystemInfo);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IADsADSystemInfo) hr=0x%08lx",hr)); goto end; }
	hr=pIADsADSystemInfo->get_UserName(&bstrUserDN);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pIADsADSystemInfo->get_UserName() hr=0x%08lx",hr)); goto end; }
	pszUserDN=GetSZFromBSTR(bstrUserDN); if (pszUserDN==NULL) goto end;
	TRACE((TRACE_INFO,_F_,"pIADsADSystemInfo->get_UserName()=%s",pszUserDN));
	
	// récupération de l'objet User dans l'AD
	sizeUserRequest=10+strlen(pszUserDN);
	pszUserRequest=(char*)malloc(sizeUserRequest); 
	if (pszUserRequest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeUserRequest)); goto end; } // 1.12B2-AC-TIE7
	sprintf_s(pszUserRequest,sizeUserRequest,"LDAP://%s",pszUserDN);
	bstrUserRequest=GetBSTRFromSZ(pszUserRequest,CP_ACP); if (bstrUserRequest==NULL) goto end;
	hr=ADsGetObject(bstrUserRequest,IID_IADsUser,(LPVOID*)&pIAdsUser);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"ADsGetObject(%S,IID_IADsUser) hr=0x%08lx\n",bstrUserRequest,hr)); goto end; }
	
	// lecture de l'attribut pwdLastSet
	bstrProp=SysAllocString(L"pwdLastSet");
	if (bstrProp==NULL) { TRACE((TRACE_ERROR,_F_,"SysAllocString()")); goto end; }
	hr=pIAdsUser->Get(bstrProp,&vtLastSet);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pUser->Get(pwdLastSet) hr=0x%08lx\n",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"vtLastSet.vt=%d",vtLastSet.vt));
	if (vtLastSet.vt!=VT_DISPATCH) { TRACE((TRACE_ERROR,_F_,"vtLastSet.vt=%d (attendu=%d)",vtLastSet.vt,VT_DISPATCH)); goto end; }
	pIDispatch=vtLastSet.pdispVal;
	TRACE((TRACE_DEBUG,_F_,"pIDispatch=0x%08lx",pIDispatch));
	if (pIDispatch==NULL) { TRACE((TRACE_ERROR,_F_,"pIDispatch=NULL")); goto end; }
	hr=pIDispatch->QueryInterface(IID_IADsLargeInteger,(void**)&pli);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pIDispatch->QueryInterface(IID_IADsLargeInteger) hr=0x%08lx\n",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"pli=0x%08lx",pli));
	hr=pli->get_HighPart(&lHighDateLastSet);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pli->get_HighPart() hr=0x%08lx\n",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"lHighDateLastSet=%lu",lHighDateLastSet));
	hr=pli->get_LowPart(&lLowDateLastSet);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pli->get_LowPart() hr=0x%08lx\n",hr)); goto end; }
	TRACE((TRACE_DEBUG,_F_,"lLowDateLastSet=%lu",lLowDateLastSet));
	sprintf_s(pszLastADPwdChange2,dwSizeofszLastADPwdChange2,"%lu,%lu",lHighDateLastSet,lLowDateLastSet);
	TRACE((TRACE_DEBUG,_F_,"pszLastADPwdChange2=%s",pszLastADPwdChange2));
	rc=0;
end:
	if (bstrUserDN!=NULL) SysFreeString(bstrUserDN);
	if (bstrUserRequest!=NULL) SysFreeString(bstrUserRequest);
	if (pszUserDN!=NULL) free(pszUserDN);
	if (pszUserRequest!=NULL) free(pszUserRequest);
	if (pIADsADSystemInfo!=NULL) pIADsADSystemInfo->Release();
	if (pIAdsUser!=NULL) pIAdsUser->Release();
	if (pIDispatch!=NULL) pIDispatch->Release(); 
	if (pli!=NULL) pli->Release();
	SysFreeString(bstrProp);

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
	CheckIfQuitMessage(msg);
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
					char *pszEncryptedADPwd=NULL;
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					// chiffrement du mot de passe AD
					pszEncryptedADPwd=swCryptEncryptString(szNewPwd1,ghKey1);
					if (pszEncryptedADPwd!=NULL)
					{
						strcpy_s(gszEncryptedADPwd,sizeof(gszEncryptedADPwd),pszEncryptedADPwd);
						free(pszEncryptedADPwd);
					}
					SecureZeroMemory(szNewPwd1,strlen(szNewPwd1));
					// Remarque : l'enregistrement dans le .ini en retour de la fonction avec SaveConfigHeader()
					EndDialog(w,IDOK);
					break;
				}
				case IDCANCEL:
					// demande à l'utilisateur de confirmer qu'il veut vraiment annuler, car s'il ne fournit pas son
					// nouveau mdp AD, le SSO sur les applications utilisant ce mdp ne fonctionnera pas !
					if (gbWarningIfNoADPwd)
					{
						 if (MessageBox(w,GetString(IDS_WARNING_AD_PWD),"swSSO",MB_YESNO | MB_ICONEXCLAMATION)==IDYES) EndDialog(w,IDCANCEL);
					}
					else
					{
						EndDialog(w,IDCANCEL);
					}
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
int AskADPwd(BOOL bWarningIfNoADPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	gbWarningIfNoADPwd=bWarningIfNoADPwd;
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
	char szLastADPwdChange2[50+1]; // ISSUE#281 : changement de format
	BOOL bAskADPwd=FALSE;
	//long lLoADPwdChange,lHiADPwdChange;
	//long lLoIniPwdChange,lHiIniPwdChange;
	
	*szLastADPwdChange2=0;
	
	// récupère la date de changement dans l'AD
	if (GetLastADPwdChange2(szLastADPwdChange2,sizeof(szLastADPwdChange2))==0) 
	{
		TRACE((TRACE_INFO,_F_,"lastADPwdChange dans l'AD    : %s",szLastADPwdChange2));
	} 
	else // si AD non dispo, pas grave, on verra la prochaine fois
	{
		TRACE((TRACE_ERROR,_F_,"Impossible de récupérer la date de dernier changement de mdp dans l'AD"));
	}

	if (*gszEncryptedADPwd==0) // pas de mot de passe AD stocké dans le .ini, on le demande
	{
		TRACE((TRACE_INFO,_F_,"Pas de mot de passe AD dans le .ini, le demande à l'utilisateur"));
		bAskADPwd=TRUE;
	}
	else // mot de passe stocké dans le .ini, il faut regarder la date de changement dans l'AD
	{
		if (*gszLastADPwdChange2==0) // pas de date de changement de mdp dans le .ini : c'est qu'on a demandé le mot de passe AD 
			                        // mais qu'on n'a pas pu joindre l'AD pour avoir la date. Dans ce cas, essaie de récupérer la date dans l'AD
									// mais on ne redemande pas le mot de passe.
		{
			TRACE((TRACE_INFO,_F_,"Mot de passe AD dans le .ini, mais pas la date de dernier changement"));
		}
		else
		{
			TRACE((TRACE_INFO,_F_,"lastADPwdChange dans le .ini : %s",gszLastADPwdChange2));
			if (strcmp(gszLastADPwdChange2,szLastADPwdChange2)!=0) // date de changement .ini différente (je tente ça en 1.12..., avant c'était antérieure) à date changement AD, on demande le mdp
			{
				TRACE((TRACE_INFO,_F_,"Mot de passe changé dans l'AD, demande le mdp à l'utilisateur"));
				bAskADPwd=TRUE;
			}
		}
	}
	
	if (bAskADPwd) // demande le mot de passe AD à l'utilisateur et le stocke dans le .ini
	{
		if (AskADPwd(TRUE)!=0) goto end;
		strcpy_s(gszLastADPwdChange2,sizeof(gszLastADPwdChange2),szLastADPwdChange2);
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
char *GetDecryptedPwd(char *szPwdEncryptedValue,BOOL bDecryptADPassword)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *ret=NULL;
	char *pszPassword=NULL;
	char *pszADPassword=NULL;
	
	pszPassword=swCryptDecryptString(szPwdEncryptedValue,ghKey1);
	if (pszPassword!=NULL) 
	{
		if (bDecryptADPassword && strcmp(pszPassword,"%ADPASSWORD%")==0) // ISSUE#361
		{
			TRACE((TRACE_DEBUG,_F_,"%%ADPASSWORD%% giPwdProtection=%d",giPwdProtection));
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

//-----------------------------------------------------------------------------
// isUserInSyncOU()
//-----------------------------------------------------------------------------
// Vérifie si l'utilisateur est dans l'OU configurée en base de registre
//-----------------------------------------------------------------------------
BOOL CheckUserInOU(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL brc=FALSE;
	HRESULT hr;
	IADsADSystemInfo *pIADsADSystemInfo=NULL;
	char *pszUserDN=NULL;
	BSTR bstrUserDN=NULL;
	
	// si OU non définie en base de registre, on retourne TRUE, la synchro sera réalisée
	if (*gszSyncSecondaryPasswordOU==0) 
	{ 
		TRACE((TRACE_INFO,_F_,"Pas d'OU definie en base de registre : go pour la synchro"));
		brc=TRUE; 
		goto end; 
	}

	// récupération du DN de l'utilisateur -- remarque : échoue si pas connecté au réseau, mais pas grave, on retourne FALSE
	// et donc rien ne sera mis à jour
	hr=CoCreateInstance(CLSID_ADSystemInfo,NULL,CLSCTX_INPROC_SERVER,IID_IADsADSystemInfo,(void**)&pIADsADSystemInfo);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"CoCreateInstance(IID_IADsADSystemInfo) hr=0x%08lx",hr)); goto end; }
	hr=pIADsADSystemInfo->get_UserName(&bstrUserDN);
	if (FAILED(hr)) { TRACE((TRACE_ERROR,_F_,"pIADsADSystemInfo->get_UserName() hr=0x%08lx",hr)); goto end; }
	pszUserDN=GetSZFromBSTR(bstrUserDN); if (pszUserDN==NULL) goto end;
	TRACE((TRACE_INFO,_F_,"pIADsADSystemInfo->get_UserName()=%s",pszUserDN));

	// vérif, retour TRUE si utilisateur dans l'OU
	// brc=(strstr(pszUserDN,gszSyncSecondaryPasswordOU)!=NULL); 
	brc=(strnistr(pszUserDN,gszSyncSecondaryPasswordOU,-1)!=NULL); // ISSUE#302, fait une comparaison non case sensitive

end:
	SysFreeString(bstrUserDN); // 1.12B2-AC-TIE5
	if (pIADsADSystemInfo!=NULL) pIADsADSystemInfo->Release();
	if (pszUserDN!=NULL) free(pszUserDN);
	TRACE((TRACE_LEAVE,_F_, "brc=%d",brc));
	return brc;
}

//-----------------------------------------------------------------------------
// GetADPassword()
//-----------------------------------------------------------------------------
// Demande le mot de passe à swSSOSVC et le stocke 
// Remarque : swSSO le chiffre par la clé dérivée de lui même ;-)
//-----------------------------------------------------------------------------
int GetADPassword(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char bufRequest[1024];
	char bufResponse[1024];
	DWORD dwLenResponse;

	// Demande le mot de passe à swSSOSVC
	// Construit la requête à envoyer à swSSOSVC : V02:GETPASS:domain(256octets)username(256octets)
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:GETPASS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	// en retour, on a reçu le mot de passe chiffré par la clé dérivée du mot de passe (si, si)
	if (dwLenResponse!=LEN_ENCRYPTED_AES256)
	{
		TRACE((TRACE_ERROR,_F_,"Longueur reponse attendue=LEN_ENCRYPTED_AES256=%d, recue=%d",LEN_ENCRYPTED_AES256,dwLenResponse)); goto end;
	}
	bufResponse[dwLenResponse]=0;
	// stocke le mot de passe chiffré, pour répondre aux demandes ultérieures traitées par GetDecryptedPwd() dans swSSOAD.cpp
	strcpy_s(gszEncryptedADPwd,sizeof(gszEncryptedADPwd),bufResponse);	

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
