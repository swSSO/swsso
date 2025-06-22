//-----------------------------------------------------------------------------
//                     swSSO - https://github.com/swSSO
//-----------------------------------------------------------------------------

// ajout 0.10.049 (ISSUE#414)
function copyToClipboard(text) {
  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(text).then(function() {
      console.log("Texte copié avec Clipboard API");
    }).catch(function(err) {
      fallbackCopy(text);
    });
  } else {
    fallbackCopy(text);
  }
}

function fallbackCopy(text) {
  const textarea = document.createElement("textarea");
  textarea.value = text;
  textarea.setAttribute("readonly", "");
  textarea.style.position = "absolute";
  textarea.style.left = "-9999px";
  document.body.appendChild(textarea);
  textarea.select();
  document.execCommand("copy");
  document.body.removeChild(textarea);
  console.log("Texte copié avec fallback execCommand");
}
// fin ajout 0.10.049 (ISSUE#414)

// Affiche la page
function showPageApp(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword)
{
	console.log("> showPageApp()",strAppli);
	addButtons(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword);
	localStorage.setItem("strAppli",strAppli);
	localStorage.setItem("strDecryptedId",strDecryptedId);
	localStorage.setItem("strEncryptedPassword",strEncryptedPassword);
	$.mobile.navigate("#PageApp");
	console.log("< showPageApp()");
}
// ajoute dynamiquement les boutons avec les bons contenus et les bons liens
function addButtons(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword)
{
	document.getElementById("PageAppTitle").textContent=strAppli;
	$("#PageAppButtons").empty();
	if (strDecryptedId!=null)
	{
		// 0.10.049 (ISSUE#414)
		// var btnCopyId=$("<button onclick=\"copyToClipboard('"+strDecryptedId+"')\" class='ui-btn ui-shadow ui-corner-all'>"+strDecryptedId+"</button>");
		//$("#PageAppButtons").append(btnCopyId);
		var btnCopyId = $("<button class='ui-btn ui-shadow ui-corner-all'>"+strDecryptedId+"</button>");
		btnCopyId.on("click", function() { copyToClipboard(strDecryptedId); });
		$("#PageAppButtons").append(btnCopyId);
	}
	if (strEncryptedPassword!=null)
	{
		var btnCopyPwd=$("<button onclick=\"copyPassword('"+strKeyValue+"','"+strEncryptedPassword+"')\" class='ui-btn ui-shadow ui-corner-all'>Copier le mot de passe</button>");
		$("#PageAppButtons").append(btnCopyPwd);
		var btnShowPwd=$("<button id='btnShowPwd' ontouchstart=\"showPassword('"+strKeyValue+"','"+strEncryptedPassword+"')\" onmousedown=\"showPassword('"+strKeyValue+"','"+strEncryptedPassword+"')\" ontouchend=\"hidePassword()\" onmouseup=\"hidePassword()\" class='ui-btn ui-shadow ui-corner-all'>Voir le mot de passe</button>");
		$("#PageAppButtons").append(btnShowPwd);
	}
	var btnRetour=$("<button onclick=\"$.mobile.navigate('#PageSafe')\" class='ui-btn ui-shadow ui-corner-all'>Retour</button>");
	$("#PageAppButtons").append(btnRetour);

}

// Restaure la page 5 (cas du rechargement ou de la réactivation de l'app sur iPhone)
function pageAppRestore(strKeyValue)
{
	console.log("> pageAppRestore()");
	strAppli=localStorage.getItem("strAppli");
	strDecryptedId=localStorage.getItem("strDecryptedId");
	strEncryptedPassword=localStorage.getItem("strEncryptedPassword");
	addButtons(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword);
	console.log("< pageAppRestore()");
}	
