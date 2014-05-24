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
// swSSOWizard.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"

HWND gwWizard=NULL;

int gtabPages[5];
int giCurrentPage=0;
BOOL gbFirstTime=TRUE;
char gszKeystorePwd[LEN_PWD+1];
HANDLE ghFontCourrier=NULL;
char gszFormattedResponseForDisplay[2048];
char gszFormattedResponseForSave[2048];
char gszKeystoreFile[_MAX_PATH+1];

//-----------------------------------------------------------------------------
// ImportKey()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int ImportKey(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	OPENFILENAME ofn;
	char szFile[_MAX_PATH+1];

	strcpy_s(szFile,sizeof(szFile),"");
	ZeroMemory(&ofn,sizeof(OPENFILENAME));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=w;
	ofn.hInstance=NULL;
	ofn.lpstrFilter="*.key";
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
	ofn.lpstrDefExt="key";
	if (!GetOpenFileName(&ofn)) goto end;
	
	SetDlgItemText(w,TB_KEYFILE,szFile);

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// OpenChallenge()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int OpenChallenge(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	OPENFILENAME ofn;
	char szFile[_MAX_PATH+1];
	FILE *hf=NULL;
	char szLine[100];
	char szFormattedChallenge[2048];
	int pos;
	int len;
	BOOL bFirst=TRUE;

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
	*szFormattedChallenge=0;
	pos=0;
	while (fgets(szLine,sizeof(szLine),hf)!=NULL)
	{
		if (bFirst)
		{
			bFirst=FALSE;
			TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)szLine,strlen(gcszBeginChallenge),"szLine"));
			// on regarde tout de suite si on a l'entête, sinon out
			if (memcmp(szLine,gcszBeginChallenge,strlen(gcszBeginChallenge))!=0)
			{
				MessageBox(w,GetString(IDS_ERROR_BADFILE),"swSSO",MB_ICONEXCLAMATION|MB_OK);
				TRACE((TRACE_ERROR,_F_,"Marque de début de challenge non trouvée")); goto end;
			}
		}
		len=strlen(szLine);
		memcpy(szFormattedChallenge+pos,szLine,len-1);
		pos+=len-1;
		szFormattedChallenge[pos]='\r'; pos++;
		szFormattedChallenge[pos]='\n'; pos++;
	}
	szFormattedChallenge[pos-2]='-';
	szFormattedChallenge[pos-1]=0;
	SetDlgItemText(w,TB_CHALLENGE,szFormattedChallenge);
	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// SaveResponse()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int SaveResponse(HWND w)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	OPENFILENAME ofn;
	char szFile[_MAX_PATH+1];
	FILE *hf=NULL;

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
	ofn.Flags=OFN_PATHMUSTEXIST;
	ofn.nFileOffset;
	ofn.lpstrDefExt="txt";
	if (!GetSaveFileName(&ofn)) goto end;
	
	errno_t err=fopen_s(&hf,szFile,"w");
	if (err!=0)
	{
		TRACE((TRACE_ERROR,_F_,"Cannot open file for writing : '%s'",szFile));
		goto end;
	}
	fputs(gszFormattedResponseForSave,hf);
	rc=0;
end:
	if (hf!=NULL) fclose(hf);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// MailResponse()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int MailResponse(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	char szMailTo[2048];
	wsprintf(szMailTo,"mailto:?subject=[swSSO]&body=%s",gszFormattedResponseForSave);
	ShellExecute(NULL,"open",szMailTo,NULL,"",SW_SHOW );
	
	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


// ----------------------------------------------------------------------------------
// InitWizard
// ----------------------------------------------------------------------------------
// Initialisation du Wizard (centrage fenêtre, ...) sur premier WM_INITDIALOG reçu
// ----------------------------------------------------------------------------------
void InitWizard(HWND w)
{
	int cx;
	int cy;
	RECT rect;
	
	if (gwWizard!=NULL) goto end; // déjà fait !

	gwWizard=GetParent(w);
	// icone ALT-TAB : TODO=icone 32x32 !
	SendMessage(gwWizard,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab); 
	SendMessage(gwWizard,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
	// 0.53 positionnement de la fenêtre !
	cx = GetSystemMetrics(SM_CXSCREEN);
	cy = GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(gwWizard,&rect);
	SetWindowPos(gwWizard,NULL,(cx-(rect.right-rect.left))/2,(cy-(rect.bottom-rect.top))/2,0,0,SWP_NOSIZE | SWP_NOZORDER);
end:;
}

// ----------------------------------------------------------------------------------
// PSPPageLogin()
// ----------------------------------------------------------------------------------
// Dialog proc de la page PSP_PAGE_LOGIN
// ----------------------------------------------------------------------------------
static int CALLBACK PSPPageLogin(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	int ret;
UNREFERENCED_PARAMETER(wp);
	switch (msg)
	{
		case WM_INITDIALOG:
			if (gwWizard==NULL) InitWizard(w); // premier affichage de la page
			rc=FALSE;
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(gwWizard,PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT :
					// Vérifie le mot de passe
					HCRYPTKEY hPrivateKey=NULL;
					GetDlgItemText(w,TB_PWD,gszKeystorePwd,sizeof(gszKeystorePwd));			
					ret=swKeystoreGetFirstPrivateKey(gszKeystorePwd,&hPrivateKey);
					if (ret==0)
					{
						// on jette la clé, on n'en a pas besoin !
						if(hPrivateKey!=NULL) { CryptDestroyKey(hPrivateKey); hPrivateKey=NULL; }
						if (IsDlgButtonChecked(w,CK_CHGPWD) && IsDlgButtonChecked(w,CK_IMPORTKEY))
						{
							gtabPages[1]=PSP_PAGE_NEWPWD;
							gtabPages[2]=PSP_PAGE_NEWKEY;
							gtabPages[3]=PSP_PAGE_CHALLENGE;
							gtabPages[4]=PSP_PAGE_RESPONSE;
						}
						else if (IsDlgButtonChecked(w,CK_CHGPWD))
						{
							gtabPages[1]=PSP_PAGE_NEWPWD;
							gtabPages[2]=PSP_PAGE_CHALLENGE;
							gtabPages[3]=PSP_PAGE_RESPONSE;
							gtabPages[4]=0;
						}
						else if (IsDlgButtonChecked(w,CK_IMPORTKEY))
						{
							gtabPages[1]=PSP_PAGE_NEWKEY;
							gtabPages[2]=PSP_PAGE_CHALLENGE;
							gtabPages[3]=PSP_PAGE_RESPONSE;
							gtabPages[4]=0;
						}
						else
						{
							gtabPages[1]=PSP_PAGE_CHALLENGE;
							gtabPages[2]=PSP_PAGE_RESPONSE;
							gtabPages[3]=0;
							gtabPages[4]=0;
						}
						giCurrentPage++;
					}
					else
					{
						swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_KEYSTORE_BADPWD,NULL,NULL,NULL);
						MessageBox(w,GetString(IDS_ERROR_BADPWD),"swSSO",MB_ICONEXCLAMATION | MB_OK) ;
					}
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
			}
			break;
		/*case WM_COMMAND: 
			switch (LOWORD(wp))
			{
				case PB_OUVRIR:
					ShellExecute(NULL,"open",gszCfgFile,NULL,"",SW_SHOW );
					break;
			}
			break;*/
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// PSPPageNewPwd()
// ----------------------------------------------------------------------------------
// Dialog proc de la page PSP_PAGE_NEWPWD
// ----------------------------------------------------------------------------------
static int CALLBACK PSPPageNewPwd(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
UNREFERENCED_PARAMETER(wp);
	switch (msg)
	{
		case WM_INITDIALOG:
			if (gwWizard==NULL) InitWizard(w); // premier affichage de la page
			if (!gbFirstTime) SetDlgItemText(w,TX_NEWPWD,GetString(IDS_CHGPWD));
			rc=FALSE;
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE  :
					// retour non autorisé à la page de login ni à la page d'import de clé
					if (gtabPages[giCurrentPage-1]==PSP_PAGE_LOGIN || gtabPages[giCurrentPage-1]==PSP_PAGE_NEWKEY)
						PropSheet_SetWizButtons(gwWizard,PSWIZB_NEXT);
					else
						PropSheet_SetWizButtons(gwWizard,PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT :
					char szPwd1[LEN_PWD+1];
					char szPwd2[LEN_PWD+1];
					GetDlgItemText(w,IDC_EDIT1,szPwd1,sizeof(szPwd1));
					GetDlgItemText(w,IDC_EDIT2,szPwd2,sizeof(szPwd2));
					if (strcmp(szPwd1,szPwd2)!=0)
					{
						MessageBox(w,GetString(IDS_PWD1_PWD2),"swSSO",MB_ICONEXCLAMATION | MB_OK);
					}
					else if (!IsPasswordPolicyCompliant(szPwd1))
					{
						MessageBox(w,gszPwdPolicy_Message,"swSSO",MB_ICONEXCLAMATION | MB_OK);
					} 
					else 
					{
						if (gbFirstTime) // définition du mot de passe initial
						{
							strcpy_s(gszKeystorePwd,sizeof(gszKeystorePwd),szPwd1);
							giCurrentPage++;
						}	
						else // changement de mot de passe (ISSUE#120)
						{
							if (swChangeKeystorePassword(gszKeystorePwd,szPwd1)==0) // changement réussi
							{
								MessageBox(w,GetString(IDS_CONFIRM_PWD_CHANGE),"swSSO",MB_ICONINFORMATION | MB_OK) ;
								swLogEvent(EVENTLOG_INFORMATION_TYPE,MSG_PWD_CHANGE,NULL,NULL,NULL);
								giCurrentPage++;
							}
							else // echec du changement...
							{
								MessageBox(w,GetString(IDS_CONFIRM_PWD_CHANGE),"swSSO",MB_ICONINFORMATION | MB_OK) ;
							}
						}
					}
					SecureZeroMemory(szPwd1,sizeof(szPwd1));
					SecureZeroMemory(szPwd2,sizeof(szPwd2));
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
				case PSN_WIZBACK:
					giCurrentPage--;
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
			}
			break;
		/*case WM_COMMAND: 
			switch (LOWORD(wp))
			{
				case PB_OUVRIR:
					ShellExecute(NULL,"open",gszCfgFile,NULL,"",SW_SHOW );
					break;
			}
			break;*/
	}
	return rc;
}


// ----------------------------------------------------------------------------------
// PSPPageNewKey()
// ----------------------------------------------------------------------------------
// Dialog proc de la page PSP_PAGE_NEWKEY
// ----------------------------------------------------------------------------------
static int CALLBACK PSPPageNewKey(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
	int ret;
UNREFERENCED_PARAMETER(wp);
	switch (msg)
	{
		case WM_INITDIALOG:
			if (gwWizard==NULL) InitWizard(w); // premier affichage de la page
			rc=FALSE;
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE  :
					// retour non autorisé à la page de login ni à la page de changement de mot de passe
					if (gtabPages[giCurrentPage-1]==PSP_PAGE_LOGIN || gtabPages[giCurrentPage-1]==PSP_PAGE_NEWPWD)
						PropSheet_SetWizButtons(gwWizard,PSWIZB_NEXT);
					else
						PropSheet_SetWizButtons(gwWizard,PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT :
					char szKeyFile[_MAX_PATH+1];
					char szKeyPwd[LEN_PWD+1];
					GetDlgItemText(w,TB_KEYFILE,szKeyFile,sizeof(szKeyFile));
					GetDlgItemText(w,TB_KEYPWD,szKeyPwd,sizeof(szKeyPwd));
					if (*szKeyFile!=0 && *szKeyPwd!=0)
					{
						ret=swKeystoreImportPrivateKey(szKeyFile,szKeyPwd,gszKeystorePwd);
						if (ret==0) // Clé importée, sauvegarde le fichier keystore
						{
							// ISSUE#119
							// ret=swKeystoreSave("./swSSO-Keystore.txt");
							ret=swKeystoreSave(gszKeystoreFile);
							if (ret==0) // Tout va bien, page suivante ! 
							{
								MessageBox(w,GetString(IDS_CONFIRM_KEY_IMPORTED),"swSSO",MB_ICONINFORMATION | MB_OK) ;
								giCurrentPage++;
							}
							else
							{
								swCryptErrorMsg(w,ret);
							}
						}
						else // erreur
						{
							swCryptErrorMsg(w,ret);
						}
					}
					SecureZeroMemory(szKeyPwd,sizeof(szKeyPwd));
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
				case PSN_WIZBACK:
					giCurrentPage--;
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
			}
			break;
		case WM_COMMAND: 
			switch (LOWORD(wp))
			{
				case PB_IMPORTKEY:
					ImportKey(w);
					break;
			}
			break;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// PSPPageChallenge()
// ----------------------------------------------------------------------------------
// Dialog proc de la page PSP_PAGE_CHALLENGE
// ----------------------------------------------------------------------------------
static int CALLBACK PSPPageChallenge(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
UNREFERENCED_PARAMETER(wp);
	switch (msg)
	{
		case WM_INITDIALOG:
			if (gwWizard==NULL) InitWizard(w); // premier affichage de la page
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_CHALLENGE),EM_LIMITTEXT,2048,0);
			if (ghFontCourrier!=NULL) PostMessage(GetDlgItem(w,TB_CHALLENGE),WM_SETFONT,(LPARAM)ghFontCourrier,TRUE);
			rc=FALSE;
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE  :
					// retour non autorisé à la page de login ni à la page de changement de mot de passe  ni import de clé
					if (gtabPages[giCurrentPage-1]==PSP_PAGE_LOGIN || 
						gtabPages[giCurrentPage-1]==PSP_PAGE_NEWPWD ||
						gtabPages[giCurrentPage-1]==PSP_PAGE_NEWKEY)
						PropSheet_SetWizButtons(gwWizard,PSWIZB_NEXT);
					else
						PropSheet_SetWizButtons(gwWizard,PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT :
					char szFormattedChallenge[2048];
					
					GetDlgItemText(w,TB_CHALLENGE,szFormattedChallenge,sizeof(szFormattedChallenge));
					if (RecoveryChallenge(w,szFormattedChallenge,gszFormattedResponseForDisplay,gszFormattedResponseForSave)!=0)
					{
						// MEssage d'erreur affiché dans RecoveryChallenge
					}
					else
					{
						giCurrentPage++;
					}
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
				case PSN_WIZBACK:
					giCurrentPage--;
					SetWindowLong(w,DWL_MSGRESULT,gtabPages[giCurrentPage]); 
					rc=TRUE;
					break;
			}
			break;
		case WM_COMMAND: 
			switch (LOWORD(wp))
			{
				case PB_IMPORTER:
					OpenChallenge(w);
					break;
			}
			break;
	}
	return rc;
}

// ----------------------------------------------------------------------------------
// PSPPageResponse()
// ----------------------------------------------------------------------------------
// Dialog proc de la page PSP_PAGE_RESPONSE
// ----------------------------------------------------------------------------------
static int CALLBACK PSPPageResponse(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	int rc=FALSE;
UNREFERENCED_PARAMETER(w);
UNREFERENCED_PARAMETER(wp);
	switch (msg)
	{
		case WM_INITDIALOG:
			if (gwWizard==NULL) InitWizard(w); // premier affichage de la page
			SendMessage(GetDlgItem(w,TB_RESPONSE),EM_LIMITTEXT,strlen(gszFormattedResponseForDisplay),0);
			if (ghFontCourrier!=NULL) PostMessage(GetDlgItem(w,TB_RESPONSE),WM_SETFONT,(LPARAM)ghFontCourrier,TRUE);
			SetDlgItemText(w,TB_RESPONSE,gszFormattedResponseForDisplay);
			rc=FALSE;
			break;
		case WM_NOTIFY:
			switch (((NMHDR FAR *)lp)->code) 
			{
				case PSN_SETACTIVE  :
					PropSheet_SetWizButtons(gwWizard,PSWIZB_FINISH);
					break;
				case PSN_WIZFINISH:
					break;
			}
			break;
		case WM_COMMAND: 
			switch (LOWORD(wp))
			{
				case PB_ENREGISTRER:
					SaveResponse(w);
					break;
				case PB_MAILTO:
					MailResponse();
					break;
			}
			break;
	}
	return rc;
}
//-----------------------------------------------------------------------------
// RecoveryWizard()
//-----------------------------------------------------------------------------
// Wizard 5 pages
// PAGE_LOGIN      : saisie du login, avec 2 options (import clé et chgt mdp)
// PAGE_NEWPWD     : changement de mot de passe
// PAGE_NEWKEY     : import d'une nouvelle clé
// PAGE_CHALLENHGE : copier-coller ou import du challenge
// PAGE_RESPONSE   : copier-coller, enregistrer sous ou mailto de la response
//-----------------------------------------------------------------------------
int RecoveryWizard(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HPROPSHEETPAGE hpsp[5];
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;
	int ret;
	HANDLE hLogo=NULL;
	int len;
	
	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));
	
	hLogo=LoadImage(ghInstance,MAKEINTRESOURCE(IDB_LOGOBAR),IMAGE_BITMAP,0,0,LR_DEFAULTCOLOR);
	if (hLogo==NULL) { TRACE((TRACE_ERROR,_F_,"LoadImage(IDB_LOGOBAR)=%ld",GetLastError)); goto end; }
	ghFontCourrier=CreateFont(14,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Courier New");

	// REMARQUE : il est possible de changer la police du titre
	// voir : ms-help://MS.PSDKSVR2003R2.1033/shellcc/platform/commctls/propsheet/wizards.htm#Implement

	// PROPSHEETHEADER
	ZeroMemory(&psh,sizeof(PROPSHEETHEADER));
	psh.dwSize=sizeof(PROPSHEETHEADER);
	psh.dwFlags=PSH_WIZARD97 | PSH_HEADER | PSH_USEHBMHEADER;
	psh.hwndParent=HWND_DESKTOP ;
	psh.nStartPage=0;
	psh.pszbmHeader=MAKEINTRESOURCE(IDB_LOGOBAR);
	psh.hbmHeader=(HBITMAP)hLogo;
	psh.nPages=5;
	
	// ISSUE#119 : fichier keystore dans le répertoire courant
	// ret=swKeystoreLoad("./swSSO-Keystore.txt");
	len=GetCurrentDirectory(_MAX_PATH-20,gszKeystoreFile);
	if (len==0) goto end;
	if (gszKeystoreFile[len-1]!='\\')
	{
		gszKeystoreFile[len]='\\';
		len++;
	}
	strcpy_s(gszKeystoreFile+len,_MAX_PATH+1,"swSSO-Keystore.txt");
	ret=swKeystoreLoad(gszKeystoreFile);

	if (ret==0) // Keystore chargé OK
		gbFirstTime=FALSE;
	else if (ret==SWCRYPT_FILENOTFOUND)
		gbFirstTime=TRUE;
	else
	{
		TRACE((TRACE_ERROR,_F_,"swKeystoreLoad(%s)=%d",gszKeystoreFile,ret));
		MessageBox(NULL,GetString(IDS_KEYSTORE_LOAD_ERROR),"swSSO",MB_ICONEXCLAMATION | MB_OK);
		goto end;
	}
		
	giCurrentPage=(gbFirstTime?1:0);
	
	gtabPages[0]=PSP_PAGE_LOGIN;
	gtabPages[1]=PSP_PAGE_NEWPWD;
	gtabPages[2]=PSP_PAGE_NEWKEY;
	gtabPages[3]=PSP_PAGE_CHALLENGE;
	gtabPages[4]=PSP_PAGE_RESPONSE;
	psh.nStartPage=giCurrentPage;

	// PROPSHEETPAGE
	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));
	psp.dwSize=sizeof(PROPSHEETPAGE);
	psp.dwFlags=PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE ;
	// PAGE_LOGIN
	psp.pszTemplate=MAKEINTRESOURCE(gtabPages[0]);
	psp.pfnDlgProc=PSPPageLogin;
	psp.lParam=0;
	psp.pszHeaderTitle=GetString(IDS_TITLE_LOGIN);
	psp.pszHeaderSubTitle="";
	hpsp[0]=CreatePropertySheetPage(&psp);
	if (hpsp[0]==NULL) goto end;
	// PAGE_NEWPWD
	psp.pszTemplate=MAKEINTRESOURCE(gtabPages[1]);
	psp.pfnDlgProc=PSPPageNewPwd;
	psp.lParam=0;
	psp.pszHeaderTitle=GetString(gbFirstTime?IDS_TITLE_LOGIN:IDS_TITLE_CHGPWD);
	psp.pszHeaderSubTitle=""; //Veuillez saisir votre mot de passe
	hpsp[1]=CreatePropertySheetPage(&psp);
	if (hpsp[1]==NULL) goto end;
	// PAGE_NEWKEY
	psp.pszTemplate=MAKEINTRESOURCE(gtabPages[2]);
	psp.pfnDlgProc=PSPPageNewKey;
	psp.lParam=0;
	psp.pszHeaderTitle=GetString(IDS_TITLE_NEWKEY);
	psp.pszHeaderSubTitle="";//Choisissez le fichier de clé à importer et saisissez le mot de passe correspondant";
	hpsp[2]=CreatePropertySheetPage(&psp);
	if (hpsp[2]==NULL) goto end;
	// PAGE_CHALLENGE
	psp.pszTemplate=MAKEINTRESOURCE(gtabPages[3]);
	psp.pfnDlgProc=PSPPageChallenge;
	psp.lParam=0;
	psp.pszHeaderTitle=GetString(IDS_TITLE_CHALLENGE);
	psp.pszHeaderSubTitle="";//Veuillez insérer ci-dessous ou importer les informations fournies par l'utilisateur";
	hpsp[3]=CreatePropertySheetPage(&psp);
	if (hpsp[3]==NULL) goto end;
	// PAGE_RESPONSE
	psp.pszTemplate=MAKEINTRESOURCE(gtabPages[4]);
	psp.pfnDlgProc=PSPPageResponse;
	psp.lParam=0;
	psp.pszHeaderTitle=GetString(IDS_TITLE_RESPONSE);
	psp.pszHeaderSubTitle="";//Voici la réponse à fournir à l'utilisateur";
	hpsp[4]=CreatePropertySheetPage(&psp);
	if (hpsp[4]==NULL) goto end;

	psh.phpage=hpsp;
	gwWizard=NULL;
	ret=PropertySheet(&psh);
	gwWizard=NULL;
	if (ret==-1)
	{
		TRACE((TRACE_ERROR,_F_, "PropertySheet()"));
	}

end:
	if (hLogo!=NULL) { DeleteObject(hLogo); hLogo=NULL; }
	if (ghFontCourrier!=NULL) { DeleteObject(ghFontCourrier); hLogo=NULL; }
	TRACE((TRACE_LEAVE,_F_, ""));
	return 0;
}
