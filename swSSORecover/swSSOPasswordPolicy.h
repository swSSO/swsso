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
// swSSOPasswordPolicy.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#define REGKEY_PASSWORD_POLICY "SOFTWARE\\swSSO\\RecoverPolicy"
//-----------------------------------------------------------------------------
#define REGVALUE_PASSWORD_POLICY_MINLENGTH			"MinLength"
#define REGVALUE_PASSWORD_POLICY_MINLETTERS			"MinLetters"
#define REGVALUE_PASSWORD_POLICY_MINUPPERCASE		"MinUpperCase"
#define REGVALUE_PASSWORD_POLICY_MINLOWERCASE		"MinLowerCase"
#define REGVALUE_PASSWORD_POLICY_MINNUMBERS			"MinNumbers"
#define REGVALUE_PASSWORD_POLICY_MINSPECIALCHARS	"MinSpecialChars"
#define REGVALUE_PASSWORD_POLICY_MAXAGE				"MaxAge"
#define REGVALUE_PASSWORD_POLICY_MINAGE				"MinAge"
#define REGVALUE_PASSWORD_POLICY_IDMAXCOMMONCHARS	"IdMaxCommonChars"
#define REGVALUE_PASSWORD_POLICY_MESSAGE			"Message"
#define REGVALUE_PASSWORD_POLICY_MINRULES			"MinRules"

extern int giPwdPolicy_MinLength;
extern int giPwdPolicy_MinLetters;
extern int giPwdPolicy_MinUpperCase;
extern int giPwdPolicy_MinLowerCase;
extern int giPwdPolicy_MinNumbers;
extern int giPwdPolicy_MinSpecialsChars;
extern int giPwdPolicy_MaxAge;
extern int giPwdPolicy_MinAge;
extern int giPwdPolicy_IdMaxCommonChars;
extern char gszPwdPolicy_Message[];
extern int giPwdPolicy_MinRules;

//-----------------------------------------------------------------------------
#define REGKEY_ENTERPRISE_OPTIONS "SOFTWARE\\swSSO\\EnterpriseOptions"
//-----------------------------------------------------------------------------
#define REGVALUE_WINDOWS_EVENT_LOG					"WindowsEventLog"
#define REGVALUE_LOG_FILE_NAME						"LogFileName"
#define REGVALUE_LOG_LEVEL							"LogLevel"						

#define LOG_LEVEL_NONE			0 // pas de log
#define LOG_LEVEL_ERROR			1 // erreurs
#define LOG_LEVEL_WARNING		2 // warning
#define LOG_LEVEL_INFO_MANAGED	3 // infos pour configurations managées uniquement
#define LOG_LEVEL_INFO_ALL		4 // infos tout

extern BOOL gbWindowsEventLog;				// 0.93 : log dans le journal d'événements de Windows
extern char gszLogFileName[];				// 0.93 : chemin complet du fichier de log
extern int  giLogLevel;						// 0.93 : niveau de log

//-----------------------------------------------------------------------------
#define REGKEY_RECOVER_OPTIONS "SOFTWARE\\swSSO\\RecoverOptions"
//-----------------------------------------------------------------------------
#define REGVALUE_MAIL_SUBJECT						"MailSubject"
#define REGVALUE_MAIL_BODY_BEFORE					"MailBodyBefore"
#define REGVALUE_MAIL_BODY_AFTER					"MailBodyAfter"						

// ISSUE#129
extern char *gpszMailSubject;
extern char *gpszMailBodyBefore;
extern char *gpszMailBodyAfter;

// FONCTIONS PUBLIQUES
int LoadPasswordPolicy(void);
BOOL IsPasswordPolicyCompliant(const char *szPwd);
