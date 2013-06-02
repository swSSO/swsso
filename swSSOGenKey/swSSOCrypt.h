//-----------------------------------------------------------------------------
// swSSOCrypt.h
//-----------------------------------------------------------------------------

#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define AES256_KEY_LEN 32   // 256 bits

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();
int  swCryptExportKey(HCRYPTKEY hRSAKey, int iKeyId, char *szPassword, char *szPublicKeyFilex86, char *szPublicKeyFilex64, char* szPrivateKeyFile);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);
void swGenRegBinValue(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDeriveKey(const char *pszPwd,HCRYPTKEY *phKey);


