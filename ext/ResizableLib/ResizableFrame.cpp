// ResizableFrame.cpp : implementation file
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
#include "ResizableFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableFrame

IMPLEMENT_DYNCREATE(CResizableFrame, CFrameWnd)

CResizableFrame::CResizableFrame()
{
	m_bEnableSaveRestore = FALSE;
	m_bRectOnly = FALSE;
}

CResizableFrame::~CResizableFrame()
{
}


BEGIN_MESSAGE_MAP(CResizableFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CResizableFrame)
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	ON_WM_NCCREATE()
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableFrame message handlers

void CResizableFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	MinMaxInfo(lpMMI);

	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	ChainMinMaxInfo(lpMMI, this, pView);
}

// NOTE: this must be called after setting the layout
//       to have the view and its controls displayed properly
BOOL CResizableFrame::EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly, BOOL bHorzResize, BOOL bVertResize)
{
	m_sSection = pszSection;

	m_bEnableSaveRestore = TRUE;
	m_bRectOnly = bRectOnly;

	// restore immediately
	return LoadWindowRect(pszSection, bRectOnly, bHorzResize, bVertResize);
}

void CResizableFrame::OnDestroy() 
{
	if (m_bEnableSaveRestore)
		SaveWindowRect(m_sSection, m_bRectOnly);

	CFrameWnd::OnDestroy();
}

BOOL CResizableFrame::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CFrameWnd::OnNcCreate(lpCreateStruct))
		return FALSE;

	MakeResizable(lpCreateStruct);

	return TRUE;
}

LRESULT CResizableFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CFrameWnd::WindowProc(message, wParam, lParam);

	// specifying valid rects needs controls already anchored
	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CFrameWnd::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}

// TODO: implement this in CResizableMinMax
// We definitely need pluggable message handlers ala WTL!
void CResizableFrame::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	if ((lpwndpos->flags & (SWP_NOSIZE|SWP_NOMOVE)) != (SWP_NOSIZE|SWP_NOMOVE))
		CFrameWnd::OnWindowPosChanging(lpwndpos);
}
