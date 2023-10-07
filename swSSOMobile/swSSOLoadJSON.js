//-----------------------------------------------------------------------------
//                     swSSO - https://github.com/swSSO
//-----------------------------------------------------------------------------

function parseAndStoreJSON(strJSON)
{
	console.log("> parseAndStoreJSON()");
	var json=jQuery.parseJSON(strJSON);
	localStorage.setItem("strPwdSalt",json.strPwdSalt);
	localStorage.setItem("strKeySalt",json.strKeySalt);
	localStorage.setItem("strIniPwdValue",json.strIniPwdValue);
	localStorage.setItem("json",JSON.stringify(json));
	console.log("< parseAndStoreJSON()");
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
		var xhr = new XMLHttpRequest();
		xhr.open('GET',strURL);
		console.log("strURL",strURL);
		if (bAuth) 
		{
			// 0.10.045
			// bloqué à partir de janvier 2020
			// strURL+='&access_token='+params['access_token'];
			// passage du token en http header
			xhr.setRequestHeader("Authorization", 'Bearer '+params['access_token']);
			console.log("Header Authorization: Bearer "+params['access_token'],strURL);
		}
		xhr.onload = function() {
			if (xhr.status==200)
			{
				parseAndStoreJSON(xhr.responseText);
				var strFileRevision=getFileRevision(bAuth,strFileId);
				if (strFileRevision!=null) localStorage.setItem("strFileRevision",strFileRevision);
				$.mobile.navigate("#PagePassword");
			}
			else
			{
				console.log(xhr.responseText);
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
				googleAuth('loadFromGDrive');
			}
		};		
		xhr.send();
	}
	else 
	{
		googleAuth('loadFromGDrive');
	}
	console.log("< loadFromGDrive()");
}

// créee un fichier swsso.json sur Google Drive (authent obligatoire)
function createFileOnGDrive()
{
	console.log("> createFileOnGDrive()");
	var access_token=null;
	var strFileId;
	
	var params=jQuery.parseJSON(localStorage.getItem('oauth2-params'));
	if (params!=null) console.log('access_token',params['access_token']);
	
	if (params && params['access_token'])
	{
		// 0.10.045
		// access_token en paramètre bloqué à partir de janvier 2020
		// var strUrl='https://www.googleapis.com/upload/drive/v3/files/?key='+strGK+'&uploadType=multipart&access_token=' + params['access_token'];
		var strUrl='https://www.googleapis.com/upload/drive/v3/files/?key='+strGK+'&uploadType=multipart';
		var xhr = new XMLHttpRequest();
		xhr.open('POST',strUrl);
		xhr.setRequestHeader('Content-Type', 'multipart/related; boundary=foo_bar_baz');
		xhr.setRequestHeader("Authorization", 'Bearer '+params['access_token']);
		console.log("strUrl",strUrl);
		console.log("Header Authorization: Bearer "+params['access_token'],strUrl);
		xhr.onload = function() {
			if (xhr.status==200)
			{
				var json=jQuery.parseJSON(xhr.responseText);
				localStorage.setItem('strFileIdPrive',json.id);
				$.mobile.navigate("#PageWaitForJSONUpload");
			}
			else
			{
				console.log(xhr.responseText);
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
				googleAuth('createFileOnGDrive');
			}
		};		
		var strBody='--foo_bar_baz\nContent-Type: application/json; charset=UTF-8\n'+
		'\n{"name": "swsso.json"}\n'+'--foo_bar_baz\n'+
		'Content-Type:text/plain\n'+'\nEmpty swSSO-Mobile File\n'+'--foo_bar_baz--';
		console.log(strBody);
		xhr.send(strBody);
	}
	else 
	{
		googleAuth('createFileOnGDrive');
	}
	console.log("< createFileOnGDrive()");
}

// récupère et stocke le n° de révision 
function getFileRevision(bAuth,strFileId)
{
	console.log("> getFileRevision() bAuth",bAuth);
	var strFileRevision=null;
	var params=jQuery.parseJSON(localStorage.getItem('oauth2-params'));
	console.log('params',params);
	if (params!=null) console.log('access_token',params['access_token']);
	
	if (!bAuth || (params && params['access_token']))
	{
		var strURL='https://www.googleapis.com/drive/v3/files/'+strFileId+'/revisions?pageSize=1&key='+strGK;
		var xhr = new XMLHttpRequest();
		xhr.open('GET',strURL);
		console.log("strURL",strURL);
		if (bAuth) 
		{
			// 0.10.045
			// bloqué à partir de janvier 2020
			// strURL+='&access_token='+params['access_token'];
			// passage du token en http header
			xhr.setRequestHeader("Authorization", 'Bearer '+params['access_token']);
			console.log("Header Authorization: Bearer "+params['access_token'],strURL);
		}
		xhr.onload = function() {
			if (xhr.status==200)
			{
				console.log(xhr.responseText);
				var json=jQuery.parseJSON(xhr.responseText);
				strFileRevision=json.revisions[0].id;
				$.mobile.navigate("#PagePassword");
			}
		};
		xhr.send();
	}
	console.log("< getFileRevision()");
	return strFileRevision;
}