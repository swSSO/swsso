//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2017 - Sylvain WERDEFROY
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
// swSSOAdmin.cpp
//-----------------------------------------------------------------------------
// Fonctions spécifiques au mode admin avec authentification serveur
//-----------------------------------------------------------------------------

#include "stdafx.h"
static int giRefreshTimer=10;

WCHAR gwcszAdminCookie[1024]=L"";
DWORD dwAdminCookie=1024;

//-----------------------------------------------------------------------------
// ServerAdminLogin()
//-----------------------------------------------------------------------------
// Vérifie le login/mdp admin avec le serveur et stocke le cookie de session
// Si szPwd=NULL, c'est qu'on est en mode mot de passe Windows, dans ce cas il 
// faut déchiffrer le mot de passe AD déjà récupéré auprès de swSSOSVC au démarrage
//-----------------------------------------------------------------------------
int ServerAdminLogin(HWND w,char *pszId, char *pszPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szGetParams[128+1];
	char szPostParams[256+1];
	char *pszEncodedAdminPwd=NULL;
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	DWORD dwStatusCode=500;
	char *pszAdminPwd=NULL;
	int lenAdminPwd;
	int lenEncodedAdminPwd;

	hCursorOld=SetCursor(ghCursorWait);

	strcpy_s(szGetParams,sizeof(szGetParams),"?action=login");
	
	if (pszPwd==NULL) // pas de mot de passe passé en paramètre, utilise mot de passe Windows
	{
		pszAdminPwd=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
		if (pszAdminPwd==NULL) goto end;
		pszEncodedAdminPwd=HTTPEncodeParam(pszAdminPwd);
		lenAdminPwd=strlen(pszAdminPwd);
		SecureZeroMemory(pszAdminPwd,lenAdminPwd);
	}
	else // utilise le mot de passe passé en paramètre
	{
		pszEncodedAdminPwd=HTTPEncodeParam(pszPwd);
	}
	if (pszEncodedAdminPwd==NULL) goto end;
	// prépare les paramètres à passer en POST
	sprintf_s(szPostParams,sizeof(szPostParams),"id=%s&pwd=%s",pszId,pszEncodedAdminPwd);
	lenEncodedAdminPwd=strlen(pszEncodedAdminPwd);
	SecureZeroMemory(pszEncodedAdminPwd,lenEncodedAdminPwd);
	// POSTe la requête
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szGetParams,L"POST",szPostParams,strlen(szPostParams),L"Content-Type: application/x-www-form-urlencoded\r\n",
						  WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,NULL,gwcszAdminCookie,dwAdminCookie,&dwStatusCode);
	SecureZeroMemory(szPostParams,sizeof(szPostParams));
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szGetParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szGetParams)); goto end; }
	if (strcmp(pszResult,"0")==0) rc=0;
	else if (strcmp(pszResult,"-1")==0) rc=-1;
	else if (strcmp(pszResult,"-2")==0) rc=-2;
	else rc=-3;
	
end:
	// si échec, c'est non bloquant mais il faut afficher un message pour prévenir que 
	// les fonctions d'upload seront non disponibles et qu'il faut vérifier que le serveur
	// est bien up ou changer le mot de passe sur le serveur pour le réaligner
	if (dwStatusCode!=200 || pszResult==NULL) // problème serveur
	{
		MessageBox(w,GetString(IDS_SERVER_ADMIN_LOGIN_KO),"swSSO",MB_ICONEXCLAMATION);
	}
	else if (rc!=0)
	{
		if (rc==-1) // identifiant ou mot de passe incorrect
		{
			MessageBox(w,GetString(IDS_SERVER_ADMIN_BAD_PWD),"swSSO",MB_ICONEXCLAMATION);
		}
		else if (rc==-2) // compte verrouillé
		{
			MessageBox(w,GetString(IDS_SERVER_ADMIN_LOCKED),"swSSO",MB_ICONEXCLAMATION);
		}
		else // erreur inconnue, peut-être parce que le serveur n'a pas encore migré, on n'affiche pas d'erreur
		{
			// MessageBox(w,gszErrorServerNotAvailable,"swSSO",MB_ICONEXCLAMATION);
		}
	}
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszEncodedAdminPwd!=NULL) free(pszEncodedAdminPwd);
	if (pszAdminPwd!=NULL) free(pszAdminPwd);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ServerAdminLogout()
//-----------------------------------------------------------------------------
// Déconnecte l'admin du serveur de configuration
//-----------------------------------------------------------------------------
int ServerAdminLogout()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szGetParams[128+1];
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	DWORD dwStatusCode=500;
	
	hCursorOld=SetCursor(ghCursorWait);
	strcpy_s(szGetParams,sizeof(szGetParams),"?action=logout");
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szGetParams,L"GET",NULL,0,NULL,
						  WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,gwcszAdminCookie,NULL,0,&dwStatusCode);
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szGetParams,dwStatusCode)); goto end; }
	rc=0;
end:
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ServerAdminSetPassword()
//-----------------------------------------------------------------------------
// Change le mot de passe admin sur le serveur
//-----------------------------------------------------------------------------
int ServerAdminSetPassword(HWND w,char *pszNewPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szGetParams[128+1];
	char szPostParams[256+1];
	char *pszEncodedAdminNewPwd=NULL;
	int lenEncodedAdminNewPwd;
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	DWORD dwStatusCode=500;
	
	hCursorOld=SetCursor(ghCursorWait);
	strcpy_s(szGetParams,sizeof(szGetParams),"?action=resetPwd");
	
	pszEncodedAdminNewPwd=HTTPEncodeParam(pszNewPwd);
	if (pszEncodedAdminNewPwd==NULL) goto end;
	
	// prépare les paramètres à passer en POST
	sprintf_s(szPostParams,sizeof(szPostParams),"newPwd=%s",pszEncodedAdminNewPwd);
	lenEncodedAdminNewPwd=strlen(pszEncodedAdminNewPwd);
	SecureZeroMemory(pszEncodedAdminNewPwd,lenEncodedAdminNewPwd);
	// POSTe la requête
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szGetParams,L"POST",szPostParams,strlen(szPostParams),L"Content-Type: application/x-www-form-urlencoded\r\n",
						  WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,gwcszAdminCookie,NULL,0,&dwStatusCode);
	SecureZeroMemory(szPostParams,sizeof(szPostParams));
	if (dwStatusCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szGetParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szGetParams)); goto end; }
	if (strcmp(pszResult,"0")==0) rc=0; // changement OK
	else if (strcmp(pszResult,"-1")==0) rc=-1; // changement KO
	else rc=-2;
	
end:
	if (dwStatusCode!=200 || pszResult==NULL) // problème serveur
	{
		MessageBox(w,GetString(IDS_CHANGE_ADMIN_PWD_KO),"swSSO",MB_ICONEXCLAMATION);
	}
	else 
	{
		if (rc==0) // changement OK
		{
			MessageBox(w,GetString(IDS_CHANGE_ADMIN_PWD_OK),"swSSO",MB_OK | MB_ICONINFORMATION);
		}
		else // erreur
		{
			MessageBox(w,GetString(IDS_CHANGE_ADMIN_PWD_KO),"swSSO",MB_ICONEXCLAMATION);
		}
	}
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszEncodedAdminNewPwd!=NULL) free(pszEncodedAdminNewPwd);
	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

