<?php
//-----------------------------------------------------------------------------
//                                  swSSO
//                Copyright (C) 2004-2026 - Sylvain WERDEFROY
//                         https://github.com/swSSO
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
include('sessions.php');

if (!isSuperAdminAuthorized())
{
	header('Location: ./login.php');
	exit();
}

$message="";

if (isset($_POST['reset'])) 
{
	if (!empty($_POST['pwd1']) && !empty($_POST['pwd2']))
	{
		if (!isSuperAdminAuthorized())
		{
			$message="Vous n'avez pas l'autorisation de réinitialiser les mots de passe.";
		}
		else 
		{
			if ($_POST['pwd1']!=$_POST['pwd2'])
			{
				$message="Les deux mots de passe ne sont pas identiques.";
			}
			else
			{
				if (resetpwd($_SESSION['useridreset'],$_POST['pwd1'])==0)
				{
					header('Location: ./superadmin.php');
					exit();
				}
				else
				{
					$message="Le mot de passe n'a pas été réinitialisé.";
				}
			}
		}
	}
	else
	{
		$message="Veuillez compléter tous les champs avant de valider.";
	}
}
else if (isset($_POST['cancel'])) 
{
	header('Location: ./superadmin.php');
	exit();	
}
else if (isset($_POST['logout'])) 
{
    header('Location: ./logout.php');
	exit();
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="fr">
	<head>
		<title>swSSO - R&eacute;initialisation de mot de passe</title>
	</head>
	<body bgcolor="#F2F2F2">
		<font face=verdana size=2>
			<table border=0px style="font-family:verdana; font-size:12px; border-spacing:0px;" align=left width=800px>
				<tr height=35px style="color:#FFFFFF; background-color:#6080B0;">
					<td width=80% align=left style="padding:10px">
						<b>R&eacute;initialisation de mot de passe</b> [Utilisateur connect&eacute; : <?php echo $_SESSION['userid'];?>] 
					</td>
					<td width=20% align=center>
						<form action="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" method="post">			
							<INPUT TYPE=submit NAME="logout" VALUE="D&eacute;connexion" style="font-family:Verdana; font-size:12px; width:120px;">
						</form>	
					</td>
				</tr>
				<tr height=15px>
				</tr>				
			</table>
			<table border=0 style="font-family:verdana; font-size:12px" width=800px>
				<caption height=50px style="color:#FF0000; padding:8px;">
				<?php
				if(!empty($message)) 
				{
					echo htmlspecialchars($message);
				}
				?>
				</caption>
				<form action="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" method="post">
					<tr height=35px><td align=right width=20%>Identifiant :</td><td width=80%><b><?php echo $_SESSION['useridreset']; ?></b></tr>
					<tr height=35px><td align=right width=20%>Mot de passe : </td><td width=80%><INPUT TYPE="password" ID="defaultfocus" NAME="pwd1" autocomplete="off" style="width:80%" maxlength='50'></td></tr>
					<tr height=35px><td align=right width=20%>V&eacute;rification : </td><td width=80%><INPUT TYPE="password" NAME="pwd2" autocomplete="off" style="width:80%" maxlength='50'></td></tr>
					<tr><td></td><td align=left><INPUT TYPE=submit NAME=reset VALUE="Valider" style="font-family:Verdana; font-size:12px; width:80px;">
					<INPUT TYPE=submit NAME=cancel VALUE="Annuler" style="font-family:Verdana; font-size:12px;width:80px;"></td></tr>
				</form>
			</table>
		</font>
	</body>
	<script>
	  document.getElementById('defaultfocus').focus();
	</script>
</html>
