//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                C�opyright (C) 2004-2025 - Sylvain WERDEFROY
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

#define REGKEY_TRACE			"SOFTWARE\\swSSO\\TraceCM"
#define REGVALUE_TRACE_LEVEL	"Level" 
#define REGVALUE_TRACE_FILENAME "FileName"
#define REGVALUE_TRACE_FILESIZE "FileSize"

#ifdef _DEBUG 
static int giTraceLevel=TRACE_DEBUG;
static char gszTraceFileName[_MAX_PATH+1]="c:\\swsso\\swssotracecm.txt";
#else
static int giTraceLevel=TRACE_NONE;
static char gszTraceFileName[_MAX_PATH+1]="";
#endif
static DWORD gdwTraceFileSize=20000000; 
static char gszTraceBuf[4096];
HANDLE ghfTrace=INVALID_HANDLE_VALUE;

static char gszTraceLevelLabel[5+1];

//-----------------------------------------------------------------------------
// ExpandFileName()
//-----------------------------------------------------------------------------
// Expande les variables d'environnement dans les noms de fichier
//-----------------------------------------------------------------------------
int ExpandFileName(char *szInFileName,char *szOutFileName, int iBufSize)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szTmpFileName[_MAX_PATH+1];
	int iPos=0;
	size_t len;

	// si chaine vide, on sort avec chaine vide
	if (*szInFileName==0) { *szOutFileName=0; rc=0; goto end; }

	// on commence par enlever les �ventuels guillemets de d�but et fin de chaine
	if (*szInFileName=='"') iPos=1;
	strcpy_s(szTmpFileName,_MAX_PATH+1,szInFileName+iPos);
	len=strlen(szTmpFileName);
	if (len>1 && szTmpFileName[len-1]=='"') szTmpFileName[len-1]=0;

	// on expande les variables d'environnement
	if (ExpandEnvironmentStrings(szTmpFileName,szOutFileName,iBufSize)==0)
	{
		TRACE((TRACE_ERROR,_F_,"ExpandEnvironmentStrings(%s)=%d",szTmpFileName,GetLastError()));
		goto end;
	}

	TRACE((TRACE_DEBUG,_F_,"ExpandEnvironmentStrings(%s)=%s",szTmpFileName,szOutFileName));
	rc=0;
	
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swTraceOpen()
//-----------------------------------------------------------------------------
// Lecture de la configuration en base de registre et ouverture du fichier trace
//-----------------------------------------------------------------------------
void swTraceOpen(void)
{
	HKEY hKey=NULL;
	int rc;
	char szValue[1024+1];
	DWORD dwValue,dwValueSize,dwValueType;
	int len;
	DWORD dw;

	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_TRACE,0,KEY_READ,&hKey);
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
		if (rc==ERROR_SUCCESS) 
		{
			char szTemp[_MAX_PATH+1];
			ExpandFileName(szValue,szTemp,_MAX_PATH+1); // ISSUE#291
			if (*szTemp!=0) sprintf_s(gszTraceFileName,sizeof(gszTraceFileName),"%s-%08lx",szTemp,GetTickCount());
		}
	}
	if (*gszTraceFileName==0) goto end; // pas de fichier sp�cifi�, pas de traces
	if (giTraceLevel==0) goto end; // niveau trace 0 : pas de trace

	// ouverture du fichier (ferm� uniquement sur appel de swTraceClose)
	ghfTrace=CreateFile(gszTraceFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	// se positionne � la fin du fichier
	SetFilePointer(ghfTrace,0,0,FILE_END);
	// entete
	len= sprintf_s(gszTraceBuf,sizeof(gszTraceBuf),"=================== TRACES INITIALISEES : level=%d ===================\r\n",giTraceLevel);
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
end:
	if (hKey!=NULL) RegCloseKey(hKey);
}

//-----------------------------------------------------------------------------
// swTraceClose()
//-----------------------------------------------------------------------------
// Est-il vraiment n�cessaire de commenter la fonction de cette fonction ? ;-)
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
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si n�cessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-t�te : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=sprintf_s(gszTraceBuf,sizeof(gszTraceBuf),"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=vsprintf_s(gszTraceBuf+len,sizeof(gszTraceBuf)-len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	FlushFileBuffers(ghfTrace);
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
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si n�cessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-t�te : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=sprintf_s(gszTraceBuf,sizeof(gszTraceBuf),"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=vsprintf_s(gszTraceBuf+len,sizeof(gszTraceBuf)-len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// v�rif buffer pas NULL
	if (pBuffer==NULL) 
	{
		strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"NULL\r\n");
		len=(int)strlen(gszTraceBuf);
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
		goto end;
	}
	// trace de la longueur
	len=sprintf_s(gszTraceBuf,sizeof(gszTraceBuf),"lenBuffer=%d\r\n",lenBuffer);
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// pr�formatage
	strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"                                                |                 \r\n");
	len=(int)strlen(gszTraceBuf);
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
		sprintf_s(gszTraceBuf+iBinOffset,sizeof(gszTraceBuf)-iBinOffset,"%02x",(unsigned char)(pBuffer[i]));
		iBinOffset+=2;
		gszTraceBuf[iBinOffset]=' '; // a �t� �cras� par le 0 du sprintf_s
		if (pBuffer[i]>=32 && pBuffer[i]<=126)
			gszTraceBuf[iCharOffset]=pBuffer[i];
		else
			gszTraceBuf[iCharOffset]='.';
		iBinOffset++;
		iCharOffset++;
	}
	//if ((i%16)!=0) 0.93B6 : visiblement empechait de voir la derni�re ligne des buffers multiples de 16 octets !
	{ 
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	}
	FlushFileBuffers(ghfTrace);
end:;
}
