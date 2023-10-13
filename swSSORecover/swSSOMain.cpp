//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2011 - Sylvain WERDEFROY
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
// swSSOMain.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

HINSTANCE ghInstance;
HFONT ghBoldFont=NULL;
HICON ghIconAltTab=NULL;
HICON ghIconSystrayActive=NULL;

//-----------------------------------------------------------------------------
// LoadIcons()
//-----------------------------------------------------------------------------
// Chargement de toutes les icones et pointeurs de souris pour l'IHM
//-----------------------------------------------------------------------------
// [rc] : 0 si OK
//-----------------------------------------------------------------------------
static int LoadIcons(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	ghIconAltTab=(HICON)LoadImage(ghInstance,MAKEINTRESOURCE(IDI_LOGO),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
	if (ghIconAltTab==NULL) goto end;
	ghIconSystrayActive=(HICON)LoadImage(ghInstance,MAKEINTRESOURCE(IDI_SYSTRAY_ACTIVE),IMAGE_ICON,	GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	if (ghIconSystrayActive==NULL) goto end;	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// UnloadIcons()
//-----------------------------------------------------------------------------
// Déchargement de toutes les icones et pointeurs de souris
//-----------------------------------------------------------------------------
static void UnloadIcons(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	if (ghIconAltTab!=NULL) { DestroyIcon(ghIconAltTab); ghIconAltTab=NULL; }
	if (ghIconSystrayActive!=NULL) { DestroyIcon(ghIconSystrayActive); ghIconSystrayActive=NULL; }
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// WinMain()
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    HANDLE hMutex=NULL;
//	HRESULT hrCoIni=E_FAIL; // code retour CoInitialize()

	//INITCOMMONCONTROLSEX CommonControls;
	// init des traces
	TRACE_OPEN();
	TRACE((TRACE_ENTER,_F_, ""));
	
	// init de toutes les globales
	ghInstance=hInstance;

	// 0.42 vérif pas déjà lancé
	hMutex=CreateMutex(NULL,TRUE,"swSSORecover.exe");
	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		TRACE((TRACE_INFO,_F_,"Une instance est deja lancee"));
		goto end;
	}
	
	// inits Window et COM
	InitCommonControls();
	//CommonControls.dwSize=sizeof(INITCOMMONCONTROLSEX);
	//CommonControls.dwICC=ICC_STANDARD_CLASSES;
	//InitCommonControlsEx(&CommonControls);
	
	/*
	hrCoIni=CoInitialize(NULL);
	if (FAILED(hrCoIni)) 
	{
		TRACE((TRACE_ERROR,_F_,"CoInitialize hr=0x%08lx",hrCoIni));
		goto end;
	}*/

	if (LoadIcons()!=0) goto end;

	// initialisation du module crypto
	if (swCryptInit()!=0) goto end;

	LoadPasswordPolicy();
	swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_START,NULL,NULL,NULL);
	
	RecoveryWizard();

end:
	if (gpszMailSubject!=NULL) free(gpszMailSubject) ;
	if (gpszMailBodyBefore!=NULL) free(gpszMailBodyBefore) ;
	if (gpszMailBodyAfter!=NULL) free(gpszMailBodyAfter) ;
	swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_STOP,NULL,NULL,NULL);
	swCryptTerm();
	UnloadIcons();
	if (ghBoldFont!=NULL) DeleteObject(ghBoldFont);
	//if (hrCoIni==S_OK) CoUninitialize();
	if (hMutex!=NULL) ReleaseMutex(hMutex);
	TRACE((TRACE_LEAVE,_F_, ""));
	TRACE_CLOSE();
	return 0; 
}

#if 0 /*************** TESTS ECRITURE JOURNAL SECURITE **************************/
	{
		/*
		AUTHZ_SOURCE_SCHEMA_REGISTRATION assr;
		assr.dwFlags=0;
		assr.szEventSourceName=L"szEventSourceName";  
		assr.szEventMessageFile=L"szEventMessageFile";  
		assr.szEventSourceXmlSchemaFile=L"szEventSourceXmlSchemaFile";
		assr.szEventAccessStringsFile=L"szEventAccessStringsFile";
		assr.szExecutableImagePath=NULL;  
		assr.pReserved=NULL;
		assr.dwObjectTypeNameCount=0;  
		//assr.ObjectTypeNames[ANYSIZE_ARRAY];

		if (!AuthzInstallSecurityEventSource(0,&assr))
		{ TRACE((TRACE_ERROR,_F_,"AuthzInstallSecurityEventSource (GetLastError()=%d)",GetLastError())); }
		*/

		BOOL bResult = TRUE;
		AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider = NULL;

		bResult = AuthzRegisterSecurityEventSource(
				  0,
				  L"szEventSourceName",
				  &hEventProvider);

		if (!bResult) { TRACE((TRACE_ERROR,_F_,"AuthzRegisterSecurityEventSource (GetLastError()=%d)",GetLastError()));  goto Cleanup; }

		bResult = AuthzReportSecurityEvent(
				  APF_AuditSuccess,
				  hEventProvider,
				  10,
				  NULL,
				  3,
				  APT_String, L"Jay Hamlin",
				  APT_String, L"March 21, 1960",
				  APT_Ulong,  45);

		if (!bResult) { TRACE((TRACE_ERROR,_F_,"AuthzReportSecurityEvent (GetLastError()=%d)",GetLastError())); }

		Cleanup:

		if (hEventProvider)
		{
			AuthzUnregisterSecurityEventSource(
			0,
			&hEventProvider);
		}
	}


#endif