// ResizableFormView.cpp : implementation file
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
#include "ResizableFormView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableFormView

IMPLEMENT_DYNAMIC(CResizableFormView, CFormView)

inline void CResizableFormView::PrivateConstruct()
{
	m_dwGripTempState = GHR_SCROLLBAR | GHR_ALIGNMENT | GHR_MAXIMIZED;
}

CResizableFormView::CResizableFormView(UINT nIDTemplate)
	: CFormView(nIDTemplate)
{
	PrivateConstruct();
}

CResizableFormView::CResizableFormView(LPCTSTR lpszTemplateName)
	: CFormView(lpszTemplateName)
{
	PrivateConstruct();
}

CResizableFormView::~CResizableFormView()
{
}


BEGIN_MESSAGE_MAP(CResizableFormView, CFormView)
	//{{AFX_MSG_MAP(CResizableFormView)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableFormView diagnostics

#ifdef _DEBUG
void CResizableFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CResizableFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CResizableFormView message handlers

void CResizableFormView::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);

	CWnd* pParent = GetParentFrame();

	// hide size grip when parent is maximized
	if (pParent->IsZoomed())
		HideSizeGrip(&m_dwGripTempState, GHR_MAXIMIZED);
	else
		ShowSizeGrip(&m_dwGripTempState, GHR_MAXIMIZED);

	// hide size grip when there are scrollbars
	CSize size = GetTotalSize();
	if ((cx < size.cx || cy < size.cy) && (m_nMapMode >= 0))
		HideSizeGrip(&m_dwGripTempState, GHR_SCROLLBAR);
	else
		ShowSizeGrip(&m_dwGripTempState, GHR_SCROLLBAR);

	// hide size grip when the parent frame window is not resizable
	// or the form is not bottom-right aligned (e.g. there's a statusbar)
	DWORD dwStyle = pParent->GetStyle();
	CRect rect, rectChild;
	GetWindowRect(rect);

	BOOL bCanResize = TRUE; // whether the grip can size the frame
	for (HWND hWndChild = ::GetWindow(m_hWnd, GW_HWNDFIRST); hWndChild != NULL;
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
		::GetWindowRect(hWndChild, rectChild);
		//! @todo check RTL layouts!
		if (rectChild.right > rect.right || rectChild.bottom > rect.bottom)
		{
			bCanResize = FALSE;
			break;
		}
	}
	if ((dwStyle & WS_THICKFRAME) && bCanResize)
		ShowSizeGrip(&m_dwGripTempState, GHR_ALIGNMENT);
	else
		HideSizeGrip(&m_dwGripTempState, GHR_ALIGNMENT);

	// update grip and layout
	UpdateSizeGrip();
	ArrangeLayout();
}

void CResizableFormView::GetTotalClientRect(LPRECT lpRect) const
{
	GetClientRect(lpRect);

	// get dialog template's size
	// (this is set in CFormView::Create)
	CSize sizeTotal, sizePage, sizeLine;
	int nMapMode = 0;
	GetDeviceScrollSizes(nMapMode, sizeTotal, sizePage, sizeLine);

	// otherwise, give the correct size if scrollbars active

	if (nMapMode < 0)	// scrollbars disabled
		return;

	// enlarge reported client area when needed
	CRect rect(lpRect);
	if (rect.Width() < sizeTotal.cx)
		rect.right = rect.left + sizeTotal.cx;
	if (rect.Height() < sizeTotal.cy)
		rect.bottom = rect.top + sizeTotal.cy;

	rect.OffsetRect(-GetDeviceScrollPosition());
	*lpRect = rect;
}

BOOL CResizableFormView::OnEraseBkgnd(CDC* pDC) 
{
	ClipChildren(pDC, FALSE);

	BOOL bRet = CFormView::OnEraseBkgnd(pDC);

	ClipChildren(pDC, TRUE);

	return bRet;
}

void CResizableFormView::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	MinMaxInfo(lpMMI);
}

void CResizableFormView::OnDestroy() 
{
	RemoveAllAnchors();

	CFormView::OnDestroy();
}

LRESULT CResizableFormView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message == WM_INITDIALOG)
		return (LRESULT)OnInitDialog();

	if (message != WM_NCCALCSIZE || wParam == 0)
		return CFormView::WindowProc(message, wParam, lParam);

	// specifying valid rects needs controls already anchored
	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CFormView::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}

BOOL CResizableFormView::OnInitDialog() 
{
	const MSG* pMsg = GetCurrentMessage();

	BOOL bRet = (BOOL)CFormView::WindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);

	// we need to associate member variables with control IDs
	UpdateData(FALSE);
	
	// set default scroll size
	CRect rectTemplate;
	GetWindowRect(rectTemplate);
	SetScrollSizes(MM_TEXT, rectTemplate.Size());

	return bRet;
}

BOOL CResizableFormView::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CFormView::OnNcCreate(lpCreateStruct))
		return FALSE;
	
	// create and init the size-grip
	if (!CreateSizeGrip())
		return FALSE;

	return TRUE;
}
