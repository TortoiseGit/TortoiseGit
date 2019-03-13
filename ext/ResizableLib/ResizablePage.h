#if !defined(AFX_RESIZABLEPAGE_H__INCLUDED_)
#define AFX_RESIZABLEPAGE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ResizablePage.h : header file
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

#include "ResizableLayout.h"
#include "ResizableMinMax.h"

/////////////////////////////////////////////////////////////////////////////
// CResizablePage window

class CResizablePage : public CPropertyPage, public CResizableLayout,
						public CResizableMinMax
{
	DECLARE_DYNCREATE(CResizablePage)

// Construction
public:
	CResizablePage();
	CResizablePage(UINT nIDTemplate, UINT nIDCaption = 0);
	CResizablePage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizablePage)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResizablePage();

// callable from derived classes
protected:

	virtual CWnd* GetResizableWnd() const override
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};

// Generated message map functions
protected:
	//{{AFX_MSG(CResizablePage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLEPAGE_H__INCLUDED_)
