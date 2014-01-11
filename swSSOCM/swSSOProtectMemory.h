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
// swSSOProtectMemory.h
//-----------------------------------------------------------------------------

int swProtectMemoryInit();
int swProtectMemory(LPVOID pData,DWORD cbData,DWORD dwFlags);
int swUnprotectMemory(LPVOID pData,DWORD cbData,DWORD dwFlags);
int swProtectMemoryTerm();

//#define PROTECTMEMORY_SAME_PROCESS	0x00
//#define ROTECTMEMORY_CROSS_PROCESS	0x01
//#define PROTECTMEMORY_SAME_LOGON	0x02
//#define CRYPTPROTECTMEMORY_SAME_PROCESS         0x00
//#define CRYPTPROTECTMEMORY_CROSS_PROCESS        0x01
//#define CRYPTPROTECTMEMORY_SAME_LOGON           0x02
//#define RTL_ENCRYPT_OPTION_CROSS_PROCESS    0x01
//#define RTL_ENCRYPT_OPTION_SAME_LOGON       0x02
