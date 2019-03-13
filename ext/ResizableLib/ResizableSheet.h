#if !defined(AFX_RESIZABLESHEET_H__INCLUDED_)
#define AFX_RESIZABLESHEET_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
#include "ResizableGrip.h"
#include "ResizableMinMax.h"
#include "ResizableSheetState.h"

/////////////////////////////////////////////////////////////////////////////
// ResizableSheet.h : header file
//

class CResizableSheet : public CPropertySheet, public CResizableLayout,
						public CResizableGrip, public CResizableMinMax,
						public CResizableSheetState
{
	DECLARE_DYNAMIC(CResizableSheet)

// Construction
public:
	CResizableSheet();
	CResizableSheet(UINT nIDCaption, CWnd *pParentWnd = NULL, UINT iSelectPage = 0);
	CResizableSheet(LPCTSTR pszCaption, CWnd *pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
private:
	// support for temporarily hiding the grip
	DWORD m_dwGripTempState;

	// flags
	BOOL m_bEnableSaveRestore;
	BOOL m_bRectOnly;
	BOOL m_bSavePage;

	// layout vars
	UINT_PTR m_nCallbackID;
	CSize m_sizePageTL, m_sizePageBR;
	BOOL m_bLayoutDone;

	// internal status
	CString m_sSection;			// section name (identifies a parent window)

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableSheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
	protected:

// Implementation
public:
	virtual ~CResizableSheet();

// used internally
private:
	void PrivateConstruct();

	BOOL IsWizard() const;

// callable from derived classes
protected:
	void PresetLayout();
	void RefreshLayout();

	// section to use in app's profile
	void EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly = FALSE,
		BOOL bWithPage = FALSE, BOOL bHorzResize = TRUE, BOOL bVertResize = TRUE);
	int GetMinWidth();	// minimum width to display all buttons


	virtual CWnd* GetResizableWnd() const override
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};

// Generated message map functions
protected:
	virtual BOOL CalcSizeExtra(HWND hWndChild, CSize sizeChild, CSize& sizeExtra) override;
	virtual BOOL ArrangeLayoutCallback(LAYOUTINFO& layout) const override;
	//{{AFX_MSG(CResizableSheet)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg BOOL OnPageChanging(NMHDR* pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif	// AFX_RESIZABLESHEET_H__INCLUDED_
