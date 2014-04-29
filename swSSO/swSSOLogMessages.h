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
// MessageId: MSG_PRIMARY_LOGIN_SUCCESS
//
// MessageText:
//
// Authentification primaire reussie
//
#define MSG_PRIMARY_LOGIN_SUCCESS        ((DWORD)0x00000001L)

//
// MessageId: MSG_QUIT
//
// MessageText:
//
// Arret de swSSO
//
#define MSG_QUIT                         ((DWORD)0x00000002L)

//
// MessageId: MSG_PRIMARY_LOGIN_ERROR
//
// MessageText:
//
// Echec d'authentification primaire : mot de passe incorrect
//
#define MSG_PRIMARY_LOGIN_ERROR          ((DWORD)0xC0000003L)

//
// MessageId: MSG_SECONDARY_LOGIN_SUCCESS
//
// MessageText:
//
// Authentification sur l'application %1 avec l'identifiant %2
//
#define MSG_SECONDARY_LOGIN_SUCCESS      ((DWORD)0xC0000004L)

//
// MessageId: MSG_SECONDARY_LOGIN_BAD_PWD
//
// MessageText:
//
// Echec d'authentification sur l'application %1 avec l'identifiant %2
//
#define MSG_SECONDARY_LOGIN_BAD_PWD      ((DWORD)0xC0000005L)

//
// MessageId: MSG_SWSSO_INI_CORRUPTED
//
// MessageText:
//
// Fichier %1 corrompu
//
#define MSG_SWSSO_INI_CORRUPTED          ((DWORD)0xC0000006L)

//
// MessageId: MSG_GENERIC_START_ERROR
//
// MessageText:
//
// Erreur technique - Impossible de démarrer swSSO
//
#define MSG_GENERIC_START_ERROR          ((DWORD)0xC0000007L)

//
// MessageId: MSG_LOCK
//
// MessageText:
//
// Verrouillage
//
#define MSG_LOCK                         ((DWORD)0x00000008L)

//
// MessageId: MSG_UNLOCK_SUCCESS
//
// MessageText:
//
// Deverrouillage reussi
//
#define MSG_UNLOCK_SUCCESS               ((DWORD)0x00000009L)

//
// MessageId: MSG_UNLOCK_BAD_PWD
//
// MessageText:
//
// Echec de deverrouillage : mot de passe incorrect
//
#define MSG_UNLOCK_BAD_PWD               ((DWORD)0xC000000AL)

//
// MessageId: MSG_PRIMARY_PWD_CHANGE_SUCCESS
//
// MessageText:
//
// Changement du mot de passe primaire reussi
//
#define MSG_PRIMARY_PWD_CHANGE_SUCCESS   ((DWORD)0x0000000BL)

//
// MessageId: MSG_PRIMARY_PWD_CHANGE_BAD_PWD
//
// MessageText:
//
// Echec de changement de mot de passe primaire : ancien mot de passe incorrect
//
#define MSG_PRIMARY_PWD_CHANGE_BAD_PWD   ((DWORD)0xC000000CL)

//
// MessageId: MSG_PRIMARY_PWD_CHANGE_ERROR
//
// MessageText:
//
// Echec de changement de mot de passe primaire : erreur technique
//
#define MSG_PRIMARY_PWD_CHANGE_ERROR     ((DWORD)0xC000000DL)

//
// MessageId: MSG_SERVER_NOT_RESPONDING
//
// MessageText:
//
// Le serveur de configuration ne repond pas (%1%2)
//
#define MSG_SERVER_NOT_RESPONDING        ((DWORD)0xC000000EL)

//
// MessageId: MSG_CONFIG_UPDATE
//
// MessageText:
//
// Mise a jour des configurations : %1 ajouts, %2 modifications, %3 desactivations
//
#define MSG_CONFIG_UPDATE                ((DWORD)0x0000000FL)

//
// MessageId: MSG_RECOVERY_STARTED
//
// MessageText:
//
// Demarrage d'une procedure de reinitialisation de mot de passe
//
#define MSG_RECOVERY_STARTED             ((DWORD)0x00000010L)

//
// MessageId: MSG_RECOVERY_SUCCESS
//
// MessageText:
//
// Reinitialisation de mot de passe reussie
//
#define MSG_RECOVERY_SUCCESS             ((DWORD)0x00000011L)

//
// MessageId: MSG_RECOVERY_FAIL
//
// MessageText:
//
// Echec de la reinitialisation de mot de passe 
//
#define MSG_RECOVERY_FAIL                ((DWORD)0xC0000012L)

//
// MessageId: MSG_CHANGE_APP_PWD
//
// MessageText:
//
// Changement du mot de passe de l'application %1 pour le compte %2
//
#define MSG_CHANGE_APP_PWD               ((DWORD)0x00000013L)

