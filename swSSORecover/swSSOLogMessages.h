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
// swSSOLogs.mc
//-----------------------------------------------------------------------------
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_START
//
// MessageText:
//
// Demarrage de swSSORecover
//
#define MSG_START                        ((DWORD)0x00000001L)

//
// MessageId: MSG_STOP
//
// MessageText:
//
// Arret de swSSORecover
//
#define MSG_STOP                         ((DWORD)0x00000002L)

//
// MessageId: MSG_IMPORT_KEY
//
// MessageText:
//
// Nouvelle cle importee : %1
//
#define MSG_IMPORT_KEY                   ((DWORD)0x00000003L)

//
// MessageId: MSG_KEYSTORE_BADPWD
//
// MessageText:
//
// Echec d'ouverture du keystore : mot de passe incorrect
//
#define MSG_KEYSTORE_BADPWD              ((DWORD)0xC0000004L)

//
// MessageId: MSG_IMPORTKEY_BADPWD
//
// MessageText:
//
// Echec d'import de la cle %1 : mot de passe incorrect
//
#define MSG_IMPORTKEY_BADPWD             ((DWORD)0xC0000005L)

//
// MessageId: MSG_RECOVERY_FORUSER
//
// MessageText:
//
// Demande de recouvrement traitee pour l'utilisateur %1
//
#define MSG_RECOVERY_FORUSER             ((DWORD)0x00000006L)

//
// MessageId: MSG_PWD_CHANGE
//
// MessageText:
//
// Changement de mot de passe du keystore
//
#define MSG_PWD_CHANGE                   ((DWORD)0x00000007L)

