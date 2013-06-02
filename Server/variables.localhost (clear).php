<?php
/*----------------------------------------------------------------------------------------------
VARIABLES DE CONNEXION A LA BASE DE DONNEES
----------------------------------------------------------------------------------------------*/
define("_HOST_","localhost");	// Nom du serveur hebergeant la base de donnees
define("_DB_","swsso"); 		// Nom de la base de données
define("_USER_","root");		// Utilisateur MySQL
define("_PWD_","");				// Mot de passe utilisateur MySQL
define("_TABLE_PREFIX_",""); // Préfixe à utiliser pour les tables

/*----------------------------------------------------------------------------------------------
CHIFFREMENT DES COLONNES titre, url, szName et szFullPathName
----------------------------------------------------------------------------------------------*/
define("_ENCRYPT_","FALSE"); 	// TRUE | FALSE (chiffre / ne chiffre pas)
define("_AESPWD_","");			// Mot de passe pour chiffrement

/*----------------------------------------------------------------------------------------------
OPTIONS
----------------------------------------------------------------------------------------------*/
define("_LOGS_","TRUE");  	// TRUE | FALSE (genere des logs a chaque getconfig / ou pas)
define("_STATS_","TRUE"); 	// TRUE | FALSE (increment un compteur a chaque getversion / ou pas)
define("_SHOWMENU_","TRUE");// TRUE | FALSE (affichage menu autorise / interdit)
define("_MENUSUFFIX_","");  // "protection" de l'URL presentant le menu
define("_READSUFFIX_","");  // "protection" des URLs permettant la lecture de la base
define("_WRITESUFFIX_",""); // "protection" des URLs permettant la modification de la base
?>