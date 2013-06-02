#include "stdafx.h"
#define PWD_LEN 256
//-----------------------------------------------------------------------------
// swBuildAndSendRequest()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swBuildAndSendRequest(LPCWSTR lpAuthentInfoType,LPVOID lpAuthentInfo)
{
	TRACE((TRACE_ENTER,_F_,""));

	int rc=-1;
	int ret;
	UNICODE_STRING usUserName,usPassword;
	char szUserName[255+1];
	char bufPassword[PWD_LEN];
	char bufRequest[1024];
	int lenRequest=0;
	
	
	// Récupération des authentifiants en fonction de la méthode d'authent
	TRACE((TRACE_DEBUG,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
	if (wcscmp(lpAuthentInfoType,L"MSV1_0:Interactive")==0)
	{
		usUserName=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->UserName;
		usPassword=((MSV1_0_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
	}
	else if (wcscmp(lpAuthentInfoType,L"Kerberos:Interactive")==0)
	{
		usUserName=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->UserName;
		usPassword=((KERB_INTERACTIVE_LOGON*)lpAuthentInfo)->Password;
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"lpAuthentInfoType=%S",lpAuthentInfoType));
		goto end;
	}

	// Conversion de chaînes UNICODE_STRING
	memset(szUserName,0,sizeof(szUserName));
	memset(bufPassword,0,sizeof(bufPassword));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)usUserName.Buffer,usUserName.Length,"usUserName"));
	TRACE_BUFFER((TRACE_PWD,_F_,(unsigned char*)usPassword.Buffer,usPassword.Length,"usPassword"));
	TRACE((TRACE_DEBUG,_F_,"usPassword.MaximumLength=%d",usPassword.MaximumLength));
	ret=WideCharToMultiByte(CP_ACP,0,usPassword.Buffer,usPassword.Length/2,bufPassword,sizeof(bufPassword),NULL,NULL);
	SecureZeroMemory(usPassword.Buffer,usPassword.MaximumLength);
	if (ret==0) { TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usPassword)=%d",GetLastError())); goto end; }
	ret=WideCharToMultiByte(CP_ACP,0,usUserName.Buffer,usUserName.Length/2,szUserName,sizeof(szUserName),NULL,NULL);
	if (ret==0)	{ TRACE((TRACE_ERROR,_F_,"WideCharToMultiByte(usUserName)=%d",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"szUserName=%s",szUserName));
	TRACE((TRACE_PWD,_F_,"bufPassword=%s",bufPassword));

	// Chiffre le mot de passe
	if (swProtectMemoryInit()!=0) goto end;
	if (swProtectMemory(bufPassword,sizeof(bufPassword),CRYPTPROTECTMEMORY_SAME_LOGON)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufPassword,PWD_LEN,"bufPassword"));

	// Construit la requête à envoyer à swSSOSVC : V01:PUTPASS:password
	memcpy(bufRequest,"V01:PUTPASS:",12);
	memcpy(bufRequest+12,bufPassword,PWD_LEN);
	lenRequest=PWD_LEN+12;

	// Envoie la requête
	rc=swPipeWrite(bufRequest,lenRequest);
	if (rc!=0) goto end; 
		
	rc=0;
end:
	swProtectMemoryTerm();
	SecureZeroMemory(bufPassword,sizeof(bufPassword));
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swPipeWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swPipeWrite(char *bufRequest,int lenRequest)
{
	HANDLE hPipe=INVALID_HANDLE_VALUE;
	int rc=-1;
	char bufResponse[1024];
    DWORD cbRead,cbWritten;
	int iNbTry;

	TRACE((TRACE_ENTER,_F_,""));

	// Ouverture du pipe créé par le service swssosvc
	iNbTry=0;
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	while (hPipe==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%d (SVC pas prêt - essai %d)",GetLastError(),iNbTry));
		if (iNbTry>30) goto end;
		Sleep(1000);
		iNbTry++;
		hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	}
	
	// Envoi de la requête
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,lenRequest,"Request to write"));
	if (!WriteFile(hPipe,bufRequest,lenRequest,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenRequest,GetLastError()));
		goto end;
	} 

	// Lecture la réponse
	if (!ReadFile(hPipe,bufResponse,sizeof(bufResponse),&cbRead,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeof(bufResponse),GetLastError()));
		goto end;
	}  
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,cbRead,"Response"));

	// Vérification de la réponse
	// TODO

	rc=0;
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}