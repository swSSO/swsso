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

#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite à moi...
#define LEN_ENCRYPTED_AES256  192
#define LEN_PWD 50

typedef struct 
{
	BOOL bPBKDF2PwdSaltReady;
	BYTE bufPBKDF2PwdSalt[PBKDF2_SALT_LEN];
	BOOL bPBKDF2KeySaltReady;
	BYTE bufPBKDF2KeySalt[PBKDF2_SALT_LEN];
} T_SALT;

extern T_SALT gSalts;
extern char gszCfgFile[];


