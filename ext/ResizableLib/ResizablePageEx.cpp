// ResizablePageEx.cpp : implementation file
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
#include "ResizablePageEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizablePageEx

IMPLEMENT_DYNCREATE(CResizablePageEx, CPropertyPageEx)

CResizablePageEx::CResizablePageEx()
{
}

CResizablePageEx::CResizablePageEx(UINT nIDTemplate, UINT nIDCaption, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle)
	: CPropertyPageEx(nIDTemplate, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle)
{
}

CResizablePageEx::CResizablePageEx(LPCTSTR lpszTemplateName, UINT nIDCaption, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle)
	: CPropertyPageEx(lpszTemplateName, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle)
{
}

CResizablePageEx::~CResizablePageEx()
{
}


BEGIN_MESSAGE_MAP(CResizablePageEx, CPropertyPageEx)
	//{{AFX_MSG_MAP(CResizablePageEx)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CResizablePageEx message handlers

void CResizablePageEx::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	ArrangeLayout();

	if (m_psp.dwFlags & PSP_HIDEHEADER)
		Invalidate();
}

BOOL CResizablePageEx::OnEraseBkgnd(CDC* pDC) 
{
	ClipChildren(pDC, FALSE);

	BOOL bRet = CPropertyPageEx::OnEraseBkgnd(pDC);

	ClipChildren(pDC, TRUE);

	return bRet;
}

void CResizablePageEx::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	MinMaxInfo(lpMMI);
}

BOOL CResizablePageEx::OnInitDialog() 
{
	CPropertyPageEx::OnInitDialog();
	
	// set the initial size as the min track size
	CRect rc;
	GetWindowRect(&rc);
	SetMinTrackSize(rc.Size());

	// HACK:  temporarily abandon subclassing
	// CAUSE: system subclasses this window after this message
	// ISSUE: our WindowProc is not the first to be called
	//        and we miss some painting related messages
	if (Attach(UnsubclassWindow()))
	{
		CWnd* pParent = GetParent();
		pParent->LockWindowUpdate();
		Post_SheetPageExHack(pParent->GetSafeHwnd(), m_hWnd);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CResizablePageEx::OnDestroy() 
{
	// remove child windows
	RemoveAllAnchors();

	CPropertyPageEx::OnDestroy();
}

HBRUSH CResizablePageEx::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	// NOTE: this message couldn't be catched without the above hack

	HBRUSH hbr = CPropertyPageEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (hbr && (m_psp.dwFlags & PSP_HIDEHEADER))
	{
		// reposition origin of background brush
		// used for transparent effect on page controls
		// (needed with double-buffering and XP themes)
		CRect rect;
		pWnd->GetWindowRect(rect);
		pWnd->SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM)&rect);
		ScreenToClient(rect);
		CPoint pt(-rect.TopLeft());
		HDC hDC = pDC->GetSafeHdc();
		::LPtoDP(hDC, &pt, 1);
		::UnrealizeObject(hbr);
		::SetBrushOrgEx(hDC, pt.x, pt.y, NULL);
	}
	return hbr;
}

LRESULT CResizablePageEx::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CPropertyPageEx::WindowProc(message, wParam, lParam);

	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CPropertyPageEx::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}
