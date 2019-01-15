//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
//-----------------------------------------------------------------------------
 
 function googleAuth(strPostAction) 
 {
    var oauth2Endpoint = 'https://accounts.google.com/o/oauth2/v2/auth';

    var form = document.createElement('form');
    form.setAttribute('method','GET');
    form.setAttribute('action',oauth2Endpoint);

    var params = {'client_id': strCID,
                  'redirect_uri': 'https://www.swsso.fr/mobile/',  // test : http://www.mytasks.fr:82/mobile/ | prod : 
                  'scope': 'https://www.googleapis.com/auth/drive.file',
                  'state': strPostAction,
                  'include_granted_scopes': 'true',
                  'response_type': 'token'};

    for (var p in params) 
	{
		var input = document.createElement('input');
		input.setAttribute('type','hidden');
		input.setAttribute('name',p);
		input.setAttribute('value',params[p]);
		form.appendChild(input);
	}

    document.body.appendChild(form);
    form.submit();
}

var fragmentString = location.hash.substring(1);

// Parse query string to see if page request is coming from OAuth 2.0 server.
var params = {};
var regex = /([^&=]+)=([^&]*)/g, m;
while (m = regex.exec(fragmentString)) {
	params[decodeURIComponent(m[1])] = decodeURIComponent(m[2]);
}
if (Object.keys(params).length > 0) 
{
	localStorage.setItem('oauth2-params', JSON.stringify(params) );
	if (params['state'])
	{
		if (params['state'] == 'loadFromGDrive') 
		{
			loadFromGDrive(true,true);
		}
		else if (params['state'] == 'createFileOnGDrive') 
		{
			createFileOnGDrive();
		}
	}
}
