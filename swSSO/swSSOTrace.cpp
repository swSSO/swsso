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
// swSSOTrace.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#define REGKEY_TRACE			"SOFTWARE\\swSSO\\Trace"
#define REGKEY_TRACEADMIN		"SOFTWARE\\swSSOAdmin\\Trace"
#define REGVALUE_TRACE_LEVEL	"Level" 
#define REGVALUE_TRACE_FILENAME "FileName"
#define REGVALUE_TRACE_FILESIZE "FileSize"

#ifdef _DEBUG 
static int giTraceLevel=TRACE_DEBUG; // TRACE_PWD dev only !
#else
static int giTraceLevel=TRACE_NONE;
#endif
static char gszTraceFileName[_MAX_PATH+1]="";
static DWORD gdwTraceFileSize=20000000; 
static char gszTraceBuf[4096];
HANDLE ghfTrace=INVALID_HANDLE_VALUE;

static char gszTraceLevelLabel[5+1];

//-----------------------------------------------------------------------------
// swTraceOpen()
//-----------------------------------------------------------------------------
// Lecture de la configuration en base de registre et ouverture du fichier trace
//-----------------------------------------------------------------------------
void swTraceOpen(void)
{
	DWORD dw;
	int len;

#ifdef _DEBUG 
	if (gbAdmin)
		strcpy_s(gszTraceFileName,sizeof(gszTraceFileName),"c:\\temp\\swssotrace-admin.txt");
	else
		strcpy_s(gszTraceFileName,sizeof(gszTraceFileName),"c:\\temp\\swssotrace.txt");
#else
	HKEY hKey=NULL;
	int rc;
	char szValue[1024+1];
	DWORD dwValue,dwValueSize,dwValueType;

	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,gbAdmin?REGKEY_TRACEADMIN:REGKEY_TRACE,0,KEY_READ,&hKey);
	if (rc==ERROR_SUCCESS)
	{
		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_TRACE_LEVEL,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) giTraceLevel=dwValue; 

		dwValueType=REG_DWORD; dwValueSize=sizeof(dwValue);
		rc=RegQueryValueEx(hKey,REGVALUE_TRACE_FILESIZE,NULL,&dwValueType,(LPBYTE)&dwValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) gdwTraceFileSize=dwValue*1000000; 

		dwValueType=REG_SZ; dwValueSize=sizeof(szValue);
		rc=RegQueryValueEx(hKey,REGVALUE_TRACE_FILENAME,NULL,&dwValueType,(LPBYTE)szValue,&dwValueSize);
		if (rc==ERROR_SUCCESS) ExpandFileName(szValue,gszTraceFileName,_MAX_PATH+1); // ISSUE#291
	}
	if (*gszTraceFileName==0) goto end; // pas de fichier spécifié, pas de traces
	if (giTraceLevel==0) goto end; // niveau trace 0 : pas de trace
#endif

	// ouverture du fichier (fermé uniquement sur appel de swTraceClose)
	ghfTrace=CreateFile(gszTraceFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	// se positionne à la fin du fichier
	SetFilePointer(ghfTrace,0,0,FILE_END);
	// entete
	len=wsprintf(gszTraceBuf,"=================== TRACES INITIALISEES : level=%d version=%s beta=%s ===================\r\n",giTraceLevel,gcszCurrentVersion,gcszCurrentBeta);
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
end:
#ifndef _DEBUG 
	if (hKey!=NULL) RegCloseKey(hKey);
#endif
	;
}

//-----------------------------------------------------------------------------
// swTraceClose()
//-----------------------------------------------------------------------------
// Est-il vraiment nécessaire de commenter la fonction de cette fonction ? ;-)
//-----------------------------------------------------------------------------
void swTraceClose(void)
{
	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	CloseHandle(ghfTrace); 
	ghfTrace=INVALID_HANDLE_VALUE; 
end:;
}

//-----------------------------------------------------------------------------
// swGetTraceLevelLabel()
//-----------------------------------------------------------------------------
static char *swGetTraceLevelLabel(int iLevel)
{
	switch (iLevel)
	{
		case TRACE_ERROR: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"ERROR"); break;
		case TRACE_ENTER: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"  -> "); break;
		case TRACE_LEAVE: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel)," <-  "); break;
		case TRACE_INFO:  strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"INFO "); break;
		case TRACE_DEBUG: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"DEBUG"); break;
		case TRACE_PWD:   strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"*PWD*"); break;
	}
	return gszTraceLevelLabel;
}
//-----------------------------------------------------------------------------
// swTraceWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swTraceWrite(int iLevel,char *szFunction,char *szTrace, ...)
{
	int len;
	SYSTEMTIME horodate;
	char *psz=gszTraceBuf;
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si nécessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-tête : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=wsprintf(psz,"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=wvsprintf(gszTraceBuf+len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
end:;
}

//-----------------------------------------------------------------------------
// swTraceWriteBuffer()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swTraceWriteBuffer(int iLevel,char *szFunction,unsigned char *pBuffer,int lenBuffer,const char *szTrace, ...)
{
	int len;
	int i,iBinOffset,iCharOffset;
	SYSTEMTIME horodate;
	char *psz=gszTraceBuf;
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si nécessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-tête : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=wsprintf(psz,"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=wvsprintf(gszTraceBuf+len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// vérif buffer pas NULL
	if (pBuffer==NULL) 
	{
		strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"NULL\r\n");
		len=strlen(gszTraceBuf);
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
		goto end;
	}
	// trace de la longueur
	len=wsprintf(gszTraceBuf,"lenBuffer=%d\r\n",lenBuffer);
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// préformatage
	strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"                                                |                 \r\n");
	len=strlen(gszTraceBuf);
	// trace le contenu du buffer
	iBinOffset=0;
	iCharOffset=50;
	for (i=0;i<lenBuffer;i++)
	{
		if (i!=0 && (i%16)==0)
		{ 
			WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
			iBinOffset=0;
			iCharOffset=50;
			memset(gszTraceBuf,' ',len-2);
		}
		wsprintf(gszTraceBuf+iBinOffset,"%02x",(unsigned char)(pBuffer[i]));
		iBinOffset+=2;
		gszTraceBuf[iBinOffset]=' '; // a été écrasé par le 0 du wsprintf
		if (pBuffer[i]>=32 && pBuffer[i]<=127)
			gszTraceBuf[iCharOffset]=pBuffer[i];
		else
			gszTraceBuf[iCharOffset]='.';
		iBinOffset++;
		iCharOffset++;
	}
	//if ((i%16)!=0) 0.93B6 : visiblement empechait de voir la dernière ligne des buffers multiples de 16 octets !
	{ 
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	}
end:;
}
