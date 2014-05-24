//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2012 - Sylvain WERDEFROY
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
// swTools.cpp
//-----------------------------------------------------------------------------
// Utilitaires
//-----------------------------------------------------------------------------

#include "stdafx.h"

char gszRes[512];

// ----------------------------------------------------------------------------------
// GetString
// ----------------------------------------------------------------------------------
// lecture d'une chaine dans la string table
// ----------------------------------------------------------------------------------
// [in] identifiant de la chaine à charger
// ----------------------------------------------------------------------------------
char *GetString(UINT uiString)
{
	*gszRes=0;
	LoadString(ghInstance,uiString,gszRes,sizeof(gszRes));
	return gszRes;
}

// ----------------------------------------------------------------------------------
// GetBSTRFromSZ
// ----------------------------------------------------------------------------------
// Convertit une sz en BSTR (l'appelant doit libérer avec SysFreeString si !NULL)
// ----------------------------------------------------------------------------------
// [in] chaine sz à convertir en BSTR
// [rc] NULL si échec, la chaine convertie sinon
// ----------------------------------------------------------------------------------
BSTR GetBSTRFromSZ(const char *sz)
{
	TRACE((TRACE_ENTER,_F_, "%s",sz));
	WCHAR *pwc=NULL;
	int rc,sizeofpwc;
	BSTR bstr=NULL;

	// premier appel pour connaitre taille à allouer (le 0 de fin de chaine est inclus)
	sizeofpwc=MultiByteToWideChar(CP_ACP,0,sz,-1,NULL,0);
	if (sizeofpwc==0) { TRACE((TRACE_ERROR,_F_,"MultiByteToWideChar(0)")); goto end; }
	TRACE((TRACE_INFO,_F_,"MultiByteToWideChar()->rc=%d",sizeofpwc));
	pwc=(WCHAR *)malloc(sizeofpwc*2); // *2 car 16 bits unicode
	if (pwc==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",sizeofpwc)); goto end; }
	// conversion
	rc=MultiByteToWideChar(CP_ACP,0,sz,-1,pwc,sizeofpwc);
	if (rc==0) { TRACE((TRACE_ERROR,_F_,"MultiByteToWideChar(%d)",sizeofpwc)); goto end; }
	// bstrisation
	bstr=SysAllocString(pwc);
	if (bstr==NULL) goto end;
end:
	if (pwc!=NULL) free(pwc);
	TRACE((TRACE_LEAVE,_F_, "%S",bstr));
	return bstr;
}

//-----------------------------------------------------------------------------
// strnistr()
//-----------------------------------------------------------------------------
// strstr non case sensitive
// trouvé chez code guru, à regarder de plus près, j'ai copié collé tel quel.
// http://www.codeguru.com/cpp/cpp/string/article.php/c5641
//-----------------------------------------------------------------------------
char *strnistr (const char *szStringToBeSearched,
				const char *szSubstringToSearchFor,
				const int  nStringLen)
{
   int            nLen;
   int            nOffset;
   int            nMaxOffset;
   char   *  pPos;
   int            nStringLenInt;

   // verify parameters
   if ( szStringToBeSearched == NULL ||
        szSubstringToSearchFor == NULL )
   {
      return (char*)szStringToBeSearched;
   }

   // get length of the substring
   nLen = _tcslen(szSubstringToSearchFor);

   // empty substring-return input (consistent w/ strstr)
   if ( nLen == 0 ) {
      return (char*)szStringToBeSearched;
   }

   if ( nStringLen == -1 || nStringLen >
               (int)_tcslen(szStringToBeSearched) )
   {
      nStringLenInt = _tcslen(szStringToBeSearched);
   } else {
      nStringLenInt = nStringLen;
   }

   nMaxOffset = nStringLenInt - nLen;

   pPos = (char*)szStringToBeSearched;

   for ( nOffset = 0; nOffset <= nMaxOffset; nOffset++ ) {

      if ( _tcsnicmp(pPos, szSubstringToSearchFor, nLen) == 0 ) {
         return pPos;
      }
      // move on to the next character
      pPos++; //_tcsinc was causing problems :(
   }

   return NULL;
}


//-----------------------------------------------------------------------------
// GetModifiedFont()
//-----------------------------------------------------------------------------
// Retourne la police des dialogbox modifiée avec les attribués passés en param
// Attention, l'appelant devra libérer avec DeleteObject().
//-----------------------------------------------------------------------------
HFONT GetModifiedFont(HWND w,long lfWeight)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HFONT hCurrentFont=NULL;
	HFONT hNewFont=NULL;

	LOGFONT logfont;
	hCurrentFont=(HFONT)SendMessage(w,WM_GETFONT,0,0);
	if(hCurrentFont!=NULL)
	{
		if(GetObject(hCurrentFont, sizeof(LOGFONT), (LPSTR)&logfont)!= NULL) 
		{
			logfont.lfWeight=lfWeight;
			hNewFont=CreateFontIndirect(&logfont);
		}
	}
	TRACE((TRACE_LEAVE,_F_, "rc=0x%08lx",hNewFont));
	return hNewFont;
}

//-----------------------------------------------------------------------------
// SetTextBold()
//-----------------------------------------------------------------------------
// Change la police d'un contrôle
// Remarque : la police Bold est affectée à ghBoldFont pour les appels ultérieurs
//            et sera libérée en fin de winmain.
//-----------------------------------------------------------------------------
void SetTextBold(HWND w,int iCtrlId)
{
	TRACE((TRACE_ENTER,_F_, ""));

	if (ghBoldFont==NULL) 
	{
		ghBoldFont=GetModifiedFont(w,FW_BOLD);
		if (ghBoldFont==NULL) goto end;
	}
	HWND wItem=GetDlgItem(w,iCtrlId);
	if(wItem==NULL) goto end;
	
	PostMessage(wItem,WM_SETFONT,(LPARAM)ghBoldFont,TRUE);
end:
	TRACE((TRACE_LEAVE,_F_, ""));
}

//-----------------------------------------------------------------------------
// atox4()
//-----------------------------------------------------------------------------
// atoi pour convertir chaine hexa de 4 caractères en int, -1 si erreur
//-----------------------------------------------------------------------------
int atox4(char *sz)
{
	TRACE((TRACE_ENTER,_F_, "in:%s",sz));
	int i,ret=-1;
	unsigned char uc;
	ret=0;
	for (i=0;i<4;i++)
	{
		if (sz[i]>='A' && sz[i]<='F')      uc=(unsigned char)(sz[i]-'A'+10);
		else if (sz[i]>='0' && sz[i]<='9') uc=(unsigned char)(sz[i]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",sz[i]));ret=-1;goto end; }
		ret*=16;
		ret+=uc;
		TRACE((TRACE_DEBUG,_F_,"i=%d uc=%d ret=%d",i,uc,ret));
	}
end:
	TRACE((TRACE_LEAVE,_F_, "out:%04X",ret));
	return ret;
}

//-----------------------------------------------------------------------------
// ExpandFileName()
//-----------------------------------------------------------------------------
// Expande les variables d'environnement dans les noms de fichier
//-----------------------------------------------------------------------------
int ExpandFileName(char *szInFileName,char *szOutFileName, int iBufSize)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	char szTmpFileName[_MAX_PATH+1];
	int iPos=0;
	int len;

	// on commence par enlever les éventuels guillemets de début et fin de chaine
	if (*szInFileName=='"') iPos=1;
	strcpy_s(szTmpFileName,_MAX_PATH+1,szInFileName+iPos);
	len=strlen(szTmpFileName);
	if (len>1 && szTmpFileName[len-1]=='"') szTmpFileName[len-1]=0;

	// on expande les variables d'environnement
	if (ExpandEnvironmentStrings(szTmpFileName,szOutFileName,iBufSize)==0)
	{
		TRACE((TRACE_ERROR,_F_,"ExpandEnvironmentStrings(%s)=%d",szTmpFileName,GetLastError()));
		goto end;
	}

	TRACE((TRACE_DEBUG,_F_,"ExpandEnvironmentStrings(%s)=%s",szTmpFileName,szOutFileName));
	rc=0;
	
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}
