//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2013 - Sylvain WERDEFROY
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
// swSSOTrace.h
//-----------------------------------------------------------------------------

#define TRACE_ERROR		1 // erreurs
#define TRACE_ENTER		2 // entrée de fonction
#define TRACE_LEAVE		3 // sortie de fonction
#define TRACE_INFO		4 // infos
#define TRACE_DEBUG     5 // pour debug : très verbeux !
#define TRACE_PWD		6 // encore pire, trace des mots de passe

#ifdef TRACES_ACTIVEES
#define TRACE_OPEN() swTraceOpen();
#define TRACE(zzz) swTraceWrite zzz
#define TRACE_BUFFER(zzz) swTraceWriteBuffer zzz
#define TRACE_CLOSE() swTraceClose();
#else
#define TRACE_OPEN()
#define TRACE(zzz)
#define TRACE_BUFFER(zzz)
#define TRACE_CLOSE()
#endif

#define _F_ __FUNCTION__
void swTraceOpen(void);
void swTraceClose(void);
void swTraceWrite(int iLevel,char *szFunction,char *szTrace, ...);
void swTraceWriteBuffer(int iLevel,char *szFunction,unsigned char *pBuffer,int lenBuffer,const char *szTrace, ...);

