// stdafx.h : include file for standard system include files, or project
// specific include files that are used frequently, but are changed infrequently
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

#if !defined(AFX_RESIZABLESTDAFX_H__INCLUDED_)
#define AFX_RESIZABLESTDAFX_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Set max target Windows platform
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

// Use target Common Controls version for compatibility
// with CPropertyPageEx, CPropertySheetEx
#define _WIN32_IE 0x0500

// let us be spared from a flood of deprecation warnings.
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_DEPRECATE 1
#define _SECURE_SCL_DEPRECATE 0
#define _HAS_ITERATOR_DEBUGGING 0

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#ifndef WS_EX_LAYOUTRTL
#pragma message("Please update your Windows header files, get the latest SDK")
#pragma message("WinUser.h is out of date!")

#define WS_EX_LAYOUTRTL		0x00400000
#endif

#ifndef WC_BUTTON
#pragma message("Please update your Windows header files, get the latest SDK")
#pragma message("CommCtrl.h is out of date!")

#define WC_BUTTON			TEXT("Button")
#define WC_STATIC			TEXT("Static")
#define WC_EDIT				TEXT("Edit")
#define WC_LISTBOX			TEXT("ListBox")
#define WC_COMBOBOX			TEXT("ComboBox")
#define WC_SCROLLBAR		TEXT("ScrollBar")
#endif

#define RSZLIB_NO_XP_DOUBLE_BUFFER

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLESTDAFX_H__INCLUDED_)
