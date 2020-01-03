//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2020 - Sylvain WERDEFROY
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
// Toutes les fonctions li�es � la r�initialisation de mot de passe
//-----------------------------------------------------------------------------

#include "stdafx.h"

int giRecoveryKeyId=-1;
DWORD gdwRecoveryKeyLen=0;
BYTE *gpRecoveryKeyValue=NULL;

char gszRecoveryInfos[5+512+1]="";
int giRecoveryInfosKeyId=-1;

const char gcszBeginChallenge[]="---swSSO CHALLENGE---";
const char gcszEndChallenge[]="---swSSO CHALLENGE---";
const char gcszBeginResponse[]="---swSSO RESPONSE---";
const char gcszEndResponse[]="---swSSO RESPONSE---";
char gszFormattedChallengeForDisplay[2048];
char gszFormattedChallengeForSave[2048];
char gszFormattedChallengeForWebservice[2048];
char gszFormattedResponse[1024];

static int giRefreshTimer=10;

HFONT ghFontCourrier=NULL;

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

//-----------------------------------------------------------------------------
// RecoveryEncryptData()
//-----------------------------------------------------------------------------
// Chiffre les donn�es avec la cl� publique de recouvrement lue en registry
//-----------------------------------------------------------------------------
// [in] pData : bloc de donn�es � chiffrer
// [in] dwDataLen : taille des donn�es (attention, doit �tre <245, limite CSP Microsoft)
// [out] ppszEncryptedData : donn�es chiffr�es en string HEXA
//-----------------------------------------------------------------------------
static int RecoveryEncryptData(char *pData,DWORD dwDataLen,char **ppszEncryptedData)
{
	TRACE((TRACE_ENTER,_F_, "gpRecoveryKeyValue=0x%08lx",gpRecoveryKeyValue));
	int rc=-1;
	BOOL brc;
	HCRYPTKEY hPubKey=NULL;
	DWORD dwEncryptedDataLen;
	DWORD dwEncryptedBufSize;
	BYTE *pEncryptedData=NULL;

	if (gpRecoveryKeyValue==NULL) goto end;
	// attention, on chiffre directement avec la cl� publique, ce que nous autorise le CSP Microsoft
	// par contre il faut que la taille du bloc � chiffrer soit inf�rieure � taille de cl� - 11 octets,
	// soit dans notre cas : 2048 / 8 - 11 = 245.
	if (dwDataLen>245) { TRACE((TRACE_ERROR,_F_,"Impossible de chiffrer plus de 245 octets avec cette cle")); goto end; }

	// import de la cl� RSA lue en base de registre par LoadPolicies
	brc=CryptImportKey(ghProv,gpRecoveryKeyValue,gdwRecoveryKeyLen,0,0,&hPubKey);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }

	// chiffrement des donn�es
	dwEncryptedBufSize=dwDataLen;
	brc=CryptEncrypt(hPubKey,0,TRUE,0,NULL,&dwEncryptedBufSize,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptEncrypt(0)=0x%08lx",GetLastError())); goto end; }
	
	pEncryptedData=(BYTE*)malloc(dwEncryptedBufSize);
	if (pEncryptedData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwEncryptedBufSize)); goto end; }
	memcpy(pEncryptedData,pData,dwDataLen);

	dwEncryptedDataLen=dwDataLen;
	brc=CryptEncrypt(hPubKey,0,TRUE,0,pEncryptedData,&dwEncryptedDataLen,dwEncryptedBufSize);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptEncrypt(1)=0x%08lx",GetLastError())); goto end; }

	*ppszEncryptedData=(char*)malloc(dwEncryptedDataLen*2+1);
	if (*ppszEncryptedData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwEncryptedDataLen*2+1)); goto end; }

	swCryptEncodeBase64(pEncryptedData,dwEncryptedDataLen,*ppszEncryptedData);
	rc=0;
end:
	if (pEncryptedData!=NULL) free(pEncryptedData);
	if (hPubKey!=NULL) CryptDestroyKey(hPubKey);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

// ----------------------------------------------------------------------------------
// SaveChallenge()
// ----------------------------------------------------------------------------------
// Sauvegarde du challenge dans fichier texte
// ----------------------------------------------------------------------------------
// [in] pszFile : chemin complet du fichier
// [in] szChallenge : challenge format ASCII/HEXA
// ----------------------------------------------------------------------------------
static void SaveChallenge(const char *pszFile, const char *szChallenge)
{
	TRACE((TRACE_ENTER,_F_,"%s",pszFile));
	FILE *hf=NULL;

	if (pszFile==NULL || *pszFile==0) goto end;

	errno_t err=fopen_s(&hf,pszFile,"w");
	if (err!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Cannot open file for writing : '%s'",pszFile));
		goto end;
	}

	fputs(szChallenge,hf);
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// ChallengeDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de challenge/response
//-----------------------------------------------------------------------------
static int CALLBACK ChallengeDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_CHALLENGE),EM_LIMITTEXT,strlen(gszFormattedChallengeForDisplay),0);
			SetDlgItemText(w,TB_CHALLENGE,gszFormattedChallengeForDisplay);
			SendMessage(GetDlgItem(w,TB_CHALLENGE),EM_SETSEL,0,-1);
			SetFocus(GetDlgItem(w,TB_CHALLENGE));
			// titre en gras
			SetTextBold(w,TX_FRAME);
			SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
			ghFontCourrier=CreateFont(14,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Courier New");
			if (ghFontCourrier!=NULL) PostMessage(GetDlgItem(w,TB_CHALLENGE),WM_SETFONT,(LPARAM)ghFontCourrier,TRUE);
			MACRO_SET_SEPARATOR;
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
					EndDialog(w,IDOK);
					break;
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case PB_SAVEAS:
					{
						OPENFILENAME ofn;
						char szFile[_MAX_PATH+1];
						strcpy_s(szFile,sizeof(szFile),"swSSO-Challenge");
						ZeroMemory(&ofn,sizeof(OPENFILENAME));
						ofn.lStructSize=sizeof(OPENFILENAME);
						ofn.hwndOwner=w;
						ofn.hInstance=NULL;
						ofn.lpstrFilter="*.txt";
						ofn.lpstrCustomFilter=NULL;
						ofn.nMaxCustFilter=0;
						ofn.nFilterIndex=0;
						ofn.lpstrFile=szFile;
						ofn.nMaxFile=sizeof(szFile);
						ofn.lpstrFileTitle=NULL;
						ofn.nMaxFileTitle=NULL;
						ofn.lpstrInitialDir=NULL;
						ofn.lpstrTitle=NULL;
						ofn.Flags=OFN_PATHMUSTEXIST;
						ofn.nFileOffset;
						ofn.lpstrDefExt="txt";
						if (GetSaveFileName(&ofn)) 
						{
							SaveChallenge(szFile,gszFormattedChallengeForSave);
							EndDialog(w,IDOK);
						}
					}
					break;
				default:
					break;
				
			}
			break;

		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;

		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
		case WM_DESTROY:
			if (ghFontCourrier!=NULL) DeleteObject(ghFontCourrier);
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// OpenResponse()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int OpenResponse(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	OPENFILENAME ofn;
	char szFile[_MAX_PATH+1];
	FILE *hf=NULL;
	char szLine[100];
	int pos;
	int len;
	BOOL bFirst=TRUE;

	strcpy_s(szFile,sizeof(szFile),"swSSO-Response");
	ZeroMemory(&ofn,sizeof(OPENFILENAME));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=w;
	ofn.hInstance=NULL;
	ofn.lpstrFilter="*.txt";
	ofn.lpstrCustomFilter=NULL;
	ofn.nMaxCustFilter=0;
	ofn.nFilterIndex=0;
	ofn.lpstrFile=szFile;
	ofn.nMaxFile=sizeof(szFile);
	ofn.lpstrFileTitle=NULL;
	ofn.nMaxFileTitle=NULL;
	ofn.lpstrInitialDir=NULL;
	ofn.lpstrTitle=NULL;
	ofn.Flags=OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	ofn.nFileOffset;
	ofn.lpstrDefExt="txt";
	if (!GetOpenFileName(&ofn)) goto end;
	
	errno_t err=fopen_s(&hf,szFile,"r");
	if (err!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Cannot open file for : '%s'",szFile));
		goto end;
	}
	*gszFormattedResponse=0;
	pos=0;
	while (fgets(szLine,sizeof(szLine),hf)!=NULL)
	{
		if (bFirst)
		{
			bFirst=FALSE;
			TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szLine,strlen(gcszBeginResponse),"szLine"));		// on regarde tout de suite si on a l'ent�te, sinon out
			if (memcmp(szLine,gcszBeginResponse,strlen(gcszBeginResponse))!=0)
			{
				MessageBox(w,GetString(IDS_ERROR_BADRESPONSEFILE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
				TRACE((TRACE_ERROR,_F_,"Marque de d�but de response non trouv�e")); goto end;
			}
		}
		len=strlen(szLine);
		if (pos+len>1024) // 1.12B2-AC-TIE01
		{ 
				MessageBox(w,GetString(IDS_ERROR_BADRESPONSEFILE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
				TRACE((TRACE_ERROR,_F_,"Fichier trop grand, ce n'est pas une reponse (>1024)")); goto end;
		}
		memcpy(gszFormattedResponse+pos,szLine,len-1);
		pos+=len-1;
		gszFormattedResponse[pos]='\r'; pos++;
		gszFormattedResponse[pos]='\n'; pos++;
	}
	gszFormattedResponse[pos-2]='-';
	gszFormattedResponse[pos-1]=0;
	SetDlgItemText(w,TB_CHALLENGE,gszFormattedResponse);
	EnableWindow(GetDlgItem(w,IDOK),TRUE);
	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ResponseDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la boite de response
//-----------------------------------------------------------------------------
static int CALLBACK ResponseDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	CheckIfQuitMessage(msg);
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			// icone ALT-TAB
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_CHALLENGE),EM_LIMITTEXT,sizeof(gszFormattedResponse)-1,0);
			SetFocus(GetDlgItem(w,TB_CHALLENGE));
			// ISSUE#165
			if (giPwdProtection==PP_WINDOWS) ShowWindow(GetDlgItem(w,PB_MDP_RETROUVE),SW_HIDE);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			SetWindowPos(w,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
			ghFontCourrier=CreateFont(14,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Courier New");
			if (ghFontCourrier!=NULL) PostMessage(GetDlgItem(w,TB_CHALLENGE),WM_SETFONT,(LPARAM)ghFontCourrier,TRUE);
			MACRO_SET_SEPARATOR;
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
					GetDlgItemText(w,TB_CHALLENGE,gszFormattedResponse,sizeof(gszFormattedResponse));
					EndDialog(w,IDOK);
					break;
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case PB_SAVEAS:
					{
						OpenResponse(w);
					}
					break;
				case PB_RECHALLENGE:
					EndDialog(w,PB_RECHALLENGE);
					break;
				case PB_MDP_RETROUVE:
					EndDialog(w,PB_MDP_RETROUVE);
					break;
				case TB_CHALLENGE:
					if (HIWORD(wp)==EN_CHANGE)
					{
						int len=GetDlgItemText(w,TB_CHALLENGE,gszFormattedResponse,sizeof(gszFormattedResponse));
						EnableWindow(GetDlgItem(w,IDOK),len==0 ? FALSE : TRUE);
					}					
					break;
				default:
					break;
				
			}
			break;

		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;

		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
		case WM_DESTROY:
			if (ghFontCourrier!=NULL) DeleteObject(ghFontCourrier);
			break;
	}
	return rc;
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// RecoveryChangeAESKeyData()
//-----------------------------------------------------------------------------
// Ecrit dans le .ini la nouvelle chaine "KpubID:(AESKeyData+UserId)Kpub"
//-----------------------------------------------------------------------------
// [in] pAESKeyData : data permettant de construire la cl� AES des mdp sec
//-----------------------------------------------------------------------------
int RecoveryChangeAESKeyData(int iKeyId)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));
	int rc=-1;

	DWORD lenUserName;
	char Data[245]; // 245=taille max chiffrable, mais en fait on n'utilisera m�me pas tout �a...
	char *pszEncryptedData=NULL;

	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (!gAESKeyInitialized[iKeyId]) { TRACE((TRACE_ERROR,_F_,"AESKey(%d) not initialized",iKeyId)); goto end; }
	if (gpRecoveryKeyValue==NULL) { rc=0; goto end; } // pas de cl� de recouvrement, donc pas de stockage.

	// RAPPEL : la taille possible � chiffrer est limit�e � 245 octets
	// C'est OK car nous chiffrons (AESKeyLen + identifiant utilisateur) = 32 + 100 (100=99+1)
	// (remarque : on tronque donc le username qui est limit� par WIndows � UNLEN = 256)
	ZeroMemory(Data,sizeof(Data));
	
	memcpy(Data,gAESProtectedKeyData[iKeyId],AES256_KEY_LEN);
	if (swUnprotectMemory(Data,AES256_KEY_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
		
	lenUserName=strlen(gszUserName);
	memcpy(Data+AES256_KEY_LEN,gszUserName,(lenUserName<100)?lenUserName:99); // au pire le 0 est l� puisque ZeroMemory
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)Data,100+AES256_KEY_LEN,"Data :"));
	
	rc=RecoveryEncryptData(Data,100+AES256_KEY_LEN,&pszEncryptedData);
	if (rc!=0) goto end;

	TRACE((TRACE_DEBUG,_F_,"RecoveryEncryptData->pszEncryptedData=%s",pszEncryptedData));

	// construit les recovery infos � �crire dans le .ini : KpubID:(MasterPwd+UserId)Kpub
	giRecoveryInfosKeyId=giRecoveryKeyId;
	wsprintf(gszRecoveryInfos,"%04d:",giRecoveryKeyId);
	strcpy_s(gszRecoveryInfos+5,sizeof(gszRecoveryInfos)-5,pszEncryptedData);
	TRACE((TRACE_DEBUG,_F_,"gszRecoveryInfos=%s",gszRecoveryInfos));
	WritePrivateProfileString("swSSO","recoveryInfos",gszRecoveryInfos,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	SecureZeroMemory(Data,sizeof(Data));
	if (pszEncryptedData!=NULL) free(pszEncryptedData);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RecoveryFirstUse()
//-----------------------------------------------------------------------------
// 0.90 : si une cl� de recouvrement existe et les infos de recouvrement n'ont pas encore
//        �t� enregistr�es dans le .ini :
//		  - Cas de la premi�re utilisation apr�s d�ploiement de la cl�
//		  - Cas du renouvellement de la cl�
//-----------------------------------------------------------------------------
// [in] w : handle fen�tre parent pour affichage messages
// [in] iKeyId : identifiant de la cl�
//-----------------------------------------------------------------------------
void RecoveryFirstUse(HWND w,int iKeyId)
{
	TRACE((TRACE_ENTER,_F_, "iKeyId=%d",iKeyId));

	int err=0;
	
	if (iKeyId!=0 && iKeyId!=1) { TRACE((TRACE_ERROR,_F_,"bad iKeyId=%d",iKeyId)); goto end; }
	if (!gAESKeyInitialized[iKeyId]) { TRACE((TRACE_ERROR,_F_,"AESKey(%d) not initialized",iKeyId)); goto end; }

	// pas de politique de recouvrement
	if (gpRecoveryKeyValue==NULL) goto end;
	
	if (*gszRecoveryInfos==0) // pas de recovery info, il faut les stocker
	{
		TRACE((TRACE_INFO,_F_,"Premier stockage cl� id %04d",giRecoveryKeyId));
		err=RecoveryChangeAESKeyData(iKeyId); 
	}
	else if (giRecoveryKeyId!=giRecoveryInfosKeyId) // des recovery infos existent mais l'id de la cl� a chang�
	{
		TRACE((TRACE_INFO,_F_,"Changement de cl� : %04d -> %04d",giRecoveryInfosKeyId,giRecoveryKeyId));
		err=RecoveryChangeAESKeyData(iKeyId); 
	}
end:
	if (err!=0)
	{
		char szErrMsg[1024+1];
		strcpy_s(szErrMsg,sizeof(szErrMsg),GetString(IDS_RECOVERYSTOREPWDFAILED));
		MessageBox(w,szErrMsg,GetString(IDS_MESSAGEBOX_TITLE),MB_OK | MB_ICONEXCLAMATION);
	}
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// RecoveryChallenge()
//-----------------------------------------------------------------------------
// G�n�re le challenge formatt� et le stocke dans les 3 globales :
// - gszFormattedChallengeForDisplay
// - gszFormattedChallengeForSave
// - gszFormattedChallengeForWebservice
// Contenu (AESKeyData+UserId)Kpub + (Ks)Kpub
// En fait (AESKeyData+UserId)Kpub correspond aux RecoveryInfos stock�es dans le .ini
// Stockage de la Ks dans le .ini 
// En fonction de la config, affiche le challenge � l'utilisateur ou l'envoie au WS
//-----------------------------------------------------------------------------
// [in] w : handle fen�tre parent pour affichage messages
//-----------------------------------------------------------------------------
// rc : 0=OK, -1=erreur, -2=l'utilisateur a annul�
//-----------------------------------------------------------------------------
int RecoveryChallenge(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	HCRYPTKEY hKs=NULL;
	DWORD dwKsDataLen;
	unsigned char *pKsData=NULL;
	char *pszKsData=NULL;
	char *pszEncyptedKsData=NULL;
	int pos;
	unsigned int i;
	char szChallenge[5+512+512+1]; // 5 (0000:) + 512 par bloc chiffr� RSA + 0

	*gszFormattedResponse=0;

	// gszRecoveryInfos=0000:(AESKeyData+UserId)Kpub
	if (*gszRecoveryInfos==0) { TRACE((TRACE_ERROR,_F_,"recoveryInfos=vide !")); goto end; }
	strcpy_s(szChallenge,sizeof(szChallenge),gszRecoveryInfos);
	pos=strlen(gszRecoveryInfos);
	
	// (Ks)Kpub
	// g�n�ration cl� de session Ks al�atoire
	brc=CryptGenKey(ghProv,CALG_AES_256,CRYPT_EXPORTABLE,&hKs);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"Erreur generation Ks (CryptGenKey()=0x%08lx)",GetLastError())); goto end; }
	// export dans BLOB
	dwKsDataLen=0;
	brc=CryptExportKey(hKs,0,PLAINTEXTKEYBLOB,0,NULL,&dwKsDataLen);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"Erreur export Ks (CryptExportKey(0)=0x%08lx)",GetLastError())); goto end; }
	pKsData=(unsigned char*)malloc(dwKsDataLen);
	if (pKsData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwKsDataLen)); goto end; }
	brc=CryptExportKey(hKs,0,PLAINTEXTKEYBLOB,0,pKsData,&dwKsDataLen);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"Erreur export Ks (CryptExportKey(1)=0x%08lx)",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pKsData,dwKsDataLen,"pKsData :"));
	// ISSUE#137 : encodage base 64 et stockage dans le .ini d�plac� plus loin au cas o� l'utilisateur annule
	// chiffrement Ks par Kpub
	if (RecoveryEncryptData((char*)pKsData,dwKsDataLen,&pszEncyptedKsData)!=0) goto end;
	// construction du challenge
	strcpy_s(szChallenge+pos,sizeof(szChallenge)-pos,pszEncyptedKsData);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)szChallenge,strlen(szChallenge),"CHALLENGE :"));

	// formattage du challenge : balise de d�but + retours chariot tous les 64 + balise de fin
	strcpy_s(gszFormattedChallengeForDisplay,sizeof(gszFormattedChallengeForDisplay),gcszBeginChallenge);
	strcat_s(gszFormattedChallengeForDisplay,sizeof(gszFormattedChallengeForDisplay),"\r\n");
	strcpy_s(gszFormattedChallengeForSave,sizeof(gszFormattedChallengeForSave),gcszBeginChallenge);
	strcat_s(gszFormattedChallengeForSave,sizeof(gszFormattedChallengeForSave),"\n");
	strcpy_s(gszFormattedChallengeForWebservice,sizeof(gszFormattedChallengeForWebservice),gcszBeginChallenge);
	for (i=0;i<strlen(szChallenge);i+=64)
	{
		strncat_s(gszFormattedChallengeForDisplay,sizeof(gszFormattedChallengeForDisplay),szChallenge+i,64);
		strcat_s(gszFormattedChallengeForDisplay,sizeof(gszFormattedChallengeForDisplay),"\r\n");
		strncat_s(gszFormattedChallengeForSave,sizeof(gszFormattedChallengeForSave),szChallenge+i,64);
		strcat_s(gszFormattedChallengeForSave,sizeof(gszFormattedChallengeForSave),"\n");
		strncat_s(gszFormattedChallengeForWebservice,sizeof(gszFormattedChallengeForWebservice),szChallenge+i,64);
	}
	strcat_s(gszFormattedChallengeForDisplay,sizeof(gszFormattedChallengeForDisplay),gcszEndChallenge);
	strcat_s(gszFormattedChallengeForSave,sizeof(gszFormattedChallengeForSave),gcszEndChallenge);
	strcat_s(gszFormattedChallengeForWebservice,sizeof(gszFormattedChallengeForWebservice),gcszEndChallenge);

	if (gbRecoveryWebserviceActive && giPwdProtection==PP_WINDOWS) // 1.08
	{
		if (RecoveryWebservice()==0) // le web service a retourn� une r�ponse correctement formatt�e, on sort
		{
			// encodage base 64 et stockage dans le .ini
			pszKsData=(char*)malloc(dwKsDataLen*2+1);
			if (pszKsData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwKsDataLen*2+1)); goto end; }
			swCryptEncodeBase64(pKsData,dwKsDataLen,pszKsData);
			WritePrivateProfileString("swSSO","recoveryRunning",pszKsData,gszCfgFile);
			StoreIniEncryptedHash(); // ISSUE#164
			swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_RECOVERY_STARTED,NULL,NULL,NULL,NULL,0);
			rc=0;
			goto end;
		}
		// si on est l�, c'est que le web service a �chou�
		if (gbRecoveryWebserviceManualBackup)
		{
			// bascule sur le mode manuel : la fen�tre de challenge s'affiche
			TRACE((TRACE_INFO,_F_,"Erreur web service recover, bascule en manuel (gbRecoveryWebserviceManualBackup=TRUE)"));
		}
		else
		{
			// sinon message d'erreur et on quitte
			TRACE((TRACE_INFO,_F_,"Erreur web service recover, ne bascule pas en manuel (gbRecoveryWebserviceManualBackup=FALSE)"));
			MessageBox(w,GetString(IDS_RECOVER_WEBSERVICE_ERROR),"swSSO",MB_ICONEXCLAMATION | MB_OK);
			rc=-1; goto end;
		}
	}
	if (DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_CHALLENGE),w,ChallengeDialogProc,(LPARAM)0)==IDOK)
	{
		// encodage base 64 et stockage dans le .ini
		pszKsData=(char*)malloc(dwKsDataLen*2+1);
		if (pszKsData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwKsDataLen*2+1)); goto end; }
		swCryptEncodeBase64(pKsData,dwKsDataLen,pszKsData);
		WritePrivateProfileString("swSSO","recoveryRunning",pszKsData,gszCfgFile);
		StoreIniEncryptedHash(); // ISSUE#164
		swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_RECOVERY_STARTED,NULL,NULL,NULL,NULL,0);
		rc=0;
	}
	else
	{
		rc=-2;
	}
end:
	if (pKsData!=NULL) free(pKsData);
	if (pszKsData!=NULL) free(pszKsData);
	if (pszEncyptedKsData!=NULL) free(pszEncyptedKsData);
	if (hKs!=NULL) CryptDestroyKey(hKs);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RecoveryResponse()
//-----------------------------------------------------------------------------
// Int�gration de la r�ponse, format : (AESKeyData)Ks
// Transchiffrement des mots de passe secondaires
// Effacement de Ks du .ini
//-----------------------------------------------------------------------------
// [in] w : handle fen�tre parent pour affichage messages
//-----------------------------------------------------------------------------
int RecoveryResponse(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szKs[128]; // en fait 72+1 suffisent
	unsigned char *pKsData=NULL;
	DWORD dwKsDataLen;
	DWORD dwKsStringLen;
	BOOL brc;
	HCRYPTKEY hKs=NULL;
	BYTE Response[64];
	int ret;
	int lenFormattedResponse;
	char *p;
	int i;
	char szResponse[256];
	DWORD dwMode=CRYPT_MODE_CBC;
	DWORD lDataToDecrypt;

	// regarde dans le .ini s'il y a un recouvrement en cours
	dwKsStringLen=GetPrivateProfileString("swSSO","recoveryRunning","",szKs,sizeof(szKs),gszCfgFile);
	TRACE((TRACE_DEBUG, _F_, "recoveryRunning=%s len=%d",szKs,dwKsStringLen));
	if (dwKsStringLen==0) { rc=-2; goto end; }// pas de recouvrement en cours

	// si web service activ� et une r�ponse recue, on n'affiche pas la fen�tre
	if (!gbRecoveryWebserviceActive || *gszFormattedResponse==0)
	{
		// affiche la fen�tre de saisie de la response
		ret=DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_RESPONSE),w,ResponseDialogProc,(LPARAM)0);
		if (ret==PB_MDP_RETROUVE) // ISSUE#138
		{
			gbRecoveryRunning=FALSE;
			WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile);
			StoreIniEncryptedHash(); // ISSUE#164
			rc=-2;
			goto end;
		}
		if (ret==PB_RECHALLENGE) // ISSUE#121
		{ 
			WritePrivateProfileString("swSSO","recoveryRunning",NULL,gszCfgFile); // ISSUE#177
			StoreIniEncryptedHash(); // ISSUE#164
			rc=-5; 
			goto end; 
		} 
		if (ret!=IDOK) { rc=-4; goto end; }
	}

	// v�rifie le format de la response
	lenFormattedResponse=strlen(gszFormattedResponse);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gszFormattedResponse,lenFormattedResponse,"gszFormattedResponse"));
	// supprimer les �ventuels espaces qui trainent � la fin
	// ISSUE#179 : et aussi les retours chariot
	while (gszFormattedResponse[lenFormattedResponse-1]==' ' ||
		   gszFormattedResponse[lenFormattedResponse-1]==0x0a ||
		   gszFormattedResponse[lenFormattedResponse-1]==0x0d) { lenFormattedResponse--; gszFormattedResponse[lenFormattedResponse]=0; }
	// v�rifie les balises d�but et fin
	if (memcmp(gszFormattedResponse,gcszBeginResponse,strlen(gcszBeginResponse))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de d�but de challenge non trouvee")); rc=-3; goto end;
	}
	if (memcmp(gszFormattedResponse+lenFormattedResponse-strlen(gcszEndResponse),gcszEndResponse,strlen(gcszEndResponse))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de fin de challenge non trouvee"));  rc=-3; goto end;
	}
	// extrait les 64 caract�res utiles...
	i=0;
	p=gszFormattedResponse+strlen(gcszBeginResponse);
	while (*p==0x0a || *p==0x0d) p++;
	while (*p!='-')
	{
		if (*p!=0x0a && *p!=0x0d) { szResponse[i]=*p; i++; }
		p++;
		if (i>255) { TRACE((TRACE_ERROR,_F_,"Trop de donnees entre les marques debut et fin de la reponse (>255)")); rc=-3; goto end; } // 1.12B2-TIE1
	}
	szResponse[i]=0;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szResponse,i,"szResponse"));

	// d�codage de la KS
	dwKsDataLen=dwKsStringLen/2;
	pKsData=(unsigned char*)malloc(dwKsDataLen);
	if (pKsData==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwKsDataLen)); rc=-3; goto end; }
	if (swCryptDecodeBase64(szKs,(char*)pKsData,dwKsDataLen)!=0) {  rc=-3; goto end; }
	
	// import KS
	brc=CryptImportKey(ghProv,pKsData,dwKsDataLen,0,0,&hKs);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"Import Ks (CryptImportKey()=0x%08lx)",GetLastError())); goto end; }

	// d�chiffrement de la response avec Ks
	if (swCryptDecodeBase64(szResponse,(char*)Response,sizeof(Response))!=0) goto end;

	TRACE_BUFFER((TRACE_DEBUG,_F_,Response,16,"iv"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16,32,"Donnees chiffrees"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16+32,16,"padding"));

	//if (swCryptDecryptDataAES256(Response,Response+16,AES256_KEY_LEN+16,hKs)!=0) goto end;
	lDataToDecrypt=AES256_KEY_LEN+16; // cl� + padding
	brc=CryptSetKeyParam(hKs,KP_IV,Response,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKs,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }
	brc = CryptDecrypt(hKs,0,true,0,Response+16,&lDataToDecrypt);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDecrypt()=0x%08lx",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16,AES256_KEY_LEN,"AESKeyData ****"));
		
	if (swStoreAESKey(Response+16,ghKey1)) goto end;

	rc=0;
end:
	if (rc==0)
	{
		// message OK affich� plus tard, apr�s transchiffrement des configurations
	}
	else if (rc==-1) // erreur technique
	{
		MessageBox(w,GetString(IDS_RECOVERY_ERROR),"swSSO",MB_ICONEXCLAMATION | MB_OK);
		swLogEvent(EVENTLOG_ERROR_TYPE,MSG_RECOVERY_FAIL,NULL,NULL,NULL,NULL,0);
	}
	else if (rc==-3) // erreur de format
	{
		MessageBox(w,GetString(IDS_RECOVERY_ERROR_FORMAT),"swSSO",MB_ICONEXCLAMATION | MB_OK);
		swLogEvent(EVENTLOG_ERROR_TYPE,MSG_RECOVERY_FAIL,NULL,NULL,NULL,NULL,0);
	}
	if (pKsData!=NULL) free(pKsData);
	if (hKs!=NULL) CryptDestroyKey(hKs);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RecoveryWebservice()
//-----------------------------------------------------------------------------
// Appelle le webservice en fournissant le challenge
// En sortie, si OK, la r�ponse est copi�e dans gszFormattedResponse
//-----------------------------------------------------------------------------
int RecoveryWebservice(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char *pszResult=NULL;
	DWORD dwHTTPReturnCode;
	char *p1,*p2=NULL;
	char szData[2048];
	BOOL bRetry,bHTTPRequestOK;
	int iNbTry;

	NOTIFYICONDATA nid;
	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize=sizeof(NOTIFYICONDATA);
	nid.hWnd=gwMain;
	nid.uID=0; 
	//nid.hIcon=;
	nid.uFlags=NIF_INFO; // szInfo, szInfoTitle, dwInfoFlags, and uTimeout
	nid.uTimeout=30000;
	strcpy_s(nid.szInfoTitle,sizeof(nid.szInfoTitle),"Veuillez patienter...");
	strcpy_s(nid.szInfo,sizeof(nid.szInfo),"Resynchronisation du mot de passe en cours...");
	Shell_NotifyIcon(NIM_MODIFY, &nid); 

	// formatte le json � envoyer en post
	sprintf_s(szData,sizeof(szData),"{\"challenge\":\"%s\"}",gszFormattedChallengeForWebservice);
	TRACE((TRACE_INFO,_F_,"Requete POST : %s%s:%d",gszRecoveryWebserviceServer,gszRecoveryWebserviceURL,giRecoveryWebservicePort));
	TRACE((TRACE_INFO,_F_,"Donnees POST : %s",szData));

	// envoie la requete
	// ISSUE#275 : giRecoveryWebserviceNbTry essais espac�s de giRecoveryWebserviceWaitBeforeRetry millisecondes
	iNbTry=0;
	bRetry=TRUE;
	bHTTPRequestOK=FALSE;
	while (bRetry)
	{
		pszResult=HTTPRequest(gszRecoveryWebserviceServer,giRecoveryWebservicePort,gbRecoveryWebserviceHTTPS,gszRecoveryWebserviceURL,
							  "",0,FALSE,"", // pas de failover pour le webservice de recouvrement pour l'instant
							  "", // pas de param�tres en GET							  
							  L"POST",
							  szData,
							  strlen(szData),
							  L"Content-Type: application/json",
							  WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW,
							  giRecoveryWebserviceTimeout,
							  NULL,NULL,NULL,0,
							  &dwHTTPReturnCode);
		iNbTry++;
		if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",gszRecoveryWebserviceURL)); }
		else if (dwHTTPReturnCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",gszRecoveryWebserviceURL,dwHTTPReturnCode)); }
		else { bRetry=FALSE; bHTTPRequestOK=TRUE; }
		
		if (bRetry)
		{
			if (iNbTry<giRecoveryWebserviceNbTry)
				Sleep(giRecoveryWebserviceWaitBeforeRetry);
			else
				bRetry=FALSE;
		}
		// if (iNbTry<1000) bRetry=TRUE; // POUR FAIRE UN TEST DE ROBUSTESSE, r�sultat :
		// 1000 requetes trait�es en 3 minutes sur mon portable avec les traces au niveau max, moins de 200ms par requete, aucun �chec de g�n�ration de RESPONSE
	}
	if (!bHTTPRequestOK) { TRACE((TRACE_ERROR,_F_,"HTTPRequest KO apr�s %d essais",iNbTry)); goto end;}
	// ici on a re�u une r�ponse au format JSON, on extrait la partie utile dans gszFormattedResponse
	// Format de la r�ponse JSON : {<br/>"response" : "------swSSO RESPONSE---.....---swSSO RESPONSE---"<br/>"}
	p1=strstr(pszResult,gcszBeginResponse);
	if (p1==NULL) { TRACE((TRACE_ERROR,_F_,"Format r�ponse incorrect (marque d�but non trouv�e) : %s",pszResult)); goto end; }
	p2=strstr(p1+strlen(gcszBeginResponse),gcszEndResponse);
	if (p2==NULL) { TRACE((TRACE_ERROR,_F_,"Format r�ponse incorrect (marque fin non trouv�e) : %s",pszResult)); goto end; }
	strncpy_s(gszFormattedResponse,sizeof(gszFormattedResponse),p1,p2+strlen(gcszEndResponse)-p1);

	TRACE((TRACE_DEBUG,_F_,"Response=%s",gszFormattedResponse));

	rc=0;
end:

	*(nid.szInfoTitle)=0;
	*(nid.szInfo)=0;
	Shell_NotifyIcon(NIM_MODIFY, &nid); 

	if (pszResult!=NULL) free(pszResult);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
