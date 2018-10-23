// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "../targetver.h"

#define NOMINMAX
#include <algorithm>
using std::min;
using std::max;

#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define CSTRING_AVAILABLE

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <windows.h>

#include <ShlObj.h>
#include <Shlwapi.h>

#include <atlbase.h>
#include <atlstr.h>

#include <conio.h>

using namespace ATL;

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <functional>

#include "git2.h"
#include "SmartLibgit2Ref.h"

#include "scope_exit_noexcept.h"
#include "DebugOutput.h"

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;

#ifdef _WIN64
#	define APP_X64_STRING	"x64"
#else
#	define APP_X64_STRING ""
#endif
