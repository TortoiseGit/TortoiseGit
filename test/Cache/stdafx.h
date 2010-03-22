// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _WIN32_IE 0x600

#ifdef UNICODE
#	ifndef WINVER
#		define WINVER 0x0501
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0501
#	endif						
#	ifndef _WIN32_WINDOWS
#		define _WIN32_WINDOWS 0x0501
#	endif
#else
#	ifndef WINVER
#		define WINVER 0x0410
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0500
#	endif						
#	ifndef _WIN32_WINDOWS
#		define _WIN32_WINDOWS 0x0410
#	endif
#endif

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

// TODO: reference additional headers your program requires here

#define CSTRING_AVAILABLE


using namespace ATL;

#pragma warning(push)
#pragma warning(disable: 4702)	// Unreachable code warnings in xtree
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>
#pragma warning(pop)

#include "svn_wc.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_pools.h"


typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;
