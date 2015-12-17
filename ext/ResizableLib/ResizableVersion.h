// ResizableVersion.h: interface for the CResizableVersion class.
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

#if !defined(AFX_RESIZABLEVERSION_H__INCLUDED_)
#define AFX_RESIZABLEVERSION_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// global variables that hold actual version numbers
// retrieved and adapted at run-time to be equivalent
// to preprocessor macros that set the target platform

#ifdef _WIN32_IE
extern DWORD real_WIN32_IE;
#endif

// called automatically by a static initializer
// (if not appropriate can be called later)
// to setup global version numbers

void InitRealVersions();


#endif // !defined(AFX_RESIZABLEVERSION_H__INCLUDED_)
