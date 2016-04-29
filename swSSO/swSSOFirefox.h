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
// swSSOFirefox.h
//-----------------------------------------------------------------------------

int SSOFirefox(HWND w,int iAction,int iBrowser);
char *GetFirefoxURL(HWND w,BOOL bGetAccessible,IAccessible **ppAccessible,int iBrowser,BOOL bWaitReady);

extern const char gcszMozillaClassName[];
extern const char gcszMozillaContentClassName[];
extern const char gcszMozillaUIClassName[];
extern const char gcszMozillaDialogClassName[];
