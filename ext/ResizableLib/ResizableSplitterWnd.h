// ResizableSplitterWnd.h : header file
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

#if !defined(AFX_RESIZABLESPLITTERWND_H__INCLUDED_)
#define AFX_RESIZABLESPLITTERWND_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizableMinMax.h"

/////////////////////////////////////////////////////////////////////////////
// CResizableSplitterWnd frame with splitter

class CResizableSplitterWnd : public CSplitterWnd, public CResizableMinMax
{
	DECLARE_DYNAMIC(CResizableSplitterWnd)

// Construction
public:
	CResizableSplitterWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableSplitterWnd)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResizableSplitterWnd();

	// Generated message map functions
	//{{AFX_MSG(CResizableSplitterWnd)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLESPLITTERWND_H__INCLUDED_)
