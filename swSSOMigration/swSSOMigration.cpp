
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
HINSTANCE ghInstance;

SID *gpSid=NULL;
char *gpszRDN=NULL;
char gszUserName[UNLEN+1]="";
char gszCfgFile[_MAX_PATH+1];
char gszLogFile[_MAX_PATH+1];
UINT guiContinueMsg=0;
T_SALT gSalts;
HCRYPTKEY ghKey1=NULL;
char gBufPassword[256];

//-----------------------------------------------------------------------------
// GetUserAndDomain() 
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int GetUserAndDomain(void)
{
	TRACE((TRACE_ENTER,_F_,""));

	DWORD cbRDN,cbSid;
	SID_NAME_USE eUse;
	DWORD lenUserName;
	int rc=-1;

	// UserName
	lenUserName=sizeof(gszUserName); 
	if (!GetUserName(gszUserName,&lenUserName))
	{
		TRACE((TRACE_ERROR,_F_,"GetUserName(%d)",GetLastError())); goto end;
	}

	// détermine le SID de l'utilisateur courant et récupère le nom de domaine
	cbSid=0;
	cbRDN=0;
	LookupAccountName(NULL,gszUserName,NULL,&cbSid,NULL,&cbRDN,&eUse); // pas de test d'erreur, car la fonction échoue forcément
	if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[1](%s)=%d",gszUserName,GetLastError()));
		goto end;
	}
	gpSid=(SID *)malloc(cbSid); if (gpSid==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbSid)); goto end; }
	gpszRDN=(char *)malloc(cbRDN); if (gpszRDN==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbRDN)); goto end; }
	if(!LookupAccountName(NULL,gszUserName,gpSid,&cbSid,gpszRDN,&cbRDN,&eUse))
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[2](%s)=%d",gszUserName,GetLastError()));
		goto end;
	}
	TRACE((TRACE_INFO,_F_,"gszUserName=%s gpszRDN=%s",gszUserName,gpszRDN));

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;

}

//-----------------------------------------------------------------------------
// MainWindowProc()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MainWindowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp) 
{
	return DefWindowProc(w,msg,wp,lp);
}

//-----------------------------------------------------------------------------
// CreateMainWindow()
//-----------------------------------------------------------------------------
// Création de la fenêtre technique qui recevra tous les messages du Systray
// et les notifications de verrouillage de session Windows
//-----------------------------------------------------------------------------
HWND CreateMainWindow(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HWND wMain=NULL;
	ATOM atom=0;
	WNDCLASS wndClass;
	
	ZeroMemory(&wndClass,sizeof(WNDCLASS));
	wndClass.style=0;
	wndClass.lpfnWndProc=MainWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = ghInstance;
	wndClass.hCursor = NULL;
	wndClass.lpszMenuName = 0;
	wndClass.hbrBackground = NULL;
	wndClass.hIcon = NULL;
	wndClass.lpszClassName = "swSSOMigrationClass";
	
	atom=RegisterClass(&wndClass);
	if (atom==0) goto end;
	
	wMain=CreateWindow("swSSOMigrationClass","swSSOMigrationWindow",0,0,0,0,0,NULL,NULL,ghInstance,0);
	if (wMain==NULL) goto end;

end:
	TRACE((TRACE_LEAVE,_F_, "wMain=0x%08lx",wMain));
	return wMain;
}

//-----------------------------------------------------------------------------
// LogMessage()
//-----------------------------------------------------------------------------
// Loggue étapes et erreurs éventuelles de migration
//-----------------------------------------------------------------------------
int LogMessage(char *szLogMessage, ...)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	HANDLE hf=INVALID_HANDLE_VALUE; 
	DWORD dw;
	int len;
	SYSTEMTIME horodate;
	char szLog[2048];

	hf=CreateFile(gszLogFile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) { TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",gszLogFile,GetLastError())); goto end; }
	// se positionne à la fin
	SetFilePointer(hf,0,0,FILE_END);
	// en-tête : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=sprintf_s(szLog,sizeof(szLog),"%02d/%02d-%02d:%02d:%02d:%03d ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds);
	// log
	len+=vsprintf_s(szLog+len,sizeof(szLog)-len,szLogMessage,(char *)(&szLogMessage+1));
	// retour chariot
	memcpy(szLog+len,"\r\n\0",3);
	len+=2;
	WriteFile(hf,szLog,len,&dw,NULL);

	rc=0;
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf); 
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swSSOMigrationPart1() 
//-----------------------------------------------------------------------------
// récupération user et domaine
// lecture des sels dans le .ini
// envoi des sels à swSSOSVC (PUTPSKS)
// demande les clés swSSOSVC (GETPHKD)
// demande du mot de passe à swSSOSVC (GETPASS)
// déchiffrement du mot de passe
// rechiffrement du mot de passe (CRYPTPROTECTMEMORY_CROSS_PROCESS)
//-----------------------------------------------------------------------------
int swSSOMigrationPart1()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char bufRequest[1024];
	char bufResponse[1024];
	DWORD dwLenResponse;
	BYTE AESKeyData[AES256_KEY_LEN];
	char *pszPassword=NULL;

	// récupère user et domaine
	if (GetUserAndDomain()!=0) { LogMessage("ERROR : Utilisateur ou domaine introuvable"); goto end; }
	// lecture des sels dans le .ini
	if (swReadPBKDF2Salt()!=0) { LogMessage("ERROR : Fichier %s absent ou corrompu",gszCfgFile); goto end; }
	LogMessage("INFO  : Fichier .ini trouve");
	// Envoie les sels à swSSOSVC : V02:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets)
	LogMessage("INFO  : Communication avec swSSOSVC 1/3");
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V02:PUTPSKS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN*2,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	LogMessage("INFO  : Communication avec swSSOSVC 1/3 -- OK");
	// Demande le keydata à swSSOSVC : V02:GETPHKD:CUR:domain(256octets)username(256octets)
	LogMessage("INFO  : Communication avec swSSOSVC 2/3");
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:GETPHKD:CUR:",16);
	memcpy(bufRequest+16,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+16+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,16+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	if (dwLenResponse!=PBKDF2_PWD_LEN+AES256_KEY_LEN)
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"dwLenResponse=%ld (attendu=%d)",dwLenResponse,PBKDF2_PWD_LEN+AES256_KEY_LEN)); goto end;
	}
	LogMessage("INFO  : Communication avec swSSOSVC 2/3 -- OK");

	// Crée la clé de chiffrement des mots de passe secondaires
	memcpy(AESKeyData,bufResponse+PBKDF2_PWD_LEN,AES256_KEY_LEN);
	swCreateAESKeyFromKeyData(AESKeyData,&ghKey1);
	// Demande le mot de passe à swSSOSVC : V02:GETPASS:domain(256octets)username(256octets)
	LogMessage("INFO  : Communication avec swSSOSVC 3/3");
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:GETPASS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN,bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); goto end;
	}
	// en retour, on a reçu le mot de passe chiffré par la clé dérivée du mot de passe (si, si)
	if (dwLenResponse!=LEN_ENCRYPTED_AES256)
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"Longueur reponse attendue=LEN_ENCRYPTED_AES256=%d, recue=%d",LEN_ENCRYPTED_AES256,dwLenResponse)); goto end;
	}
	LogMessage("INFO  : Communication avec swSSOSVC 3/3 -- OK");
	bufResponse[dwLenResponse]=0;
	// déchiffre le mot de passe
	pszPassword=swCryptDecryptString(bufResponse,ghKey1);
	if (pszPassword==NULL) { LogMessage("ERROR : Impossible de recuperer le mot de passe"); goto end; }
	SecureZeroMemory(gBufPassword,sizeof(gBufPassword));
	strcpy_s(gBufPassword,sizeof(gBufPassword),pszPassword);
	SecureZeroMemory(pszPassword,strlen(pszPassword));
	//TRACE_BUFFER((TRACE_PWD,_F_,(unsigned char*)gBufPassword,sizeof(gBufPassword),"gBufPassword"));
	// rechiffre le mot de passe pour le repasser à SVC dans l'étape 2
	if (swProtectMemory(gBufPassword,sizeof(gBufPassword),CRYPTPROTECTMEMORY_CROSS_PROCESS)!=0){ LogMessage("ERROR : Impossible de chiffrer le mot de passe"); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gBufPassword,sizeof(gBufPassword),"gBufPassword"));
	
	rc=0;
end:
	if (pszPassword!=NULL) free(pszPassword);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swSSOMigrationPart2() 
//-----------------------------------------------------------------------------
// envoie PUTPASS (domaine, user, password) à swSSOSVC
//-----------------------------------------------------------------------------
int swSSOMigrationPart2()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	char bufRequest[1280];
	char bufResponse[1024];
	DWORD dwLenResponse;

	LogMessage("INFO  : Communication avec swSSOSVC 1/1");
	// ISSUE#384 : commence par essayer en V03 (swSSOSVC 1.18+)
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	memcpy(bufRequest,"V03:PUTPASS:",12);
	memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
	memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
	memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN*2,gBufPassword,sizeof(gBufPassword));
	
	// Envoie la requête
	if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN*2+sizeof(gBufPassword),bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
	{
		LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); 
		goto end;
	}
	// ISSUE#384 : teste le retour, si BADREQUEST ré-essaie en V02
	if (dwLenResponse==10 && memcmp(bufResponse,"BADREQUEST",10)==0)
	{
		SecureZeroMemory(bufRequest,sizeof(bufRequest));
		memcpy(bufRequest,"V02:PUTPASS:",12);
		memcpy(bufRequest+12,gpszRDN,strlen(gpszRDN)+1);
		memcpy(bufRequest+12+DOMAIN_LEN,gszUserName,strlen(gszUserName)+1);
		memcpy(bufRequest+12+DOMAIN_LEN+USER_LEN,gBufPassword,sizeof(gBufPassword));
		// Envoie la requête
		if (swPipeWrite(bufRequest,12+DOMAIN_LEN+USER_LEN+sizeof(gBufPassword),bufResponse,sizeof(bufResponse),&dwLenResponse)!=0) 
		{
			LogMessage("ERROR : Le service swSSOSVC ne repond pas"); 
			TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); 
			goto end;
		}
	}
	if (dwLenResponse!=2)
	{
		LogMessage("ERROR : Le service swSSOSVC a retourne une erreur");
		TRACE((TRACE_ERROR,_F_,"Erreur swPipeWrite()")); 
		goto end;
	}
	LogMessage("INFO  : Communication avec swSSOSVC 1/1 -- OK");
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// WinMain() 
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(nCmdShow);
	UNREFERENCED_PARAMETER(hPrevInstance);
	TRACE_OPEN();
	TRACE((TRACE_ENTER,_F_, ""));
	
	int rc=-1;
	int lenCmdLine;
	HANDLE hMutex=NULL;
	MSG msg;
	HWND gwMain=NULL;
	char *p;
	BOOL bLogAtTheEnd=TRUE;

	ghInstance=hInstance;
	gpSid=NULL;
	gpszRDN=NULL;

	// ligne de commande
	lenCmdLine=strlen(lpCmdLine);
	if (lenCmdLine==0 || lenCmdLine>_MAX_PATH*2) { TRACE((TRACE_ERROR,_F_,"lenCmdLine=%d",lenCmdLine)); goto end; }
	TRACE((TRACE_INFO,_F_,"lenCmdLine=%d lpCmdLine=%s",lenCmdLine,lpCmdLine));
	p=strstr(lpCmdLine,".ini ");
	if (p==NULL) { TRACE((TRACE_ERROR,_F_,"Pas de .ini en parametre : lpCmdLine=%s",lpCmdLine)); goto end; }
	p+=4;
	TRACE((TRACE_DEBUG,_F_,"lenCmdLine=%d (p-lpCmdLine)=%d",lenCmdLine,p-lpCmdLine));
	if (lenCmdLine<=(p-lpCmdLine)) { TRACE((TRACE_ERROR,_F_,"Pas de fichier log en parametre : lpCmdLine=%s",lpCmdLine)); goto end; }
	*p=0;
	p++;
	TRACE((TRACE_INFO,_F_,".ini=%s",lpCmdLine));
	TRACE((TRACE_INFO,_F_,"Log =%s",p));

	// récupère le nom du .ini en paramètre
	ExpandFileName(lpCmdLine,gszCfgFile,_MAX_PATH+1);
	TRACE((TRACE_INFO,_F_,"gszCfgFile=%s",gszCfgFile));
	// récupère le nom du log en paramètre
	ExpandFileName(p,gszLogFile,_MAX_PATH+1);
	TRACE((TRACE_INFO,_F_,"gszLogFile=%s",gszLogFile));

	// enregistrement des messages pour réception de paramètres en ligne de commande quand déjà
	guiContinueMsg=RegisterWindowMessage("swssomigration-continue");
	if (guiContinueMsg==0)	{ TRACE((TRACE_ERROR,_F_,"RegisterWindowMessage(swssomigration-continue)=%d",GetLastError())); }
	TRACE((TRACE_INFO,_F_,"RegisterWindowMessage(swssomigration-continue) OK -> msg=0x%08lx",guiContinueMsg));

	// vérif pas déjà lancé
	hMutex=CreateMutex(NULL,TRUE,"swSSOMigration.exe");
	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		TRACE((TRACE_INFO,_F_,"Demande à l'instance précédente de continuer"));
		PostMessage(HWND_BROADCAST,guiContinueMsg,0,0);
		bLogAtTheEnd=FALSE;
		goto end;
	}

	LogMessage("================================================================================");
	LogMessage("INFO  : Demarrage de swSSOMigration");
	LogMessage("INFO  : Fichier .ini : %s",gszCfgFile);
	LogMessage("INFO  : Fichier log  : %s",gszLogFile);
		
	// initalise la crypto
	if (swCryptInit()!=0) goto end;
	if (swProtectMemoryInit()!=0) goto end;

	// création fenêtre technique pour réception des messages
	gwMain=CreateMainWindow();

	LogMessage("INFO  : Debut migration partie 1/2 (recup infos du service)");

	// première partie de la migration
	rc=swSSOMigrationPart1();
	if (rc!=0) { LogMessage("INFO  : Fin migration partie 1/2 (recup infos du service) -- KO"); goto end; }
	TRACE((TRACE_INFO,_F_,"swSSOMigrationPart1 termine, se met en attente"));
	LogMessage("INFO  : Fin migration partie 1/2 (recup infos du service) -- OK");
	LogMessage("INFO  : En attente debut migration partie 2/2 (envoi des infos au service)");

	// boucle de message pour attendre le signal pour la 2nde partie de la migration
	while((rc=GetMessage(&msg,NULL,0,0))!=0)
    { 
		if (rc!=-1)
	    {
			if (msg.message==guiContinueMsg) 
			{
				TRACE((TRACE_INFO,_F_,"Message recu : swssomigration-continue (0x%08lx)",guiContinueMsg));
				LogMessage("INFO  : Debut migration partie 2/2 (envoi des infos au service)");
				rc=swSSOMigrationPart2();
				if (rc==0)
					LogMessage("INFO  : Fin migration partie 2/2 (envoi des infos au service) -- OK");
				else
					LogMessage("INFO  : Fin migration partie 2/2 (envoi des infos au service) -- KO"); 
				goto end;
			}
			else
			{
		        TranslateMessage(&msg); 
		        DispatchMessage(&msg); 
			}
	    }
	}

	rc=0;
end:
	if (bLogAtTheEnd)
	{
		if (rc==0)
			LogMessage("INFO  : Arret de swSSOMigration -- Migration OK");
		else
			LogMessage("INFO  : Arret de swSSOMigration -- Migration KO");
	}
	swCryptDestroyKey(ghKey1);
	swCryptTerm();
	swProtectMemoryTerm();
	if (gwMain!=NULL) DestroyWindow(gwMain); 
	if (gpSid!=NULL) free(gpSid);
	if (gpszRDN!=NULL) free(gpszRDN);
	if (hMutex!=NULL) ReleaseMutex(hMutex);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	TRACE_CLOSE();
	return rc; 
}