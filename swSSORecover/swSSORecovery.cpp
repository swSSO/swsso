//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2011 - Sylvain WERDEFROY
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
// swSSORecovery.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

const char gcszBeginChallenge[]="---swSSO CHALLENGE---";
const char gcszEndChallenge[]="---swSSO CHALLENGE---";

const char gcszBeginResponse[]="---swSSO RESPONSE---";
const char gcszEndResponse[]="---swSSO RESPONSE---";

#define ERR_CHALLENGE	1
#define ERR_CANCEL		2
#define ERR_OTHER		-1

char gbufMsg[2048];

//-----------------------------------------------------------------------------
// RecoveryChallenge()
//-----------------------------------------------------------------------------
// Analyse du Challenge et préparation de la Response
// szFormattedChallenge : gcszBeginChallenge + id clé + (AESKeyData+UserId)Kpub + (Ks)Kpub + gcszEndChallenge
// szFormattedResponse  : gcszBeginResponse + (AESKeyData)Ks + gcszEndResponse
//-----------------------------------------------------------------------------
int RecoveryChallenge(HWND w,char *szFormattedChallenge,char *szFormattedResponseForDisplay,char *szFormattedResponseForSave)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=ERR_CHALLENGE;
	int lenFormattedChallenge;
	char szChallenge[2048];
	BYTE Challenge[512];		//  (AESKeyData+UserId)Kpub + (Ks)Kpub
	BYTE *pChallengePart1;	// (AESKeyData+UserId)Kpub
	BYTE *pChallengePart2;	// (Ks)Kpub
	BYTE *pDecryptedChallengePart1=NULL; // (AESKeyData+UserId)
	BYTE *pDecryptedChallengePart2=NULL; // Ks
	DWORD lenDecryptedChallengePart1;
	DWORD lenDecryptedChallengePart2;
	char *p;
	int i;
	char bufKeyId[5];
	int iKeyId;
	HCRYPTKEY hKs=NULL;
	char *pszCoreResponse=NULL;

	lenFormattedChallenge=strlen(szFormattedChallenge);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)szFormattedChallenge,lenFormattedChallenge,"szFormattedChallenge"));
	// supprimer les éventuels espaces qui trainent à la fin
	while (szFormattedChallenge[lenFormattedChallenge-1]==' ') { lenFormattedChallenge--; szFormattedChallenge[lenFormattedChallenge]=0; }
	// vérifie les balises début et fin
	if (memcmp(szFormattedChallenge,gcszBeginChallenge,strlen(gcszBeginChallenge))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de début de challenge non trouvée")); goto end;
	}
	if (memcmp(szFormattedChallenge+lenFormattedChallenge-strlen(gcszEndChallenge),gcszEndChallenge,strlen(gcszEndChallenge))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de fin de challenge non trouvée")); goto end;
	}
	// extrait le key id
	p=szFormattedChallenge+strlen(gcszBeginChallenge);
	while (*p==0x0a || *p==0x0d) p++;
	memcpy(bufKeyId,p,4); 
	bufKeyId[4]=0;
	iKeyId=atoi(bufKeyId);
	TRACE((TRACE_INFO,_F_,"iKeyId=%d",iKeyId));
	// passe le séparateur :
	p+=4;
	if (*p!=':')
	{
		TRACE((TRACE_ERROR,_F_,"Séparateur : entre keyid et challenge non trouvé")); goto end;
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
		TRACE_BUFFER((TRACE_ERROR,_F_,(BYTE*)szChallenge,i,"Taille de challenge attendue=1024, lue=%d",i)); goto end;
	}
	// décode base 64 le challenge
	if (swCryptDecodeBase64(szChallenge,(char*)Challenge,512)!=0) goto end;
	pChallengePart1=Challenge;		//  (AESKeyData+UserId)Kpub
	pChallengePart2=Challenge+256;	//  (Ks)Kpub
	TRACE_BUFFER((TRACE_DEBUG,_F_,pChallengePart1,256,"ChallengePart1")); 
	TRACE_BUFFER((TRACE_DEBUG,_F_,pChallengePart2,256,"ChallengePart2"));
	
	// déchiffre les 2 parties du challenge
	if (swCryptDecryptDataRSA(iKeyId,gszKeystorePwd,pChallengePart1,256,&pDecryptedChallengePart1,&lenDecryptedChallengePart1)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart1,lenDecryptedChallengePart1,"pDecryptedChallengePart1")); 
	if (swCryptDecryptDataRSA(iKeyId,gszKeystorePwd,pChallengePart2,256,&pDecryptedChallengePart2,&lenDecryptedChallengePart2)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart2,lenDecryptedChallengePart2,"pDecryptedChallengePart2")); 
	
	// affiche le nom de l'utilisateur et demande confirmation
	wsprintf(gbufMsg,GetString(IDS_CONFIRM_USER),(char*)(pChallengePart1+AES256_KEY_LEN));
	if (MessageBox(w,gbufMsg,"swSSO",MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK)
	{
		rc=ERR_CANCEL; goto end;
	}
	swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_RECOVERY_FORUSER,(char*)(pChallengePart1+AES256_KEY_LEN),NULL,NULL);
	// récupère Ks dans la partie 2 du challenge
	//if (swCreateAESKeyFromKeyData(pDecryptedChallengePart2,&hKs)!=0) goto end;
	if (!CryptImportKey(ghProv,pDecryptedChallengePart2,lenDecryptedChallengePart2,NULL,0,&hKs))
	{ TRACE((TRACE_ERROR,_F_,"CryptImportKey()=%ld",GetLastError())); goto end; }

	// chiffre AESKeyData avec Ks
	pszCoreResponse=swCryptEncryptAES(pDecryptedChallengePart1,AES256_KEY_LEN,hKs);
	if (pszCoreResponse==NULL) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pszCoreResponse,strlen(pszCoreResponse),"pszCoreResponse")); 
	// formatte la response
	strcpy_s(szFormattedResponseForDisplay,2048,gcszBeginResponse);
	strcat_s(szFormattedResponseForDisplay,2048,"\r\n");
	strcpy_s(szFormattedResponseForSave,2048,gcszBeginResponse);
	strcat_s(szFormattedResponseForSave,2048,"\n");
	for (i=0;i<(int)strlen(pszCoreResponse);i+=64)
	{
		strncat_s(szFormattedResponseForDisplay,2048,pszCoreResponse+i,64);
		strcat_s(szFormattedResponseForDisplay,2048,"\r\n");
		strncat_s(szFormattedResponseForSave,2048,pszCoreResponse+i,64);
		strcat_s(szFormattedResponseForSave,2048,"\n");
	}
	strcat_s(szFormattedResponseForDisplay,2048,gcszEndResponse);
	strcat_s(szFormattedResponseForSave,2048,gcszEndResponse);	

	rc=0;
end:
	if (rc==ERR_CHALLENGE) 
		MessageBox(w,GetString(IDS_ERROR_CHALLENGE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
	else if (rc!=0 && rc!=ERR_CANCEL)
		MessageBox(w,GetString(IDS_ERROR_RESPONSE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
	if (pDecryptedChallengePart1!=NULL) free(pDecryptedChallengePart1);
	if (pDecryptedChallengePart2!=NULL) free(pDecryptedChallengePart2);
	if (hKs!=NULL) CryptDestroyKey(hKs);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

