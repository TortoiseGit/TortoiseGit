// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#define XMESSAGEBOX_APPREGPATH "Software\\TortoiseGit\\"

#include "../../src\targetver.h"

#define NOMINMAX
#include <algorithm>
using std::min;
using std::max;

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <WinInet.h>

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <afxctl.h>
#include <afxtempl.h>
#include <afxmt.h>
#include <afxext.h>         // MFC extensions
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#include <atlbase.h>

#include "git2.h"
#include "SmartLibgit2Ref.h"

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <functional>

#define __WIN32__

#include "scope_exit_noexcept.h"
#include "DebugOutput.h"

#include "SmartHandle.h"

// Header for gtest
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "AutoTempDir.h"
