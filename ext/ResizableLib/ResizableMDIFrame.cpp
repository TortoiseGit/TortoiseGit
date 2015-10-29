// ResizableMDIFrame.cpp : implementation file
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
#include "ResizableMDIFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableMDIFrame

IMPLEMENT_DYNCREATE(CResizableMDIFrame, CMDIFrameWnd)

CResizableMDIFrame::CResizableMDIFrame()
{
	m_bEnableSaveRestore = FALSE;
	m_bRectOnly = FALSE;
}

CResizableMDIFrame::~CResizableMDIFrame()
{
}


BEGIN_MESSAGE_MAP(CResizableMDIFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CResizableMDIFrame)
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	ON_WM_NCCREATE()
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableMDIFrame message handlers

void CResizableMDIFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	// MDI should call default implementation
	CMDIFrameWnd::OnGetMinMaxInfo(lpMMI);

	MinMaxInfo(lpMMI);

	BOOL bMaximized = FALSE;
	CMDIChildWnd* pChild = MDIGetActive(&bMaximized);
	if (pChild != NULL && bMaximized)
		ChainMinMaxInfo(lpMMI, this, pChild);
}

// NOTE: this must be called after setting the layout
//       to have the view and its controls displayed properly
BOOL CResizableMDIFrame::EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly, BOOL bHorzResize, BOOL bVertResize)
{
	m_sSection = pszSection;

	m_bEnableSaveRestore = TRUE;
	m_bRectOnly = bRectOnly;

	// restore immediately
	return LoadWindowRect(pszSection, bRectOnly, bHorzResize, bVertResize);
}

void CResizableMDIFrame::OnDestroy() 
{
	if (m_bEnableSaveRestore)
		SaveWindowRect(m_sSection, m_bRectOnly);

	CMDIFrameWnd::OnDestroy();
}

BOOL CResizableMDIFrame::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CMDIFrameWnd::OnNcCreate(lpCreateStruct))
		return FALSE;

	MakeResizable(lpCreateStruct);
	
	return TRUE;
}

LRESULT CResizableMDIFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CMDIFrameWnd::WindowProc(message, wParam, lParam);

	// specifying valid rects needs controls already anchored
	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CMDIFrameWnd::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}

void CResizableMDIFrame::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CMDIFrameWnd::OnWindowPosChanging(lpwndpos);

	// since this window class doesn't have the style CS_HREDRAW|CS_VREDRAW
	// the client area is not invalidated during a resize operation and
	// this prevents the system from using WM_NCCALCSIZE to validate rects
	Invalidate();
}
