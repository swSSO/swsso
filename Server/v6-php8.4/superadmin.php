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
// VERSION INTERNE : 6.7.0 (PHP7)
//------------------------------------------------------------------------------
include('variables.php');
include('util.php');
include('sessions.php');

if (!isSuperAdminAuthorized())
{
	header('Location: ./login.php');
	exit();
}
if (isset($_POST['create']))
{
    header('Location: ./createadmin.php');
	exit();
}
elseif (isset($_POST['logout']))
{
    header('Location: ./logout.php');
	exit();
}
elseif (isset($_POST['disable']))
{
	lockUser($_POST['useridlock'],1);
}
elseif (isset($_POST['enable']))
{
	lockUser($_POST['useridlock'],0);
}
else if (isset($_POST['reset']))
{
    $_SESSION['useridreset']=$_POST['useridreset'];
	header('Location: ./resetpwd.php');
	exit();
}
else if (isset($_POST['change']))
{
    $_SESSION['useridchange']=$_POST['useridchange'];
	header('Location: ./changepwd.php');
	exit();
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="fr">
	<head>
		<title>swSSO - Gestion des administrateurs</title>
	</head>
	<body bgcolor="#F2F2F2">
		<font face=verdana size=2>
			<table border=0px style="font-family:verdana; font-size:12px; border-spacing:0px;" align=left width=800px>
				<tr height=35px style="color:#FFFFFF; background-color:#6080B0;">
					<td width=80% align=left style="padding:10px">
						<b>Gestion des administrateurs</b> [Utilisateur connect&eacute; : <?php echo $_SESSION['userid'];?>] 
					</td>
					<td width=20% align=center>
						<form action="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" method="post">			
							<INPUT TYPE=submit NAME="logout" VALUE="D&eacute;connexion" style="font-family:Verdana; font-size:12px; width:120px;" maxlength="50">
						</form>	
					</td>
				</tr>
				<tr height=15px>
				</tr>				
			</table>
			<table border=0 style="font-family:verdana; font-size:12px; border-spacing:0px 3px;" align=left width=800px>
				<tr height=30px style="background-color:#DDDDFF;">
					<th width=15% >Identifiant</th>
					<th width=10%>Role</th>
					<th width=35%>Pr&eacute;nom Nom</th>
					<th width=15%>Etat</th>
					<th width=15%>Actions</th>
					<th width=20%>Mot de passe</th>
				</tr>
<?php
	$cnx=sqliConnect();
	if ($cnx)
	{
		$szRequest="select userid,userfirstname,userlastname,userrole,userlocked from "._TABLE_PREFIX_."admins";
		$stmt = mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_execute($stmt);
			mysqli_stmt_bind_result($stmt,$userid,$userfirstname,$userlastname,$userrole,$userlocked);
			while (mysqli_stmt_fetch($stmt))
			{
				echo "<tr height=30px>";
				echo "<td bgcolor='#FDFDFD' align=center>".$userid."</td>";
				echo "<td bgcolor='#FDFDFD' align=center>".$userrole."</td>";
				echo "<td bgcolor='#FDFDFD' align=center>".$userfirstname." ".$userlastname."</td>";
				if ($userlocked==0)
				{
					echo "<td bgcolor='#77FF77' align=center>Actif</td>";
					if ($_SESSION['userid']!=$userid)
					{
						echo "<td style='padding:0px 10px'><form action='".htmlspecialchars($_SERVER['PHP_SELF'])."' method='post'>";
						echo "<INPUT TYPE=submit NAME='disable' VALUE='Verrouiller' style='font-family:Verdana; font-size:12px; width:100px;' maxlength='50'>";
						echo "<INPUT TYPE=hidden NAME='useridlock' VALUE=".$userid.">";
						echo "</form></td>";
					}
					else
					{
						echo "<td></td>";
					}
				}
				else
				{
					echo "<td bgcolor='#FF7777' align=center>Verrouillé</td>";
					if ($_SESSION['userid']!=$userid)
					{
						echo "<td style='padding:0px 10px'><form action='".htmlspecialchars($_SERVER['PHP_SELF'])."' method='post'>";
						echo "<INPUT TYPE=submit NAME='enable' VALUE='D&eacute;verrouiller' style='font-family:Verdana; font-size:12px; width:100px;' maxlength='50'>";
						echo "<INPUT TYPE=hidden NAME='useridlock' VALUE=".$userid.">";
						echo "</form></td>";
					}
					else
					{
						echo "<td></td>";
					}
				}
				echo "<td style='padding:0px 0px'><form action='".htmlspecialchars($_SERVER['PHP_SELF'])."' method='post'>";
				if ($_SESSION['userid']==$userid)
				{
					echo "<INPUT TYPE=submit NAME='change' VALUE='Changer' style='font-family:Verdana; font-size:12px; width:100px;' maxlength='50'>";
					echo "<INPUT TYPE=hidden NAME='useridchange' VALUE=".$userid.">";
				}
				else
				{
					echo "<INPUT TYPE=submit NAME='reset' VALUE='Réinitialiser' style='font-family:Verdana; font-size:12px; width:100px;' maxlength='50'>";
					echo "<INPUT TYPE=hidden NAME='useridreset' VALUE=".$userid.">";
				}
				echo "</form></td>";
				echo "</tr>";
			}
			mysqli_stmt_close($stmt);
		}
		sqliClose($cnx);
	}
?>
				<form action="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" method="post">
					<tr height=35px>
						<td>
							<INPUT TYPE=submit NAME="create" VALUE="Cr&eacute;er" style="font-family:Verdana; font-size:12px; width:120px;">
						</td>
					</tr>		
				</form>	
			</table>
		 </font>
	</body>
</html>