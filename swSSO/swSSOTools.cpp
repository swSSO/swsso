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
// swTools.cpp
//-----------------------------------------------------------------------------
// Utilitaires
//-----------------------------------------------------------------------------

#include "stdafx.h"
// #define HTTP_RESULT_MAX_SIZE 512000
// estimation : moyenne 1 Ko par config / 500 configs => 512 Ko
static DWORD gdwHTTPResultFactor=2048; // 2 Ko octets par config
static int giRefreshTimer=10;
char gszRes[512];
char gcszK3[]="33333333";
char gszComputedValue[256+1];
HWND gwMessageBox3B=NULL;
BOOL gbLastRequestOnFailOverServer=FALSE;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************


//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

// ----------------------------------------------------------------------------------
// GetString
// ----------------------------------------------------------------------------------
// lecture d'une chaine dans la string table
// ----------------------------------------------------------------------------------
// [in] identifiant de la chaine à charger
// ----------------------------------------------------------------------------------
char *GetString(UINT uiString)
{
	*gszRes=0;
	LoadString(ghInstance,uiString,gszRes,sizeof(gszRes));
	return gszRes;
}

// ----------------------------------------------------------------------------------
// GetBSTRFromSZ
// ----------------------------------------------------------------------------------
// Convertit une sz en BSTR (l'appelant doit libérer avec SysFreeString si !NULL)
// ----------------------------------------------------------------------------------
// [in] chaine sz à convertir en BSTR
// [rc] NULL si échec, la chaine convertie sinon
// ----------------------------------------------------------------------------------
BSTR GetBSTRFromSZ(const char *sz)
{
	TRACE((TRACE_ENTER,_F_,""));
	WCHAR *pwc=NULL;
	int rc,sizeofpwc;
	BSTR bstr=NULL;
	
	// Trace à activer en debug uniquement car peut contenir un mot de passe
	// TRACE((TRACE_DEBUG,_F_, "%s",sz));

	// premier appel pour connaitre taille à allouer (le 0 de fin de chaine est inclus)
	sizeofpwc=MultiByteToWideChar(CP_ACP,0,sz,-1,NULL,0);
	if (sizeofpwc==0) { TRACE((TRACE_ERROR,_F_,"MultiByteToWideChar(0)")); goto end; }
	TRACE((TRACE_INFO,_F_,"MultiByteToWideChar()->rc=%d",sizeofpwc));
	pwc=(WCHAR *)malloc(sizeofpwc*2); // *2 car 16 bits unicode
	if (pwc==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeofpwc)); goto end; }
	// conversion
	rc=MultiByteToWideChar(CP_ACP,0,sz,-1,pwc,sizeofpwc);
	if (rc==0) { TRACE((TRACE_ERROR,_F_,"MultiByteToWideChar(%d)",sizeofpwc)); goto end; }
	// bstrisation
	bstr=SysAllocString(pwc);
	if (bstr==NULL) goto end;
end:
	if (pwc!=NULL) free(pwc);
	// Trace à activer en debug uniquement car peut contenir un mot de passe
	// TRACE((TRACE_DEBUG,_F_, "%S",bstr));
	TRACE((TRACE_LEAVE,_F_, ""));
	return bstr;
}

// ----------------------------------------------------------------------------------
// Convertit une BSTR en sz (l'appelant doit libérer avec free si !NULL)
// Nouvelle fonction en 1.00 pour remplacer la conversion sprintf("%S") qui échouait
// lorsquela chaine contenait des caractères unicode (cf. ISSUE#122)
// ----------------------------------------------------------------------------------
// [in] chaine BSTR à convertir en sz
// [rc] NULL si échec, la chaine convertie sinon
// ----------------------------------------------------------------------------------
char *GetSZFromBSTR(BSTR bstr)
{
	//TRACE((TRACE_ENTER,_F_,""));
	int len;
	char *rc=NULL;

	// Trace à activer en debug uniquement car peut contenir un mot de passe
	// TRACE((TRACE_ENTER,_F_, "%S",bstr));
	len=SysStringLen(bstr);
	rc=(char*)malloc(len+1);
	if (rc==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",len+1)); goto end; }
	WideCharToMultiByte( CP_ACP, 0, bstr, -1, rc, len+1, NULL, NULL );
	rc[len]=0;
end:
	// Trace à activer en debug uniquement car peut contenir un mot de passe
	// TRACE((TRACE_DEBUG,_F_, "%s",rc));
	//TRACE((TRACE_LEAVE,_F_,""));
	return rc;
}

// ----------------------------------------------------------------------------------
// CompareBSTRtoSZ
// ----------------------------------------------------------------------------------
// Compare une BSTR à une sz
// ----------------------------------------------------------------------------------
// [rc] FALSE si différent ou erreur
// ----------------------------------------------------------------------------------
BOOL CompareBSTRtoSZ(BSTR bstr,const char *sz)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	BSTR bstrsz=NULL;
	int lenbstr,lenbstrsz;

	TRACE((TRACE_DEBUG,_F_, "%S vs %s",bstr,sz));
	bstrsz=GetBSTRFromSZ(sz); if (bstrsz==NULL) goto end;
	lenbstr=SysStringByteLen(bstr);
	lenbstrsz=SysStringByteLen(bstrsz);
	if (lenbstr==lenbstrsz && memcmp(bstr,bstrsz,lenbstr)==0) rc=TRUE;
	
end:
	if (bstrsz!=NULL) SysFreeString(bstrsz);
	TRACE((TRACE_LEAVE,_F_, "%d",rc));
	return rc;
}


// ----------------------------------------------------------------------------------
// HTTPRequestOneServer
// ----------------------------------------------------------------------------------
// Exécute la requête HTTP passée en paramètre
// L'appelant doit libérer le resultat !
// ----------------------------------------------------------------------------------
char *HTTPRequestOneServer(const char *pszServer,			// [in] FQDN du serveur (www.swsso.fr)
				  int iPort,						// [in] port
				  BOOL bHTTPS,						// [in] TRUE=https, FALSE=http
				  const char *pszAddress,			// [in] adresse du service (/webservice5.php)
				  const char *pszParams,			// [in] ?param1=...&param2=...
				  LPCWSTR pwszMethod,				// [in] Méthode : GET | POST | PUT | ...
				  void *pRequestData,				// [in] Données à envoyer avec la requête (NULL si aucune)
				  DWORD dwLenRequestData,			// [in] Taille des données à envoyer avec la requête (0 si aucune)
				  LPCWSTR pwszHeaders,				// [in] Entêtes à envoyer (NULL si aucune)
				  DWORD dwAutologonSecurityLevel,	// [in] WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW | MEDIUM | HIGH
				  int timeout,						// [in] timeout
				  T_PROXYPARAMS *pInProxyParams,	// [in] paramètre proxy ou NULL si pas de proxy
				  LPWSTR pwszInCookie,				// [in] cookie à envoyer
				  LPWSTR pwszOutCookie,				// [out] cookie reçu (buffer alloué par l'appelant, de taille suffisante)
				  DWORD  dwOutCookie,				// [out] taille du buffer fourni pour recevoir le cookie
				  DWORD *pdwStatusCode)				// [out] status http renseigné
{
	TRACE((TRACE_ENTER,_F_, ""));
	DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
	DWORD dwLenResult=0;
	char *pszResult=NULL;
	char *p;
    BOOL  brc = FALSE;
    HINTERNET hSession = NULL; 
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	T_PROXYPARAMS *pProxyParams=NULL;
	DWORD dwHTTPResultMaxSize;
	DWORD dwOptions;
	DWORD dwFlags;
	DWORD dwStatusCodeSize=sizeof(DWORD);
	BSTR bstrProxyURL=NULL;
	BSTR bstrProxyUser=NULL;
	BSTR bstrProxyPwd=NULL;
	BSTR bstrServerAddress=NULL;
	BSTR bstrRequest=NULL;
	char *pszURLWithParams=NULL;
	DWORD sizeofURLWithParams;

	*pdwStatusCode=418; // valeur par défaut si jamais on sort avant la fin

	// 0.89 : si pas de paramètres proxy reçus, utilise les valeurs globales
	if (pInProxyParams==NULL)
	{
		pProxyParams=(T_PROXYPARAMS *)malloc(sizeof(T_PROXYPARAMS));
		if (pProxyParams==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeof(T_PROXYPARAMS))); goto end;} // 1.12B2
		pProxyParams->bInternetUseProxy=gbInternetUseProxy;
		strcpy_s(pProxyParams->szProxyURL, LEN_PROXY_URL+1 ,gszProxyURL);
		strcpy_s(pProxyParams->szProxyUser,LEN_PROXY_USER+1,gszProxyUser);
		strcpy_s(pProxyParams->szProxyPwd, LEN_PROXY_PWD+1 ,gszProxyPwd);
	}
	else
		pProxyParams=pInProxyParams;

	if (pProxyParams->bInternetUseProxy && *(pProxyParams->szProxyURL)!=0)
	{
		TRACE((TRACE_INFO,_F_,"Proxy:%s",pProxyParams->szProxyURL)); 
		bstrProxyURL=GetBSTRFromSZ(pProxyParams->szProxyURL);
		if (bstrProxyURL==NULL) goto end;
		hSession = WinHttpOpen(L"swsso.exe",WINHTTP_ACCESS_TYPE_NAMED_PROXY,bstrProxyURL,WINHTTP_NO_PROXY_BYPASS, 0);
	}
	else
	{
		TRACE((TRACE_INFO,_F_,"Proxy:PAS DE PROXY")); 
		hSession = WinHttpOpen(L"swsso.exe",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS, 0);
	}
	if (hSession==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpOpen(proxy:%s)",pProxyParams->szProxyURL)); goto end; }
    
	WinHttpSetTimeouts(hSession, timeout*1000, timeout*1000, timeout*1000, timeout*1000); 
	
	// WinHttpConnect
	bstrServerAddress=GetBSTRFromSZ(pszServer);
	if (bstrServerAddress==NULL) goto end;
	hConnect = WinHttpConnect(hSession,bstrServerAddress,(INTERNET_PORT)iPort, 0); // ISSUE#162 (port configurable)
	if (hConnect==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpConnect(%s) - port : %d",pszServer,iPort)); goto end; }
    
	// WinHttpOpenRequest
	// construit la requête avec les paramètres
	sizeofURLWithParams=strlen(pszAddress)+strlen(pszParams)+1;
	pszURLWithParams=(char*)malloc(sizeofURLWithParams);
	if (pszURLWithParams==NULL)  { TRACE((TRACE_ERROR,_F_,"malloc(%d+%d)",pszAddress,pszParams)); goto end; }
	strcpy_s(pszURLWithParams,sizeofURLWithParams,pszAddress);
	strcat_s(pszURLWithParams,sizeofURLWithParams,pszParams);

	bstrRequest=GetBSTRFromSZ(pszURLWithParams);
	if (bstrRequest==NULL) goto end;
	dwFlags=WINHTTP_FLAG_ESCAPE_PERCENT;
	if (bHTTPS) dwFlags|=WINHTTP_FLAG_SECURE;
    hRequest = WinHttpOpenRequest(hConnect,pwszMethod,bstrRequest,NULL, WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,dwFlags); // ISSUE#162 (HTTPS possible)
	TRACE((TRACE_INFO,_F_,"WinHttpOpenRequest(%s://%s:%d%s)",bHTTPS?"https":"http",pszServer,iPort,pszURLWithParams)); 
	if (hRequest==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpOpenRequest(%s %s)",pwszMethod,pszURLWithParams)); goto end; }

	if (bHTTPS && !gbCheckCertificates) // ISSUE#252
	{
		dwOptions=SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		brc=WinHttpSetOption(hRequest,WINHTTP_OPTION_SECURITY_FLAGS,&dwOptions,sizeof(dwOptions));
		if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpSetOption(0x%08lx)=0x%08lx",dwOptions,GetLastError())); goto end; }
		TRACE((TRACE_DEBUG,_F_,"WinHttpSetOption(SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA)"));
	}

	dwOptions=dwAutologonSecurityLevel;
	brc=WinHttpSetOption(hRequest,WINHTTP_OPTION_AUTOLOGON_POLICY,&dwOptions,sizeof(dwOptions));
	if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpSetOption(WINHTTP_OPTION_AUTOLOGON_POLICY %ld)=0x%08lx",dwAutologonSecurityLevel,GetLastError())); goto end; }

	if (*(pProxyParams->szProxyUser)!=0)
	{
		bstrProxyUser=GetBSTRFromSZ(pProxyParams->szProxyUser);
		if (bstrProxyUser==NULL) goto end;
		bstrProxyPwd=GetBSTRFromSZ(pProxyParams->szProxyPwd);
		if (bstrProxyPwd==NULL) goto end;
		brc=WinHttpSetCredentials(hRequest,WINHTTP_AUTH_TARGET_PROXY,WINHTTP_AUTH_SCHEME_BASIC,bstrProxyUser,bstrProxyPwd,NULL);
		TRACE((TRACE_INFO,_F_,"WinHttpSetCredentials(user:%s)",pProxyParams->szProxyUser)); 
		if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpSetCredentials()=0x%08lx",GetLastError())); goto end; }
	}
	// Trace à activer en debug uniquement car dans les données postées il peut y avoir le mot de passe admin !
	// TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pRequestData,dwLenRequestData,"pRequestData (methode %S)",pwszMethod));
	TRACE((TRACE_INFO,_F_,"pRequestData (methode %S) len=%d",pwszMethod,dwLenRequestData));
	
	// ISSUE#342 : envoi cookie si fourni
	if (pwszInCookie!=NULL && wcslen(pwszInCookie)!=0)
	{
		WCHAR wcszCookie[1024]=L"Cookie:";
		if (wcslen(pwszInCookie)>900)
		{
			TRACE((TRACE_ERROR,_F_,"Cookie trop long (%d) ignoré :",wcslen(pwszInCookie),pwszInCookie));
		}
		else
		{
			wcscat_s(wcszCookie,1024,pwszInCookie);
			if (WinHttpAddRequestHeaders(hRequest,wcszCookie,(DWORD)-1L,WINHTTP_ADDREQ_FLAG_ADD))
			{
				TRACE((TRACE_INFO,_F_,"Cookie ajouté : %S",wcszCookie));
			}
			else
			{
				TRACE((TRACE_ERROR,_F_,"WinHttpAddRequestHeaders()=%ld, cookie non ajouté : %S",GetLastError(),wcszCookie));
			}
		}
	}

	brc = WinHttpSendRequest(hRequest,pwszHeaders==NULL?WINHTTP_NO_ADDITIONAL_HEADERS:pwszHeaders,(DWORD)-1L,pRequestData,dwLenRequestData,dwLenRequestData,0);
	if (!brc) 
	{ 
		TRACE((TRACE_ERROR,_F_,"WinHttpSendRequest()=%ld",GetLastError())); // ISSUE#275, déplacement de la trace avant le logevent
		swLogEvent(EVENTLOG_ERROR_TYPE,MSG_SERVER_NOT_RESPONDING,(char*)pszServer,(char*)pszURLWithParams,NULL,NULL,0); 
		goto end; 
	}
 
    brc = WinHttpReceiveResponse(hRequest, NULL);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpReceiveResponse()=%d",GetLastError())); goto end; }

	dwHTTPResultMaxSize=gdwHTTPResultFactor*(giMaxConfigs<=500?500:giMaxConfigs);
	TRACE((TRACE_INFO,_F_,"dwHTTPResultMaxSize=%u",dwHTTPResultMaxSize));
	pszResult=(char*)malloc(dwHTTPResultMaxSize);
	TRACE((TRACE_DEBUG,_F_,"pszResult=0x%08lx",pszResult));
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwHTTPResultMaxSize)); goto end; }
	ZeroMemory(pszResult,dwHTTPResultMaxSize);
	p=pszResult;
    do 
	{
        dwSize = 0;
        brc=WinHttpQueryDataAvailable(hRequest,&dwSize);
		TRACE((TRACE_DEBUG,_F_,"WinHttpQueryDataAvailable=%ld",dwSize));
		if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpQueryDataAvailable()")); goto end; }
		if (p+dwSize > pszResult+dwHTTPResultMaxSize) { TRACE((TRACE_ERROR,_F_,"Buf too small (dwSize=%ld)",dwSize)); goto end; }
        brc=WinHttpReadData(hRequest,p, dwSize, &dwDownloaded);
        if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpReadData()")); goto end; }
		p+=dwSize;
		dwLenResult+=dwDownloaded;
    } while (dwSize>0);

	WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,NULL,pdwStatusCode,&dwStatusCodeSize,NULL);
	TRACE((TRACE_INFO,_F_,"dwStatusCode=%d",*pdwStatusCode));
	
	// Récupère les cookies éventuels si buffer passé en paramètre
	if (pwszOutCookie!=NULL && dwOutCookie>0)
	{
		if (WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_SET_COOKIE,WINHTTP_HEADER_NAME_BY_INDEX,pwszOutCookie,&dwOutCookie,WINHTTP_NO_HEADER_INDEX))
		{
			TRACE((TRACE_DEBUG,_F_,"Cookie recu : %S",pwszOutCookie));
		}
	}
	
#ifdef TRACES_ACTIVEES	
	if (dwLenResult>2048)
	{
		TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pszResult,2048,"pszResult (2048 premiers caractères):"));
	}
	else
	{
		TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pszResult,dwLenResult,"pszResult (taille %d):",dwLenResult));
	}	
#endif
end:
    if (hRequest!=NULL) WinHttpCloseHandle(hRequest);
    if (hConnect!=NULL) WinHttpCloseHandle(hConnect);
    if (hSession!=NULL) WinHttpCloseHandle(hSession);
	if (pszURLWithParams!=NULL) free(pszURLWithParams);
	SysFreeString(bstrProxyURL);
	SysFreeString(bstrProxyUser);
	SysFreeString(bstrProxyPwd);
	SysFreeString(bstrServerAddress);
	SysFreeString(bstrRequest);
	if (pInProxyParams==NULL && pProxyParams!=NULL) free(pProxyParams); // 1.12B2-TIE4 + ne faire le free que si pas reçu en paramètre ! Corrigé en 1.12B3
	TRACE((TRACE_LEAVE,_F_, "pszResult=0x%08lx",pszResult));
	return pszResult;
}

// ----------------------------------------------------------------------------------
// HTTPRequest (avec failover si un 2ème serveur est défini)
// ----------------------------------------------------------------------------------
// Exécute la requête HTTP passée en paramètre
// L'appelant doit libérer le resultat !
// ----------------------------------------------------------------------------------
char *HTTPRequest(const char *pszServer,			// [in] FQDN du serveur (www.swsso.fr)
				  int iPort,						// [in] port
				  BOOL bHTTPS,						// [in] TRUE=https, FALSE=http
				  const char *pszAddress,			// [in] adresse du service (/webservice5.php)
				  const char *pszServer2,			// [in] FQDN du serveur (www.swsso.fr) -- failover
				  int iPort2,						// [in] port -- failover
				  BOOL bHTTPS2,						// [in] TRUE=https, FALSE=http -- failover
				  const char *pszAddress2,			// [in] adresse du service (/webservice5.php) -- failover
				  const char *pszParams,			// [in] ?param1=...&param2=...
				  LPCWSTR pwszMethod,				// [in] Méthode : GET | POST | PUT | ...
				  void *pRequestData,				// [in] Données à envoyer avec la requête (NULL si aucune)
				  DWORD dwLenRequestData,			// [in] Taille des données à envoyer avec la requête (0 si aucune)
				  LPCWSTR pwszHeaders,				// [in] Entêtes à envoyer (NULL si aucune)
				  DWORD dwAutologonSecurityLevel,	// [in] WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW | MEDIUM | HIGH
				  int timeout,						// [in] timeout
				  T_PROXYPARAMS *pInProxyParams,	// [in] paramètre proxy ou NULL si pas de proxy
				  LPWSTR pwszInCookie,				// [in] cookie à envoyer
				  LPWSTR pwszOutCookie,				// [out] cookie reçu (buffer alloué par l'appelant, de taille suffisante)
				  DWORD  dwOutCookie,				// [out] taille du buffer fourni pour recevoir le cookie
				  DWORD *pdwStatusCode)				// [out] status http renseigné
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszResult=NULL;
	gbLastRequestOnFailOverServer=FALSE;
	pszResult=HTTPRequestOneServer(pszServer,iPort,bHTTPS,pszAddress,
						pszParams,pwszMethod,pRequestData,dwLenRequestData,pwszHeaders,
						dwAutologonSecurityLevel, timeout==-1?giWebServiceTimeout:timeout,pInProxyParams,
						pwszInCookie,pwszOutCookie,dwOutCookie,pdwStatusCode);
	if (*pdwStatusCode!=200 || pszResult==NULL)
	{
		if (*pszServer2!=0 && *pszAddress2!=0)
		{
			gbLastRequestOnFailOverServer=TRUE;
			TRACE((TRACE_INFO,_F_, "Trying failover server now"));
			if (pszResult!=NULL) free(pszResult);
			pszResult=HTTPRequestOneServer(pszServer2,iPort2,bHTTPS2,pszAddress2,
						pszParams,pwszMethod,pRequestData,dwLenRequestData,pwszHeaders,
						dwAutologonSecurityLevel, timeout==-1?giWebServiceTimeout2:timeout,pInProxyParams,
						pwszInCookie,pwszOutCookie,dwOutCookie,pdwStatusCode);
		}
	}
	TRACE((TRACE_LEAVE,_F_, "pszResult=0x%08lx",pszResult));
	return pszResult;
}

//-----------------------------------------------------------------------------
// HTTPEncodeURL()
//-----------------------------------------------------------------------------
// Encode une URL -- ISSUE#332
// Dans la version actuelle, seuls le caractère ? et & sont encodé en %3F et %26
//-----------------------------------------------------------------------------
char *HTTPEncodeURL(char *pszToEncode)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszEncoded=NULL;
	int lenToEncode,lenEncoded;
	int nbCarsToEncode;
	int i,j;
	
	if (pszToEncode==NULL) goto end; // ce n'est pas une erreur

	// compte le nb de caractères & pour connaitre la taille à allouer
	lenToEncode=strlen(pszToEncode);
	if (lenToEncode>1024) // c'est quand même beaucoup, cf. ISSUE#298, dans ce cas, on tronque 
	{
		lenToEncode=1024;
	}
	nbCarsToEncode=0;
	for (i=0;i<lenToEncode;i++)
	{
		if (pszToEncode[i]=='?' || pszToEncode[i]=='&') nbCarsToEncode++;
	}
	lenEncoded=lenToEncode+(2*nbCarsToEncode);
	TRACE((TRACE_DEBUG,_F_,"lenToEncode=%d nbCarsToEncode=%d lenEncoded=%d",lenToEncode,nbCarsToEncode,lenEncoded));
	// allocation destination
	pszEncoded=(char*)malloc(lenEncoded+1);
	if (pszEncoded==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenEncoded+1)); goto end; }
	// copie avec encodage des caractères
	for (i=0,j=0;i<lenToEncode;i++)
	{
		if (pszToEncode[i]=='?') 
		{
			memcpy(pszEncoded+j,"%3F",3);
			j+=3;
		}
		else if (pszToEncode[i]=='&') 
		{
			memcpy(pszEncoded+j,"%26",3);
			j+=3;
		}
		else
		{
			pszEncoded[j]=pszToEncode[i];
			j++;
		}
	}
	pszEncoded[j]=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%s",pszEncoded));
	return pszEncoded;
}

//-----------------------------------------------------------------------------
// HTTPEncodeParam()
//-----------------------------------------------------------------------------
// Encode un paramètre à passer dans une URL
// Dans la version actuelle, seul le caractère & est encodé en %26
//-----------------------------------------------------------------------------
char *HTTPEncodeParam(char *pszToEncode)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszEncoded=NULL;
	int lenToEncode,lenEncoded;
	int nbCarsToEncode;
	int i,j;
	
	// compte le nb de caractères & pour connaitre la taille à allouer
	lenToEncode=strlen(pszToEncode);
	if (lenToEncode>1024) // c'est quand même beaucoup, cf. ISSUE#298, dans ce cas, on tronque 
	{
		lenToEncode=1024;
	}
	nbCarsToEncode=0;
	for (i=0;i<lenToEncode;i++)
	{
		if (pszToEncode[i]=='&') nbCarsToEncode++;
	}
	lenEncoded=lenToEncode+(2*nbCarsToEncode);
	TRACE((TRACE_DEBUG,_F_,"lenToEncode=%d nbCarsToEncode=%d lenEncoded=%d",lenToEncode,nbCarsToEncode,lenEncoded));
	// allocation destination
	pszEncoded=(char*)malloc(lenEncoded+1);
	if (pszEncoded==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenEncoded+1)); goto end; }
	// copie avec encodage des caractères
	for (i=0,j=0;i<lenToEncode;i++)
	{
		if (pszToEncode[i]=='&') 
		{
			memcpy(pszEncoded+j,"%26",3);
			j+=3;
		}
		else
		{
			pszEncoded[j]=pszToEncode[i];
			j++;
		}
	}
	pszEncoded[j]=0;
end:
	// Trace à activer en debug uniquement car peut contenir un mot de passe
	// TRACE((TRACE_DEBUG_F_, "pszEncoded=%s",pszEncoded));
	TRACE((TRACE_LEAVE,_F_,""));
	return pszEncoded;
}

//-----------------------------------------------------------------------------
// HTTPDecodeParam()
//-----------------------------------------------------------------------------
// Decode un paramètre URL recu dans le fichier XML
// Dans la version actuelle, seul le caractère %26 est décodé en &
//-----------------------------------------------------------------------------
char *HTTPDecodeParam(char *pszToDecode)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char *pszDecoded=NULL;
	int lenToDecode,lenDecoded;
	int nbCarsToDecode;
	int i,j;
	
	// compte le nb de caractères %26 pour connaitre la taille à allouer
	lenToDecode=strlen(pszToDecode);
	nbCarsToDecode=0;
	for (i=0;i<lenToDecode;i++)
	{
		if (memcmp(pszToDecode+i,"%26",3)==0) nbCarsToDecode++;
	}
	lenDecoded=lenToDecode-(2*nbCarsToDecode);
	TRACE((TRACE_DEBUG,_F_,"lenToDecode=%d nbCarsToDecode=%d lenDecoded=%d",lenToDecode,nbCarsToDecode,lenDecoded));
	// allocation destination
	pszDecoded=(char*)malloc(lenDecoded+1);
	if (pszDecoded==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenDecoded+1)); goto end; }
	// copie avec décodage des caractères
	for (i=0,j=0;i<lenToDecode;j++)
	{
		if (memcmp(pszToDecode+i,"%26",3)==0)
		{
			pszDecoded[j]='&';
			i+=3;
		}
		else
		{
			pszDecoded[j]=pszToDecode[i];
			i++;
		}
	}
	pszDecoded[j]=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%s",pszDecoded));
	return pszDecoded;
}
// ----------------------------------------------------------------------------------
// swGetTopWindow
// ----------------------------------------------------------------------------------
// Retourne la fenêtre qui est au premier plan
// #110 : s'il y a des fenêtres topmost (type barre d'outils, ex. "Quick Launch" de 
//        Windows, la fenêtre retournée n'est pas forcément la bonne.
//        D'où l'idée de gérer une liste d'exclusion
// ----------------------------------------------------------------------------------
int swGetTopWindow(HWND *w, char *szTitle,int sizeofTitle)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int rc=-1;
	int i=0;
	
	*w=GetTopWindow(HWND_DESKTOP);
	while (rc!=0 && *w!=NULL)
	{
		if (IsWindowVisible(*w))
		{
			GetWindowText(*w,szTitle,sizeofTitle);
			if (*szTitle!=0) // si titre non vide, le compare à la liste des fenêtres exclues (#110)
			{
				rc=0;
				if (strcmp(szTitle,"swSSO - Lanceur d'applications")==0 || 
					strcmp(szTitle,"swSSO - Application launcher")==0) // 1.20b2 : ajout de la traduction...
				{
					TRACE((TRACE_INFO,_F_, "Fenêtre exclue : %s",szTitle));
					rc=-1;
				}
				if ((rc==0) && (giOSVersion==OS_WINDOWS_VISTA || giOSVersion==OS_WINDOWS_7 || giOSVersion==OS_WINDOWS_8))
				{
					if (strcmp(szTitle,"Démarrer")==0 || strcmp(szTitle,"Start")==0)
					{
						TRACE((TRACE_INFO,_F_, "Fenêtre exclue : %s",szTitle));
						rc=-1;
					}
				}
				// ISSUE#143 : exclut la fenêtre gestion des sites et applications
				if (swStringMatch(szTitle,"swSSO - Gestion des sites et application*") ||
					swStringMatch(szTitle,"swSSO - Web sites and applications manager*")) // 1.20b2 : ajout de la traduction...
				{
					TRACE((TRACE_INFO,_F_, "Fenêtre exclue : %s",szTitle));
					rc=-1;
				}
				// ISSUE#347 : exclut les fenêtres techniques de Edge
				if (strcmp(szTitle,"Microsoft Edge")==0 ||
					strcmp(szTitle,"Hôte contextuel")==0 ||
					strcmp(szTitle,"Contextual host")==0)
				{
					TRACE((TRACE_INFO,_F_, "Fenêtre exclue : %s",szTitle));
					rc=-1;
				}
				if (rc==0) 
				{
					for (i=0;i<giNbExcludedWindows;i++)
					{
						// ISSUE#124 : on utilise la fonction de comparaison qui prend en compte les jokers *
						// if (_stricmp(szTitle,gtabszExcludedWindows[i])==0) // fenêtre exclue
						if (swStringMatch(szTitle,gtabszExcludedWindows[i])) // fenêtre exclue
						{ 
							TRACE((TRACE_INFO,_F_, "Fenêtre exclue : %s",gtabszExcludedWindows[i]));
							rc=-1 ; // comme rc=-1, la boucle continue à la recherche d'une autre fenêtre
							break;
						}
					}
				}
			}
		}
		if (rc!=0) *w=GetNextWindow(*w,GW_HWNDNEXT);
	}
	if (rc==0)
	{
		TRACE((TRACE_INFO,_F_,"Fenêtre trouvée : %s",szTitle));
	}
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// GetConfigBoolValue()
// ----------------------------------------------------------------------------------
// Lecture d'une valeur YES | NO et si pas trouvé retourne + écrit la valeur par défaut
// ----------------------------------------------------------------------------------
BOOL GetConfigBoolValue(char *szSection,char *szItem,BOOL bDefault,BOOL bWriteIfNotFound)
{
	TRACE((TRACE_ENTER,_F_, "%s",szItem));
	char szTmp[3+1];
	BOOL rc=bDefault;
	GetPrivateProfileString(szSection,szItem,"",szTmp,sizeof(szTmp),gszCfgFile);
	if (*szTmp==0) // valeur non trouvé, on retourne la valeur par défaut et on l'écrit dans le fichier de config
	{
		if (bWriteIfNotFound && !gbCheckIniIntegrity) // ISSUE#164 : si vérif d'intégrité, n'écrit pas les valeurs par défaut pour que la lecture reste une lecture...
		{ 
			WritePrivateProfileString(szSection,szItem,bDefault?"YES":"NO",gszCfgFile); 
		} 
	}
	else if (strcmp(szTmp,"NO")==0)
		rc=FALSE;
	else
		rc=TRUE;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// Help()
//-----------------------------------------------------------------------------
// Ouvre le fichier swsso.chm si présent, sinon ouvre le pdf, sinon pointe vers site web
//-----------------------------------------------------------------------------
void Help(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szHelpFile[_MAX_PATH+1];
	int len;
	int rc;
	BOOL bFound=FALSE;
	char *p;

	// construit le nom du fichier chm
	// ISSUE#249 il faut chercher le .chm dans le dossier de l'exécutable et pas dans le current directory qui peut-être ailleurs...
	// len=GetCurrentDirectory(_MAX_PATH-50,szHelpFile);
	len=GetModuleFileName(ghInstance,szHelpFile,_MAX_PATH-10);
	if (len==0) goto end;
	p=strrchr(szHelpFile,'\\');
	if (p==NULL) goto end;
	len=p+1-szHelpFile;

	// 1er essai : swsso.chm
	strcpy_s(szHelpFile+len,_MAX_PATH-len,"swSSO.chm"); // ISSUE #110 : correction plantage
	rc=(int)ShellExecute(NULL,"open",szHelpFile,NULL,"",SW_SHOW);
	TRACE((TRACE_INFO,_F_,"ShellExecute(%s)=%d",szHelpFile,rc)); 
	if (rc>32) bFound=TRUE;
end:
	if (!bFound)
	{
		// 2ème essai : swSSO vX.YY - Manuel utilisateur.pdf
		if (strcmp(gcszCurrentBeta,"0000")==0) // pas de beta, ouvre le manuel de la release
			wsprintf(szHelpFile+len,"swSSO v%c.%c%c - Manuel utilisateur.pdf",gcszCurrentVersion[0],gcszCurrentVersion[1],gcszCurrentVersion[2]);
		else // ouvre le manuel de la beta
			wsprintf(szHelpFile+len,"swSSO v%c.%c%c - Manuel utilisateur.pdf",gcszCurrentBeta[0],gcszCurrentBeta[1],gcszCurrentBeta[2]);
		rc=(int)ShellExecute(NULL,"open",szHelpFile,NULL,"",SW_SHOW);
		TRACE((TRACE_INFO,_F_,"ShellExecute(%s)=%d",szHelpFile,rc));
		if (rc>32) bFound=TRUE;
	}
	if (!bFound)
	{
		// 3ème essai : swSSO.pdf
		strcpy_s(szHelpFile+len,_MAX_PATH-len,"swSSO.pdf"); // ISSUE #110 : correction plantage
		rc=(int)ShellExecute(NULL,"open",szHelpFile,NULL,"",SW_SHOW);
		TRACE((TRACE_INFO,_F_,"ShellExecute(%s)=%d",szHelpFile,rc));
		if (rc>32) bFound=TRUE;
	}
	if (!bFound)
	{
		// 4ème essai : pointe vers le site web
		rc=(int)ShellExecute(NULL,"open","http://www.swsso.fr/?page_id=108",NULL,"",SW_SHOW );
		TRACE((TRACE_INFO,_F_,"ShellExecute(www.swsso.fr)=%d",rc)); 
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// strnistr()
//-----------------------------------------------------------------------------
// strstr non case sensitive
// trouvé chez code guru, à regarder de plus près, j'ai copié collé tel quel.
// http://www.codeguru.com/cpp/cpp/string/article.php/c5641
//-----------------------------------------------------------------------------
char *strnistr (const char *szStringToBeSearched,
				const char *szSubstringToSearchFor,
				const int  nStringLen)
{
   int            nLen;
   int            nOffset;
   int            nMaxOffset;
   char   *  pPos;
   int            nStringLenInt;

   // verify parameters
   if ( szStringToBeSearched == NULL ||
        szSubstringToSearchFor == NULL )
   {
      return (char*)szStringToBeSearched;
   }

   // get length of the substring
   nLen = _tcslen(szSubstringToSearchFor);

   // empty substring-return input (consistent w/ strstr)
   if ( nLen == 0 ) {
      return (char*)szStringToBeSearched;
   }

   if ( nStringLen == -1 || nStringLen >
               (int)_tcslen(szStringToBeSearched) )
   {
      nStringLenInt = _tcslen(szStringToBeSearched);
   } else {
      nStringLenInt = nStringLen;
   }

   nMaxOffset = nStringLenInt - nLen;

   pPos = (char*)szStringToBeSearched;

   for ( nOffset = 0; nOffset <= nMaxOffset; nOffset++ ) {

      if ( _tcsnicmp(pPos, szSubstringToSearchFor, nLen) == 0 ) {
         return pPos;
      }
      // move on to the next character
      pPos++; //_tcsinc was causing problems :(
   }

   return NULL;
}

//-----------------------------------------------------------------------------
// MessageBox3BDialogProc()
//-----------------------------------------------------------------------------
// Dialogproc de la MessageBox à 3 boutons avec bandeau logo
//-----------------------------------------------------------------------------
static int CALLBACK MessageBox3BDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
				gwMessageBox3B=w;
				T_MESSAGEBOX3B_PARAMS *pParams=(T_MESSAGEBOX3B_PARAMS *)lp;
				// icone ALT-TAB
				SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
				SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// titre en gras
				SetTextBold(w,TX_SUBTITLE);
				// titre et message 
				SetDlgItemText(w,TX_SUBTITLE,pParams->szSubTitle);
				SetDlgItemText(w,TX_MESSAGE,pParams->szMessage);
				// cache les boutons non définis et met les libellés dans les boutons définis
				if (pParams->iB1String!=-1)
					SetDlgItemText(w,PB_B1,GetString(pParams->iB1String));
				else
					ShowWindow(GetDlgItem(w,PB_B1),SW_HIDE);
				if (pParams->iB2String!=-1)
					SetDlgItemText(w,PB_B2,GetString(pParams->iB2String));
				else
					ShowWindow(GetDlgItem(w,PB_B2),SW_HIDE);
				if (pParams->iB3String!=-1)
					SetDlgItemText(w,IDCANCEL,GetString(pParams->iB3String));
				else
					ShowWindow(GetDlgItem(w,IDCANCEL),SW_HIDE);
				// centre le boutons s'il n'y en a qu'un
				if (pParams->iB2String==-1 && pParams->iB3String==-1)
				{
					RECT rect;
					GetClientRect(w,&rect);
					SetWindowPos(GetDlgItem(w,PB_B1),NULL,((rect.right-rect.left)/2)-40,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
				}
				else if (pParams->iB3String==-1) // centre les boutons s'il n'y en a que deux
				{
					RECT rect;
					GetClientRect(w,&rect);
					SetWindowPos(GetDlgItem(w,PB_B1),NULL,((rect.right-rect.left)/2)-90,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
					SetWindowPos(GetDlgItem(w,PB_B2),NULL,((rect.right-rect.left)/2)+10,rect.bottom-30,0,0,SWP_NOSIZE | SWP_NOZORDER);
				}

				SetWindowText(w,GetString(pParams->iTitleString));
				SendDlgItemMessage(w,STATIC_ICONE,STM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIcon(NULL,pParams->szIcone));
				
				if (pParams->bVCenterSubTitle)
				{
					LONG lStyle=GetWindowLong(GetDlgItem(w,TX_SUBTITLE),GWL_STYLE);
					SetWindowLong(GetDlgItem(w,TX_SUBTITLE),GWL_STYLE,lStyle | SS_CENTERIMAGE);
				}
				if (pParams->szMailTo!=NULL)
				{
					ShowWindow(GetDlgItem(w,TX_MAILTO),SW_SHOW);
					SetDlgItemText(w,TX_MAILTO,pParams->szMailTo);
					// liens MAILTO soulignes
					LOGFONT logfont;
					HFONT hFont;
					hFont=(HFONT)SendMessage(w,WM_GETFONT,0,0);
					if(hFont!=NULL)
					{
						if(GetObject(hFont, sizeof(LOGFONT), (LPSTR)&logfont)!= NULL) logfont.lfUnderline=TRUE;
						hFont=CreateFontIndirect(&logfont);
						if(hFont!=NULL) PostMessage(GetDlgItem(w,TX_MAILTO),WM_SETFONT,(LPARAM)hFont,TRUE);
					}
				}

				MACRO_SET_SEPARATOR;
				// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
				// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
				if (giRefreshTimer==giTimer) giRefreshTimer=11;
				SetTimer(w,giRefreshTimer,200,NULL);
			}
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
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case PB_B1:
					EndDialog(w,B1);
					break;
				case PB_B2:
					EndDialog(w,B2);
					break;
				case IDCANCEL:
					EndDialog(w,B3);
					break;
			}
			break;
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_SUBTITLE:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
				case TX_MAILTO:
					SetTextColor((HDC)wp,RGB(0,0,255));
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH); // 0.60
					break;
			}
			break;
		case WM_SETCURSOR:
		{
			POINT point={0,0};
			HWND hwndChild=NULL;
			INT iDlgCtrlID=0;
		
			GetCursorPos(&point);
			MapWindowPoints(HWND_DESKTOP,w,&point,1);
			hwndChild=ChildWindowFromPoint(w,point);
			iDlgCtrlID=GetDlgCtrlID(hwndChild);
			if (iDlgCtrlID==TX_MAILTO)
			{
				if(IsWindowVisible(hwndChild))
				{
					SetCursor(ghCursorHand);
					rc=TRUE;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			POINT pt;
			pt.x=LOWORD(lp); 
			pt.y=HIWORD(lp);
			MapWindowPoints(w,HWND_DESKTOP,&pt,1);
			RECT rect;
			GetWindowRect(GetDlgItem(w,TX_MAILTO),&rect);
			if ((pt.x >= rect.left)&&(pt.x <= rect.right)&& (pt.y >= rect.top) &&(pt.y <= rect.bottom))
			{
				int lenMailTo;
				char *pszMailTo=NULL;
				lenMailTo=100+strlen(gpszTitleBeingAdded)+strlen(gszConfigNotFoundMailTo)+strlen(gpszConfigNotFoundMailSubject)+strlen(gpszConfigNotFoundMailBody)+strlen(gszDomainLabel);
				if (gpszURLBeingAdded!=NULL) lenMailTo+=strlen(gpszURLBeingAdded);
					
				pszMailTo=(char*)malloc(lenMailTo);
				if (pszMailTo==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenMailTo)); goto end; }
				sprintf_s(pszMailTo,lenMailTo,"mailto:%s?subject=%s&body=%s",gszConfigNotFoundMailTo,gpszConfigNotFoundMailSubject,gpszConfigNotFoundMailBody);
				strcat_s(pszMailTo,lenMailTo,"%0DDomaine : ");
				strcat_s(pszMailTo,lenMailTo,gszDomainLabel);
				strcat_s(pszMailTo,lenMailTo,"%0DTitre : ");
				strcat_s(pszMailTo,lenMailTo,gpszTitleBeingAdded);
				if (gpszURLBeingAdded!=NULL) 
				{
					strcat_s(pszMailTo,lenMailTo,"%0DURL : ");
					strcat_s(pszMailTo,lenMailTo,gpszURLBeingAdded);
				}
				ShellExecute(NULL,"open",pszMailTo,NULL,"",SW_SHOW );
				free(pszMailTo);
			}
			break;
		}
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
end:
	return rc;
}

//-----------------------------------------------------------------------------
// MessageBox3B()
//-----------------------------------------------------------------------------
// MessageBox à 3 boutons avec bandeau logo
//-----------------------------------------------------------------------------
int MessageBox3B(T_MESSAGEBOX3B_PARAMS *pParams)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc;

	rc=DialogBoxParam(ghInstance,MAKEINTRESOURCE(pParams->szMailTo==NULL?IDD_MESSAGEBOX3B:IDD_MESSAGEBOX3BLINK),pParams->wParent,MessageBox3BDialogProc,(LPARAM)pParams);
	gwMessageBox3B=NULL;

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetModifiedFont()
//-----------------------------------------------------------------------------
// Retourne la police des dialogbox modifiée avec les attribués passés en param
// Attention, l'appelant devra libérer avec DeleteObject().
//-----------------------------------------------------------------------------
HFONT GetModifiedFont(HWND w,long lfWeight)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HFONT hCurrentFont=NULL;
	HFONT hNewFont=NULL;

	LOGFONT logfont;
	hCurrentFont=(HFONT)SendMessage(w,WM_GETFONT,0,0);
	if(hCurrentFont!=NULL)
	{
		if(GetObject(hCurrentFont, sizeof(LOGFONT), (LPSTR)&logfont)!= NULL) 
		{
			logfont.lfWeight=lfWeight;
			hNewFont=CreateFontIndirect(&logfont);
		}
	}
	TRACE((TRACE_LEAVE,_F_, "rc=0x%08lx",hNewFont));
	return hNewFont;
}

//-----------------------------------------------------------------------------
// SetTextBold()
//-----------------------------------------------------------------------------
// Change la police d'un contrôle
// Remarque : la police Bold est affectée à ghBoldFont pour les appels ultérieurs
//            et sera libérée en fin de winmain.
//-----------------------------------------------------------------------------
void SetTextBold(HWND w,int iCtrlId)
{
	TRACE((TRACE_ENTER,_F_, ""));

	if (ghBoldFont==NULL) 
	{
		ghBoldFont=GetModifiedFont(w,FW_BOLD);
		if (ghBoldFont==NULL) goto end;
	}
	HWND wItem=GetDlgItem(w,iCtrlId);
	if(wItem==NULL) goto end;
	
	PostMessage(wItem,WM_SETFONT,(LPARAM)ghBoldFont,TRUE);
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// DrawTransparentBitmap()
//-----------------------------------------------------------------------------
// Affichage d'une image avec transparence
//-----------------------------------------------------------------------------
BOOL DrawTransparentBitmap(HANDLE hBitmap,HDC dc,int x,int y,int cx,int cy,COLORREF crColour)
{
	TRACE((TRACE_ENTER,_F_, ""));

	BOOL  bResult=TRUE;
	COLORREF crOldBack = SetBkColor(dc,RGB(255,255,255));
	COLORREF crOldText = SetTextColor(dc,RGB(0,0,0));
	HDC dcImage=NULL;
	HDC dcTrans=NULL;
	HBITMAP bitmapTrans=NULL;
	HGDIOBJ hOldBitmapTrans=NULL;
	HGDIOBJ hOldBitmapImage=NULL;
 
	// Create two memory dcs for the image and the mask
	dcImage=CreateCompatibleDC(dc);
	dcTrans=CreateCompatibleDC(dc);
	
	// Select the image into the appropriate dc
	hOldBitmapImage = SelectObject(dcImage,hBitmap);
	
	// Create the mask bitmap
	bitmapTrans=CreateBitmap(cx,cy, 1, 1, NULL);
	if (bitmapTrans==NULL) { TRACE((TRACE_ERROR,_F_,"CreateBitmap()")); goto end;}
	
	// Select the mask bitmap into the appropriate dc
	hOldBitmapTrans = SelectObject(dcTrans,bitmapTrans);

	// Build mask based on transparent colour
	SetBkColor(dcImage,crColour);
	BitBlt(dcTrans,0, 0, cx,cy, dcImage, 0, 0, SRCCOPY);

	// Do the work - True Mask method - cool if not actual display
	if (bResult) bResult=BitBlt(dc,x,y,cx,cy,dcImage,0,0,SRCINVERT);
	if (bResult) bResult=BitBlt(dc,x,y,cx,cy,dcTrans,0,0,SRCAND);
	if (bResult) bResult=BitBlt(dc,x,y,cx,cy,dcImage,0,0,SRCINVERT);
	
end:
	// Restore settings
	SelectObject(dcImage,hOldBitmapImage);
	SelectObject(dcTrans,hOldBitmapTrans);
	SetBkColor(dc,crOldBack);
	SetTextColor(dc,crOldText);
	// libérations
	if (dcImage!=NULL) DeleteDC(dcImage);
	if (dcTrans!=NULL) DeleteDC(dcTrans);
	if (bitmapTrans!=NULL) DeleteObject(bitmapTrans);

	TRACE((TRACE_LEAVE,_F_, "bResult=%d",bResult));
	return bResult;
}

//-----------------------------------------------------------------------------
// DrawBitmap()
//-----------------------------------------------------------------------------
// Affichage d'une image
//-----------------------------------------------------------------------------
void DrawBitmap(HANDLE hBitmap,HDC dc,int x,int y,int cx,int cy)
{
	TRACE((TRACE_ENTER,_F_, "hBitmap=0x%08lx dc=0x%08lx",hBitmap,dc));

	HDC dcImage=NULL;
	HGDIOBJ hOldBitmapImage=NULL;
 
	if (hBitmap==NULL) { TRACE((TRACE_ERROR,_F_,"hBitmap=NULL !")); goto end; }
	if (dc==NULL) { TRACE((TRACE_ERROR,_F_,"dc=NULL !")); goto end; }

	dcImage=CreateCompatibleDC(dc);
	if (dcImage==NULL) { TRACE((TRACE_ERROR,_F_,"CreateCompatibleDC()=%ld",GetLastError())); goto end; }

	hOldBitmapImage = SelectObject(dcImage,hBitmap);
	if (!BitBlt(dc,x,y,cx,cy,dcImage,0,0,SRCCOPY)) { TRACE((TRACE_ERROR,_F_,"BitBlt(logo,SRCCOPY)=%ld",GetLastError())); }
end:
	if (dcImage!=NULL) 
	{
		if (hOldBitmapImage!=NULL) SelectObject(dcImage,hOldBitmapImage);
		DeleteDC(dcImage);
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// DrawLogoBar()
//-----------------------------------------------------------------------------
// Affichage du bandeau blanc avec logo swSSO
//-----------------------------------------------------------------------------
void DrawLogoBar(HWND w,int iHeight,HANDLE hLogoFondBlanc)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));

    PAINTSTRUCT ps;
	RECT rect;
	HDC dc=NULL;

	if (!GetClientRect(w,&rect)) { TRACE((TRACE_ERROR,_F_,"GetClientRect(0x%08lx)=%ld",w,GetLastError()));  goto end; }
	dc=BeginPaint(w,&ps);
	if (dc==NULL) { TRACE((TRACE_ERROR,_F_,"BeginPaint()=%ld",GetLastError())); goto end;}
	DrawBitmap(hLogoFondBlanc,dc,0,0,60,iHeight);
	if (!BitBlt(dc,60,0,rect.right-60,iHeight,0,0,0,WHITENESS)) { TRACE((TRACE_ERROR,_F_,"BitBlt(WHITENESS)=%ld",GetLastError())); }
end:
	if (dc!=NULL) EndPaint(w,&ps);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// KBSimEx()
//-----------------------------------------------------------------------------
// Mots clés supportés : [ID][ID2][ID3][PWD][ENTER][TAB][STAB][pause de x millisec]
// A partir de la 0.94 (ISSUE#67) : [ALT][/ALT][SHIFT][/SHIFT][CTRL][/CTRL]
// A partir de la 0.95 (ISSUE#76) : [LEFT][RIGHT][UP][DOWN]
//-----------------------------------------------------------------------------
int KBSimEx(HWND w,char *szCmd, char *szId1,char *szId2,char *szId3,char *szId4,char *szPwd)
{
	TRACE((TRACE_ENTER,_F_,"w=0x%08lx szCmd=%s",w,szCmd));
	TRACE((TRACE_DEBUG,_F_,"szId1=%s szId2=%s szId3=%s szId4=%s",szId1,szId2,szId3,szId4));
	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));

	int rc=-1;
	char *p=szCmd;
	char szTempo[10];
	int iTempo;
	int n;

	while (*p!=0)
	{
		// mise au premier plan de la fenêtre
		if (w!=NULL) { SetForegroundWindow(w); SetFocus(w); }

		if (_strnicmp(p,"[TAB]",strlen("[TAB]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[TAB]"));
			keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),0,0);	
			keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[TAB]");
		}
		else if (_strnicmp(p,"[STAB]",strlen("[STAB]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[STAB]"));
			keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);
			keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),0,0);
			keybd_event(VK_TAB,LOBYTE(MapVirtualKey(VK_TAB,0)),KEYEVENTF_KEYUP,0);
			keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0);
			p+=strlen("[STAB]");
		}
		else if (_strnicmp(p,"[ENTER]",strlen("[ENTER]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ENTER]"));
			keybd_event(VK_RETURN,LOBYTE(MapVirtualKey(VK_RETURN,0)),0,0);	
			keybd_event(VK_RETURN,LOBYTE(MapVirtualKey(VK_RETURN,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[ENTER]");
		}
		else if (_strnicmp(p,"[SANSNOM]",strlen("[SANSNOM]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[SANSNOM]"));
			keybd_event(VK_RETURN,LOBYTE(MapVirtualKey(VK_RETURN,0)),0,0);	
			keybd_event(VK_RETURN,LOBYTE(MapVirtualKey(VK_RETURN,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[SANSNOM]");
		}
		else if (_strnicmp(p,"[ID]",strlen("[ID]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ID]=%s",szId1));
			KBSim(w,FALSE,0,GetComputedValue(szId1),FALSE);
			p+=strlen("[ID]");
		}
		else if (_strnicmp(p,"[ID2]",strlen("[ID2]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[I2]=%s",szId2));
			KBSim(w,FALSE,0,GetComputedValue(szId2),FALSE);
			p+=strlen("[ID2]");
		}
		else if (_strnicmp(p,"[ID3]",strlen("[ID3]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ID3]=%s",szId3));
			KBSim(w,FALSE,0,GetComputedValue(szId3),FALSE);
			p+=strlen("[ID3]");
		}
		else if (_strnicmp(p,"[ID4]",strlen("[ID4]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ID4]=%s",szId4));
			KBSim(w,FALSE,0,GetComputedValue(szId4),FALSE);
			p+=strlen("[ID4]");
		}
		else if (_strnicmp(p,"[PWD]",strlen("[PWD]"))==0) 
		{ 	
			//TRACE((TRACE_PWD,_F_,"[PWD]=%s",szPwd));
			KBSim(w,FALSE,0,szPwd,TRUE);
			p+=strlen("[PWD]");
		}
		else if (_strnicmp(p,"[NOFOCUS]",strlen("[NOFOCUS]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[NOFOCUS]"));
			p+=strlen("[NOFOCUS]");
		}
		else if (_strnicmp(p,"[SHIFT]",strlen("[SHIFT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[SHIFT]"));
			keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),0,0);	
			p+=strlen("[SHIFT]");
		}
		else if (_strnicmp(p,"[/SHIFT]",strlen("[/SHIFT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[/SHIFT]"));
			keybd_event(VK_SHIFT,LOBYTE(MapVirtualKey(VK_SHIFT,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[/SHIFT]");
		}
		else if (_strnicmp(p,"[ALT]",strlen("[ALT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ALT]"));
			keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),0,0);	
			p+=strlen("[ALT]");
		}
		else if (_strnicmp(p,"[/ALT]",strlen("[/ALT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[/ALT]"));
			keybd_event(VK_MENU,LOBYTE(MapVirtualKey(VK_MENU,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[/ALT]");
		}
		else if (_strnicmp(p,"[CTRL]",strlen("[CTRL]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[CTRL]"));
			keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),0,0);	
			p+=strlen("[CTRL]");
		}
		else if (_strnicmp(p,"[/CTRL]",strlen("[/CTRL]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[/CTRL]"));
			keybd_event(VK_CONTROL,LOBYTE(MapVirtualKey(VK_CONTROL,0)),KEYEVENTF_KEYUP,0); 
			p+=strlen("[/CTRL]");
		}
		else if (_strnicmp(p,"[LEFT]",strlen("[LEFT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[LEFT]"));
			keybd_event(VK_LEFT,LOBYTE(MapVirtualKey(VK_LEFT,0)),0,0);	
			p+=strlen("[LEFT]");
		}
		else if (_strnicmp(p,"[RIGHT]",strlen("[RIGHT]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[RIGHT]"));
			keybd_event(VK_RIGHT,LOBYTE(MapVirtualKey(VK_RIGHT,0)),0,0);	
			p+=strlen("[RIGHT]");
		}
		else if (_strnicmp(p,"[UP]",strlen("[UP]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[UP]"));
			keybd_event(VK_UP,LOBYTE(MapVirtualKey(VK_UP,0)),0,0);	
			p+=strlen("[UP]");
		}
		else if (_strnicmp(p,"[DOWN]",strlen("[DOWN]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[DOWN]"));
			keybd_event(VK_DOWN,LOBYTE(MapVirtualKey(VK_DOWN,0)),0,0);	
			p+=strlen("[DOWN]");
		}
		else if (*p=='[')
		{
			// cherche le crochet fermant et conserve le contenu entre crochets
			p++;
			n=0;
			while (*p!=']' && *p!=0 && n<6)
			{
				szTempo[n]=*p;
				p++;
				n++;
			}
			szTempo[n]=0;
			TRACE((TRACE_DEBUG,_F_,"[%s]",szTempo));
			iTempo=atoi(szTempo);
			Sleep(iTempo);
			*p++;
		}
		else // saisie des caractères hors mots-clés
		{
			char sz[2];
			sz[0]=*p;
			sz[1]=0;
			KBSim(w,FALSE,0,sz,FALSE);
			p++;
		}
	}
	rc=0;

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// atox4()
//-----------------------------------------------------------------------------
// atoi pour convertir chaine hexa de 4 caractères en int, -1 si erreur
//-----------------------------------------------------------------------------
int atox4(char *sz)
{
	TRACE((TRACE_ENTER,_F_, "in:%s",sz));
	int i,ret=-1;
	unsigned char uc;
	ret=0;
	for (i=0;i<4;i++)
	{
		if (sz[i]>='A' && sz[i]<='F')      uc=(unsigned char)(sz[i]-'A'+10);
		else if (sz[i]>='0' && sz[i]<='9') uc=(unsigned char)(sz[i]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",sz[i]));ret=-1;goto end; }
		ret*=16;
		ret+=uc;
		TRACE((TRACE_DEBUG,_F_,"i=%d uc=%d ret=%d",i,uc,ret));
	}
end:
	TRACE((TRACE_LEAVE,_F_, "out:%04X",ret));
	return ret;
}

//-----------------------------------------------------------------------------
// swStringMatch()
//-----------------------------------------------------------------------------
// Comparaison de chaines non case sensitive avec prise en compte du joker * avant ou après
// [in] szToBeCompared : chaine à évaluer
// [in] szPattern : chaine avec joker
// [rc] TRUE si matche, FALSE sinon.
//
// Exemples de matchings attendus :
// 
// 1) szToBeCompared="08/06/2010 TOTO"            / szPattern="*TOTO"
// 2) szToBeCompared="TOTO 08/06/2010"            / szPattern="TOTO*"
// 3) szToBeCompared="08/06/2010 TOTO 08/06/2010" / szPattern="*TOTO*"
// 4) szToBeCompared="08/06/2010"				  / szPattern="08/06/2010"
//
// Idée d'implémentation simple (il faut que ça booste car appelé 2 fois par seconde
// pour chaque fenêtre visible à l'écran !) pour ces 3 cas :
// 1) Si szPattern commence par * et longueur 5, on compare les 4 derniers caractères 
//    de szToBeCompared avec les 4 derniers caractères de szPattern
// 2) Si szPattern finit par * et longueur 5, on compare les 4 premiers caractères 
//    de szToBeCompared avec les 4 premiers caractères de szPattern
// 3) Si szPattern commence et termine par *, on regarde si la chaine entre les * 
//    est inclue dans szToBeCompared.
// 4) Si pas de joker en début ou fin de szPattern, comparaison stricte
//
// Nouveau en 1.02 (ISSUE#161) : possibilité de placer en complément une * ailleurs que début et fin
// 4 cas de matching attendus
// A) szToBeCompared="abcdXYZ1234"					/ szPattern="abcd*1234"
// B) szToBeCompared="XYZabcd891234"				/ szPattern="*abcd*1234"
// C) szToBeCompared="abcdXYZ123489"				/ szPattern="abcd*1234*"
// D) szToBeCompared="XYZabcdEFG123489"				/ szPattern="*abcd*1234*"
//-----------------------------------------------------------------------------
BOOL swStringMatch(char *szToBeCompared,char *szPattern)
{
	//TROP VERBEUX ! TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	int lenToBeCompared,lenPattern;
	char *pMidJoker;
	char *pszPatG=NULL;
	char *pszPatD=NULL;
	int lenPatG,lenPatD;
	
	//TROP VERBEUX ! TRACE((TRACE_DEBUG,_F_, "szToBeCompared=%s szPattern=%s",szToBeCompared,szPattern));
	if (szToBeCompared==NULL || szPattern==NULL) goto end;
	lenToBeCompared=strlen(szToBeCompared);
	lenPattern=strlen(szPattern);
	if (lenPattern==1 && szPattern[0]=='*') { rc=TRUE; goto end; } // * matche avec tout
	if (lenToBeCompared==0 || lenPattern==0) goto end; // chaines vides ne matchent jamais... à revoir peut-être.

	// Pour ne pas perturber ni ralentir le fonctionnement, on commence par tester s'il y a un joker 
	// ailleurs que début et fin : si oui, on part dans une branche à part, sinon on branche sur l'existant
	pMidJoker=strchr(szPattern+1,'*');
	if (pMidJoker!=NULL && pMidJoker!=szPattern+lenPattern-1) // on a trouvé une * et ce n'est pas le dernier caractère (ni le premier puisqu'on a fait le strchr sur szPattern+1)
	// traitement des cas A à D - ISSUE#161
	{
		if (lenPattern>2 && szPattern[0]=='*' && szPattern[lenPattern-1]=='*') // cas D : szPattern commence et se termine par *
		{
			// stocke les 2 parties du pattern dans PatG et PatD
			lenPatG=pMidJoker-szPattern-1;
			lenPatD=szPattern+lenPattern-pMidJoker-2;
			if (lenToBeCompared<lenPatG+lenPatD) goto end; // ne peut pas matcher si plus courte que la taille des 2 patterns
			pszPatG=(char*)malloc(lenPatG+1); if (pszPatG==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatG+1)); goto end; }
			pszPatD=(char*)malloc(lenPatD+1); if (pszPatD==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatD+1)); goto end; }
			strncpy_s(pszPatG,lenPatG+1,szPattern+1,lenPatG);
			strncpy_s(pszPatD,lenPatD+1,pMidJoker+1,lenPatD);
			// PatG doit être dans szToBeCompared avant la position de PatD
			// PatD doit être dans szToBeCompared à partir de la fin de la position de PatG
			rc=((strnistr(szToBeCompared,pszPatG,lenToBeCompared-lenPatD)!=NULL) &&
				(strnistr(szToBeCompared+lenPatG,pszPatD,-1)!=NULL));
		}
		else if (lenPattern>1 && szPattern[0]=='*') // cas B : szPattern commence par *
		{
			// stocke les 2 parties du pattern dans PatG et PatD
			lenPatG=pMidJoker-szPattern-1;
			lenPatD=szPattern+lenPattern-pMidJoker-1;
			if (lenToBeCompared<lenPatG+lenPatD) goto end; // ne peut pas matcher si plus courte que la taille des 2 patterns
			pszPatG=(char*)malloc(lenPatG+1); if (pszPatG==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatG+1)); goto end; }
			pszPatD=(char*)malloc(lenPatD+1); if (pszPatD==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatD+1)); goto end; }
			strncpy_s(pszPatG,lenPatG+1,szPattern+1,lenPatG);
			strncpy_s(pszPatD,lenPatD+1,pMidJoker+1,lenPatD);
			// szToBeCompared doit finir par PatD et PatG doit être dans szToBeCompared avant la position de PatD
			rc=((_strnicmp(szToBeCompared+lenToBeCompared-lenPatD,pszPatD,lenPatD)==0) &&
				(strnistr(szToBeCompared,pszPatG,lenToBeCompared-lenPatD)!=NULL));
		}
		else if (lenPattern>1 && szPattern[lenPattern-1]=='*') // cas C : szPattern se termine par *
		{
			// stocke les 2 parties du pattern dans PatG et PatD
			lenPatG=pMidJoker-szPattern;
			lenPatD=szPattern+lenPattern-pMidJoker-2;
			if (lenToBeCompared<lenPatG+lenPatD) goto end; // ne peut pas matcher si plus courte que la taille des 2 patterns
			pszPatG=(char*)malloc(lenPatG+1); if (pszPatG==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatG+1)); goto end; }
			pszPatD=(char*)malloc(lenPatD+1); if (pszPatD==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatD+1)); goto end; }
			strncpy_s(pszPatG,lenPatG+1,szPattern,lenPatG);
			strncpy_s(pszPatD,lenPatD+1,pMidJoker+1,lenPatD);
			// szToBeCompared doit commencer par PatG et PatD doit être dans szToBeCompared à partir de la fin de la position de PatG
			rc=((_strnicmp(szToBeCompared,pszPatG,lenPatG)==0) &&
				(strnistr(szToBeCompared+lenPatG,pszPatD,-1)!=NULL));
		}
		else // cas A : pas de * en début et fin
		{
			// stocke les 2 parties du pattern dans PatG et PatD
			lenPatG=pMidJoker-szPattern;
			lenPatD=szPattern+lenPattern-pMidJoker-1;
			if (lenToBeCompared<lenPatG+lenPatD) goto end; // ne peut pas matcher si plus courte que la taille des 2 patterns
			pszPatG=(char*)malloc(lenPatG+1); if (pszPatG==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatG+1)); goto end; }
			pszPatD=(char*)malloc(lenPatD+1); if (pszPatD==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenPatD+1)); goto end; }
			strncpy_s(pszPatG,lenPatG+1,szPattern,lenPatG);
			strncpy_s(pszPatD,lenPatD+1,pMidJoker+1,lenPatD);
			// szToBeCompared doit commencer par PatG et finir par PatD pour matcher
			rc=((_strnicmp(szToBeCompared,pszPatG,lenPatG)==0) &&
				(_strnicmp(szToBeCompared+lenToBeCompared-lenPatD,pszPatD,lenPatD)==0));
		}
	}
	else // pas de * hors début et fin, traitement des cas 1 à 4
	{
		if (lenPattern>2 && szPattern[0]=='*' && szPattern[lenPattern-1]=='*') // cas n°3 : szPattern commence et se termine par *
		{
			char *pszModifiedPattern=(char*)malloc(lenPattern-2+1); // +1 ajouté dans 0.92B7, pouvait provoquer le plantage vu par Erwan
			if (pszModifiedPattern==NULL) goto end;
			memcpy(pszModifiedPattern,szPattern+1,lenPattern-2);
			pszModifiedPattern[lenPattern-2]=0;
			// TROP VERBEUX TRACE((TRACE_DEBUG,_F_,"szToBeCompared=%s pszModifiedPattern=%s",szToBeCompared,pszModifiedPattern));
			rc=(strnistr(szToBeCompared,pszModifiedPattern,-1)!=NULL);
			free(pszModifiedPattern);
		}
		else if (lenPattern>1 && szPattern[0]=='*') // cas n°1 : szPattern commence par *
		{
			rc=(_strnicmp(szToBeCompared+lenToBeCompared-(lenPattern-1),szPattern+1,lenPattern-1)==0);
		}
		else if (lenPattern>1 && szPattern[lenPattern-1]=='*') // cas n°2 : szPattern se termine par *
		{
			rc=(_strnicmp(szToBeCompared,szPattern,lenPattern-1)==0);
		}
		else // cas n°4 : pas de *, comparaison stricte
		{
			rc=(_stricmp(szToBeCompared,szPattern)==0);
		}
	}
end:
	//TROP VERBEUX ! TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	if (pszPatG!=NULL) free(pszPatG);
	if (pszPatD!=NULL) free(pszPatD);
	return rc;
}

//-----------------------------------------------------------------------------
// swURLMatch()
//-----------------------------------------------------------------------------
// Match URL
// 0.93B6 | ISSUE#43 : pour compatibilité Chrom tente le matching avec/sans http:// 
// 1.13B5 | ISSUE#304 : tolérance sur présence / absence du / à la fin
//-----------------------------------------------------------------------------
BOOL swURLMatch(char *szToBeCompared,char *szPattern)
{
	TRACE((TRACE_ENTER,_F_, "szToBeCompared=%s szPattern=%s",szToBeCompared,szPattern));
	BOOL rc=FALSE;
	int lenPattern,lenToBeCompared;

	// 1er test : si ça matche, on termine direct
	if (swStringMatch(szToBeCompared,szPattern)) { rc=TRUE; goto end; }
	// ISSUE#304 : 2nd test avec tolérance sur présence / absence du / à la fin. Si ça matche, on termine direct
	lenToBeCompared=strlen(szToBeCompared);
	if (lenToBeCompared>4096) { TRACE((TRACE_ERROR,_F_,"lenToBeCompared>4096 (%d)",lenToBeCompared));goto end; }
	if (lenToBeCompared>1 && szToBeCompared[lenToBeCompared-1]=='/')
	{
		char szToBeCompared2[4096+1];
		memcpy(szToBeCompared2,szToBeCompared,lenToBeCompared-1);
		szToBeCompared2[lenToBeCompared-1]=0;
		if (swStringMatch(szToBeCompared2,szPattern)) { rc=TRUE; goto end; }
	}
	// n'a pas matché direct : peut-être que l'utilisateur a défini http://www.... alors qu'il est 
	// sous chrome et donc on va essayer en virant le http:// (sauf si commence par * auquel cas ça 
	// aurait dû matcher dans swStringMatch)
	lenPattern=strlen(szPattern);
	if (lenPattern>1 && szPattern[0]=='*') goto end;
	if (lenPattern<7) goto end;
	if (_strnicmp(szPattern,"http://",7)==0) 
	{
		TRACE((TRACE_DEBUG,_F_,"szPattern commence par http://, on tente le matching entre %s et %s",szToBeCompared,szPattern+7));
		if (swStringMatch(szToBeCompared,szPattern+7)) { rc=TRUE; goto end; }
		// dernier essai :avec ou sans / de fin d'URL
		if (lenToBeCompared>1 && szToBeCompared[lenToBeCompared-1]=='/')
		{
			char szToBeCompared2[4096+1];
			memcpy(szToBeCompared2,szToBeCompared,lenToBeCompared-1);
			szToBeCompared2[lenToBeCompared-1]=0;
			if (swStringMatch(szToBeCompared2,szPattern+7)) { rc=TRUE; goto end; }
		}
	}
	else if (_strnicmp(szToBeCompared,"http://",7)==0) // l'url commence par http://, mais pas la pattern
	{
		if (lenToBeCompared<7) goto end;
		TRACE((TRACE_DEBUG,_F_,"szToBeCompared commence par http://, on tente le matching entre %s et %s",szToBeCompared+7,szPattern));
		if (swStringMatch(szToBeCompared+7,szPattern)) { rc=TRUE; goto end; }
		// dernier essai :avec ou sans / de fin d'URL
		if (szToBeCompared[lenToBeCompared-1]=='/')
		{
			char szToBeCompared2[4096+1];
			memcpy(szToBeCompared2,szToBeCompared+7,lenToBeCompared-7-1);
			szToBeCompared2[lenToBeCompared-7-1]=0;
			if (swStringMatch(szToBeCompared2,szPattern)) { rc=TRUE; goto end; }
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

void ReplaceStr(char *pszSource, int sizeofSource,char *pszToReplace, char *pszReplaceWith)
{
	TRACE((TRACE_ENTER,_F_, "pszSource=%s pszToReplace=%s pszReplace=%s",pszSource,pszToReplace,pszReplaceWith));
	char buffer[UNLEN+1];
	char *p;

	p=strstr(pszSource,pszToReplace);
	if (p==NULL) goto end;

	strncpy_s(buffer,sizeof(buffer),pszSource,p-pszSource);
	buffer[p-pszSource]=0;
	UNREFERENCED_PARAMETER(sizeofSource);
	sprintf_s(buffer+(p-pszSource),sizeof(buffer)-(p-pszSource),"%s%s",pszReplaceWith,p+strlen(pszToReplace));
	strcpy_s(pszSource,sizeofSource,buffer);

end:
	TRACE((TRACE_LEAVE,_F_, "pszSource=%s",pszSource));
	return;
}

//-----------------------------------------------------------------------------
// GetComputedValue() - 0.93B1
//-----------------------------------------------------------------------------
// Retourne la valeur passée en paramètre ou sa valeur calculée si elle désigne
// une variable d'environnement (commence et se termine par %).
// Si échec lecture var environnement, retourne la valeur passée en paramètre
// C'est moche, mais valeur retournée est une globale, donc pas besoin de libérer
// par l'appelant
//-----------------------------------------------------------------------------
char *GetComputedValue(const char *szValue)
{
	TRACE((TRACE_ENTER,_F_, "szValue=%s",szValue));
	int len;
	int rc;
	char szCopyOfValue[LEN_ID+1];
	char szEnvVariableName[50+1];
	char szEnvVariableNameWithMarks[50+2+1];
	char szEnvVariableValue[100+1];
	char *p,*p1,*p2;
	BOOL bFini=FALSE;
	
	// par défaut, retourne la valeur fournie en paramètre
	strcpy_s(gszComputedValue,sizeof(gszComputedValue),szValue);

	len=strlen(szValue);
	if (len>LEN_ID) { TRACE((TRACE_ERROR,_F_,"len(%s)=%d > %d",szValue,len,LEN_ID)); }

	// ISSUE#355
	// 1) Commence par remplacer les mots clés swSSO : %UPPER_USERNAME% et %LOWER_USERNAME% (%USERNAME% est une vairable d'environnement standard !)
	if (strstr(gszComputedValue,"%UPPER_USERNAME%")!=NULL)
	{
		char szUpperUserName[UNLEN+1];
		int i;
		int lenUserName=strlen(gszUserName);
		for (i=0;i<=lenUserName;i++) { szUpperUserName[i]=(char)toupper(gszUserName[i]); }
		ReplaceStr(gszComputedValue,sizeof(gszComputedValue),"%UPPER_USERNAME%",szUpperUserName);
	}
	if (strstr(gszComputedValue,"%LOWER_USERNAME%")!=NULL)
	{
		char szLowerUserName[UNLEN+1];
		int i;
		int lenUserName=strlen(gszUserName);
		for (i=0;i<=lenUserName;i++) { szLowerUserName[i]=(char)tolower(gszUserName[i]); }
		ReplaceStr(gszComputedValue,sizeof(gszComputedValue),"%LOWER_USERNAME%",szLowerUserName);
	}
	
	// 2) Remplace ensuite les variables d'environnement
	p=gszComputedValue;
	while (!bFini)
	{
		p1=strchr(p,'%');
		if (p1==NULL)
		{
			bFini=TRUE;
		}
		else
		{
			p=p1+1;
			p2=strchr(p1+1,'%');
			if (p2!=NULL) // on a une variable d'environnement entre p1+1 et p2-1
			{
				strncpy_s(szEnvVariableNameWithMarks,sizeof(szEnvVariableNameWithMarks),p1,p2-p1+1);
				strncpy_s(szEnvVariableName,sizeof(szEnvVariableName),p1+1,p2-p1-1);
				rc=GetEnvironmentVariable(szEnvVariableName,szEnvVariableValue,sizeof(szEnvVariableValue));
				if (rc==0) // non trouvée, on ne touche à rien 
				{ 
					TRACE((TRACE_INFO,_F_,"GetEnvironmentVariable(%s)=%d",szCopyOfValue,GetLastError()));
				}
				else
				{
					ReplaceStr(gszComputedValue,sizeof(gszComputedValue),szEnvVariableNameWithMarks,szEnvVariableValue);
				}
			}
		}
	}

	TRACE((TRACE_LEAVE,_F_, "gszComputedValue=%s",gszComputedValue));
	return gszComputedValue;
}

//-----------------------------------------------------------------------------
// LastDetect_AddOrUpdateWindow()
//-----------------------------------------------------------------------------
// ajoute ou met à jour une fenêtre dans la liste des dernières détectées
//-----------------------------------------------------------------------------
int LastDetect_AddOrUpdateWindow(HWND w,int iPopupType)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	int rc=-1;
	int i;
	for (i=0;i<MAX_NB_LAST_DETECT;i++) 
	{ 
		if (gTabLastDetect[i].wLastDetect==NULL ||	// emplacement libre
			gTabLastDetect[i].wLastDetect==w)		// fenêtre déjà connue 
		{
			gTabLastDetect[i].tag=1;
			time(&gTabLastDetect[i].tLastDetect);
			gTabLastDetect[i].wLastDetect=w;
			gTabLastDetect[i].iPopupType=iPopupType;
			rc=0;
			goto end;
		}
	}
end:
	//for (i=0;i<MAX_NB_LAST_DETECT;i++) { TRACE((TRACE_DEBUG,_F_,"[0x%08lx|%d|%ld]",gTabLastDetect[i].wLastDetect,gTabLastDetect[i].tag,gTabLastDetect[i].tLastDetect));	}

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// LastDetect_RemoveWindow()
//-----------------------------------------------------------------------------
// supprime une fenêtre dans la liste des dernières détectées
//-----------------------------------------------------------------------------
int LastDetect_RemoveWindow(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	int rc=-1;
	int i;
	for (i=0;i<MAX_NB_LAST_DETECT;i++) 
	{ 
		if (gTabLastDetect[i].wLastDetect==w)
		{
			gTabLastDetect[i].tag=0;
			gTabLastDetect[i].tLastDetect=-1;
			gTabLastDetect[i].wLastDetect=NULL;
			gTabLastDetect[i].iPopupType=POPUP_NONE;
			rc=0;
			goto end;
		}
	}
end:
	//for (i=0;i<MAX_NB_LAST_DETECT;i++) { TRACE((TRACE_DEBUG,_F_,"[0x%08lx|%d|%ld]",gTabLastDetect[i].wLastDetect,gTabLastDetect[i].tag,gTabLastDetect[i].tLastDetect));	}

	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// LastDetect_TagWindow()
//-----------------------------------------------------------------------------
// marque la fenêtre comme toujours présente
//-----------------------------------------------------------------------------
int LastDetect_TagWindow(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	int rc=-1;
	int i;

	for (i=0;i<MAX_NB_LAST_DETECT;i++) 
	{ 
		if (gTabLastDetect[i].wLastDetect==w) // fenêtre trouvée
		{
			gTabLastDetect[i].tag=1;
			rc=0;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// LastDetect_UntagAllWindows()
//-----------------------------------------------------------------------------
// détaggue toutes les fenêtres
//-----------------------------------------------------------------------------
void LastDetect_UntagAllWindows(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int i;

	for (i=0;i<MAX_NB_LAST_DETECT;i++)
	{
		gTabLastDetect[i].tag=0;
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// LastDetect_RemoveUntaggedWindows()
//-----------------------------------------------------------------------------
// efface toutes les fenêtres non tagguées
//-----------------------------------------------------------------------------
void LastDetect_RemoveUntaggedWindows(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int i,j;

	for (i=0;i<MAX_NB_LAST_DETECT;i++)
	{
		if (gTabLastDetect[i].tag==0 && gTabLastDetect[i].wLastDetect!=NULL)
		{
			TRACE((TRACE_DEBUG,_F_,"Removing : 0x%08lx",gTabLastDetect[i].wLastDetect));

			// ISSUE#187 : pour les pages web, il faut réinitialiser le tLastSSO de toutes les actions qui
			// ont été traitées dans cette fenêtre puisqu'elle a disparu.
			for (j=0;j<giNbActions;j++)
			{
				if (gptActions[j].iType==WEBSSO || gptActions[j].iType==XEBSSO ||
					(gptActions[j].iType==POPSSO && gTabLastDetect[i].iPopupType==POPUP_CHROME))
				{
					if (gptActions[j].wLastSSO==gTabLastDetect[i].wLastDetect)
					{
						TRACE((TRACE_DEBUG,_F_,"Réinitialisation du tLastSSO de l'action '%s'",gptActions[j].szApplication));
						gptActions[j].tLastSSO=-1;
					}
				}
			}
			
			gTabLastDetect[i].tLastDetect=-1;
			gTabLastDetect[i].wLastDetect=NULL;

		}
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// LastDetect_GetTime()
//-----------------------------------------------------------------------------
// retourne la date de dernière détection d'une fenêtre, -1 si pas trouvée
//-----------------------------------------------------------------------------
time_t LastDetect_GetTime(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));
	time_t rc=-1;
	int i;

	for (i=0;i<MAX_NB_LAST_DETECT;i++)
	{
		if (gTabLastDetect[i].wLastDetect==w) // trouvé la fenêtre !
		{
			rc=gTabLastDetect[i].tLastDetect;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// EnumWindowsProc()
//-----------------------------------------------------------------------------
// Callback énumération des fenêtres à exclure
//-----------------------------------------------------------------------------
static int CALLBACK EnumWindowsProc(HWND w, LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);
	char szTitre[255+1];	  // pour stockage titre de fenêtre
	
	if (!IsWindowVisible(w)) goto end; // fenêtre cachée, on passe ! <optim+compteur>
	GetWindowText(w,szTitre,sizeof(szTitre));
	if (*szTitre==0) goto end; // si fenêtre sans titre, on passe ! <optim>
	TRACE((TRACE_INFO,_F_,"Fenêtre exclue (%d) : %s (0x%08lx)",giNbExcludedHandles,szTitre,w));
	gTabExcludedHandles[giNbExcludedHandles]=w;
	giNbExcludedHandles++;
end:
	return TRUE;
}

//-----------------------------------------------------------------------------
// ExcludeOpenWindows()
//-----------------------------------------------------------------------------
// Exclut toutes les fenêtres ouvertes à l'écran
//-----------------------------------------------------------------------------
void ExcludeOpenWindows(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	giNbExcludedHandles=0;
	EnumWindows(EnumWindowsProc,0);
	TRACE((TRACE_INFO,_F_,"giNbExcludedHandles=%d",giNbExcludedHandles));
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// IsExcluded()
//-----------------------------------------------------------------------------
// TRUE si fenêtre exclue, FALSE sinon
//-----------------------------------------------------------------------------
BOOL IsExcluded(HWND w)
{
	// TROP VERBEUX TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	int i;
	for (i=0;i<giNbExcludedHandles;i++)
	{
		if (gTabExcludedHandles[i]==w) 
		{ 
			TRACE((TRACE_DEBUG,_F_,"Fenêtre 0x%08lx exclue",w));
			rc=TRUE; goto end; 
		}
	}
end:
	// TROP VERBEUX TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swPipeWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swPipeWrite(char *bufRequest,int lenRequest,char *bufResponse,DWORD sizeofBufResponse,DWORD *pdwLenResponse)
{
	HANDLE hPipe=INVALID_HANDLE_VALUE;
	int rc=-1;
	DWORD cbWritten;
	DWORD dwLastError;

	TRACE((TRACE_ENTER,_F_,""));
	
	// Ouverture du pipe créé par le service swsso
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	if (hPipe==INVALID_HANDLE_VALUE)
	{
		// Parfois entre deux appels le pipe n'est pas dispo tout de suite, donc on attend un peu
		dwLastError=GetLastError();
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%ld",dwLastError));
		if (dwLastError!=ERROR_PIPE_BUSY) goto end;
		for (int i=0;i<10;i++) // on attend 10 secondes max, sinon tant pis
		{
			TRACE((TRACE_DEBUG,_F_,"WaitNamedPipe()"));
			if (WaitNamedPipe("\\\\.\\pipe\\swsso",1000)) // attend 1 seconde, si OK retente l'ouverture
			{
				hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
				break;
			}
		}
		if (hPipe==INVALID_HANDLE_VALUE)
		{
			TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%ld",dwLastError));
			goto end;
		}
	}
	
	// Envoi de la requête
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,lenRequest,"Request to write"));
	if (!WriteFile(hPipe,bufRequest,lenRequest,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenRequest,GetLastError()));
		goto end;
	} 

	// Lecture la réponse
	if (!ReadFile(hPipe,bufResponse,sizeofBufResponse,pdwLenResponse,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeofBufResponse,GetLastError()));
		goto end;
	}  
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,*pdwLenResponse,"Response"));

	rc=0;
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

typedef struct
{
	int iPopupType;
	char *pszCompare;
	BOOL bFound;
} T_ENUM_BROWSER;

//-----------------------------------------------------------------------------
// EnumBrowserProc()
//-----------------------------------------------------------------------------
// Callback d'énumération de fenêtres présentes sur le bureau et déclenchement
// du SSO le cas échéant
//-----------------------------------------------------------------------------
// [rc] : toujours TRUE (continuer l'énumération)
//-----------------------------------------------------------------------------
static int CALLBACK EnumBrowserProc(HWND w, LPARAM lp)
{
	int rc=TRUE; // true=continuer l'énumération
	char szClassName[128+1]; // pour stockage nom de classe de la fenêtre
	char *pszURL=NULL;
	T_ENUM_BROWSER *pEnumBrowser=(T_ENUM_BROWSER*)lp;
	
	GetClassName(w,szClassName,sizeof(szClassName));
	if ((strcmp(szClassName,"IEFrame")==0 || strcmp(szClassName,"ApplicationFrameWindow")==0) && (pEnumBrowser->iPopupType==POPUP_W7 || pEnumBrowser->iPopupType==POPUP_XP)) // IE
	{
		pszURL=GetIEURL(w,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL IE non trouvee : on passe !")); goto end; }
	}
	else if ((strcmp(szClassName,gcszMozillaUIClassName)==0) && (pEnumBrowser->iPopupType==POPUP_FIREFOX)) // FF3
	{
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX3,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 3- non trouvee : on passe !")); goto end; }
	}
	else if ((strcmp(szClassName,gcszMozillaClassName)==0) && (pEnumBrowser->iPopupType==POPUP_FIREFOX)) // FF4
	{
		pszURL=GetFirefoxURL(w,NULL,FALSE,NULL,BROWSER_FIREFOX4,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 4+ non trouvee : on passe !")); goto end; }
	}
	else if ((strncmp(szClassName,"Chrome_WidgetWin_",17)==0)  && (pEnumBrowser->iPopupType==POPUP_CHROME)) // Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	{
		pszURL=GetChromeURL(w);
		if (pszURL==NULL) pszURL=GetChromeURL51(w); // ISSUE#282
		if (pszURL==NULL) pszURL=NewGetChromeURL(w,NULL,FALSE,NULL); // ISSUE#314
		if (gpAccessibleChromeURL!=NULL) { gpAccessibleChromeURL->Release(); gpAccessibleChromeURL=NULL; }
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Chrome non trouvee : on passe !")); goto end; }
	}
	if (pszURL!=NULL) // on a trouvé un navigateur du même type que la popup et on a réussi à lire l'URL, on compare avec l'URL recherchée !
	{
		TRACE((TRACE_INFO,_F_,"URLBar trouvee  = %s",pszURL));
		TRACE((TRACE_INFO,_F_,"URLBar attendue = %s",pEnumBrowser->pszCompare));
		if (swURLMatch(pEnumBrowser->pszCompare,pszURL)) 
		{
			rc=FALSE; // on a trouvé, on arrête l'énumération !
			pEnumBrowser->bFound=TRUE;
		}
	}
end:
	if (pszURL!=NULL) free(pszURL);
	return rc;	
}
//-----------------------------------------------------------------------------
// swCheckBrowserURL() 
//-----------------------------------------------------------------------------
// Vérifie si on trouve un navigateur ouvert du type de la popup et dont l'URL
// matche avec l'URL fournie dans pszCompare
//-----------------------------------------------------------------------------
int swCheckBrowserURL(int iPopupType,char *pszCompare)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	T_ENUM_BROWSER tEnumBrowser;

	tEnumBrowser.bFound=FALSE;
	tEnumBrowser.iPopupType=iPopupType;
	tEnumBrowser.pszCompare=pszCompare;

	// recherche le navigateur ouvert correspondant au type de popup (pas génial, mais on ne peut pas retrouver la fenêtre du navigateur autrement...)
	EnumWindows(EnumBrowserProc,(LPARAM)&tEnumBrowser);
	if (tEnumBrowser.bFound)
	{
		rc=0;
	}

//end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetNbActiveApps() 
//-----------------------------------------------------------------------------
// Retourne le nb de configurations actives
//-----------------------------------------------------------------------------
void GetNbActiveApps(int *piNbActiveApps,int *piNbActiveAppsFromServer)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int i;
	*piNbActiveApps=0;
	*piNbActiveAppsFromServer=0;
	for (i=0;i<giNbActions;i++)
	{
		if (gptActions[i].bActive) 
		{
			(*piNbActiveApps)++;
			if (gptActions[i].iConfigId!=0)
			{
				(*piNbActiveAppsFromServer)++;
			}
		}
	}
	TRACE((TRACE_LEAVE,_F_, "iNbActiveApps=%d iNbActiveAppsFromServer=%d",*piNbActiveApps,*piNbActiveAppsFromServer));
}

//-----------------------------------------------------------------------------
// RevealPasswordField()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void RevealPasswordField(HWND w,BOOL bReveal)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szPwd[LEN_PWD+1];
	
	ShowWindow(GetDlgItem(w,TB_PWD),bReveal?SW_HIDE:SW_SHOW);
	ShowWindow(GetDlgItem(w,TB_PWD_CLEAR),bReveal?SW_SHOW:SW_HIDE);
	GetDlgItemText(w,bReveal?TB_PWD:TB_PWD_CLEAR,szPwd,sizeof(szPwd));
	SetDlgItemText(w,bReveal?TB_PWD_CLEAR:TB_PWD,szPwd);
	SecureZeroMemory(szPwd,sizeof(szPwd));

	// un ptit coup de refresh sur les contrôles
	RECT rect;
	GetClientRect(GetDlgItem(w,TB_PWD),&rect);
	InvalidateRect(GetDlgItem(w,TB_PWD),&rect,FALSE);
	GetClientRect(GetDlgItem(w,TB_PWD_CLEAR),&rect);
	InvalidateRect(GetDlgItem(w,TB_PWD_CLEAR),&rect,FALSE);

	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ClipboardCopy()
//-----------------------------------------------------------------------------
// Copie la chaine passée en paramètre dans le presse papier
//-----------------------------------------------------------------------------
void ClipboardCopy(char *sz)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int len=strlen(sz)+1;
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), sz, len);
	GlobalUnlock(hMem);
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ClipboardDelete()
//-----------------------------------------------------------------------------
// Vide le presse papier
//-----------------------------------------------------------------------------
void ClipboardDelete()
{
	TRACE((TRACE_ENTER,_F_, ""));
	OpenClipboard(NULL);
	EmptyClipboard();
	CloseClipboard();
	TRACE((TRACE_LEAVE,_F_, ""));
}


//-----------------------------------------------------------------------------
// ExpandFileName()
//-----------------------------------------------------------------------------
// Expande les variables d'environnement dans les noms de fichier
//-----------------------------------------------------------------------------
int ExpandFileName(char *szInFileName,char *szOutFileName, int iBufSize)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szTmpFileName[_MAX_PATH+1];
	int iPos=0;
	int len;

	// si chaine vide, on sort avec chaine vide
	if (*szInFileName==0) { *szOutFileName=0; rc=0; goto end; }

	// on commence par enlever les éventuels guillemets de début et fin de chaine
	if (*szInFileName=='"') iPos=1;
	strcpy_s(szTmpFileName,_MAX_PATH+1,szInFileName+iPos);
	len=strlen(szTmpFileName);
	if (len>1 && szTmpFileName[len-1]=='"') szTmpFileName[len-1]=0;

	// on expande les variables d'environnement
	if (ExpandEnvironmentStrings(szTmpFileName,szOutFileName,iBufSize)==0)
	{
		TRACE((TRACE_ERROR,_F_,"ExpandEnvironmentStrings(%s)=%d",szTmpFileName,GetLastError()));
		goto end;
	}

	TRACE((TRACE_DEBUG,_F_,"ExpandEnvironmentStrings(%s)=%s",szTmpFileName,szOutFileName));
	rc=0;
	
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// GetIniHash()
//-----------------------------------------------------------------------------
// Retourne un hash du fichier .ini
//-----------------------------------------------------------------------------
int GetIniHash(unsigned char *pBufHashValue)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HANDLE hf=INVALID_HANDLE_VALUE;
	DWORD dwFileSize;
	DWORD dwByteRead;
	char *pBytesToHash=NULL;
	HCRYPTHASH hHash=NULL;
	DWORD lenHash;
	int rc=-1;
	
	// ouvre le .ini en lecture
	hf=CreateFile(gszCfgFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) {	TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",gszCfgFile,GetLastError())); goto end;	}
	// regarde la taille et alloue le buffer pour la lecture
	dwFileSize=GetFileSize(hf,NULL);
	if (dwFileSize==INVALID_FILE_SIZE) { TRACE((TRACE_ERROR,_F_,"GetFileSize(%s)=INVALID_FILE_SIZE",gszCfgFile)); goto end;	}
	pBytesToHash=(char*)malloc(32+dwFileSize);
	if (pBytesToHash==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",32+dwFileSize)); goto end;	}
	// sel
	memcpy(pBytesToHash,gcszK1,8);
	memcpy(pBytesToHash+8,gcszK2,8);
	memcpy(pBytesToHash+16,gcszK3,8);
	memcpy(pBytesToHash+24,gcszK4,8);

	// TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pBytesToHash,32,"sel"));

	// lit le fichier complet
	if (!ReadFile(hf,pBytesToHash+32,dwFileSize,&dwByteRead,NULL)) {	TRACE((TRACE_ERROR,_F_,"ReadFile(%s)=0x%08lx",gszCfgFile,GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"ReadFile(%s) : %ld octets lus",gszCfgFile,dwByteRead));

	// TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pBytesToHash+32,dwFileSize,"fichier"));
	// TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pBytesToHash,dwFileSize+32,"bloc à hasher"));

	// ferme le fichier
	CloseHandle(hf); hf=INVALID_HANDLE_VALUE;
	// calcul du hash
	if (!CryptCreateHash(ghProv,CALG_SHA1,0,0,&hHash)) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash(CALG_SHA1)=0x%08lx",GetLastError())); goto end; }
	if (!CryptHashData(hHash,(unsigned char*)pBytesToHash,32+dwFileSize,0)) { TRACE((TRACE_ERROR,_F_,"CryptHashData()=0x%08lx",GetLastError())); goto end; }
	lenHash=HASH_LEN;
	if (!CryptGetHashParam(hHash,HP_HASHVAL,pBufHashValue,&lenHash,0)) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)=0x%08lx",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pBufHashValue,lenHash,"hash"));
	rc=0;
	
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf);
	if (pBytesToHash!=NULL) free(pBytesToHash);
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// StoreIniEncryptedHash()
//-----------------------------------------------------------------------------
// Stocke le hash chiffré dans le fichier swsso.chk
//-----------------------------------------------------------------------------
int StoreIniEncryptedHash(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HANDLE hf=INVALID_HANDLE_VALUE; 
	char szFilename[_MAX_PATH+1];
	char *p;
	DWORD dw;
	unsigned char bufHashValue[HASH_LEN];

	if (!gbCheckIniIntegrity) { rc=0; goto end; }

	// calcule le hash 
	if (GetIniHash(bufHashValue)!=0) goto end;

	// nom du fichier : swsso.ini -> swsso.chk
	strcpy_s(szFilename,sizeof(szFilename),gszCfgFile);
	p=strrchr(szFilename,'.');
	if (p==NULL) { TRACE((TRACE_ERROR,_F_,"gszCfgFile=%s",gszCfgFile)); goto end; }
	memcpy(p+1,"chk",4);

	// écrit le hash chiffré dans le fichier chk
	hf=CreateFile(szFilename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFilename,GetLastError())); goto end; }
	if (!WriteFile(hf,bufHashValue,HASH_LEN,&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"WriteFile(%s,%ld)=%d",szFilename,HASH_LEN,GetLastError())); goto end; }

	rc=0;
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// CheckIniHash()
//-----------------------------------------------------------------------------
// Vérifie l'intégrité du fichier .ini (via le hash chiffré du fichier .chk)
//-----------------------------------------------------------------------------
int CheckIniHash(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HANDLE hf=INVALID_HANDLE_VALUE; 
	char szFilename[_MAX_PATH+1];
	unsigned char bufHashValueFichierChk[HASH_LEN];
	unsigned char bufHashValueCalcule[HASH_LEN];
	char *p;
	DWORD dw;

	if (!gbCheckIniIntegrity) { rc=0; goto end; }
	
	// nom du fichier : swsso.ini -> swsso.chk
	strcpy_s(szFilename,sizeof(szFilename),gszCfgFile);
	p=strrchr(szFilename,'.');
	if (p==NULL) { TRACE((TRACE_ERROR,_F_,"gszCfgFile=%s",gszCfgFile)); goto end; }
	memcpy(p+1,"chk",4);
		
	// lit le hash chiffré dans le fichier swsso.chk
	hf=CreateFile(szFilename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFilename,GetLastError())); goto end; }
	if (!ReadFile(hf,bufHashValueFichierChk,HASH_LEN,&dw,NULL)) { TRACE((TRACE_ERROR,_F_,"ReadFile(%s)",gszCfgFile)); goto end; }

	// calcule le hash chiffré du fichier .ini
	if (GetIniHash(bufHashValueCalcule)!=0) goto end;
	
	// compare
	if (memcmp(bufHashValueCalcule,bufHashValueFichierChk,HASH_LEN)!=0)
	{
		TRACE_BUFFER((TRACE_ERROR,_F_,bufHashValueCalcule,HASH_LEN,   "Hash calculé :"));
		TRACE_BUFFER((TRACE_ERROR,_F_,bufHashValueFichierChk,HASH_LEN,"Hash fichier :"));
		goto end;
	}
	
	rc=0;
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// CheckIfQuitMessage()
//-----------------------------------------------------------------------------
// Vérifie si reçu un message d'arrêt. A appeler dans toutes les DialogProc
//-----------------------------------------------------------------------------
BOOL CheckIfQuitMessage(UINT msg)
{
	BOOL rc=FALSE;
	if (msg==guiStandardQuitMsg && !gbAdmin) // ISSUE#227
	{
		TRACE((TRACE_INFO,_F_,"Message recu : swsso-quit (0x%08lx)",guiStandardQuitMsg));
		rc=TRUE;
	}
	else if (msg==guiAdminQuitMsg && gbAdmin) // ISSUE#239
	{
		TRACE((TRACE_INFO,_F_,"Message recu : swssoadmin-quit (0x%08lx)",guiAdminQuitMsg));
		rc=TRUE;
	}
	
	if (rc) PostMessage(gwMain,WM_COMMAND,MAKEWORD(TRAY_MENU_QUITTER,0),0);

	return rc;
}

//-----------------------------------------------------------------------------
// KillswSSO()
//-----------------------------------------------------------------------------
// Tue le process swSSO si toujours présent
//-----------------------------------------------------------------------------
void KillswSSO(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	char szCurrentProcessFullPathName[MAX_PATH];
	DWORD dwCurrentProcessId=GetCurrentProcessId();
	HANDLE hProcessSnap=INVALID_HANDLE_VALUE;
	HANDLE hProcess=NULL;
	PROCESSENTRY32 pe32;

	TRACE((TRACE_INFO,_F_,"GetCurrentProcessId()=%ld",dwCurrentProcessId));

	hProcessSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if(hProcessSnap==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateToolhelp32Snapshot() KO"));
		goto end;
	}
	// on fait un premier tour pour déterminer le nom du process courant
	pe32.dwSize=sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap,&pe32))
	{
		TRACE((TRACE_ERROR,_F_,"Process32First() KO"));
		goto end;
	}
	do
	{
		TRACE((TRACE_DEBUG,_F_,"Process : %s (pid=%ld)",pe32.szExeFile,pe32.th32ProcessID));
		if (pe32.th32ProcessID==dwCurrentProcessId)
		{
			strcpy_s(szCurrentProcessFullPathName,sizeof(szCurrentProcessFullPathName),pe32.szExeFile);
			TRACE((TRACE_INFO,_F_,"Trouvé ! Process : %s (pid=%ld)",szCurrentProcessFullPathName,dwCurrentProcessId));
			goto suite;
		}
	} while (Process32Next(hProcessSnap,&pe32));
suite:
	// on fait un second tour pour tuer tous les process qui ont le même nom que le process courant
	pe32.dwSize=sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap,&pe32))
	{
		TRACE((TRACE_ERROR,_F_,"Process32First() KO"));
		goto end;
	}
	do
	{
		TRACE((TRACE_DEBUG,_F_,"Process : %s (pid=%ld)",pe32.szExeFile,pe32.th32ProcessID));
		if (_stricmp(szCurrentProcessFullPathName,pe32.szExeFile)==0 && pe32.th32ProcessID!=dwCurrentProcessId)
		{
			TRACE((TRACE_INFO,_F_,"Trouvé on le tue (pid=%ld) !",pe32.th32ProcessID));
			hProcess=OpenProcess(PROCESS_TERMINATE,FALSE,pe32.th32ProcessID);
			if (hProcess!=NULL)
			{
				TerminateProcess(hProcess,0);
				CloseHandle(hProcess);
				hProcess=NULL;
				// remarque : on continue pour tuer tous les swSSO.exe lancés
			}
		}
	} while (Process32Next(hProcessSnap,&pe32));


end:
  if (hProcessSnap!=INVALID_HANDLE_VALUE) CloseHandle(hProcessSnap);
  if (hProcess!=NULL) CloseHandle(hProcess);
	TRACE((TRACE_LEAVE,_F_, ""));
}

const char gcszUpperCase[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char gcszLowerCase[]="abcdefghijklmnopqrstuvwxyz";
const char gcszNumbers[]="1234567890";
const char gcszSpecialChars[]="&'(-_)=+}]{[,?;.:/!*$";

//-----------------------------------------------------------------------------
// GenerateNewPassword()
//-----------------------------------------------------------------------------
// Génère un nouveau mot de passe en fonction de la politique configurée en base de registre
//-----------------------------------------------------------------------------
// [in] pszPolicy : chaine de type %RANDOMxx%, où xx est la référence de la politique de mot de passe en base de registre
// [out] pszNewPassword
//-----------------------------------------------------------------------------
int GenerateNewPassword(char *pszNewPassword,char *pszPolicy)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szPolicyId[2+1];
	int iPolicyId=0;
	char szCharSet[100]="";
	int tiCountCharSet[100];
	int iLenCharSet=0;
	int i;
	BYTE alea;
	int iPwdLen;
	BYTE *pBuf=NULL;
	BOOL bNewPasswordCompliant=FALSE;
	int iIndexCharSet;
	int iNbConsecutiveChars;

	// Récupère l'identifiant de la politique de mot de passe à appliquer
	if (strlen(pszPolicy)!=strlen("%RANDOMxx%")) { TRACE((TRACE_ERROR,_F_,"Format politique incorrect : %s",pszPolicy)); goto end; }
	szPolicyId[0]=pszPolicy[7];
	szPolicyId[1]=pszPolicy[8];
	szPolicyId[2]=0;
	iPolicyId=atoi(szPolicyId);
	if (iPolicyId<=0) { TRACE((TRACE_ERROR,_F_,"Identifiant politique incorrect : %s",szPolicyId)); goto end; }
	// Vérifie que la politique est bien renseignée en base de registre
	if (!gptNewPasswordPolicies[iPolicyId].isDefined) { TRACE((TRACE_ERROR,_F_,"Politique %d non configurée en base de registre",szPolicyId)); goto end; }
	// Constitue le jeu de caractères
	if (gptNewPasswordPolicies[iPolicyId].MinUpperCase>0) strcat_s(szCharSet,sizeof(szCharSet),gcszUpperCase); 
	if (gptNewPasswordPolicies[iPolicyId].MinLowerCase>0) strcat_s(szCharSet,sizeof(szCharSet),gcszLowerCase); 
	if (gptNewPasswordPolicies[iPolicyId].MinNumbers>0) strcat_s(szCharSet,sizeof(szCharSet),gcszNumbers); 
	if (gptNewPasswordPolicies[iPolicyId].MinSpecialChars>0) strcat_s(szCharSet,sizeof(szCharSet),gcszSpecialChars);
	iLenCharSet=strlen(szCharSet);
	TRACE((TRACE_DEBUG,_F_,"szCharSet=%s",szCharSet));
	// Tire au sort la longueur du mot de passe
	if (!CryptGenRandom(ghProv,1,&alea)) { TRACE((TRACE_ERROR,_F_,"CryptGenRandom()=0x%08lx",GetLastError())); goto end; }
	iPwdLen=gptNewPasswordPolicies[iPolicyId].MinLength+(alea%(gptNewPasswordPolicies[iPolicyId].MaxLength-gptNewPasswordPolicies[iPolicyId].MinLength+1));
	TRACE((TRACE_DEBUG,_F_,"iPwdLen=%d",iPwdLen));
	// Construit le mot de passe
	pBuf=(BYTE*)malloc(iPwdLen);
	if (pBuf==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iPwdLen)); goto end; }
	// réessaie jusqu'à trouver un mot de passe conforme à la politique
	while (!bNewPasswordCompliant)
	{
		// génère l'aléa
		if (!CryptGenRandom(ghProv,iPwdLen,pBuf)) { TRACE((TRACE_ERROR,_F_,"CryptGenRandom()=0x%08lx",GetLastError())); goto end; }
		// reset des compteurs de doublons
		SecureZeroMemory(&tiCountCharSet,sizeof(tiCountCharSet));
		iNbConsecutiveChars=0;
		// remplit le mot de passe
		for (i=0;i<iPwdLen;i++)	
		{
			iIndexCharSet=pBuf[i]%iLenCharSet;
			// vérifie que le nombre de caractères identiques consécutifs n'est pas dépassé
			if (gptNewPasswordPolicies[iPolicyId].MaxConsecutiveCommonChars>0)
			{
				if (i>0 && pszNewPassword[i-1]==szCharSet[iIndexCharSet])
				{
					iNbConsecutiveChars++;
					if (iNbConsecutiveChars>gptNewPasswordPolicies[iPolicyId].MaxConsecutiveCommonChars)
					{
						iIndexCharSet++;
						if (iIndexCharSet==iLenCharSet) iIndexCharSet=0;
					}			
				}
			}
			// vérifie que le nombre total de caractères identiques n'est pas dépassé
			if (gptNewPasswordPolicies[iPolicyId].MaxCommonChars>0)
			{
				while (tiCountCharSet[iIndexCharSet] >= gptNewPasswordPolicies[iPolicyId].MaxCommonChars) 
				{
					iIndexCharSet++;
					if (iIndexCharSet==iLenCharSet) iIndexCharSet=0;
				}
				tiCountCharSet[iIndexCharSet]++;
			}
			pszNewPassword[i]=szCharSet[iIndexCharSet];
		}
		pszNewPassword[iPwdLen]=0;
		// vérifie la conformité à la politique
		bNewPasswordCompliant=TRUE;
		if (GetNbCharsInString(pszNewPassword,SEARCHTYPE_UPPERCASE)<gptNewPasswordPolicies[iPolicyId].MinUpperCase) bNewPasswordCompliant=FALSE;
		if (GetNbCharsInString(pszNewPassword,SEARCHTYPE_LOWERCASE)<gptNewPasswordPolicies[iPolicyId].MinLowerCase) bNewPasswordCompliant=FALSE;
		if (GetNbCharsInString(pszNewPassword,SEARCHTYPE_NUMBERS)<gptNewPasswordPolicies[iPolicyId].MinNumbers) bNewPasswordCompliant=FALSE;
		if (GetNbCharsInString(pszNewPassword,SEARCHTYPE_SPECIALCHARS)<gptNewPasswordPolicies[iPolicyId].MinSpecialChars) bNewPasswordCompliant=FALSE;
		if (gptNewPasswordPolicies[iPolicyId].IdMaxCommonChars>0)
		{
			if (!CheckCommonChars(pszNewPassword,gszUserName,gptNewPasswordPolicies[iPolicyId].IdMaxCommonChars)) bNewPasswordCompliant=FALSE;
		}
	}
	rc=0;
end:
	if (pBuf!=NULL) free(pBuf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
