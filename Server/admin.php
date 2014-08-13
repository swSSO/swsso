<?php
include('variables.php');
include('util.php');
include('functions.php');
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

$title="<title>swSSO - Serveur de configuration v1.03</title>";

// ------------------------------------------------------------
// showall : génère une page html avec l'ensemble des configs actives
// ------------------------------------------------------------
if ($_GET['action']=="showall"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	showAll(1,$var_domain);
}
// ------------------------------------------------------------
// showold : génère une page html avec l'ensemble des configs archivées
//           Remarque : dans la V4 les configurations ne sont plus archivées
// ------------------------------------------------------------
else if ($_GET['action']=="showold"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	showAll(0,$var_domain);
}
// ------------------------------------------------------------
// showlogs : génère une page html avec les logs
// ------------------------------------------------------------
else if ($_GET['action']=="showlogs"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	$var_result=utf8_decode(addslashes($_GET['result']));
	showLogs($var_domain,$var_result);
}
// ------------------------------------------------------------
// showcategories : génère une page html avec liste des catégories
// ------------------------------------------------------------
else if ($_GET['action']=="showcategories"._READSUFFIX_)
{
	$var_domain=utf8_decode(addslashes($_GET['domain']));
	showCategories($var_domain);
}
// ------------------------------------------------------------
// showdomains : génère une page html avec liste des domaines
// ------------------------------------------------------------
else if ($_GET['action']=="showdomains"._READSUFFIX_)
{
	showDomains();
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
// showstats : génère une page html avec le compteur de stat
// ------------------------------------------------------------
else if ($_GET['action']=="showstats"._READSUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Statistiques</b><br/><br/>";

	$szRequest="select getversion from "._TABLE_PREFIX_."stats where id=0";
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }
	$ligne=mysql_fetch_row($req);
	echo "<font face=verdana size=2>";
	echo "Nombre d'appels getversion : ".$ligne[0]."<br/><br/>";
	echo "<td><a href=\"./admin.php?action=razstats"._WRITESUFFIX_."\" ".
		" onclick=\"return confirm('Confirmez-vous la remise &agrave; z&eacute;ro des statistiques ?');\">Remise &agrave; z&eacute;ro des statistiques</a></td>";
	echo "<br/>";
	echo "</font>";
	echo "</html>";
	dbClose($cnx);
}
// ------------------------------------------------------------
// razstats : remise à 0 des stats
// ------------------------------------------------------------
else if ($_GET['action']=="razstats"._WRITESUFFIX_)
{
	$cnx=dbConnect();
	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Statistiques</b><br/><br/>";
	
	$szRequest="update "._TABLE_PREFIX_."stats set getversion=0 where id=0";      

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }
	echo "<font face=verdana size=2>";
	echo "Remise &agrave; z&eacute;ro des statistiques effectu&eacute;e.<br/>";
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
	showAll($var_active,$var_domain);
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
	
	$szRequest="delete from "._TABLE_PREFIX_."config where id=".$var_id." AND active=0";

	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	dbClose($cnx);
	showAll($var_active,$var_domain);
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
	showCategories(var_domain);
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
	showDomains();
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
		$var_domain=utf8_decode(myaddslashes($_GET['domain'])); 
		if ($var_domain=="") // menu principal
		{
			echo "<font face=verdana size=2><b>Menu principal</b><br/>";
			echo "<br/>+ G&eacute;rer mes domaines";
			menuShowDomains();
			echo "<br/>+ <a href=./admin.php?action=showdomains"._READSUFFIX_.">Ajouter ou supprimer un domaine</a>";
			if (_STATS_=="TRUE") echo "<br/>+ <a href=./admin.php?action=showstats"._READSUFFIX_.">Statistiques</a>";      
			echo "<br/>+ <a href=./webservice5.php?action=isalive>Test \"isalive\"</a>";   
			if (_ENCRYPT_=="TRUE")
				echo "<br/><br/>Chiffrement : activ&eacute;";
			else
				echo "<br/><br/>Chiffrement : non activ&eacute;";
			echo "</font>";
		}
		else // menu du domaine
		{
			echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Gestion du domaine ".getDomainLabel(null,$var_domain)."</b><br/>";
			echo "<br/>+ <a href=./admin.php?action=showall"._READSUFFIX_."&domain=".$var_domain.">Configurations actives</a>";      
			echo "<br/>+ <a href=./admin.php?action=showold"._READSUFFIX_."&domain=".$var_domain.">Configurations archiv&eacute;es</a>";      
			echo "<br/>+ <a href=./admin.php?action=showcategories"._READSUFFIX_."&domain=".$var_domain.">Cat&eacute;gories</a>"; 
			if (_LOGS_=="TRUE") echo "<br/>+ <a href=./admin.php?action=showlogs"._READSUFFIX_."&domain=".$var_domain.">Logs</a>";      
			if (_LOGS_=="TRUE") echo "<br/>+ <a href=./admin.php?action=deletelogs"._WRITESUFFIX_."&domain=".$var_domain.">Effacer les logs</a>";   
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
