//-----------------------------------------------------------------------------
//                     swSSO - https://github.com/swSSO
//-----------------------------------------------------------------------------

// vérifie le mot de passe et génère la clé de déchiffrement
function unlockSafe()
{
	console.log("> unlockSafe()");
	var strKeyValue=null;
	var strPwd=document.getElementById("password").value;
	document.getElementById("password").value="";
	strKeyValue=checkPassword(strPwd,localStorage.getItem("strPwdSalt"),localStorage.getItem("strKeySalt"),localStorage.getItem("strIniPwdValue"));
	if (strKeyValue==null)
	{
		alert("Mot de passe incorrect.");
	}
	else
	{
		localStorage.setItem("strKeyValue",strKeyValue);
		var d = new Date();
		var dateLastLogin = d.getTime();
		localStorage.setItem("dateLastLogin",dateLastLogin);
		$.mobile.navigate("#PageSafe");
	}
	console.log("< unlockSafe()");
}
