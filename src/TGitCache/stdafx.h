// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define CSTRING_AVAILABLE

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <windows.h>

#include <Shlobj.h>
#include <Shlwapi.h>

#include <atlbase.h>
#include <atlstr.h>

#include <conio.h>

using namespace ATL;

#pragma warning(push)
#pragma warning(disable: 4702)	// Unreachable code warnings in xtree
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>
#include <functional>
#pragma warning(pop)

#pragma warning(push)
#include "git2.h"
#pragma warning(pop)

#include "DebugOutput.h"

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;

#ifdef _WIN64
#	define APP_X64_STRING	"x64"
#else
#	define APP_X64_STRING ""
#endif
