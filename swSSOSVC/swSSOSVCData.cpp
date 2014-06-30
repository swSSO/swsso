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

HANDLE ghPipe=INVALID_HANDLE_VALUE;
#define BUF_SIZE 1024

#define PBKDF2_PWD_LEN 32	// 256 bits
#define AES256_KEY_LEN 32   // 256 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilis� avec PBKDF2 (512 bits)
#define PWD_LEN 256
#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite � moi...

typedef struct
{
	// domain et user name
	char szUserName[USER_LEN];
	char szLogonDomainName[DOMAIN_LEN];
	// mot de passe fourni par swSSOCM (chiffr� par CryptProtectMemory)
	char bufPassword[PWD_LEN]; 
	char bufPasswordOld[PWD_LEN]; 
	BOOL bPasswordStored;
	BOOL bPasswordOldStored;
	// PSKS (Password Salt + Key Salt)
	T_SALT Salts;
} T_USER_DATA;

T_USER_DATA gUserData[100]; // max 100 user sur le poste de travail, apr�s on explose...
int giMaxUserDataIndex=0;
	
//-----------------------------------------------------------------------------
// swInitData()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swInitData(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	SecureZeroMemory(gUserData,sizeof(gUserData));
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCreatePipe()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swCreatePipe()
{
	int rc=-1;
	SECURITY_ATTRIBUTES sa;
	char szACL[]="D:(A;;FRFW;;;IU)(A;;FRFW;;;SY)"; // IU=utilisateur session locale, SY=local system
	
	TRACE((TRACE_ENTER,_F_,""));
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=false;
	sa.lpSecurityDescriptor=NULL;
	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szACL,SDDL_REVISION_1,&(sa.lpSecurityDescriptor),NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ConvertStringSecurityDescriptorToSecurityDescriptor(%s)=%d",szACL,GetLastError()));
		goto end;	
	}
	ghPipe=CreateNamedPipe("\\\\.\\pipe\\swsso",
						PIPE_ACCESS_DUPLEX, 
						PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | PIPE_REJECT_REMOTE_CLIENTS, non support� sous XP !!
						PIPE_UNLIMITED_INSTANCES,
						BUF_SIZE,
						BUF_SIZE,
						1000,
						&sa);
	if (ghPipe==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%d",GetLastError()));
		goto end;
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// swGetUserDataIndex()
//-----------------------------------------------------------------------------
// Retourne l'index de l'utilisateur dans la table gUserData ou -1 si non trouv�
// BufRequest est de la forme : xxxxxxxxx:domain(len=256):username(len=256):xxxxxxxxx
// iOffset pointe ici -------------------^
//-----------------------------------------------------------------------------
int swGetUserDataIndex(const char *BufRequest,int iOffset)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	int i;
	char *pszUserName,*pszLogonDomainName;
	pszLogonDomainName=(char*)BufRequest+iOffset;
	pszUserName=(char*)BufRequest+iOffset+USER_LEN;
	for (i=0;i<giMaxUserDataIndex;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"pszLogonDomainName=%s gUserData[%d].szLogonDomainName=%s",pszLogonDomainName,i,gUserData[i].szLogonDomainName));
		TRACE((TRACE_DEBUG,_F_,"pszUserName=%s gUserData[%d].szUserName=%s",pszUserName,i,gUserData[i].szUserName));
		if (_stricmp(pszUserName,gUserData[i].szUserName)==0 &&
			_stricmp(pszLogonDomainName,gUserData[i].szLogonDomainName)==0)
		{
			TRACE((TRACE_INFO,_F_,"Trouv� %s\\%s index %d",pszLogonDomainName,pszUserName,i));
			rc=i;
			goto end;
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}
//-----------------------------------------------------------------------------
// swWaitForMessage()
//-----------------------------------------------------------------------------
// Attend un message et le traite. Commandes support�es :
// V02:PUTPASS:domain(256octets)username(256octets)password(256octets) (chiffr� par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_CROSS_PROCESS)
// V02:GETPHKD:CUR:domain(256octets)username(256octets) > demande � SVC de fournir le KeyData courant
// V02:GETPHKD:OLD:domain(256octets)username(256octets) > demande � SVC de fournir le KeyData pr�c�dent (si chgt de mdp par exemple)
// V02:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets) > demande � SVC de stocker PwdSalt et KeySalt
// Ne sont plus support�es :
// V01:PUTPHKD:PwdHash(32octets)KeyData(32octets) > demande � SVC de stocker PwdHash et KeyData 
// V01:PUTPASS:password(256octets)(chiffr� par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_SAME_LOGON)
// V01:GETPHKD:CUR > demande � SVC de fournir le KeyData courant
// V01:GETPHKD:OLD > demande � SVC de fournir le KeyData pr�c�dent (si chgt de mdp par exemple)
// V01:PUTPSKS:PwdSalt(64octets)KeySalt(64octets) > demande � SVC de stocker PwdSalt et KeySalt
// V01:GETPSKS: >demande � SVC de fournir PwdSalt et KeySalt
//-----------------------------------------------------------------------------
int swWaitForMessage()
{
	int rc=-1;
	char bufRequest[1024];
	char bufResponse[1024];
	char szEventName[1024];
	int lenResponse;
	DWORD cbRead,cbWritten;
	HANDLE hEvent=NULL;
	BOOL brc;
	char tmpBufPwd[PWD_LEN];
	int iUserDataIndex;
	// PHKD (Password Hash + Key Data)
	BYTE PBKDF2Pwd[PBKDF2_PWD_LEN];
	BYTE AESKeyData[AES256_KEY_LEN];
	
	TRACE((TRACE_ENTER,_F_,""));	
	
	// Se met en attente d'un message
	if (!ConnectNamedPipe(ghPipe, NULL))
	{
		DWORD dwLastError=GetLastError();
		if (dwLastError==ERROR_PIPE_CONNECTED) // en fait dans ce cas l� ce n'est pas une erreur (si, si, regardez bien le MSDN...)
		{
			TRACE((TRACE_INFO,_F_,"ConnectNamedPipe()=ERROR_PIPE_CONNECTED, pas une erreur"));
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"ConnectNamedPipe()=%d",GetLastError()));
			goto end;
		}
	}

	// Lit la requ�te
    if (!ReadFile(ghPipe,bufRequest,sizeof(bufRequest),&cbRead,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeof(bufRequest),GetLastError()));
		goto end;
	}          
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,cbRead,"Request"));
	if (cbRead<12)
	{
		TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu >12",cbRead));
		strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
		lenResponse=strlen(bufResponse);
	}
	else // Analyse la requ�te
	{
		if (memcmp(bufRequest,"V02:",4)==0)
		{
			//-------------------------------------------------------------------------------------------------------------
			if (memcmp(bufRequest+4,"PUTPASS:",8)==0) // Format = V01:PUTPASS:domain(256octets)username(256octets)password(256 octets, chiffr�)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (cbRead!=12+DOMAIN_LEN+USER_LEN+PWD_LEN)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+256));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					iUserDataIndex=swGetUserDataIndex(bufRequest,12);
					if (iUserDataIndex==-1) // utilisateur non connu, on le cr�e
					{
						iUserDataIndex=giMaxUserDataIndex;
						memcpy(gUserData[iUserDataIndex].szLogonDomainName,bufRequest+12,DOMAIN_LEN);
						memcpy(gUserData[iUserDataIndex].szUserName,bufRequest+12+DOMAIN_LEN,USER_LEN);
						TRACE((TRACE_INFO,_F_,"Utilisateur %s\\%s pas encore connu, on le cr�e",gUserData[iUserDataIndex].szLogonDomainName,gUserData[iUserDataIndex].szUserName));
						giMaxUserDataIndex++;
					}
					TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
					// extrait le mdp de la requ�te dans une variable temporaire effac�e de mani�re s�curis�e � la fin de la fonction
					memcpy(tmpBufPwd,bufRequest+12+DOMAIN_LEN+USER_LEN,PWD_LEN);
					// ISSUE#156 : on ne calcule plus le PKHD tout de suite, on le fait maintenant syst�matiquement quand on recoit un GETPKHD
					//             comme �a on est s�r d'�tre � jour � la fois du mot de passe et des sels. Et �a simplifie le code.
					// ISSUE#156 : on d�chiffre le mot de passe re�u qui est chiffr� avec CRYPTPROTECTMEMORY_CROSS_PROCESS
					//             pour le rechiffrer avec CRYPTPROTECTMEMORY_SAME_PROCESS
					if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_CROSS_PROCESS)!=0) goto end;
					TRACE((TRACE_PWD,_F_,"tmpBufPwd=%s",tmpBufPwd));
					if (swProtectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
					// V�rifie qu'on n'a pas d�j� ce mot de passe
					if (memcmp(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN)==0)
					{
						TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
					}
					else
					{
						// Si on a d�j� stock� un mot de passe, on copie la valeur pr�c�dente dans old
						if (gUserData[iUserDataIndex].bPasswordStored)
						{
							memcpy(gUserData[iUserDataIndex].bufPasswordOld,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
							gUserData[iUserDataIndex].bPasswordOldStored=TRUE;
						}
						// Stocke le nouveau mot de passe re�u
						memcpy(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN);
						gUserData[iUserDataIndex].bPasswordStored=TRUE;
						// Si swSSO est lanc�, envoie un message pour signaler le nouveau mot de passe pour cet utilisateur
						sprintf_s(szEventName,"Global\\swsso-pwdchange-%s-%s",gUserData[iUserDataIndex].szLogonDomainName,gUserData[iUserDataIndex].szUserName);
						hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,szEventName);
						if (hEvent==NULL)
						{
							TRACE((TRACE_ERROR,_F_,"swSSO pas lanc� (OpenEvent()=%ld)",GetLastError()));
						}
						else
						{
							TRACE((TRACE_DEBUG,_F_,"hEvent=0x%08lx",hEvent));
							brc=SetEvent(hEvent);
							TRACE((TRACE_DEBUG,_F_,"SetEvent=%d",brc));
							CloseHandle(hEvent);
							hEvent=NULL;
						}
					}
					// R�ponse
					strcpy_s(bufResponse,sizeof(bufResponse),"OK");
					lenResponse=strlen(bufResponse);
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"GETPHKD:",8)==0) // Format = V01:GETPHKD:CUR:domain(256octets)username(256octets) ou V01:GETPHKD:OLD:domain(256octets)username(256octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (memcmp(bufRequest+12,"CUR:",4)==0)
				{
					iUserDataIndex=swGetUserDataIndex(bufRequest,16);
					if (iUserDataIndex==-1) // utilisateur non connu, erreur !
					{
						TRACE((TRACE_ERROR,_F_,"Unknown user"));
						strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
						lenResponse=strlen(bufResponse);
					}
					else // Pr�pare la r�ponse, format = PwdHash(32octets)KeyData(32octets)
					{
						TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
						// ISSUE#156 : maintenant le calcul PKHD est fait ici et uniquement ici
						if (gUserData[iUserDataIndex].bPasswordStored)
						{
							memcpy(tmpBufPwd,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
							if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
							TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPassword=%s",iUserDataIndex,tmpBufPwd));
							if (swPBKDF2((BYTE*)PBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						}
						memcpy(bufResponse,PBKDF2Pwd,PBKDF2_PWD_LEN);
						memcpy(bufResponse+PBKDF2_PWD_LEN,AESKeyData,AES256_KEY_LEN);
						lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
					}
				}
				else if (memcmp(bufRequest+12,"OLD:",4)==0)
				{
					iUserDataIndex=swGetUserDataIndex(bufRequest,16);
					if (iUserDataIndex==-1) // utilisateur non connu, erreur !
					{
						TRACE((TRACE_ERROR,_F_,"Unknown user"));
						strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
						lenResponse=strlen(bufResponse);
					}
					else // Pr�pare la r�ponse, format = PwdHash(32octets)KeyData(32octets)
					{
						TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
						// ISSUE#156 : maintenant le calcul PKHD est fait ici et uniquement ici
						if (gUserData[iUserDataIndex].bPasswordOldStored)
						{
							memcpy(tmpBufPwd,gUserData[iUserDataIndex].bufPasswordOld,PWD_LEN);
							if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
							TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPasswordOld=%s",iUserDataIndex,tmpBufPwd));
							if (swPBKDF2((BYTE*)PBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						}
						memcpy(bufResponse,PBKDF2Pwd,PBKDF2_PWD_LEN);
						memcpy(bufResponse+PBKDF2_PWD_LEN,AESKeyData,AES256_KEY_LEN);
						lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
					}
				}
				else
				{
					TRACE((TRACE_ERROR,_F_,"Mot cl� inconnu"));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
					lenResponse=strlen(bufResponse);
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"PUTPSKS:",8)==0) // V01:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (cbRead!=12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN*2)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+PBKDF2_SALT_LEN*2));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					iUserDataIndex=swGetUserDataIndex(bufRequest,12);
					if (iUserDataIndex==-1) // utilisateur non connu, erreur !
					{
						TRACE((TRACE_ERROR,_F_,"Unknown user"));
						strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
						lenResponse=strlen(bufResponse);
					}
					else
					{
						memcpy(gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,bufRequest+12+DOMAIN_LEN+USER_LEN,PBKDF2_SALT_LEN);
						memcpy(gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,bufRequest+12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN,PBKDF2_SALT_LEN);
						gUserData[iUserDataIndex].Salts.bPBKDF2PwdSaltReady=TRUE;
						gUserData[iUserDataIndex].Salts.bPBKDF2KeySaltReady=TRUE;
						TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gUserData[%d].Salts.bufPBKDF2PwdSalt",iUserDataIndex));
						TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gUserData[%d].Salts.bufPBKDF2KeySalt",iUserDataIndex));
						// R�ponse
						strcpy_s(bufResponse,sizeof(bufResponse),"OK");
						lenResponse=strlen(bufResponse);
					}	
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else
			//-------------------------------------------------------------------------------------------------------------
			{
				TRACE((TRACE_ERROR,_F_,"Mot cl� inconnu"));
				strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
				lenResponse=strlen(bufResponse);
			}
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"Version non support�e !"));
			strcpy_s(bufResponse,sizeof(bufResponse),"BADVERSION");
			lenResponse=strlen(bufResponse);
		}
	}
	// Envoie la r�ponse
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,lenResponse,"Response"));
	if (!WriteFile(ghPipe,bufResponse,lenResponse,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenResponse,GetLastError()));
		goto end;
	}  

	FlushFileBuffers(ghPipe);
	DisconnectNamedPipe(ghPipe); 
	rc=0;
end:
	// Efface la requ�te de mani�re s�curis�e (elle peut contenir un mot de passe !)
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	SecureZeroMemory(tmpBufPwd,sizeof(tmpBufPwd));
	SecureZeroMemory(PBKDF2Pwd,sizeof(PBKDF2Pwd));
	SecureZeroMemory(AESKeyData,sizeof(AESKeyData));
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swDestroyPipe()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swDestroyPipe(void)
{
	TRACE((TRACE_ENTER,_F_,""));	
	if (ghPipe!=INVALID_HANDLE_VALUE) CloseHandle(ghPipe);
	TRACE((TRACE_LEAVE,_F_,""));
}
