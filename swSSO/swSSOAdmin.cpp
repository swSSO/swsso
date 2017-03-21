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
// Fonctions sp�cifiques au mode admin avec authentification serveur
//-----------------------------------------------------------------------------

#include "stdafx.h"
static int giRefreshTimer=10;

//-----------------------------------------------------------------------------
// ServerAdminLogin()
//-----------------------------------------------------------------------------
// V�rifie le login/mdp admin avec le serveur et stocke le cookie de session
// Si szPwd=NULL, c'est qu'on est en mode mot de passe Windows, dans ce cas il 
// faut d�chiffrer le mot de passe AD d�j� r�cup�r� aupr�s de swSSOSVC au d�marrage
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
	
	if (pszPwd==NULL)
	{
		pszAdminPwd=swCryptDecryptString(gszEncryptedADPwd,ghKey1);
		if (pszAdminPwd==NULL) goto end;
		pszEncodedAdminPwd=HTTPEncodeParam(pszAdminPwd);
		lenAdminPwd=strlen(pszAdminPwd);
		SecureZeroMemory(pszAdminPwd,lenAdminPwd);
	}
	else
	{
		pszEncodedAdminPwd=HTTPEncodeParam(pszPwd);
	}
	if (pszEncodedAdminPwd==NULL) goto end;
	sprintf_s(szPostParams,sizeof(szPostParams),"id=%s&pwd=%s",pszId,pszEncodedAdminPwd);
	lenEncodedAdminPwd=strlen(pszEncodedAdminPwd);
	SecureZeroMemory(pszEncodedAdminPwd,lenEncodedAdminPwd);

	pszResult=HTTPRequest(gszServerAddress,giServerPort,gbServerHTTPS,gszWebServiceAddress,
						  gszServerAddress2,giServerPort2,gbServerHTTPS2,gszWebServiceAddress2,
						  szGetParams,L"POST",szPostParams,strlen(szPostParams),NULL,
						  WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH,-1,NULL,&dwStatusCode);
	if (dwStatusCode!=200){ TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szGetParams,dwStatusCode)); goto end; }
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szGetParams)); goto end; }

	rc=atoi(pszResult);

end:
	// si �chec, c'est non bloquant mais il faut afficher un message pour pr�venir que 
	// les fonctions d'upload seront non disponibles et qu'il faut v�rifier que le serveur
	// est bien up ou changer le mot de passe sur le serveur pour le r�aligner
	if (dwStatusCode!=200 || pszResult==NULL) // probl�me serveur
	{
		MessageBox(w,GetString(IDS_CONFIG_PROXY),"swSSO",MB_ICONEXCLAMATION);
	}
	else if (rc!=0)
	{
		if (rc==-1) // identifiant ou mot de passe incorrect
		{
			MessageBox(w,GetString(IDS_SERVER_ADMIN_BAD_PWD),"swSSO",MB_ICONEXCLAMATION);
		}
		else if (rc==-2) // compte verrouill�
		{
			MessageBox(w,GetString(IDS_SERVER_ADMIN_LOCKED),"swSSO",MB_ICONEXCLAMATION);
		}
		else // erreur inconnue
		{
			MessageBox(w,GetString(IDS_CONFIG_PROXY),"swSSO",MB_ICONEXCLAMATION);
		}
	}
	if (hCursorOld!=NULL) SetCursor(hCursorOld);
	if (pszEncodedAdminPwd!=NULL) free(pszEncodedAdminPwd);
	if (pszAdminPwd!=NULL) free(pszAdminPwd);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ServerAdminLogout()
//-----------------------------------------------------------------------------
// D�connecte l'admin du serveur de configuration
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// ServerAdminChangePassword()
//-----------------------------------------------------------------------------
// Change le mot de passe admin sur le serveur
//-----------------------------------------------------------------------------

