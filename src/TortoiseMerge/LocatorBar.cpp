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
#include "DiffColors.h"


IMPLEMENT_DYNAMIC(CLocatorBar, CPaneDialog)
CLocatorBar::CLocatorBar() : CPaneDialog()
	, m_pMainFrm(NULL)
	, m_pCacheBitmap(NULL)
	, m_bMouseWithin(FALSE)
	, m_nLines(-1)
{
}

CLocatorBar::~CLocatorBar()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
}

BEGIN_MESSAGE_MAP(CLocatorBar, CPaneDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

void CLocatorBar::DocumentUpdated()
{
	m_pMainFrm = (CMainFrame *)this->GetParentFrame();
	m_arLeftIdent.RemoveAll();
	m_arLeftState.RemoveAll();
	m_arRightIdent.RemoveAll();
	m_arRightState.RemoveAll();
	m_arBottomIdent.RemoveAll();
	m_arBottomState.RemoveAll();
	DiffStates state = DIFFSTATE_UNKNOWN;
	long identcount = 1;
	m_nLines = 0;
	if (m_pMainFrm->m_pwndLeftView->m_pViewData)
	{
		if (m_pMainFrm->m_pwndLeftView->m_pViewData->GetCount())
			state = m_pMainFrm->m_pwndLeftView->m_pViewData->GetState(0);
		for (int i=0; i<m_pMainFrm->m_pwndLeftView->m_pViewData->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndLeftView->m_pViewData->GetState(i))
			{
				identcount++;
			}
			else
			{
				m_arLeftIdent.Add(identcount);
				m_arLeftState.Add(state);
				state = m_pMainFrm->m_pwndLeftView->m_pViewData->GetState(i);
				identcount = 1;
			}
		}
		m_arLeftIdent.Add(identcount);
		m_arLeftState.Add(state);
	}

	if (m_pMainFrm->m_pwndRightView->m_pViewData)
	{
		if (m_pMainFrm->m_pwndRightView->m_pViewData->GetCount())
			state = m_pMainFrm->m_pwndRightView->m_pViewData->GetState(0);
		identcount = 1;
		for (int i=0; i<m_pMainFrm->m_pwndRightView->m_pViewData->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndRightView->m_pViewData->GetState(i))
			{
				identcount++;
			}
			else
			{
				m_arRightIdent.Add(identcount);
				m_arRightState.Add(state);
				state = m_pMainFrm->m_pwndRightView->m_pViewData->GetState(i);
				identcount = 1;
			}
		}
		m_arRightIdent.Add(identcount);
		m_arRightState.Add(state);
	}

	if (m_pMainFrm->m_pwndBottomView->m_pViewData)
	{
		if (m_pMainFrm->m_pwndBottomView->m_pViewData->GetCount())
			state = m_pMainFrm->m_pwndBottomView->m_pViewData->GetState(0);
		identcount = 1;
		for (int i=0; i<m_pMainFrm->m_pwndBottomView->m_pViewData->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndBottomView->m_pViewData->GetState(i))
			{
				identcount++;
			}
			else
			{
				m_arBottomIdent.Add(identcount);
				m_arBottomState.Add(state);
				state = m_pMainFrm->m_pwndBottomView->m_pViewData->GetState(i);
				identcount = 1;
			}
		}
		m_arBottomIdent.Add(identcount);
		m_arBottomState.Add(state);
		m_nLines = (int)max(m_pMainFrm->m_pwndBottomView->m_pViewData->GetCount(), m_pMainFrm->m_pwndRightView->m_pViewData->GetCount());
	}
	else if (m_pMainFrm->m_pwndRightView->m_pViewData)
		m_nLines = (int)max(0, m_pMainFrm->m_pwndRightView->m_pViewData->GetCount());

	if (m_pMainFrm->m_pwndLeftView->m_pViewData)
		m_nLines = (int)max(m_nLines, m_pMainFrm->m_pwndLeftView->m_pViewData->GetCount());
	else
		m_nLines = 0;
	m_nLines++;
	Invalidate();
}

void CLocatorBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	long height = rect.Height();
	long width = rect.Width();
	long nTopLine = 0;
	long nBottomLine = 0;
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	{
		nTopLine = m_pMainFrm->m_pwndLeftView->m_nTopLine;
		nBottomLine = nTopLine + m_pMainFrm->m_pwndLeftView->GetScreenLines();
	}
	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(&dc));

	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);


	COLORREF color, color2;

	CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, color, color2);
	cacheDC.FillSolidRect(rect, color);

	long barwidth = (width/3);
	DWORD state = 0;
	long identcount = 0;
	long linecount = 0;

	if (m_nLines)
		cacheDC.FillSolidRect(rect.left, height*nTopLine/m_nLines,
			rect.Width(), (height*nBottomLine/m_nLines)-(height*nTopLine/m_nLines), RGB(180,180,255));
	if (m_pMainFrm->m_pwndLeftView->IsWindowVisible())
	{
		for (long i=0; i<m_arLeftIdent.GetCount(); i++)
		{
			identcount = m_arLeftIdent.GetAt(i);
			state = m_arLeftState.GetAt(i);
			CDiffColors::GetInstance().GetColors((DiffStates)state, color, color2);
			if ((DiffStates)state != DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left, height*linecount/m_nLines,
							barwidth, max(height*identcount/m_nLines,1), color);
			linecount += identcount;
		}
	}
	state = 0;
	identcount = 0;
	linecount = 0;

	if (m_pMainFrm->m_pwndRightView->IsWindowVisible())
	{
		for (long i=0; i<m_arRightIdent.GetCount(); i++)
		{
			identcount = m_arRightIdent.GetAt(i);
			state = m_arRightState.GetAt(i);
			CDiffColors::GetInstance().GetColors((DiffStates)state, color, color2);
			if ((DiffStates)state != DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left + (width*2/3), height*linecount/m_nLines,
							barwidth, max(height*identcount/m_nLines,1), color);
			linecount += identcount;
		}
	}
	state = 0;
	identcount = 0;
	linecount = 0;
	if (m_pMainFrm->m_pwndBottomView->IsWindowVisible())
	{
		for (long i=0; i<m_arBottomIdent.GetCount(); i++)
		{
			identcount = m_arBottomIdent.GetAt(i);
			state = m_arBottomState.GetAt(i);
			CDiffColors::GetInstance().GetColors((DiffStates)state, color, color2);
			if ((DiffStates)state != DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left + (width/3), height*linecount/m_nLines,
							barwidth, max(height*identcount/m_nLines,1), color);
			linecount += identcount;
		}
	}

	if (m_nLines == 0)
		m_nLines = 1;
	cacheDC.FillSolidRect(rect.left, height*nTopLine/m_nLines,
		rect.Width(), 2, RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left, height*nBottomLine/m_nLines,
		rect.Width(), 2, RGB(0,0,0));
	//draw two vertical lines, so there are three rows visible indicating the three panes
	cacheDC.FillSolidRect(rect.left + (width/3), rect.top, 1, height, RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left + (width*2/3), rect.top, 1, height, RGB(0,0,0));

	// draw the fish eye
	if (m_bMouseWithin)
	{
		int fishstart = m_MousePos.y - height/20;
		int fishheight = height/10;
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
				color = cacheDC.GetPixel(i, j);
				int r,g,b;
				r = max(GetRValue(color)-20, 0);
				g = max(GetGValue(color)-20, 0);
				b = max(GetBValue(color)-20, 0);
				cacheDC.SetPixel(i, j, RGB(r,g,b));
			}
		}
	}
	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

void CLocatorBar::OnSize(UINT nType, int cx, int cy)
{
	CPaneDialog::OnSize(nType, cx, cy);

	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	Invalidate();
}

BOOL CLocatorBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CLocatorBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(rect);
	int nLine = point.y*m_nLines/rect.Height();

	if (nLine < 0)
		nLine = 0;
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
		m_pMainFrm->m_pwndBottomView->GoToLine(nLine, FALSE);
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
		m_pMainFrm->m_pwndLeftView->GoToLine(nLine, FALSE);
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndRightView))
		m_pMainFrm->m_pwndRightView->GoToLine(nLine, FALSE);
	Invalidate();
	CPaneDialog::OnLButtonDown(nFlags, point);
}

void CLocatorBar::OnMouseMove(UINT nFlags, CPoint point)
{
	m_MousePos = point;
	if (!m_bMouseWithin)
	{
		m_bMouseWithin = TRUE;
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		_TrackMouseEvent(&tme);
	}

	if (nFlags & MK_LBUTTON)
	{
		CRect rect;
		GetClientRect(rect);
		int nLine = point.y*m_nLines/rect.Height();

		if (nLine < 0)
			nLine = 0;
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
			m_pMainFrm->m_pwndBottomView->GoToLine(nLine, FALSE);
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
			m_pMainFrm->m_pwndLeftView->GoToLine(nLine, FALSE);
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndRightView))
			m_pMainFrm->m_pwndRightView->GoToLine(nLine, FALSE);
	}
	Invalidate();
}

LRESULT CLocatorBar::OnMouseLeave(WPARAM, LPARAM)
{
	m_bMouseWithin = FALSE;
	Invalidate();
	return 0;
}


