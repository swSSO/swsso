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

const char gcszBeginChallenge[]="---swSSO CHALLENGE---";
const char gcszEndChallenge[]="---swSSO CHALLENGE---";

const char gcszBeginResponse[]="---swSSO RESPONSE---";
const char gcszEndResponse[]="---swSSO RESPONSE---";

char gszKeystorePwd[]="toto"; // POUR TEST
char gszKeystoreFile[]="c:\\swSSO.Recover\\swSSO-Keystore.txt" ; // POUR TEST

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
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=ERR_OTHER;
	
	int lenFormattedChallenge;
	char szChallenge[2048];
	char szFormattedChallengeCopy[2048];
	BYTE Challenge[512];		//  (AESKeyData+UserId)Kpub + (Ks)Kpub
	BYTE *pChallengePart1;		// (AESKeyData+UserId)Kpub
	BYTE *pChallengePart2;		// (Ks)Kpub
	BYTE *pDecryptedChallengePart1 = NULL; // (AESKeyData+UserId)
	BYTE *pDecryptedChallengePart2 = NULL; // Ks
	DWORD lenDecryptedChallengePart1;
	DWORD lenDecryptedChallengePart2;
	char *p;
	int i;
	char bufKeyId[5];
	int iKeyId;
	HCRYPTKEY hKs = NULL;
	char *pszCoreResponse = NULL;
	int ret;

	// trace et vérification des paramètres
	TRACE((TRACE_INFO,_F_, "iMaxCount=%d",iMaxCount));
	if (szDomainUserName==NULL) { TRACE((TRACE_ERROR,_F_, "szDomainUserName=NULL")); goto end; }
	TRACE((TRACE_INFO,_F_, "szDomainUserName=%s",szDomainUserName));
	if (szFormattedChallenge == NULL) { TRACE((TRACE_ERROR, _F_, "szFormattedChallenge=NULL")); goto end; }
	TRACE((TRACE_INFO, _F_, "szFormattedChallenge=%s", szFormattedChallenge));
	if (szFormattedResponse == NULL) { TRACE((TRACE_ERROR, _F_, "szFormattedResponse=NULL")); goto end; }
	
	// extraction du coeur du challenge
	lenFormattedChallenge=strlen(szFormattedChallenge);
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
	ret=swKeystoreLoad(gszKeystoreFile);
	if (ret!=0)
	{
		TRACE((TRACE_ERROR,_F_,"swKeystoreLoad(%s)=%d",gszKeystoreFile,ret));
		goto end;
	}

	// déchiffre les 2 parties du challenge
	if (swCryptDecryptDataRSA(iKeyId,gszKeystorePwd,pChallengePart1,256,&pDecryptedChallengePart1,&lenDecryptedChallengePart1)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart1,lenDecryptedChallengePart1,"pDecryptedChallengePart1")); 
	if (swCryptDecryptDataRSA(iKeyId,gszKeystorePwd,pChallengePart2,256,&pDecryptedChallengePart2,&lenDecryptedChallengePart2)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,pDecryptedChallengePart2,lenDecryptedChallengePart2,"pDecryptedChallengePart2")); 
	
	// affiche le nom de l'utilisateur et demande confirmation
	if (_stricmp((char*)(pChallengePart1+AES256_KEY_LEN),szDomainUserName)!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Utilisateur recu en paramètre=%s",szDomainUserName)); 
		TRACE((TRACE_ERROR,_F_,"Utilisateur dans le challenge=%s",(char*)(pChallengePart1+AES256_KEY_LEN)));
		goto end;
	}
	// récupère Ks dans la partie 2 du challenge
	//if (swCreateAESKeyFromKeyData(pDecryptedChallengePart2,&hKs)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pDecryptedChallengePart2,lenDecryptedChallengePart2,"Ks (len=%d)",lenDecryptedChallengePart2)); 
	if (!CryptImportKey(ghProv,pDecryptedChallengePart2,lenDecryptedChallengePart2,NULL,0,&hKs))
	{ TRACE((TRACE_ERROR,_F_,"CryptImportKey()=%ld",GetLastError())); goto end; }

	// chiffre AESKeyData avec Ks
	pszCoreResponse=swCryptEncryptAES(pDecryptedChallengePart1,AES256_KEY_LEN,hKs);
	if (pszCoreResponse==NULL) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pszCoreResponse,strlen(pszCoreResponse),"pszCoreResponse")); 
	// formatte la response
	strcpy_s(szFormattedResponse,2048,gcszBeginResponse);
	strcat_s(szFormattedResponse,2048,"\n");
	for (i=0;i<(int)strlen(pszCoreResponse);i+=64)
	{
		strncat_s(szFormattedResponse,2048,pszCoreResponse+i,64);
		strcat_s(szFormattedResponse,2048,"\n");
	}
	strcat_s(szFormattedResponse,2048,gcszEndResponse);	

	/* bouchon (début)
	*szResponse=0;
	strcpy_s(szResponse,iMaxCount,"---swSSO RESPONSE---9876543210ABCDEFGHI---swSSO RESPONSE---");
	TRACE((TRACE_INFO,_F_, "szResponse=%s",szResponse));
	bouchon (fin) */

	rc=0;
end:
	if (pDecryptedChallengePart1 != NULL) free(pDecryptedChallengePart1);
	if (pDecryptedChallengePart2 != NULL) free(pDecryptedChallengePart2);
	if (hKs != NULL) CryptDestroyKey(hKs);
	TRACE((TRACE_LEAVE, _F_, "rc=%d", rc));
	return rc;
}
