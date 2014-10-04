<?php
include('variables.php');
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
