//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2025 - Sylvain WERDEFROY
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
// swSSOFirefoxTools.h
//-----------------------------------------------------------------------------

void KBSim(HWND w,BOOL bErase,int iTempo,const char *sz,BOOL bPwd);
int KBSimWeb(HWND w,BOOL bErase,int iTempo,const char *sz,BOOL bPwd,int iAction,int iBrowser,IAccessible *pTextField,VARIANT vtChild);
char *GetFirefoxPopupURL(HWND w);
IAccessible *GetFirefoxPopupIAccessible(HWND w);
void PutAccValue(HWND w,IAccessible *pAccessible,VARIANT index,const char *szValue);
int PutAccValueWeb(HWND w,IAccessible *pAccessible,VARIANT index,const char *szValue,int iAction,int iBrowser);
BOOL CheckIfURLStillOK(HWND w,int iAction,int iBrowser,IAccessible *pInAccessible,BOOL bGetAccessible,IAccessible **ppOutAccessible);



