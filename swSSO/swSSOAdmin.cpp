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
	char szParams[512+1];
	char *pszResult=NULL;
	HCURSOR hCursorOld=NULL;
	DWORD dwStatusCode=500;
	char *pszAdminPwd=NULL;
	int lenAdminPwd;

	hCursorOld=SetCursor(ghCursorWait);

	if (pszPwd==NULL)
	{
		pszAdminPwd=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
		if (pszAdminPwd==NULL) goto end;
		lenAdminPwd=strlen(pszAdminPwd);
		sprintf_s(szParams,sizeof(szParams),"?action=login&id=%s&pwd=%s",pszId,pszAdminPwd);
		SecureZeroMemory(pszAdminPwd,lenAdminPwd);
		free(pszAdminPwd); pszAdminPwd=NULL;
	}
	else
	{
		sprintf_s(szParams,sizeof(szParams),"?action=login&id=%s&pwd=%s",pszId,pszPwd);
	}
	
	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szParams,L"GET",NULL,0,NULL,WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szParams)); goto end; }

	rc=atoi(pszResult);

end:
	// si échec, c'est non bloquant mais il faut afficher un message pour prévenir que 
	// les fonctions d'upload seront non disponibles et qu'il faut vérifier que le serveur
	// est bien up ou changer le mot de passe sur le serveur pour le réaligner
	if (dwStatusCode!=200 || pszResult==NULL) // problème serveur
	{
		MessageBox(w,GetString(IDS_CONFIG_PROXY),"swSSO",MB_ICONEXCLAMATION);
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
		else // erreur inconnue
		{
			MessageBox(w,GetString(IDS_CONFIG_PROXY),"swSSO",MB_ICONEXCLAMATION);
		}
	}
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ServerAdminLogout()
//-----------------------------------------------------------------------------
// Déconnecte l'admin du serveur de configuration
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// ServerAdminChangePassword()
//-----------------------------------------------------------------------------
// Change le mot de passe admin sur le serveur
//-----------------------------------------------------------------------------

