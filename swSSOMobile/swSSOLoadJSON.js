//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

function parseAndStoreJSON(strJSON)
{
	console.log("> parseAndStoreJSON()");
	var json=jQuery.parseJSON(strJSON);
	localStorage.setItem("strPwdSalt",json.strPwdSalt);
	localStorage.setItem("strKeySalt",json.strKeySalt);
	localStorage.setItem("strIniPwdValue",json.strIniPwdValue);
	localStorage.setItem("json",JSON.stringify(json));
	console.log("> parseAndStoreJSON()");
}

// charge le fichier JSON depuis une URL publique et stocke les données dans le localstorage
function loadFromURL()
{
	console.log("> loadFromURL()");
	
	var strURL=document.getElementById("tbURLPublique").value;
	if (strURL==null || strURL=='') return;
	localStorage.setItem('strURLPublique',strURL);
	
	var xhr = new XMLHttpRequest();
	xhr.open('GET',strURL);
	xhr.onload = function() {
		if (xhr.status==200)
		{
			parseAndStoreJSON(xhr.responseText);
			$.mobile.navigate("#PagePassword");
		}
		else
		{
			console.log(xhr.response);
			alert("Fichier introuvable (erreur "+xhr.status+").");
		}
	};
	xhr.onerror = function() {
		alert("Fichier introuvable.");
    };
    xhr.send();
	console.log("< loadFromURL()");
}

// charge un fichier GoogleDrive avec ou sans authentification
function loadFromGDrive(bAuth,fromGoogleAuth)
{
	console.log("> loadFromGDrive() bAuth",bAuth,"fromGoogleAuth",fromGoogleAuth);
	var access_token=null;
	var strFileId;
	if (bAuth)
	{
		if (fromGoogleAuth)
		{
			strFileId=localStorage.getItem('strFileIdPrive');
			if (strFileId==null || strFileId=='') return;
		}
		else
		{
			strFileId=document.getElementById("tbIDGDrivePrive").value;
			if (strFileId==null || strFileId=='') return;
			localStorage.setItem('strFileIdPrive',strFileId);
		}
	}
	else
	{
		strFileId=document.getElementById("tbIDGDrivePublic").value;
		if (strFileId==null || strFileId=='') return;
		localStorage.setItem('strFileIdPublic',strFileId);
	}
	
	//var params = JSON.parse(localStorage.getItem('oauth2-params'));
	var params=jQuery.parseJSON(localStorage.getItem('oauth2-params'));
	
	console.log('params',params);
	if (params!=null) console.log('access_token',params['access_token']);
	
	if (!bAuth || (params && params['access_token']))
	{
		var strURL='https://www.googleapis.com/drive/v3/files/'+strFileId+'?alt=media&key='+strGK;
		if (bAuth) strURL+='&access_token='+params['access_token'];
		var xhr = new XMLHttpRequest();
		xhr.open('GET',strURL);
		console.log("strURL",strURL);
		xhr.onload = function() {
			if (xhr.status==200)
			{
				parseAndStoreJSON(xhr.responseText);
				$.mobile.navigate("#PagePassword");
			}
			else
			{
				if (xhr.status!=401) alert("Fichier introuvable (erreur "+xhr.status+").");
			}
		};
		xhr.onerror = function() {
			alert("Fichier introuvable.");
		};
		xhr.onreadystatechange = function (e) {
			if (xhr.readyState === 4 && xhr.status === 200) 
			{
				// à supprimer ?
			} 
			else if (xhr.readyState === 4 && xhr.status === 401) 
			{
				// Token invalide, lance l'authent
				googleAuth();
			}
		};		
		xhr.send();
	}
	else 
	{
		googleAuth();
	}
	console.log("< loadFromGDrive()");
}

