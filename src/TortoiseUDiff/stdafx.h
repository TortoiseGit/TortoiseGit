// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "../targetver.h"

#define NOMINMAX
#include <algorithm>
using std::max;
using std::min;

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <windows.h>
#include <Commdlg.h>
#include <emmintrin.h>

#define COMMITMONITOR_FINDMSGPREV		(WM_APP+1)
#define COMMITMONITOR_FINDMSGNEXT		(WM_APP+2)
#define COMMITMONITOR_FINDEXIT			(WM_APP+3)
#define COMMITMONITOR_FINDRESET			(WM_APP+4)

#ifdef _WIN64
#   define APP_X64_STRING "x64"
#else
#   define APP_X64_STRING ""
#endif
