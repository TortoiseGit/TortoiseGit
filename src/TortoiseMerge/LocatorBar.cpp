// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2012, 2015 - TortoiseSVN

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
#include "DiffColors.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CLocatorBar, CPaneDialog)
CLocatorBar::CLocatorBar() : CPaneDialog()
	, m_pMainFrm(nullptr)
	, m_pCacheBitmap(nullptr)
	, m_regUseFishEye(L"Software\\TortoiseGitMerge\\UseFishEye", TRUE)
	, m_nLines(-1)
	, m_minWidth(0)
{
}

CLocatorBar::~CLocatorBar()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = nullptr;
	}
}

BEGIN_MESSAGE_MAP(CLocatorBar, CPaneDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

void CLocatorBar::DocumentUpdated()
{
	m_pMainFrm = static_cast<CMainFrame*>(this->GetParentFrame());
	if (!m_pMainFrm)
		return;

	m_nLines = 0;
	DocumentUpdated(m_pMainFrm->m_pwndLeftView, m_arLeftIdent, m_arLeftState);
	DocumentUpdated(m_pMainFrm->m_pwndRightView, m_arRightIdent, m_arRightState);
	DocumentUpdated(m_pMainFrm->m_pwndBottomView, m_arBottomIdent, m_arBottomState);

	if (m_pMainFrm->m_pwndLeftView)
	{
		m_nLines = m_pMainFrm->m_pwndLeftView->GetLineCount();
		if (m_pMainFrm->m_pwndRightView)
		{
				m_nLines = std::max<int>(m_nLines, m_pMainFrm->m_pwndRightView->GetLineCount());
			if (m_pMainFrm->m_pwndBottomView)
			{
				m_nLines = std::max<int>(m_nLines, m_pMainFrm->m_pwndBottomView->GetLineCount());
			}
		}
	}
	Invalidate();
}

void CLocatorBar::DocumentUpdated(CBaseView* view, CDWordArray& indents, CDWordArray& states)
{
	indents.RemoveAll();
	states.RemoveAll();
	CViewData* viewData = view->m_pViewData;
	if(viewData == 0)
		return;

	long identcount = 1;
	const int linesInView = view->GetLineCount();
	DiffStates state = DIFFSTATE_UNKNOWN;
	if (linesInView)
		state = viewData->GetState(0);
	for (int i=1; i<linesInView; i++)
	{
		const DiffStates lineState = viewData->GetState(view->GetViewLineForScreen(i));
		if (state == lineState)
		{
			identcount++;
		}
		else
		{
			indents.Add(identcount);
			states.Add(state);
			state = lineState;
			identcount = 1;
		}
	}
	indents.Add(identcount);
	states.Add(state);
}

CSize CLocatorBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	auto s = __super::CalcFixedLayout(bStretch, bHorz);
	s.cx = m_minWidth;
	return s;
}

void CLocatorBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	const long height = rect.Height();
	const long width = rect.Width();
	long nTopLine = 0;
	long nBottomLine = 0;
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	{
		nTopLine = m_pMainFrm->m_pwndLeftView->m_nTopLine;
		nBottomLine = nTopLine + m_pMainFrm->m_pwndLeftView->GetScreenLines();
	}
	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(&dc));

	if (!m_pCacheBitmap)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	COLORREF color, color2;
	CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, color, color2);
	cacheDC.FillSolidRect(rect, color);

	if (m_nLines)
	{
		cacheDC.FillSolidRect(rect.left, height*nTopLine/m_nLines,
			width, (height*nBottomLine/m_nLines)-(height*nTopLine/m_nLines), RGB(180,180,255));

		if (m_pMainFrm)
		{
			PaintView (cacheDC, m_pMainFrm->m_pwndLeftView, m_arLeftIdent, m_arLeftState, rect, 0);
			PaintView (cacheDC, m_pMainFrm->m_pwndRightView, m_arRightIdent, m_arRightState, rect, 2);
			PaintView (cacheDC, m_pMainFrm->m_pwndBottomView, m_arBottomIdent, m_arBottomState, rect, 1);
		}
	}

	if (m_nLines == 0)
		m_nLines = 1;
	cacheDC.FillSolidRect(rect.left, height*nTopLine/m_nLines,
		width, 2, RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left, height*nBottomLine/m_nLines,
		width, 2, RGB(0,0,0));
	//draw two vertical lines, so there are three rows visible indicating the three panes
	cacheDC.FillSolidRect(rect.left + (width/3), rect.top, 1, height, RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left + (width*2/3), rect.top, 1, height, RGB(0,0,0));

	// draw the fish eye
	DWORD pos = GetMessagePos();
	CRect screenRect = rect;
	ClientToScreen(screenRect);
	POINT pt;
	pt.x = GET_X_LPARAM(pos);
	pt.y = GET_Y_LPARAM(pos);

	if ((screenRect.PtInRect(pt))&&(DWORD(m_regUseFishEye)))
		DrawFishEye (cacheDC, rect);

	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

void CLocatorBar::OnSize(UINT nType, int cx, int cy)
{
	CPaneDialog::OnSize(nType, cx, cy);

	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = nullptr;
	}
	Invalidate();
}

BOOL CLocatorBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CLocatorBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	ScrollOnMouseMove(point);
	Invalidate();
	CPaneDialog::OnLButtonDown(nFlags, point);
}

void CLocatorBar::OnMouseMove(UINT nFlags, CPoint point)
{
	m_MousePos = point;

	if (nFlags & MK_LBUTTON)
	{
		SetCapture();
		ScrollOnMouseMove(point);
	}

	TRACKMOUSEEVENT Tme;
	Tme.cbSize = sizeof(TRACKMOUSEEVENT);
	Tme.dwFlags = TME_LEAVE;
	Tme.hwndTrack = m_hWnd;
	TrackMouseEvent(&Tme);


	Invalidate();
}

LRESULT CLocatorBar::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	Invalidate();
	return 0;
}

void CLocatorBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	Invalidate();

	CPaneDialog::OnLButtonUp(nFlags, point);
}

void CLocatorBar::ScrollOnMouseMove(const CPoint& point )
{
	if (m_pMainFrm == 0)
		return;

	CRect rect;
	GetClientRect(rect);

	int nLine = point.y*m_nLines/rect.Height();
	if (nLine < 0)
		nLine = 0;

	ScrollViewToLine(m_pMainFrm->m_pwndBottomView, nLine);
	ScrollViewToLine(m_pMainFrm->m_pwndLeftView, nLine);
	ScrollViewToLine(m_pMainFrm->m_pwndRightView, nLine);
}

void CLocatorBar::ScrollViewToLine(CBaseView* view, int nLine) const
{
	if (view != 0)
		view->GoToLine(nLine, FALSE);
}

void CLocatorBar::PaintView(CDC& cacheDC, CBaseView* view, CDWordArray& indents,
							CDWordArray& states, const CRect& rect, int stripeIndex)
{
	if (!view->IsWindowVisible())
		return;

	const long height = rect.Height();
	const long width = rect.Width();
	const long barwidth = (width/3);
	long linecount = 0;
	for (long i=0; i<indents.GetCount(); i++)
	{
		COLORREF color, color2;
		const long identcount = indents.GetAt(i);
		const DWORD state = states.GetAt(i);
		CDiffColors::GetInstance().GetColors(static_cast<DiffStates>(state), color, color2);
		if (static_cast<DiffStates>(state) != DIFFSTATE_NORMAL)
		{
			cacheDC.FillSolidRect(rect.left + (width*stripeIndex/3), height*linecount/m_nLines,
						barwidth, max(static_cast<int>(height * identcount / m_nLines), 1), static_cast<int>(color));
		}
		linecount += identcount;
	}
	if (view->GetMarkedWord()[0])
	{
		COLORREF color, color2;
		CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, color, color2);
		color = CAppUtils::IntenseColor(200, color);
		for (size_t i=0; i<view->m_arMarkedWordLines.size(); ++i)
		{
			if (view->m_arMarkedWordLines[i])
			{
				cacheDC.FillSolidRect(rect.left + (width * stripeIndex / 3), static_cast<int>(height * i / m_nLines),
					barwidth, max(static_cast<int>(height / m_nLines), 2), static_cast<int>(color));
			}
		}
	}
	if (view->GetFindString()[0])
	{
		COLORREF color, color2;
		CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, color, color2);
		color = CAppUtils::IntenseColor(30, color);
		for (size_t i=0; i<view->m_arFindStringLines.size(); ++i)
		{
			if (view->m_arFindStringLines[i])
			{
				cacheDC.FillSolidRect(rect.left + (width*stripeIndex/3), static_cast<int>(height * i / m_nLines),
					barwidth, max(static_cast<int>(height / m_nLines), 2), static_cast<int>(color));
			}
		}
	}

}

void CLocatorBar::DrawFishEye(CDC& cacheDC, const CRect& rect )
{
	const long height = rect.Height();
	const long width = rect.Width();
	const int fishstart = m_MousePos.y - height/20;
	const int fishheight = height/10;
	cacheDC.FillSolidRect(rect.left, fishstart-1, width, 1, RGB(0,0,100));
	cacheDC.FillSolidRect(rect.left, fishstart+fishheight+1, width, 1, RGB(0,0,100));
	VERIFY(cacheDC.StretchBlt(rect.left, fishstart, width, fishheight,
		&cacheDC, 0, fishstart + (3*fishheight/8), width, fishheight/4, SRCCOPY));
	// draw the magnified area a little darker, so the
	// user has a clear indication of the magnifier
	for (int i=rect.left; i<(width - rect.left); i++)
	{
		for (int j=fishstart; j<fishstart+fishheight; j++)
		{
			const COLORREF color = cacheDC.GetPixel(i, j);
			int r = max(GetRValue(color)-20, 0);
			int g = max(GetGValue(color)-20, 0);
			int b = max(GetBValue(color)-20, 0);
			cacheDC.SetPixel(i, j, RGB(r,g,b));
		}
	}
}

