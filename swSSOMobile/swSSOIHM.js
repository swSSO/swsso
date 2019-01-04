//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

// =================================================== PAGE 1 (WELCOME) ==========================================
// Initialise la page 1 (welcome)
function pageWelcomeInit()
{
	console.log("> pageWelcomeInit()");
	if (localStorage.getItem("strPwdSalt")!=null) $.mobile.navigate("#PagePassword");
	console.log("< pageWelcomeInit()");
}
// charge le fichier JSON et stocke les données dans le localstorage
function loadJSON()
{
	console.log("> loadJSON()");
	$.ajax({
		type: "GET",
		url: "./test.json", // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX A FAIRE XXXXXXXXXXXXXXXXXXXXXXXX
		processData: true,
		data: {},
		dataType: "json",
		success: function(json) {
			localStorage.setItem("strPwdSalt",json.strPwdSalt);
			localStorage.setItem("strKeySalt",json.strKeySalt);
			localStorage.setItem("strIniPwdValue",json.strIniPwdValue);
			localStorage.setItem("json",JSON.stringify(json));
			$.mobile.navigate("#PagePassword");
		},
		error: function(x,y,z) {
			alert("Fichier introuvable.");
		}
	});
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
		$.mobile.navigate("#PageSafe");
	}
	console.log("< unlockSafe()");
}

// =================================================== PAGE 3 (SAFE) =============================================
// Initialise la page 3 (safe)
function pageSafeInit()
{
	console.log("> pageSafeInit()");
	if (localStorage.getItem("strKeyValue")==null)
	{
		$.mobile.navigate("#PagePassword");
	}
	else
	{
		var json=jQuery.parseJSON(localStorage.getItem("json"));
		// 1er tour pour déchiffrer le strApp (pour pouvoir trier)
		for(var i=0;i<json.apps.length;i++) 
		{
			var varDecryptedApp=decryptString(localStorage.getItem("strKeyValue"),json.apps[i].strApp);
			json.apps[i].strApp=varDecryptedApp;
			console.log("app#"+i+"="+json.apps[i].strApp);
		}
		// tri des applis par ordre alphabétique
		json.apps.sort(function(first, second) {
			return first.strApp.localeCompare(second.strApp);
		});	
		// 2eme tour pour afficher les applis		
		for(var i=0;i<json.apps.length;i++) 
		{
			console.log("app#"+i+"="+json.apps[i].strApp);
			var strDecryptedId=decryptString(localStorage.getItem("strKeyValue"),json.apps[i].strId);
			console.log(strDecryptedId);
			addItem(i,localStorage.getItem("strKeyValue"),json.apps[i].strApp,strDecryptedId,json.apps[i].strPassword);
		}
	}
	console.log("< pageSafeInit()");
}

// Copie une chaine dans le presse papier --> XXXXXXXXXXX à corriger : l'élément est créé en haut de page et fait scroller
function copyToClipboard(str) 
{
	console.log("> copyToClipboard()");
	var tempText = document.createElement('textArea');
	tempText.value = str;
	document.body.appendChild(tempText);
	console.log('useragent',navigator.userAgent);
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
		tempText.setSelectionRange(0, 100);
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
function showPassword(buttonPwdId,strKeyValue,strEncryptedPassword)
{
	console.log("> showPassword()");
	strClearPassword=decryptString(strKeyValue,strEncryptedPassword);
	$(buttonPwdId).text(strClearPassword);
	$(buttonPwdId).css('background-color','#AAAAAA');
	$(buttonPwdId).css('color','#FFFFFF');
	console.log("< showPassword()");
}
// affiche/masque le mot de passe 
function hidePassword(buttonPwdId)
{
	console.log("> hidePassword()");
	$(buttonPwdId).text("Voir le mot de passe");
	$(buttonPwdId).css('color','#333');
	$(buttonPwdId).css('background-color','#ededed');
	console.log("< hidePassword()");
}
// Ajoute une appli dans la liste
function addItem(i,strKeyValue,strAppli,strDecryptedId,strEncryptedPassword) 
{
	var content = "<div data-role='collapsible' data-filtertext='"+strAppli+"'><h3>"+strAppli+"</h3>";
	content+="<button onclick=\"copyToClipboard('"+strDecryptedId+"')\" class='ui-btn ui-shadow ui-corner-all'>"+strDecryptedId+"</button>";
	content+="<button onclick=\"copyPassword('"+strKeyValue+"','"+strEncryptedPassword+"')\" class='ui-btn ui-shadow ui-corner-all'>Copier le mot de passe</button>";
	content+="<button id=buttonPwd"+i+" ontouchstart=\"showPassword('#buttonPwd"+i+"','"+strKeyValue+"','"+strEncryptedPassword+"')\" ontouchend=\"hidePassword('#buttonPwd"+i+"')\" class='ui-btn ui-shadow ui-corner-all'>Voir le mot de passe</button>";
	content+="</div>";
	$("#listeApplis").append(content).collapsibleset('refresh');
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

// Supprime 
function restartFromScratch()
{
	console.log("> restartFromScratch()");
	localStorage.removeItem("strPwdSalt");
	localStorage.removeItem("strKeySalt");
	localStorage.removeItem("strIniPwdValue");
	localStorage.removeItem("json");
	localStorage.removeItem("strKeyValue");
	$.mobile.navigate("#PageWelcome");
	console.log("< restartFromScratch()");
}

// --------------------------------------------------------------------------------------------------
// ------------------------------------------ POUBELLE /TEST ---------------------------------------
// --------------------------------------------------------------------------------------------------

/* vérifie le mot de passe maitre et retourne la clé de chiffrement si OK
function showAccounts()
{
	// TODO : à lire dans le .ini
	var strPwdSalt = '66E9D278AB3B6AFD409CF40864388D88B708042CA9D61269BCD201A1A747B621FC2E11365CA90E51B2CFC19DC8104A47B5493E7EC1A2AC2943339A9DB34C234C';
	var strKeySalt = 'B14FF138A5DCEF2323FCF5E11ED25CDC93532600D47AC01B7AD0AB9B8B251F0925BF4816A7E02F86024C854D5226743902CA6167E90A471B21B60E6DA901717B';
	var strIniPwdValue = '20CE8D73FEC68162665227A853578629E6F1EDAF2DBB4BF25A100F0B0C25F081';
	// vérification du mot de passe
	var strPwd=document.getElementById("password").value;
	strPwd="Demo1234"; // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX A SUPPRIMER XXXXXXXXXXXXXXXXXXXXXXXX
	var binKeyValue=null;
	binKeyValue=checkPassword(strPwd,strPwdSalt,strKeySalt,strIniPwdValue);
	if (binKeyValue==null)
	{
		console.log('Mot de passe KO');
		alert('Mot de passe incorrect');
	}
	else
	{
		console.log('Mot de passe OK');
		var strEncryptedPassword='EB502BE24AF484289ED8528614F244427E306FC6384325F53046E4884AEC131C04ED0F1532CEE7EB0CDFFF7D3EE4A99C586CA683FCC3F267118BE52673AF79587D081A06F73F9363ED8C339811937C51188FE6EE800823EF91FF1C3BE6BA8810';
		var strEncryptedId='391025D0C4D8D3927FF869E134E6006ED7A3CADBA5864E3952C0BEBB04C9B079179134A5D86EED0611AAA1074696E0E8F8A083D5B4BF44F1DCBEADF1D54626395537A4322C3BF284E50063A78C031F0713E16070F1B95ECA5EB302EB35288881';
		var strApp='Demo swsso';
		var strClearId=decryptString(binKeyValue,strEncryptedId);
		//var tableToRemove=document.getElementById('table');
		//if (tableToRemove!=null) document.removeChild(tableToRemove);
		var table = document.createElement('table');
		for (var i = 1; i < 10; i++){
			var tr = document.createElement('tr');   
			var td1 = document.createElement('td');
			var td2 = document.createElement('td');
			var td3 = document.createElement('td');
			var textApp = document.createTextNode(strApp);
			var textId = document.createTextNode(strClearId);
			var button = document.createElement('button');
			button.innerHTML = 'Mot de passe';
			button.onclick = function(){
				copyPassword(binKeyValue,strEncryptedPassword);
				return false;
			};
			td1.appendChild(textApp);
			td2.appendChild(textId);
			td3.appendChild(button);
			tr.appendChild(td1);
			tr.appendChild(td2);
			tr.appendChild(td3);
			table.appendChild(tr);
		}
		document.body.appendChild(table);
	}
}*/

