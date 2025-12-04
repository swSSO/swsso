//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2026 - Sylvain WERDEFROY
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
// swSSOIETools.h
//-----------------------------------------------------------------------------

typedef struct
{
	char *pszURL;
	BOOL bWaitReady;
}
T_GETURL;

typedef struct
{
	char *pszURL;
	BOOL bWaitReady;
	int iAction;
}
T_CHECKURL;

char *CheckIEURL(HWND w, BOOL bWaitReady,int iAction);
char *GetIEURL(HWND w, BOOL bWaitReady);
char *GetMaxthonURL(void);
char *GetIEWindowTitle(void);
char *GetW7PopupURL(HWND w);
char *GetW10PopupURL(HWND w);
IAccessible *GetW7PopupIAccessible(HWND w);
IAccessible *GetW10PopupIAccessible(HWND w);
