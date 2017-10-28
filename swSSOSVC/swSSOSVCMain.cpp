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
SERVICE_STATUS_HANDLE ghServiceStatus=NULL;
SERVICE_STATUS gServiceStatus;
DWORD gdwThreadID;
HANDLE ghSvcStopEvent=NULL;

//-----------------------------------------------------------------------------
// ServiceCtrl()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void WINAPI ServiceCtrl(DWORD dwOpcode)
{
    TRACE((TRACE_ENTER,_F_,"dwOpcode=%ld",dwOpcode));
	switch (dwOpcode)
    {
		case SERVICE_CONTROL_STOP:
			TRACE((TRACE_INFO,_F_,"SERVICE_CONTROL_STOP"));
			gServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(ghServiceStatus, &gServiceStatus);
			SetEvent(ghSvcStopEvent);
			break;
		default:
			TRACE((TRACE_ERROR,_F_,"dwOpCode non géré (%ld)",dwOpcode));
			;
    }
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// WaitThread()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
DWORD WINAPI WaitThread(LPVOID lpvParam)
{
	UNREFERENCED_PARAMETER(lpvParam);
	TRACE((TRACE_ENTER,_F_,""));
	
	swWaitForMessage();
	
	TRACE((TRACE_LEAVE,_F_,""));
	return 0;
}

//-----------------------------------------------------------------------------
// ServiceMain()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void WINAPI ServiceMain()
{
    TRACE((TRACE_ENTER,_F_,""));
	HANDLE hThread;
	DWORD dwrc;
	HANDLE handles[2];

	ghServiceStatus = RegisterServiceCtrlHandler("swssosvc", ServiceCtrl);
    if (ghServiceStatus == NULL)
    {
        TRACE((TRACE_ERROR,_F_,"RegisterServiceCtrlHandler()=%d",GetLastError()));
        goto end;
    }
	gServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	SetServiceStatus(ghServiceStatus, &gServiceStatus);

	ghSvcStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
                        
	swServiceInit();
	if (swCryptInit()!=0) goto end;
	if (swCreatePipe()!=0) goto end;
	if (swProtectMemoryInit()!=0) goto end;

	gServiceStatus.dwWin32ExitCode = S_OK;
    gServiceStatus.dwCheckPoint = 0;
    gServiceStatus.dwWaitHint = 0;
	gServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(ghServiceStatus, &gServiceStatus);	

	hThread=CreateThread(NULL,0,WaitThread,0,0,NULL);
	if (hThread==NULL) { TRACE((TRACE_ERROR,_F_,"CreateThread()=%ld",GetLastError())); goto end; }

#pragma warning( disable : 4127)
	
	while(1)
    {
		handles[0]=ghSvcStopEvent;
		handles[1]=hThread;
		dwrc=WaitForMultipleObjects(2,handles,FALSE,INFINITE);
		if (dwrc==WAIT_OBJECT_0) // StopEvent
		{
			TRACE((TRACE_INFO,_F_,"WaitForMultipleObjects() --> StopEvent !"));
			gServiceStatus.dwCurrentState = SERVICE_STOPPED;
			// A NE PAS FAIRE ICI MAIS TOUT A LA FIN, VOIR COMMENTAIRE 
			// SetServiceStatus(ghServiceStatus, &gServiceStatus);
			goto end;
		}
		else if (dwrc==WAIT_OBJECT_0+1) // Thread
		{
			TRACE((TRACE_INFO,_F_,"WaitForMultipleObjects() --> Thread !"));
			CloseHandle(hThread); hThread=NULL;
			hThread=CreateThread(NULL,0,WaitThread,0,0,NULL);
			if (hThread==NULL) { TRACE((TRACE_ERROR,_F_,"CreateThread()=%ld",GetLastError())); goto end; }
		}
		else
		{
			TRACE((TRACE_ERROR,_F_,"WaitForMultipleObjects()=%ld",dwrc));
			goto end;
		}
		Sleep(1000); // pas la peine de prendre toutes les ressources s'il y a un blème...
    }
end:
	swProtectMemoryTerm();
	swServiceTerm();
	swCryptTerm();
	if (ghSvcStopEvent!=NULL) CloseHandle(ghSvcStopEvent);
	TRACE((TRACE_LEAVE,_F_,""));
	// A FAIRE TOUT TOUT A LA FIN CAR : 
	// "Do not attempt to perform any additional work after calling SetServiceStatus with SERVICE_STOPPED, because the service process can be terminated at any time"
	if (gServiceStatus.dwCurrentState==SERVICE_STOPPED) SetServiceStatus(ghServiceStatus, &gServiceStatus);
}

//-----------------------------------------------------------------------------
// Usage()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void Usage(void)
{
	TRACE((TRACE_ENTER,_F_,""));
	printf("Usage : swssosvc.exe install | uninstall\n");
	TRACE((TRACE_LEAVE,_F_,""));
}

//-----------------------------------------------------------------------------
// main()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int main(int argc, _TCHAR* argv[])
{
	int rc=0;
	
	TRACE_OPEN();
	TRACE((TRACE_ENTER,_F_,""));
	SERVICE_TABLE_ENTRY st[] =
    {
        { "swssosvc", (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };
	gdwThreadID = GetCurrentThreadId();
	ghServiceStatus = NULL;
    gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwCurrentState = SERVICE_STOPPED;
    gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    gServiceStatus.dwWin32ExitCode = 0;
    gServiceStatus.dwServiceSpecificExitCode = 0;
    gServiceStatus.dwCheckPoint = 0;
    gServiceStatus.dwWaitHint = 0;
	if (argc==2)
	{
		if (strcmp(argv[1],"install")==0)
		{
			if (IsInstalled()) { printf ("Already installed\n"); goto end; }
			rc=Install();
			if (rc==0)
				printf("Service installed successfully\n"); 
			else
				printf("Service installation failed\n");
		}
		else if (strcmp(argv[1],"uninstall")==0)
		{
			if (!IsInstalled()) { printf ("Not installed\n"); goto end; }
			rc=Uninstall();
			if (rc==0)
				printf("Service uninstalled successfully\n"); 
			else
				printf("Service installation failed\n");
		}
		else
			{ Usage(); rc=-1; goto end; }
	}
	else if (argc==1)
	{ 
		StartServiceCtrlDispatcher(st);
	}
	else 
		{ Usage(); rc=-1; goto end; }
end:
	TRACE((TRACE_LEAVE,_F_,""));
	TRACE_CLOSE();
	return rc;
}

