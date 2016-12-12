// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2008, 2010 - TortoiseSVN

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
	m_pMainFrm = nullptr;
	m_pCacheBitmap = nullptr;
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
		m_pCacheBitmap = nullptr;
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
	SetWindowPos(nullptr, 0, 0, size.cx, 2 * m_nLineHeight, SWP_NOMOVE);
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
	if (!m_pCacheBitmap)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	CRect upperrect = CRect(rect.left, rect.top, rect.right, rect.bottom/2);
	CRect lowerrect = CRect(rect.left, rect.bottom/2, rect.right, rect.bottom);

	if (m_pMainFrm!=0)
	{
		CLeftView* leftView = m_pMainFrm->m_pwndLeftView;
		CRightView* rightView = m_pMainFrm->m_pwndRightView;
		if (CBaseView::IsViewGood(leftView)&&CBaseView::IsViewGood(rightView))
		{
			BOOL bViewWhiteSpace = leftView->m_bViewWhitespace;
			BOOL bInlineDiffs = leftView->m_bShowInlineDiff;

			leftView->m_bViewWhitespace = TRUE;
			leftView->m_bShowInlineDiff = TRUE;
			leftView->m_bWhitespaceInlineDiffs = true;
			leftView->m_bShowSelection = false;
			rightView->m_bViewWhitespace = TRUE;
			rightView->m_bShowInlineDiff = TRUE;
			rightView->m_bWhitespaceInlineDiffs = true;
			rightView->m_bShowSelection = false;

			// Use left and right view to display lines next to each other
			leftView->DrawSingleLine(&cacheDC, &upperrect, m_nLineIndex);
			rightView->DrawSingleLine(&cacheDC, &lowerrect, m_nLineIndex);

			leftView->m_bViewWhitespace = bViewWhiteSpace;
			leftView->m_bShowInlineDiff = bInlineDiffs;
			leftView->m_bWhitespaceInlineDiffs = false;
			leftView->m_bShowSelection = true;
			rightView->m_bViewWhitespace = bViewWhiteSpace;
			rightView->m_bShowInlineDiff = bInlineDiffs;
			rightView->m_bWhitespaceInlineDiffs = false;
			rightView->m_bShowSelection = true;
		}
	}

	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

void CLineDiffBar::OnSize(UINT nType, int cx, int cy)
{
	CPaneDialog::OnSize(nType, cx, cy);

	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = nullptr;
	}
	SetWindowPos(nullptr, 0, 0, cx, 2 * m_nLineHeight, SWP_NOMOVE);
	Invalidate();
}

BOOL CLineDiffBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}
