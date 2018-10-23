// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "../../targetver.h"

#define NOMINMAX
#include <algorithm>
using std::min;
using std::max;

#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <afxctl.h>
#include <afxtempl.h>
#include <afxmt.h>

#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>

#include <conio.h>

#define CSTRING_AVAILABLE

using namespace ATL;

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <functional>

#include "git2.h"
#include "SmartLibgit2Ref.h"

#include "scope_exit_noexcept.h"

#include "DebugOutput.h"

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;
