// ResizableSplitterWnd.cpp : implementation file
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
#include "ResizableSplitterWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableSplitterWnd

IMPLEMENT_DYNAMIC(CResizableSplitterWnd, CSplitterWnd)

CResizableSplitterWnd::CResizableSplitterWnd()
{
}

CResizableSplitterWnd::~CResizableSplitterWnd()
{
}

BEGIN_MESSAGE_MAP(CResizableSplitterWnd, CSplitterWnd)
	//{{AFX_MSG_MAP(CResizableSplitterWnd)
	ON_WM_GETMINMAXINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableSplitterWnd message handlers

void CResizableSplitterWnd::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	if ((GetStyle() & SPLS_DYNAMIC_SPLIT) &&
		GetRowCount() == 1 && GetColumnCount() == 1)
	{
		CWnd* pPane = GetActivePane();
		if (pPane != NULL)
		{
			// get the extra size from child to parent
			CRect rectClient, rectWnd;
			GetWindowRect(rectWnd);
			pPane->GetWindowRect(rectClient);
			CSize sizeExtra = rectWnd.Size() - rectClient.Size();

			ChainMinMaxInfo(lpMMI, pPane->m_hWnd, sizeExtra);
		}
	}
	else if (!(GetStyle() & SPLS_DYNAMIC_SPLIT))
	{
		ASSERT(m_nCols > 0 && m_nRows > 0);

		CSize sizeMin(0,0), sizeMax(0,0);
		for (int col = 0; col < m_nCols; ++col)
		{
			// calc min and max width for each column
			int min = 0;
			int max = LONG_MAX;
			for (int row = 0; row < m_nRows; ++row)
			{
				CWnd* pWnd = GetPane(row, col);
				// ask the child window for track size
				MINMAXINFO mmiChild = *lpMMI;
				mmiChild.ptMinTrackSize.x = 0;
				mmiChild.ptMaxTrackSize.x = LONG_MAX;
				::SendMessage(pWnd->GetSafeHwnd(), WM_GETMINMAXINFO, 0, (LPARAM)&mmiChild);
				max = __min(max, mmiChild.ptMaxTrackSize.x);
				min = __max(min, mmiChild.ptMinTrackSize.x);
			}
			// sum all column widths
			if (sizeMax.cx == LONG_MAX || max == LONG_MAX)
				sizeMax.cx = LONG_MAX;
			else
				sizeMax.cx += max + m_cxSplitterGap;
			sizeMin.cx += min + m_cxSplitterGap;
		}
		for (int row = 0; row < m_nRows; ++row)
		{
			// calc min and max height for each row
			int min = 0;
			int max = LONG_MAX;
			for (int col = 0; col < m_nCols; ++col)
			{
				CWnd* pWnd = GetPane(row, col);
				// ask the child window for track size
				MINMAXINFO mmiChild = *lpMMI;
				mmiChild.ptMinTrackSize.y = 0;
				mmiChild.ptMaxTrackSize.y = LONG_MAX;
				::SendMessage(pWnd->GetSafeHwnd(), WM_GETMINMAXINFO, 0, (LPARAM)&mmiChild);
				max = __min(max, mmiChild.ptMaxTrackSize.y);
				min = __max(min, mmiChild.ptMinTrackSize.y);
			}
			// sum all row heights
			if (sizeMax.cy == LONG_MAX || max == LONG_MAX)
				sizeMax.cy = LONG_MAX;
			else
				sizeMax.cy += max + m_cySplitterGap;
			sizeMin.cy += min + m_cySplitterGap;
		}
		// adjust total size: add the client border and
		// we counted one splitter more than necessary
		sizeMax.cx += 2*m_cxBorder - m_cxSplitterGap;
		sizeMax.cy += 2*m_cyBorder - m_cySplitterGap;
		sizeMin.cx += 2*m_cxBorder - m_cxSplitterGap;
		sizeMin.cy += 2*m_cyBorder - m_cySplitterGap;
		// add non-client size
		CRect rectExtra(0,0,0,0);
		::AdjustWindowRectEx(&rectExtra, GetStyle(), !(GetStyle() & WS_CHILD) &&
			::IsMenu(GetMenu()->GetSafeHmenu()), GetExStyle());
		sizeMax += rectExtra.Size();
		sizeMin += rectExtra.Size();
		// set minmax info
		lpMMI->ptMinTrackSize.x = __max(lpMMI->ptMinTrackSize.x, sizeMin.cx);
		lpMMI->ptMinTrackSize.y = __max(lpMMI->ptMinTrackSize.y, sizeMin.cy);
		lpMMI->ptMaxTrackSize.x = __min(lpMMI->ptMaxTrackSize.x, sizeMax.cx);
		lpMMI->ptMaxTrackSize.y = __min(lpMMI->ptMaxTrackSize.y, sizeMax.cy);
	}
}
