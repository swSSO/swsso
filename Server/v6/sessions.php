<?php
//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2017 - Sylvain WERDEFROY
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
// VERSION INTERNE : 6.5.3
// - Nouvelles fonctions de gestion des sessions admin
//------------------------------------------------------------------------------

// ------------------------------------------------------------
// checkPwd
// ------------------------------------------------------------
// rc : 0=OK, -1=utilisateur inconnu ou mot de passe incorrect, -2 compte verrouillé
// ------------------------------------------------------------
function checkPwd($id,$pwd)
{
	$cnx=sqliConnect();	if (!$cnx) return;
	$szRequest="select userpwd,userlocked from "._TABLE_PREFIX_."admins where userid=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"s",$id);
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$hashPwd,$locked);
		$fetch=mysqli_stmt_fetch($stmt);
		mysqli_stmt_close($stmt);
	}
	sqliClose($cnx);
	if ($fetch==NULL) return -1;
	if ($locked) return -2;
	if (password_verify($pwd,$hashPwd))
		return 0;
	else
		return -1;
}
// ------------------------------------------------------------
// login
// ------------------------------------------------------------
// rc : 0=OK, -1=utilisateur inconnu ou mot de passe incorrect, -2 compte verrouillé
// ------------------------------------------------------------
function login($id,$pwd)
{
	$rc=checkPwd($id,$pwd);
	if ($rc==0)
	{
		$cnx=sqliConnect();	if (!$cnx) return;
		$szRequest="select userrole from "._TABLE_PREFIX_."admins where userid=?";
		$stmt=mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_bind_param($stmt,"s",$id);
			mysqli_stmt_execute($stmt);
			mysqli_stmt_bind_result($stmt,$role);
			$fetch=mysqli_stmt_fetch($stmt);
			mysqli_stmt_close($stmt);
		}
		sqliClose($cnx);
		if ($fetch==NULL) return -1;
		session_start();
		$_SESSION['userid']=$id;
		$_SESSION['userrole']=$role;
	}
	return $rc;
}
// ------------------------------------------------------------
// logout
// ------------------------------------------------------------
function logout()
{
	if(!isset($_SESSION)) session_start();
	session_unset();
	session_destroy();
}
// ------------------------------------------------------------
// isLoggedIn
// ------------------------------------------------------------
function isLoggedIn()
{
	if(!isset($_SESSION)) session_start();
	return isset($_SESSION['userid']);
}
// ------------------------------------------------------------
// isClientReadAuthorized()
// ------------------------------------------------------------
function isClientReadAuthorized()
{
	if (!isset($_SERVER['HTTP_USER_AGENT']) || $_SERVER['HTTP_USER_AGENT']!="swsso.exe") 
	{
		header("HTTP/1.0 403 Forbidden"); 
		return FALSE;
	}
	return TRUE;
}
// ------------------------------------------------------------
// isClientWriteAuthorized
// ------------------------------------------------------------
function isClientWriteAuthorized()
{
	if (!isClientReadAuthorized()) return FALSE;
	if (_AUTH_=="TRUE" && !isLoggedIn()) 
	{
		header("HTTP/1.0 401 Unauthorized");
		return FALSE;
	}
	return TRUE;
}
// ------------------------------------------------------------
// isAdminAuthorized
// ------------------------------------------------------------
function isAdminAuthorized()
{
	if (_AUTH_=="TRUE" && !isLoggedIn()) 
	{
		header("HTTP/1.0 401 Unauthorized");
		return FALSE;
	}
	return TRUE;
}// ------------------------------------------------------------
// isSuperAdminAuthorized
// ------------------------------------------------------------
function isSuperAdminAuthorized()
{
	if (_AUTH_=="TRUE" && !isLoggedIn()) 
	{
		header("HTTP/1.0 401 Unauthorized");
		return FALSE;
	}
	if (!isset($_SESSION['userrole']) || $_SESSION['userrole']!="superadmin")
	{
		header("HTTP/1.0 401 Unauthorized");
		return FALSE;
	}
	return TRUE;
}
// ------------------------------------------------------------
// createUser
// ------------------------------------------------------------
// rc : 0=OK, -1=erreur technique, -2=politique mdp
// ------------------------------------------------------------
function createUser($id,$pwd,$role,$firstname,$lastname)
{
	// TODO
	
	// vérif des rôles
	if ($role!="admin" && $role!="superadmin") return -1;
	
	$pwdHash=password_hash($pwd,PASSWORD_DEFAULT);
	
	$cnx=sqliConnect(); if (!$cnx) return -1;
	
	// ajout de l'utilisateur dans la table des admins
	$szRequest="insert into "._TABLE_PREFIX_."admins (userid,userpwd,userrole,userfirstname,userlastname) values (?,?,?,?,?)";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"sssss",$id,$pwdHash,$role,$firstname,$lastname);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
			$rc=0;
		else
			$rc=-1;
		mysqli_stmt_close($stmt);
	}
	
	sqliClose($cnx);
	return $rc;
}
// ------------------------------------------------------------
// lockUser()
// ------------------------------------------------------------
// rc : 0=OK, -1=erreur technique/utilisateur non trouvé
// ------------------------------------------------------------
function lockUser($id,$locked)
{
	$cnx=sqliConnect(); if (!$cnx) return -1;
	
	$szRequest="update "._TABLE_PREFIX_."admins set userlocked=? where userid=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"ds",$locked,$id);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
			$rc=0;
		else
			$rc=-1;
		mysqli_stmt_close($stmt);
	}
	
	sqliClose($cnx);
	return $rc;
}
// ------------------------------------------------------------
// resetPwd()
// ------------------------------------------------------------
// rc : 0=OK, -1=erreur technique/utilisateur non trouvé
// ------------------------------------------------------------
function resetPwd($id,$pwd)
{
	$cnx=sqliConnect(); if (!$cnx) return -1;
	
	$pwdHash=password_hash($pwd,PASSWORD_DEFAULT);
	
	$szRequest="update "._TABLE_PREFIX_."admins set userpwd=? where userid=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"ss",$pwdHash,$id);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
			$rc=0;
		else
			$rc=-1;
		mysqli_stmt_close($stmt);
	}
	
	sqliClose($cnx);
	return $rc;
}
?>
