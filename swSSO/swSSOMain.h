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
// swSSOMain.h
//-----------------------------------------------------------------------------

extern const char gcszCurrentVersion[];
extern const char gcszCurrentBeta[];
extern HWND gwMain;
extern HINSTANCE ghInstance;
extern bool gbSSOActif;
extern HICON ghIconAltTab;
extern HICON ghIconSystrayActive;
extern HICON ghIconSystrayInactive; 
extern HICON ghIconLoupe; 
extern HICON ghIconHelp;
extern HANDLE ghLogo;
extern HANDLE ghLogoFondBlanc50;
extern HANDLE ghLogoFondBlanc90;
extern HANDLE ghLogoExclamation;
extern HANDLE ghLogoQuestion;
extern HCURSOR ghCursorHand;
extern HCURSOR ghCursorWait;
extern HIMAGELIST ghImageList;
extern HFONT ghBoldFont;

extern UINT guiNbWEBSSO;
extern UINT guiNbPOPSSO;
extern UINT guiNbWINSSO;
extern UINT guiNbWindows;
extern UINT guiNbVisibleWindows;
extern int giNbFenetres;
extern BOOL gbRememberOnThisComputer;
extern BOOL gbRecoveryRunning;
extern int giaccChildCountErrors;
extern int giaccChildErrors;

extern UINT guiStandardQuitMsg;
extern UINT guiAdminQuitMsg;

// #define PP_NONE 1		--> supprimé en 0.98 : ISSUE#83
// #define PP_ENCODED 2		--> supprimé en 0.98 : ISSUE#83
#define PP_UNDEFINED 0
#define PP_ENCRYPTED 3
#define PP_WINDOWS 4 // 0.96 : couplage mot de passe Windows
extern int giPwdProtection;

extern T_ACTION *gptActions;
extern int giNbActions;
extern int giBadPwdCount;

extern HWND gwAskPwd ; 

extern BOOL gAESKeyInitialized[2];
extern BYTE gAESKeyDataPart1[][AES256_KEY_PART_LEN];
extern BYTE gAESKeyDataPart2[][AES256_KEY_PART_LEN];
extern BYTE gAESKeyDataPart3[][AES256_KEY_PART_LEN];
extern BYTE gAESKeyDataPart4[][AES256_KEY_PART_LEN];
// astuce pour limiter les modifs de code : ghKey1 et ghKey2 étaient les handle des 2 clés, ils deviennent les index pour le tableau des clés
extern const int ghKey1; 
extern const int ghKey2;

extern char gcszK1[8+1];
extern char gcszK2[8+1];
extern char gcszK3[8+1];
extern char gcszK4[8+1];

extern SID *gpSid;
extern char *gpszRDN;
extern char gszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
extern char gszUserName[UNLEN+1];

extern int giTimer;

extern int giLastApplicationSSO;   
extern int giLastApplicationConfig;

extern T_DOMAIN gtabDomains[];
extern int giNbDomains;

extern SYSTEMTIME gLastLoginTime; // ISSUE#106

#define OS_WINDOWS_XP		1
#define OS_WINDOWS_VISTA	2
#define OS_WINDOWS_7		3
#define OS_WINDOWS_8		4
#define OS_WINDOWS_OTHER	99
extern int giOSVersion;

#define OS_32	32
#define OS_64	64
extern int giOSBits;

#define BROWSER_NONE 		0
#define BROWSER_FIREFOX3 	1
#define BROWSER_FIREFOX4 	2
#define BROWSER_IE			3
#define BROWSER_MAXTHON		4
#define BROWSER_CHROME		5

#define POPUP_NONE			0
#define POPUP_XP			1
#define POPUP_FIREFOX		2
#define POPUP_W7			3
#define POPUP_CHROME		4
#define POPUP_W10			5 // ISSUE#297

extern T_LAST_DETECT gTabLastDetect[];

int AskPwd(HWND wParent,BOOL bUseDPAPI);
int LaunchTimer(void);

extern BOOL gbWillTerminate;
extern BOOL gbAdmin;

extern HBRUSH ghRedBrush;

extern int giNbTranscryptError;
