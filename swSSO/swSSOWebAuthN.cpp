//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
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
// swSSOWebAuthN.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

// ----------------------------------------------------------------------------------
// WebEnumChildProc()
// ----------------------------------------------------------------------------------
// enum des fils à la recherche de la fenêtre de rendu du navigateur
// ----------------------------------------------------------------------------------
int swWebAuthNGetAssertion(HWND w)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	WEBAUTHN_ASSERTION* pAssertion = NULL;

	LPCWSTR rpId = L"swSSO";
	HRESULT hr;
	
	WEBAUTHN_EXTENSIONS extensions;
	ZeroMemory(&extensions, sizeof(WEBAUTHN_EXTENSIONS));
	extensions.cExtensions = 0;
	extensions.pExtensions = NULL;

	WEBAUTHN_CLIENT_DATA clientData;
	ZeroMemory(&clientData, sizeof(WEBAUTHN_CLIENT_DATA));
	char clientDataJSON[] = "{""key1"":""value1"",""key2"":""value2"";}";
	clientData.dwVersion = WEBAUTHN_CLIENT_DATA_CURRENT_VERSION;
	clientData.cbClientDataJSON = (DWORD)strlen(clientDataJSON); // La taille des données JSON du client
	clientData.pbClientDataJSON = (PBYTE)clientDataJSON; // Les données JSON du client
	clientData.pwszHashAlgId = WEBAUTHN_HASH_ALGORITHM_SHA_256; // L'algorithme de hachage utilisé

	WEBAUTHN_CREDENTIALS credentialList2 ;
	ZeroMemory(&credentialList2, sizeof(WEBAUTHN_CREDENTIALS));
	BYTE credential[64] = { 0x7e,0x40,0x64,0x4f,0x4c,0xe0,0xb5,0xd3,0xa6,0xd7,0xe0,0x2d,0x70,0xe7,0xcc,0xb8,0x30,0x36,0x0d,0x2f,0xd6,0x14,0xbb,0x1b,0x47,0x41,0x27,0xc0,0xce,0xd0,0x9c,0xb7,0x05,0x97,0x6f,0xd1,0x8f,0x71,0x22,0x71,0xbb,0xd0,0x57,0xaf,0xee,0xec,0x2d,0xec,0x3b,0x44,0xc4,0xc7,0x58,0x04,0x64,0x8b,0x6f,0x4b,0x67,0xb3,0x73,0x7d,0xd5,0xe8 };
	credentialList2.cCredentials = 1; // Le nombre de credentials dans la liste
	credentialList2.pCredentials = (WEBAUTHN_CREDENTIAL*)malloc(sizeof(WEBAUTHN_CREDENTIAL)); // Allouer de la mémoire pour le tableau de credentials
	if (credentialList2.pCredentials == NULL) { TRACE((TRACE_ERROR, _F_, "malloc(%d)", sizeof(WEBAUTHN_CREDENTIAL))); goto end; }
	credentialList2.pCredentials[0].dwVersion = WEBAUTHN_CREDENTIAL_CURRENT_VERSION; // La version du credential
	credentialList2.pCredentials[0].cbId = 64;
	credentialList2.pCredentials[0].pbId = credential;
	credentialList2.pCredentials[0].pwszCredentialType = WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY; // Le type du credential

	WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS options ;
	ZeroMemory(&options, sizeof(WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS));
	options.dwVersion = WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS_CURRENT_VERSION;
	options.dwTimeoutMilliseconds = 5000; // Le délai maximal en millisecondes
	options.CredentialList = credentialList2; // La liste des credentials acceptés
	options.Extensions = extensions;
	options.dwAuthenticatorAttachment = WEBAUTHN_AUTHENTICATOR_ATTACHMENT_ANY; // Tout type d'authentificateur
	options.dwUserVerificationRequirement = WEBAUTHN_USER_VERIFICATION_REQUIREMENT_ANY; // Pas de vérification d'utilisateur requise
	options.dwFlags = 0; // Pas de flags spécifiques

	// Appeler l'API WebAuthnAuthenticatorGetAssertion
	hr = WebAuthNAuthenticatorGetAssertion(w,rpId,&clientData, &options, &pAssertion);
	if (FAILED(hr))
	{
		TRACE((TRACE_ERROR, _F_, "WebAuthNAuthenticatorGetAssertion()=0x%08lx",hr));
		goto end;
	}

end:
	if (pAssertion != NULL) WebAuthNFreeAssertion(pAssertion);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
