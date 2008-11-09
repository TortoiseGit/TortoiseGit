#if !defined(AFX_RESIZABLECOMBOBOX_H__CAEDF06D_C4F5_49E4_A783_AD7D894CF9A0__INCLUDED_)
#define AFX_RESIZABLECOMBOBOX_H__CAEDF06D_C4F5_49E4_A783_AD7D894CF9A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ResizableComboBox.h : header file
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2004 by Paolo Messina
// (http://www.geocities.com/ppescher - ppescher@hotmail.com)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "ResizableComboLBox.h"

/////////////////////////////////////////////////////////////////////////////
// CResizableComboBox window

class CResizableComboBox : public CComboBox
{
	friend class CResizableComboLBox;

// Construction
public:
	CResizableComboBox();

// Attributes
public:

protected:
	CResizableComboLBox m_ctrlListBox;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableComboBox)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResizableComboBox();

protected:
	RECT m_rectDropDown;
	BOOL m_bClipMaxHeight;
	BOOL m_bIntegralHeight;
	int m_iExtent;

	void InitHorizontalExtent();
	void UpdateHorizontalExtent(LPCTSTR szText);
	int MakeIntegralHeight(const int height);

// Generated message map functions
protected:
	//{{AFX_MSG(CResizableComboBox)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLECOMBOBOX_H__CAEDF06D_C4F5_49E4_A783_AD7D894CF9A0__INCLUDED_)
