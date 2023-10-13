//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2012 - Sylvain WERDEFROY
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
// swTools.h
//-----------------------------------------------------------------------------

extern char gszRes[];
char *GetString(UINT uiString);
BSTR GetBSTRFromSZ(const char *sz);
char *strnistr (const char *szStringToBeSearched,
				const char *szSubstringToSearchFor,
				const int  nStringLen);

HFONT GetModifiedFont(HWND w,long lfWeight);
void SetTextBold(HWND w,int iCtrlId);
int atox4(char *sz);
int ExpandFileName(char *szInFileName,char *szOutFileName, int iBufSize);
