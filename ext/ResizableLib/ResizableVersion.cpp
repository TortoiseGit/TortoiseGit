// ResizableVersion.cpp: implementation of the CResizableVersion class.
//
/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004 by Paolo Messina
// http://www.geocities.com/ppescher - mailto:ppescher@hotmail.com
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizableVersion.h"

//////////////////////////////////////////////////////////////////////
// Static initializer object (with macros to hide in ClassView)

// static intializer must be called before user code
#pragma warning(disable:4073)
#pragma init_seg(lib)

#ifdef _UNDEFINED_
#define BEGIN_HIDDEN {
#define END_HIDDEN }
#else
#define BEGIN_HIDDEN
#define END_HIDDEN
#endif

BEGIN_HIDDEN
struct _VersionInitializer
{
	_VersionInitializer()
	{
		InitRealVersions();
	};
};
END_HIDDEN

// The one and only version-check object
static _VersionInitializer g_version;

//////////////////////////////////////////////////////////////////////
// Private implementation

// DLL Version support
#include <shlwapi.h>

static DLLVERSIONINFO g_dviCommCtrls;

static void CheckCommCtrlsVersion()
{
	// Check Common Controls version
	SecureZeroMemory(&g_dviCommCtrls, sizeof(DLLVERSIONINFO));
	HMODULE hMod = AtlLoadSystemLibraryUsingFullPath(_T("comctl32.dll"));
	if (hMod != NULL)
	{
		// Get the version function
		DLLGETVERSIONPROC pfnDllGetVersion;
		pfnDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hMod, "DllGetVersion");

		if (pfnDllGetVersion != NULL)
		{
			// Obtain version information
			g_dviCommCtrls.cbSize = sizeof(DLLVERSIONINFO);
			if (SUCCEEDED(pfnDllGetVersion(&g_dviCommCtrls)))
			{
				::FreeLibrary(hMod);
				return;
			}
		}

		::FreeLibrary(hMod);
	}

	// Set values for the worst case
	g_dviCommCtrls.dwMajorVersion = 4;
	g_dviCommCtrls.dwMinorVersion = 0;
	g_dviCommCtrls.dwBuildNumber = 0;
	g_dviCommCtrls.dwPlatformID = DLLVER_PLATFORM_WINDOWS;
}


//////////////////////////////////////////////////////////////////////
// Exported global symbols

#ifdef _WIN32_IE
DWORD real_WIN32_IE = 0;
#endif

// macro to convert version numbers to hex format
#define CNV_OS_VER(x) ((BYTE)(((BYTE)(x) / 10 * 16) | ((BYTE)(x) % 10)))

void InitRealVersions()
{
	CheckCommCtrlsVersion();

#ifdef _WIN32_IE
	switch (g_dviCommCtrls.dwMajorVersion)
	{
	case 4:
		switch (g_dviCommCtrls.dwMinorVersion)
		{
		case 70:
			real_WIN32_IE = 0x0300;
			break;
		case 71:
			real_WIN32_IE = 0x0400;
			break;
		case 72:
			real_WIN32_IE = 0x0401;
			break;
		default:
			real_WIN32_IE = 0x0200;
		}
		break;
	case 5:
		if (g_dviCommCtrls.dwMinorVersion > 80)
			real_WIN32_IE = 0x0501;
		else
			real_WIN32_IE = 0x0500;
		break;
	case 6:
		real_WIN32_IE = 0x0600;	// includes checks for 0x0560 (IE6)
		break;
	default:
		real_WIN32_IE = 0;
	}
#endif
}
