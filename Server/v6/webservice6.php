<?php
include('variables.php');
include('util.php');
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
// webservice6.PHP : Utilisé à partir de la version swSSO 1.05 pour la gestion des domaines multiples.
//                   Le client 1.05 et les suivants resteront compatibles avec webservice5.php tant
//                   qu'ils n'auront pas besoin de la gestion des domaines multiples
// VERSION INTERNE : 6.0
//------------------------------------------------------------------------------
// Commandes : isalive, getversion, putconfig, getconfig et getdomains
//------------------------------------------------------------------------------

$swssoVersion="000:0000"; // "000:0000" désactive le contrôle de version côté client

// ------------------------------------------------------------
// isalive
// ------------------------------------------------------------
if ($_GET['action']=="isalive")
{
    $cnx=dbConnect();
  	if ($cnx) echo "ALIVE";
	dbClose($cnx);
}
// ------------------------------------------------------------
// getconfig
// ------------------------------------------------------------
else if ($_GET['action']=="getconfig")
{
	$cnx=dbConnect();
	if (!$cnx) return;

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
	
	if (_ENCRYPT_=="TRUE")
	{
		$param_url="AES_DECRYPT(url,'"._AESPWD_."')";
		$param_title="AES_DECRYPT(title,'"._AESPWD_."')";
		$param_szFullPathName="AES_DECRYPT(szFullPathName,'"._AESPWD_."')";
		$param_szName="AES_DECRYPT(szName,'"._AESPWD_."')";
		$param_id1Value="AES_DECRYPT(id1Value,'"._AESPWD_."')";
		$param_id2Value="AES_DECRYPT(id2Value,'"._AESPWD_."')";
		$param_id3Value="AES_DECRYPT(id3Value,'"._AESPWD_."')";
		$param_id4Value="AES_DECRYPT(id4Value,'"._AESPWD_."')";
		$param_pwdValue="AES_DECRYPT(pwdValue,'"._AESPWD_."')";
	}
	else
	{
		$param_url="url";
		$param_title="title";
		$param_szFullPathName="szFullPathName";
		$param_szName="szName";
	}

	// la liste des champs à retourner dans la structure XML est comune à TOUTES les requêtes mais dépend de chiffré ou non
	$columns="typeapp,".$param_title.",".$param_url.",id1Name,pwdName,id2Name,id2Type,id3Name,".
			"id3Type,id4Name,id4Type,validateName,bKBSim,szKBSim,".
			$param_szName.",".$param_szFullPathName.",categId,"._TABLE_PREFIX_."categ.label,".
			_TABLE_PREFIX_."config.id,lastModified,active,"._TABLE_PREFIX_."configs_domains.domainId,pwdGroup,autoLock";
			
	if (_ENCRYPT_=="TRUE")
	{
		$columns=$columns.",".$param_id1Value.",".$param_id2Value.",".$param_id3Value.",".$param_id4Value.",".$param_pwdValue;
	}
				
	if (isset($_GET["debug"])) echo $columns;
	if (isset($_GET["debug"])) echo "new=".$var_new." mod=".$var_mod." old=".$var_old;
	
	// si titre vide, c'est une récupération de l'ensemble des configs au démarrage
	if ($var_title=="")
	{
		// var_new=1 -> retourne les configurations qui ne sont pas dans la liste fournie 
		//              et dont la date de modification est supérieure à var_modifiedSince
		//              et active=1
		// var_mod=1 -> retourne les configurations qui sont dans la liste fournie
		//	            et dont la date de modification est supérieure à var_modifiedSince 
		//              et active=1
		// var_old=1 -> retourne les configurations qui sont dans la liste fournie
		//				et dont la date de modification est supérieure à var_modifiedSince 
		//              et active=0
		// REMARQUE : si var_new=1 et liste vide et lastModified=01/01/2000, retourne toutes les configs actives
		// REMARQUE : les 3 options sont cumulables !
		
		// Construction de la clause IN ou NOT IN et active=0/1
		$conditions="";
		if ($var_ids!="") // une liste de config id a été passée en paramètre
		{
			if (($var_mod=="1" || $var_old=="1") && $var_new=="0") 
				$conditions="AND "._TABLE_PREFIX_."config.id IN (".$var_ids.") ";
			else if ($var_new=="1" && (var_mod=="0" && $var_old=="0"))
				$conditions="AND "._TABLE_PREFIX_."config.id NOT IN (".$var_ids.") ";
		}
		if (($var_new=="1" || $var_mod=="1") && $var_old=="0")
			$conditions=$conditions."AND active=1 ";
		else if ($var_old=="1" && ($var_new=="0" || $var_mod=="0"))
			$conditions=$conditions." AND active=0 ";
		
		$conditions=$conditions."AND config.id=configs_domains.configId ";
		// ISSUE#125  : lorsque GetModifiedConfigsAtStart=1 et GetNewConfigsAtStart=0, seules les configurations modifiées devraient être mises à jour)
		// L'affectation de $conditions au lieu d'un append écrasait la condition IN () préalablement construite...
		//if ($var_domainId!=-1) $conditions="and ("._TABLE_PREFIX_."config.domainId=1 or "._TABLE_PREFIX_."config.domainId=".$var_domainId.") ";
		if ($var_domainId!=-1) $conditions=$conditions."and ("._TABLE_PREFIX_."configs_domains.domainId=1 or "._TABLE_PREFIX_."configs_domains.domainId=".$var_domainId.") ";
		$szRequest= "select ".$columns." from "._TABLE_PREFIX_."config,"._TABLE_PREFIX_."categ,"._TABLE_PREFIX_."configs_domains ".
					"where "._TABLE_PREFIX_."categ.id="._TABLE_PREFIX_."config.categId AND lastModified>".$var_modifiedSince." ".
					$conditions." ".
					"group by "._TABLE_PREFIX_."config.id ".
					"order by "._TABLE_PREFIX_."config.id desc, lastModified desc";
	}
	else // titre non vide, c'est une requête suite à un clic droit / ajouter cette application
	{
		// il faut que les configs avec id=0 arrivent en dernier pour que le client swsso sélectionne
		// prioritairement les configs avec id (d'où le orderby desc)
		if ($var_domainId!=-1) $conditions="and ("._TABLE_PREFIX_."configs_domains.domainId=1 or "._TABLE_PREFIX_."configs_domains.domainId=".$var_domainId.") ";
		$conditions=$conditions." AND config.id=configs_domains.configId ";
		if ($var_url=="")
		{
			// URL vide = popup IE ou fenêtre native Windows
			// On ne cherche pas forcément une appli avec URL vide -> donc URL n'est plus un critère depuis 0.85B6
			// si plusieurs config matchent, on les retourne toutes, le tri sera fait en local par CheckURL()
			$szRequest= "select ".$columns." from "._TABLE_PREFIX_."config,"._TABLE_PREFIX_."categ,"._TABLE_PREFIX_."configs_domains ".
						"where active=1 AND "._TABLE_PREFIX_."categ.id="._TABLE_PREFIX_."config.categId AND typeapp<>'WEB' AND ".
						" ((right(".$param_title.",1)='*' and left(".$param_title.",char_length(".$param_title.")-1) = left('".$var_title."',char_length(".$param_title.")-1)) OR ".
						"  (left (".$param_title.",1)='*' and right(".$param_title.",char_length(".$param_title.")-1) = right('".$var_title."',char_length(".$param_title.")-1)) OR ".
						"  (left (".$param_title.",1)='*' and right(".$param_title.",1)='*' and locate(substr(".$param_title.",2,char_length(".$param_title.")-2),'".$var_title."')<>0) OR ". 
						"  (".$param_title."='".$var_title."')) ".
						$conditions.
						"group by "._TABLE_PREFIX_."config.id ".
						"order by "._TABLE_PREFIX_."config.id desc, lastModified desc";
		}
		else
		{
			// URL non vide : popup firefox ou site web sous IE et Firefox
			//
			// Cas 1 : URL sans *
			// - url        =http://toto
			// - (A) var_url=http://toto      -> matche
			// - (B) var_url=http://toto/titi -> ne dois pas matcher
			// 
			// Cas 2 : URL avec *
			// - url    =http://toto*
			// - (A) var_url=http://toto/titi -> matche
			// - (B) var_url=http://toto      -> doit aussi matcher
			if ($var_type=="") 
				$szTypeCondition="";
			else
				$szTypeCondition="AND typeapp='".$var_type."'";
				$szRequest= "select ".$columns." from "._TABLE_PREFIX_."config,"._TABLE_PREFIX_."categ,"._TABLE_PREFIX_."configs_domains ".
						"where active=1 AND "._TABLE_PREFIX_."categ.id="._TABLE_PREFIX_."config.categId AND ".
						" ((right(".$param_title.",1)='*' and left(".$param_title.",char_length(".$param_title.")-1) = left('".$var_title."',char_length(".$param_title.")-1)) OR ".
						"  (left (".$param_title.",1)='*' and right(".$param_title.",char_length(".$param_title.")-1) = right('".$var_title."',char_length(".$param_title.")-1)) OR ".
						"  (left (".$param_title.",1)='*' and right(".$param_title.",1)='*' and locate(substr(".$param_title.",2,char_length(".$param_title.")-2),'".$var_title."')<>0) OR ". 
						"  (".$param_title."='".$var_title."')) AND ".
						"char_length(".$param_url.")>1 AND ".
						" ((right(".$param_url.",1)='*' and left(".$param_url.",char_length(".$param_url.")-1) = left('".$var_url."',char_length(".$param_url.")-1)) OR ".
						"  (left (".$param_url.",1)='*' and right(".$param_url.",char_length(".$param_url.")-1) = right('".$var_url."',char_length(".$param_url.")-1)) OR ".
						"  (left (".$param_url.",1)='*' and right(".$param_url.",1)='*' and locate(substr(".$param_url.",2,char_length(".$param_url.")-2),'".$var_url."')<>0) OR ". 
						"  (".$param_url."='".$var_url."'))".
						" ".$szTypeCondition." ".
						$conditions.
						"group by "._TABLE_PREFIX_."config.id ".
						"order by typeapp desc, "._TABLE_PREFIX_."config.id desc, lastModified desc LIMIT 1";
		}
	}
	if (isset($_GET["debug"])) echo $szRequest;
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }

	// trace tous les appels et le résultat ne doit pas être bloquant en cas d'erreur
	if (_LOGS_=="TRUE")
	{
		$szLogRequest="insert into "._TABLE_PREFIX_."logs (title,url,result,domainId) values ('".$var_title."','".$var_url."',".mysql_num_rows($req).",'".$var_domainId."')";
		mysql_query($szLogRequest,$cnx);
	}
	 
	header("Content-type: text/xml; charset=UTF-8");
	if(mysql_num_rows($req)==0) 
	{
		echo "<app>NOT FOUND</app>";
	}
	else
	{
		echo "<apps>\n";
		$i=0;
		while($ligne=mysql_fetch_row($req))
		{
			echo "<app num=\"".$i."\">\n";
			echo "<configId>$ligne[18]</configId>\n";
			echo "<active>$ligne[20]</active>\n";
			echo "<lastModified>$ligne[19]</lastModified>\n";
			echo "<type>".$ligne[0]."</type>\n";
			echo "<title><![CDATA[".$ligne[1]."]]></title>\n";
			echo "<url><![CDATA[".$ligne[2]."]]></url>\n";
			echo "<id1Name>".$ligne[3]."</id1Name>\n";
			echo "<pwdName>".$ligne[4]."</pwdName>\n";
			echo "<id2Name>".$ligne[5]."</id2Name>\n";
			echo "<id2Type>".$ligne[6]."</id2Type>\n";
			echo "<id3Name>".$ligne[7]."</id3Name>\n";
			echo "<id3Type>".$ligne[8]."</id3Type>\n";
			echo "<id4Name>".$ligne[9]."</id4Name>\n";
			echo "<id4Type>".$ligne[10]."</id4Type>\n";
			echo "<validateName>".$ligne[11]."</validateName>\n";
			echo "<bKBSim>".$ligne[12]."</bKBSim>\n";
			echo "<szKBSim><![CDATA[".$ligne[13]."]]></szKBSim>\n";
			echo "<szName><![CDATA[".$ligne[14]."]]></szName>\n";
			echo "<szFullPathName><![CDATA[".$ligne[15]."]]></szFullPathName>\n";
			echo "<categId><![CDATA[".$ligne[16]."]]></categId>\n";
			echo "<categLabel><![CDATA[".$ligne[17]."]]></categLabel>\n";
			echo "<domainId><![CDATA[".$ligne[21]."]]></domainId>\n";
			echo "<pwdGroup><![CDATA[".$ligne[22]."]]></pwdGroup>\n";
			echo "<autoLock><![CDATA[".$ligne[23]."]]></autoLock>\n";
			if (isset($ligne[24])) echo "<id1Value><![CDATA[".$ligne[24]."]]></id1Value>\n";
			if (isset($ligne[25])) echo "<id2Value><![CDATA[".$ligne[25]."]]></id2Value>\n";
			if (isset($ligne[26])) echo "<id3Value><![CDATA[".$ligne[26]."]]></id3Value>\n";
			if (isset($ligne[27])) echo "<id4Value><![CDATA[".$ligne[27]."]]></id4Value>\n";
			if (isset($ligne[28])) echo "<pwdValue><![CDATA[".$ligne[28]."]]></pwdValue>\n";
			echo "</app>\n";
			$i++;
		}
		echo "</apps>";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// putconfig
// ------------------------------------------------------------
else if ($_GET['action']=="putconfig")
{
	$cnx=dbConnect();
    if (!$cnx) return;

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
	if (isset($_GET["withIdPwd"]))	$var_withIdPwd	=utf8_decode(myaddslashes($_GET['withIdPwd'])); // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id1Value"])) 	$var_id1Value	=utf8_decode(myaddslashes($_GET['id1Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id2Value"])) 	$var_id2Value	=utf8_decode(myaddslashes($_GET['id2Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id3Value"])) 	$var_id3Value	=utf8_decode(myaddslashes($_GET['id3Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["id4Value"])) 	$var_id4Value	=utf8_decode(myaddslashes($_GET['id4Value']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["pwdValue"])) 	$var_pwdValue	=utf8_decode(myaddslashes($_GET['pwdValue']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["pwdGroup"])) 	$var_pwdGroup	=utf8_decode(myaddslashes($_GET['pwdGroup']));  // ajouté en 5.3 pour client 1.03
	if (isset($_GET["autoLock"])) 	$var_autoLock	=utf8_decode(myaddslashes($_GET['autoLock']));  // ajouté en 5.5 pour client 1.04
	
	if ($var_pwdGroup=='') $var_pwdGroup=-1;  // pour compatibilité avec les clients <1.03 qui ne gèrent pas ce paramètre
	if ($var_withIdPwd=='') $var_withIdPwd=0; // pour compatibilité avec les clients <1.03 qui ne gèrent pas ce paramètre
	if ($var_autoLock=='') $var_autoLock=1;   // pour compatibilité avec les clients <1.04 qui ne gèrent pas ce paramètre
    
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
		if ($var_withIdPwd==1)
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
		if ($var_withIdPwd==1) 
		{
			$szRequestionOptions=",withIdPwd=1,".
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
		if ($var_withIdPwd==1) 
		{
			$szRequestOptions1=",withIdPwd,id1Value,id2Value,id3Value,id4Value,pwdValue";
			$szRequestOptions2=",1,".$param_id1Value.",".$param_id2Value.",".$param_id3Value.",".$param_id4Value.",".$param_pwdValue;
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
		$tok = strtok($var_domainId,",");
		$bFirst=1;
		$szRequest="";
		while ($tok !== false) 
		{
			if ($bFirst==0)
				$szRequest=$szRequest.",";
			else
			{
				$szRequest="insert into "._TABLE_PREFIX_."configs_domains (configId,domainId) values ";
				$bFirst=0;
			}
			$szRequest=$szRequest."(".$var_configId.",".$tok.") ";
			$tok = strtok(",");
		}
		if ($szRequest!="")
		{
			$szRequest=$szRequest.";";
			if (isset($_GET["debug"])) echo $szRequest;
			$result=mysql_query($szRequest,$cnx);
			if (!$result) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		}
	}
	dbClose($cnx);
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
	// compteur de stat, ne doit pas être bloquant en cas d'erreur
	if (_STATS_=="TRUE")
	{
		$cnx=dbConnect();
		if (!$cnx) return;
		$szCountRequest="update "._TABLE_PREFIX_."stats set getversion=getversion+1 where id=0";
		mysql_query($szCountRequest,$cnx);
		dbClose($cnx);
	}
	header("Content-type: text/xml; charset=UTF-8");
	echo $swssoVersion;
}
// ------------------------------------------------------------
// getdomains
// ------------------------------------------------------------
else if ($_GET['action']=="getdomains")
{
	$cnx=dbConnect();
	if (!$cnx) return;

	// commence par lire le libellé du domaine commun (pour qu'il soit placé en tête de liste puis lit tous les autres)
	$szRequest= "select label from "._TABLE_PREFIX_."domains where id=1";
	if (isset($_GET["debug"])) echo $szRequest;
	$req=mysql_query($szRequest,$cnx);
	if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }
	header("Content-type: text/xml; charset=UTF-8");
	if (mysql_num_rows($req)==0) 
	{
		echo "<domains>NOT FOUND</domains>";
	}
	else
	{
		echo "<domains>\n";
		$ligne=mysql_fetch_row($req);
		echo "<domain num=\"0\">\n";
		echo "<id><![CDATA[1]]></id>\n";
		echo "<label><![CDATA[".$ligne[0]."]]></label>\n";
		echo "</domain>\n";
		$szRequest= "select id,label from "._TABLE_PREFIX_."domains where id<>1 order by label";
		if (isset($_GET["debug"])) echo $szRequest;
		$req=mysql_query($szRequest,$cnx);
		if (!$req) { dbError($cnx,$szRequest); dbClose($cnx); return; }
		
		if(mysql_num_rows($req)!=0) 
		{
			$i=1;
			while($ligne=mysql_fetch_row($req))
			{
				echo "<domain num=\"".$i."\">\n";
				echo "<id><![CDATA[".$ligne[0]."]]></id>\n";
				echo "<label><![CDATA[".$ligne[1]."]]></label>\n";
				echo "</domain>\n";
				$i++;
			}
		}
		echo "</domains>";
	}
	dbClose($cnx);
}
// ------------------------------------------------------------
// getconfigdomains
// ------------------------------------------------------------
else if ($_GET['action']=="getconfigdomains")
{
	$cnx=dbConnect();
	if (!$cnx) return;
	
	// récupération des paramètres passés dans l'URL
    $var_configId=utf8_decode(myaddslashes($_GET['configId']));
	
	$szRequest= "select configs_domains.domainId,domains.label ".
			"from "._TABLE_PREFIX_."configs_domains,"._TABLE_PREFIX_."domains where ".
			"configId=".$var_configId." and configs_domains.domainId=domains.id order by configs_domains.domainId";
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
			echo "</domain>\n";
			$i++;
		}
		echo "</domains>";
	}
	dbClose($cnx);
}?>
