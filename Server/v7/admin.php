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
include('variables.php');
include('util.php');
include('functions.php');
include('sessions.php');

if (!isAdminAuthorized())
{
	header('Location: ./login.php');
	exit();
}
if (!isset($_GET['action']))
{
	header('Location: ./admin.php?action=menu');
	exit();
}

$title="<title>swSSO - Serveur de configuration (v6.5.3)</title>";

// ------------------------------------------------------------
// showall : génère une page html avec l'ensemble des configs actives
// ------------------------------------------------------------
if ($_GET['action']=="showall"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	showAll(1,$var_domain,$title,0);
}
// ------------------------------------------------------------
// showold : génère une page html avec l'ensemble des configs archivées
//           Remarque : dans la V4 les configurations ne sont plus archivées
// ------------------------------------------------------------
else if ($_GET['action']=="showold"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	showAll(0,$var_domain,$title,0);
}
// ------------------------------------------------------------
// showlogs : génère une page html avec les logs
// ------------------------------------------------------------
else if ($_GET['action']=="showlogs"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	$var_result="";
	if (isset($_GET["result"])) $var_result=utf8_decode(addslashes($_GET['result']));
	showLogs($var_domain,$var_result,$title);
}
// ------------------------------------------------------------
// showcategories : génère une page html avec liste des catégories
// ------------------------------------------------------------
else if ($_GET['action']=="showcategories"._READSUFFIX_)
{
	showCategories($title);
}
// ------------------------------------------------------------
// showdomains : génère une page html avec liste des domaines
// ------------------------------------------------------------
else if ($_GET['action']=="showdomains"._READSUFFIX_)
{
	showDomains($title);
}
// ------------------------------------------------------------
// deletelogs : suppression des logs
// ------------------------------------------------------------
else if ($_GET['action']=="deletelogs"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	$var_domain=utf8_decode(myaddslashes($_GET['domain'])); 
	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > <a href=./admin.php?action=menu"._MENUSUFFIX_."&domain=".$var_domain.">Gestion du domaine ".getDomainLabel($cnx,$var_domain)."</a> > Logs</b><br/><br/>";
	
	$szRequest="delete from "._TABLE_PREFIX_."logs";
	if ($var_domain!=0) $szRequest=$szRequest." where domainId='".$var_domain."'";

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }
	echo "<font face=verdana size=2>";
	echo "Tous les logs ont &eacute;t&eacute; effac&eacute;s.<br/>";
	echo "</font>";
	echo "</html>";
	dbClose($cnx);
}
// ------------------------------------------------------------
// archiveconfig : archivage d'une configuration
// ------------------------------------------------------------
else if ($_GET['action']=="archiveconfig"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	$var_id=utf8_decode(addslashes($_GET['id']));
	$var_active=utf8_decode(addslashes($_GET['active']));
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	
	$szRequest="update "._TABLE_PREFIX_."config set active=0, lastModified=NOW() where id=".$var_id." AND active=1";    

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showAll($var_active,$var_domain,$title,0);
}
// ------------------------------------------------------------
// restoreconfig : restauration d'une configuration
// ------------------------------------------------------------
else if ($_GET['action']=="restoreconfig"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	$var_id=utf8_decode(addslashes($_GET['id']));
	$var_active=utf8_decode(addslashes($_GET['active']));
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	
	$szRequest="update "._TABLE_PREFIX_."config set active=1, lastModified=NOW() where id=".$var_id." AND active=0";    

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showAll($var_active,$var_domain,$title,0);
}
// ------------------------------------------------------------
// deleteconfig : suppression définitive d'une configuration
// ------------------------------------------------------------
else if ($_GET['action']=="deleteconfig"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	$var_id=utf8_decode(addslashes($_GET['id']));
	$var_active=utf8_decode(addslashes($_GET['active']));
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	
	$szRequest="delete from "._TABLE_PREFIX_."config where id=".$var_id." AND active=".$var_active;

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showAll($var_active,$var_domain,$title,0);
}
// ------------------------------------------------------------
// deletecateg : suppression définitive d'une configuration
// ------------------------------------------------------------
else if ($_GET['action']=="deletecateg"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	$var_id=utf8_decode(addslashes($_GET['id']));
	$var_domain=utf8_decode(addslashes($_GET['domain']));

	$szRequest="delete from "._TABLE_PREFIX_."categ where id=".$var_id;      

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showCategories($title);
}
// ------------------------------------------------------------
// deletedomain : suppression définitive d'un domain
// ------------------------------------------------------------
else if ($_GET['action']=="deletedomain"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	$var_id=utf8_decode(addslashes($_GET['id']));
	$szRequest="delete from "._TABLE_PREFIX_."domains where id=".$var_id;      

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showDomains($title);
}
// ------------------------------------------------------------
// deleteadminpwd : suppression du mot de passe admin
// ------------------------------------------------------------
else if ($_GET['action']=="deleteadminpwd"._WRITESUFFIX_)
{
	if (_SHOWRESETPWD_=="TRUE")
	{
		$cnx=dbConnect();
		if (!$cnx) return;

		$szRequest="delete from "._TABLE_PREFIX_."adminpwd where pwd!=''";      

		$req=mysql_query($szRequest,$cnx);
		if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

		echo "Mot de passe administrateur effac&eacute;.";
		
		dbClose($cnx);
	}
}
// ------------------------------------------------------------
// MENU
// ------------------------------------------------------------
else if ($_GET['action']=="menu"._MENUSUFFIX_)
{
	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	if (_SHOWMENU_=="TRUE")
	{
		if(isset($_GET["domain"])) 
		{
			$var_domain=utf8_decode(myaddslashes($_GET['domain'])); 
			echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Gestion du domaine ".getDomainLabel(null,$var_domain)."</b><br/>";
			echo "<br/>+ <a href=./admin.php?action=showall"._READSUFFIX_."&domain=".$var_domain.">Configurations actives</a>";      
			echo "<br/>+ <a href=./admin.php?action=showold"._READSUFFIX_."&domain=".$var_domain.">Configurations archiv&eacute;es</a>";      
			if (_LOGS_=="TRUE") echo "<br/>+ <a href=./admin.php?action=showlogs"._READSUFFIX_."&domain=".$var_domain.">Logs</a>";      
			if (_LOGS_=="TRUE") echo "<br/>+ <a href=./admin.php?action=deletelogs"._WRITESUFFIX_."&domain=".$var_domain.">Effacer les logs</a>";   
			echo "</font>";
		}
		else
		{
			echo "<font face=verdana size=2><b>Menu principal</b><br/>";
			echo "<br/>+ G&eacute;rer mes domaines";
			menuShowDomains();
			echo "<br/>+ <a href=./admin.php?action=showcategories"._READSUFFIX_.">Cat&eacute;gories</a>"; 
			echo "<br/>+ <a href=./admin.php?action=showdomains"._READSUFFIX_.">Ajouter ou supprimer un domaine</a>";
			echo "<br/>+ <a href=./export.php?data=stats>T&eacute;l&eacute;charger les statistiques (csv)</a>";    
			if (_SHOWRESETPWD_=="TRUE") echo "<br/>+ <a href=./admin.php?action=deleteadminpwd"._WRITESUFFIX_.">Effacer le mot de passe administrateur</a>";   
			echo "<br/>+ <a href=./webservice6.php?action=isalive>Test \"isalive\"</a>"; 
			if (_AUTH_=="TRUE") echo "<br/>+ <a href=./logout.php>D&eacute;connexion</a>";    			
			if (_ENCRYPT_=="TRUE")
				echo "<br/><br/>Chiffrement : activ&eacute;";
			else
				echo "<br/><br/>Chiffrement : non activ&eacute;";
			
			echo "</font>";
		}
	}
	else
	{
		echo "<font face=verdana size=2>L'affichage du menu n'est pas autoris&eacute;.";
		echo "</font>";
	}
	echo "</html>";
}
else if ($_GET['action']=="")
{
}
else
{
	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	echo "<font face=verdana size=1>Commande inconnue : ".$_GET['action'];
	echo "</font>";
	echo "</html>";
}

?>
