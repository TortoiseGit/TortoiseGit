// ResizableMsgSupport.cpp: support messages for custom resizable wnds
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2002 by Paolo Messina
// (http://www.geocities.com/ppescher - ppescher@yahoo.com)
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
#include "ResizableMsgSupport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Registered message to communicate with the library

// static intializer must be called before user code
#pragma warning(disable:4073)
#pragma init_seg(lib)

const UINT WMU_RESIZESUPPORT = ::RegisterWindowMessage(TEXT("WMU_RESIZESUPPORT"));

