<?php
include('variables.php');

//***********************************************************************//
// WEBSERVICE.PHP : Utilisé jusqu'à la version 0.85 BETA 5
//                  Remplacé par WEBSERVICE2.PHP à partir de 0.85 BETA 6
//***********************************************************************//
$swssoVersion="095:0962";

/* webservice.php?action=getversion&rnd=<alea anti-cache>
   -> 079
         
   webservice.php?action=getconfig&title=<title>&url=<url>
   -> <app>
         <type></type>
         <title></title>
         <url></url>
         <id1Name></id1Name>
         <id1Type></id1Type>
         <id2Name></id2Name>
         <id2Type></id2Type>
         <id3Name></id3Name>
         <id3Type></id3Type>
         <id4Name></id4Name>
         <id4Type></id4Type>
         <id5Name></id5Name>
         <id5Type></id5Type>
         <pwdName></pwdName>
         <validateName></validateName>
      </app>
            
   webservice.php?action=putconfig&title=<title>&url=<url>&typeapp=<typeapp>&id1Name=<id1Name>&pwdName=<pwdName>&validateName=<validateName>&id5Name....
   
   webservice.php?action=showall
   webservice.php?action=showold
*/

function dbConnect() 
{
   $cnx = mysql_connect(_HOST_,_USER_,_PWD_);
   if (!$cnx) 
   {
      echo "<error>Connexion impossible au serveur SQL</error>";
   }
   else 
   {
     $ok = mysql_select_db(_DB_, $cnx);
     if (!$ok)
     {
       echo "<error>Connexion impossible a la base</error>";
     }
     else
       return $cnx;
   }
}

// ------------------------------------------------------------
// getconfig
// ------------------------------------------------------------
if ($_GET['action']=="getconfig")
{
      $cnx=dbConnect();
      header("Content-type: text/xml; charset=UTF-8");

      $var_title=utf8_decode(addslashes($_GET['title']));
      $var_url	=utf8_decode(addslashes($_GET['url']));
      if ($var_url=="")
      {
         // URL vide = popup IE ou fenêtre native Windows
         // On ne cherche pas forcément une appli avec URL vide -> donc URL n'est plus un critère depuis 0.85B6
         // si plusieurs config matchent, on les retourne toutes, le tri sera fait en local par CheckURL()
         
         $szRequest="select typeapp,title,url,id1Name,pwdName,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,id5Name,id5Type,validateName ".
                    "from config where active=1 AND ".
                    "left(title,char_length(title)-1) = left('".$var_title."',char_length(title)-1)";
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
      
         $szRequest="select typeapp,title,url,id1Name,pwdName,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,id5Name,id5Type,validateName ".
                    "from config where active=1 AND ".
                    "left(title,char_length(title)-1) = left('".$var_title."',char_length(title)-1) AND ".
                    "char_length(url)>1 AND ".
                    "( (right(url,1)='*' and left(url,char_length(url)-1) = left('".$var_url."',char_length(url)-1)) OR ".
                    "  (url='".$var_url."')) ".
                    "LIMIT 1";
      }
      $req=mysql_query($szRequest,$cnx);
      if (!$req) {echo "<error>Message de MySql :". mysql_errno($cnx) . mysql_error($cnx)."</error>";return;}
      if(mysql_num_rows($req)==0) 
      {
         echo "<app>NOT FOUND</app>";
      }
      else
      {
         echo "<apps>";
         $i=0;
         while($ligne=mysql_fetch_row($req))
         {
            echo "<app id=\"".$i."\">";
            echo "    <type>".$ligne[0]."</type>";
            echo "    <title>".stripslashes($ligne[1])."</title>";
            echo "    <url>".$ligne[2]."</url>";
            echo "    <id1Name>".$ligne[3]."</id1Name>";
            echo "    <pwdName>".$ligne[4]."</pwdName>";
            echo "    <id2Name>".$ligne[5]."</id2Name>";
            echo "    <id2Type>".$ligne[6]."</id2Type>";
            echo "    <id3Name>".$ligne[7]."</id3Name>";
            echo "    <id3Type>".$ligne[8]."</id3Type>";
            echo "    <id4Name>".$ligne[9]."</id4Name>";
            echo "    <id4Type>".$ligne[10]."</id4Type>";
            echo "    <id5Name>".$ligne[11]."</id5Name>";
            echo "    <id5Type>".$ligne[12]."</id5Type>";
            echo "    <validateName>".$ligne[13]."</validateName>";
            echo "</app>";
            $i++;
         }
         echo "</apps>";
      }
      mysql_close($cnx);
}
// ------------------------------------------------------------
// putconfig
// ------------------------------------------------------------
else if ($_GET['action'] =="putconfig")
{
	$cnx=dbConnect();
    header("Content-type: text/xml; charset=UTF-8");
    //http://www.swsso.fr/server/webservice.php?action=putconfig&title=titrefenetre&url=url&typeapp=WIN&id1Name=champidentifiant&id1Type=EDIT&pwdName=champmotdepasse&validateName=boutonvalidation
    $var_typeapp		=utf8_decode(addslashes($_GET['typeapp']));
    $var_title			=utf8_decode(addslashes($_GET['title']));
    $var_url			=utf8_decode(addslashes($_GET['url']));
    $var_id1Name		=utf8_decode(addslashes($_GET['id1Name']));
    $var_pwdName		=utf8_decode(addslashes($_GET['pwdName']));
    $var_validateName=utf8_decode(addslashes($_GET['validateName']));
    $var_id2Name		=utf8_decode(addslashes($_GET['id2Name']));
    $var_id2Type		=utf8_decode(addslashes($_GET['id2Type']));
    $var_id3Name		=utf8_decode(addslashes($_GET['id3Name']));
    $var_id3Type		=utf8_decode(addslashes($_GET['id3Type']));
    $var_id4Name		=utf8_decode(addslashes($_GET['id4Name']));
    $var_id4Type		=utf8_decode(addslashes($_GET['id4Type']));
    $var_id5Name		=utf8_decode(addslashes($_GET['id5Name']));
    $var_id5Type		=utf8_decode(addslashes($_GET['id5Type']));
    
    // Marque une éventuelle config déjà existante pour ce titre et cette URL comme désactivée pour laisser la place à la nouvelle
    
    //$szRequest="update config set active=0 where active=1 AND title='".$var_title."' AND typeapp='".$var_typeapp."'";
    // NOUVEAU : prend en compte l'URL si présente pour ne pas écraser des configs différentes avec titres identiques
    //           (exemple : config popup firefox qui ont toujours le même titre "Authentification requise")
    if ($var_url=="")
		$szRequest="update config set active=0 where active=1 AND title='".$var_title."' AND typeapp='".$var_typeapp."'";
	else 
		$szRequest="update config set active=0 where active=1 AND title='".$var_title."' AND url='".$var_url."'  AND typeapp='".$var_typeapp."'";
		
    $result=mysql_query($szRequest,$cnx);
    if ($result)
    {
		$szRequest="insert into config (active,typeapp,title,url,id1Name,id1Type,pwdName,validateName,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,id5Name,id5Type) values (1,'".$var_typeapp."','".$var_title."','".$var_url."','".$var_id1Name."','EDIT','".$var_pwdName."','".$var_validateName."','".$var_id2Name."','".$var_id2Type."','".$var_id3Name."','".$var_id3Type."','".$var_id4Name."','".$var_id4Type."','".$var_id5Name."','".$var_id5Type."')";
	    
		$result=mysql_query($szRequest,$cnx);
		if ($result)
		{
			echo "OK";
		}
		else
		{
			echo "<error>";
			echo "Request: ".$szRequest;
			echo "MySql said:". mysql_errno($cnx) . mysql_error($cnx);
			echo "</error>";
		}
	}
	else
	{
		echo "<error>";
		echo "Request: ".$szRequest;
		echo "MySql said:". mysql_errno($cnx) . mysql_error($cnx);
		echo "</error>";
	}
   
    
    
	mysql_close($cnx);
}
// ------------------------------------------------------------
// getversion (format : release:beta)
// release : 080  = 0.80
// beta    : 0811 = 0.81 BETA 1
//           081A = 0.81 BETA 10
//           0000 = pas de BETA disponible
// ------------------------------------------------------------
else if ($_GET['action'] =="getversion")
{
    header("Content-type: text/xml; charset=UTF-8");
    echo $swssoVersion;
}
// ------------------------------------------------------------
// showall
// ------------------------------------------------------------
else if ($_GET['action']=="showall08235ekop")
{
	$cnx=dbConnect();
      header("Content-type: text/html; charset=UTF-8");
      $szRequest="select title,url,typeapp,id1Name,pwdName,validateName,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,id5Name,id5Type from config where active=1 order by id";      
      $req=mysql_query($szRequest,$cnx);
      if (!$req) {echo "<html><p>Erreur : ". mysql_errno($cnx) . mysql_error($cnx)."</p></html>";return;}
      echo "<html><table border=1 style=\"font-family:Verdana; font-size:11px;\"><caption><B>swSSO - Liste des ".mysql_num_rows($req)." applications et sites enregistr&eacute;s</B></caption>";
      echo "<tr>";
      echo "<th>Titre</th>";
      echo "<th>URL</th>";
      echo "<th>Type</th>";
      echo "<th>Champ identifiant</th>";
      echo "<th>Champ mot de passe</th>";
      echo "<th>Bouton ou formulaire</th>";
      echo "<th>2nd id.</th>";
      echo "<th>3eme id.</th>";
      echo "<th>4eme id.</th>";
      //echo "<th>5eme id.</th>"; INUTILE POUR L'INSTANT -> 5EME ID PAS GERE PAR SWSSO.EXE
      echo "</tr>";
      for ($i=0;$i<mysql_num_rows($req);$i++)
      {
         $ligne = mysql_fetch_row($req);
         echo "<tr>";
         echo "<td>".utf8_encode($ligne[0])."</td>";
         if ($ligne[1]!="") echo "<td>".utf8_encode($ligne[1])."</td>"; else echo "<td align=center>-</td>";
         echo "<td>".utf8_encode($ligne[2])."</td>";
         if ($ligne[3]!="") echo "<td>".utf8_encode($ligne[3])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[4]!="") echo "<td>".utf8_encode($ligne[4])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[5]!="") echo "<td>".utf8_encode($ligne[5])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[6]!="") echo "<td>".utf8_encode($ligne[6])."(".utf8_encode($ligne[7]).")</td>"; else echo "<td align=center>-</td>";
         if ($ligne[8]!="") echo "<td>".utf8_encode($ligne[8])."(".utf8_encode($ligne[9]).")</td>"; else echo "<td align=center>-</td>";
         if ($ligne[10]!="") echo "<td>".utf8_encode($ligne[10])."(".utf8_encode($ligne[11]).")</td>"; else echo "<td align=center>-</td>";
         // INUTILE POUR L'INSTANT -> 5EME ID PAS GERE PAR SWSSO.EXE
         // if ($ligne[12]!="") echo "<td>".utf8_encode($ligne[12])."(".utf8_encode($ligne[13]).")</td>"; else echo "<td align=center>-</td>";
         echo "</tr>";
      }
      echo "</table>";
      echo "<br/><a href=./webservice.php?action=showold style=\"font-family:Verdana; font-size:11px;\">Voir les configurations archiv&eacute;es</a>";
      echo "</html>";
	mysql_close($cnx);
}
// ------------------------------------------------------------
// showold
// ------------------------------------------------------------
else if ($_GET['action']=="showold08235ekop")
{
	$cnx=dbConnect();
      header("Content-type: text/html; charset=UTF-8");
      $szRequest="select title,url,typeapp,id1Name,pwdName,validateName,id2Name,id2Type,id3Name,id3Type,id4Name,id4Type,id5Name,id5Type from config where active=0 order by id";      
      $req=mysql_query($szRequest,$cnx);
      if (!$req) {echo "<html><p>Erreur : ". mysql_errno($cnx) . mysql_error($cnx)."</p></html>";return;}
      echo "<html><table border=1 style=\"font-family:Verdana; font-size:11px;\"><caption><B>swSSO - Liste des ".mysql_num_rows($req)." configurations archiv&eacute;es</B></caption>";
      echo "<tr>";
      echo "<th>Titre</th>";
      echo "<th>URL</th>";
      echo "<th>Type</th>";
      echo "<th>Champ identifiant</th>";
      echo "<th>Champ mot de passe</th>";
      echo "<th>Bouton ou formulaire</th>";
      echo "<th>2nd id.</th>";
      echo "<th>3eme id.</th>";
      echo "<th>4eme id.</th>";
      // echo "<th>5eme id.</th>"; INUTILE POUR L'INSTANT -> 5EME ID PAS GERE PAR SWSSO.EXE
      echo "</tr>";
      for ($i=0;$i<mysql_num_rows($req);$i++)
      {
         $ligne = mysql_fetch_row($req);
         echo "<tr>";
         echo "<td>".utf8_encode($ligne[0])."</td>";
         if ($ligne[1]!="") echo "<td>".utf8_encode($ligne[1])."</td>"; else echo "<td align=center>-</td>";
         echo "<td>".utf8_encode($ligne[2])."</td>";
         if ($ligne[3]!="") echo "<td>".utf8_encode($ligne[3])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[4]!="") echo "<td>".utf8_encode($ligne[4])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[5]!="") echo "<td>".utf8_encode($ligne[5])."</td>"; else echo "<td align=center>-</td>";
         if ($ligne[6]!="") echo "<td>".utf8_encode($ligne[6])."(".utf8_encode($ligne[7]).")</td>"; else echo "<td align=center>-</td>";
         if ($ligne[8]!="") echo "<td>".utf8_encode($ligne[8])."(".utf8_encode($ligne[9]).")</td>"; else echo "<td align=center>-</td>";
         if ($ligne[10]!="") echo "<td>".utf8_encode($ligne[10])."(".utf8_encode($ligne[11]).")</td>"; else echo "<td align=center>-</td>";
         // INUTILE POUR L'INSTANT -> 5EME ID PAS GERE PAR SWSSO.EXE
         // if ($ligne[12]!="") echo "<td>".utf8_encode($ligne[12])."(".utf8_encode($ligne[13]).")</td>"; else echo "<td align=center>-</td>";
         echo "</tr>";
      }
      echo "</table></html>";
	mysql_close($cnx);
}

?>
