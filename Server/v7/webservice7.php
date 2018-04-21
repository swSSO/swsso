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
// webservice6.PHP : Utilisé à partir de la version swSSO 1.05 pour la gestion des domaines multiples.
//                   Le client 1.05 et les suivants resteront compatibles avec webservice5.php tant
//                   qu'ils n'auront pas besoin de la gestion des domaines multiples
//------------------------------------------------------------------------------
// VERSION INTERNE : 7.0.0
//------------------------------------------------------------------------------
include('variables.php');
include('util.php');
include('functions.php');
include('sessions.php');

$swssoVersion="000:0000"; // "000:0000" désactive le contrôle de version côté client

// ------------------------------------------------------------
// isalive
// ------------------------------------------------------------
if ($_GET['action']=="isalive")
{
    $cnx=sqliConnect();	
	if ($cnx) {
		echo "ALIVE";
		sqliClose($cnx);
	}
}
// ------------------------------------------------------------
// login
// -----------------------------------------------------------
else if ($_GET['action']=="login")
{
	if (!isClientReadAuthorized()) return;
	if (_AUTH_=="TRUE")
	{
		if (isset($_POST['id']) && isset($_POST['pwd']))
		{
			$var_userid			=utf8_decode(myaddslashes($_POST['id'])); 
			$var_userpwd		=utf8_decode(myaddslashes($_POST['pwd'])); 
			$rc=login($var_userid,$var_userpwd);
			echo $rc;
		}
		else
		{
			header("HTTP/1.0 400 Bad Request");
		}
	}
	else
	{
		echo "0";
	}
}
// ------------------------------------------------------------
// logout
// -----------------------------------------------------------
else if ($_GET['action']=="logout")
{
	if (!isClientWriteAuthorized()) return;
	logout();
	echo "0";
}
// ------------------------------------------------------------
// resetPwd
// -----------------------------------------------------------
else if ($_GET['action']=="resetPwd")
{
	if (!isClientWriteAuthorized()) return;
	if (_AUTH_=="TRUE")
	{
		if (isset($_POST['newPwd']))
		{
			$var_newpwd	=utf8_decode(myaddslashes($_POST['newPwd'])); 
			if (resetPwd($_SESSION['userid'],$var_newpwd)==0)
			{
				echo "0"; // changement OK
			}
			else
			{
				echo "-1"; // changement KO 
			}
		}
		else
		{
			header("HTTP/1.0 400 Bad Request");
		}
	}
	else
	{
		echo "0";
	}
}
// ------------------------------------------------------------
// getconfig
// ------------------------------------------------------------
else if ($_GET['action']=="getconfig")
{
	if (!isClientReadAuthorized()) return;

	$var_title			="";
	$var_url  			="";
	$var_ids  			="";
	$var_new			="";
	$var_mod			="";
	$var_old			="";
	$var_modifiedSince	="";
	$var_type			="";
	$var_version		="";
	$var_domainId		="";

	if (isset($_GET["title"]))		$var_title			=utf8_decode(myaddslashes($_GET['title'])); 
	if (isset($_GET["url"]))		$var_url  			=utf8_decode(myaddslashes($_GET['url']));
	if (isset($_GET["ids"]))		$var_ids  			=utf8_decode(myaddslashes($_GET['ids']));  	// liste d'ids de config
	if (isset($_GET["new"]))		$var_new			=utf8_decode(myaddslashes($_GET['new']));	// retourne configs non comprises dans la liste & active=1
	if (isset($_GET["mod"]))		$var_mod			=utf8_decode(myaddslashes($_GET['mod']));	// retourne configs comprises dans la liste & active=1
	if (isset($_GET["old"]))		$var_old			=utf8_decode(myaddslashes($_GET['old'])); 	// retourne configs comprises dans la liste & active=0
	if (isset($_GET["date"]))		$var_modifiedSince	=utf8_decode(myaddslashes($_GET['date']));	// date à partir de laquelle on veut récupérer les configs
	if (isset($_GET["type"]))		$var_type			=utf8_decode(myaddslashes($_GET['type']));  // 0.92B6 : type de la config recherchée (WIN, POP, WEB ou XEB) ou chaine vide sinon
	if (isset($_GET["version"]))	$var_version		=utf8_decode(myaddslashes($_GET['version'])); // 0.92B8 : version client swsso (format 0928)
	if (isset($_GET["domainId"]))	$var_domainId		=utf8_decode(myaddslashes($_GET['domainId'])); // 0.94 domaine 
	
	if ($var_new=="") $var_new="0";
	if ($var_mod=="") $var_mod="0";
	if ($var_old=="") $var_old="0";

	getconfig($var_title,$var_url,$var_ids,$var_new,$var_mod,$var_old,$var_modifiedSince,$var_type,$var_version,$var_domainId,"0");
}
// ------------------------------------------------------------
// getconfigautopublish
// ------------------------------------------------------------
else if ($_GET['action']=="getconfigautopublish")
{
	if (!isClientReadAuthorized()) return;

	$var_title			="";
	$var_url  			="";
	$var_ids  			="";
	$var_new			="";
	$var_mod			="";
	$var_old			="";
	$var_modifiedSince	="";
	$var_type			="";
	$var_version		="";
	$var_domainId		="";

	if (isset($_GET["title"]))		$var_title			=utf8_decode(myaddslashes($_GET['title'])); 
	if (isset($_GET["url"]))		$var_url  			=utf8_decode(myaddslashes($_GET['url']));
	if (isset($_GET["ids"]))		$var_ids  			=utf8_decode(myaddslashes($_GET['ids']));  	// liste d'ids de config
	if (isset($_GET["new"]))		$var_new			=utf8_decode(myaddslashes($_GET['new']));	// retourne configs non comprises dans la liste & active=1
	if (isset($_GET["mod"]))		$var_mod			=utf8_decode(myaddslashes($_GET['mod']));	// retourne configs comprises dans la liste & active=1
	if (isset($_GET["old"]))		$var_old			=utf8_decode(myaddslashes($_GET['old'])); 	// retourne configs comprises dans la liste & active=0
	if (isset($_GET["date"]))		$var_modifiedSince	=utf8_decode(myaddslashes($_GET['date']));	// date à partir de laquelle on veut récupérer les configs
	if (isset($_GET["type"]))		$var_type			=utf8_decode(myaddslashes($_GET['type']));  // 0.92B6 : type de la config recherchée (WIN, POP, WEB ou XEB) ou chaine vide sinon
	if (isset($_GET["version"]))	$var_version		=utf8_decode(myaddslashes($_GET['version'])); // 0.92B8 : version client swsso (format 0928)
	if (isset($_GET["domainId"]))	$var_domainId		=utf8_decode(myaddslashes($_GET['domainId'])); // 0.94 domaine 
	
	if ($var_new=="") $var_new="0";
	if ($var_mod=="") $var_mod="0";
	if ($var_old=="") $var_old="0";

	getconfig($var_title,$var_url,$var_ids,$var_new,$var_mod,$var_old,$var_modifiedSince,$var_type,$var_version,$var_domainId,"1");
}
// ------------------------------------------------------------
// putconfig TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="putconfig")
{
	if (!isClientWriteAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;

	// récupération des paramètres passés dans l'URL
    $var_configId		=utf8_decode(myaddslashes($_GET['configId']));
    $var_typeapp		=utf8_decode(myaddslashes($_GET['typeapp']));
    $var_title			=utf8_decode(myaddslashes($_GET['title']));
    $var_url			=utf8_decode(myaddslashes($_GET['url']));
    $var_id1Name		=utf8_decode(myaddslashes($_GET['id1Name']));
    $var_pwdName		=utf8_decode(myaddslashes($_GET['pwdName']));
    $var_validateName	=utf8_decode(myaddslashes($_GET['validateName']));
    $var_id2Name		=utf8_decode(myaddslashes($_GET['id2Name']));
    $var_id2Type		=utf8_decode(myaddslashes($_GET['id2Type']));
    $var_id3Name		=utf8_decode(myaddslashes($_GET['id3Name']));
    $var_id3Type		=utf8_decode(myaddslashes($_GET['id3Type']));
    $var_id4Name		=utf8_decode(myaddslashes($_GET['id4Name']));
    $var_id4Type		=utf8_decode(myaddslashes($_GET['id4Type']));
    $var_bKBSim			=utf8_decode(myaddslashes($_GET['bKBSim'])); 
    $var_szKBSim		=utf8_decode(myaddslashes($_GET['szKBSim']));
    $var_szName			=utf8_decode(myaddslashes($_GET['szName']));
    $var_categId        =utf8_decode(myaddslashes($_GET['categId']));
    $var_categLabel     =utf8_decode(myaddslashes($_GET['categLabel']));
    $var_szFullPathName =utf8_decode(myaddslashes($_GET['szFullPathName']));
    $var_lastModified   =utf8_decode(myaddslashes($_GET['lastModified']));
	$var_domainId		=utf8_decode(myaddslashes($_GET['domainId']));
	$var_withIdPwd	="";
	$var_id1Value	="";
	$var_id2Value	="";
	$var_id3Value	="";
	$var_id4Value	="";
	$var_pwdValue	="";
	$var_pwdGroup	="";
	$var_autoLock	="";
	$var_domainAutoPublish="";
	if (isset($_GET["withIdPwd"]))		$var_withIdPwd	=utf8_decode(myaddslashes($_GET['withIdPwd'])); // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id1Value"])) 		$var_id1Value	=utf8_decode(myaddslashes($_GET['id1Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id2Value"])) 		$var_id2Value	=utf8_decode(myaddslashes($_GET['id2Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id3Value"])) 		$var_id3Value	=utf8_decode(myaddslashes($_GET['id3Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id4Value"])) 		$var_id4Value	=utf8_decode(myaddslashes($_GET['id4Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["pwdValue"])) 		$var_pwdValue	=utf8_decode(myaddslashes($_GET['pwdValue']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["pwdGroup"])) 		$var_pwdGroup	=utf8_decode(myaddslashes($_GET['pwdGroup']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["autoLock"])) 		$var_autoLock	=utf8_decode(myaddslashes($_GET['autoLock']));  // ajouté en 5.5 pour client 1.04
	// if (isset($_GET["autoPublish"])) 	$var_autoPublish=utf8_decode(myaddslashes($_GET['autoPublish']));  // ajouté en 6.5 pour client 1.14 - supprimé en 6.5.4 pour client 1.15
	if (isset($_GET["domainAutoPublish"])) 	$var_domainAutoPublish	=utf8_decode(myaddslashes($_GET['domainAutoPublish'])); // ajouté en 6.5.4 pour client 1.15
	
	if ($var_pwdGroup=='') $var_pwdGroup=-1;  		// pour compatibilité avec les clients <1.03 qui ne gèrent pas ce paramètre
	if ($var_withIdPwd=='') $var_withIdPwd=0; 		// pour compatibilité avec les clients <1.03 qui ne gèrent pas ce paramètre
	if ($var_autoLock=='') $var_autoLock=1;   		// pour compatibilité avec les clients <1.04 qui ne gèrent pas ce paramètre
    
	// V6 : gestion des domaines multiples, exemple : $var_domainId=1,3,5
	// Si la configuration existe déjà, supprime toutes les associations dans la table configs_domains
	// sauf si le client demande explicitement à ne pas les modifier (cas concret : déplacement d'une config d'une categorie à une autre)
	if ($var_configId!="0" && $var_domainId!="DONTCHANGE")
	{
		$szRequest="delete from "._TABLE_PREFIX_."configs_domains where configId=".$var_configId;
		if (isset($_GET["debug"])) echo $szRequest;
		$result=mysql_query($szRequest,$cnx);
		if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
	}
	
	// V4 : gestion des catégories
	// Ajoute la catégorie si n'existe pas ou met à jour le label
	if ($var_categId!="" AND $var_categLabel!="") // ne fait rien si pas d'info categorie remontée
	{
		$szRequest="update "._TABLE_PREFIX_."categ set label='".$var_categLabel."' WHERE id='".$var_categId."'";
		if (isset($_GET["debug"])) echo $szRequest;
    	$result=mysql_query($szRequest,$cnx);
		if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		if (mysql_affected_rows()==0) // la catégorie n'existe pas, on la crée
		{
			// lecture du dernier categId affecté
			$szRequest="select max(id) from "._TABLE_PREFIX_."categ";
			if (isset($_GET["debug"])) echo $szRequest;
	    	$result=mysql_query($szRequest,$cnx);
			if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
			$max=mysql_fetch_row($result);
			$var_categId=$max[0];
			if ($var_categId==0) $var_categId=10000; else $var_categId=$var_categId+1;
			// incrément et ajout de la catégorie
			$szRequest="insert into "._TABLE_PREFIX_."categ (id,label) values ('".$var_categId."','".$var_categLabel."')";
			if (isset($_GET["debug"])) echo $szRequest;
	    	$result=mysql_query($szRequest,$cnx);
			if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		}
	}

	// En fonction de la configuration (chiffrement ou pas), prépare les paramètres de la requête SQL
	$param_id1Value="";
	$param_id2Value="";
	$param_id3Value="";
	$param_id4Value="";
	$param_pwdValue="";
	if (_ENCRYPT_=="TRUE")
	{
		$param_url=           "AES_ENCRYPT('".$var_url."','"._AESPWD_."')";
		$param_title=         "AES_ENCRYPT('".$var_title."','"._AESPWD_."')";
		$param_szFullPathName="AES_ENCRYPT('".$var_szFullPathName."','"._AESPWD_."')";
		$param_szName=        "AES_ENCRYPT('".$var_szName."','"._AESPWD_."')";
		if ($var_withIdPwd!=0)
		{
			$param_id1Value=  "AES_ENCRYPT('".$var_id1Value."','"._AESPWD_."')";
			$param_id2Value=  "AES_ENCRYPT('".$var_id2Value."','"._AESPWD_."')";
			$param_id3Value=  "AES_ENCRYPT('".$var_id3Value."','"._AESPWD_."')";
			$param_id4Value=  "AES_ENCRYPT('".$var_id4Value."','"._AESPWD_."')";
			$param_pwdValue=  "AES_ENCRYPT('".$var_pwdValue."','"._AESPWD_."')";
		}
	}
	else
	{
		$param_url=           "'".$var_url."'";
		$param_title=         "'".$var_title."'";
		$param_szFullPathName="'".$var_szFullPathName."'";
		$param_szName=        "'".$var_szName."'";
	}
	
	// si configId=0 -> ajout et affectation d'un Id 
	//                  ce nouveau mode de fonctionnement va entrainer des doublons dans la base : 
	//                  en effet, il y a deux raisons pour que le configid soit à 0
	//                  1) cas nominal avec cette nouvelle version c'est une nouvelle config -> donc OK pas de doublon
	//                  2) cas des anciennes versions, toutes les configs ont un id à 0 -> on va générer un doublon
	//                     avec un Id, mais ce n'est pas grave car lorsque les utilisateurs feront un getconfig,
	// 					   ils vont récupérer prioritairement les configs avec Id!=0, donc les nouvelles. Au final
	//					   les anciennes config vont donc mourir tranquillement jusqu'à ce que quelqu'un fasse le ménage.
	// si configId!=0 -> remplacement pur et simple (disparition de la notion d'archive)

	if ($var_configId!="0")
	{
		$szRequestOptions="";
		if ($var_withIdPwd!=0) 
		{
			$szRequestOptions=",withIdPwd=".$var_withIdPwd.",".
								 "id1Value=".$param_id1Value.",".
								 "id2Value=".$param_id2Value.",".
								 "id3Value=".$param_id3Value.",".
								 "id4Value=".$param_id4Value.",".
								 "pwdValue=".$param_pwdValue;
		}
		$szRequest="update "._TABLE_PREFIX_."config set typeapp='".$var_typeapp."',".
									  "title=".$param_title.",".
									  "url=".$param_url.",".
									  "id1Name='".$var_id1Name."',".
									  "id1Type='EDIT',".
									  "pwdName='".$var_pwdName."',".
									  "validateName='".$var_validateName."',".
									  "id2Name='".$var_id2Name."',".
									  "id2Type='".$var_id2Type."',".
									  "id3Name='".$var_id3Name."',".
									  "id3Type='".$var_id3Type."',".
									  "id4Name='".$var_id4Name."',".
									  "id4Type='".$var_id4Type."',".
									  "bKBSim='".$var_bKBSim."',".
									  "szKBSim='".$var_szKBSim."',".
									  "szName=".$param_szName.",".
									  "categId='".$var_categId."',".
									  "szFullPathName=".$param_szFullPathName.",".
									  "lastModified='".$var_lastModified."',".
									  "pwdGroup=".$var_pwdGroup.",".
									  "active='1',".
									  "autoLock=".$var_autoLock.
									  $szRequestOptions." WHERE ".
									  _TABLE_PREFIX_."config.id='".$var_configId."'";
		
		if (isset($_GET["debug"])) echo $szRequest;
		$result=mysql_query($szRequest,$cnx);
		if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		
		// si jamais ce configId n'existe plus dans base, on l'ajoute.
		if (mysql_affected_rows()==0) $var_configId="0";
	}
	if ($var_configId=="0") // =0 soit parce que 0 passé en param soit parce que id passé en param non trouvé en base
	{
		$szRequestOptions1="";
		$szRequestOptions2="";
		if ($var_withIdPwd!="0") 
		{
			$szRequestOptions1=",withIdPwd,id1Value,id2Value,id3Value,id4Value,pwdValue";
			$szRequestOptions2=",".$var_withIdPwd.",".$param_id1Value.",".$param_id2Value.",".$param_id3Value.",".$param_id4Value.",".$param_pwdValue;
		}
		
		$szRequest="insert into "._TABLE_PREFIX_."config (active,typeapp,title,url,id1Name,id1Type,pwdName,validateName,".
	           "id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,bKBSim,szKBSim,szName,categId,".
			   "szFullPathName,lastModified,pwdGroup,autoLock".$szRequestOptions1.") ".
	           "values (1,'".$var_typeapp."',".$param_title.",".$param_url.",'".$var_id1Name."','EDIT','".
	           $var_pwdName."','".$var_validateName."','".$var_id2Name."','".$var_id2Type."','".
	           $var_id3Name."','".$var_id3Type."','".$var_id4Name."','".$var_id4Type."','".
	           $var_bKBSim."','".$var_szKBSim."',".$param_szName.",'".
	           $var_categId."',".$param_szFullPathName.",".$var_lastModified.",".$var_pwdGroup.",".$var_autoLock.$szRequestOptions2.")";
		if (isset($_GET["debug"])) echo $szRequest;
		$result=mysql_query($szRequest,$cnx);
		if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		$var_configId=mysql_insert_id(); // id passé en paramètre si !=0, nouvel id sinon
	}
	// V6 : gestion des domaines multiples, exemple : $var_domainId=1,3,5
	// A ce stade, la config a été ajoutée ou mise à jour, on a son id dans $var_configId, 
	// il faut maintenant mettre à jour les associations dans configs_domains
	// sauf si le client demande explicitement à ne pas les modifier (cas concret : déplacement d'une config d'une categorie à une autre)
	if ($var_domainId!="DONTCHANGE")
	{
		$var_domainsList = explode(",",$var_domainId);
		$var_autoPubList="";
		if ($var_domainAutoPublish!="") 
			$var_autoPubList = explode(",",$var_domainAutoPublish); // client 1.15+
		
		$szRequest="";
		if ($var_autoPubList!="") // client 1.15+
			$szRequest="insert into "._TABLE_PREFIX_."configs_domains (configId,domainId,domainAutoPublish) values ";
		else // client 1.14-
			$szRequest="insert into "._TABLE_PREFIX_."configs_domains (configId,domainId) values ";
		for ($i=0;$i<count($var_domainsList);$i++)
		{
			if ($var_domainsList[$i]=="") continue;
			if ($i!=0)
				$szRequest=$szRequest.",";
			if ($var_autoPubList!="") // client 1.15+
				$szRequest=$szRequest."(".$var_configId.",".$var_domainsList[$i].",".$var_autoPubList[$i].") ";
			else
				$szRequest=$szRequest."(".$var_configId.",".$var_domainsList[$i].") ";
		}
		if ($szRequest!="")
		{
			$szRequest=$szRequest.";";
			if (isset($_GET["debug"])) echo $szRequest;
			$result=mysql_query($szRequest,$cnx);
			if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		}
	}
	sqliClose($cnx);
	echo "OK:".$var_configId.":".$var_categId;
}
// ------------------------------------------------------------
// getversion (format : release:beta)
// release : 080  = 0.80
// beta    : 0811 = 0.81 BETA 1
//           081A = 0.81 BETA 10
//           0000 = pas de BETA disponible
// ------------------------------------------------------------
else if ($_GET['action']=="getversion")
{
	if (!isClientReadAuthorized()) return;
	
	header("Content-type: text/xml; charset=UTF-8");
	echo $swssoVersion;
}
// ------------------------------------------------------------
// getdomains
// ------------------------------------------------------------
else if ($_GET['action']=="getdomains")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;

	// commence par lire le libellé du domaine commun (pour qu'il soit placé en tête de liste puis lit tous les autres)
	$szRequest= "select label from "._TABLE_PREFIX_."domains where id=1";
	$stmt=mysqli_stmt_init($cnx);
	$fetch=NULL;
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$domainLabel);
		$fetch=mysqli_stmt_fetch($stmt);
		mysqli_stmt_close($stmt);
	}
	header("Content-type: text/xml; charset=UTF-8");
	if (!$fetch)
	{
		echo "<domains>NOT FOUND</domains>";	
	}
	else // lit les autres domaines
	{
		echo "<domains>\n";
		echo "<domain num=\"0\">\n";
		echo "<id><![CDATA[1]]></id>\n";
		echo "<label><![CDATA[".$domainLabel."]]></label>\n";
		echo "</domain>\n";
		
		$szRequest= "select id,label from "._TABLE_PREFIX_."domains where id<>1 order by label";

		$stmt=mysqli_stmt_init($cnx);
		$fetch=NULL;
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_execute($stmt);
			mysqli_stmt_bind_result($stmt,$domainId,$domainLabel);
			$fetch=mysqli_stmt_fetch($stmt);
			$i=1;
			while ($fetch)
			{
				echo "<domain num=\"".$i."\">\n";
				echo "<id><![CDATA[".$domainId."]]></id>\n";
				echo "<label><![CDATA[".$domainLabel."]]></label>\n";
				echo "</domain>\n";
				$fetch=mysqli_stmt_fetch($stmt);
				$i++;
			}
		}
		echo "</domains>";
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// getconfigdomains TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="getconfigdomains")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	// récupération des paramètres passés dans l'URL
	$var_configId="-1";
	if (isset($_GET["configId"])) $var_configId=utf8_decode(myaddslashes($_GET['configId']));
	
	$szRequest= "select "._TABLE_PREFIX_."configs_domains.domainId,"._TABLE_PREFIX_."domains.label,"._TABLE_PREFIX_."configs_domains.domainAutoPublish ".
			"from "._TABLE_PREFIX_."configs_domains,"._TABLE_PREFIX_."domains where ".
			"configId=".$var_configId." and "._TABLE_PREFIX_."configs_domains.domainId="._TABLE_PREFIX_."domains.id order by "._TABLE_PREFIX_."configs_domains.domainId";
	if (isset($_GET["debug"])) echo $szRequest;
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	header("Content-type: text/xml; charset=UTF-8");
	if(mysql_num_rows($req)==0) 
	{
		echo "<domains>NOT FOUND</domains>";
	}
	else
	{
		echo "<domains>\n";
		$i=0;
		while($ligne=mysql_fetch_row($req))
		{
			echo "<domain num=\"".$i."\">\n";
			echo "<id><![CDATA[".$ligne[0]."]]></id>\n";
			echo "<label><![CDATA[".$ligne[1]."]]></label>\n";
			echo "<domainAutoPublish><![CDATA[".$ligne[2]."]]></domainAutoPublish>\n";
			echo "</domain>\n";
			$i++;
		}
		echo "</domains>";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// getdomainconfigs TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="getdomainconfigs")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	// récupération des paramètres passés dans l'URL
	$var_domainId="-1";	if (isset($_GET["domainId"])) $var_domainId=utf8_decode(myaddslashes($_GET['domainId']));

	$conditions="";
	if ($var_domainId!=-1) $conditions="and (domainId=1 or domainId=".$var_domainId.")";

	$szRequest= "select "._TABLE_PREFIX_."config.id ".
			"from "._TABLE_PREFIX_."configs_domains,"._TABLE_PREFIX_."config where ".
			"active=1 ".$conditions." and configId="._TABLE_PREFIX_."config.id";
	if (isset($_GET["debug"])) echo $szRequest;
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	header("Content-type: text/xml; charset=UTF-8");
	if(mysql_num_rows($req)==0) 
	{
		echo "NONE";
	}
	else
	{
		while($ligne=mysql_fetch_row($req))
		{
			echo $ligne[0].",";
		}
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// getdomainconfigsautopublish TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="getdomainconfigsautopublish")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	// récupération des paramètres passés dans l'URL
	$var_domainId="-1";	if (isset($_GET["domainId"])) $var_domainId=utf8_decode(myaddslashes($_GET['domainId']));
	
	$conditions="";
	if ($var_domainId!=-1) $conditions="and domainId=".$var_domainId;

	$szRequest="select "._TABLE_PREFIX_."config.id, configs_domains.domainAutoPublish ".
			"from "._TABLE_PREFIX_."configs_domains,"._TABLE_PREFIX_."config where ".
			"active=1 ".$conditions." and configId="._TABLE_PREFIX_."config.id order by "._TABLE_PREFIX_."config.id ASC";
	if (isset($_GET["debug"])) echo $szRequest;
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	header("Content-type: text/xml; charset=UTF-8");
	if(mysql_num_rows($req)==0) 
	{
		echo "<configs>NOT FOUND</configs>";
	}
	else
	{
		echo "<configs>\n";
		$i=0;
		while($ligne=mysql_fetch_row($req))
		{
			echo "<config num=\"".$i."\">\n";
			echo "<id><![CDATA[".$ligne[0]."]]></id>\n";
			echo "<domainAutoPublish><![CDATA[".$ligne[1]."]]></domainAutoPublish>\n";
			echo "</config>\n";
			$i++;
		}
		echo "</configs>";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// isadminpwd TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="isadminpwdset")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	$szRequest= "select pwd from "._TABLE_PREFIX_."adminpwd where pwd!=''";
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	header("Content-type: text/xml; charset=UTF-8");
	if(mysql_num_rows($req)==0) 
	{
		echo "NO";
	}
	else
	{
		echo "YES";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// checkadminpwd TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="checkadminpwd")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	$var_salt="";
	$var_pwd="";
	if (isset($_GET["salt"])) $var_salt=utf8_decode(myaddslashes($_GET['salt']));
	if (isset($_GET["pwd"])) $var_pwd=utf8_decode(myaddslashes($_GET['pwd']));
	
	if ($var_salt=="" || $var_pwd=="")
	{
		echo "KO";
	}
	else
	{
		// calcul le hash du pwd salé
		$averifier=sha1($var_salt.$var_pwd);
		
		// lit la pwd salé en base
		$szRequest= "select pwd from "._TABLE_PREFIX_."adminpwd where pwd!=''";
		$req=mysql_query($szRequest,$cnx);
		if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

		header("Content-type: text/xml; charset=UTF-8");
		if(mysql_num_rows($req)==0) 
		{
			echo "KO";
		}
		else
		{
			// compare
			$luenbase=mysql_fetch_row($req);
			if ($luenbase[0]==$averifier)
				echo "OK";
			else
				echo "KO";
		}
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// setadminpwd TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="setadminpwd")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	$var_salt="";
	$var_pwd="";
	if (isset($_GET["salt"])) $var_salt=utf8_decode(myaddslashes($_GET['salt']));
	if (isset($_GET["pwd"])) $var_pwd=utf8_decode(myaddslashes($_GET['pwd']));
	
	if ($var_salt=="" || $var_pwd=="")
	{
		echo "KO";
	}
	else
	{
		// calcul le hash du pwd salé
		$pwdhashesale=sha1($var_salt.$var_pwd);
		
		$szRequest="insert into "._TABLE_PREFIX_."adminpwd (pwd) values ('".$pwdhashesale."')";
		$req=mysql_query($szRequest,$cnx);
		header("Content-type: text/xml; charset=UTF-8");
		if (!$req) 
			echo "KO";
		else
			echo "OK";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// deleteconfig
// ------------------------------------------------------------
else if ($_GET['action']=="deleteconfig")
{
	if (!isClientWriteAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["configId"])) $configId=utf8_decode(myaddslashes($_GET['configId']));
	
	// ISSUE#231 : il faut aussi supprimer le lien config - domaines
	$szRequest="delete from "._TABLE_PREFIX_."configs_domains where configId=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"i",$configId);
		mysqli_stmt_execute($stmt);
		mysqli_stmt_close($stmt);	
	}
	
	$szRequest="delete from "._TABLE_PREFIX_."config where id=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"i",$configId);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
			echo "OK";
		else
			echo "KO";
		mysqli_stmt_close($stmt);	
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// deletecateg
// ------------------------------------------------------------
else if ($_GET['action']=="deletecateg")
{
	if (!isClientWriteAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["categId"])) $categId=utf8_decode(myaddslashes($_GET['categId']));

	$szRequest="delete from "._TABLE_PREFIX_."categ where id=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"i",$categId);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
			echo "OK";
		else
			echo "KO";
		mysqli_stmt_close($stmt);	
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// deletedomain
// ------------------------------------------------------------
else if ($_GET['action']=="deletedomain")
{
	if (!isClientWriteAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["domainId"])) $domainId=utf8_decode(myaddslashes($_GET['domainId']));

	// vérifie qu'aucune config n'est attachée à ce domaine
	$szRequest="select count(*) from "._TABLE_PREFIX_."configs_domains where domainId=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"i",$domainId);
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$nbDomains);
		$fetch=mysqli_stmt_fetch($stmt);
		mysqli_stmt_close($stmt);	
	}
	if ($nbDomains!=0) 
	{
		echo "KO";
	}
	else
	{
		$szRequest="delete from "._TABLE_PREFIX_."domains where id=?";
		$stmt = mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_bind_param($stmt,"i",$domainId);
			mysqli_stmt_execute($stmt);
			if (mysqli_stmt_affected_rows($stmt)>0)
				echo "OK";
			else
				echo "KO";
			mysqli_stmt_close($stmt);	
		}
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// adddomain
// ------------------------------------------------------------
else if ($_GET['action']=="adddomain")
{
	if (!isClientWriteAuthorized()) return;

	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["domainLabel"])) $domainLabel=utf8_decode(myaddslashes($_GET['domainLabel']));
	$szRequest="insert into "._TABLE_PREFIX_."domains (label) values (?)";

	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"s",$domainLabel);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
		{
			$domainId=mysqli_insert_id($cnx);
			echo "OK:".$domainId;
		}
		else
		{
			echo "KO";
		}
		mysqli_stmt_close($stmt);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// renamedomain
// ------------------------------------------------------------
else if ($_GET['action']=="renamedomain")
{
	if (!isClientWriteAuthorized()) return;

	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["domainId"])) $domainId=utf8_decode(myaddslashes($_GET['domainId']));
	if (isset($_GET["domainLabel"])) $domainLabel=utf8_decode(myaddslashes($_GET['domainLabel']));

	$szRequest="update "._TABLE_PREFIX_."domains set label=? where id=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"si",$domainLabel,$domainId);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0)
		{
			echo "OK";
		}
		else
		{
			echo "KO";
		}
		mysqli_stmt_close($stmt);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// getdomainid
// ------------------------------------------------------------
else if ($_GET['action']=="getdomainid")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=sqliConnect();	if (!$cnx) return;
	
	if (isset($_GET["domainLabel"])) $domainLabel=utf8_decode(myaddslashes($_GET['domainLabel']));

	$szRequest="select id from "._TABLE_PREFIX_."domains where label=?";	
	$stmt=mysqli_stmt_init($cnx);
	$fetch=NULL;
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"s",$domainLabel);
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$domainId);
		$fetch=mysqli_stmt_fetch($stmt);
		mysqli_stmt_close($stmt);
	}
	header("Content-type: text/xml; charset=UTF-8");
	if ($fetch)
	{
		echo $domainId;
	}
	else
	{
		echo "NOT FOUND";
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// uploadstats TODO_PHP7
// ------------------------------------------------------------
else if ($_GET['action']=="uploadstats")
{
	if (!isClientReadAuthorized()) return;
	
	$cnx=dbConnect();
	if (!$cnx) return;
	
	$var_shausername=utf8_decode(myaddslashes($_GET['shausername']));
	$var_logindate=utf8_decode(myaddslashes($_GET['logindate']));
	$var_nconfigs=utf8_decode(myaddslashes($_GET['nconfigs']));
	$var_nsso=utf8_decode(myaddslashes($_GET['nsso']));
	$var_nenrolled=utf8_decode(myaddslashes($_GET['nenrolled']));
	$var_computername=utf8_decode(myaddslashes($_GET['computername']));
	$statRecorded=0;
	if (_STATOVERWRITE_=="TRUE")
	{
		// commence par tenter un update, si échec on fera un insert
		$szRequest="update "._TABLE_PREFIX_."stats set logindate='".$var_logindate."',nconfigs='".$var_nconfigs.
					"',nsso='".$var_nsso."',nenrolled='".$var_nenrolled."',computername='".$var_computername."' ".
				   "WHERE shausername='".$var_shausername."'";
    	$result=mysql_query($szRequest,$cnx);
		if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		if (mysql_affected_rows()!=0) $statRecorded=1;
	}
	if ($statRecorded==0)
	{
		$szRequest="insert into "._TABLE_PREFIX_."stats (shausername,logindate,nconfigs,nsso,nenrolled,computername) ".
				   "values ('".$var_shausername."','".$var_logindate."','".$var_nconfigs."','".$var_nsso."','".$var_nenrolled."','".$var_computername."')";
		mysql_query($szRequest,$cnx);
	}
	dbClose($cnx);
}
?>
