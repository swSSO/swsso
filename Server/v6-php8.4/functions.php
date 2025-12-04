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
// VERSION INTERNE : 6.7.1 (PHP7) -- ISSUE#391
// VERSION INTERNE : 6.7.2 -- ISSUE#396
// VERSION INTERNE : 6.8.0 -- ISSUE#415 (PHP8.4)
//------------------------------------------------------------------------------

// ------------------------------------------------------------
// getconfig [pas de code retour]
// ------------------------------------------------------------
function getconfig($var_title,$var_url,$var_ids,$var_new,$var_mod,$var_old,$var_modifiedSince,$var_type,$var_version,$var_domainId,$var_autoPublish)
{
	$cnx=sqliConnect();	if (!$cnx) return;

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
		$columns=$columns.",withIdPwd,".$param_id1Value.",".$param_id2Value.",".$param_id3Value.",".$param_id4Value.",".$param_pwdValue;
	}
				
	$conditions="";
	
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
		// var_autoPublish=1 -> retourne les configurations qui ne sont pas dans la liste fournie 
		//              et qui ont le flag autoPublish et active=1 (indépendamment de la date)
		// REMARQUE : si var_new=1 et liste vide et lastModified=01/01/2000, retourne toutes les configs actives
		// REMARQUE : les 3 options sont cumulables !
		
		// Construction de la clause IN ou NOT IN et active=0/1
		if ($var_ids!="") // une liste de config id a été passée en paramètre
		{
			if (($var_mod=="1" || $var_old=="1") && $var_new=="0") 
				$conditions="AND "._TABLE_PREFIX_."config.id IN (".$var_ids.") ";
			else if ($var_new=="1" && (var_mod=="0" && $var_old=="0"))
				$conditions="AND "._TABLE_PREFIX_."config.id NOT IN (".$var_ids.") ";
			else if ($var_autoPublish=="1")
				$conditions="AND "._TABLE_PREFIX_."config.id NOT IN (".$var_ids.") ";
		}
		if (($var_new=="1" || $var_mod=="1") && $var_old=="0")
			$conditions=$conditions."AND active=1 ";
		else if ($var_old=="1" && ($var_new=="0" || $var_mod=="0"))
			$conditions=$conditions." AND active=0 ";
		else if ($var_autoPublish=="1")
			$conditions=$conditions." AND active=1 AND configs_domains.domainAutoPublish=1 ";
		
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
	$req=mysqli_query($cnx,$szRequest);
	if (!$req) { sqliError($cnx,$szRequest); dbClose($cnx); return; }

	// trace tous les appels et le résultat ne doit pas être bloquant en cas d'erreur
	if (_LOGS_=="TRUE")
	{
		$szLogRequest="insert into "._TABLE_PREFIX_."logs (title,url,result,domainId) values ('".$var_title."','".$var_url."',".mysqli_num_rows($req).",'".$var_domainId."')";
		mysqli_query($cnx,$szLogRequest);
	}
	 
	header("Content-type: text/xml; charset=UTF-8");
	if(mysqli_num_rows($req)==0) 
	{
		echo "<app>NOT FOUND</app>";
	}
	else
	{
		echo "<apps>\n";
		$i=0;
		while($ligne=mysqli_fetch_row($req))
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
			if (isset($ligne[24])) echo "<withIdPwd><![CDATA[".$ligne[24]."]]></withIdPwd>\n";
			if (isset($ligne[25])) echo "<id1Value><![CDATA[".$ligne[25]."]]></id1Value>\n";
			if (isset($ligne[26])) echo "<id2Value><![CDATA[".$ligne[26]."]]></id2Value>\n";
			if (isset($ligne[27])) echo "<id3Value><![CDATA[".$ligne[27]."]]></id3Value>\n";
			if (isset($ligne[28])) echo "<id4Value><![CDATA[".$ligne[28]."]]></id4Value>\n";
			if (isset($ligne[29])) echo "<pwdValue><![CDATA[".$ligne[29]."]]></pwdValue>\n";
			echo "</app>\n";
			$i++;
		}
		echo "</apps>";
	}
	sqliClose($cnx);
}

// ------------------------------------------------------------
// showAll -> appelé par showall et showold  [pas de code retour]
// ------------------------------------------------------------
function showAll($active,$domain,$title,$export)
{
	$cnx=sqliConnect();	if (!$cnx) return;

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
		$param_id1Value="id1Value";
		$param_id2Value="id2Value";
		$param_id3Value="id3Value";
		$param_id4Value="id4Value";
		$param_pwdValue="pwdValue";
	}

	$szWhere="";
	if ($domain!=0)
	{
		$szWhere=$szWhere." AND config.id=configs_domains.configId AND configs_domains.domainId=".$domain;
		$szDomainField="configs_domains.domainId";
		$szDomainTable=","._TABLE_PREFIX_."configs_domains";
	}
	else
	{
		$szDomainField="-1";
		$szDomainTable="";
	}
	if (_ENCRYPT_=="TRUE")
	{
		$szRequest="select config.id,categId,categ.label,".$szDomainField.",".$param_szName.",".$param_title.",".$param_url.",typeapp,bKBSim,id1Name,pwdName,".
				   "validateName,szKBSim,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,".$param_szFullPathName.
				   ",lastModified,pwdGroup,autoLock,withIdPwd,".$param_id1Value.",".$param_id2Value.",".$param_id3Value.
				   ",".$param_id4Value.",".$param_pwdValue.
				   " from "._TABLE_PREFIX_."config,"._TABLE_PREFIX_."categ".$szDomainTable." where active=".$active." AND categId=categ.id ".
				   $szWhere." group by id order by id";
	}	
	else
	{
		$szRequest="select config.id,categId,categ.label,".$szDomainField.",".$param_szName.",".$param_title.",".$param_url.",typeapp,bKBSim,id1Name,pwdName,".
				   "validateName,szKBSim,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,".$param_szFullPathName.
				   ",lastModified,pwdGroup,autoLock".
				   " from "._TABLE_PREFIX_."config,"._TABLE_PREFIX_."categ".$szDomainTable." where active=".$active." AND categId=categ.id ".
				   $szWhere." group by id order by id";
	}
	$req=mysqli_query($cnx,$szRequest);
	if (!$req) { sqliError($cnx,$szRequest); dbClose($cnx); return; }
	if ($export==0)
	{
		header("Content-type: text/html; charset=UTF-8");
		echo "<html>";
		if ($active==1)
			echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > <a href=./admin.php?action=menu"._MENUSUFFIX_."&domain=".$domain.">Gestion du domaine ".getDomainLabel($cnx,$domain)."</a> > Liste des configurations actives (<a href=./export.php?data=configs&domain=".$domain.">Exporter</a>)</b><br/><br/>";
		else
			echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > <a href=./admin.php?action=menu"._MENUSUFFIX_."&domain=".$domain.">Gestion du domaine ".getDomainLabel($cnx,$domain)."</a> > Liste des configurations archiv&eacute;s (<a href=./export.php?data=configs&domain=".$domain.">Exporter</a>)</b><br/><br/>";
		echo $title;
		if ($active==1)
			echo "<table border=1 style=\"font-family:Verdana; font-size:11px;\">";
		else
			echo "<table border=1 style=\"font-family:Verdana; font-size:11px;\">";
		echo "<tr>";
		echo "<th>Action</th>";
		echo "<th>Id.</th>";
		echo "<th>Categ.</th>";
		echo "<th>Domaine</th>";
		echo "<th>Nom</th>";
		echo "<th>Titre</th>";
		echo "<th>URL</th>";
		echo "<th>Type</th>";
		echo "<th>KBSim?</th>";
		echo "<th>Champ identifiant</th>";
		echo "<th>Champ mot de passe</th>";
		echo "<th>Bouton ou formulaire</th>";
		echo "<th>Saisie clavier</th>";
		echo "<th>2nd id.</th>";
		echo "<th>3eme id.</th>";
		echo "<th>4eme id.</th>";
		echo "<th>szFullPathName</th>";
		echo "<th>lastModified</th>";
		echo "<th>pwdGroup</th>";
		echo "<th>autoLock</th>";
		if (_ENCRYPT_=="TRUE")
		{
			echo "<th>withIdPwd</th>";
			echo "<th>id1Value</th>";
			echo "<th>id2Value</th>";
			echo "<th>id3Value</th>";
			echo "<th>id4Value</th>";
			echo "<th>pwdValue</th>";
		}
		echo "</tr>";
	}
	else // $export=1
	{
		$csv=fopen('php://output','w');
		header('Content-Type: application/csv');
		if ($active==1)
			header('Content-Disposition: attachement; filename="swsso-config-actives.csv";');
		else
			header('Content-Disposition: attachement; filename="swsso-config-archivees.csv";');
		$header="Id."._SEPARATOR_."CategId."._SEPARATOR_."CategName."._SEPARATOR_."Domaine"._SEPARATOR_."Nom"._SEPARATOR_."Titre"._SEPARATOR_."URL"._SEPARATOR_."Type"._SEPARATOR_."KBSim?"._SEPARATOR_.
				"Champ id."._SEPARATOR_."Champ mdp"._SEPARATOR_."Validation"._SEPARATOR_."Saisie clavier"._SEPARATOR_."Nom Id2"._SEPARATOR_."Type Id2"._SEPARATOR_."Nom Id3"._SEPARATOR_."Type Id3"._SEPARATOR_.
				"Nom Id4"._SEPARATOR_."Type Id4"._SEPARATOR_."FullPathName"._SEPARATOR_."LastModified"._SEPARATOR_."PwdGroup"._SEPARATOR_."AutoLock";
		if (_ENCRYPT_=="TRUE")
		{		
			$header=$header._SEPARATOR_."withIdPwd"._SEPARATOR_."id1Value"._SEPARATOR_."id2Value"._SEPARATOR_."id3Value"._SEPARATOR_."id4Value"._SEPARATOR_."pwdValue";
		}
		$header=$header."\n";
		fputs($csv,$header);
	}
	$domainLabel=getDomainLabel($cnx,$domain);
	for ($i=0;$i<mysqli_num_rows($req);$i++)
	{
		$ligne = mysqli_fetch_row($req);
		if ($export==0)
		{
			echo "<tr>";
			if ($active==1)
				echo "<td><a href=\"./admin.php?action=archiveconfig"._WRITESUFFIX_."&id=".$ligne[0]."&active=".$active."\" ".
					" onclick=\"return confirm('Confirmez-vous l\'archivage de ".$ligne[4]." [".$ligne[0]."] ?');\">Archiver</a></br/>".
					"<a href=\"./admin.php?action=deleteconfig"._WRITESUFFIX_."&id=".$ligne[0]."&active=".$active."\" ".
					" onclick=\"return confirm('Confirmez-vous la SUPPRESSION de ".$ligne[4]." [".$ligne[0]."] ?');\">Supprimer</a></td>";
			else
			{
				echo "<td><a href=\"./admin.php?action=restoreconfig"._WRITESUFFIX_."&id=".$ligne[0]."&active=".$active."\" ".
					" onclick=\"return confirm('Confirmez-vous la restauration de ".$ligne[4]." [".$ligne[0]."] ?');\">Restaurer</a><br/>".
					"<a href=\"./admin.php?action=deleteconfig"._WRITESUFFIX_."&id=".$ligne[0]."&active=".$active."\" ".
					" onclick=\"return confirm('Confirmez-vous la SUPPRESSION de ".$ligne[4]." [".$ligne[0]."] ?');\">Supprimer</a></td>";
			}
			echo "<td>".$ligne[0]."</td>";    
			if ($ligne[1]!="") echo "<td>".$ligne[2]."(".$ligne[1].")"."</td>"; else echo "<td align=center>-</td>";   // categId
			if ($ligne[3]!="") // domainId
			{
				if ($ligne[3]!="-1")
				{
					echo "<td>".$domainLabel."(".$domain.")</td>";
				}
				else
				{
					$szRequestDomains="select label,domainId from domains,configs_domains where domains.id=configs_domains.domainId and configs_domains.configId=".$ligne[0]." order by domainId";
					$reqDomains=mysqli_query($cnx,$szRequestDomains);
					if (!$reqDomains) { sqliError($cnx,$szRequestDomains); dbClose($cnx); return; }
					$szDomainsList="";
					for ($j=0;$j<mysqli_num_rows($reqDomains);$j++)
					{
						if ($j!=0) $szDomainsList = $szDomainsList."<br/>";
						$ligneDomain=mysqli_fetch_row($reqDomains);
						$szDomainsList = $szDomainsList.$ligneDomain[0]."(".$ligneDomain[1].")";
					}
					if ($szDomainsList=="") $szDomainsList="-";
					echo "<td>".$szDomainsList."</td>";  
				}
			}
			if ($ligne[4]!="") echo "<td>".$ligne[4]."</td>"; else echo "<td align=center>-</td>";   // szName
			echo "<td>".$ligne[5]."</td>";                                                           // title
			if ($ligne[6]!="") echo "<td>".$ligne[6]."</td>"; else echo "<td align=center>-</td>";   // URL
			echo "<td>".$ligne[7]."</td>";                                                           // typeapp
			echo "<td>".$ligne[8]."</td>";                                                           // bKBSim
			if ($ligne[9]!="") echo "<td>".$ligne[9]."</td>"; else echo "<td align=center>-</td>";   // id1Name
			if ($ligne[10]!="") echo "<td>".$ligne[10]."</td>"; else echo "<td align=center>-</td>";   // pwdName
			if ($ligne[11]!="") echo "<td>".$ligne[11]."</td>"; else echo "<td align=center>-</td>";   // validateName
			if ($ligne[12]!="") echo "<td>".$ligne[12]."</td>"; else echo "<td align=center>-</td>";   // szKBSim
			if ($ligne[13]!="") echo "<td>".$ligne[13]."(".$ligne[14].")</td>"; else echo "<td align=center>-</td>";    // id2Name+id2Type
			if ($ligne[15]!="") echo "<td>".$ligne[15]."(".$ligne[16].")</td>"; else echo "<td align=center>-</td>";   // id3Name+id3Type
			if ($ligne[17]!="") echo "<td>".$ligne[17]."(".$ligne[18].")</td>"; else echo "<td align=center>-</td>"; // id4Name+id4Type
			if ($ligne[19]!="") echo "<td>".$ligne[19]."</td>"; else echo "<td align=center>-</td>";   // szFullPathName
			if ($ligne[20]!="") echo "<td>".$ligne[20]."</td>"; else echo "<td align=center>-</td>";   // lastModified
			if ($ligne[21]!="") echo "<td>".$ligne[21]."</td>"; else echo "<td align=center>-</td>";   // pwdGroup
			if ($ligne[22]!="") echo "<td>".$ligne[22]."</td>"; else echo "<td align=center>-</td>";   // autoLock
			if (_ENCRYPT_=="TRUE")
			{
				if ($ligne[23]!="") echo "<td>".$ligne[23]."</td>"; else echo "<td align=center>-</td>";   // withIdPwd
				if ($ligne[24]!="") echo "<td>".$ligne[24]."</td>"; else echo "<td align=center>-</td>";   // id1Value
				if ($ligne[25]!="") echo "<td>".$ligne[25]."</td>"; else echo "<td align=center>-</td>";   // id2Value
				if ($ligne[26]!="") echo "<td>".$ligne[26]."</td>"; else echo "<td align=center>-</td>";   // id3Value
				if ($ligne[27]!="") echo "<td>".$ligne[27]."</td>"; else echo "<td align=center>-</td>";   // id4Value
				if ($ligne[28]!="") echo "<td>".$ligne[28]."</td>"; else echo "<td align=center>-</td>";   // pwdValue
			}
			echo "</tr>";
		}
		else // $export!=0
		{
			if ($ligne[3]!="") // domainId
			{
				if ($ligne[3]=="-1")
				{
					$szRequestDomains="select label,domainId from domains,configs_domains where domains.id=configs_domains.domainId and configs_domains.configId=".$ligne[0]." order by domainId";
					$reqDomains=mysqli_query($cnx,$szRequestDomains);
					if (!$reqDomains) { sqliError($cnx,$szRequestDomains); dbClose($cnx); return; }
					$szDomainsList="";
					for ($j=0;$j<mysqli_num_rows($reqDomains);$j++)
					{
						if ($j!=0) $szDomainsList = $szDomainsList."+";
						$ligneDomain=mysqli_fetch_row($reqDomains);
						$szDomainsList = $szDomainsList.$ligneDomain[0]."(".$ligneDomain[1].")";
					}
					if ($szDomainsList=="") $szDomainsList="-";
					$ligne[3]=$szDomainsList;
				}
				else
				{
					$ligne[3]=$domainLabel."(".$domain.")";
				}
			}
			fputcsv($csv,$ligne,_SEPARATOR_);
		}
	}
	if ($export==0)
	{
		echo "</table>";
		echo "<font face=verdana size=2>";
		if ($active==1)
			echo "<br/>Pour archiver une configuration, cliquez sur son identifiant puis validez la confirmation qui vous sera demand&eacute;e.<br/>";
		else
			echo "<br/>Pour supprimer d&eacute;finitivement une configuration, cliquez sur son identifiant puis validez la confirmation qui vous sera demand&eacute;e.<br/>";
		echo "</font>";
		echo "</html>";
	}
	else // $export!=0
	{
		fclose($csv);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// getDomainLabel [retour : libellé du domaine]
// ------------------------------------------------------------
function getDomainLabel($cnx,$domainId)
{
	if ($domainId==0)
		return "Tous les domaines";
	else
	{
		$bClose=false;
		if ($cnx==null) 
		{ 
			$cnx=sqliConnect();	if (!$cnx) return;
			$bClose=true;
		}
		$szRequest="select label from "._TABLE_PREFIX_."domains where id=?";      
		$stmt=mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_bind_param($stmt,"i",$domainId);
			mysqli_stmt_execute($stmt);
			mysqli_stmt_bind_result($stmt,$domainLabel);
			$fetch=mysqli_stmt_fetch($stmt);
			mysqli_stmt_close($stmt);
		}
		if ($bClose) sqliClose($cnx);
		return $domainLabel;
	}
}
// ------------------------------------------------------------
// showCategories [pas de code retour]
// ------------------------------------------------------------
function showCategories($title)
{
	$cnx=sqliConnect();	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;

	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Liste des cat&eacute;gories</b><br/><br/>";
	
	$szRequest="select id,label from "._TABLE_PREFIX_."categ order by id";  
	$stmt=mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$categId,$categLabel);
		echo "<table border=1 style=\"font-family:Verdana; font-size:11px;\">";
		echo "<tr>";
		echo "<th>Identifiant</th>";
		echo "<th>Libell&eacute; de la cat&eacute;gorie</th>";
		echo "</tr>";
		while (mysqli_stmt_fetch($stmt))
		{
			echo "<tr>";
			if ($categId=='0')
				echo "<td>".$categId."</td>";
			else
				echo "<td><a href=\"./admin.php?action=deletecateg"._WRITESUFFIX_."&id=".$categId."\" ".
					" onclick=\"return confirm('Confirmez-vous la suppression de la cat&eacute;gorie ".$categLabel." [".$categId."] ?');\">".$categId."</a></td>";
			if ($categLabel!="") echo "<td>".$categLabel."</td>"; else echo "<td align=center>-</td>"; // label
			echo "</tr>";
		}
		echo "</table>";
		echo "<font face=verdana size=2>";
		echo "<br/>Pour supprimer une cat&eacute;gorie, cliquez sur son identifiant puis validez la confirmation qui vous sera demand&eacute;e.<br/>";
		echo "<br/><b>ATTENTION : veuillez v&eacute;rifier qu'aucune application n'est rattach&eacute;e &agrave une cat&eacute;gorie avant de la supprimer.</b><br/>";
		echo "</font>";
		echo "</html>";
		mysqli_stmt_close($stmt);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// showDomains [pas de code retour]
// ------------------------------------------------------------
function showDomains($title)
{
	$cnx=sqliConnect();	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;

	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > Gestion des domaines</b><br/><br/>";

	if (isset($_GET["new"]))
	{
		$var_new=addslashes($_GET['new']);
		$szRequest="insert into "._TABLE_PREFIX_."domains (label) values (?)";

		$stmt = mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_bind_param($stmt,"s",$var_new);
			mysqli_stmt_execute($stmt);
			mysqli_stmt_close($stmt);
		}
	}
	
	$szRequest="select id,label from "._TABLE_PREFIX_."domains order by id";      
	$stmt=mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$domainId,$domainLabel);
		echo "<table border=1 style=\"font-family:Verdana; font-size:11px;\">";
		echo "<tr>";
		echo "<th>Identifiant</th>";
		echo "<th>Libell&eacute; du domaine</th>";
		echo "</tr>";
		while (mysqli_stmt_fetch($stmt))
		{
			echo "<tr>";
			if ($domainId=='1')
				echo "<td>".$domainId."</td>";
			else
				echo "<td><a href=\"./admin.php?action=deletedomain"._WRITESUFFIX_."&id=".$domainId."\" ".
					" onclick=\"return confirm('Confirmez-vous la suppression du domaine ".$domainLabel." [".$domainId."] ?');\">".$domainId."</a></td>";
			if ($domainLabel!="") echo "<td>".$domainLabel."</td>"; else echo "<td align=center>-</td>"; // label
			echo "</tr>";
		}
		echo "</table>";
		echo "<font face=verdana size=2>";
		echo "<br/>Pour supprimer un domaine, cliquez sur son identifiant puis validez la confirmation qui vous sera demand&eacute;e.<br/>";
		echo "<br/>Ajouter un domaine : ";
		echo "<form method=\"GET\" action=\"./admin.php\">";
		echo "<input name=\"action\" type=\"hidden\" value=\"showdomains"._MENUSUFFIX_."\"/>";
		echo "<input name=\"new\" type=\"text\"/>";
		echo "<input name=\"valider\" type=\"submit\" value=\"OK\"/>";
		echo "</form>";
		echo "</font>";
		echo "</html>";
		mysqli_stmt_close($stmt);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// menuShowDomains [pas de code retour]
// ------------------------------------------------------------
function menuShowDomains()
{
	$cnx=sqliConnect();	if (!$cnx) return;

	$szRequest="select id,label from "._TABLE_PREFIX_."domains order by label";      
	$stmt=mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$domainId,$domainLabel);
		while (mysqli_stmt_fetch($stmt))
		{
			echo "<br>&nbsp;&nbsp;&nbsp;-&nbsp;";
			echo "<a href=\"./admin.php?action=menu"._MENUSUFFIX_."&domain=".$domainId."\">".$domainLabel."</a>";
		}
		mysqli_stmt_close($stmt);
	}
	echo "<br>&nbsp;&nbsp;&nbsp;-&nbsp;";
	echo "<a href=\"./admin.php?action=menu"._MENUSUFFIX_."&domain=0\">Tous les domaines</a>";
	
	sqliClose($cnx);
}
// ------------------------------------------------------------
// exportStats [pas de code retour]
// ------------------------------------------------------------
function exportStats()
{
	$cnx=sqliConnect();	if (!$cnx) return;

	$szRequest="select shausername,logindate,nconfigs,nsso,nenrolled,computername from "._TABLE_PREFIX_."stats order by logindate";      
	$stmt=mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		$csv=fopen('php://output','w');
		header('Content-Type: application/csv');
		header('Content-Disposition: attachement; filename="swsso-statistiques.csv";');
		mysqli_stmt_execute($stmt);
		mysqli_stmt_bind_result($stmt,$shausername,$logindate,$nconfigs,$nsso,$nenrolled,$computername);
		while (mysqli_stmt_fetch($stmt))
		{
			fprintf($csv,"%s%c%s%c%d%c%d%c%d%c%s",$shausername,_SEPARATOR_,$logindate,_SEPARATOR_,$nconfigs,_SEPARATOR_,$nsso,_SEPARATOR_,$nenrolled,_SEPARATOR_,$computername);
		}
		mysqli_stmt_close($stmt);
		fclose($csv);
	}
	sqliClose($cnx);
}
// ------------------------------------------------------------
// deleteConfig [code retour : TRUE si OK, FALSE sinon] -- ISSUE#391
// ------------------------------------------------------------
function deleteConfig($configId)
{
	$rc=FALSE;
	
	$cnx=sqliConnect();	if (!$cnx) return $rc;
		
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
		if (mysqli_stmt_affected_rows($stmt)>0) $rc=TRUE;
		mysqli_stmt_close($stmt);	
	}
	sqliClose($cnx);
	return $rc; //ISSUE#391
}
// ------------------------------------------------------------
// deleteCateg [code retour : TRUE si OK, FALSE sinon] -- ISSUE#391
// ------------------------------------------------------------
function deleteCateg($categId)
{
	$rc=FALSE;
	
	$cnx=sqliConnect();	if (!$cnx) return $rc;
	
	$szRequest="delete from "._TABLE_PREFIX_."categ where id=?";
	$stmt = mysqli_stmt_init($cnx);
	if (mysqli_stmt_prepare($stmt,$szRequest))
	{
		mysqli_stmt_bind_param($stmt,"i",$categId);
		mysqli_stmt_execute($stmt);
		if (mysqli_stmt_affected_rows($stmt)>0) $rc=TRUE;
		mysqli_stmt_close($stmt);	
	}
	sqliClose($cnx);
	return $rc; //ISSUE#391
}
// ------------------------------------------------------------
// deleteDomain [code retour : TRUE si OK, FALSE sinon] -- ISSUE#391
// ------------------------------------------------------------
function deleteDomain($domainId)
{
	$rc=FALSE;
	
	$cnx=sqliConnect();	if (!$cnx) return $rc;
	
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
	if ($nbDomains==0) 
	{
		$szRequest="delete from "._TABLE_PREFIX_."domains where id=?";
		$stmt = mysqli_stmt_init($cnx);
		if (mysqli_stmt_prepare($stmt,$szRequest))
		{
			mysqli_stmt_bind_param($stmt,"i",$domainId);
			mysqli_stmt_execute($stmt);
			if (mysqli_stmt_affected_rows($stmt)>0) $rc=TRUE;
			mysqli_stmt_close($stmt);	
		}
	}
	sqliClose($cnx);
	return $rc; //ISSUE#391
}
// ------------------------------------------------------------
// showLogs [pas de code retour]
// ------------------------------------------------------------
function showLogs($domain,$result,$title)
{
	$cnx=sqliConnect();	if (!$cnx) return;

	header("Content-type: text/html; charset=UTF-8");
	echo "<html>";
	echo $title;
	echo "<font face=verdana size=2><b><a href=./admin.php?action=menu"._MENUSUFFIX_.">Menu principal</a> > <a href=./admin.php?action=menu"._MENUSUFFIX_."&domain=".$domain.">Gestion du domaine ".getDomainLabel($cnx,$domain)."</a> > Logs</b><br/><br/>";

	$szWhere="";
	if ($domain!=0)
	{
		$szWhere="WHERE domainId=".$domain;
	}
	$szRequest="select horodate,title,url,result,domainId from "._TABLE_PREFIX_."logs ".$szWhere." order by horodate desc";      

	$req=mysqli_query($cnx,$szRequest);
	if (!$req) { sqliError($cnx,$szRequest); dbClose($cnx); return; }
	echo "<table border=1 style=\"font-family:Verdana; font-size:11px;\">";
	echo "<tr>";
	echo "<th>Date</th>";
	echo "<th>Domaine</th>";
	echo "<th>Titre</th>";
	echo "<th>URL</th>";
	echo "<th>R&eacute;sultat</th>";
	echo "</tr>";
	for ($i=0;$i<mysqli_num_rows($req);$i++)
	{
		$ligne = mysqli_fetch_row($req);
		echo "<tr>";
		echo "<td>".$ligne[0]."</td>";
		if ($ligne[4]!="") echo "<td>".$ligne[4]."</td>"; else echo "<td align=center>-</td>";
		if ($ligne[1]!="") echo "<td>".$ligne[1]."</td>"; else echo "<td align=center>-</td>";
		if ($ligne[2]!="") echo "<td>".$ligne[2]."</td>"; else echo "<td align=center>-</td>";
		if ($ligne[3]!="") echo "<td>".$ligne[3]."</td>"; else echo "<td align=center>-</td>";
		echo "</tr>";
	}
	echo "</table>";
	echo "</html>";
	sqliClose($cnx);
}
?>
