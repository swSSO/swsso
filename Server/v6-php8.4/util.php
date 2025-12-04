<?php
//-----------------------------------------------------------------------------
//                                  swSSO
//                Copyright (C) 2004-2026 - Sylvain WERDEFROY
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
// VERSION INTERNE : 6.7.0 (PHP7)
// VERSION INTERNE : 6.8.0 -- ISSUE#415 (PHP8.4)
//------------------------------------------------------------------------------

// ------------------------------------------------------------
// sqliConnect()
// ------------------------------------------------------------
function sqliConnect() 
{
	$cnx = mysqli_init();
	mysqli_real_connect($cnx,_HOST_,_USER_,_PWD_,_DB_,3306,null,MYSQLI_CLIENT_FOUND_ROWS);
	if (mysqli_connect_errno()) 
	{
		sqliError($cnx,"mysqli_connect");
		$cnx=null;
	}
	return $cnx;
}
// ------------------------------------------------------------
// sqliClose()
// ------------------------------------------------------------
function sqliClose($cnx)
{
	mysqli_close($cnx);
}
// ------------------------------------------------------------
// sqliError()
// ------------------------------------------------------------
function sqliError($cnx,$szRequest)
{
	echo "<error>";
	echo "+ Serveur : "._HOST_."</br>";
	echo "+ Base    : "._DB_."</br>";
	echo "+ Request : ".$szRequest."</br>";
	if ($cnx) echo "+ Error   : ".mysqli_errno($cnx)." (".mysqli_error($cnx).")</br>";
	echo "</error>";
}
?>
