//-----------------------------------------------------------------------------
//               swSSO - http://www.swsso.fr - sylvain@swsso.fr
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

// hmacSHA1
var hmacSHA1 = function (key) {
	var hasher = new sjcl.misc.hmac(key, sjcl.hash.sha1);
	this.encrypt = function () { return hasher.encrypt.apply(hasher, arguments); };
};

// déchiffre un identifiant ou un mot de passe du swsso.ini
function decryptString(strKeyValue,strEncryptedIniString)
{
	var strDecryptedIniString='ERROR';
	var binKeyValue=sjcl.codec.hex.toBits(strKeyValue);
	if (strEncryptedIniString.length==192)
	{
		var strIV=strEncryptedIniString.substring(0,32);
		var strEncrypedDataAndOV=strEncryptedIniString.substring(32,192);
		console.log('strIV',strIV);
		console.log('strEncrypedDataAndOV',strEncrypedDataAndOV);
		
		var binIV=sjcl.codec.hex.toBits(strIV);
		var binEncrypedDataAndOV=sjcl.codec.hex.toBits(strEncrypedDataAndOV);
		sjcl.beware["CBC mode is dangerous because it doesn't protect message integrity."]();
		var prp = new sjcl.cipher.aes(binKeyValue);
		var binDecryptedIniString=sjcl.mode.cbc.decrypt(prp, binEncrypedDataAndOV, binIV);
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
	var strPwdValue=sjcl.codec.hex.fromBits(sjcl.misc.pbkdf2(strPwd, binPwdSalt, 10000, 256, hmacSHA1)).toUpperCase();
	console.log('strPwdValue',strPwdValue);
	console.log('strIniPwdValue',strIniPwdValue);
	if (strPwdValue==strIniPwdValue) // mot de passe maitre OK
	{
		console.log('password:OK');
		binKeyValue=sjcl.misc.pbkdf2(strPwd, binKeySalt, 10000, 256, hmacSHA1);
		strKeyValue=sjcl.codec.hex.fromBits(binKeyValue);
	}
	else
	{
		console.log('password:KO');
		strKeyValue=null;
	}
	return strKeyValue;
}

/* Fonction de test (calcul du pwdSalt et déchiffrement d'un identifiant d'un .ini)
function test() 
{
	var sPwd = 'Demo1234';
	var sPwdSalt = '66E9D278AB3B6AFD409CF40864388D88B708042CA9D61269BCD201A1A747B621FC2E11365CA90E51B2CFC19DC8104A47B5493E7EC1A2AC2943339A9DB34C234C';
	var sKeySalt = 'B14FF138A5DCEF2323FCF5E11ED25CDC93532600D47AC01B7AD0AB9B8B251F0925BF4816A7E02F86024C854D5226743902CA6167E90A471B21B60E6DA901717B';
	var pwdSalt = sjcl.codec.hex.toBits(sPwdSalt);
	var keySalt = sjcl.codec.hex.toBits(sKeySalt);
	var hmacSHA1 = function (key) {
		var hasher = new sjcl.misc.hmac(key, sjcl.hash.sha1);
		this.encrypt = function () {
			return hasher.encrypt.apply(hasher, arguments);
		};
	};
	var sPwdValue=sjcl.codec.hex.fromBits(sjcl.misc.pbkdf2(sPwd, pwdSalt, 10000, 256, hmacSHA1))
	console.log('sPwdValue',sPwdValue);
	var sKeyValue=sjcl.codec.hex.fromBits(sjcl.misc.pbkdf2(sPwd, keySalt, 10000, 256, hmacSHA1))
	console.log('sKeyValue',sKeyValue);
	
	var siv = '391025D0C4D8D3927FF869E134E6006E';
	var iv=sjcl.codec.hex.toBits(siv);
	var adata;
	var sEncryptedData = 'D7A3CADBA5864E3952C0BEBB04C9B079179134A5D86EED0611AAA1074696E0E8F8A083D5B4BF44F1DCBEADF1D54626395537A4322C3BF284E50063A78C031F0713E16070F1B95ECA5EB302EB35288881';
	var encryptedData=sjcl.codec.hex.toBits(sEncryptedData);
	var keyValue=sjcl.codec.hex.toBits(sKeyValue);
	
	sjcl.beware["CBC mode is dangerous because it doesn't protect message integrity."]();
	var prp = new sjcl.cipher.aes(keyValue);
	var decryptedData=sjcl.mode.cbc.decrypt(prp, encryptedData, iv);
	var sDecryptedData=sjcl.codec.hex.fromBits(decryptedData);
	console.log('sDecryptedData (hex)',sDecryptedData);
	var sDecryptedData2=hexa2ascii(sDecryptedData);
	console.log('sDecryptedData (ascii)',sDecryptedData2);

	var test=decryptString(keyValue,siv+sEncryptedData);
	console.log('test',test);
}	

------------------------
Données de test
------------------------
mot de passe : Demo1234
id chiffré : identifiant
keySalt : B14FF138A5DCEF2323FCF5E11ED25CDC93532600D47AC01B7AD0AB9B8B251F0925BF4816A7E02F86024C854D5226743902CA6167E90A471B21B60E6DA901717B
pwdSalt : 66E9D278AB3B6AFD409CF40864388D88B708042CA9D61269BCD201A1A747B621FC2E11365CA90E51B2CFC19DC8104A47B5493E7EC1A2AC2943339A9DB34C234C
idValue : 391025D0C4D8D3927FF869E134E6006ED7A3CADBA5864E3952C0BEBB04C9B079179134A5D86EED0611AAA1074696E0E8F8A083D5B4BF44F1DCBEADF1D54626395537A4322C3BF284E50063A78C031F0713E16070F1B95ECA5EB302EB35288881
IV : 391025D0C4D8D3927FF869E134E6006E
CH : D7A3CADBA5864E3952C0BEBB04C9B079179134A5D86EED0611AAA1074696E0E8F8A083D5B4BF44F1DCBEADF1D54626395537A4322C3BF284E50063A78C031F07
OV : 13E16070F1B95ECA5EB302EB35288881
pwdValue : 20CE8D73FEC68162665227A853578629E6F1EDAF2DBB4BF25A100F0B0C25F081

------------------------
Recompilation sjcl.js
------------------------
C:\users\sylvain\.bashrc
export JAVA_HOME='/c/Program Files/Java/jdk1.8.0_191'
export PATH=$JAVA_HOME/bin:$PATH
./configure --with-sha1 --with-cbc
make sjcl.js
*/
