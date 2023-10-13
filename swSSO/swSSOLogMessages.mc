;//-----------------------------------------------------------------------------
;//
;//                                  swSSO
;//
;//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
;//
;//                Copyright (C) 2004-2023 - Sylvain WERDEFROY
;//
;//
;//                   
;//                       sylvain.werdefroy@gmail.com
;//
;//-----------------------------------------------------------------------------
;// 
;//  This file is part of swSSO.
;//  
;//  swSSO is free software: you can redistribute it and/or modify
;//  it under the terms of the GNU General Public License as published by
;//  the Free Software Foundation, either version 3 of the License, or
;//  (at your option) any later version.
;//
;//  swSSO is distributed in the hope that it will be useful,
;//  but WITHOUT ANY WARRANTY; without even the implied warranty of
;//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;//  GNU General Public License for more details.
;//
;//  You should have received a copy of the GNU General Public License
;//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
;// 
;//-----------------------------------------------------------------------------
;// swSSOLogs.mc
;//-----------------------------------------------------------------------------

LanguageNames=(French=0x040c:MSG0040C)
MessageIdTypedef=DWORD

MessageId=0x00000001
Severity=Success
Facility=Application
SymbolicName=MSG_PRIMARY_LOGIN_SUCCESS
Language=French
Authentification primaire reussie
.

MessageId=0x00000002
Severity=Success
Facility=Application
SymbolicName=MSG_QUIT
Language=French
Arret de swSSO
.

MessageId=0x00000003
Severity=Error
Facility=Application
SymbolicName=MSG_PRIMARY_LOGIN_ERROR
Language=French
Echec d'authentification primaire : mot de passe incorrect
.

MessageId=0x00000004
Severity=Error
Facility=Application
SymbolicName=MSG_SECONDARY_LOGIN_SUCCESS
Language=French
Authentification sur l'application %1 avec l'identifiant %2
.

MessageId=0x00000005
Severity=Error
Facility=Application
SymbolicName=MSG_SECONDARY_LOGIN_BAD_PWD
Language=French
Echec d'authentification sur l'application %1 avec l'identifiant %2
.

MessageId=0x00000006
Severity=Error
Facility=Application
SymbolicName=MSG_SWSSO_INI_CORRUPTED
Language=French
Fichier %1 corrompu
.

MessageId=0x00000007
Severity=Error
Facility=Application
SymbolicName=MSG_GENERIC_START_ERROR
Language=French
Erreur technique - Impossible de démarrer swSSO
.

MessageId=0x00000008
Severity=Success
Facility=Application
SymbolicName=MSG_LOCK
Language=French
Verrouillage
.

MessageId=0x00000009
Severity=Success
Facility=Application
SymbolicName=MSG_UNLOCK_SUCCESS
Language=French
Deverrouillage reussi
.

MessageId=0x0000000A
Severity=Error
Facility=Application
SymbolicName=MSG_UNLOCK_BAD_PWD
Language=French
Echec de deverrouillage : mot de passe incorrect
.

MessageId=0x0000000B
Severity=Success
Facility=Application
SymbolicName=MSG_PRIMARY_PWD_CHANGE_SUCCESS
Language=French
Changement du mot de passe primaire reussi
.

MessageId=0x0000000C
Severity=Error
Facility=Application
SymbolicName=MSG_PRIMARY_PWD_CHANGE_BAD_PWD
Language=French
Echec de changement de mot de passe primaire : ancien mot de passe incorrect
.

MessageId=0x0000000D
Severity=Error
Facility=Application
SymbolicName=MSG_PRIMARY_PWD_CHANGE_ERROR
Language=French
Echec de changement de mot de passe primaire : erreur technique
.

MessageId=0x0000000E
Severity=Error
Facility=Application
SymbolicName=MSG_SERVER_NOT_RESPONDING
Language=French
Le serveur de configuration ne repond pas (%1%2)
.

MessageId=0x0000000F
Severity=Success
Facility=Application
SymbolicName=MSG_CONFIG_UPDATE
Language=French
Mise a jour des configurations : %1 ajouts, %2 modifications, %3 desactivations, %4 suppressions
.

MessageId=0x00000010
Severity=Success
Facility=Application
SymbolicName=MSG_RECOVERY_STARTED
Language=French
Demarrage d'une procedure de reinitialisation de mot de passe
.

MessageId=0x00000011
Severity=Success
Facility=Application
SymbolicName=MSG_RECOVERY_SUCCESS
Language=French
Reinitialisation de mot de passe reussie
.

MessageId=0x00000012
Severity=Error
Facility=Application
SymbolicName=MSG_RECOVERY_FAIL
Language=French
Echec de la reinitialisation de mot de passe 
.

MessageId=0x00000013
Severity=Success
Facility=Application
SymbolicName=MSG_CHANGE_APP_PWD
Language=French
Changement du mot de passe de l'application %1 pour le compte %2
.

