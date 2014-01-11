//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2014 - Sylvain WERDEFROY
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
#define BUF_SIZE 1024

char Response[BUF_SIZE];
char Dump[BUF_SIZE*2];

void DoRequest(char *Request,int lenRequest)
{
	HANDLE hPipe=INVALID_HANDLE_VALUE;
	 DWORD cbRead,cbWritten;
	 int len;
	 int iBinOffset,iCharOffset;
	hPipe = CreateFile("\\\\.\\pipe\\swsso",GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
	if (hPipe==INVALID_HANDLE_VALUE)
	{
		printf("CreateNamedPipe()=%d\n",GetLastError());
		goto end;
	}
	
   
	if (!WriteFile(hPipe,Request,lenRequest,&cbWritten,NULL))
	{
		printf("WriteFile(%d)=%d\n",lenRequest,GetLastError());
		goto end;
	} 
	if (!ReadFile(hPipe,Response,sizeof(Response),&cbRead,NULL))
	{
		printf("ReadFile(%d)=%d\n",sizeof(Response),GetLastError());
		goto end;
	}  
	
		// préformatage
	strcpy_s(Dump,sizeof(Dump),"                                                |                 \r\n");
	len=strlen(Dump);
	// trace le contenu du buffer
	iBinOffset=0;
	iCharOffset=50;
	for (DWORD i=0;i<cbRead;i++)
	{
		if (i!=0 && (i%16)==0)
		{ 
			printf(Dump);
			iBinOffset=0;
			iCharOffset=50;
			memset(Dump,' ',len-2);
		}
		wsprintf(Dump+iBinOffset,"%02x",(unsigned char)(Response[i]));
		iBinOffset+=2;
		Dump[iBinOffset]=' '; // a été écrasé par le 0 du wsprintf
		if (Response[i]>=32 && Response[i]<=127)
			Dump[iCharOffset]=Response[i];
		else
			Dump[iCharOffset]='.';
		iBinOffset++;
		iCharOffset++;
	}
	printf(Dump);
end:
	if (hPipe!=INVALID_HANDLE_VALUE) CloseHandle(hPipe); 
}

int main(int argc, _TCHAR* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	char szCmd[256];
	int iCmd;
	char bufPassword[256];
	char Request[1024];
	//swProtectMemoryInit();
again:
	printf("1->V01:PUTPASS:password1\n");
	printf("2->V01:PUTPASS:password2\n");
	printf("3->V01:GETPHKD:CUR\n");
	printf("4->V01:GETPHKD:OLD\n");
	printf("5->V01:PUTPSKS:5*64(PwdSalt)+6*64(KeySalt)\n");
	printf("6->V01:GETPSKS:\n");
	printf("7->SetEvent(swsso-pwdchange)\n");
	printf("9->END!\n");
	gets_s(szCmd,sizeof(szCmd));
	iCmd=atoi(szCmd);
	switch (iCmd)
	{
		case 1:
			strcpy_s(bufPassword,sizeof(bufPassword),"password1");
			//swProtectMemory(bufPassword,sizeof(bufPassword),CRYPTPROTECTMEMORY_CROSS_PROCESS);
			memcpy(Request,"V01:PUTPASS:",12);
			memcpy(Request+12,bufPassword,256);
			DoRequest(Request,12+256);
			break;
		case 2:
			strcpy_s(bufPassword,sizeof(bufPassword),"password2");
			//swProtectMemory(bufPassword,sizeof(bufPassword),CRYPTPROTECTMEMORY_CROSS_PROCESS);
			memcpy(Request,"V01:PUTPASS:",12);
			memcpy(Request+12,bufPassword,256);
			DoRequest(Request,12+256);
			break;
		case 3:
			DoRequest("V01:GETPHKD:CUR",strlen("V01:GETPHKD:CUR"));
			break;
		case 4:
			DoRequest("V01:GETPHKD:OLD",strlen("V01:GETPHKD:OLD"));
			break;
		case 5:
			DoRequest("V01:PUTPSKS:55555555555555555555555555555555555555555555555555555555555555556666666666666666666666666666666666666666666666666666666666666666",
			   strlen("V01:PUTPSKS:55555555555555555555555555555555555555555555555555555555555555556666666666666666666666666666666666666666666666666666666666666666"));
			break;
		case 6:
			DoRequest("V01:GETPSKS:",strlen("V01:GETPSKS:"));
			break;
		case 7:
			{
				HANDLE hEvent=NULL;
				hEvent=OpenEvent(EVENT_MODIFY_STATE,FALSE,"swsso-pwdchange");
				printf("hEvent=0x%08lx\n",hEvent);
				if (hEvent!=NULL)
				{
					BOOL brc=SetEvent(hEvent);
					printf("SetEvent=%d\n",brc);
				}
				CloseHandle(hEvent);
			}
			break;
		case 9:
			goto end;
			break;
	}
	goto again;
end:
	//swProtectMemoryTerm();
	return 0;
}

