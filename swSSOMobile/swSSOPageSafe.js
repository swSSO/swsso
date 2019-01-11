//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

// Initialise la page 3 (safe)
function pageSafeInit()
{
	console.log("> pageSafeInit()");
	$("#listeApplis").empty();
	var json=jQuery.parseJSON(localStorage.getItem("json"));
	// 1er tour pour déchiffrer le strApp (pour pouvoir trier)
	for(var i=0;i<json.apps.length;i++) 
	{
		var varDecryptedApp=decryptString(localStorage.getItem("strKeyValue"),json.apps[i].strApp);
		json.apps[i].strApp=varDecryptedApp;
	}
	// tri des applis par ordre alphabétique
	json.apps.sort(function(first, second) {
		return first.strApp.localeCompare(second.strApp);
	});	
	// 2eme tour pour afficher les applis		
	for(i=0;i<json.apps.length;i++) 
	{
		//console.log("app#"+i+"="+json.apps[i].strApp);
		var strDecryptedId=decryptString(localStorage.getItem("strKeyValue"),json.apps[i].strId);
		//console.log(strDecryptedId);
		addItem(i,localStorage.getItem("strKeyValue"),json.apps[i].strApp,strDecryptedId,json.apps[i].strPassword);
	}
	console.log("< pageSafeInit()");
}

// Copie une chaine dans le presse papier
function copyToClipboard(strToCopy) 
{
	console.log("> copyToClipboard()");
	var tempText = document.createElement('textArea');
	tempText.value = strToCopy;
	document.body.appendChild(tempText);
	if (navigator.userAgent.match(/ipad|ipod|iphone/i))
	{
		var range, selection;
		tempText.contentEditable = true;
		tempText.readOnly = false;
		range = document.createRange();
		range.selectNodeContents(tempText);
		selection = window.getSelection();
		selection.removeAllRanges();
		selection.addRange(range);
		tempText.setSelectionRange(0,100);
	}
	else
	{
		tempText.select();
	}
	document.execCommand('copy');
	document.body.removeChild(tempText);
	console.log("< copyToClipboard()");
}

// Déchiffre le mot de passe et le copie dans le presse papier
function copyPassword(strKeyValue,strEncryptedPassword)
{
	console.log('> copyPassword()');
	strClearPassword=decryptString(strKeyValue,strEncryptedPassword);
	copyToClipboard(strClearPassword);
	console.log('< copyPassword()');
}

// affiche/masque le mot de passe 
function showPassword(strKeyValue,strEncryptedPassword)
{
	console.log("> showPassword()");
	strClearPassword=decryptString(strKeyValue,strEncryptedPassword);
	$("#btnShowPwd").text(strClearPassword);
	$("#btnShowPwd").css('background-color','#AAAAAA');
	$("#btnShowPwd").css('color','#FFFFFF');
	console.log("< showPassword()");
}

// affiche/masque le mot de passe 
function hidePassword()
{
	console.log("> hidePassword()");
	$("#btnShowPwd").text("Voir le mot de passe");
	$("#btnShowPwd").css('color','#333');
	$("#btnShowPwd").css('background-color','#ededed');
	console.log("< hidePassword()");
}

// Ajoute une appli dans la liste
function addItem(i,strKeyValue,strAppli,strDecryptedId,strEncryptedPassword) 
{
	var content="<li onclick=\"showPageApp('"+strKeyValue+"','"+strAppli+"','"+strDecryptedId+"','"+strEncryptedPassword+"');\" id=item"+i+"><a href='#PageApp'>"+strAppli+"</a></li>";
	$("#appList").append(content);
	$("#appList").listview("refresh");
}

// verrouillage du coffre
function lockSafe()
{
	console.log("> lockSafe()");
	localStorage.removeItem("strKeyValue");
	$.mobile.navigate("#PagePassword");
	console.log("< lockSafe()");
}
