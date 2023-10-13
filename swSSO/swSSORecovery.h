//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
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
// swSSORecovery.h
//-----------------------------------------------------------------------------

extern int giRecoveryKeyId;
extern DWORD gdwRecoveryKeyLen;
extern BYTE *gpRecoveryKeyValue;
extern char gszRecoveryInfos[5+512+1];
extern int giRecoveryInfosKeyId;


int RecoveryChangeAESKeyData(int iKeyId);
void RecoveryFirstUse(HWND w,int iKeyId);

int RecoveryChallenge(HWND w);
int RecoveryResponse(HWND w);
int RecoveryWebservice(void);



