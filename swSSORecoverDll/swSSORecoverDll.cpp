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
#include "stdafx.h"

//-----------------------------------------------------------------------------
// ()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
SWSSORECOVERDLL_API int RecoveryGetResponse(
		const char *szChallenge,
		const char *szDomainUserName,
		char *szResponse,
		int iMaxCount)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-4;

	if (szChallenge==NULL) { TRACE((TRACE_INFO,_F_, "szChallenge=NULL",szChallenge)); goto end; }
	if (szDomainUserName==NULL) { TRACE((TRACE_INFO,_F_, "szDomainUserName=NULL",szDomainUserName)); goto end; }
	if (szResponse==NULL) { TRACE((TRACE_INFO,_F_, "szResponse=NULL",szResponse)); goto end; }

	TRACE((TRACE_INFO,_F_, "szChallenge=%s",szChallenge));
	TRACE((TRACE_INFO,_F_, "szDomainUserName=%s",szDomainUserName));
	TRACE((TRACE_INFO,_F_, "iMaxCount=%d",iMaxCount));
	
	*szResponse=0;

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
