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
// swTools.h
//-----------------------------------------------------------------------------

extern char gszRes[];
extern char gszComputedValue[];
char *GetString(UINT uiString);
BSTR GetBSTRFromSZ(const char *sz);
char *GetSZFromBSTR(BSTR bstr);
BOOL CompareBSTRtoSZ(BSTR bstr,const char *sz);
char *HTTPRequest(const char *pszServer,int iPort,BOOL bHTTPS,const char *pszRequest,LPCWSTR pwszMethod,void *pRequestData,DWORD dwLenRequestData,int timeout,T_PROXYPARAMS *pInProxyParams,DWORD *pdwStatusCode);
char *HTTPEncodeParam(char *pszToEncode);
char *HTTPDecodeParam(char *pszToDecode);
int swGetTopWindow(HWND *w, char *szTitle,int sizeofTitle);
BOOL GetConfigBoolValue(char *szSection,char *szItem,BOOL bDefault,BOOL bWriteIfNotFound);
void Help(void);
char *strnistr (const char *szStringToBeSearched,
				const char *szSubstringToSearchFor,
				const int  nStringLen);

#define B1 1
#define B2 2
#define B3 3

typedef struct
{
	HWND wParent;
	int  iTitleString;
	char *szSubTitle;
	char *szMessage;
	char *szIcone;
	int  iB1String;
	int  iB2String;
	int  iB3String;
} T_MESSAGEBOX3B_PARAMS;

int MessageBox3B(T_MESSAGEBOX3B_PARAMS *pParams);

HFONT GetModifiedFont(HWND w,long lfWeight);
void SetTextBold(HWND w,int iCtrlId);
BOOL DrawTransparentBitmap(HANDLE hBitmap,HDC dc,int x,int y,int cx,int cy,COLORREF crColour);
void DrawBitmap(HANDLE hBitmap,HDC dc,int x,int y,int cx,int cy);
void DrawLogoBar(HWND w,int height,HANDLE hLogoFondBlanc);
int KBSimEx(HWND w,char *szCmd, char *szId1,char *szId2,char *szId3,char *szId4,char *szPwd);
int atox4(char *sz);
BOOL swStringMatch(char *szToBeCompared,char *szPattern);
BOOL swURLMatch(char *szToBeCompared,char *szPattern);
char *GetComputedValue(const char *szValue);
int swCheckBrowserURL(int iPopupType,char *pszCompare);
void GetNbActiveApps(int *piNbActiveApps,int *piNbActiveAppsFromServer);

// 0.93 : liste des derni�res fen�tres d�tect�es et dont la configuration est connue de swSSO
#define MAX_NB_LAST_DETECT 500
typedef struct
{
	BYTE   tag;         // tag pour rep�rage pr�sence fen�tre (1=taggu�e,0=non taggu�e)
	time_t tLastDetect;	// derniere d�tection de cette fenetre 
	HWND   wLastDetect;	// handle de cette fenetre d�j� d�tect�e
	int	   iPopupType; // type de la popup pour traitement particulier des popup chrome qui ne sont pas des fen�tres...
}
T_LAST_DETECT;

int    LastDetect_AddOrUpdateWindow(HWND w, int iPopupType);	// ajoute ou met � jour une fen�tre dans la liste des derni�res d�tect�es
int    LastDetect_RemoveWindow(HWND w);			// supprime une fen�tre dans la liste des derni�res d�tect�es
time_t LastDetect_GetTime(HWND w);				// retourne la date de derni�re d�tection d'une fen�tre
int    LastDetect_TagWindow(HWND w);			// marque la fen�tre comme toujours pr�sente
void   LastDetect_UntagAllWindows();		// d�taggue toutes les fen�tres
void   LastDetect_RemoveUntaggedWindows();	// efface toutes les fen�tres non taggu�es
void   ExcludeOpenWindows();
BOOL   IsExcluded(HWND w);
int swPipeWrite(char *bufRequest,int lenRequest,char *bufResponse,DWORD sizeofBufResponse,DWORD *pdwLenResponse);
void RevealPasswordField(HWND w,BOOL bReveal);
void ClipboardCopy(char *sz);
void ClipboardDelete();
int ExpandFileName(char *szInFileName,char *szOutFileName, int iBufSize);
int GetIniHash(unsigned char *pBufHashValue);
int StoreIniEncryptedHash();
int CheckIniHash();
BOOL CheckIfQuitMessage(UINT msg);
void KillswSSO(void);

// comme RESEDIT est un peu merdique et me change la taille du s�parateur quand il a envie
// cette macro (� positionner dans WM_INITDIALOG) le replace correctement !
#define MACRO_SET_SEPARATOR { RECT rect; GetClientRect(w,&rect); MoveWindow(GetDlgItem(w,IDC_SEPARATOR),0,50,rect.right+1,2,FALSE); }
#define MACRO_SET_SEPARATOR_75 { RECT rect; GetClientRect(w,&rect); MoveWindow(GetDlgItem(w,IDC_SEPARATOR),0,75,rect.right+1,2,FALSE); }
#define MACRO_SET_SEPARATOR_140 { RECT rect; GetClientRect(w,&rect); MoveWindow(GetDlgItem(w,IDC_SEPARATOR),0,140,rect.right+1,2,FALSE); }
#define MACRO_SET_SEPARATOR_90 { RECT rect; GetClientRect(w,&rect); MoveWindow(GetDlgItem(w,IDC_SEPARATOR),0,90,rect.right+1,2,FALSE); }




