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
// swSSOProtectMemory.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

typedef BOOL (WINAPI *CRYPTPROTECTMEMORY)(LPVOID pData,DWORD cbData,DWORD dwFlags);
typedef BOOL (WINAPI *CRYPTUNPROTECTMEMORY)(LPVOID pData,DWORD cbData,DWORD dwFlags);
typedef NTSTATUS (WINAPI *RTLENCRYPTMEMORY)(PVOID Memory,ULONG MemoryLength,ULONG OptionFlags);
typedef NTSTATUS (WINAPI *RTLDECRYPTMEMORY)(PVOID Memory,ULONG MemoryLength,ULONG OptionFlags);

HMODULE ghLibCrypt32=NULL;
HMODULE ghLibAdvapi32=NULL;
CRYPTPROTECTMEMORY glpfnCryptProtectMemory=NULL;
CRYPTUNPROTECTMEMORY glpfnCryptUnprotectMemory=NULL;
RTLENCRYPTMEMORY glpfnRtlEncryptMemory=NULL;
RTLDECRYPTMEMORY glpfnRtlDecryptMemory=NULL;

//-----------------------------------------------------------------------------
// swProtectMemoryInit()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swProtectMemoryInit(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	// Commence par rechercher les fonctions CryptProtectMemory et CryptUnprotectMemory dans Crypt32.dll
	// Si on ne les trouve pas, c'est qu'on est sous XP et dans ce cas on recherche les fonctions
	// RtlEncryptMemory et RtlDecryptMemory dans Advapi32.dll
	ghLibCrypt32=LoadLibrary("Crypt32.dll");
	if (ghLibCrypt32==NULL) { TRACE((TRACE_ERROR,_F_,"LoadLibrary(Crypt32.dll)=%ld",GetLastError())); goto end; }
	glpfnCryptProtectMemory=(CRYPTPROTECTMEMORY)GetProcAddress(ghLibCrypt32,"CryptProtectMemory");
	glpfnCryptUnprotectMemory=(CRYPTUNPROTECTMEMORY)GetProcAddress(ghLibCrypt32,"CryptUnprotectMemory");
	TRACE((TRACE_DEBUG,_F_,"glpfnCryptProtectMemory=0x%08lx glpfnCryptUnprotectMemory=0x%08lx",glpfnCryptProtectMemory,glpfnCryptUnprotectMemory));
	if (glpfnCryptProtectMemory==NULL || glpfnCryptUnprotectMemory==NULL) // pas trouvé, on est sous XP
	{
		glpfnCryptProtectMemory=NULL; glpfnCryptUnprotectMemory=NULL;
		// plus besoin Crypt32.dll puisqu'on n'a rien trouvé
		FreeLibrary(ghLibCrypt32); ghLibCrypt32=NULL;
		ghLibAdvapi32=LoadLibrary("Advapi32.dll");
		if (ghLibAdvapi32==NULL) { TRACE((TRACE_ERROR,_F_,"LoadLibrary(Advapi32.dll)=%ld",GetLastError())); goto end; }
		glpfnRtlEncryptMemory=(RTLENCRYPTMEMORY)GetProcAddress(ghLibAdvapi32,"SystemFunction040");
		glpfnRtlDecryptMemory=(RTLDECRYPTMEMORY)GetProcAddress(ghLibAdvapi32,"SystemFunction041");
		TRACE((TRACE_DEBUG,_F_,"glpfnRtlEncryptMemory=0x%08lx glpfnRtlDecryptMemory=0x%08lx",glpfnRtlEncryptMemory,glpfnRtlDecryptMemory));
		if (glpfnRtlEncryptMemory==NULL || glpfnRtlDecryptMemory==NULL)
		{
			glpfnRtlEncryptMemory=NULL; glpfnRtlDecryptMemory=NULL;
			TRACE((TRACE_ERROR,_F_,"Pas trouvé CryptProtectMemory ni RtlEncryptMemory...")); 
			goto end;
		}
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swProtectMemoryTerm()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swProtectMemoryTerm(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	if (ghLibCrypt32!=NULL) { FreeLibrary(ghLibCrypt32); ghLibCrypt32=NULL; }
	if (ghLibAdvapi32!=NULL) { FreeLibrary(ghLibAdvapi32); ghLibAdvapi32=NULL; }
	glpfnCryptProtectMemory=NULL;
	glpfnCryptUnprotectMemory=NULL;
	glpfnRtlEncryptMemory=NULL;
	glpfnRtlDecryptMemory=NULL;
	
	TRACE((TRACE_LEAVE,_F_, ""));
	return 0;
}

//-----------------------------------------------------------------------------
// swProtectMemory()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swProtectMemory(LPVOID pData,DWORD cbData,DWORD dwFlags)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	NTSTATUS ntrc;
	
	//TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)pData,cbData,"pData dwFlags=0x%08lx",dwFlags));
	//TRACE((TRACE_DEBUG,_F_,"cbData=%ld dwFlags=%ld",(ULONG)cbData,(ULONG)dwFlags));
	if (glpfnCryptProtectMemory!=NULL)
	{
		if (!glpfnCryptProtectMemory(pData,cbData,dwFlags)) { TRACE((TRACE_ERROR,_F_,"glpfnCryptProtectMemory()=%ld",GetLastError())); goto end; }
	}
	else if (glpfnRtlEncryptMemory!=NULL)
	{
		ntrc=glpfnRtlEncryptMemory(pData,cbData,dwFlags);
		if (ntrc!=STATUS_SUCCESS) { TRACE((TRACE_ERROR,_F_,"glpfnRtlEncryptMemory()=0x%08lx",ntrc)); goto end; }
	}
	else { TRACE((TRACE_ERROR,_F_,"swProtectMemory not initialized...")); goto end; }
	
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swUnprotectMemory()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swUnprotectMemory(LPVOID pData,DWORD cbData,DWORD dwFlags)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	NTSTATUS ntrc;
	
	if (glpfnCryptUnprotectMemory!=NULL)
	{
		if (!glpfnCryptUnprotectMemory(pData,cbData,dwFlags)) { TRACE((TRACE_ERROR,_F_,"glpfnCryptUnprotectMemory()=%ld",GetLastError())); goto end; }
	}
	else if (glpfnRtlDecryptMemory!=NULL)
	{
		ntrc=glpfnRtlDecryptMemory(pData,cbData,dwFlags);
		if (ntrc!=STATUS_SUCCESS) { TRACE((TRACE_ERROR,_F_,"glpfnRtlDecryptMemory()=0x%08lx",ntrc)); goto end; }
	}
	else { TRACE((TRACE_ERROR,_F_,"swProtectMemory not initialized...")); goto end; }

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}



