//-----------------------------------------------------------------------------
// swSSOCrypt.h
//-----------------------------------------------------------------------------

#define HASH_LEN 20   // SHA1=160 bits
#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define AES256_KEY_LEN 32   // 256 bits

#define MAX_NB_PRIVATEKEYS 100

#define SWCRYPT_ERROR			1 // erreur générique
#define SWCRYPT_BADPWD			2 // mot de passe de la clé à importer incorrect
#define SWCRYPT_KEYEXISTS		3 // cette clé est déjà dans le keystore
#define SWCRYPT_FILENOTFOUND	4 
#define SWCRYPT_FILEREAD		5 
#define SWCRYPT_FILEWRITE		6 

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();

int  swCryptDeriveKey(BYTE *pSalt,const char *pszMasterPwd,HCRYPTKEY *phKey);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);

int swCryptGetPrivateKeyFromSZData(char *szSaltData,char *szPrivateKeyData,char *szPassword,HCRYPTKEY *phPrivateKey);
int swCryptGetSZDataFromPrivateKey(HCRYPTKEY hPrivateKey,BYTE *pSalt,char *szPassword,char **ppszPrivateKeyData);

int swKeystoreLoad(char *szKeystoreFile);
int swKeystoreSave(char *szKeystoreFile);

int swKeystoreImportPrivateKey(char *szPrivateKeyFile, char *szPrivateKeyPassword, char *szKeystorePassword);
int swKeystoreGetPrivateKey(int iKeyId,char *szPassword,HCRYPTKEY *phPrivateKey);
int swKeystoreGetFirstPrivateKey(char *szPassword,HCRYPTKEY *phPrivateKey);

void swCryptErrorMsg(HWND w,int ret);

int swCryptDecryptDataRSA(int iKeyId,char *szKeystorePassword,BYTE *pEncryptedData,DWORD dwEncryptedDataLen,BYTE **ppData,DWORD *pdwDataLen);
char *swCryptEncryptAES(BYTE *pData,DWORD dwLenData,HCRYPTKEY hKey);

int swCreateAESKeyFromKeyData(BYTE *pAESKeyData,HCRYPTKEY *phKey);
