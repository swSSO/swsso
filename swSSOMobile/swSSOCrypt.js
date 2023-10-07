//-----------------------------------------------------------------------------
//                     swSSO - https://github.com/swSSO
//-----------------------------------------------------------------------------

// conversion d'une chaine hexa en ascii
function hexa2ascii(strHexa)
 {
	var strResult='';
	var iAsciiCode;
	for (var n=0;n<strHexa.length;n+=2)
	{
		iAsciiCode=parseInt(strHexa.substr(n,2),16)
		if (iAsciiCode==0) break;
		strResult+=String.fromCharCode(iAsciiCode);
	}
	return strResult;
}

// conversion d'une chaine ascii en hexa : nécessaire pour traiter le mot de passe maitre car la fonction PBKDF2 de sjcl
// utilise sjcl.codec.utf8String.toBits pour convertir le mot de passe s'il est passé en string (alors que le mot de passe
// côté Windows est converti avec le jeu de caractères ISO-8859-1)
function ascii2hexa(str)
{
	var tab=[];
	for (var n=0,l=str.length;n<l;n++) 
    {
		var hex=Number(str.charCodeAt(n)).toString(16);
		tab.push(hex);
	}
	return tab.join('');
}

// hmacSHA1
var hmacSHA1 = function (key) {
	var hasher = new sjcl.misc.hmac(key, sjcl.hash.sha1);
	this.encrypt = function () { return hasher.encrypt.apply(hasher, arguments); };
};

// déchiffre un identifiant ou un mot de passe du swsso.ini
function decryptString(strKeyValue,strEncryptedIniString)
{
	var strDecryptedIniString='[VIDE]';
	var binKeyValue=sjcl.codec.hex.toBits(strKeyValue);
	if (strEncryptedIniString.length==192)
	{
		var strIV=strEncryptedIniString.substring(0,32);
		var strEncrypedDataAndOV=strEncryptedIniString.substring(32,192);
		//console.log('strIV',strIV);
		//console.log('strEncrypedDataAndOV',strEncrypedDataAndOV);
		
		var binIV=sjcl.codec.hex.toBits(strIV);
		var binEncrypedDataAndOV=sjcl.codec.hex.toBits(strEncrypedDataAndOV);
		sjcl.beware["CBC mode is dangerous because it doesn't protect message integrity."]();
		var prp=new sjcl.cipher.aes(binKeyValue);
		var binDecryptedIniString=sjcl.mode.cbc.decrypt(prp,binEncrypedDataAndOV,binIV);
		var strHexDecryptedIniString=sjcl.codec.hex.fromBits(binDecryptedIniString);
		strDecryptedIniString=hexa2ascii(strHexDecryptedIniString);	
	}
	return strDecryptedIniString;
}

// vérifie le mot de passe maitre et retourne la clé de chiffrement si OK
function checkPassword(strPwd,strPwdSalt,strKeySalt,strIniPwdValue)
{
	console.log('checkPassword');
	var strKeyValue=null;
	var binKeyValue;
	var binPwdSalt=sjcl.codec.hex.toBits(strPwdSalt);
	var binKeySalt=sjcl.codec.hex.toBits(strKeySalt);
	var strPwdValue=sjcl.codec.hex.fromBits(sjcl.misc.pbkdf2(sjcl.codec.hex.toBits(ascii2hexa(strPwd)), binPwdSalt, 10000, 256, hmacSHA1)).toUpperCase();
	console.log('strPwdValue',strPwdValue);
	console.log('strIniPwdValue',strIniPwdValue);
	if (strPwdValue==strIniPwdValue) // mot de passe maitre OK
	{
		console.log('password:OK');
		binKeyValue=sjcl.misc.pbkdf2(sjcl.codec.hex.toBits(ascii2hexa(strPwd)), binKeySalt, 10000, 256, hmacSHA1);
		strKeyValue=sjcl.codec.hex.fromBits(binKeyValue);
	}
	else
	{
		console.log('password:KO');
		strKeyValue=null;
	}
	return strKeyValue;
}

/*
------------------------
Recompilation sjcl.js
------------------------
Dans C:\users\sylvain\.bashrc ajouter :
export JAVA_HOME='/c/Program Files/Java/jdk1.8.0_191'
export PATH=$JAVA_HOME/bin:$PATH
Lignes de commande : 
./configure --with-sha1 --with-cbc
make sjcl.js
*/
