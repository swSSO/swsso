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
// swSSOPolicies.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#define REGKEY_HOTKEY "SOFTWARE\\swSSO\\HotKey"
//-----------------------------------------------------------------------------
#define REGVALUE_PASTEPWD_CTRL	"PastePwd_Ctrl"
#define REGVALUE_PASTEPWD_ALT 	"PastePwd_Alt"
#define REGVALUE_PASTEPWD_SHIFT	"PastePwd_Shift"
#define REGVALUE_PASTEPWD_WIN	"PastePwd_Win"
#define REGVALUE_PASTEPWD_KEY	"PastePwd_Key"

extern int giPastePwd_Ctrl;
extern int giPastePwd_Alt;
extern int giPastePwd_Shift;
extern int giPastePwd_Win;
extern int giPastePwd_Key;

void LoadPolicies(void);
