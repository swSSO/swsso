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
// swSSORecovery.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions liées à la réinitialisation de mot de passe
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
// Chiffre les données avec la clé publique de recouvrement lue en registry
//-----------------------------------------------------------------------------
// [in] pData : bloc de données à chiffrer
// [in] dwDataLen : taille des données (attention, doit être <245, limite CSP Microsoft)
// [out] ppszEncryptedData : données chiffrées en string HEXA
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
	// attention, on chiffre directement avec la clé publique, ce que nous autorise le CSP Microsoft
	// par contre il faut que la taille du bloc à chiffrer soit inférieure à taille de clé - 11 octets,
	// soit dans notre cas : 2048 / 8 - 11 = 245.
	if (dwDataLen>245) { TRACE((TRACE_ERROR,_F_,"Impossible de chiffrer plus de 245 octets avec cette cle")); goto end; }

	// import de la clé RSA lue en base de registre par LoadPolicies
	brc=CryptImportKey(ghProv,gpRecoveryKeyValue,gdwRecoveryKeyLen,0,0,&hPubKey);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }

	// chiffrement des données
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
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
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
			TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szLine,strlen(gcszBeginResponse),"szLine"));		// on regarde tout de suite si on a l'entête, sinon out
			if (memcmp(szLine,gcszBeginResponse,strlen(gcszBeginResponse))!=0)
			{
				MessageBox(w,GetString(IDS_ERROR_BADRESPONSEFILE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
				TRACE((TRACE_ERROR,_F_,"Marque de début de response non trouvée")); goto end;
			}
		}
		len=strlen(szLine);
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
			// magouille suprême : pour gérer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui exécutera un invalidaterect pour forcer la peinture
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
// [in] pAESKeyData : data permettant de construire la clé AES des mdp sec
//-----------------------------------------------------------------------------
int RecoveryChangeAESKeyData(BYTE *pAESKeyData)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	DWORD lenUserName;
	char Data[245]; // 245=taille max chiffrable, mais en fait on n'utilisera même pas tout ça...
	char *pszEncryptedData=NULL;

	if (gpRecoveryKeyValue==NULL) { rc=0; goto end; } // pas de clé de recouvrement, donc pas de stockage.

	// RAPPEL : la taille possible à chiffrer est limitée à 245 octets
	// C'est OK car nous chiffrons (AESKeyLen + identifiant utilisateur) = 32 + 100 (100=99+1)
	// (remarque : on tronque donc le username qui est limité par WIndows à UNLEN = 256)
	ZeroMemory(Data,sizeof(Data));
	memcpy(Data,pAESKeyData,AES256_KEY_LEN);

	lenUserName=strlen(gszUserName);
	memcpy(Data+AES256_KEY_LEN,gszUserName,(lenUserName<100)?lenUserName:99); // au pire le 0 est là puisque ZeroMemory
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)Data,100+AES256_KEY_LEN,"Data :"));
	
	rc=RecoveryEncryptData(Data,100+AES256_KEY_LEN,&pszEncryptedData);
	if (rc!=0) goto end;

	TRACE((TRACE_DEBUG,_F_,"RecoveryEncryptData->pszEncryptedData=%s",pszEncryptedData));

	// construit les recovery infos à écrire dans le .ini : KpubID:(MasterPwd+UserId)Kpub
	giRecoveryInfosKeyId=giRecoveryKeyId;
	wsprintf(gszRecoveryInfos,"%04d:",giRecoveryKeyId);
	strcpy_s(gszRecoveryInfos+5,sizeof(gszRecoveryInfos)-5,pszEncryptedData);
	TRACE((TRACE_DEBUG,_F_,"gszRecoveryInfos=%s",gszRecoveryInfos));
	WritePrivateProfileString("swSSO","recoveryInfos",gszRecoveryInfos,gszCfgFile);
	StoreIniEncryptedHash(); // ISSUE#164
	rc=0;
end:
	if (pszEncryptedData!=NULL) free(pszEncryptedData);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// RecoveryFirstUse()
//-----------------------------------------------------------------------------
// 0.90 : si une clé de recouvrement existe et les infos de recouvrement n'ont pas encore
//        été enregistrées dans le .ini :
//		  - Cas de la première utilisation après déploiement de la clé
//		  - Cas du renouvellement de la clé
//-----------------------------------------------------------------------------
// [in] w : handle fenêtre parent pour affichage messages
// [in] pAESKeyData : clé AES de chiffrement des mdp secondaires
//-----------------------------------------------------------------------------
void RecoveryFirstUse(HWND w,BYTE *pAESKeyData)
{
	TRACE((TRACE_ENTER,_F_, ""));

	int err=0;

	// pas de politique de recouvrement
	if (gpRecoveryKeyValue==NULL) goto end;
	
	if (*gszRecoveryInfos==0) // pas de recovery info, il faut les stocker
	{
		TRACE((TRACE_INFO,_F_,"Premier stockage clé id %04d",giRecoveryKeyId));
		err=RecoveryChangeAESKeyData(pAESKeyData); 
	}
	else if (giRecoveryKeyId!=giRecoveryInfosKeyId) // des recovery infos existent mais l'id de la clé a changé
	{
		TRACE((TRACE_INFO,_F_,"Changement de clé : %04d -> %04d",giRecoveryInfosKeyId,giRecoveryKeyId));
		err=RecoveryChangeAESKeyData(pAESKeyData); 
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
// Génère le challenge formatté et le stocke dans les 3 globales :
// - gszFormattedChallengeForDisplay
// - gszFormattedChallengeForSave
// - gszFormattedChallengeForWebservice
// Contenu (AESKeyData+UserId)Kpub + (Ks)Kpub
// En fait (AESKeyData+UserId)Kpub correspond aux RecoveryInfos stockées dans le .ini
// Stockage de la Ks dans le .ini 
// En fonction de la config, affiche le challenge à l'utilisateur ou l'envoie au WS
//-----------------------------------------------------------------------------
// [in] w : handle fenêtre parent pour affichage messages
//-----------------------------------------------------------------------------
// rc : 0=OK, -1=erreur, -2=l'utilisateur a annulé
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
	char szChallenge[5+512+512+1]; // 5 (0000:) + 512 par bloc chiffré RSA + 0

	*gszFormattedResponse=0;

	// gszRecoveryInfos=0000:(AESKeyData+UserId)Kpub
	if (*gszRecoveryInfos==0) { TRACE((TRACE_ERROR,_F_,"recoveryInfos=vide !")); goto end; }
	strcpy_s(szChallenge,sizeof(szChallenge),gszRecoveryInfos);
	pos=strlen(gszRecoveryInfos);
	
	// (Ks)Kpub
	// génération clé de session Ks aléatoire
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
	// ISSUE#137 : encodage base 64 et stockage dans le .ini déplacé plus loin au cas où l'utilisateur annule
	// chiffrement Ks par Kpub
	if (RecoveryEncryptData((char*)pKsData,dwKsDataLen,&pszEncyptedKsData)!=0) goto end;
	// construction du challenge
	strcpy_s(szChallenge+pos,sizeof(szChallenge)-pos,pszEncyptedKsData);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)szChallenge,strlen(szChallenge),"CHALLENGE :"));

	// formattage du challenge : balise de début + retours chariot tous les 64 + balise de fin
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
		if (RecoveryWebservice()==0) // le web service a retourné une réponse correctement formattée, on sort
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
		// sinon, on bascule sur le mode manuel : la fenêtre de challenge s'affiche
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
// Intégration de la réponse, format : (AESKeyData)Ks
// Transchiffrement des mots de passe secondaire
// Effacement de Ks du .ini
//-----------------------------------------------------------------------------
// [in] w : handle fenêtre parent pour affichage messages
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

	// regarde dans le .ini s'il y a un recouvrement en cours
	dwKsStringLen=GetPrivateProfileString("swSSO","recoveryRunning","",szKs,sizeof(szKs),gszCfgFile);
	TRACE((TRACE_DEBUG, _F_, "recoveryRunning=%s len=%d",szKs,dwKsStringLen));
	if (dwKsStringLen==0) { rc=-2; goto end; }// pas de recouvrement en cours

	// si web service activé et une réponse recue, on n'affiche pas la fenêtre
	if (!gbRecoveryWebserviceActive || *gszFormattedResponse==0)
	{
		// affiche la fenêtre de saisie de la response
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

	// vérifie le format de la response
	lenFormattedResponse=strlen(gszFormattedResponse);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gszFormattedResponse,lenFormattedResponse,"gszFormattedResponse"));
	// supprimer les éventuels espaces qui trainent à la fin
	// ISSUE#179 : et aussi les retours chariot
	while (gszFormattedResponse[lenFormattedResponse-1]==' ' ||
		   gszFormattedResponse[lenFormattedResponse-1]==0x0a ||
		   gszFormattedResponse[lenFormattedResponse-1]==0x0d) { lenFormattedResponse--; gszFormattedResponse[lenFormattedResponse]=0; }
	// vérifie les balises début et fin
	if (memcmp(gszFormattedResponse,gcszBeginResponse,strlen(gcszBeginResponse))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de début de challenge non trouvee")); rc=-3; goto end;
	}
	if (memcmp(gszFormattedResponse+lenFormattedResponse-strlen(gcszEndResponse),gcszEndResponse,strlen(gcszEndResponse))!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Marque de fin de challenge non trouvee"));  rc=-3; goto end;
	}
	// extrait les 64 caractères utiles...
	i=0;
	p=gszFormattedResponse+strlen(gcszBeginResponse);
	while (*p==0x0a || *p==0x0d) p++;
	while (*p!='-')
	{
		if (*p!=0x0a && *p!=0x0d) { szResponse[i]=*p; i++; }
		p++;
	}
	szResponse[i]=0;
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szResponse,i,"szResponse"));

	// décodage de la KS
	dwKsDataLen=dwKsStringLen/2;
	pKsData=(unsigned char*)malloc(dwKsDataLen);
	if (swCryptDecodeBase64(szKs,(char*)pKsData,dwKsDataLen)!=0) {  rc=-3; goto end; }
	
	// import KS
	brc=CryptImportKey(ghProv,pKsData,dwKsDataLen,0,0,&hKs);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"Import Ks (CryptImportKey()=0x%08lx)",GetLastError())); goto end; }

	// déchiffrement de la response avec Ks
	if (swCryptDecodeBase64(szResponse,(char*)Response,sizeof(Response))!=0) goto end;

	TRACE_BUFFER((TRACE_DEBUG,_F_,Response,16,"iv"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16,32,"Donnees chiffrees"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16+32,16,"padding"));

	if (swCryptDecryptDataAES256(Response,Response+16,AES256_KEY_LEN+16,hKs)!=0) goto end;
	TRACE_BUFFER((TRACE_DEBUG,_F_,Response+16,AES256_KEY_LEN,"AESKeyData ****"));

	if (swCreateAESKeyFromKeyData(Response+16,&ghKey1)) goto end;

	rc=0;
end:
	if (rc==0)
	{
		// message OK affiché plus tard, après transchiffrement des configurations
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
// En sortie, si OK, la réponse est copiée dans gszFormattedResponse
//-----------------------------------------------------------------------------
int RecoveryWebservice(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char *pszResult=NULL;
	char szRequest[2048];
	DWORD dwHTTPReturnCode;
	char *p1,*p2=NULL;

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
	
	// formatte la requete au web service
	sprintf_s(szRequest,"%s?challenge=%s",gszRecoveryWebserviceURL,gszFormattedChallengeForWebservice);

	// envoie la requete
	pszResult=HTTPRequest(gszRecoveryWebserviceServer,giRecoveryWebservicePort,gbRecoveryWebserviceHTTPS,szRequest,giRecoveryWebserviceTimeout,NULL,&dwHTTPReturnCode);
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }
	if (dwHTTPReturnCode!=200) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=%d",szRequest,dwHTTPReturnCode)); goto end; }

	// ici on a reçu une réponse au format JSON, on extrait la partie utile dans gszFormattedResponse
	// Format de la réponse JSON : {<br/>"response" : "------swSSO RESPONSE---.....---swSSO RESPONSE---"<br/>"}
	p1=strstr(pszResult,gcszBeginResponse);
	if (p1==NULL) { TRACE((TRACE_ERROR,_F_,"Format réponse incorrect (marque début non trouvée) : %s",pszResult)); goto end; }
	p2=strstr(p1+strlen(gcszBeginResponse),gcszEndResponse);
	if (p2==NULL) { TRACE((TRACE_ERROR,_F_,"Format réponse incorrect (marque fin non trouvée) : %s",pszResult)); goto end; }
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