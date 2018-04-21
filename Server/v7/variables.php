<?php
//-----------------------------------------------------------------------------
//                                  swSSO
//                Copyright (C) 2004-2018 - Sylvain WERDEFROY
//                https://www.swsso.fr | https://www.swsso.com
//                             sylvain@swsso.fr
//-----------------------------------------------------------------------------
//  This file is part of swSSO.
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------
// VERSION INTERNE : 7.0.0
//------------------------------------------------------------------------------

/*----------------------------------------------------------------------------------------------
VARIABLES DE CONNEXION A LA BASE DE DONNEES
----------------------------------------------------------------------------------------------*/
define("_HOST_","localhost");	// Nom du serveur hebergeant la base de donnees
define("_DB_","swsso"); 		// Nom de la base de données
define("_USER_","root");		// Utilisateur MySQL
define("_PWD_","");				// Mot de passe utilisateur MySQL
define("_TABLE_PREFIX_",""); 	// Préfixe à utiliser pour les tables

/*----------------------------------------------------------------------------------------------
CHIFFREMENT DES COLONNES titre, url, szName et szFullPathName
----------------------------------------------------------------------------------------------*/
define("_ENCRYPT_","TRUE"); 	// TRUE | FALSE (chiffre / ne chiffre pas)
define("_AESPWD_","ChangeMe!");	// Mot de passe pour chiffrement

/*----------------------------------------------------------------------------------------------
OPTIONS
----------------------------------------------------------------------------------------------*/
define("_AUTH_","TRUE");  			// TRUE | FALSE (impose l'authentification pour les fonctions d'écriture)
define("_LOGS_","FALSE");  			// TRUE | FALSE (genere des logs a chaque getconfig / ou pas)
define("_SHOWMENU_","TRUE");		// TRUE | FALSE (affichage menu autorise / interdit)
define("_MENUSUFFIX_","");  		// "protection" de l'URL presentant le menu
define("_READSUFFIX_","");  		// "protection" des URLs permettant la lecture de la base
define("_WRITESUFFIX_",""); 		// "protection" des URLs permettant la modification de la base
define("_SHOWRESETPWD_","FALSE"); 	// TRUE | FALSE (montre / cache la fonction d'effacement du mdp admin)
define("_SEPARATOR_",";");			// séparateur pour les exports CSV
define("_STATOVERWRITE_","TRUE");	// chaque remontée de stat écrase la ligne précédemment remontée pour cet utilisateur
?>