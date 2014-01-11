;//-----------------------------------------------------------------------------
;//
;//                                  swSSO
;//
;//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
;//
;//                Copyright (C) 2004-2014 - Sylvain WERDEFROY
;//
;//							 http://www.swsso.fr
;//                   
;//                             sylvain@swsso.fr
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
SymbolicName=MSG_START
Language=French
Demarrage de swSSORecover
.

MessageId=0x00000002
Severity=Success
Facility=Application
SymbolicName=MSG_STOP
Language=French
Arret de swSSORecover
.

MessageId=0x00000003
Severity=Success
Facility=Application
SymbolicName=MSG_IMPORT_KEY
Language=French
Nouvelle cle importee : %1
.

MessageId=0x00000004
Severity=Error
Facility=Application
SymbolicName=MSG_KEYSTORE_BADPWD
Language=French
Echec d'ouverture du keystore : mot de passe incorrect
.

MessageId=0x00000005
Severity=Error
Facility=Application
SymbolicName=MSG_IMPORTKEY_BADPWD
Language=French
Echec d'import de la cle %1 : mot de passe incorrect
.

MessageId=0x00000006
Severity=Success
Facility=Application
SymbolicName=MSG_RECOVERY_FORUSER
Language=French
Demande de recouvrement traitee pour l'utilisateur %1
.
