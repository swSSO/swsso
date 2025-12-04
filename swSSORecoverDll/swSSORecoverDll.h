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

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SWSSORECOVERDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SWSSORECOVERDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SWSSORECOVERDLL_EXPORTS
#define SWSSORECOVERDLL_API __declspec(dllexport)
#else
#define SWSSORECOVERDLL_API __declspec(dllimport)
#endif

SWSSORECOVERDLL_API int RecoveryGetResponse(
		const char *szChallenge,
		const char *szDomainUserName,
		char *szResponse,
		int iMaxCount);

#define ERR_BAD_CHALLENGE		-1
#define ERR_BAD_USER			-2
#define ERR_BUFFER_TOO_SMALL	-3
#define ERR_OTHER				-4
#define ERR_CONFIG_NOT_FOUND	-5
#define ERR_KEYSTORE_NOT_FOUND	-6
#define ERR_KEYSTORE_BAD_PWD	-7
#define ERR_KEYSTORE_CORRUPTED	-8
