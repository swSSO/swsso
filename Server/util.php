<?php
include('variables.php');

//------------------------------------------------------------------------------
// UTIL.PHP
//------------------------------------------------------------------------------
//
//                      Serveur de configurations swSSO
//
//                 Copyright (C) 2004-2012 - Sylvain WERDEFROY
//
//                           http://www.swsso.fr
//                   
//                             contact@swsso.fr
//
//------------------------------------------------------------------------------
// LICENCE : Cette composante serveur de swSSO n'est pas sous licence GNU GPL.
//           L'utilisation, la modification ou la diffusion de ce code source
//           sont interdites sauf autorisation écrite préalable.
//------------------------------------------------------------------------------

// ------------------------------------------------------------
// dbConnect()
// ------------------------------------------------------------
function dbConnect() 
{
	$cnx = mysql_connect(_HOST_,_USER_,_PWD_,false,2); //CLIENT_FOUND_ROWS=2
	if (!$cnx) 
	{
		dbError($cnx,"mysql_connect");
	}
	else 
	{
		$ok = mysql_select_db(_DB_, $cnx);
		if (!$ok)
		{
			dbError($cnx,"mysql_select_db");
			$cnx=null;
		}
	}
	return $cnx;
}
// ------------------------------------------------------------
// dbClose()
// ------------------------------------------------------------
function dbClose($cnx)
{
	if ($cnx) mysql_close($cnx);
	$cnx=null;
}
// ------------------------------------------------------------
// dbError()
// ------------------------------------------------------------
function dbError($cnx,$szRequest)
{
	echo "<error>";
	echo "+ Serveur : "._HOST_."</br>";
	echo "+ Base    : "._DB_."</br>";
	echo "+ Request : ".$szRequest."</br>";
	if ($cnx) echo "+ Error   : ".mysql_errno($cnx)." (".mysql_error($cnx).")</br>";
	echo "</error>";
}
// ------------------------------------------------------------
// myaddslashes()
// ------------------------------------------------------------
function myaddslashes($str)
{
	if (!get_magic_quotes_gpc()) 
	{
		return addslashes($str);
	} else 
	{
		return $str;
	}
}
?>
