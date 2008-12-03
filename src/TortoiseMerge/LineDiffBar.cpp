// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "LocatorBar.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"

IMPLEMENT_DYNAMIC(CLineDiffBar, CPaneDialog)
CLineDiffBar::CLineDiffBar() : CPaneDialog()
{
	m_pMainFrm = NULL;
	m_pCacheBitmap = NULL;
	m_nLineIndex = -1;
	m_nLineHeight = 0;
	m_bExclusiveRow = TRUE;
}

CLineDiffBar::~CLineDiffBar()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
}

BEGIN_MESSAGE_MAP(CLineDiffBar, CPaneDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CLineDiffBar::DocumentUpdated()
{
	//resize according to the font size
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	{
		m_nLineHeight = m_pMainFrm->m_pwndLeftView->GetLineHeight();
	}
	CRect rect;
	GetWindowRect(rect);
	CSize size = rect.Size();
	size.cy = 2 * m_nLineHeight;
	SetMinSize(size);
	SetWindowPos(NULL, 0, 0, size.cx, 2*m_nLineHeight, SWP_NOMOVE);
	RecalcLayout();
	if (m_pMainFrm)
		m_pMainFrm->RecalcLayout();
}

void CLineDiffBar::ShowLines(int nLineIndex)
{
	m_nLineIndex = nLineIndex;
	Invalidate();
}

void CLineDiffBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	int height = rect.Height();
	int width = rect.Width();

	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(&dc));
	cacheDC.FillSolidRect(&rect, ::GetSysColor(COLOR_WINDOW));
	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	CRect upperrect = CRect(rect.left, rect.top, rect.right, rect.bottom/2);
	CRect lowerrect = CRect(rect.left, rect.bottom/2, rect.right, rect.bottom);

	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView)&&(m_pMainFrm->m_pwndRightView))
	{
		if ((m_pMainFrm->m_pwndLeftView->IsWindowVisible())&&(m_pMainFrm->m_pwndRightView->IsWindowVisible()))
		{
			BOOL bViewWhiteSpace = m_pMainFrm->m_pwndLeftView->m_bViewWhitespace;
			BOOL bInlineDiffs = m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff;
			
			m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = TRUE;
			m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff = TRUE;
			m_pMainFrm->m_pwndLeftView->m_bShowSelection = false;
			m_pMainFrm->m_pwndRightView->m_bViewWhitespace = TRUE;
			m_pMainFrm->m_pwndRightView->m_bShowInlineDiff = TRUE;
			m_pMainFrm->m_pwndRightView->m_bShowSelection = false;

			// Use left and right view to display lines next to each other
			m_pMainFrm->m_pwndLeftView->DrawSingleLine(&cacheDC, &upperrect, m_nLineIndex);
			m_pMainFrm->m_pwndRightView->DrawSingleLine(&cacheDC, &lowerrect, m_nLineIndex);

			m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = bViewWhiteSpace;
			m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff = bInlineDiffs;
			m_pMainFrm->m_pwndLeftView->m_bShowSelection = true;
			m_pMainFrm->m_pwndRightView->m_bViewWhitespace = bViewWhiteSpace;
			m_pMainFrm->m_pwndRightView->m_bShowInlineDiff = bInlineDiffs;
			m_pMainFrm->m_pwndRightView->m_bShowSelection = true;
		}
	} 

	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

void CLineDiffBar::OnSize(UINT nType, int cx, int cy)
{
	CPaneDialog::OnSize(nType, cx, cy);

	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	SetWindowPos(NULL, 0, 0, cx, 2*m_nLineHeight, SWP_NOMOVE);
	Invalidate();
}

BOOL CLineDiffBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}


