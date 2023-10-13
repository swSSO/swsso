//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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

#include "stdafx.h"

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
	int len;

	// si chaine vide, on sort avec chaine vide
	if (*szInFileName==0) { *szOutFileName=0; rc=0; goto end; }

	// on commence par enlever les éventuels guillemets de début et fin de chaine
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
// strnistr()
//-----------------------------------------------------------------------------
// strstr non case sensitive
// trouvé chez code guru, à regarder de plus près, j'ai copié collé tel quel.
// http://www.codeguru.com/cpp/cpp/string/article.php/c5641
//-----------------------------------------------------------------------------
char *strnistr (const char *szStringToBeSearched,
				const char *szSubstringToSearchFor,
				const int  nStringLen)
{
   int            nLen;
   int            nOffset;
   int            nMaxOffset;
   char   *  pPos;
   int            nStringLenInt;

   // verify parameters
   if ( szStringToBeSearched == NULL ||
        szSubstringToSearchFor == NULL )
   {
      return (char*)szStringToBeSearched;
   }

   // get length of the substring
   nLen = _tcslen(szSubstringToSearchFor);

   // empty substring-return input (consistent w/ strstr)
   if ( nLen == 0 ) {
      return (char*)szStringToBeSearched;
   }

   if ( nStringLen == -1 || nStringLen >
               (int)_tcslen(szStringToBeSearched) )
   {
      nStringLenInt = _tcslen(szStringToBeSearched);
   } else {
      nStringLenInt = nStringLen;
   }

   nMaxOffset = nStringLenInt - nLen;

   pPos = (char*)szStringToBeSearched;

   for ( nOffset = 0; nOffset <= nMaxOffset; nOffset++ ) {

      if ( _tcsnicmp(pPos, szSubstringToSearchFor, nLen) == 0 ) {
         return pPos;
      }
      // move on to the next character
      pPos++; //_tcsinc was causing problems :(
   }

   return NULL;
}

//-----------------------------------------------------------------------------
// swPipeWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swPipeWrite(char *bufRequest,int lenRequest,char *bufResponse,DWORD sizeofBufResponse,DWORD *pdwLenResponse)
{
	HANDLE hPipe=INVALID_HANDLE_VALUE;
	int rc=-1;
	DWORD cbWritten;
	DWORD dwLastError;

	TRACE((TRACE_ENTER,_F_,""));
	
	// Ouverture du pipe créé par le service swsso
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	if (hPipe==INVALID_HANDLE_VALUE)
	{
		// Parfois entre deux appels le pipe n'est pas dispo tout de suite, donc on attend un peu
		dwLastError=GetLastError();
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%ld",dwLastError));
		if (dwLastError!=ERROR_PIPE_BUSY) goto end;
		for (int i=0;i<10;i++) // on attend 10 secondes max, sinon tant pis
		{
			TRACE((TRACE_DEBUG,_F_,"WaitNamedPipe()"));
			if (WaitNamedPipe("\\\\.\\pipe\\swsso",1000)) // attend 1 seconde, si OK retente l'ouverture
			{
				hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
				break;
			}
		}
		if (hPipe==INVALID_HANDLE_VALUE)
		{
			TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%ld",dwLastError));
			goto end;
		}
	}
	
	// Envoi de la requête
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,lenRequest,"Request to write"));
	if (!WriteFile(hPipe,bufRequest,lenRequest,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenRequest,GetLastError()));
		goto end;
	} 

	// Lecture la réponse
	if (!ReadFile(hPipe,bufResponse,sizeofBufResponse,pdwLenResponse,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeofBufResponse,GetLastError()));
		goto end;
	}  
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,*pdwLenResponse,"Response"));

	rc=0;
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}
