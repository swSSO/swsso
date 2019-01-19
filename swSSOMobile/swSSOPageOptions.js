//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

// Affiche la page 4 (options)
function showPageOptions()
{
	console.log("> showPageOptions()");
	$.mobile.navigate("#PageOptions");
	console.log("< showPageOptions()");
}

// Efface tous les éléments du coffre et repart à 0
function restartFromScratch()
{
	console.log("> restartFromScratch()");
	if (confirm("Voulez-vous effacer votre coffre et charger un nouveau fichier swsso.json ?"))
	{
		localStorage.removeItem("strPwdSalt");
		localStorage.removeItem("strKeySalt");
		localStorage.removeItem("strIniPwdValue");
		localStorage.removeItem("json");
		localStorage.removeItem("strKeyValue");
		$("#listeApplis").empty();
		$.mobile.navigate("#PageWelcome");
	}
	console.log("< restartFromScratch()");
}

// Sauvegarde les options
function saveOptions()
{
	console.log("> saveOptions()");
	var pwdTimeOut=$('#sliderPwdTimeout').val();
	console.log("sliderPwdTimeout",pwdTimeOut);
	localStorage.setItem("pwdTimeOut",pwdTimeOut);
	console.log("< saveOptions()");
}

// vérifie si le mot de passe a expiré. Si oui, efface la clé du localstorage
function isPwdExpired()
{
	var pwdTimeOut=localStorage.getItem("pwdTimeOut");
	if (pwdTimeOut==null) pwdTimeOut=5;
	var dateLastLogin=localStorage.getItem("dateLastLogin");
	if (dateLastLogin==null) // jamais connecté, demande le mot de passe
	{
		localStorage.removeItem("strKeyValue");
		return true;
	}
	var d = new Date();
	var dateNow = d.getTime();
	console.log("dateNow-dateLastLogin",dateNow-dateLastLogin);
	if (dateNow-dateLastLogin > (pwdTimeOut*60*1000))
	{
		localStorage.removeItem("strKeyValue");
		return true;
	}
	else
	{
		return false;
	}
}