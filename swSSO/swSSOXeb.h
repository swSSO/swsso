//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
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
// swSSOXeb.h
//-----------------------------------------------------------------------------

#define MAX_TEXT_FIELDS 100

typedef struct 
{
	HWND w;
	int iAction; // id action
	int iErreur; // le positionner à -1 pour sortir de la récursivité de DoAccessible
	int iLevel;  // niveau de récursivité
	IAccessible *pTextFields[MAX_TEXT_FIELDS]; // pour mémoriser les champs textes
	int iPwdIndex;
	int iNbPwdFound;
	int iTextFieldIndex;
	char szClassName[128+1]; // classe de fenêtre recherchée
	char szExclude[128+1]; // classe de fenêtre à exclure : si trouvée, on arrête l'énum avec retour null
	int iBrowser; // ISSUE#279
	BOOL bLabelFound; // ISSUE#373
} T_SUIVI_ACCESSIBLE;

int SSOWebAccessible(HWND w,int iAction,int iBrowser);


