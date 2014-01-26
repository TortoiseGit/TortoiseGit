// ResizableComboBox.cpp : implementation file
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

#include "stdafx.h"
#include "ResizableComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableComboBox

CResizableComboBox::CResizableComboBox()
{
	m_bClipMaxHeight = TRUE;
	m_bIntegralHeight = TRUE;
	m_iExtent = 0;
}

CResizableComboBox::~CResizableComboBox()
{
	if (m_ctrlListBox.GetSafeHwnd() != NULL)
		m_ctrlListBox.UnsubclassWindow();
}


BEGIN_MESSAGE_MAP(CResizableComboBox, CComboBox)
	//{{AFX_MSG_MAP(CResizableComboBox)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableComboBox message handlers

HBRUSH CResizableComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
	
	if (nCtlColor == CTLCOLOR_LISTBOX)
	{
		if (!(GetStyle() & CBS_SIMPLE)
			&& (m_ctrlListBox.m_hWnd == NULL))
		{
			TRACE("ComboLBox: 0x%08X\n", pWnd->m_hWnd);

			// attach to the owned listbox
			m_ctrlListBox.m_pOwnerCombo = this;
			m_ctrlListBox.SubclassWindow(pWnd->m_hWnd);
		}
	}

	return hbr;
}

LRESULT CResizableComboBox::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
	case CB_GETDROPPEDCONTROLRECT:
		*(LPRECT)lParam = m_rectDropDown;
		MapWindowPoints(NULL, (LPRECT)lParam);
		return TRUE;
	}

	LRESULT lResult = CComboBox::WindowProc(message, wParam, lParam);

	// if listbox is attached, update horizontal extent

	switch (message)
	{
	case CB_INSERTSTRING:
	case CB_ADDSTRING:
		if (lResult != CB_ERR && lResult != CB_ERRSPACE)
			UpdateHorizontalExtent((LPCTSTR)lParam);
		break;

	case CB_DIR:
		if (lResult != CB_ERR && lResult != CB_ERRSPACE)
			InitHorizontalExtent();
		break;

	case CB_RESETCONTENT:
		InitHorizontalExtent();
		break;
	}

	return lResult;
}

void CResizableComboBox::InitHorizontalExtent()
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(GetFont());

	CString str;
	
	m_iExtent = 0;
	int n = GetCount();
	for (int i=0; i<n; i++)
	{
		GetLBText(i, str);
		int cx = dc.GetTextExtent(str).cx;
		if (cx > m_iExtent)
			m_iExtent = cx;
	}

	SetHorizontalExtent(m_iExtent
		+ LOWORD(GetDialogBaseUnits()));

	dc.SelectObject(pOldFont);
}

void CResizableComboBox::UpdateHorizontalExtent(LPCTSTR szText)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(GetFont());

	int cx = dc.GetTextExtent(szText, lstrlen(szText)).cx;
	if (cx > m_iExtent)
	{
		m_iExtent = cx;

		SetHorizontalExtent(m_iExtent
			+ LOWORD(GetDialogBaseUnits()));
	}

	dc.SelectObject(pOldFont);
}

void CResizableComboBox::PreSubclassWindow() 
{
	ASSERT(GetStyle() & CBS_NOINTEGRALHEIGHT);

	InitHorizontalExtent();
	
	GetDroppedControlRect(&m_rectDropDown);
	::MapWindowPoints(NULL, GetSafeHwnd(),
		(LPPOINT)&m_rectDropDown, 2);
	
	CComboBox::PreSubclassWindow();
}

int CResizableComboBox::MakeIntegralHeight(const int height)
{
	int inth = height;	// integral height (result)
	int availh = height;	// available height
	int n = GetCount();

	DWORD dwStyle = GetStyle();

	if (!m_bIntegralHeight || n == 0)
		return inth;
	
	if (dwStyle & CBS_OWNERDRAWVARIABLE)
	{
		inth = 0;	// try to reach availh by integral steps
		int i = 0;
		// use items below the first visible
		for (i=GetTopIndex(); availh>0 && i<n; i++)
		{
			int h = GetItemHeight(i);
			if (h == CB_ERR)
				break;

			inth += h;
			availh -= h;
		}
		// to fill the remaining height, use items above
		for (i=GetTopIndex()-1; availh>0 && i>=0; i--)
		{
			int h = GetItemHeight(i);
			if (h == CB_ERR)
				break;

			inth += h;
			availh -= h;
		}
		// scroll into view
		SetTopIndex(i);

		if (!m_bClipMaxHeight) // it can be higher than all the items
		{
			// to fill the remaining height, use last item
			int h = GetItemHeight(n-1);
			if (h != CB_ERR)
			{
				inth += availh - availh % h;
			}
		}
	}
	else
	{
		// every item has the same height (take the first)
		int h = GetItemHeight(0);
		if (h != CB_ERR && n != CB_ERR)
		{
			int rows = availh / h;
			// can't be higher than all the items
			if (m_bClipMaxHeight && rows > n)
				rows = n;
			inth = rows * h;
			// scroll into view
			if (n - rows < GetTopIndex())
				SetTopIndex(n-rows);
		}
	}

	return inth;
}
