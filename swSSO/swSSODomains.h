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
// swSSODomains.h
//-----------------------------------------------------------------------------

#define LEN_DOMAIN 50
typedef struct 
{
	int iDomainId;
	char szDomainLabel[LEN_DOMAIN+1];
}
T_DOMAIN; // liste des domaines

typedef struct 
{
	int iDomainId;
	char szDomainLabel[LEN_DOMAIN+1];
	BOOL bAutoPublish;
}
T_CONFIGS_DOMAIN; // liste des domaines de publication d'une configuration

typedef struct 
{
	int iConfigId;
	BOOL bAutoPublish;
}
T_DOMAIN_CONFIGS; // liste des configurations publiées sur un domaine

#define MAX_DOMAINS 100

extern int  giDomainId;
extern char gszDomainLabel[LEN_DOMAIN+1];

int GetAllDomains(T_DOMAIN *pgtabDomain);
int GetConfigDomains(int iConfigId,T_CONFIGS_DOMAIN *pgtabDomain);
int GetDomainConfigsAutoPublish(int iDomainId,T_DOMAIN_CONFIGS *pgtabConfig);

void GetDomainLabel(int iDomainId);
int SelectDomain(void);

int ReadDomainLabel(char *pszDomainLabel);
int GetDomainIdFromLabel(const char *cszDomainLabel,int *piDomainId);

