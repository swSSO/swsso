//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//							 http://www.swsso.fr
//                   
//                             sylvain@swsso.fr
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

#include "stdafx.h"

HANDLE ghPipe=INVALID_HANDLE_VALUE;
#define BUF_SIZE 1024

#define PBKDF2_PWD_LEN 32	// 256 bits
#define AES256_KEY_LEN 32   // 256 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilisé avec PBKDF2 (512 bits)
#define PWD_LEN 256
#define USER_LEN 256	// limite officielle
#define DOMAIN_LEN 256	// limite à moi...
#define SID_LEN 128     // limite à moi

typedef struct
{
	// domain et user name
	char szUserName[USER_LEN];
	char szLogonDomainName[DOMAIN_LEN];
	// mot de passe fourni par swSSOCM (chiffré par CryptProtectMemory)
	char bufPassword[PWD_LEN]; 
	char bufPasswordOld[PWD_LEN]; 
	BOOL bPasswordStored;
	BOOL bPasswordOldStored;
	// PSKS (Password Salt + Key Salt)
	T_SALT Salts;
	// SID
	char szSID[SID_LEN];
} T_USER_DATA;

T_USER_DATA gUserData[100]; // max 100 user sur le poste de travail, après on explose...
int giMaxUserDataIndex=0;
#define REGKEY_SVC					"SOFTWARE\\swSSO\\SVC"
#define REGVALUE_SWSSO_CLIENT		"swSSOClient" 
#define REGVALUE_SWSSO_MIGRATION	"swSSOMigration" 
// char gszHash_swSSOClient[64+1];    // -- en théorie devrait suffire... mais erreur 234 en v1.11... (ISSUE#293) donc je mets 256 dans la v1.11 patch 1 et on n'en parle plus
// char gszHash_swSSOMigration[64+1]; // -- en théorie devrait suffire... mais erreur 234 en v1.11... (ISSUE#293) donc je mets 256 dans la v1.11 patch 1 et on n'en parle plus
char gszHash_swSSOClient[256+1];
char gszHash_swSSOMigration[256+1];

BOOL gbNewVersion=FALSE; // ISSUE#307 : le flag passe à TRUE lorsque swSSOMigration appelle PUTPASS. Ensuite, il est retourné à swSSO.exe sur appel de

//-----------------------------------------------------------------------------
// swInitData()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swInitData(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	SecureZeroMemory(gUserData,sizeof(gUserData));
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// swCreatePipe()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int swCreatePipe()
{
	int rc=-1;
	SECURITY_ATTRIBUTES sa;
	char szACL[]="D:(A;;FRFW;;;IU)(A;;FRFW;;;SY)"; // IU=utilisateur session locale, SY=local system
	
	TRACE((TRACE_ENTER,_F_,""));
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=false;
	sa.lpSecurityDescriptor=NULL;
	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szACL,SDDL_REVISION_1,&(sa.lpSecurityDescriptor),NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ConvertStringSecurityDescriptorToSecurityDescriptor(%s)=%d",szACL,GetLastError()));
		goto end;	
	}
	ghPipe=CreateNamedPipe("\\\\.\\pipe\\swsso",
						PIPE_ACCESS_DUPLEX, 
						PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | PIPE_REJECT_REMOTE_CLIENTS, non supporté sous XP !!
						PIPE_UNLIMITED_INSTANCES,
						BUF_SIZE,
						BUF_SIZE,
						1000,
						&sa);
	if (ghPipe==INVALID_HANDLE_VALUE)
	{
		TRACE((TRACE_ERROR,_F_,"CreateNamedPipe()=%d",GetLastError()));
		goto end;
	}
	rc=0;
end:
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swGetSID()
//-----------------------------------------------------------------------------
// Retourne l'index de l'utilisateur dans la table gUserData ou -1 si non trouvé
// BufRequest est de la forme : xxxxxxxxx:domain(len=256):username(len=256):xxxxxxxxx
// iOffset pointe ici -------------------^
//-----------------------------------------------------------------------------
char *swGetSID(char *pszUserName)
{
	TRACE((TRACE_ENTER,_F_,""));
	DWORD cbRDN,cbSid;
	SID_NAME_USE eUse;
	SID *pSid=NULL;
	char *pszRDN=NULL;
	char *pszSid=NULL;
	
	cbSid=0;
	cbRDN=0;
	LookupAccountName(NULL,pszUserName,NULL,&cbSid,NULL,&cbRDN,&eUse); // pas de test d'erreur, car la fonction échoue forcément
	if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[1](%s)=%d",pszUserName,GetLastError()));
		goto end;
	}
	pSid=(SID *)malloc(cbSid); if (pSid==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbSid)); goto end; }
	pszRDN=(char *)malloc(cbRDN); if (pszRDN==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",cbRDN)); goto end; }
	if(!LookupAccountName(NULL,pszUserName,pSid,&cbSid,pszRDN,&cbRDN,&eUse))
	{
		TRACE((TRACE_ERROR,_F_,"LookupAccountName[2](%s)=%d",pszUserName,GetLastError()));
		goto end;
	}
	if (!ConvertSidToStringSid(pSid,&pszSid))
	{
		TRACE((TRACE_ERROR,_F_,"ConvertSidToStringSid()=%d",GetLastError()));
		goto end;
	}
end:
	if (pSid!=NULL) free(pSid);
	if (pszRDN!=NULL) free(pszRDN);
	TRACE((TRACE_LEAVE,_F_,"pszSid=%s",pszSid));
	return pszSid;
}

//-----------------------------------------------------------------------------
// swGetUserDataIndex()
//-----------------------------------------------------------------------------
// Retourne l'index de l'utilisateur dans la table gUserData ou -1 si non trouvé
// BufRequest est de la forme : xxxxxxxxx:domain(len=256):username(len=256):xxxxxxxxx
// iOffset pointe ici -------------------^
//-----------------------------------------------------------------------------
int swGetUserDataIndex(const char *BufRequest,int iOffset)
{
	TRACE((TRACE_ENTER,_F_,""));
	int rc=-1;
	int i;
	char *pszUserName,*pszLogonDomainName;
	char szShortLogonDomainName[DOMAIN_LEN];
	char *p;
	char *pszSID=NULL;
	pszLogonDomainName=(char*)BufRequest+iOffset;
	pszUserName=(char*)BufRequest+iOffset+USER_LEN;

	// ISSUE#360
	TRACE((TRACE_DEBUG,_F_,"pszUserName=%s",pszUserName));
	TRACE((TRACE_DEBUG,_F_,"LISTE DES %d UTILISATEURS CONNUS :",giMaxUserDataIndex));
#ifdef TRACES_ACTIVEES
	for (i=0;i<giMaxUserDataIndex;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"gUserData[%d] : szUserName=%s SID=%s",i,gUserData[i].szUserName,gUserData[i].szSID));
	}
#endif
	for (i=0;i<giMaxUserDataIndex;i++)
	{
		TRACE((TRACE_DEBUG,_F_,"pszUserName=%s gUserData[%d] : szUserName=%s SID=%s",pszUserName,i,gUserData[i].szUserName,gUserData[i].szSID));
		// ISSUE#346
		strcpy_s(szShortLogonDomainName,DOMAIN_LEN,gUserData[i].szLogonDomainName);
		p=strchr(szShortLogonDomainName,'.');
		if (p!=NULL) *p=0;
		TRACE((TRACE_DEBUG,_F_,"pszLogonDomainName=%s gUserData[%d].szLogonDomainName=%s szShortLogonDomainName=%s",pszLogonDomainName,i,gUserData[i].szLogonDomainName,szShortLogonDomainName));
		if (_stricmp(pszUserName,gUserData[i].szUserName)==0 &&
			(*gUserData[i].szLogonDomainName==0 || _stricmp(pszLogonDomainName,gUserData[i].szLogonDomainName)==0 || _stricmp(pszLogonDomainName,szShortLogonDomainName)==0)) // ISSUE#346
		{
			TRACE((TRACE_INFO,_F_,"Trouvé par son nom %s\\%s index %d",pszLogonDomainName,pszUserName,i));
			rc=i;
			goto end;
		}
	}
	if (rc==-1) // pas trouvé, recherche par SID
	{
		pszSID=swGetSID(pszUserName);
		if (pszSID==NULL) goto end;
		for (i=0;i<giMaxUserDataIndex;i++)
		{
			TRACE((TRACE_DEBUG,_F_,"pszUserName=%s gUserData[%d] : szUserName=%s SID=%s",pszUserName,i,gUserData[i].szUserName,gUserData[i].szSID));
			if (_stricmp(pszSID,gUserData[i].szSID)==0)
			{
				TRACE((TRACE_INFO,_F_,"Trouvé par son SID, on va mettre à jour les infos Domaine et UserName"));
				TRACE((TRACE_INFO,_F_,"Avant : gUserData[%d] : szLogonDomainName=%s szUserName=%s",i,gUserData[i].szLogonDomainName,gUserData[i].szUserName));
				memcpy(gUserData[i].szLogonDomainName,pszLogonDomainName,DOMAIN_LEN);
				memcpy(gUserData[i].szUserName,pszUserName,USER_LEN);
				TRACE((TRACE_INFO,_F_,"Après : gUserData[%d] : szLogonDomainName=%s szUserName=%s",i,gUserData[i].szLogonDomainName,gUserData[i].szUserName));
 				rc=i;
				goto end;
			}
		}
	}
end:
	if (pszSID!=NULL) LocalFree(pszSID); 
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// ReadAllowedHash()
//-----------------------------------------------------------------------------
// Lit les hashs autorisés à appeler SVC dans la base de registre
//-----------------------------------------------------------------------------
int ReadAllowedHash(void)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	int ret;
	HKEY hKey=NULL;
	DWORD dwValueSize,dwValueType;

	SecureZeroMemory(gszHash_swSSOClient,sizeof(gszHash_swSSOClient));
	SecureZeroMemory(gszHash_swSSOMigration,sizeof(gszHash_swSSOMigration)); // ajouté suite à ISSUE#293

	ret=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REGKEY_SVC,0,KEY_READ,&hKey);
	if (ret!=ERROR_SUCCESS) { TRACE((TRACE_ERROR,_F_,"RegOpenKeyEx(HKLM\\Software\\swSSO\\SVC)=%ld",ret)); goto end; }

	// hash swSSOClient -- si non trouvé, on arrête tout, swSSO ne doit pas fonctioner sans vérifier ce hash
	dwValueType=REG_SZ;
	dwValueSize=sizeof(gszHash_swSSOClient);
	ret=RegQueryValueEx(hKey,REGVALUE_SWSSO_CLIENT,NULL,&dwValueType,(LPBYTE)gszHash_swSSOClient,&dwValueSize);
	if (ret!=ERROR_SUCCESS) 
	{ 
		TRACE((TRACE_ERROR,_F_,"RegQueryValueEx(HKLM\\Software\\swSSO\\SVC\\swSSOClient)=%ld",ret));
		if (ret==ERROR_MORE_DATA) { TRACE((TRACE_ERROR,_F_,"taille passee=%d taille necessaire=%d",sizeof(gszHash_swSSOClient),dwValueSize)); }
		*gszHash_swSSOClient=0;
		goto end; 
	} 
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gszHash_swSSOClient,dwValueSize,"gszHash_swSSOClient"));

	// hash swSSOMigration -- si non trouvé, le service marche quand même mais ne répondra pas à swSSOMigration
	dwValueType=REG_SZ;
	dwValueSize=sizeof(gszHash_swSSOMigration);
	ret=RegQueryValueEx(hKey,REGVALUE_SWSSO_MIGRATION,NULL,&dwValueType,(LPBYTE)gszHash_swSSOMigration,&dwValueSize);
	if (ret==ERROR_SUCCESS) 
	{
		TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)gszHash_swSSOMigration,dwValueSize,"gszHash_swSSOMigration"));
	}
	else
	{ 
		TRACE((TRACE_INFO,_F_,"RegQueryValueEx(HKLM\\Software\\swSSO\\SVC\\swSSOMigration)=%ld",ret)); 
		if (ret==ERROR_MORE_DATA) { TRACE((TRACE_ERROR,_F_,"taille passee=%d taille necessaire=%d",sizeof(gszHash_swSSOMigration),dwValueSize)); }
		*gszHash_swSSOMigration=0;
	} 

	rc=0;
end:
	if (hKey!=NULL) RegCloseKey(hKey);	
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// GetFileHash()
//-----------------------------------------------------------------------------
// Retourne un hash du fichier 
//-----------------------------------------------------------------------------
int GetFileHash(char *szFile,unsigned char *pBufHashValue)
{
	TRACE((TRACE_ENTER,_F_, ""));
	HANDLE hf=INVALID_HANDLE_VALUE;
	DWORD dwFileSize;
	DWORD dwByteRead;
	char *pBytesToHash=NULL;
	HCRYPTHASH hHash=NULL;
	DWORD lenHash;
	int rc=-1;
	
	// ouvre le .ini en lecture
	hf=CreateFile(szFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hf==INVALID_HANDLE_VALUE) {	TRACE((TRACE_ERROR,_F_,"CreateFile(%s)=%d",szFile,GetLastError())); goto end;	}
	// regarde la taille et alloue le buffer pour la lecture
	dwFileSize=GetFileSize(hf,NULL);
	if (dwFileSize==INVALID_FILE_SIZE) { TRACE((TRACE_ERROR,_F_,"GetFileSize(%s)=INVALID_FILE_SIZE",szFile)); goto end;	}
	pBytesToHash=(char*)malloc(dwFileSize);
	if (pBytesToHash==NULL) { TRACE((TRACE_ERROR,_F_,"malloc(%d)",dwFileSize)); goto end;	}
		// lit le fichier complet
	if (!ReadFile(hf,pBytesToHash,dwFileSize,&dwByteRead,NULL)) { TRACE((TRACE_ERROR,_F_,"ReadFile(%s)=0x%08lx",szFile,GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"ReadFile(%s) : %ld octets lus",szFile,dwByteRead));
	// ferme le fichier
	CloseHandle(hf); hf=INVALID_HANDLE_VALUE;
	// calcul du hash
	if (!CryptCreateHash(ghProv,CALG_SHA_256,0,0,&hHash)) { TRACE((TRACE_ERROR,_F_,"CryptCreateHash(CALG_SHA_256)=0x%08lx",GetLastError())); goto end; }
	if (!CryptHashData(hHash,(unsigned char*)pBytesToHash,dwFileSize,0)) { TRACE((TRACE_ERROR,_F_,"CryptHashData()=0x%08lx",GetLastError())); goto end; }
	lenHash=32;
	if (!CryptGetHashParam(hHash,HP_HASHVAL,pBufHashValue,&lenHash,0)) { TRACE((TRACE_ERROR,_F_,"CryptGetHashParam(HP_HASHVAL)=0x%08lx",GetLastError())); goto end; }
	TRACE_BUFFER((TRACE_DEBUG,_F_,pBufHashValue,lenHash,"hash"));
	rc=0;
	
end:
	if (hf!=INVALID_HANDLE_VALUE) CloseHandle(hf);
	if (pBytesToHash!=NULL) free(pBytesToHash);
	if (hHash!=NULL) CryptDestroyHash(hHash);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// IsCallingProcessAllowed()
//-----------------------------------------------------------------------------
// Vérifie que le hash du process appelant le pipe est bien connu
//-----------------------------------------------------------------------------
BOOL IsCallingProcessAllowed(unsigned long ulClientProcessId)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	HANDLE hCallingProcess=NULL;
	char szCallingProcess[MAX_PATH];
	unsigned char bufHashValue[32]; // SHA-256 = 32 octets
	char szBufHashValue[32*2+1]; // hash en hexadecimal
	
	// Récupère le chemin complet du process
	hCallingProcess=OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ulClientProcessId);
	if (hCallingProcess==NULL) { TRACE((TRACE_ERROR,_F_,"OpenProcess(%d)=%d",ulClientProcessId,GetLastError())); goto end; }

	if (GetModuleFileNameEx(hCallingProcess,NULL,szCallingProcess,MAX_PATH)==0) { TRACE((TRACE_ERROR,_F_,"GetModuleFileNameEx(%d)=%d",ulClientProcessId,GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"GetModuleFileNameEx(%d)-->%s",ulClientProcessId,szCallingProcess));

	// Calcule le hash du fichier
	if (GetFileHash(szCallingProcess,bufHashValue)!=0) goto end;

	// Convertit le hash en hexa (maintenant que tout le monde sait que ma fonction d'encodage en base 64 n'en sera jamais une, je l'utilise pour faire de l'encodage hexa)
	swCryptEncodeBase64(bufHashValue,32,szBufHashValue);
	TRACE((TRACE_INFO,_F_,"szBufHashValue=%s",szBufHashValue));

	// Lit les hashs autorisés à appeler SVC (en base de registre)
	if (ReadAllowedHash()!=0) goto end;

	// Compare avec le hash du client swSSO et celui du client de migration s'il est connu
	if (_stricmp(szBufHashValue,gszHash_swSSOClient)==0)
	{
		TRACE((TRACE_INFO,_F_,"L'appel vient de swSSOClient"));
	}
	else if (*gszHash_swSSOMigration!=0 && _stricmp(szBufHashValue,gszHash_swSSOMigration)==0)
	{
		TRACE((TRACE_INFO,_F_,"L'appel vient de swSSOMigration"));
		gbNewVersion=TRUE; // ISSUE#307
	}
	else
	{
		TRACE((TRACE_ERROR,_F_,"Invalid calling process (hash : %s)",szBufHashValue)); 
		goto end; 
	}

	rc=TRUE;
end:
	if (hCallingProcess!=NULL) CloseHandle(hCallingProcess);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// IsCallingProcessMPNotifier()
//-----------------------------------------------------------------------------
// Pour PUTPASS, on n'accepte que les appels du mpnotifier.exe dans system32
//-----------------------------------------------------------------------------
BOOL IsCallingProcessMPNotifier(unsigned long ulClientProcessId)
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	HANDLE hCallingProcess=NULL;
	char szCallingProcessImageName[MAX_PATH]; // \Device\HarddiskVolume2\Windows\System32\mpnotify.exe
	char szSystemDirectory[MAX_PATH];		  // c:\windows\system32
	char szAllowedName[MAX_PATH];			  // \windows\system32\mpnotify.exe
	DWORD len,lenAllowedName,lenCallingProcessImageName;

	// Récupère le chemin complet du process
	hCallingProcess=OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ulClientProcessId);
	if (hCallingProcess==NULL) { TRACE((TRACE_ERROR,_F_,"OpenProcess(%d)=%d",ulClientProcessId,GetLastError())); goto end; }
	
	// Comme le service est compilé en 32 bits et que le mpnotifier.exe est 64 bits sur les systèmes 64 bits, la fonction GetModuleFileNameEx
	// ne fonctionne pas. Il faut utiliser GetProcessImageFileName qui retourne un truc du genre :
	// \Device\HarddiskVolume2\Windows\System32\mpnotify.exe
	if (GetProcessImageFileName(hCallingProcess,szCallingProcessImageName,MAX_PATH)==0) { TRACE((TRACE_ERROR,_F_,"GetProcessImageFileName(%d)=%d",ulClientProcessId,GetLastError())); goto end; }
	TRACE((TRACE_DEBUG,_F_,"GetModuleFileNameEx(%d)-->%s",ulClientProcessId,szCallingProcessImageName));

	// construit le chemin théorique autorisé
	len=GetSystemDirectory(szSystemDirectory,sizeof(szSystemDirectory));
	if (len<2) { TRACE((TRACE_ERROR,_F_,"GetSystemDirectory()=%d",GetLastError())); goto end; }
	strcpy_s(szAllowedName,sizeof(szAllowedName),szSystemDirectory+2); 
	strcat_s(szAllowedName,sizeof(szAllowedName),"\\mpnotify.exe");
	TRACE((TRACE_DEBUG,_F_,"szAllowedName=%s",szAllowedName));

	// vérifie que le szCallingProcessImageName se termine bien par le szAllowedName
	lenAllowedName=strlen(szAllowedName);
	lenCallingProcessImageName=strlen(szCallingProcessImageName);
	if (lenAllowedName>lenCallingProcessImageName) { TRACE((TRACE_ERROR,_F_,"szAllowedName(%s) plus court que szCallingProcessImageName(%s) --> probleme !",szAllowedName,szCallingProcessImageName)); goto end; }

	if (_strnicmp(szCallingProcessImageName+lenCallingProcessImageName-lenAllowedName,szAllowedName,lenAllowedName)!=0)
	{
		TRACE((TRACE_ERROR,_F_,"%s forbidden",szCallingProcessImageName)); goto end;
	}
	TRACE((TRACE_INFO,_F_,"L'appel vient de mpnotify"));

	rc=TRUE;
end:
	if (hCallingProcess!=NULL) CloseHandle(hCallingProcess);
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swWaitForMessage()
//-----------------------------------------------------------------------------
// Attend un message et le traite. Commandes supportées :
// 1.13+ :
// V02:NEWVERS > demande si nouvelle version > retourne YES ou NO
// 1.08+ :
// V02:GETPASS:domain(256octets)username(256octets) > demande à SVC de fournir le mot de passe Windows (chiffré par PHKD)
// 1.08- :
// V02:PUTPASS:domain(256octets)username(256octets)password(256octets) (chiffré par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_CROSS_PROCESS)
// V02:GETPHKD:CUR:domain(256octets)username(256octets) > demande à SVC de fournir le KeyData courant
// V02:GETPHKD:OLD:domain(256octets)username(256octets) > demande à SVC de fournir le KeyData précédent (si chgt de mdp par exemple)
// V02:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets) > demande à SVC de stocker PwdSalt et KeySalt
// Ne sont plus supportées :
// V01:PUTPHKD:PwdHash(32octets)KeyData(32octets) > demande à SVC de stocker PwdHash et KeyData 
// V01:PUTPASS:password(256octets)(chiffré par CryptProtectMemory avec flag CRYPTPROTECTMEMORY_SAME_LOGON)
// V01:GETPHKD:CUR > demande à SVC de fournir le KeyData courant
// V01:GETPHKD:OLD > demande à SVC de fournir le KeyData précédent (si chgt de mdp par exemple)
// V01:PUTPSKS:PwdSalt(64octets)KeySalt(64octets) > demande à SVC de stocker PwdSalt et KeySalt
// V01:GETPSKS: >demande à SVC de fournir PwdSalt et KeySalt
// Remarque : à partir de la v1.11, l'appelant est vérifié dans les méthodes GETPHKD et GETPASS
//-----------------------------------------------------------------------------
int swWaitForMessage()
{
	int rc=-1;
	char bufRequest[1024];
	char bufResponse[1024];
	char szEventName[1024];
	int lenResponse;
	DWORD cbRead,cbWritten;
	HANDLE hEvent=NULL;
	BOOL brc;
	char tmpBufPwd[PWD_LEN];
	int iUserDataIndex;
	// PHKD (Password Hash + Key Data)
	BYTE PBKDF2Pwd[PBKDF2_PWD_LEN];
	BYTE AESKeyData[AES256_KEY_LEN];
	HCRYPTKEY hKey=NULL;
	char *pszEncryptedPwd=NULL;
	unsigned long ulClientProcessId;

	TRACE((TRACE_ENTER,_F_,""));	
	
	// Se met en attente d'un message
	if (!ConnectNamedPipe(ghPipe, NULL))
	{
		DWORD dwLastError=GetLastError();
		if (dwLastError==ERROR_PIPE_CONNECTED) // en fait dans ce cas là ce n'est pas une erreur (si, si, regardez bien le MSDN...)
		{
			TRACE((TRACE_INFO,_F_,"ConnectNamedPipe()=ERROR_PIPE_CONNECTED, pas une erreur"));
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"ConnectNamedPipe()=%d",GetLastError()));
			goto end;
		}
	}

	// Récupère le processid du process connecté au pipe
	brc=GetNamedPipeClientProcessId(ghPipe,&ulClientProcessId);
	if (!brc) {	TRACE((TRACE_ERROR,_F_,"GetNamedPipeClientProcessId()=%d",GetLastError())); goto end; }
	TRACE((TRACE_INFO,_F_,"ulClientProcessId=%ld",ulClientProcessId));

	// Lit la requête
    if (!ReadFile(ghPipe,bufRequest,sizeof(bufRequest),&cbRead,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"ReadFile(%d)=%d",sizeof(bufRequest),GetLastError()));
		goto end;
	}          
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufRequest,cbRead,"Request"));
	if (cbRead<11)
	{
		TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu >=11",cbRead));
		strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
		lenResponse=strlen(bufResponse);
	}
	else // Analyse la requête
	{
		if (memcmp(bufRequest,"V02:",4)==0)
		{
			//-------------------------------------------------------------------------------------------------------------
			if (memcmp(bufRequest+4,"PUTPASS:",8)==0) // Format = V01:PUTPASS:domain(256octets)username(256octets)password(256 octets, chiffré)
			//-------------------------------------------------------------------------------------------------------------
			{
				// nouveau en 1.12B3 : regarde si l'appel vient de mpnotifier ou d'un module swsso autorisé par hash
				if (!IsCallingProcessMPNotifier(ulClientProcessId) && !IsCallingProcessAllowed(ulClientProcessId)) 
				{
					strcpy_s(bufResponse,sizeof(bufResponse),"FORBIDDEN"); 
					lenResponse=strlen(bufResponse);
				}
				else
				{
					if (cbRead!=12+DOMAIN_LEN+USER_LEN+PWD_LEN)
					{
						TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+DOMAIN_LEN+USER_LEN+PWD_LEN));
						strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
						lenResponse=strlen(bufResponse);
					}
					else
					{
						char szUserName[USER_LEN];
						memcpy(szUserName,bufRequest+12+DOMAIN_LEN,USER_LEN);
						CharUpper(szUserName);
						iUserDataIndex=swGetUserDataIndex(bufRequest,12);
						if (iUserDataIndex==-1) // utilisateur non connu, on le crée
						{
							char *pszSid=NULL;
							TRACE((TRACE_INFO,_F_,"Utilisateur %s pas encore connu, on le crée",szUserName));
							pszSid=swGetSID(szUserName);
							iUserDataIndex=giMaxUserDataIndex;
							memcpy(gUserData[iUserDataIndex].szLogonDomainName,bufRequest+12,DOMAIN_LEN);
							memcpy(gUserData[iUserDataIndex].szUserName,bufRequest+12+DOMAIN_LEN,USER_LEN);
							// ISSUE#247 : passage du username en majuscule pour éviter les pb de différences de casse (vu avec POA Sophos)
							CharUpper(gUserData[iUserDataIndex].szUserName);
							if (pszSid!=NULL) 
							{
								strcpy_s(gUserData[iUserDataIndex].szSID,SID_LEN,pszSid);
								LocalFree(pszSid);
							}
							TRACE((TRACE_INFO,_F_,"gUserData[%d].szLogonDomainName=%s",iUserDataIndex,gUserData[iUserDataIndex].szLogonDomainName));
							TRACE((TRACE_INFO,_F_,"gUserData[%d].szUserName=%s",iUserDataIndex,gUserData[iUserDataIndex].szUserName));
							TRACE((TRACE_INFO,_F_,"gUserData[%d].szSID=%s",iUserDataIndex,gUserData[iUserDataIndex].szSID));
							giMaxUserDataIndex++;
						}
						TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
						// extrait le mdp de la requête dans une variable temporaire effacée de manière sécurisée à la fin de la fonction
						memcpy(tmpBufPwd,bufRequest+12+DOMAIN_LEN+USER_LEN,PWD_LEN);
						// ISSUE#156 : on ne calcule plus le PKHD tout de suite, on le fait maintenant systématiquement quand on recoit un GETPKHD
						//             comme ça on est sûr d'être à jour à la fois du mot de passe et des sels. Et ça simplifie le code.
						// ISSUE#156 : on déchiffre le mot de passe reçu qui est chiffré avec CRYPTPROTECTMEMORY_CROSS_PROCESS
						//             pour le rechiffrer avec CRYPTPROTECTMEMORY_SAME_PROCESS
						if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_CROSS_PROCESS)!=0) goto end;
						//TRACE((TRACE_PWD,_F_,"tmpBufPwd=%s",tmpBufPwd));
						if (swProtectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
						// Vérifie qu'on n'a pas déjà ce mot de passe
						// ISSUE#173 : vérifie aussi bufPasswordOld
						if (memcmp(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN)==0 || memcmp(gUserData[iUserDataIndex].bufPasswordOld,tmpBufPwd,PWD_LEN)==0)
						{
							TRACE((TRACE_INFO,_F_,"ON A DEJA CE MOT DE PASSE, ON IGNORE"));
						}
						else
						{
							// Si on a déjà stocké un mot de passe, on copie la valeur précédente dans old
							if (gUserData[iUserDataIndex].bPasswordStored)
							{
								memcpy(gUserData[iUserDataIndex].bufPasswordOld,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
								gUserData[iUserDataIndex].bPasswordOldStored=TRUE;
							}
							// Stocke le nouveau mot de passe reçu
							memcpy(gUserData[iUserDataIndex].bufPassword,tmpBufPwd,PWD_LEN);
							gUserData[iUserDataIndex].bPasswordStored=TRUE;
							// Si swSSO est lancé, envoie un message pour signaler le nouveau mot de passe pour cet utilisateur
							// ISSUE#346 - sprintf_s(szEventName,"Global\\swsso-pwdchange-%s-%s",gUserData[iUserDataIndex].szLogonDomainName,gUserData[iUserDataIndex].szUserName);
							// ISSUE#360 - sprintf_s(szEventName,"Global\\swsso-pwdchange-%s",gUserData[iUserDataIndex].szUserName);
							sprintf_s(szEventName,"Global\\swsso-pwdchange-%s",szUserName);
							hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,szEventName);
							if (hEvent==NULL)
							{
								TRACE((TRACE_ERROR,_F_,"swSSO pas lancé (OpenEvent(%s)=%ld)",szEventName,GetLastError()));
							}
							else
							{
								TRACE((TRACE_DEBUG,_F_,"hEvent=0x%08lx",hEvent));
								brc=SetEvent(hEvent);
								TRACE((TRACE_DEBUG,_F_,"SetEvent=%d",brc));
								CloseHandle(hEvent);
								hEvent=NULL;
							}
						}
						// Réponse
						strcpy_s(bufResponse,sizeof(bufResponse),"OK");
						lenResponse=strlen(bufResponse);
					}
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"GETPHKD:",8)==0) // Format = V01:GETPHKD:CUR:domain(256octets)username(256octets) ou V01:GETPHKD:OLD:domain(256octets)username(256octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				// Vérifie que l'appelant est autorisé
				if (!IsCallingProcessAllowed(ulClientProcessId)) // nouveau en 1.11
				{
					strcpy_s(bufResponse,sizeof(bufResponse),"FORBIDDEN");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					if (memcmp(bufRequest+12,"CUR:",4)==0)
					{
						iUserDataIndex=swGetUserDataIndex(bufRequest,16);
						if (iUserDataIndex==-1) // utilisateur non connu, erreur !
						{
							TRACE((TRACE_ERROR,_F_,"Unknown user"));
							strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
							lenResponse=strlen(bufResponse);
						}
						else // Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
						{
							TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
							// ISSUE#156 : maintenant le calcul PKHD est fait ici et uniquement ici
							if (gUserData[iUserDataIndex].bPasswordStored)
							{
								memcpy(tmpBufPwd,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
								if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
								//TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPassword=%s",iUserDataIndex,tmpBufPwd));
								if (swPBKDF2((BYTE*)PBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
								if (swPBKDF2((BYTE*)AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							}
							memcpy(bufResponse,PBKDF2Pwd,PBKDF2_PWD_LEN);
							memcpy(bufResponse+PBKDF2_PWD_LEN,AESKeyData,AES256_KEY_LEN);
							lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
						}
					}
					else if (memcmp(bufRequest+12,"OLD:",4)==0)
					{
						iUserDataIndex=swGetUserDataIndex(bufRequest,16);
						if (iUserDataIndex==-1) // utilisateur non connu, erreur !
						{
							TRACE((TRACE_ERROR,_F_,"Unknown user"));
							strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
							lenResponse=strlen(bufResponse);
						}
						else // Prépare la réponse, format = PwdHash(32octets)KeyData(32octets)
						{
							TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
							// ISSUE#156 : maintenant le calcul PKHD est fait ici et uniquement ici
							if (gUserData[iUserDataIndex].bPasswordOldStored)
							{
								memcpy(tmpBufPwd,gUserData[iUserDataIndex].bufPasswordOld,PWD_LEN);
								if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
								//TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPasswordOld=%s",iUserDataIndex,tmpBufPwd));
								if (swPBKDF2((BYTE*)PBKDF2Pwd,PBKDF2_PWD_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
								if (swPBKDF2((BYTE*)AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
							}
							memcpy(bufResponse,PBKDF2Pwd,PBKDF2_PWD_LEN);
							memcpy(bufResponse+PBKDF2_PWD_LEN,AESKeyData,AES256_KEY_LEN);
							lenResponse=PBKDF2_PWD_LEN+AES256_KEY_LEN;
						}
					}
					else
					{
						TRACE((TRACE_ERROR,_F_,"Mot clé inconnu"));
						strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
						lenResponse=strlen(bufResponse);
					}
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"PUTPSKS:",8)==0) // V01:PUTPSKS:domain(256octets)username(256octets)PwdSalt(64octets)KeySalt(64octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				if (cbRead!=12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN*2)
				{
					TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+PBKDF2_SALT_LEN*2));
					strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					iUserDataIndex=swGetUserDataIndex(bufRequest,12);
					if (iUserDataIndex==-1) // utilisateur non connu, erreur !
					{
						TRACE((TRACE_ERROR,_F_,"Unknown user"));
						strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
						lenResponse=strlen(bufResponse);
					}
					else
					{
						memcpy(gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,bufRequest+12+DOMAIN_LEN+USER_LEN,PBKDF2_SALT_LEN);
						memcpy(gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,bufRequest+12+DOMAIN_LEN+USER_LEN+PBKDF2_SALT_LEN,PBKDF2_SALT_LEN);
						gUserData[iUserDataIndex].Salts.bPBKDF2PwdSaltReady=TRUE;
						gUserData[iUserDataIndex].Salts.bPBKDF2KeySaltReady=TRUE;
						TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].Salts.bufPBKDF2PwdSalt,PBKDF2_SALT_LEN,"gUserData[%d].Salts.bufPBKDF2PwdSalt",iUserDataIndex));
						TRACE_BUFFER((TRACE_DEBUG,_F_,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,"gUserData[%d].Salts.bufPBKDF2KeySalt",iUserDataIndex));
						// Réponse
						strcpy_s(bufResponse,sizeof(bufResponse),"OK");
						lenResponse=strlen(bufResponse);
					}	
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"GETPASS:",8)==0) // Format = V02:GETPASS:domain(256octets)username(256octets)
			//-------------------------------------------------------------------------------------------------------------
			{
				// Vérifie que l'appelant est autorisé
				if (!IsCallingProcessAllowed(ulClientProcessId)) // nouveau en 1.11
				{
					strcpy_s(bufResponse,sizeof(bufResponse),"FORBIDDEN");
					lenResponse=strlen(bufResponse);
				}
				else
				{
					if (cbRead!=12+DOMAIN_LEN+USER_LEN)
					{
						TRACE((TRACE_ERROR,_F_,"cbRead=%ld, attendu %d",cbRead,12+DOMAIN_LEN+USER_LEN));
						strcpy_s(bufResponse,sizeof(bufResponse),"BADFORMAT");
						lenResponse=strlen(bufResponse);
					}
					else
					{
						iUserDataIndex=swGetUserDataIndex(bufRequest,12);
						if (iUserDataIndex==-1) // utilisateur non connu, erreur !
						{
							TRACE((TRACE_ERROR,_F_,"Unknown user"));
							strcpy_s(bufResponse,sizeof(bufResponse),"UNKNOWN_USER");
							lenResponse=strlen(bufResponse);
						}
						else
						{
							TRACE((TRACE_INFO,_F_,"iUserDataIndex=%d",iUserDataIndex));
							if (!gUserData[iUserDataIndex].bPasswordStored)
							{
								TRACE((TRACE_ERROR,_F_,"Pas de mot de passe connu pour cet utilisateur",iUserDataIndex));
								strcpy_s(bufResponse,sizeof(bufResponse),"NO PASSWORD");
								lenResponse=strlen(bufResponse);
							}
							else // Prépare la réponse, format = ?
							{
								// Déchiffre le mot de passe
								memcpy(tmpBufPwd,gUserData[iUserDataIndex].bufPassword,PWD_LEN);
								if (swUnprotectMemory(tmpBufPwd,PWD_LEN,CRYPTPROTECTMEMORY_SAME_PROCESS)!=0) goto end;
								//TRACE((TRACE_PWD,_F_,"gUserData[%d].bufPassword=%s",iUserDataIndex,tmpBufPwd));
								// Crée la clé de chiffrement des mots de passe secondaires
								if (swPBKDF2((BYTE*)AESKeyData,AES256_KEY_LEN,tmpBufPwd,gUserData[iUserDataIndex].Salts.bufPBKDF2KeySalt,PBKDF2_SALT_LEN,10000)!=0) goto end;
								if (swCreateAESKeyFromKeyData(AESKeyData,&hKey)) goto end;
								// Chiffre le mot de passe
								pszEncryptedPwd=swCryptEncryptString(tmpBufPwd,hKey);
								if (pszEncryptedPwd==NULL)
								{
									TRACE((TRACE_ERROR,_F_,"Erreur lors du chiffrement du mot de passe"));
									strcpy_s(bufResponse,sizeof(bufResponse),"PASSWORD ENCRYPT ERROR");
									lenResponse=strlen(bufResponse);
								}
								else
								{
									// Réponse
									lenResponse=strlen(pszEncryptedPwd);
									memcpy(bufResponse,pszEncryptedPwd,lenResponse);
								}
							}
						}
					}
				}
			}
			//-------------------------------------------------------------------------------------------------------------
			else if (memcmp(bufRequest+4,"NEWVERS",7)==0) // Format = V02:NEWVERS
			//-------------------------------------------------------------------------------------------------------------
			{
				strcpy_s(bufResponse,sizeof(bufResponse),gbNewVersion?"YES":"NO");
				gbNewVersion=FALSE;
				lenResponse=strlen(bufResponse);
			}
			//-------------------------------------------------------------------------------------------------------------
			else
			//-------------------------------------------------------------------------------------------------------------
			{
				TRACE((TRACE_ERROR,_F_,"Mot clé inconnu"));
				strcpy_s(bufResponse,sizeof(bufResponse),"BADREQUEST");
				lenResponse=strlen(bufResponse);
			}
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"Version non supportée !"));
			strcpy_s(bufResponse,sizeof(bufResponse),"BADVERSION");
			lenResponse=strlen(bufResponse);
		}
	}
	// Envoie la réponse
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)bufResponse,lenResponse,"Response"));
	if (!WriteFile(ghPipe,bufResponse,lenResponse,&cbWritten,NULL))
	{
		TRACE((TRACE_ERROR,_F_,"WriteFile(%d)=%d",lenResponse,GetLastError()));
		goto end;
	}  

	FlushFileBuffers(ghPipe);
	DisconnectNamedPipe(ghPipe); 
	rc=0;
end:
	swCryptDestroyKey(hKey);
	if (pszEncryptedPwd!=NULL) free(pszEncryptedPwd);
	// Efface la requête de manière sécurisée (elle peut contenir un mot de passe !)
	SecureZeroMemory(bufRequest,sizeof(bufRequest));
	SecureZeroMemory(tmpBufPwd,sizeof(tmpBufPwd));
	SecureZeroMemory(PBKDF2Pwd,sizeof(PBKDF2Pwd));
	SecureZeroMemory(AESKeyData,sizeof(AESKeyData));
	TRACE((TRACE_LEAVE,_F_,"rc=0x%08lx",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// swDestroyPipe()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swDestroyPipe(void)
{
	TRACE((TRACE_ENTER,_F_,""));	
	if (ghPipe!=INVALID_HANDLE_VALUE) CloseHandle(ghPipe);
	TRACE((TRACE_LEAVE,_F_,""));
}
