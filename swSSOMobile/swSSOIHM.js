//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------

// Aiguillage et initialisations des pages
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

		if (data.toPage[0].id=="PageWelcome") // ------------------------------------------ PageWelcome
		{
			if (typeof data.options.fromPage=="undefined") // restauration de l'app sur iPhone ou appel depuis un signet
			{
				var prevPage=localStorage.getItem("curPage");
				if (prevPage!=null && prevPage!="#PageWelcome") { $.mobile.navigate(prevPage); e.preventDefault(); }
			}
		}
		else if (data.toPage[0].id=="PageURLPublique") // ------------------------------------- PageURLPublique
		{
			var strURL=localStorage.getItem("strURLPublique");
			if (strURL!=null) {document.getElementById("tbURLPublique").value=strURL;}; 
		}
		else if (data.toPage[0].id=="PageGDrivePublic") // ------------------------------------- PageGDrivePublic
		{
			var strFileId=localStorage.getItem("strFileIdPublic");
			if (strFileId!=null) {document.getElementById("tbIDGDrivePublic").value=strFileId;}; 
		}
		else if (data.toPage[0].id=="PageGDrivePrive") // ------------------------------------- PageGDrivePrive
		{
			var strFileId=localStorage.getItem("strFileIdPrive");
			if (strFileId!=null) {document.getElementById("tbIDGDrivePrive").value=strFileId;}; 
		}
		else if (data.toPage[0].id=="PagePassword") // ------------------------------------- PagePassword
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
		else if (data.toPage[0].id=="PageSafe") // ------------------------------------------ PageSafe
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
		else if (data.toPage[0].id=="PageOptions") // ------------------------------------------ PageOptions 
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
				var pwdTimeOut=localStorage.getItem("pwdTimeOut");
				if (pwdTimeOut==null) pwdTimeOut=5;
				$('#sliderPwdTimeout').val(pwdTimeOut).slider("refresh");
			}
		}
		else if (data.toPage[0].id=="PageApp") // ---------------------------------------------- PageApp 
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

