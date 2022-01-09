// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>

#define NOMINMAX
#include <algorithm>
using std::max;
using std::min;

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string>

#include <windows.h>
#include <Commdlg.h>
#if (defined(_M_IX86) || defined(_M_X64))
#include <emmintrin.h>
#elif defined(_M_ARM64)
//#include ".\sse2neon\emmintrin.h"
#elif
/#   error Unsupported architecture
#endif


#define COMMITMONITOR_FINDMSGPREV		(WM_APP+1)
#define COMMITMONITOR_FINDMSGNEXT		(WM_APP+2)
#define COMMITMONITOR_FINDEXIT			(WM_APP+3)
#define COMMITMONITOR_FINDRESET			(WM_APP+4)

#define REGSTRING_DARKTHEME L"Software\\TortoiseGit\\UDiffDarkTheme"

#include "SmartHandle.h"
#include "scope_exit_noexcept.h"

#ifdef _WIN64
#   define APP_X64_STRING "x64"
#else
#   define APP_X64_STRING ""
#endif
