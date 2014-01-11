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
// swTools.cpp
//-----------------------------------------------------------------------------
// Utilitaires
//-----------------------------------------------------------------------------

#include "stdafx.h"
#define HTTP_RESULT_MAX_SIZE 512000
// estimation : moyenne 1 Ko par config / 500 configs => 512 Ko

char gszRes[512];
WCHAR gwcTmp1_512[512+1];
WCHAR gwcTmp2_512[512+1];
char gszComputedValue[256+1];

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
	TRACE((TRACE_ENTER,_F_, "%s",sz));
	WCHAR *pwc=NULL;
	int rc,sizeofpwc;
	BSTR bstr=NULL;

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
	TRACE((TRACE_LEAVE,_F_, "%S",bstr));
	return bstr;
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
// HTTPRequest
// ----------------------------------------------------------------------------------
// Exécute la requête HTTP passée en paramètre
// L'appelant doit libérer le resultat !
// ----------------------------------------------------------------------------------
char *HTTPRequest(const char *szRequest,int timeout,T_PROXYPARAMS *pInProxyParams)
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

	// 0.89 : si pas de paramètres proxy reçus, utilise les valeurs globales
	if (pInProxyParams==NULL)
	{
		pProxyParams=(T_PROXYPARAMS *)malloc(sizeof(T_PROXYPARAMS));
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
		MultiByteToWideChar(CP_ACP,0,pProxyParams->szProxyURL,-1,gwcTmp1_512,sizeof(gwcTmp1_512)-1);
		hSession = WinHttpOpen(L"swsso.exe",WINHTTP_ACCESS_TYPE_NAMED_PROXY,gwcTmp1_512,WINHTTP_NO_PROXY_BYPASS, 0);
	}
	else
	{
		TRACE((TRACE_INFO,_F_,"Proxy:PAS DE PROXY")); 
		hSession = WinHttpOpen(L"swsso.exe",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS, 0);
	}
	if (hSession==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpOpen(proxy:%s)",pProxyParams->szProxyURL)); goto end; }
    
	WinHttpSetTimeouts(hSession, timeout*1000, timeout*1000, timeout*1000, timeout*1000); 

	// WinHttpConnect
	MultiByteToWideChar(CP_ACP,0,gszServerAddress,-1,gwcTmp1_512,sizeof(gwcTmp1_512)-1);
    hConnect = WinHttpConnect(hSession,gwcTmp1_512,INTERNET_DEFAULT_HTTP_PORT, 0);
	if (hConnect==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpConnect(%s)",gszServerAddress)); goto end; }
    
	// WinHttpOpenRequest
	MultiByteToWideChar(CP_ACP,0,szRequest,-1,gwcTmp1_512,sizeof(gwcTmp1_512)-1);
    hRequest = WinHttpOpenRequest( hConnect,L"GET",gwcTmp1_512,NULL, WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,0);
	TRACE((TRACE_INFO,_F_,"WinHttpOpenRequest(GET %s%s)",gszServerAddress,szRequest)); 
	if (hRequest==NULL) { TRACE((TRACE_ERROR,_F_,"WinHttpOpenRequest(GET %s)",szRequest)); goto end; }

	if (*(pProxyParams->szProxyUser)!=0)
	{
		MultiByteToWideChar(CP_ACP,0,pProxyParams->szProxyUser,-1,gwcTmp1_512,sizeof(gwcTmp1_512)-1);
		MultiByteToWideChar(CP_ACP,0,pProxyParams->szProxyPwd,-1,gwcTmp2_512,sizeof(gwcTmp2_512)-1);
		brc=WinHttpSetCredentials(hRequest,WINHTTP_AUTH_TARGET_PROXY,WINHTTP_AUTH_SCHEME_BASIC,gwcTmp1_512,gwcTmp2_512,NULL);
		TRACE((TRACE_INFO,_F_,"WinHttpSetCredentials(user:%s)",pProxyParams->szProxyUser)); 
		if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpSetCredentials()")); goto end; }
	}

	brc = WinHttpSendRequest(hRequest,WINHTTP_NO_ADDITIONAL_HEADERS, 0,WINHTTP_NO_REQUEST_DATA,0,0,0);
	if (!brc) { swLogEvent(EVENTLOG_ERROR_TYPE,MSG_SERVER_NOT_RESPONDING,gszServerAddress,(char*)szRequest,NULL,0); TRACE((TRACE_ERROR,_F_,"WinHttpSendRequest()")); goto end; }
 
    brc = WinHttpReceiveResponse(hRequest, NULL);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpReceiveResponse()")); goto end; }

	pszResult=(char*)malloc(HTTP_RESULT_MAX_SIZE);
	TRACE((TRACE_DEBUG,_F_,"pszResult=0x%08lx",pszResult));
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(HTTP_RESULT_MAX_SIZE)")); goto end; }
	ZeroMemory(pszResult,HTTP_RESULT_MAX_SIZE);
	p=pszResult;
    do 
	{
        dwSize = 0;
        brc=WinHttpQueryDataAvailable(hRequest,&dwSize);
		TRACE((TRACE_DEBUG,_F_,"WinHttpQueryDataAvailable=%ld",dwSize));
		if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpQueryDataAvailable()")); goto end; }
		// TODO ajouter un controle...
		if (p+dwSize > pszResult+HTTP_RESULT_MAX_SIZE) { TRACE((TRACE_ERROR,_F_,"Buf too small (dwSize=%ld)",dwSize)); goto end; }
        brc=WinHttpReadData(hRequest,p, dwSize, &dwDownloaded);
        if (!brc) { TRACE((TRACE_ERROR,_F_,"WinHttpReadData()")); goto end; }
		p+=dwSize;
		dwLenResult+=dwDownloaded;
    } while (dwSize>0);
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
	TRACE((TRACE_LEAVE,_F_, "pszResult=0x%08lx",pszResult));
	return pszResult;
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
	TRACE((TRACE_LEAVE,_F_, "rc=%s",pszEncoded));
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
	while(rc!=0 && *w!=NULL)
	{
		if (IsWindowVisible(*w))
		{
			GetWindowText(*w,szTitle,sizeofTitle);
			if (*szTitle!=0) // si titre non vide, le compare à la liste des fenêtres exclues (#110)
			{
				rc=0;
				if (strcmp(szTitle,"swSSO - Lanceur d'applications")==0) // fenêtre exclue
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
				if (rc==0) 
				{
					for (i=0;i<giNbExcludedWindows;i++)
					{
						if (_stricmp(szTitle,gtabszExcludedWindows[i])==0) // fenêtre exclue
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
		if (bWriteIfNotFound) WritePrivateProfileString(szSection,szItem,bDefault?"YES":"NO",gszCfgFile);
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
void Help(void){
	TRACE((TRACE_ENTER,_F_, ""));
	char szHelpFile[_MAX_PATH+1];
	int len;
	int rc;
	BOOL bFound=FALSE;

	// construit le nom du fichier chm
	len=GetCurrentDirectory(_MAX_PATH-10,szHelpFile);
	if (len==0) goto end;
	if (szHelpFile[len-1]!='\\')
	{
		szHelpFile[len]='\\';
		len++;
	}
	// 1er essai : swsso.chm
	strcpy_s(szHelpFile+len,_MAX_PATH+1,"swSSO.chm");
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
		strcpy_s(szHelpFile+len,_MAX_PATH+1,"swSSO.pdf");
		rc=(int)ShellExecute(NULL,"open",szHelpFile,NULL,"",SW_SHOW);
		TRACE((TRACE_INFO,_F_,"ShellExecute(%s)=%d",szHelpFile,rc));
		if (rc>32) bFound=TRUE;
	}
	if (!bFound)
	{
		// 4ème essai : pointe vers le site web
		rc=(int)ShellExecute(NULL,"open","http://www.swsso.fr/index.php?option=com_content&view=category&layout=blog&id=7&Itemid=7",NULL,"",SW_SHOW );
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
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
				T_MESSAGEBOX3B_PARAMS *pParams=(T_MESSAGEBOX3B_PARAMS *)lp;
				// icone ALT-TAB
				SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
				SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
				// titre en gras
				SetTextBold(w,TX_SUBTITLE);
				// libellés 
				SetDlgItemText(w,TX_SUBTITLE,pParams->szSubTitle);
				SetDlgItemText(w,TX_MESSAGE,pParams->szMessage);
				SetDlgItemText(w,PB_B1,GetString(pParams->iB1String));
				SetDlgItemText(w,PB_B2,GetString(pParams->iB2String));
				SetDlgItemText(w,IDCANCEL,GetString(pParams->iB3String));
				SetWindowText(w,GetString(pParams->iTitleString));
				SendDlgItemMessage(w,STATIC_ICONE,STM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIcon(NULL,pParams->szIcone));
				MACRO_SET_SEPARATOR;
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
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w);
			rc=TRUE;
			break;
	}
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

	rc=DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_MESSAGEBOX3B),pParams->wParent,MessageBox3BDialogProc,(LPARAM)pParams);

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
void DrawLogoBar(HWND w)
{
	TRACE((TRACE_ENTER,_F_, "w=0x%08lx",w));

    PAINTSTRUCT ps;
	RECT rect;
	HDC dc=NULL;

	if (!GetClientRect(w,&rect)) { TRACE((TRACE_ERROR,_F_,"GetClientRect(0x%08lx)=%ld",w,GetLastError()));  goto end; }
	dc=BeginPaint(w,&ps);
	if (dc==NULL) { TRACE((TRACE_ERROR,_F_,"BeginPaint()=%ld",GetLastError())); goto end;}
	DrawBitmap(ghLogoFondBlanc,dc,0,0,60,50);
	if (!BitBlt(dc,60,0,rect.right-60,50,0,0,0,WHITENESS)) { TRACE((TRACE_ERROR,_F_,"BitBlt(WHITENESS)=%ld",GetLastError())); }
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
	TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));

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
			KBSim(FALSE,0,GetComputedValue(szId1),FALSE);
			p+=strlen("[ID]");
		}
		else if (_strnicmp(p,"[ID2]",strlen("[ID2]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[I2]=%s",szId2));
			KBSim(FALSE,0,GetComputedValue(szId2),FALSE);
			p+=strlen("[ID2]");
		}
		else if (_strnicmp(p,"[ID3]",strlen("[ID3]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ID3]=%s",szId3));
			KBSim(FALSE,0,GetComputedValue(szId3),FALSE);
			p+=strlen("[ID3]");
		}
		else if (_strnicmp(p,"[ID4]",strlen("[ID4]"))==0) 
		{ 	
			TRACE((TRACE_DEBUG,_F_,"[ID4]=%s",szId4));
			KBSim(FALSE,0,GetComputedValue(szId4),FALSE);
			p+=strlen("[ID4]");
		}
		else if (_strnicmp(p,"[PWD]",strlen("[PWD]"))==0) 
		{ 	
			TRACE((TRACE_PWD,_F_,"[PWD]=%s",szPwd));
			KBSim(FALSE,0,szPwd,TRUE);
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
			KBSim(FALSE,0,sz,FALSE);
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
//-----------------------------------------------------------------------------
BOOL swStringMatch(char *szToBeCompared,char *szPattern)
{
	//TROP VERBEUX ! TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	int lenToBeCompared,lenPattern;
	
	//TROP VERBEUX ! TRACE((TRACE_DEBUG,_F_, "szToBeCompared=%s szPattern=%s",szToBeCompared,szPattern));
	if (szToBeCompared==NULL || szPattern==NULL) goto end;
	lenToBeCompared=strlen(szToBeCompared);
	lenPattern=strlen(szPattern);
	if (lenToBeCompared==0 || lenPattern==0) goto end; // chaines vides ne matchent jamais... à revoir peut-être.
	
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
	else 
	{
		rc=(_stricmp(szToBeCompared,szPattern)==0);
	}
end:
	//TROP VERBEUX ! TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// swURLMatch()
//-----------------------------------------------------------------------------
// Match URL
// 0.93B6 | ISSUE#43 : pour compatibilité Chrom tente le matching avec/sans http:// 
//-----------------------------------------------------------------------------
BOOL swURLMatch(char *szToBeCompared,char *szPattern)
{
	TRACE((TRACE_ENTER,_F_, "szToBeCompared=%s szPattern=%s",szToBeCompared,szPattern));
	BOOL rc=FALSE;
	int lenPattern,lenToBeCompared;

	if (swStringMatch(szToBeCompared,szPattern)) { rc=TRUE; goto end; }
	// n'a pas matché direct : peut-être que l'utilisateur a défini http://www.... alors qu'il est 
	// sous chrome et donc on va essayer en virant le http:// (sauf si commence par * auquel cas ça 
	// aurait dû matcher dans swStringMatch)
	lenPattern=strlen(szPattern);
	if (lenPattern>1 && szPattern[0]=='*') goto end;
	if (lenPattern<7) goto end;
	if (_strnicmp(szPattern,"http://",7)!=0) { TRACE((TRACE_DEBUG,_F_,"szPattern ne commence pas par http://"));goto end; }
	TRACE((TRACE_DEBUG,_F_,"szPattern commence par http://, on tente le matching entre %s et %s",szToBeCompared,szPattern+7));
	if (swStringMatch(szToBeCompared,szPattern+7)) { rc=TRUE; goto end; }
	// dernier essai :avec ou sans / de fin d'URL
	lenToBeCompared=strlen(szToBeCompared);
	if (lenToBeCompared>1 && szToBeCompared[lenToBeCompared-1]=='/')
	{
		char szToBeCompared2[256+1];
		memcpy(szToBeCompared2,szToBeCompared,lenToBeCompared-1);
		szToBeCompared2[lenToBeCompared-1]=0;
		if (swStringMatch(szToBeCompared2,szPattern)) { rc=TRUE; goto end; }
	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
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

	// par défaut, retourne la valeur fournie en paramètre
	strcpy_s(gszComputedValue,sizeof(gszComputedValue),szValue);

	len=strlen(szValue);
	if (len>LEN_ID) { TRACE((TRACE_ERROR,_F_,"len(%s)=%d > %d",szValue,len,LEN_ID)); }

	if (len>2)
	{
		if (szValue[0]=='%' && szValue[len-1]=='%')
		{
			strncpy_s(szCopyOfValue,sizeof(szCopyOfValue),szValue+1,len-2);
			rc=GetEnvironmentVariable(szCopyOfValue,gszComputedValue,sizeof(gszComputedValue));
			if (rc==0)
			{ 
				TRACE((TRACE_ERROR,_F_,"GetEnvironmentVariable(%s)=%d",szCopyOfValue,GetLastError()));
				// Echec, gszComputedValue a déjà été initialisée donc on ne fait rien de plus
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
int LastDetect_AddOrUpdateWindow(HWND w)
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
	int i;

	for (i=0;i<MAX_NB_LAST_DETECT;i++)
	{
		if (gTabLastDetect[i].tag==0 && gTabLastDetect[i].wLastDetect!=NULL)
		{
			TRACE((TRACE_DEBUG,_F_,"Removing : 0x%08lx",gTabLastDetect[i].wLastDetect));
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

typedef struct
{
	int iPopupType;
	char *pszCompare;
	BOOL bFound;
} T_ENUM_BROWSER;

//-----------------------------------------------------------------------------
// EnumWindowsProc()
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
	if ((strcmp(szClassName,"IEFrame")==0) && (pEnumBrowser->iPopupType==POPUP_W7 || pEnumBrowser->iPopupType==POPUP_XP)) // IE
	{
		pszURL=GetIEURL(w,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL IE non trouvee : on passe !")); goto end; }
	}
	else if ((strcmp(szClassName,gcszMozillaUIClassName)==0) && (pEnumBrowser->iPopupType==POPUP_FIREFOX)) // FF3
	{
		pszURL=GetFirefoxURL(w,FALSE,NULL,BROWSER_FIREFOX3,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 3- non trouvee : on passe !")); goto end; }
	}
	else if ((strcmp(szClassName,gcszMozillaClassName)==0) && (pEnumBrowser->iPopupType==POPUP_FIREFOX)) // FF4
	{
		pszURL=GetFirefoxURL(w,FALSE,NULL,BROWSER_FIREFOX4,FALSE);
		if (pszURL==NULL) { TRACE((TRACE_ERROR,_F_,"URL Firefox 4+ non trouvee : on passe !")); goto end; }
	}
	else if ((strncmp(szClassName,"Chrome_WidgetWin_",17)==0)  && (pEnumBrowser->iPopupType==POPUP_CHROME)) // Chrome 20+ : Chrome_WidgetWin_0 -> Chrome_WidgetWin_
	{
		pszURL=GetChromeURL(w);
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