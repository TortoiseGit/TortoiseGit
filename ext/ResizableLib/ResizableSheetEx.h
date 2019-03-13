#if !defined(AFX_RESIZABLESHEETEX_H__INCLUDED_)
#define AFX_RESIZABLESHEETEX_H__INCLUDED_

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
#include <afxpriv.h>

/////////////////////////////////////////////////////////////////////////////
// ResizableSheetEx.h : header file
//

class CResizableSheetEx : public CPropertySheetEx, public CResizableLayout,
						public CResizableGrip, public CResizableMinMax,
						public CResizableSheetState
{
	DECLARE_DYNAMIC(CResizableSheetEx)

// Construction
public:
	CResizableSheetEx();
	CResizableSheetEx(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0,
		HBITMAP hbmWatermark = NULL, HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);
	CResizableSheetEx(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0,
		HBITMAP hbmWatermark = NULL, HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);

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
	enum
	{
		PSH_IE4WIZARD97 = 0x00002000,
		PSH_IE5WIZARD97 = 0x01000000,
	};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableSheetEx)
	public:
	virtual BOOL OnInitDialog();
	INT_PTR DoModal();
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResizableSheetEx();

// used internally
private:
	void PrivateConstruct();

	BOOL IsWizard() const;
	BOOL IsWizard97() const;

// callable from derived classes
protected:
	void RefreshLayout();
	void PresetLayout();

	// section to use in app's profile
	void EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly = FALSE,
		BOOL bWithPage = FALSE, BOOL bHorzResize = TRUE, BOOL bVertResize = TRUE);
	int GetMinWidth();	// minimum width to display all buttons

	void BuildPropPageArray() override
	{
		CPropertySheet::BuildPropPageArray();

		// create a copy of existing PROPSHEETPAGE array which can be modified
		int nPages = static_cast<int>(m_pages.GetSize());
		int nBytes = 0;
		for (decltype(nPages) i = 0; i < nPages; ++i)
		{
			auto pPage = GetPage(i);
			nBytes += pPage->m_psp.dwSize;
		}
		auto ppsp0 = static_cast<LPPROPSHEETPAGE>(malloc(nBytes));
		Checked::memcpy_s(ppsp0, nBytes, m_psh.ppsp, nBytes);
		auto ppsp = ppsp0;
		for (decltype(nPages) i = 0; i < nPages; ++i)
		{
			const DLGTEMPLATE* pResource = ppsp->pResource;
			CDialogTemplate dlgTemplate(pResource);
			dlgTemplate.SetFont(L"MS Shell Dlg 2", 9);
			HGLOBAL hNew = GlobalAlloc(GPTR, dlgTemplate.m_dwTemplateSize);
			ppsp->pResource = (DLGTEMPLATE*)GlobalLock(hNew);
			Checked::memcpy_s((void*)ppsp->pResource, dlgTemplate.m_dwTemplateSize, dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
			GlobalUnlock(hNew);
			(BYTE*&)ppsp += ppsp->dwSize;
		}
		// free existing PROPSHEETPAGE array and assign the new one
		free((void*)m_psh.ppsp);
		m_psh.ppsp = ppsp0;
	}

	virtual CWnd* GetResizableWnd() const override
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};
	static int CALLBACK XmnPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam);

// Generated message map functions
protected:
	void GetHeaderRect(LPRECT lpRect);
	virtual BOOL CalcSizeExtra(HWND hWndChild, CSize sizeChild, CSize& sizeExtra) override;
	virtual BOOL ArrangeLayoutCallback(LAYOUTINFO& layout) const override;
	//{{AFX_MSG(CResizableSheetEx)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg BOOL OnPageChanging(NMHDR* pNotifyStruct, LRESULT* pResult);
	afx_msg LRESULT OnResizeSupport(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif	// AFX_RESIZABLESHEETEX_H__INCLUDED_
