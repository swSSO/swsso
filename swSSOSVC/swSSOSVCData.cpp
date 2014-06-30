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
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define PWD_LEN 256
#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite à moi...

typedef struct
{
	// domain et user name
	char szUserName[USER_LEN];
	char szLogonDomainName[DOMAIN_LEN];
	// mot de passe fourni par swSSOCM (chiffré par CryptProtectMemory)
	char bufPassword[PWD_LEN]; 
	char bufPasswordOld[PWD_LEN]; 
	BOOL bPasswordStored;
	BOOL bPasswordOldStored;
	// PHKD (Password Hash + Key Data)
	BYTE PBKDF2Pwd[PBKDF2_PWD_LEN];
	BYTE AESKeyData[AES256_KEY_LEN];
	BYTE PBKDF2PwdOld[PBKDF2_PWD_LEN];
	BYTE AESKeyDataOld[AES256_KEY_LEN];
	// PSKS (Password Salt + Key Salt)
	T_SALT Salts;
} T_USER_DATA;

T_USER_DATA gUserData[100]; // max 100 user sur le poste de travail, après on explose...
int giMaxUserDataIndex=0;
	
//-----------------------------------------------------------------------------
// swInitData()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swInitData(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	ZeroMemory(gUserData,sizeof(gUserData));
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
						PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | PIPE_REJECT_REMOTE_CLIENTS, non supporté sous XP !!
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
// Retourne l'index de l'utilisateur dans la table gUserData ou -1 si non trouvé
// BufRequest est de la forme : xxxxxxxxx:domain(len=256):username(len=256):xxxxxxxxx
// iOffset pointe ici -------------------|
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
			TRACE((TRACE_INFO,_F_,"Trouvé %s\\%s index %d",pszLogonDomainName,pszUserName,i));
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
// Attend un message et le traite. Commandes supportées :
// V02:PUTPASS:domain(256octets)username(256octets)password(256octets) (chiffré par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_SAME_LOGON)
// V02:GETPHKD:CUR:domain(256octets)username(256octets) > demande à SVC de fournir le KeyData courant
// V02:GETPHKD:OLD:domain(256octets)username(256octets) > demande à SVC de fournir le KeyData précédent (si chgt de mdp par exemple)
// V02:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets) > demande à SVC de stocker PwdSalt et KeySalt
// N'est plus supporté :
// V01:PUTPHKD:PwdHash(32octets)KeyData(32octets) > demande à SVC de stocker PwdHash et KeyData 
// V01:PUTPASS:password(256octets)(chiffré par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_SAME_LOGON)
// V01:GETPHKD:CUR > demande à SVC de fournir le KeyData courant
// V01:GETPHKD:OLD > demande à SVC de fournir le KeyData précédent (si chgt de mdp par exemple)
// V01:PUTPSKS:PwdSalt(64octets)KeySalt(64octets) > demande à SVC de stocker PwdSalt et KeySalt
// V01:GETPSKS: >demande à SVC de fournir PwdSalt et KeySalt
//-----------------------------------------------------------------------------
int swWaitForMessage()
{
	int rc=-1;
	char bufRequest[1024];
	char bufResponse[1024];
	char tempPBKDF2Pwd[PBKDF2_PWD_LEN];
	int lenResponse;
	DWORD cbRead,cbWritten;
	HANDLE hEvent=NULL;
	BOOL brc;
	char tmpBufPwd[PWD_LEN];
	BOOL bSendEvent=TRUE;
	int iUserDataIndex;
	
	TRACE((TRACE_ENTER,_F_,""));	
	
	// Se met en attente d'un message
	if (!ConnectNamedPipe(ghPipe, NULL))
	{
		DWORD dwLastError=GetLastError();
		if (dwLastError==ERROR_PIPE_CONNECTED) // en fait dans ce cas là ce n'est pas une erreur (si, si, regardez bien le MSDN...)
		{
			TRACE((TRACE_INFO,_F_,"ConnectNamedPipe()=ERROR_PIPE_CONNECTED, pas une erreur"));
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"ConnectNamedPipe()=%d",GetLastError()));
			goto end;
		}
	}

	// Lit la requête
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
	else // Analyse la requête
	{
		if (memcmp(bufRequest,"V02:",4)==0)
		{
			//-------------------------------------------------------------------------------------------------------------
			if (memcmp(bufRequest+4,"PUTPASS:",8)==0) // Format = V01:PUTPASS:domain(256octets)username(256octets)password(256 octets, chiffré)
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
					if (iUserDataIndex==-1) // utilisateur non connu, on le crée
					{
						iUserDataIndex=giMaxUserDataIndex;
						memcpy(gUserData[iUserDataIndex].szLogonDomainName,bufRequest+12,DOMAIN_LEN);
						memcpy(gUserData[iUserDataIndex].szUserName,bufRequest+12+DOMAIN_LEN,USER_LEN);
						giMaxUserDataIndex++;
					}
					TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
					// extrait le mdp de la requête dans une variable temporaire effacée de manière sécurisée à la fin de la fonction
					memcpy(tmpBufPwd,bufRequest+12+DOMAIN_LEN+USER_LEN,PWD_LEN);
					if (gUserData[iUserDataIndex].Salts.bPBKDF2KeySaltReady && gUserData[iUserDataIndex].Salts.bPBKDF2PwdSaltReady) // Les sels sont connus, on peut calculer PHKD sans conserver le mot de passe :-)
					{
						TRACE((TRACE_DEBUG,_F_,"Sels OK, on calcule PHKD"));
						// Déchiffre le mot de passe
						if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
						TRACE((TRACE_PWD,_F_,"tmpBufPwd=%s",tmpBufPwd));
						// On fait déjà le calcul dans un buffer temporaire pour vérifier qu'on n'a pas déjà ces informations
						if (swPBKDF2((BYTE*)tempPBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						if (memcmp(gUserData[iUserDataIndex].PBKDF2Pwd,tempPBKDF2Pwd,PBKDF2_PWD_LEN)==0)
						{
							TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
							bSendEvent=FALSE;
						}
						else // OK, on n'a pas, on garde !
						{
							// Archive les valeurs précédentes
							memcpy(gUserData[iUserDataIndex].PBKDF2PwdOld,gUserData[iUserDataIndex].PBKDF2Pwd,PBKDF2_PWD_LEN);
							memcpy(gUserData[iUserDataIndex].AESKeyDataOld,gUserData[iUserDataIndex].AESKeyData,AES256_KEY_LEN);
							TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].PBKDF2PwdOld,PBKDF2_PWD_LEN,"gUserData[%d].PBKDF2PwdOld",iUserDataIndex));
							TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].AESKeyDataOld,AES256_KEY_LEN,"gUserData[%d].AESKeyDataOld",iUserDataIndex));
							// Calcul PHKD
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].PBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						}
					}
					else // Sinon, on garde le mot de passe et c'est au moment où les sels seront fournis (PUTPSKS) que le calcul sera fait et le mot de passe effacé.
					{
						TRACE((TRACE_DEBUG,_F_,"Sels NOK, on calculera PHKD plus tard..."));
						// Vérifie qu'on n'a pas déjà ce mot de passe
						if (memcmp(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN)==0)
						{
							TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
							bSendEvent=FALSE;
						}
						else
						{
							// Conserve valeur précédente + stocke la nouvelle
							if (gUserData[iUserDataIndex].bPasswordStored)
							{
								memcpy(gUserData[iUserDataIndex].bufPasswordOld,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
								gUserData[iUserDataIndex].bPasswordOldStored=TRUE;
							}
							memcpy(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN);
							gUserData[iUserDataIndex].bPasswordStored=TRUE;
						}
					}
					// Si swSSO est lancé, envoie un message pour signaler le nouveau mot de passe
					if (bSendEvent)
					{
						char szEventName[1024];
						sprintf_s(szEventName,"Global\\swsso-pwdchange-%s-%s",gUserData[iUserDataIndex].szLogonDomainName,gUserData[iUserDataIndex].szUserName);
						hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,szEventName);
						if (hEvent==NULL)
						{
							TRACE((TRACE_ERROR,_F_,"swSSO pas lancé (OpenEvent()=%ld)",GetLastError()));
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
					// Réponse
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
					else // Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
					{
						TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
						memcpy(bufResponse,gUserData[iUserDataIndex].PBKDF2Pwd,PBKDF2_PWD_LEN);
						memcpy(bufResponse+PBKDF2_PWD_LEN,gUserData[iUserDataIndex].AESKeyData,AES256_KEY_LEN);
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
					else // Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
					{
						TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
						memcpy(bufResponse,gUserData[iUserDataIndex].PBKDF2PwdOld,PBKDF2_PWD_LEN);
						memcpy(bufResponse+PBKDF2_PWD_LEN,gUserData[iUserDataIndex].AESKeyDataOld,AES256_KEY_LEN);
						lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
					}
				}
				else
				{
					TRACE((TRACE_ERROR,_F_,"Mot clé inconnu"));
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
						// Si on a des mots de passe en mémoire, maintenant qu'on a les sels on peut les traiter et les effacer !
						if (gUserData[iUserDataIndex].bPasswordStored)
						{
							// Calcul PHKD (CUR)
							// Déchiffre le mot de passe
							if (swUnprotectMemory(gUserData[iUserDataIndex].bufPassword,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
							TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPassword=%s",iUserDataIndex,gUserData[iUserDataIndex].bufPassword));
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].PBKDF2Pwd,PBKDF2_PWD_LEN,gUserData[iUserDataIndex].bufPassword,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].AESKeyData,AES256_KEY_LEN,gUserData[iUserDataIndex].bufPassword,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							SecureZeroMemory(gUserData[iUserDataIndex].bufPassword,sizeof(gUserData[iUserDataIndex].bufPassword));
							gUserData[iUserDataIndex].bPasswordStored=FALSE;
						}
						if (gUserData[iUserDataIndex].bPasswordOldStored)
						{
							// Calcul PHKD (OLD)
							// Déchiffre le mot de passe
							if (swUnprotectMemory(gUserData[iUserDataIndex].bufPasswordOld,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
							TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPasswordOld=%s",iUserDataIndex,gUserData[iUserDataIndex].bufPasswordOld));
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].PBKDF2PwdOld,PBKDF2_PWD_LEN,gUserData[iUserDataIndex].bufPasswordOld,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)gUserData[iUserDataIndex].AESKeyDataOld,AES256_KEY_LEN,gUserData[iUserDataIndex].bufPasswordOld,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							SecureZeroMemory(gUserData[iUserDataIndex].bufPasswordOld,sizeof(gUserData[iUserDataIndex].bufPasswordOld));
							gUserData[iUserDataIndex].bPasswordOldStored=FALSE;
						}
						// Réponse
						strcpy_s(bufResponse,sizeof(bufResponse),"OK");
						lenResponse=strlen(bufResponse);
					}	
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else
			//-------------------------------------------------------------------------------------------------------------
			{
				TRACE((TRACE_ERROR,_F_,"Mot clé inconnu"));
				strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
				lenResponse=strlen(bufResponse);
			}
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"Version non supportée !"));
			strcpy_s(bufResponse,sizeof(bufResponse),"BADVERSION");
			lenResponse=strlen(bufResponse);
		}
	}
	// Envoie la réponse
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
	// Efface la requête de manière sécurisée (elle peut contenir un mot de passe !)
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	SecureZeroMemory(tmpBufPwd,sizeof(tmpBufPwd));
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