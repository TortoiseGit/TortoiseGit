// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008 - TortoiseSVN

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
#include ".\splittercontrol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl

// hCursor1 is for vertical one
// and hCursor2 is for horizontal one
static HCURSOR SplitterControl_hCursor1 = NULL;
static HCURSOR SplitterControl_hCursor2 = NULL;

CSplitterControl::CSplitterControl()
{
	// Mouse is pressed down or not ?
	m_bIsPressed = FALSE;	

	// Min and Max range of the splitter.
	m_nMin = m_nMax = -1;
	m_bMouseOverControl = false;
}

CSplitterControl::~CSplitterControl()
{
}


BEGIN_MESSAGE_MAP(CSplitterControl, CStatic)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl message handlers


// Set style for splitter control
// nStyle = SPS_VERTICAL or SPS_HORIZONTAL
int CSplitterControl::SetSplitterStyle(int nStyle)
{
	int m_nOldStyle = m_nType;
	m_nType = nStyle;
	return m_nOldStyle;
}
int CSplitterControl::GetSplitterStyle()
{
	return m_nType;
}

void CSplitterControl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rcClient;
	GetClientRect(rcClient);
	if (m_bMouseOverControl)
	{
		CPen pen, *pOP;

		rcClient.DeflateRect(1,1,1,1);

		pen.CreatePen(0, 1, GetSysColor(COLOR_3DSHADOW));
		pOP = dc.SelectObject(&pen);

		dc.MoveTo(rcClient.left, rcClient.top);
		dc.LineTo(rcClient.right, rcClient.top);
		dc.MoveTo(rcClient.left, rcClient.bottom);
		dc.LineTo(rcClient.right, rcClient.bottom);

		// Restore pen
		dc.SelectObject(pOP);	
	}
	else
	{
		dc.SetBkColor(GetSysColor(COLOR_3DFACE));
		dc.ExtTextOut(0, 0, ETO_OPAQUE, &rcClient, NULL, 0, NULL);
	}
}

void CSplitterControl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bIsPressed)
	{
		CWindowDC dc(NULL);
		DrawLine(&dc);
		
		CPoint pt = point;
		ClientToScreen(&pt);
		GetParent()->ScreenToClient(&pt);

		if (pt.x < m_nMin)
			pt.x = m_nMin;
		if (pt.y < m_nMin)
			pt.y = m_nMin;

		if (pt.x > m_nMax)
			pt.x = m_nMax;
		if (pt.y > m_nMax)
			pt.y = m_nMax;

		GetParent()->ClientToScreen(&pt);
		m_nX = pt.x;
		m_nY = pt.y;
		DrawLine(&dc);
	}
	if (!m_bMouseOverControl)
	{
		TRACKMOUSEEVENT Tme;
		Tme.cbSize = sizeof(TRACKMOUSEEVENT);
		Tme.dwFlags = TME_LEAVE;
		Tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&Tme);

		m_bMouseOverControl = true;
		Invalidate();
	}
	CStatic::OnMouseMove(nFlags, point);
}

LRESULT CSplitterControl::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_bMouseOverControl = false;
	Invalidate();
	return 0;
}

BOOL CSplitterControl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (nHitTest == HTCLIENT)
	{
		(m_nType == SPS_VERTICAL)?(::SetCursor(SplitterControl_hCursor1))
			:(::SetCursor(SplitterControl_hCursor2));
		return 0;
	}
	else
		return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

void CSplitterControl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CStatic::OnLButtonDown(nFlags, point);
	
	m_bIsPressed = TRUE;
	SetCapture();
	CRect rcWnd;
	GetWindowRect(rcWnd);
	
	if (m_nType == SPS_VERTICAL)
		m_nX = rcWnd.left + rcWnd.Width() / 2;	
	
	else
		m_nY = rcWnd.top  + rcWnd.Height() / 2;
	
	if (m_nType == SPS_VERTICAL)
		m_nSavePos = m_nX;
	else
		m_nSavePos = m_nY;

	CWindowDC dc(NULL);
	DrawLine(&dc);
}

void CSplitterControl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bIsPressed)
	{
		ClientToScreen(&point);
		CWindowDC dc(NULL);

		DrawLine(&dc);
		CPoint pt(m_nX, m_nY);
		m_bIsPressed = FALSE;
		CWnd *pOwner = GetOwner();
		if (pOwner && IsWindow(pOwner->m_hWnd))
		{
			CRect rc;
			int delta;
			pOwner->GetClientRect(rc);
			pOwner->ScreenToClient(&pt);
			MoveWindowTo(pt);

			if (m_nType == SPS_VERTICAL)
				delta = m_nX - m_nSavePos;
			else
				delta = m_nY - m_nSavePos;
			
			
			SPC_NMHDR nmsp;
		
			nmsp.hdr.hwndFrom = m_hWnd;
			nmsp.hdr.idFrom   = GetDlgCtrlID();
			nmsp.hdr.code     = SPN_SIZED;
			nmsp.delta = delta;

			pOwner->SendMessage(WM_NOTIFY, nmsp.hdr.idFrom, (LPARAM)&nmsp);
		}
	}

	CStatic::OnLButtonUp(nFlags, point);
	ReleaseCapture();
}

void CSplitterControl::DrawLine(CDC* pDC)
{
	int nRop = pDC->SetROP2(R2_NOTXORPEN);

	CRect rcWnd;
	int d = 1;
	GetWindowRect(rcWnd);
	CPen  pen;
	pen.CreatePen(0, 1, ::GetSysColor(COLOR_GRAYTEXT));
	CPen *pOP = pDC->SelectObject(&pen);
	
	if (m_nType == SPS_VERTICAL)
	{
		pDC->MoveTo(m_nX - d, rcWnd.top);
		pDC->LineTo(m_nX - d, rcWnd.bottom);

		pDC->MoveTo(m_nX + d, rcWnd.top);
		pDC->LineTo(m_nX + d, rcWnd.bottom);
	}
	else // m_nType == SPS_HORIZONTAL
	{
		pDC->MoveTo(rcWnd.left, m_nY - d);
		pDC->LineTo(rcWnd.right, m_nY - d);
		
		pDC->MoveTo(rcWnd.left, m_nY + d);
		pDC->LineTo(rcWnd.right, m_nY + d);
	}
	pDC->SetROP2(nRop);
	pDC->SelectObject(pOP);
}

void CSplitterControl::MoveWindowTo(CPoint pt)
{
	CRect rc;
	GetWindowRect(rc);
	CWnd* pParent;
	pParent = GetParent();
	if (!pParent || !::IsWindow(pParent->m_hWnd))
		return;

	pParent->ScreenToClient(rc);
	if (m_nType == SPS_VERTICAL)
	{	
		int nMidX = (rc.left + rc.right) / 2;
		int dx = pt.x - nMidX;
		rc.OffsetRect(dx, 0);
	}
	else
	{	
		int nMidY = (rc.top + rc.bottom) / 2;
		int dy = pt.y - nMidY;
		rc.OffsetRect(0, dy);
	}
	MoveWindow(rc);
}

void CSplitterControl::ChangeWidth(CWnd *pWnd, int dx, DWORD dwFlag)
{
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		if (dwFlag == CW_LEFTALIGN)
			rcWnd.right += dx;
		else if (dwFlag == CW_RIGHTALIGN)
			rcWnd.left -= dx;
		pWnd->MoveWindow(rcWnd);
	}
}

void CSplitterControl::ChangeHeight(CWnd *pWnd, int dy, DWORD dwFlag)
{
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		if (dwFlag == CW_TOPALIGN)
			rcWnd.bottom += dy;
		else if (dwFlag == CW_BOTTOMALIGN)
			rcWnd.top -= dy;
		pWnd->MoveWindow(rcWnd);
	}
}

void CSplitterControl::ChangePos(CWnd* pWnd, int dx, int dy)
{
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		rcWnd.OffsetRect(-dx, dy);

		pWnd->MoveWindow(rcWnd);
	}	
}

void CSplitterControl::SetRange(int nMin, int nMax)
{
	m_nMin = nMin;
	m_nMax = nMax;
}

// Set splitter range from (nRoot - nSubtraction) to (nRoot + nAddition)
// If (nRoot < 0)
//		nRoot =  <current position of the splitter>
void CSplitterControl::SetRange(int nSubtraction, int nAddition, int nRoot)
{
	if (nRoot < 0)
	{
		CRect rcWnd;
		GetWindowRect(rcWnd);
		if (m_nType == SPS_VERTICAL)
			nRoot = rcWnd.left + rcWnd.Width() / 2;
		else // if m_nType == SPS_HORIZONTAL
			nRoot = rcWnd.top + rcWnd.Height() / 2;
	}
	m_nMin = nRoot - nSubtraction;
	m_nMax = nRoot + nAddition;
}
void CSplitterControl::PreSubclassWindow()
{
	// Enable notifications - CStatic has this disabled by default
	DWORD dwStyle = GetStyle();
	::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	CRect rc;
	GetClientRect(rc);

	// Determine default type base on it's size.
	m_nType = (rc.Width() < rc.Height())?SPS_VERTICAL:SPS_HORIZONTAL;

	if (!SplitterControl_hCursor1)
	{
		SplitterControl_hCursor1 = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
		SplitterControl_hCursor2 = AfxGetApp()->LoadStandardCursor(IDC_SIZENS);
	}

	// force the splitter not to be splitted.
	SetRange(0, 0, -1);


	CStatic::PreSubclassWindow();
}

BOOL CSplitterControl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}
