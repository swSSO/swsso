<?php
include('variables.php');
include('util.php');
include('functions.php');
include('sessions.php');

$message="";

if (countSuperadmin()==0)
{
	header('Location: ./createadmin.php');
	exit();
}

if (!empty($_POST)) 
{
	if(!empty($_POST['id']) && !empty($_POST['pwd'])) 
	{
		logout();
		$rc=login($_POST['id'],$_POST['pwd']);
		if ($rc==0)
		{
			if (isSuperAdminAuthorized())
			{
				header('Location: ./superadmin.php');
				exit();
			}
			if (isAdminAuthorized())
			{
				header('Location: ./admin.php?action=menu');
				exit();
			}
			$message="Utilisateur non autorisé";
			logout();
			exit();
		}
		else if ($rc==-1)
		{
			$message="Identifiant ou mot de passe incorrect";
		}
		else if ($rc==-2)
		{
			$message="Compte verrouillé";
		}
		else if ($rc==-4)
		{
			$message="ok je vois...";
		}
	}
	else
	{
		$message="Veuillez saisir votre identifiant et votre mot de passe";
	}
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="fr">
	<head>
		<title>swSSO - Connexion au serveur de configuration</title>
	</head>
	<body <body bgcolor="#F2F2F2">
		<font face=verdana size=2>
			<form action="<?php echo htmlspecialchars($_SERVER['PHP_SELF']); ?>" method="post">
				<table border=0 style="font-family:verdana; font-size:12px" align=center width=380px>
					<caption height=50px style="color:#FFFFFF; background-color:#6080B0; padding:8px;">Connexion au serveur de configuration swSSO</caption>
					<caption height=50px style="color:#FF0000; padding:8px;">
					<?php
					if(!empty($message)) 
					{
						echo htmlspecialchars($message);
					}
					?>
					</caption>
					<tr height=35px><td align=right width=30%>Identifiant :</td><td width=70%><INPUT TYPE="text" NAME="id" ID="defaultfocus" autocomplete="off" style="width:80%" maxlength='50'></td></tr>
					<tr height=35px><td align=right width=30%>Mot de passe : </td><td width=70%><INPUT TYPE="password" NAME="pwd" autocomplete="off" style="width:80%" maxlength='50'></td></tr>
					<tr><td></td><td align=left><INPUT TYPE=submit VALUE="Connexion" style="font-family:Verdana; font-size:12px;"></td></tr>
				</table>
			</form>
		</font>
	</body>
<script>
  document.getElementById('defaultfocus').focus();
</script></html>
