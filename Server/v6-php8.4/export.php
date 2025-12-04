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
include('variables.php');
include('util.php');
include('functions.php');

// ------------------------------------------------------------
// export des stats
// ------------------------------------------------------------
if ($_GET['data']=="stats")
{
	exportStats();
}
// ------------------------------------------------------------
// export des config actives
// ------------------------------------------------------------
else if ($_GET['data']=="configs")
{
	$var_domain="0";
	if (isset($_GET["domain"])) $var_domain=addslashes($_GET['domain']);
 	showAll(1,$var_domain,"",1);
}
// ------------------------------------------------------------
// export des config archivees
// ------------------------------------------------------------
else if ($_GET['data']=="archived")
{
	$var_domain="0";
	if (isset($_GET["domain"])) $var_domain=addslashes($_GET['domain']);
  	showAll(0,$var_domain,"",1);
}
?>
