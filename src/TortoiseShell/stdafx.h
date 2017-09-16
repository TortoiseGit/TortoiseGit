// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "../targetver.h"

#define ISOLATION_AWARE_ENABLED 1

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <windows.h>

#include <commctrl.h>
#include <ShlObj.h>
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
#include <algorithm>
#include <functional>

#define CSTRING_AVAILABLE

#pragma warning(push)
#pragma warning(disable: 4510 4610)
#include "git2.h"
#pragma warning(pop)
#include "SmartLibgit2Ref.h"

#include "scope_exit_noexcept.h"
#include "SysInfo.h"
#include "DebugOutput.h"

