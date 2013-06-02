#include "stdafx.h"

HANDLE ghPipe=INVALID_HANDLE_VALUE;
#define BUF_SIZE 1024

#define PBKDF2_PWD_LEN 32	// 256 bits
#define AES256_KEY_LEN 32   // 256 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define PWD_LEN 256
// mot de passe fourni par swSSOCM (chiffré par CryptProtectMemory)
char gbufPassword[PWD_LEN]; 
char gbufPasswordOld[PWD_LEN]; 
BOOL bPasswordStored=FALSE;
BOOL bPasswordOldStored=FALSE;
// PHKD (Password Hash + Key Data)
BYTE gPBKDF2Pwd[PBKDF2_PWD_LEN];
BYTE gAESKeyData[AES256_KEY_LEN];
BYTE gPBKDF2PwdOld[PBKDF2_PWD_LEN];
BYTE gAESKeyDataOld[AES256_KEY_LEN];
// PSKS (Password Salt + Key Salt)
T_SALT gSalts;

	
//-----------------------------------------------------------------------------
// swInitData()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swInitData(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	ZeroMemory(gPBKDF2Pwd,PBKDF2_PWD_LEN);
	ZeroMemory(gAESKeyData,AES256_KEY_LEN);
	ZeroMemory(gPBKDF2PwdOld,PBKDF2_PWD_LEN);
	ZeroMemory(gAESKeyDataOld,AES256_KEY_LEN);
	ZeroMemory(gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	ZeroMemory(gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	gSalts.bPBKDF2KeySaltReady=FALSE;
	gSalts.bPBKDF2PwdSaltReady=FALSE;
	ZeroMemory(gbufPassword,PWD_LEN);
	ZeroMemory(gbufPasswordOld,PWD_LEN);
	bPasswordStored=FALSE;
	bPasswordOldStored=FALSE;
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
// swWaitForMessage()
//-----------------------------------------------------------------------------
// Attend un message et le traite. Commandes supportées :
// V01:PUTPASS:password(256octets)(chiffré par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_SAME_LOGON)
// V01:GETPHKD:CUR > demande à SVC de fournir le KeyData courant
// V01:GETPHKD:OLD > demande à SVC de fournir le KeyData précédent (si chgt de mdp par exemple)
// V01:PUTPSKS:PwdSalt(64octets)KeySalt(64octets) > demande à SVC de stocker PwdSalt et KeySalt
// V01:GETPSKS: >demande à SVC de fournir PwdSalt et KeySalt
// N'est plus supporté :
// V01:PUTPHKD:PwdHash(32octets)KeyData(32octets) > demande à SVC de stocker PwdHash et KeyData 
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
		if (memcmp(bufRequest,"V01:",4)==0)
		{
#if 0
			if (memcmp(bufRequest+4,"PUTPHKD:",8)==0) // Format = V01:PUTPHKD:PwdHash(32octets)KeyData(32octets)
			{
				if (cbRead!=12+AES256_KEY_LEN+PBKDF2_PWD_LEN)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+AES256_KEY_LEN+PBKDF2_PWD_LEN));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					// Ignore le PUT si les données envoyées sont déjà connues
					if (memcmp(gPBKDF2Pwd,bufRequest+12,PBKDF2_PWD_LEN)==0)
					{
						TRACE((TRACE_INFO,_F_,"ON A DEJA CES VALEURS, ON IGNORE"));
					}
					else
					{
						// Archive les valeurs précédentes
						memcpy(gPBKDF2PwdOld,gPBKDF2Pwd,PBKDF2_PWD_LEN);
						memcpy(gAESKeyDataOld,gAESKeyData,AES256_KEY_LEN);
						TRACE_BUFFER((TRACE_DEBUG,_F_,gPBKDF2PwdOld,PBKDF2_PWD_LEN,"gPBKDF2PwdOld"));
						TRACE_BUFFER((TRACE_DEBUG,_F_,gAESKeyDataOld,AES256_KEY_LEN,"gAESKeyDataOld"));
					
						// Récupère les nouvelles valeurs
						memcpy(gPBKDF2Pwd,bufRequest+12,PBKDF2_PWD_LEN);
						memcpy(gAESKeyData,bufRequest+12+PBKDF2_PWD_LEN,AES256_KEY_LEN);
						TRACE_BUFFER((TRACE_DEBUG,_F_,gPBKDF2Pwd,PBKDF2_PWD_LEN,"gPBKDF2Pwd"));
						TRACE_BUFFER((TRACE_DEBUG,_F_,gAESKeyData,AES256_KEY_LEN,"gAESKeyData"));
						
						// Si swSSO est lancé, envoie un message pour signaler le nouveau mot de passe
						hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,"Global\\swsso-pwdchange");
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
#endif
			//-------------------------------------------------------------------------------------------------------------
			if (memcmp(bufRequest+4,"PUTPASS:",8)==0) // Format = V01:PUTPASS:password(256 octets, chiffré)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (cbRead!=12+256)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+256));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					// extrait le mdp de la requête dans une variable temporaire effacée de manière sécurisée à la fin de la fonction
					memcpy(tmpBufPwd,bufRequest+12,PWD_LEN);
					if (gSalts.bPBKDF2KeySaltReady && gSalts.bPBKDF2PwdSaltReady) // Les sels sont connus, on peut calculer PHKD sans conserver le mot de passe :-)
					{
						TRACE((TRACE_DEBUG,_F_,"Sels OK, on calcule PHKD"));
						// Déchiffre le mot de passe
						if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
						TRACE((TRACE_PWD,_F_,"tmpBufPwd=%s",tmpBufPwd));
						// On fait déjà le calcul dans un buffer temporaire pour vérifier qu'on n'a pas déjà ces informations
						if (swPBKDF2((BYTE*)tempPBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						if (memcmp(gPBKDF2Pwd,tempPBKDF2Pwd,PBKDF2_PWD_LEN)==0)
						{
							TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
							bSendEvent=FALSE;
						}
						else // OK, on n'a pas, on garde !
						{
							// Archive les valeurs précédentes
							memcpy(gPBKDF2PwdOld,gPBKDF2Pwd,PBKDF2_PWD_LEN);
							memcpy(gAESKeyDataOld,gAESKeyData,AES256_KEY_LEN);
							TRACE_BUFFER((TRACE_DEBUG,_F_,gPBKDF2PwdOld,PBKDF2_PWD_LEN,"gPBKDF2PwdOld"));
							TRACE_BUFFER((TRACE_DEBUG,_F_,gAESKeyDataOld,AES256_KEY_LEN,"gAESKeyDataOld"));
							// Calcul PHKD
							if (swPBKDF2((BYTE*)gPBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							if (swPBKDF2((BYTE*)gAESKeyData,AES256_KEY_LEN,tmpBufPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						}
					}
					else // Sinon, on garde le mot de passe et c'est au moment où les sels seront fournis (PUTPSKS) que le calcul sera fait et le mot de passe effacé.
					{
						TRACE((TRACE_DEBUG,_F_,"Sels NOK, on calculera PHKD plus tard..."));
						// Vérifie qu'on n'a pas déjà ce mot de passe
						if (memcmp(gbufPassword,tmpBufPwd,PWD_LEN)==0)
						{
							TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
							bSendEvent=FALSE;
						}
						else
						{
							// Conserve valeur précédente + stocke la nouvelle
							if (bPasswordStored)
							{
								memcpy(gbufPasswordOld,gbufPassword,PWD_LEN);
								bPasswordOldStored=TRUE;
							}
							memcpy(gbufPassword,tmpBufPwd,PWD_LEN);
							bPasswordStored=TRUE;
						}
					}
					// Si swSSO est lancé, envoie un message pour signaler le nouveau mot de passe
					if (bSendEvent)
					{
						hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,"Global\\swsso-pwdchange");
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
			else if (memcmp(bufRequest+4,"GETPHKD:",8)==0) // Format = V01:GETPHKD:CUR ou V01:GETPHKD:OLD
			//-------------------------------------------------------------------------------------------------------------
			{
				if (memcmp(bufRequest+12,"CUR",3)==0)
				{
					// Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
					memcpy(bufResponse,gPBKDF2Pwd,PBKDF2_PWD_LEN);
					memcpy(bufResponse+PBKDF2_PWD_LEN,gAESKeyData,AES256_KEY_LEN);
					lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
				}
				else if (memcmp(bufRequest+12,"OLD",3)==0)
				{
					// Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
					memcpy(bufResponse,gPBKDF2PwdOld,PBKDF2_PWD_LEN);
					memcpy(bufResponse+PBKDF2_PWD_LEN,gAESKeyDataOld,AES256_KEY_LEN);
					lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
				}
				else
				{
					TRACE((TRACE_ERROR,_F_,"Mot clé inconnu"));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
					lenResponse=strlen(bufResponse);
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"PUTPSKS:",8)==0) // V01:PUTPSKS:PwdSalt(64octets)KeySalt(64octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (cbRead!=12+PBKDF2_SALT_LEN*2)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+PBKDF2_SALT_LEN*2));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					memcpy(gSalts.bufPBKDF2PwdSalt,bufRequest+12,PBKDF2_SALT_LEN);
					memcpy(gSalts.bufPBKDF2KeySalt,bufRequest+12+PBKDF2_SALT_LEN,PBKDF2_SALT_LEN);
					gSalts.bPBKDF2PwdSaltReady=TRUE;
					gSalts.bPBKDF2KeySaltReady=TRUE;
					TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2PwdSalt"));
					TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2KeySalt"));
					// Si on a des mots de passe en mémoire, maintenant qu'on a les sels on peut les traiter et les effacer !
					if (bPasswordStored)
					{
						// Calcul PHKD (CUR)
						// Déchiffre le mot de passe
						if (swUnprotectMemory(gbufPassword,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
						TRACE((TRACE_PWD,_F_,"gbufPassword=%s",gbufPassword));
						if (swPBKDF2((BYTE*)gPBKDF2Pwd,PBKDF2_PWD_LEN,gbufPassword,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						if (swPBKDF2((BYTE*)gAESKeyData,AES256_KEY_LEN,gbufPassword,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						SecureZeroMemory(gbufPassword,sizeof(gbufPassword));
						bPasswordStored=FALSE;
					}
					if (bPasswordOldStored)
					{
						// Calcul PHKD (OLD)
						// Déchiffre le mot de passe
						if (swUnprotectMemory(gbufPasswordOld,PWD_LEN,CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
						TRACE((TRACE_PWD,_F_,"gbufPasswordOld=%s",gbufPasswordOld));
						if (swPBKDF2((BYTE*)gPBKDF2PwdOld,PBKDF2_PWD_LEN,gbufPasswordOld,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						if (swPBKDF2((BYTE*)gAESKeyDataOld,AES256_KEY_LEN,gbufPasswordOld,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
						SecureZeroMemory(gbufPasswordOld,sizeof(gbufPasswordOld));
						bPasswordOldStored=FALSE;
					}
					// Réponse
					strcpy_s(bufResponse,sizeof(bufResponse),"OK");
					lenResponse=strlen(bufResponse);
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"GETPSKS:",8)==0) // V01:GETPSKS: >demande à SVC de fournir PwdSalt et KeySalt
			//-------------------------------------------------------------------------------------------------------------
			{
				// Prépare la réponse, format = PwdSalt(64octets)KeySalt(64octets)
				memcpy(bufResponse,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
				memcpy(bufResponse+PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
				lenResponse=PBKDF2_SALT_LEN*2;
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