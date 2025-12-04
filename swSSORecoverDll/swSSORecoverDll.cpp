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
#include "stdafx.h"

const char gcszBeginChallenge[]="---swSSO CHALLENGE---";
const char gcszEndChallenge[]="---swSSO CHALLENGE---";

const char gcszBeginResponse[]="---swSSO RESPONSE---";
const char gcszEndResponse[]="---swSSO RESPONSE---";

//-----------------------------------------------------------------------------
// RecoveryGetResponse()
//-----------------------------------------------------------------------------
// [in] szFormattedChallenge : Challenge
// [in] szDomainUsername	 : Identifiant et domaine de l'utilisateur sous la forme domaine\identifiant 
// [out] szFormattedResponse : Réponse (buffer alloué et libéré par l'appelant)
// [in] iMaxCount			 : Taille du buffer alloué par l'appelant recevoir la réponse. Taille à prévoir : 257 octets – 256 + 0 de fin de chaine.
//-----------------------------------------------------------------------------
SWSSORECOVERDLL_API int RecoveryGetResponse(
		const char *szFormattedChallenge,
		const char *szDomainUserName,
		char *szFormattedResponse,
		int iMaxCount)
{
	TRACE_OPEN();
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=ERR_OTHER;
	
	int lenFormattedChallenge;
	int lenFormattedResponse;
	char szChallenge[2048];
	char szFormattedChallengeCopy[2048];
	BYTE Challenge[512];		//  (AESKeyData+UserId)Kpub + (Ks)Kpub
	BYTE *pChallengePart1;		// (AESKeyData+UserId)Kpub
	BYTE *pChallengePart2;		// (Ks)Kpub
	BYTE *pDecryptedChallengePart1 = NULL; // (AESKeyData+UserId)
	BYTE *pDecryptedChallengePart2 = NULL; // Ks
	DWORD lenDecryptedChallengePart1;
	DWORD lenDecryptedChallengePart2;
	DWORD lenConfigFile;
	char *p;
	int i;
	char bufKeyId[5];
	int iKeyId;
	HCRYPTKEY hKs = NULL;
	char *pszCoreResponse = NULL;
	int ret;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataSalt;
	char szClearKeystorePwd[100];
	char szEncryptedKeystorePwd[512];
	char EncryptedKeystorePwd[256];
	char szKeystoreFile[1024]; 
	char szConfigFile[1024];
	DWORD dwEncryptedKeystorePwdLen;
	char *pszUsername;
	HANDLE hMutex=NULL;
	DWORD dwWaitForSingleObject;
	
	hMutex=CreateMutex(NULL,TRUE,"Global\\swSSORecoverDll.dll");
	if (hMutex==NULL) { TRACE((TRACE_INFO,_F_, "CreateMutex(swSSORecoverDll)=%d",GetLastError())); goto end; }
	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		TRACE((TRACE_INFO,_F_, "(Mutex) Autre operation en cours, attente..."));
		dwWaitForSingleObject=WaitForSingleObject(hMutex,2000);
		if (dwWaitForSingleObject==WAIT_OBJECT_0)
		{
			TRACE((TRACE_INFO,_F_, "(Mutex) Autre operation terminee"));
		}
		else if (dwWaitForSingleObject==WAIT_ABANDONED)
		{
				TRACE((TRACE_ERROR,_F_, "(Mutex) Abandon attente fin operation")); 
				goto end;
		}
		else 
		{
			TRACE((TRACE_INFO,_F_, "(Mutex) WaitForSingleObject=%d",dwWaitForSingleObject));
			goto end;
		}
	}	
	TRACE((TRACE_INFO,_F_, "Mutex pris"));
	// trace et vérification des paramètres
	TRACE((TRACE_INFO,_F_, "iMaxCount=%d",iMaxCount));
	if (szDomainUserName==NULL) { TRACE((TRACE_ERROR,_F_, "szDomainUserName=NULL")); goto end; }
	TRACE((TRACE_INFO,_F_, "szDomainUserName=%s",szDomainUserName));
	if (szFormattedChallenge == NULL) { TRACE((TRACE_ERROR, _F_, "szFormattedChallenge=NULL")); goto end; }
	TRACE((TRACE_INFO, _F_, "szFormattedChallenge=%s", szFormattedChallenge));
	if (szFormattedResponse == NULL) { TRACE((TRACE_ERROR, _F_, "szFormattedResponse=NULL")); goto end; }
	
	*szFormattedResponse=0;
	
	if (swCryptInit()!=0) goto end;

	// reconstitue le chemin complet du fichier de configuration
	lenConfigFile=GetModuleFileName(ghModule,szConfigFile,sizeof(szConfigFile));
	if (lenConfigFile<10) { TRACE((TRACE_ERROR, _F_, "GetModuleFileName() error")); }
	memcpy(szConfigFile+lenConfigFile-3,"ini",3);
	TRACE((TRACE_INFO, _F_, "szConfigFile=%s", szConfigFile));

	// lit les infos du keystore dans le fichier de configuration
	GetPrivateProfileString("Keystore","Filename","",szKeystoreFile,sizeof(szKeystoreFile),szConfigFile);
	if (*szKeystoreFile==0) { TRACE((TRACE_ERROR, _F_, "Keystore filename not found in %s", szConfigFile)); rc=ERR_CONFIG_NOT_FOUND; goto end; }
	GetPrivateProfileString("Keystore","Password","",szEncryptedKeystorePwd,sizeof(szEncryptedKeystorePwd),szConfigFile);
	if (*szEncryptedKeystorePwd==0) { TRACE((TRACE_ERROR, _F_, "Keystore password not found in %s", szConfigFile)); rc=ERR_CONFIG_NOT_FOUND; goto end; }
	TRACE((TRACE_INFO,_F_,"szEncryptedKeystorePwd=%s",szEncryptedKeystorePwd));

	dwEncryptedKeystorePwdLen=(DWORD)strlen(szEncryptedKeystorePwd)/2;
	if (swCryptDecodeBase64(szEncryptedKeystorePwd,EncryptedKeystorePwd,dwEncryptedKeystorePwdLen)!=0) goto end;

	// déchiffre le mot de passe du keystore
	DataOut.pbData=NULL;
	DataOut.cbData=0;
	DataSalt.pbData=NULL;
	DataSalt.cbData=0;
	DataIn.pbData=(BYTE*)EncryptedKeystorePwd;
	DataIn.cbData=dwEncryptedKeystorePwdLen;
	if (!CryptUnprotectData(&DataIn,NULL,&DataSalt,NULL,NULL,0,&DataOut))
	{
		TRACE((TRACE_ERROR,_F_, "CryptUnprotectData()")); rc=ERR_KEYSTORE_BAD_PWD; goto end;
	}
	DataOut.pbData[DataOut.cbData]=0;
	strcpy_s(szClearKeystorePwd,sizeof(szClearKeystorePwd),(char*)DataOut.pbData);
	//TRACE((TRACE_PWD,_F_,"szClearKeystorePwd=%s",szClearKeystorePwd));

	// extraction du coeur du challenge
	lenFormattedChallenge=(int)strlen(szFormattedChallenge);
	if (lenFormattedChallenge>(sizeof(szFormattedChallengeCopy)-1)) 
	{
		TRACE((TRACE_ERROR,_F_,"Challenge trop long (%d)",lenFormattedChallenge)); rc=ERR_BAD_CHALLENGE; goto end;
	}
	strcpy_s(szFormattedChallengeCopy,sizeof(szFormattedChallengeCopy),szFormattedChallenge);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)szFormattedChallengeCopy,lenFormattedChallenge,"szFormattedChallengeCopy"));
	// supprimer les éventuels espaces qui trainent à la fin
	while (szFormattedChallengeCopy[lenFormattedChallenge-1]==' ') { lenFormattedChallenge--; szFormattedChallengeCopy[lenFormattedChallenge]=0; }
	// vérifie les balises début et fin
	if (memcmp(szFormattedChallengeCopy,gcszBeginChallenge,strlen(gcszBeginChallenge))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de début de challenge non trouvée")); rc=ERR_BAD_CHALLENGE; goto end;
	}
	if (memcmp(szFormattedChallengeCopy+lenFormattedChallenge-strlen(gcszEndChallenge),gcszEndChallenge,strlen(gcszEndChallenge))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de fin de challenge non trouvée")); rc=ERR_BAD_CHALLENGE; goto end;
	}
	// extrait le key id
	p=szFormattedChallengeCopy+strlen(gcszBeginChallenge);
	while (*p==0x0a || *p==0x0d) p++;
	memcpy(bufKeyId,p,4); 
	bufKeyId[4]=0;
	iKeyId=atoi(bufKeyId);
	TRACE((TRACE_INFO,_F_,"iKeyId=%d",iKeyId));
	// passe le séparateur :
	p+=4;
	if (*p!=':')
	{
		TRACE((TRACE_ERROR,_F_,"Séparateur : entre keyid et challenge non trouvé")); rc=ERR_BAD_CHALLENGE; goto end;
	}
	p++;
	// extrait le challenge
	i=0;
	while (*p!='-')
	{
		if (*p!=0x0a && *p!=0x0d) { szChallenge[i]=*p; i++; }
		p++;
	}
	szChallenge[i]=0;
	if (i!=1024)
	{
		TRACE_BUFFER((TRACE_ERROR,_F_,(BYTE*)szChallenge,i,"Taille de challenge attendue=1024, lue=%d",i)); rc=ERR_BAD_CHALLENGE; goto end;
	}
	// décode base 64 le challenge
	if (swCryptDecodeBase64(szChallenge,(char*)Challenge,512)!=0) goto end;
	pChallengePart1=Challenge;		//  (AESKeyData+UserId)Kpub
	pChallengePart2=Challenge+256;	//  (Ks)Kpub
	TRACE_BUFFER((TRACE_DEBUG,_F_,pChallengePart1,256,"ChallengePart1")); 
	TRACE_BUFFER((TRACE_DEBUG,_F_,pChallengePart2,256,"ChallengePart2"));
	
	// charge le keystore
	ret=swKeystoreLoad(szKeystoreFile);
	if (ret!=0)
	{
		TRACE((TRACE_ERROR,_F_,"swKeystoreLoad(%s)=%d",szKeystoreFile,ret));
		if (ret==SWCRYPT_FILENOTFOUND) rc=ERR_KEYSTORE_NOT_FOUND;
		else if (ret==SWCRYPT_FILEREAD) rc=ERR_KEYSTORE_CORRUPTED;
		goto end;
	}

	// déchiffre les 2 parties du challenge
	if (swCryptDecryptDataRSA(iKeyId,szClearKeystorePwd,pChallengePart1,256,&pDecryptedChallengePart1,&lenDecryptedChallengePart1)!=0) { rc=ERR_KEYSTORE_BAD_PWD; goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart1,lenDecryptedChallengePart1,"pDecryptedChallengePart1")); 
	if (swCryptDecryptDataRSA(iKeyId,szClearKeystorePwd,pChallengePart2,256,&pDecryptedChallengePart2,&lenDecryptedChallengePart2)!=0) { rc=ERR_KEYSTORE_BAD_PWD; goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart2,lenDecryptedChallengePart2,"pDecryptedChallengePart2")); 
	
	// compare le nom de l'utilisateur
	pszUsername=(char*)strchr(szDomainUserName,'\\');
	if (pszUsername==NULL) 
	{
		TRACE((TRACE_ERROR,_F_,"Le nom du user n'est pas au format domaine\\user : %s",szDomainUserName));
		rc=ERR_BAD_USER;
		goto end;

	}
	pszUsername++;
	if (_stricmp((char*)(pChallengePart1+AES256_KEY_LEN),pszUsername)!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Utilisateur recu en parametre=%s",szDomainUserName)); 
		TRACE((TRACE_ERROR,_F_,"Utilisateur dans le challenge=%s",(char*)(pChallengePart1+AES256_KEY_LEN)));
		rc=ERR_BAD_USER;
		goto end;
	}
	// récupère Ks dans la partie 2 du challenge
	//if (swCreateAESKeyFromKeyData(pDecryptedChallengePart2,&hKs)!=0) goto end; // TODO : comprendre pourquoi c'est en commentaire !
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pDecryptedChallengePart2,lenDecryptedChallengePart2,"Ks (len=%d)",lenDecryptedChallengePart2)); 
	if (!CryptImportKey(ghProv,pDecryptedChallengePart2,lenDecryptedChallengePart2,NULL,0,&hKs))
	{ TRACE((TRACE_ERROR,_F_,"CryptImportKey()=%ld",GetLastError())); goto end; }

	// chiffre AESKeyData avec Ks
	pszCoreResponse=swCryptEncryptAES(pDecryptedChallengePart1,AES256_KEY_LEN,hKs);
	if (pszCoreResponse==NULL) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pszCoreResponse,(int)strlen(pszCoreResponse),"pszCoreResponse")); 
	
	// calcule la longueur de la réponse
	lenFormattedResponse=(int)strlen(gcszBeginResponse)+(int)strlen(pszCoreResponse)+(int)strlen(gcszEndResponse);
	if (lenFormattedResponse+1>iMaxCount)
	{
		TRACE((TRACE_ERROR,_F_,"Buffer reponse trop court=%d (%d necessaire)",iMaxCount,lenFormattedResponse+1)); 
		rc=ERR_BUFFER_TOO_SMALL;
		goto end;
	}

	// formatte la response
	strcpy_s(szFormattedResponse,iMaxCount,gcszBeginResponse);
	strcat_s(szFormattedResponse,iMaxCount,pszCoreResponse);
	strcat_s(szFormattedResponse,iMaxCount,gcszEndResponse);	
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szFormattedResponse,(int)strlen(szFormattedResponse),"szFormattedResponse")); 
	
	rc=0;
end:
	SecureZeroMemory(szClearKeystorePwd,sizeof(szClearKeystorePwd));
	swCryptTerm();
	if (hMutex!=NULL)
	{
		ReleaseMutex(hMutex);
		TRACE((TRACE_INFO,_F_, "Mutex relache"));
		CloseHandle(hMutex);
	}
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	TRACE_CLOSE();
	return rc;
}
