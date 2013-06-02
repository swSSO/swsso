
#pragma once

#include "targetver.h"
#include <windows.h>
#include <Sddl.h>
#include <stdio.h>
#include <tchar.h>
#include "swSSOTrace.h"
#include "swSSOCrypt.h"
#include "swSSOProtectMemory.h"

// swSSOSVCInstall
bool IsInstalled(void);
int Install(void);
int Uninstall(void);

// swSSOSVCData
void swInitData(void);
int swWaitForMessage(void);
int swCreatePipe(void);
void swDestroyPipe(void);
