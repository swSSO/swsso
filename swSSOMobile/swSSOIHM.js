//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

// Aiguillage et initialisations
function pageChange(e,data)
{
	console.log("> pageChange()");
	
	if (typeof data.toPage[0].id!='undefined') 
	{
		console.log("toPage",data.toPage[0].id);
		if (typeof data.options.fromPage=="undefined") 
			console.log("fromPage","undefined");
		else 
			console.log("fromPage",data.options.fromPage[0].id);

		if (data.toPage[0].id=="PageWelcome") // ------------------------------------------ WELCOME
		{
			if (typeof data.options.fromPage=="undefined") // restauration de l'app sur iPhone ou appel depuis un signet
			{
				var prevPage=localStorage.getItem("curPage");
				if (prevPage!=null && prevPage!="#PageWelcome") { $.mobile.navigate(prevPage); e.preventDefault(); }
			}
		}
		else if (data.toPage[0].id=="PagePassword") // ------------------------------------- PASSWORD
		{
			if (localStorage.getItem("strPwdSalt")==null) 
			{
				$.mobile.navigate("#PageWelcome"); e.preventDefault();
			}
			else if (localStorage.getItem("strKeyValue")!=null)
			{
				$.mobile.navigate("#PageSafe"); e.preventDefault();
			}
		}
		else if (data.toPage[0].id=="PageSafe") // ------------------------------------------ SAFE
		{
			if (localStorage.getItem("strPwdSalt")==null) 
			{
				$.mobile.navigate("#PageWelcome"); e.preventDefault();
			}
			else if (localStorage.getItem("strKeyValue")==null)
			{
				$.mobile.navigate("#PagePassword"); e.preventDefault();
			}
			else if (isPwdExpired())
			{
				$.mobile.navigate("#PagePassword"); e.preventDefault();
			}
			else
			{
				if (document.getElementById("item0")==null)
				{
					pageSafeInit();
				}
			}
		}
		else if (data.toPage[0].id=="PageOptions") // ------------------------------------------ OPTIONS 
		{
			if (localStorage.getItem("strPwdSalt")==null) 
			{
				$.mobile.navigate("#PageWelcome"); e.preventDefault();
			}
			else if (localStorage.getItem("strKeyValue")==null)
			{
				$.mobile.navigate("#PagePassword"); e.preventDefault();
			}
			else
			{
				var pwdTimeOut=localStorage.getItem("pwdTimeOut");
				if (pwdTimeOut==null) pwdTimeOut=5;
				$('#sliderPwdTimeout').val(pwdTimeOut).slider( "refresh" );
			}
		}
		else if (data.toPage[0].id=="PageApp") // ---------------------------------------------- APP 
		{
			if (localStorage.getItem("strPwdSalt")==null) 
			{
				$.mobile.navigate("#PageWelcome"); e.preventDefault();
			}
			else if (localStorage.getItem("strKeyValue")==null)
			{
				$.mobile.navigate("#PagePassword"); e.preventDefault();
			}
			else
			{
				if (document.getElementById("btnShowPwd")==null)
				{
					pageAppRestore(localStorage.getItem("strKeyValue"));
				}
			}

		}
		localStorage.setItem("curPage","#"+data.toPage[0].id);
	}
	console.log("< pageChange()");
}

// =================================================== PAGE 1 (WELCOME) ==========================================
// Initialise la page 1 (welcome)
function pageWelcomeInit(e,data)
{
	console.log("> pageWelcomeInit()");
	if (localStorage.getItem("strPwdSalt")!=null) $.mobile.navigate("#PagePassword");
	console.log("< pageWelcomeInit()");
}

// charge le fichier JSON et stocke les données dans le localstorage
function loadJSON()
{
	console.log("> loadJSON()");
	// remarque : on ne peut pas le faire avec $.ajax car la callback ne fonctionne pas avec la google API
	// erreur 400 : "parameter callback can only be used on requests returning json or xml data"
	var strFileId=document.getElementById("fileid").value;
	var strUrl='https://www.googleapis.com/drive/v3/files/'+strFileId+'?key='+strGK+'&alt=media';
	var xhr = new XMLHttpRequest();
	xhr.open('GET', strUrl);
	xhr.onload = function() {
		if (xhr.status==200)
		{
			var json=jQuery.parseJSON(xhr.responseText);
			localStorage.setItem("strPwdSalt",json.strPwdSalt);
			localStorage.setItem("strKeySalt",json.strKeySalt);
			localStorage.setItem("strIniPwdValue",json.strIniPwdValue);
			localStorage.setItem("json",JSON.stringify(json));
			$.mobile.navigate("#PagePassword");
		}
		else
		{
			alert("Fichier introuvable (erreur "+xhr.status+").");
		}
	};
	xhr.onerror = function() {
		alert("Fichier introuvable.");
    };
    xhr.send();
	console.log("< loadJSON()");
}
// =================================================== PAGE 2 (PASSWORD) =========================================
// Initialise la page 2 (password)
function pagePasswordInit()
{
	console.log("> pagePasswordInit()");
	if (localStorage.getItem("strPwdSalt")==null) 
	{
		$.mobile.navigate("#PageWelcome");
	}
	else
	{
		if (localStorage.getItem("strKeyValue")!=null) $.mobile.navigate("#PageSafe");
	}
	console.log("< pagePasswordInit()");
}

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

// =================================================== PAGE 3 (SAFE) =============================================
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
	var content="<li onclick=\"showApp('"+strKeyValue+"','"+strAppli+"','"+strDecryptedId+"','"+strEncryptedPassword+"');\" id=item"+i+"><a href='#PageApp'>"+strAppli+"</a></li>";
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

// =================================================== PAGE 4 (OPTIONS) =============================================
// Affiche la page 4 (options)
function showOptions()
{
	console.log("> showOptions()");
	$.mobile.navigate("#PageOptions");
	console.log("< showOptions()");
}

// Efface tous les éléments du coffre et repart à 0
function restartFromScratch()
{
	console.log("> restartFromScratch()");
	localStorage.removeItem("strPwdSalt");
	localStorage.removeItem("strKeySalt");
	localStorage.removeItem("strIniPwdValue");
	localStorage.removeItem("json");
	localStorage.removeItem("strKeyValue");
	$("#listeApplis").empty();
	$.mobile.navigate("#PageWelcome");
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

// =================================================== PAGE 5 (APP) =============================================
// Affiche la page 5 (App)
function showApp(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword)
{
	console.log("> showApp()",strAppli);
	addButtons(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword);
	localStorage.setItem("strAppli",strAppli);
	localStorage.setItem("strDecryptedId",strDecryptedId);
	localStorage.setItem("strEncryptedPassword",strEncryptedPassword);
	$.mobile.navigate("#PageApp");
	console.log("< showApp()");
}
// ajoute dynamiquement les boutons avec les bons contenus et les bons liens
function addButtons(strKeyValue,strAppli,strDecryptedId,strEncryptedPassword)
{
	document.getElementById("PageAppTitle").textContent=strAppli;
	$("#PageAppButtons").empty();
	if (strDecryptedId!=null)
	{
		var btnCopyId=$("<button onclick=\"copyToClipboard('"+strDecryptedId+"')\" class='ui-btn ui-shadow ui-corner-all'>"+strDecryptedId+"</button>");
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
