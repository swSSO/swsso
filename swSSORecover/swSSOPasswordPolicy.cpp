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
// swSSOPasswordPolicy.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

// REGKEY_PASSWORD_POLICY
int giPwdPolicy_MinLength=0;
int giPwdPolicy_MinLetters=0;
int giPwdPolicy_MinUpperCase=0;
int giPwdPolicy_MinLowerCase=0;
int giPwdPolicy_MinNumbers=0;
int giPwdPolicy_MinSpecialsChars=0;
int giPwdPolicy_MaxAge=0;
int giPwdPolicy_MinAge=0;
int giPwdPolicy_IdMaxCommonChars=0;
char gszPwdPolicy_Message[1024+1];
int giPwdPolicy_MinRules=0;

BOOL gbWindowsEventLog=FALSE;				// 0.93 : log dans le journal d'�v�nements de Windows
char gszLogFileName[_MAX_PATH+1];			// 0.93 : chemin complet du fichier de log
int  giLogLevel=LOG_LEVEL_NONE;				// 0.93 : niveau de log

// ISSUE129
char *gpszMailSubject=NULL;
char *gpszMailBodyBefore=NULL;
char *gpszMailBodyAfter=NULL;

//-----------------------------------------------------------------------------
// LoadPasswordPolicy()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int LoadPasswordPolicy(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc;
	int ret=-1;
	HKEY hKey=NULL;
	char szValue[1024+1];
	DWORD dwValue,dwValueSize,dwValueType;

	// valeurs par d�faut pour les chaines de caract�res
	// les valeurs par d�faut pour les DWORD sont initialis�es dans la d�claration des variables globales
	strcpy_s(gszPwdPolicy_Message,sizeof(gszPwdPolicy_Message),GetString(IDS_PASSWORD_POLICY_MESSAGE));

	//--------------------------------------------------------------
	// PASSWORD POLICY
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_PASSWORD_POLICY,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLENGTH,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLength=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLETTERS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLetters=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINUPPERCASE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinUpperCase=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINLOWERCASE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinLowerCase=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINNUMBERS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinNumbers=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINSPECIALCHARS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinSpecialsChars=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MAXAGE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MaxAge=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINAGE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinAge=(int)dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_IDMAXCOMMONCHARS,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_IdMaxCommonChars=(int)dwValue+1; 

		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MESSAGE,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) 
			strcpy_s(gszPwdPolicy_Message,sizeof(gszPwdPolicy_Message),szValue);

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_PASSWORD_POLICY_MINRULES,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giPwdPolicy_MinRules=(int)dwValue; 

		RegCloseKey(hKey);
	}

	//--------------------------------------------------------------
	// ENTERPRISE OPTIONS
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_ENTERPRISE_OPTIONS,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_WINDOWS_EVENT_LOG,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gbWindowsEventLog=(BOOL)dwValue; 
		
		dwValueType=REG_SZ;
		dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_LOG_FILE_NAME,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			//strcpy_s(gszLogFileName,sizeof(gszLogFileName),szValue);
			ExpandFileName(szValue,gszLogFileName,_MAX_PATH+1);
		}
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_LOG_LEVEL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giLogLevel=(int)dwValue; 
		
		RegCloseKey(hKey);
	}

	//--------------------------------------------------------------
	// RECOVER OPTIONS
	//--------------------------------------------------------------
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_RECOVER_OPTIONS,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{		
		dwValueType=REG_SZ;
		dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_MAIL_SUBJECT,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			gpszMailSubject=(char*)malloc(dwValueSize);
			if (gpszMailSubject==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwValueSize)); goto end; }
			rc=RegQueryValueEx(hKey,REGVALUE_MAIL_SUBJECT,NULL,&dwValueType,(LPBYTE)gpszMailSubject,&dwValueSize);
			if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(MailObject)")); goto end; }
		}
		dwValueType=REG_SZ;
		dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_MAIL_BODY_BEFORE,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			gpszMailBodyBefore=(char*)malloc(dwValueSize);
			if (gpszMailBodyBefore==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwValueSize)); goto end; }
			rc=RegQueryValueEx(hKey,REGVALUE_MAIL_BODY_BEFORE,NULL,&dwValueType,(LPBYTE)gpszMailBodyBefore,&dwValueSize);
			if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(MailObject)")); goto end; }
		}
		dwValueType=REG_SZ;
		dwValueSize=0;
		rc=RegQueryValueEx(hKey,REGVALUE_MAIL_BODY_AFTER,NULL,&dwValueType,NULL,&dwValueSize);
		if (rc==ERROR_SUCCESS)
		{
			gpszMailBodyAfter=(char*)malloc(dwValueSize);
			if (gpszMailBodyAfter==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwValueSize)); goto end; }
			rc=RegQueryValueEx(hKey,REGVALUE_MAIL_BODY_AFTER,NULL,&dwValueType,(LPBYTE)gpszMailBodyAfter,&dwValueSize);
			if (rc!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(MailObject)")); goto end; }
		}
		RegCloseKey(hKey);
	}

	if (gpszMailSubject==NULL)
	{
		gpszMailSubject=(char*)malloc(10);
		if (gpszMailSubject==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(1)")); goto end; }
		strcpy_s(gpszMailSubject,10,"[swSSO]");
	}
	if (gpszMailBodyBefore==NULL)
	{
		gpszMailBodyBefore=(char*)malloc(1);
		if (gpszMailBodyBefore==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(1)")); goto end; }
		*gpszMailBodyBefore=0;
	}
	if (gpszMailBodyAfter==NULL)
	{
		gpszMailBodyAfter=(char*)malloc(1);
		if (gpszMailBodyAfter==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(1)")); goto end; }
		*gpszMailBodyAfter=0;
	}

#ifdef TRACES_ACTIVEES
	TRACE((TRACE_INFO,_F_,"PASSWORD POLICY-------------------"));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLength=%d"		,giPwdPolicy_MinLength));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLetters=%d"		,giPwdPolicy_MinLetters));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinUpperCase=%d"		,giPwdPolicy_MinUpperCase));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinLowerCase=%d"		,giPwdPolicy_MinLowerCase));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinNumbers=%d"		,giPwdPolicy_MinNumbers));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinSpecialsChars=%d"	,giPwdPolicy_MinSpecialsChars));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MaxAge=%d"			,giPwdPolicy_MaxAge));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_MinAge=%d"			,giPwdPolicy_MinAge));
	TRACE((TRACE_INFO,_F_,"giPwdPolicy_IdMaxCommonChars=%d"	,giPwdPolicy_IdMaxCommonChars));
	TRACE((TRACE_INFO,_F_,"ENTERPRISE OPTIONS ---------"));
	TRACE((TRACE_INFO,_F_,"giLogLevel=%d"					,giLogLevel));
	TRACE((TRACE_INFO,_F_,"gszLogFileName=%s"				,gszLogFileName));
	TRACE((TRACE_INFO,_F_,"gbWindowsEventLog=%d"			,gbWindowsEventLog));
	TRACE((TRACE_INFO,_F_,"RECOVER OPTIONS ---------"));
	TRACE((TRACE_INFO,_F_,"gpszMailSubject=%s"				,gpszMailSubject));
	TRACE((TRACE_INFO,_F_,"gpszMailBodyBefore=%s"			,gpszMailBodyBefore));
	TRACE((TRACE_INFO,_F_,"gpszMailBodyAfter=%s"			,gpszMailBodyAfter));
#endif
	ret=0;
end:
	TRACE((TRACE_LEAVE,_F_, ""));
	return ret;
}

#define SEARCHTYPE_LETTERS		1
#define SEARCHTYPE_UPPERCASE	2
#define SEARCHTYPE_LOWERCASE	3
#define SEARCHTYPE_NUMBERS		4
#define SEARCHTYPE_SPECIALCHARS	5

//-----------------------------------------------------------------------------
// GetNbCharsInString()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetNbCharsInString(const char *s,int iSearchType)
{
	TRACE((TRACE_ENTER,_F_, "iSearchType=%d",iSearchType));
	int count=0;
	unsigned int i;

	for (i=0;i<strlen(s);i++)
	{
		switch (iSearchType)
		{
			case SEARCHTYPE_LETTERS:
				if ((s[i]>='a' && s[i]<='z') || (s[i]>='A' && s[i]<='Z')) count++;
				break;
			case SEARCHTYPE_UPPERCASE:
				if (s[i]>='A' && s[i]<='Z') count++;
				break;
			case SEARCHTYPE_LOWERCASE:
				if (s[i]>='a' && s[i]<='z') count++;
				break;
			case SEARCHTYPE_NUMBERS:
				if (s[i]>='0' && s[i]<='9') count++;
				break;
			case SEARCHTYPE_SPECIALCHARS: // tout le reste !
				if (!(s[i]>='a' && s[i]<='z') && !(s[i]>='A' && s[i]<='Z') && !(s[i]>='0' && s[i]<='9')) count++;
				break;
		}
	}
	TRACE((TRACE_LEAVE,_F_, "count=%d",count));
	return count;
}

//-----------------------------------------------------------------------------
// CheckCommonChars()
//-----------------------------------------------------------------------------
// V�rifie que szPwd ne contient pas plus de giPwdPolicy_IdMaxCommonChars
// caract�res cons�cutifs de la chaine szUserName
//-----------------------------------------------------------------------------
BOOL CheckCommonChars(const char *szPwd,const char *szUserName,int iPwdPolicy_IdMaxCommonChars)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=TRUE;
	char szExtract[50+1];
	unsigned int i;

	TRACE((TRACE_DEBUG,_F_,"szPwd=%s szUserName=%s iPwdPolicy_IdMaxCommonChars=%d",szPwd,szUserName, iPwdPolicy_IdMaxCommonChars));
	if (iPwdPolicy_IdMaxCommonChars>30) goto end;

	for (i=0;i<strlen(szUserName)- iPwdPolicy_IdMaxCommonChars +1;i++)
	{
		memcpy(szExtract,szUserName+i, iPwdPolicy_IdMaxCommonChars);
		szExtract[iPwdPolicy_IdMaxCommonChars]=0;
		TRACE((TRACE_DEBUG,_F_,"Look for szExtract=%s in pwd=%s",szExtract,szPwd));

		//if (strstr(szPwd,szExtract)!=NULL) { rc=FALSE; goto end; }
		//0.85B6 : comparaison non case-sensitive !
		if (strnistr(szPwd,szExtract,-1)!=NULL) { rc=FALSE; goto end; }

	}
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// IsPasswordPolicyCompliant()
//-----------------------------------------------------------------------------
// int giPwdPolicy_MinLength=0;
// int giPwdPolicy_MinLetters=0;
// int giPwdPolicy_MinUpperCase=0;
// int giPwdPolicy_MinLowerCase=0;
// int giPwdPolicy_MinNumbers=0;
// int giPwdPolicy_MinSpecialsChars=0;
// int giPwdPolicy_IdMaxCommonChars=0;
//-----------------------------------------------------------------------------
BOOL IsPasswordPolicyCompliant(const char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	char szUserName[UNLEN+1];
	DWORD lenUserName=UNLEN+1;
	int iNbNoCompliancy; // superbe nommage... qui veut dire : nombre d'entorses aux r�gles (permet de g�rer le m parmi n).

	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));

	// v�rification longueur minimale 
	if (strlen(szPwd)<(unsigned)giPwdPolicy_MinLength) goto end;
	
	// v�rification composition : comptage des caract�res de chaque type puis v�rif
	if (GetNbCharsInString(szPwd,SEARCHTYPE_LETTERS)<giPwdPolicy_MinLetters) goto end;
	iNbNoCompliancy=0;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_UPPERCASE)<giPwdPolicy_MinUpperCase) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_LOWERCASE)<giPwdPolicy_MinLowerCase) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_NUMBERS)<giPwdPolicy_MinNumbers) iNbNoCompliancy++;
	if (GetNbCharsInString(szPwd,SEARCHTYPE_SPECIALCHARS)<giPwdPolicy_MinSpecialsChars) iNbNoCompliancy++;
	TRACE((TRACE_INFO,_F_,"iNbNoCompliancy=%d giPwdPolicy_MinRules=%d",iNbNoCompliancy,giPwdPolicy_MinRules));
	if (giPwdPolicy_MinRules==0 && iNbNoCompliancy!=0) goto end; // si MinRules=0 : aucune tol�rance.
	if (iNbNoCompliancy>(4-giPwdPolicy_MinRules)) goto end;

	// v�rification composition : Ne contenant pas plus de X caract�res cons�cutifs de l'identifiant Windows
	if (giPwdPolicy_IdMaxCommonChars!=0)
	{
		if (GetUserName(szUserName,&lenUserName)==0)
		{
			TRACE((TRACE_ERROR,_F_,"GetUserName()=0x%08lx -> pas de v�rification de giPwdPolicy_IdMaxCommonChars",GetLastError()));
		}
		else
		{
			TRACE((TRACE_INFO,_F_,"szUserName=%s",szUserName));
			if (!CheckCommonChars(szPwd,szUserName,giPwdPolicy_IdMaxCommonChars)) goto end;
		}
	}
	rc=TRUE; // yes!, pas facile de trouver un mot de passe, hein ? ;-)
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}