// ResizableMDIChild.cpp : implementation file
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
#include "ResizableMDIChild.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableMDIChild

IMPLEMENT_DYNCREATE(CResizableMDIChild, CMDIChildWnd)

CResizableMDIChild::CResizableMDIChild()
{
	m_bEnableSaveRestore = FALSE;
	m_bRectOnly = FALSE;
}

CResizableMDIChild::~CResizableMDIChild()
{
}


BEGIN_MESSAGE_MAP(CResizableMDIChild, CMDIChildWnd)
	//{{AFX_MSG_MAP(CResizableMDIChild)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableMDIChild message handlers

void CResizableMDIChild::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	// MDI should call default implementation
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);

	MinMaxInfo(lpMMI);

	CWnd* pView = GetDlgItem(AFX_IDW_PANE_FIRST);//GetActiveView();
	if (pView != NULL)
		ChainMinMaxInfo(lpMMI, this, pView);
}

void CResizableMDIChild::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

/* Why was this necessary???

	// make sure the MDI parent frame doesn't clip
	// this child window when it is maximized
	if (nType == SIZE_MAXIMIZED)
	{
		CMDIFrameWnd* pFrame = GetMDIFrame();

		CRect rect;
		pFrame->GetWindowRect(rect);
		pFrame->MoveWindow(rect);
	}
/*/
}

// NOTE: this must be called after setting the layout
//       to have the view and its controls displayed properly
BOOL CResizableMDIChild::EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly, BOOL bHorzResize, BOOL bVertResize)
{
	m_sSection = pszSection;

	m_bEnableSaveRestore = TRUE;
	m_bRectOnly = bRectOnly;

	// restore immediately
	return LoadWindowRect(pszSection, bRectOnly, bHorzResize, bVertResize);
}

void CResizableMDIChild::OnDestroy() 
{
	if (m_bEnableSaveRestore)
		SaveWindowRect(m_sSection, m_bRectOnly);

	CMDIChildWnd::OnDestroy();
}


LRESULT CResizableMDIChild::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CMDIChildWnd::WindowProc(message, wParam, lParam);

	// specifying valid rects needs controls already anchored
	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CMDIChildWnd::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}

BOOL CResizableMDIChild::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CMDIChildWnd::OnNcCreate(lpCreateStruct))
		return FALSE;
	ModifyStyle(0, WS_CLIPCHILDREN);
	return TRUE;
}
