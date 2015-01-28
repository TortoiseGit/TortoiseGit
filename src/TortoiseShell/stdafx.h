// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "..\targetver.h"

#define ISOLATION_AWARE_ENABLED 1

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <windows.h>

#include <commctrl.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <wininet.h>
#include <Aclapi.h>

#include <atlbase.h>
#include <atlexcept.h>
#include <atlstr.h>

#include <string>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>

#define CSTRING_AVAILABLE

// TODO add integrity stuff here!

#include "SysInfo.h"
#include "DebugOutput.h"

