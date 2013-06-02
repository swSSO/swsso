// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(ul_reason_for_call);
	UNREFERENCED_PARAMETER(lpReserved);

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			TRACE_OPEN();
			TRACE((TRACE_ENTER,_F_,"ATTACH"));
			
			TRACE((TRACE_LEAVE,_F_,"ATTACH"));
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			TRACE((TRACE_ENTER,_F_,"DETACH"));
			
			TRACE((TRACE_LEAVE,_F_,"DETACH"));
			TRACE_CLOSE();
			break;
	}
	return TRUE;
}

