//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//
//                   
//                       sylvain.werdefroy@gmail.com
//
//-----------------------------------------------------------------------------
// 
//  This file is part of swSSO.
//  
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
// 
//-----------------------------------------------------------------------------
// swSSOCrypt.cpp
//-----------------------------------------------------------------------------
// Toutes les fonctions crypto utilis�es pour la s�curisation du mot de passe 
// maitre et le chiffrement/d�chiffrement des mots de passe secondaires
//-----------------------------------------------------------------------------
// 0.65 : => perte compatibilit� v0.50 et v0.51
//        => le mot de passe maitre ne traine plus en m�moire (la cl�, oui...)
//-----------------------------------------------------------------------------

#include "stdafx.h"

HCRYPTPROV ghProv=NULL;



const char gcszAlpha[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char gcszNum[]="1234567890";
const char gcszSpecialChars[]="&'(-_)=+}]{[,?;.:/!*$";

//*****************************************************************************
//                             FONCTIONS PRIVEES
//*****************************************************************************

//-----------------------------------------------------------------------------
// swXORBuff() : XOR de deux buffer de m�me taille
// utilisation des unsigned char pour eviter erreur taille DWORD 32b 64b ?
// non optimis�
//-----------------------------------------------------------------------------
// [in/out]result = buffer � XORer
// [in] key = cl� pour le xor
// [in] len = longueur du buffer
//-----------------------------------------------------------------------------
static void swXORBuff(BYTE *result, BYTE *key, int len)
{
    int i;
    for(i=0; i<len; i++)
    {
        result[i] ^= key[i];
    }
}

//*****************************************************************************
//                             FONCTIONS PUBLIQUES
//*****************************************************************************

//-----------------------------------------------------------------------------
// swPBKDF2() : impl�mentation de PBKDF2 RFC 2898 (http://www.ietf.org/rfc/rfc2898.txt)
// Limit�e � 2 blocs de 160 bits en sortie, dont seuls les 256 premiers sont
// fournis � l'appelant. Ce r�sultat est utilis� pour :
// 1) Alimenter la fonction de d�rivation de cl� AES-256 (CryptDeriveKey)
// 2) Stocker le mot de passe ma�tre
//-----------------------------------------------------------------------------
// Avec 2 blocs, l'algo propos� par la RFC donne :
// DK = T_1 || T_2
// T_1 = F (P, S, c, 1)
// T_2 = F (P, S, c, 2)
// o� :
// P = password, an octet string
// S = salt, an octet string
// c = iteration count, a positive integer
// F (P, S, c, i) = U_1 \xor U_2 \xor ... \xor U_c
//   U_1 = PRF (P, S || INT (i))
//   U_2 = PRF (P, U_1)
//   ...
//   U_c = PRF (P, U_{c-1})
//-----------------------------------------------------------------------------
// MERCI A GUILLAUME POUR SA PROPOSITION D'IMPLEMENTATION !
//-----------------------------------------------------------------------------
// [out]bufResult    = pointeur vers un buffer de bytes (allou� par l'appelant)
// [in] bufResultLen = taille de cl� souhait�e (max HASH_LENx2)
// [in] szPwd = mot de passe maitre a deriver
// [in] bufSalt = buffer contenant le sel 
// [in] bufSaltLen = taille du buffer de sel (PBKDF2_SALT_LEN)
// [in] iNbIterations = nombre d'it�rations
//-----------------------------------------------------------------------------
int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations)
{
    TRACE((TRACE_ENTER,_F_,"bufResultLen=%d iNbIterations=%d",bufResultLen,iNbIterations));
	//TRACE((TRACE_PWD,_F_,"szPwd=%s",szPwd));
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)bufSalt,bufSaltLen,"sel"));
	
	int brc;
	int c; // it�rations
    int rc=-1;
    HCRYPTKEY hKey=NULL;
    HCRYPTHASH hHMAC=NULL;
    HMAC_INFO  hmacinfo;
    KEYBLOB *pKey=NULL;
	int iKeySize;
    DWORD dwLenHash;
    BYTE bufU_c[HASH_LEN]; // stocke successivement U_1, U_2, ..., U_c
	BYTE bufT[HASH_LEN*2]; // stocke le r�sultat final : T_1 � l'index 0 et T_2 � l'index HASH_LEN
	BYTE bufSaltWithBlocIndex[PBKDF2_SALT_LEN+4];
	int iBloc;

	if (bufResultLen>HASH_LEN*2) { TRACE((TRACE_ERROR,_F_,"bufResultLen=%d > valeur autoris�e HASH_LEN*2=%d",bufResultLen,HASH_LEN*2)); goto end; }
	if (bufSaltLen!=PBKDF2_SALT_LEN) { TRACE((TRACE_ERROR,_F_,"bufSaltLen=%d != valeur impos�e PBKDF2_SALT_LEN=%d",bufSaltLen,PBKDF2_SALT_LEN)); goto end; }

    // Cr�ation de la cl� HMAC en mode PLAINTEXTKEYBLOB, � partir du mot de passe
	iKeySize=sizeof(KEYBLOB)+strlen(szPwd);
    pKey=(KEYBLOB*)malloc(iKeySize);
	if (pKey==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iKeySize)); goto end; }
    ZeroMemory(pKey,iKeySize);

	// Cr�ation de la cl� sym�trique pour le HMAC
	// cf. doc de CryptImportKey() -> http://msdn.microsoft.com/en-us/library/aa380207(v=vs.85).aspx :
	// "When importing a Hash-Based Message Authentication Code (HMAC) key, the caller must identify 
	// the imported key as a PLAINTEXTKEYBLOB type"
	// "The HMAC algorithms do not have their own algorithm identifiers; use CALG_RC2 instead. 
	// CRYPT_IPSEC_HMAC_KEY allows the import of RC2 keys longer than 16 bytes."
	// cf. tests vectors de la RFC 2202 HMAC-SHA1
    pKey->header.bType=PLAINTEXTKEYBLOB;
    pKey->header.bVersion=CUR_BLOB_VERSION;
    pKey->header.reserved=0;
    pKey->header.aiKeyAlg=CALG_RC2;
    pKey->dwKeySize=strlen(szPwd);
	memcpy(pKey->KeyData,szPwd,pKey->dwKeySize);

	// test case 1 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=20;
	memset(pKey->KeyData,0x0b,pKey->dwKeySize);*/
	// fin test case 1 RFC 2202 HMAC-SHA1
	// test case 7 RFC 2202 HMAC-SHA1
	/*pKey->dwKeySize=80;
	memset(pKey->KeyData,0xaa,pKey->dwKeySize);*/
	// fin test case 7 RFC 2202 HMAC-SHA1

	//TRACE_BUFFER((TRACE_PWD,_F_,(BYTE*)pKey,iKeySize,"pKey (iKeySize=%d)",iKeySize));
    brc= CryptImportKey(ghProv,(LPBYTE)pKey,iKeySize,NULL,CRYPT_IPSEC_HMAC_KEY,&hKey);
    if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }

	// Initialisation du buffer resultat a zero 
	ZeroMemory(bufT,HASH_LEN*2);

	// It�ration pour sortir 2 blocs de 160 bits chacun (T_1 et T_2)
	for (iBloc=1;iBloc<=2;iBloc++) 
	{
		// concat�nation de l'index de bloc au sel
		memcpy(bufSaltWithBlocIndex,bufSalt,PBKDF2_SALT_LEN);
		bufSaltWithBlocIndex[bufSaltLen]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+1]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+2]=0x00;
		bufSaltWithBlocIndex[bufSaltLen+3]=(BYTE)iBloc;
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufSaltWithBlocIndex,bufSaltLen+4,"bufSaltWithBlocIndex"));

	    // Cr�ation du HMAC
		// cf. http://msdn.microsoft.com/en-us/library/aa382379(VS.85).aspx
		
		brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
		// Initialisation des param�tres du HMAC
		ZeroMemory(&hmacinfo,sizeof(hmacinfo));
		hmacinfo.HashAlgid=CALG_SHA1;
		brc=CryptSetHashParam(hHMAC,HP_HMAC_INFO, (BYTE*)&hmacinfo,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetHashParam()=0x%08lx",GetLastError())); goto end; }

		// test case 1 RFC 2202 HMAC-SHA1
		/*brc=CryptHashData(hHMAC,(const BYTE*)"Hi There",8, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam()=0x%08lx",GetLastError())); goto end; }
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"Test case 1 RFC 2202 HMAC-SHA1"));
		if (brc) goto end;*/
		// fin test case 1 RFC 2202 HMAC-SHA1
		// test case 7 RFC 2202 HMAC-SHA1
		/*brc=CryptHashData(hHMAC,(const BYTE*)"Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data",73, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam()=0x%08lx",GetLastError())); goto end; }
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"Test case 1 RFC 2202 HMAC-SHA1"));
		if (brc) goto end;*/
		// fin test case 7 RFC 2202 HMAC-SHA1

		// HMAC du sel enrichi de l'id de bloc
		brc=CryptHashData(hHMAC,bufSaltWithBlocIndex,bufSaltLen+4, 0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptHashData(bufSaltWithBlocIndex)=0x%08lx",GetLastError())); goto end; }
	
		// R�cup�ration du hash
		dwLenHash=sizeof(bufU_c);
		brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_1)=0x%08lx",GetLastError())); goto end; }
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufU_c,HASH_LEN,"bufU_1"));
		
		// Destruction du HMAC
		brc=CryptDestroyHash(hHMAC);
		if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
		hHMAC=NULL;

		// XOR dans le buffer r�sultat
		swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);

		// Iterations
		for (c=2;c<=iNbIterations;c++) // on d�marre � 1, la 1�re it�ration �tant faite pr�c�demment hors de la boucle
		{
		    // Cr�ation du HMAC
			brc=CryptCreateHash(ghProv,CALG_HMAC,hKey,0,&hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash()=0x%08lx",GetLastError())); goto end; }
    
			// Initialisation des param�tres du HMAC
			ZeroMemory(&hmacinfo,sizeof(hmacinfo));
			hmacinfo.HashAlgid=CALG_SHA1;
			brc=CryptSetHashParam(hHMAC,HP_HMAC_INFO,(BYTE*)&hmacinfo,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetHashParam()=0x%08lx",GetLastError())); goto end; }

			// HMAC du r�sultat de l'it�ration pr�c�dente
			brc=CryptHashData(hHMAC,bufU_c,HASH_LEN,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptHashData(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// Recup du hash
			brc=CryptGetHashParam(hHMAC,HP_HASHVAL,bufU_c,&dwLenHash,0);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(bufU_%d)=0x%08lx",c,GetLastError())); goto end; }

			// D�truire le HMAC
			brc=CryptDestroyHash(hHMAC);
			if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDestroyHash()=0x%08lx",GetLastError())); goto end; }
			hHMAC=NULL;

			// XOR dans le resultat
			swXORBuff(bufT+(iBloc-1)*HASH_LEN,bufU_c,HASH_LEN);
		}
		TRACE_BUFFER((TRACE_DEBUG,_F_,bufT+(iBloc-1)*HASH_LEN,HASH_LEN,"bufT_%d",iBloc));
	}
	// Les 2 blocs sont aliment�s, on extrait les bufResultLen pour alimenter bufResult
	memcpy(bufResult,bufT,bufResultLen);
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufResult,bufResultLen,"bufResult"));
	rc=0;
end:
	if (hHMAC!=NULL) CryptDestroyHash(hHMAC);
    if (hKey!=NULL) CryptDestroyKey(hKey);
    if (pKey!=NULL) free(pKey);
	
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));   
    return rc;
}

//-----------------------------------------------------------------------------
// swCryptInit()
//-----------------------------------------------------------------------------
// Initialisation de l'environnement crypto 
// Jusqu'� la version 0.92 : MS_ENHANCED_PROV / PROV_RSA_FULL
// A partir de la 0.93 : MS_ENH_RSA_AES_PROV ou MS_ENH_RSA_AES_PROV_XP / PROV_RSA_AES
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptInit()
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=-1;
	DWORD dwLastError=0;

	brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV,PROV_RSA_AES,0);
	if (brc) { rc=0; goto end; }

	dwLastError=GetLastError();
	TRACE((TRACE_INFO,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV | PROV_RSA_AES)=0x%08lx",dwLastError)); 
	if (dwLastError==NTE_BAD_KEYSET)
	{
		brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_NEWKEYSET);
		if (brc) { rc=0; goto end; }
		dwLastError=GetLastError();
		TRACE((TRACE_ERROR,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV | PROV_RSA_AES | CRYPT_NEWKEYSET)=0x%08lx",dwLastError)); 
	}
	else if (dwLastError==NTE_KEYSET_NOT_DEF) // provider non disponible, on doit �tre sous XP, on essaie l'autre
	{
		brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV_XP,PROV_RSA_AES,0);
		if (brc) { rc=0; goto end; }

		dwLastError=GetLastError();
		TRACE((TRACE_INFO,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV_XP | PROV_RSA_AES)=0x%08lx",dwLastError)); 
		if (dwLastError==NTE_BAD_KEYSET) 
		{
			brc=CryptAcquireContext(&ghProv,"swSSO.MS_ENH_RSA_AES_PROV",MS_ENH_RSA_AES_PROV_XP,PROV_RSA_AES,CRYPT_NEWKEYSET);
			if (brc) { rc=0; goto end; }
			dwLastError=GetLastError();
			TRACE((TRACE_ERROR,_F_,"CryptAcquireContext(MS_ENH_RSA_AES_PROV_XP | PROV_RSA_AES | CRYPT_NEWKEYSET)=0x%08lx",dwLastError)); 
		}
	}
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptTerm()
//-----------------------------------------------------------------------------
// Lib�ration du CSP 
//-----------------------------------------------------------------------------
void swCryptTerm()
{
	TRACE((TRACE_ENTER,_F_,""));
	if (ghProv!=NULL) { CryptReleaseContext(ghProv,0); ghProv=NULL; }
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptDestroyKey()
//-----------------------------------------------------------------------------
// Lib�ration d'une cl�
//-----------------------------------------------------------------------------
void swCryptDestroyKey(HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	if (hKey!=NULL) CryptDestroyKey(hKey); 
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swIsPBKDF2KeySaltReady()
//-----------------------------------------------------------------------------
// Retourne TRUE si le sel a d�j� �t� g�n�r� et est pr�t � �tre utilis�
//-----------------------------------------------------------------------------
BOOL swIsPBKDF2KeySaltReady(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=gSalts.bPBKDF2KeySaltReady;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swIsPBKDF2PwdSaltReady()
//-----------------------------------------------------------------------------
// Retourne TRUE si le sel a d�j� �t� g�n�r� et est pr�t � �tre utilis�
//-----------------------------------------------------------------------------
BOOL swIsPBKDF2PwdSaltReady(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=gSalts.bPBKDF2PwdSaltReady;
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

#define DRAIN_BUFFER_SIZE 0x20000 // (128 Ko)
//-----------------------------------------------------------------------------
// swGenPBKDF2Salt()
//-----------------------------------------------------------------------------
// G�n�re deux sels al�atoires pour gbufPBKDF2PwdSalt et gbufPBKDF2KeySalt
//-----------------------------------------------------------------------------
int swGenPBKDF2Salt(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	BOOL brc;
	BYTE *pBuffer128Ko=NULL;

	// Pour �tre s�r d'avoir un al�a non attaquable, je suis la pr�co du �8.1.2 du document 
	// "Cryptanalysis of the Windows Random Number Generator" de Leo Dorrendorf :
	// 1. Request and discard 128 kilobytes of WRNG output.
	// 2. Request as many bytes as needed to generate the secure token.
	// 3. Request and discard 128 kilobytes of WRNG output.
	// Remarque : c'est vraiment du luxe dans notre cas !
	
	// 1. Request and discard 128 kilobytes of WRNG output.
	pBuffer128Ko=(BYTE*)malloc(DRAIN_BUFFER_SIZE);
	if (pBuffer128Ko==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",128*1024)); goto end; }
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 1/2"));
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);

	// 2. Request as many bytes as needed to generate the secure token.
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,gSalts.bufPBKDF2PwdSalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2PwdSalt)=0x%08lx",GetLastError())); goto end; }
	brc=CryptGenRandom(ghProv,PBKDF2_SALT_LEN,gSalts.bufPBKDF2KeySalt);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2KeySalt)=0x%08lx",GetLastError())); goto end; }

	// 3. Request and discard 128 kilobytes of WRNG output.
	brc=CryptGenRandom(ghProv,DRAIN_BUFFER_SIZE,pBuffer128Ko);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE)=0x%08lx",GetLastError())); goto end; }
	SecureZeroMemory(pBuffer128Ko,DRAIN_BUFFER_SIZE);
	TRACE((TRACE_DEBUG,_F_,"CryptGenRandom(DRAIN_BUFFER_SIZE) 2/2"));

	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2PwdSalt"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gSalts.bufPBKDF2KeySalt"));

	gSalts.bPBKDF2KeySaltReady=TRUE;
	gSalts.bPBKDF2PwdSaltReady=TRUE;
	rc=0;
end:
	if (pBuffer128Ko!=NULL) free(pBuffer128Ko);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// swReadPBKDF2Salt()
//-----------------------------------------------------------------------------
// Lit les sels dans le .ini
//-----------------------------------------------------------------------------
int swReadPBKDF2Salt(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	char szPBKDF2Salt[PBKDF2_SALT_LEN*2+1];
	
	// Lecture du sel mot de passe
	GetPrivateProfileString("swSSO","pwdSalt","",szPBKDF2Salt,sizeof(szPBKDF2Salt),gszCfgFile);
	if (strlen(szPBKDF2Salt)!=PBKDF2_SALT_LEN*2) goto end;
	swCryptDecodeBase64(szPBKDF2Salt,(char*)gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN);
	gSalts.bPBKDF2PwdSaltReady=TRUE;
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gbufPBKDF2PwdSalt"));
	// Lecture du sel d�rivation de cl�
	GetPrivateProfileString("swSSO","keySalt","",szPBKDF2Salt,sizeof(szPBKDF2Salt),gszCfgFile);
	if (strlen(szPBKDF2Salt)!=PBKDF2_SALT_LEN*2) goto end;
	swCryptDecodeBase64(szPBKDF2Salt,(char*)gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN);
	gSalts.bPBKDF2KeySaltReady=TRUE;
	TRACE_BUFFER((TRACE_DEBUG,_F_,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gbufPBKDF2KeySalt"));

	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCreateAESKeyFromKeyData()
//-----------------------------------------------------------------------------
// Cr�e la cl� AES � partir de donn�es de la cl� (256 bits / 32 octets)
// et l'importe dans le ghProv
//-----------------------------------------------------------------------------
// [in] cszMasterPwd = mot de passe maitre
// [in/out] hKey = handle de cl�
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey)
{
	int iAESKeySize;
	KEYBLOB *pAESKey=NULL;
	BOOL brc;
	int rc=-1;
	TRACE((TRACE_ENTER,_F_,""));
	BYTE ZeroBuf[AES256_KEY_LEN];

	// 
	ZeroMemory(ZeroBuf,AES256_KEY_LEN);
	if (memcmp(pAESKeyData,ZeroBuf,AES256_KEY_LEN)==0) 
	{
		TRACE((TRACE_ERROR,_F_,"pAESKeyData is zerobuf")); 
		goto end;
	}

	// Cr�e la cl� AES
	iAESKeySize=sizeof(KEYBLOB)+AES256_KEY_LEN;
	pAESKey=(KEYBLOB*)malloc(iAESKeySize);
	if (pAESKey==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iAESKeySize)); goto end; }
	ZeroMemory(pAESKey,iAESKeySize);
	pAESKey->header.bType=PLAINTEXTKEYBLOB;
	pAESKey->header.bVersion=CUR_BLOB_VERSION;
	pAESKey->header.reserved=0;
	pAESKey->header.aiKeyAlg=CALG_AES_256;
	pAESKey->dwKeySize=AES256_KEY_LEN;
	memcpy(pAESKey->KeyData,pAESKeyData,AES256_KEY_LEN);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(BYTE*)pAESKey,iAESKeySize,"pAESKey (iAESKeySize=%d)",iAESKeySize));
	brc= CryptImportKey(ghProv,(LPBYTE)pAESKey,iAESKeySize,NULL,0,phKey);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptImportKey()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	if (pAESKey!=NULL) free(pAESKey);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDeriveKey()
//-----------------------------------------------------------------------------
// Calcul de la cl� de chiffrement des mots de passe par d�rivation du mot de 
// passe maitre
//-----------------------------------------------------------------------------
// [in] cszMasterPwd = mot de passe maitre
// [out] hKey = handle de cl�
// [out] pAESKeyData = bloc de donn�es pour ceux qui voudraient reconstruire la cl�
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptDeriveKey(const char *pszMasterPwd,HCRYPTKEY *phKey,BYTE *pAESKeyData,BOOL bForceOldDerivationFunction)
{
	TRACE((TRACE_ENTER,_F_,""));

	BOOL brc;
	int rc=-1;
	HCRYPTHASH hHash=NULL;
	
	// si la cl� a d�j� �t� cr��e, on la lib�re
	if (*phKey!=NULL) { CryptDestroyKey(*phKey); *phKey=NULL; }

	// cr�ation d'un hash
	brc=CryptCreateHash(ghProv,CALG_SHA1,0,0,&hHash);           
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptCreateHash()")); goto end; }
	if (bForceOldDerivationFunction)
	{
		// hash du mot de passe
		brc=CryptHashData(hHash,(unsigned char*)pszMasterPwd,strlen(pszMasterPwd),0); 
		if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptHashData()")); goto end; }
		// d�rivation
		brc=CryptDeriveKey(ghProv,CALG_AES_256,hHash,0,phKey); 
		TRACE((TRACE_INFO,_F_,"CryptDeriveKey(CALG_AES_256)"));
	}
	else // nouveau m�canisme +secure en 0.93 (PBKDF2) ISSUE#56
	{
		// Obtient 256 bits (32 octets) aupr�s de PBKDF2 pour g�n�rer une cl� AES-256
		if (!swIsPBKDF2KeySaltReady()) { TRACE((TRACE_ERROR,_F_,"swIsPBKDF2SaltReady()=FALSE")); goto end; }
		if (swPBKDF2(pAESKeyData,AES256_KEY_LEN,pszMasterPwd,gSalts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
		if (swCreateAESKeyFromKeyData(pAESKeyData,phKey)!=0) goto end;
	}
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDeriveKey()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptData()
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les donn�es � chiffrer / chiffr�es
// [in] lData = taille des donn�es � chiffrer (lData en entr�e = lData en sortie)
// [in] hKey = cl� de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptEncryptData(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	BOOL brc;
	int rc=-1;
	DWORD dwMode=CRYPT_MODE_CBC;

	brc=CryptSetKeyParam(hKey,KP_IV,iv,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKey,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }

	brc=CryptEncrypt(hKey,0,TRUE,0,pData,&lData,lData+16);
	if (!brc) 
	{
		TRACE((TRACE_ERROR,_F_,"CryptEncrypt()=0x%08lx",GetLastError()));
		goto end;
	}
	TRACE_BUFFER((TRACE_DEBUG,_F_,iv,16,"iv"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pData,64,"Donnees chiffrees"));
	TRACE_BUFFER((TRACE_DEBUG,_F_,pData+64,16,"ov"));
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncryptString()
//-----------------------------------------------------------------------------
// Chiffrement d'une chaine de caract�re. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pszSource = chaine source (termin�e par 0 !)
// [in] hKey = cl� de chiffrement
// [rc] chaine chiffr�e encod�e base64 termin�e par 0. A lib�rer par l'appelant.
//      Format du buffer de sortie (avant encodage base64) :
//      iv 16 octets (alea) + 64 octets (4 blocs de 16 octets) + ov 16 octets
///-----------------------------------------------------------------------------
char *swCryptEncryptString(const char *pszSource,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	char *pszDest=NULL;
	char unsigned *pSourceCopy=NULL;
	int lenSource,lenSourceCopy,lenDest;
	BOOL brc;
	
	lenSource=strlen(pszSource);
	if (lenSource>63) { TRACE((TRACE_ERROR,_F_,"lenSource=%d>63 (taille max chiffrable avec cette fonction=63(utile)+1(0)",lenSource)); goto end; } // 1.12B2-AC-TIE1
	lenSourceCopy=16+64+16; // iv + taille d'un multiple de 16 (128 bits) pour chiffrement AES
	lenDest=lenSourceCopy*2+1;
	//TRACE((TRACE_PWD,_F_,"pszSource=%s",pszSource));
	TRACE((TRACE_DEBUG,_F_,"lenSource=%d lenSourceCopy=%d lenDest=%d",lenSource,lenSourceCopy,lenDest));

	// 0.51 : init avec random sur taille max, puis copie du mot de passe.
	pSourceCopy=(unsigned char*)malloc(lenSourceCopy);
	if (pSourceCopy==NULL) goto end;

	brc = CryptGenRandom(ghProv,lenSourceCopy,pSourceCopy);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()")); goto end; }
	// on laisse les 16 premiers octets en alea (c'est l'iv)
	// on prend le 0 de fin de chaine qui permettra de distingue le mdp du padding
	memcpy(pSourceCopy+16,pszSource,lenSource+1); 
	
	if (swCryptEncryptData(pSourceCopy,pSourceCopy+16,64,hKey)!=0) goto end;

	pszDest=(char*)malloc(lenDest); // sera lib�r� par l'appelant
	if (pszDest==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenDest)); goto end; }
	swCryptEncodeBase64(pSourceCopy,lenSourceCopy,pszDest,lenDest);

	TRACE((TRACE_DEBUG,_F_,"pszDest=%s",pszDest));

end:
	if (pSourceCopy!=NULL) free(pSourceCopy); // free ajout� en 0.93B6
	TRACE((TRACE_LEAVE,_F_,"pszDest=0x%08lx",pszDest));
	return pszDest;
}

//-----------------------------------------------------------------------------
// swCryptDecryptDataAES()
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les donn�es � d�chiffrer / d�chiffr�es
// [in] lData = taille des donn�es � d�chiffrer (lData en entr�e = lData en sortie)
// [in] hKey = cl� de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptDecryptDataAES256(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	BOOL brc;
	DWORD dwMode=CRYPT_MODE_CBC;

	brc=CryptSetKeyParam(hKey,KP_IV,iv,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_IV)")); goto end; }
	brc=CryptSetKeyParam(hKey,KP_MODE,(BYTE*)&dwMode,0);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam(KP_MODE)")); goto end; }
	
	brc = CryptDecrypt(hKey,0,true,0,pData,&lData);
	if (!brc) { TRACE((TRACE_ERROR,_F_,"CryptDecrypt()=0x%08lx",GetLastError())); goto end; }
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDecryptData3DES()
// ANCIENNE FONCTION : n'est plus utilis�e que pour la migration 0.92 -> 0.93
//-----------------------------------------------------------------------------
// [in] iv = vecteur d'initialisation
// [in/out] pData = pointeur vers les donn�es � d�chiffrer / d�chiffr�es
// [in] lData = taille des donn�es � d�chiffrer (lData en entr�e = lData en sortie)
// [in] hKey = cl� de chiffrement
//-----------------------------------------------------------------------------
// Retour : 0 si OK
//-----------------------------------------------------------------------------
int swCryptDecryptData3DES(unsigned char *iv, unsigned char *pData,DWORD lData,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;

	BOOL brc;
	brc=CryptSetKeyParam(hKey,KP_IV,iv,0);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptSetKeyParam()")); goto end; }
	
	brc = CryptDecrypt(hKey,0,true,0,pData,&lData);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptDecrypt()=0x%08lx",GetLastError())); goto end;	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptDecryptString()
//-----------------------------------------------------------------------------
// D�chiffrement d'une chaine de caract�re. LA SOURCE N'EST PAS MODIFIEE
//-----------------------------------------------------------------------------
// [in] pszSource = chaine source chiffr�e encod�e en base 64 (termin�e par 0 !)
// [in] hKey = cl� de chiffrement
// [rc] chaine d�chiffr�e encod�e. A lib�rer par l'appelant.
//-----------------------------------------------------------------------------
char *swCryptDecryptString(const char *pszSource,HCRYPTKEY hKey)
{
	TRACE((TRACE_ENTER,_F_,""));
	char *pszDest=NULL;
	char *pszRet=NULL;
	int lenSource,lenDest;
	int lenVecteur;

	lenSource=strlen(pszSource);
	lenDest=lenSource/2;

	pszDest=(char*)malloc(lenDest+1); // pour le 0 apr�s d�chiffrement
	if (pszDest==NULL) goto end;

	if (swCryptDecodeBase64(pszSource,pszDest,lenDest)!=0) goto end;
	
	if (lenSource==LEN_ENCRYPTED_AES256)
	{
		lenVecteur=16;
		if (swCryptDecryptDataAES256((unsigned char*)pszDest,(unsigned char*)pszDest+16,64+16,hKey)!=0) goto end; 
		pszDest[lenDest]=0; // 0 de fin de chaine.
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"lenSource=%d (au lieu de %d (AES-256) attendu !)",lenSource,LEN_ENCRYPTED_AES256));
		goto end;
	}

	pszRet=(char*)malloc(lenDest-lenVecteur); // r�alloue un buf de 16 de moins et recopie
	if (pszRet==NULL) goto end;
	memcpy(pszRet,pszDest+lenVecteur,lenDest-lenVecteur);

end:
	if (pszDest!=NULL && pszDest!=pszRet) free(pszDest);
	TRACE((TRACE_LEAVE,_F_,"pszRet=0x%08lx",pszRet));
	return pszRet;
}


//-----------------------------------------------------------------------------
// swCryptSaltAndHashPassword()
// N'EST PLUS UTILISEE A PARTIR DE LA VERSION 0.93 ET PBKDF2
//-----------------------------------------------------------------------------
// Sale et hashe le mot de passe maitre
// si le buffer de sel est NULL, g�n�ration al�a (cr�ation, sinon c'est une v�rif) 
// 0.72 : nouveau param�tre iNbIterations (nb d'it�rations de hashage)
// 0.73 : nouveau param�tre bV72 pour correction it�ration mal impl�ment�e en 0.72
//-----------------------------------------------------------------------------
int swCryptSaltAndHashPassword(char *bufSalt, const char *szPwd,char **pszHashedPwd,int iNbIterations,bool bV72)
{
	TRACE((TRACE_ENTER,_F_,"bufSalt=0x%08lx",bufSalt));
	int rc=-1;
	
	BOOL brc;
	HCRYPTHASH hHash=NULL;
	DWORD lenPwd;
	DWORD lenHashedPwd;
	unsigned char *bufSaltedPwd=NULL;
	DWORD lenSaltedPwd;
	unsigned char  bufHash[HASH_LEN];
	DWORD lenHash;
	// 0.73
	unsigned char bufToHash[HASH_LEN];

	lenPwd=strlen(szPwd);
	lenSaltedPwd=SALT_LEN+lenPwd;
	lenHashedPwd=SALT_LEN*2+HASH_LEN*2+1;
	int i;

	// allocation du buffer pour stocker le sel et le mot de passe
	// c'est ce buffer qu'on hache ensuite
	bufSaltedPwd=(unsigned char*)malloc(lenSaltedPwd);
	if (bufSaltedPwd==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenSaltedPwd)); goto end; }
	if (bufSalt==NULL) // g�n�re le sel
	{
		brc = CryptGenRandom(ghProv,SALT_LEN,bufSaltedPwd);
		if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom()")); goto end; }
	}
	else // utilise le sel pass� en param�tre
	{
		memcpy(bufSaltedPwd,bufSalt,SALT_LEN);
	}
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufSaltedPwd,SALT_LEN,"sel"));

	// concat�ne le mot de passe
	memcpy(bufSaltedPwd+SALT_LEN,szPwd,lenPwd);
	//TRACE_BUFFER((TRACE_PWD,_F_,bufSaltedPwd,lenSaltedPwd,"� hacher")); // 0.72 (trace d�plac�e)
	// hashe le tout
	brc= CryptCreateHash(ghProv,CALG_SHA1,0,0,&hHash);           
	if (!brc) 
	{
		TRACE((TRACE_ERROR,_F_,"CryptCreateHash()"));
		goto end;
	}
	if 	(bV72)
	{
		// 0.72 : boucle hashage anti-cassage de mot de passe
		// foireux, j'ai oubli� de r�injecter le hash... honte � moi
		// corrig� ci-dessous en 0.73
		for (i=0;i<iNbIterations;i++)
		{
			brc = CryptHashData(hHash,bufSaltedPwd,lenSaltedPwd,0); 
			if (!brc) 
			{
				TRACE((TRACE_ERROR,_F_,"CryptHashData()"));
				goto end;
			}
		}
		// r�cup�re la valeur du hash
		lenHash=sizeof(bufHash);
		brc = CryptGetHashParam(hHash,HP_HASHVAL,bufHash,&lenHash,0);
		if (!brc) 
		{
			TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)"));
			goto end;
		}
	}
	else // version 073 corrige la 0.72...
	{
		// hashe le mot de passe
		brc = CryptHashData(hHash,bufSaltedPwd,lenSaltedPwd,0); 
		if (!brc) 
		{
			TRACE((TRACE_ERROR,_F_,"CryptHashData()"));
			goto end;
		}
		// r�cup�re la valeur du hash
		lenHash=sizeof(bufHash);
		brc = CryptGetHashParam(hHash,HP_HASHVAL,bufHash,&lenHash,0);
		if (!brc) 
		{
			TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)"));
			goto end;
		}
		CryptDestroyHash(hHash); hHash=NULL;
		// it�rations = hash le hash iNbIterations fois.
		for (i=0;i<iNbIterations;i++)
		{
			// copie le r�sultat du hash dans le buffer � hasher pour it�rer
			memcpy(bufToHash,bufHash,lenHash);
			brc= CryptCreateHash(ghProv,CALG_SHA1,0,0,&hHash);           
			if (!brc) 
			{
				TRACE((TRACE_ERROR,_F_,"CryptCreateHash()"));
				goto end;
			}
			// hashe le hash
			brc = CryptHashData(hHash,bufToHash,lenHash,0); 
			if (!brc) 
			{
				TRACE((TRACE_ERROR,_F_,"CryptHashData()"));
				goto end;
			}
			// r�cup�re la valeur du hash
			lenHash=sizeof(bufHash);
			brc = CryptGetHashParam(hHash,HP_HASHVAL,bufHash,&lenHash,0);
			if (!brc) 
			{
				TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)"));
				goto end;
			}	
			CryptDestroyHash(hHash); hHash=NULL;
		}
	}
	TRACE_BUFFER((TRACE_DEBUG,_F_,bufHash,lenHash,"hash"));
	// il reste � convertir chaque morceau (sel et r�sultat du hash) en base 64
	// et � concat�ner les 2

	// allocation de la destination
	*pszHashedPwd=(char*)malloc(lenHashedPwd);
	if (*pszHashedPwd==NULL) 
	{
		TRACE((TRACE_ERROR,_F_,"malloc(%d)",lenHashedPwd));
		goto end;
	}
	swCryptEncodeBase64(bufSaltedPwd,SALT_LEN,*pszHashedPwd,lenHashedPwd);
	swCryptEncodeBase64(bufHash,HASH_LEN,*pszHashedPwd+SALT_LEN*2,lenHashedPwd-SALT_LEN*2);

	TRACE((TRACE_DEBUG,_F_,"*pszHashedPwd=%s",*pszHashedPwd));
	rc=0;
end:
	if (bufSaltedPwd!=NULL) free(bufSaltedPwd);
	// 0.72 : lib�ration du hash
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swCryptEncodeBase64()
//-----------------------------------------------------------------------------
// Le relecteur attentif verra tout de suite que ce n'est pas un encodage
// base 64... Au moment o� j'ai �crit �a, j'ai voulu me simplifier la vie...
// Et maintenant, pour des raisons de compatibilit� ascendante, je pr�f�re
// laisser comme �a pour l'instant... Comme d'hab, le provisoire dure ;-)
//-----------------------------------------------------------------------------
// TODO : faire un vrai encodage base64
//-----------------------------------------------------------------------------
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString,int sizeofDestString)
{
	TRACE((TRACE_ENTER,_F_,""));
	int i;
    for (i=0;i<lenSrcData;i++) 
    {
		sprintf_s(pszDestString+2*i,sizeofDestString-2*i,"%02X",pSrcData[i]);
	}
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCryptDecodeBase64()
//-----------------------------------------------------------------------------
// Cf. remarque dans swCryptEncodeBase64()
//-----------------------------------------------------------------------------
// TODO : faire un vrai decodage base64
//-----------------------------------------------------------------------------
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData)
{
	TRACE((TRACE_ENTER,_F_,""));
	int iPosSrc=0;
	int iPosDest=0;
	int lenSrcString;
	int rc=-1;
	unsigned char uc;

	// si la longueur de la chaine n'est pas paire, on arr�te tout de suite !
	lenSrcString=strlen(szSrcString);
	TRACE((TRACE_DEBUG,_F_,"lenSrcString=%d lenDestData=%d",lenSrcString,lenDestData)); 
	if ((lenSrcString%2)!=0) { TRACE((TRACE_ERROR,_F_,"lenSrcString=%d",lenSrcString)); goto end; }

	while (iPosSrc<lenSrcString)
	{
		// v�rifie qu'on ne d�borde pas de pData
		if (iPosDest>=lenDestData) goto end; 
		// quartet de poids fort
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=(unsigned char)(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=(unsigned char)(szSrcString[iPosSrc]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",szSrcString[iPosSrc]));goto end; }
		iPosSrc++;
		uc=uc*16;
		// quartet de poids faible
		if (szSrcString[iPosSrc]>='A' && szSrcString[iPosSrc]<='F') uc=uc+(szSrcString[iPosSrc]-'A'+10);
		else if (szSrcString[iPosSrc]>='0' && szSrcString[iPosSrc]<='9') uc=uc+(szSrcString[iPosSrc]-'0');
		else { TRACE((TRACE_ERROR,_F_,"Invalid character=%c",szSrcString[iPosSrc]));goto end; }
		iPosSrc++;
		//TRACE((TRACE_DEBUG,_F_,"%c%c -> 0x%02x",szSrcString[iPosSrc-2],szSrcString[iPosSrc-1],uc));
		pDestData[iPosDest]=(char)uc;
		iPosDest++;
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=%d",rc));
    return rc;
}

//-----------------------------------------------------------------------------
// swGenerateRandomPwd()
//-----------------------------------------------------------------------------
// L'appelant a allou� le buffer de longueur suffisante
// Charge � lui de lib�rer et surtout d'effacer de mani�re s�curis�e
//-----------------------------------------------------------------------------
void swGenerateRandomPwd(char *pszPwd,int iPwdLen,int iPwdType)
{
	TRACE((TRACE_ENTER,_F_,"iPwdLen=%d iPwdType=%d",iPwdLen,iPwdType));
	BOOL brc;
	BYTE *pBuf=NULL;
	char szCharSet[100]="";
	int i;
	int iLenCharSet=0;
	
	pBuf=(BYTE*)malloc(iPwdLen);
	if (pBuf==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",iPwdLen)); goto end; }

	brc=CryptGenRandom(ghProv,iPwdLen,pBuf);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"CryptGenRandom(gbufPBKDF2PwdSalt)=0x%08lx",GetLastError())); goto end; }

	if (iPwdType & PWDTYPE_ALPHA)		 strcat_s(szCharSet,sizeof(szCharSet),gcszAlpha); 
	if (iPwdType & PWDTYPE_NUM)			 strcat_s(szCharSet,sizeof(szCharSet),gcszNum); 
	if (iPwdType & PWDTYPE_SPECIALCHARS) strcat_s(szCharSet,sizeof(szCharSet),gcszSpecialChars); 
	
	TRACE((TRACE_DEBUG,_F_,"szCharSet=%s",szCharSet));
	iLenCharSet=strlen(szCharSet);
	for (i=0;i<iPwdLen;i++)
	{
		pszPwd[i]=szCharSet[pBuf[i]%iLenCharSet];
		TRACE((TRACE_DEBUG,_F_,"pszPwd[%d]=%c",i,pszPwd[i]));
	}
	pszPwd[iPwdLen]=0;
	
end:
	if (pBuf!=NULL) free(pBuf);
	TRACE((TRACE_LEAVE,_F_,""));
}
