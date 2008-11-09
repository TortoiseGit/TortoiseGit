// ResizablePage.cpp : implementation file
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
#include "ResizablePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizablePage

IMPLEMENT_DYNCREATE(CResizablePage, CPropertyPage)

CResizablePage::CResizablePage()
{
}

CResizablePage::CResizablePage(UINT nIDTemplate, UINT nIDCaption)
	: CPropertyPage(nIDTemplate, nIDCaption)
{
}

CResizablePage::CResizablePage(LPCTSTR lpszTemplateName, UINT nIDCaption)
	: CPropertyPage(lpszTemplateName, nIDCaption)
{
}

CResizablePage::~CResizablePage()
{
}


BEGIN_MESSAGE_MAP(CResizablePage, CPropertyPage)
	//{{AFX_MSG_MAP(CResizablePage)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CResizablePage message handlers

void CResizablePage::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	ArrangeLayout();
}

BOOL CResizablePage::OnEraseBkgnd(CDC* pDC) 
{
	ClipChildren(pDC, FALSE);

	BOOL bRet = CPropertyPage::OnEraseBkgnd(pDC);

	ClipChildren(pDC, TRUE);

	return bRet;
}

void CResizablePage::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	MinMaxInfo(lpMMI);
}

BOOL CResizablePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// set the initial size as the min track size
	CRect rc;
	GetWindowRect(&rc);
	SetMinTrackSize(rc.Size());

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CResizablePage::OnDestroy() 
{
	// remove child windows
	RemoveAllAnchors();

	CPropertyPage::OnDestroy();
}

LRESULT CResizablePage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CPropertyPage::WindowProc(message, wParam, lParam);

	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CPropertyPage::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}
