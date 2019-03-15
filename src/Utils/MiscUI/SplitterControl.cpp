// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2003-2006, 2008-2013, 2017 - TortoiseSVN

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
#include "SplitterControl.h"
#include "MyMemDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl

// hCursor1 is for vertical one
// and hCursor2 is for horizontal one
static HCURSOR SplitterControl_hCursor1 = nullptr;
static HCURSOR SplitterControl_hCursor2 = nullptr;

CSplitterControl::CSplitterControl()
	: m_bIsPressed(false)
	, m_nMin(-1)
	, m_nMax(-1)
	, m_bMouseOverControl(false)
	, m_nType(0)
	, m_nX(0)
	, m_nY(0)
	, m_nSavePos(0)
{
	m_AnimVarHot = Animator::Instance().CreateAnimationVariable(0.0);

	// GDI+ initialization
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartup(&m_gdiPlusToken, &input, nullptr);
}

CSplitterControl::~CSplitterControl()
{
	// GDI+ cleanup
	Gdiplus::GdiplusShutdown(m_gdiPlusToken);
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
	CPaintDC dcreal(this); // device context for painting
	{
		CRect rcClient;
		GetClientRect(rcClient);
		Gdiplus::Rect rc(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height());
		Gdiplus::Graphics g(dcreal);

		Gdiplus::Color c1;
		c1.SetFromCOLORREF(GetSysColor(COLOR_3DFACE));
		Gdiplus::Color c2;
		c2.SetFromCOLORREF(GetSysColor(COLOR_BTNSHADOW));

		Gdiplus::SolidBrush bkgBrush(c1);
		g.FillRectangle(&bkgBrush, rc);

		// m_AnimVarHot changes from 0.0 (not hot) to 1.0 (hot)
		auto alpha = Animator::GetValue(m_AnimVarHot);
		c1.SetValue(Gdiplus::Color::MakeARGB(BYTE(alpha*255.0), c1.GetRed(), c1.GetBlue(), c1.GetGreen()));
		c2.SetValue(Gdiplus::Color::MakeARGB(BYTE(alpha*255.0), c2.GetRed(), c2.GetBlue(), c2.GetGreen()));

		if (m_nType == SPS_VERTICAL)
		{
			Gdiplus::LinearGradientBrush b1(Gdiplus::Point(rc.GetLeft(), rc.GetBottom()), Gdiplus::Point(rc.GetLeft() + rc.Width/2, rc.GetBottom()), c1, c2);

			Gdiplus::LinearGradientBrush b2(Gdiplus::Point(rc.GetLeft() + rc.Width / 2, rc.GetBottom()), Gdiplus::Point(rc.GetRight(), rc.GetBottom()), c2, c1);

			g.FillRectangle(&b1, Gdiplus::Rect(rcClient.left, rcClient.top, rcClient.Width()/2, rcClient.Height()));
			g.FillRectangle(&b2, Gdiplus::Rect(rcClient.left+rcClient.Width()/2, rcClient.top, rcClient.Width()/2, rcClient.Height()));
		}
		else
		{
			Gdiplus::LinearGradientBrush b1(Gdiplus::Point(rc.GetLeft(), rc.GetBottom()), Gdiplus::Point(rc.GetLeft(), rc.GetTop() + rc.Height / 2), c1, c2);

			Gdiplus::LinearGradientBrush b2(Gdiplus::Point(rc.GetLeft(), rc.GetTop() + rc.Height / 2), Gdiplus::Point(rc.GetLeft(), rc.GetTop()), c2, c1);

			g.FillRectangle(&b1, Gdiplus::Rect(rcClient.left, rcClient.top + rcClient.Height() / 2, rcClient.Width(), rcClient.Height()));
			g.FillRectangle(&b2, Gdiplus::Rect(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height() / 2));
		}


		//dc.SetBkColor(GetSysColor(COLOR_3DFACE));
		//dc.ExtTextOut(0, 0, ETO_OPAQUE, &rcClient, NULL, 0, NULL);

		//auto c2 = ::GetSysColor(COLOR_BTNSHADOW);
		//auto c1 = ::GetSysColor(COLOR_3DFACE);

		//// m_AnimVarHot changes from 0.0 (not hot) to 1.0 (hot)
		//auto fraction = Animator::GetValue(m_AnimVarHot);

		//int r1 = static_cast<int>(GetRValue(c1)); int g1 = static_cast<int>(GetGValue(c1)); int b1 = static_cast<int>(GetBValue(c1));
		//int r2 = static_cast<int>(GetRValue(c2)); int g2 = static_cast<int>(GetGValue(c2)); int b2 = static_cast<int>(GetBValue(c2));
		//auto clr = RGB((r2 - r1)*fraction + r1, (g2 - g1)*fraction + g1, (b2 - b1)*fraction + b1);

		//CBrush brush;
		//brush.CreateHatchBrush(HS_DIAGCROSS, clr);

		//auto oldBrush = dc.SelectObject(&brush);
		//rcClient.DeflateRect(1, 1, 1, 1);
		//dc.FillRect(&rcClient, &brush);
		//dc.SelectObject(&oldBrush);
	}
}

void CSplitterControl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bIsPressed)
	{
		CWnd * pParent = GetParent();
		{
			CPoint pt = point;
			ClientToScreen(&pt);
			pParent->ScreenToClient(&pt);

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
		}
		CPoint pt(m_nX, m_nY);
		CWnd* pOwner = GetOwner();
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
			nmsp.hdr.idFrom = GetDlgCtrlID();
			nmsp.hdr.code = SPN_SIZED;
			nmsp.delta = delta;

			pOwner->SendMessage(WM_NOTIFY, nmsp.hdr.idFrom, reinterpret_cast<LPARAM>(&nmsp));
			if (m_nType == SPS_VERTICAL)
				m_nSavePos = m_nX;
			else
				m_nSavePos = m_nY;
			pOwner->Invalidate();
		}

	}
	else if (!m_bMouseOverControl)
	{
		TRACKMOUSEEVENT Tme;
		Tme.cbSize = sizeof(TRACKMOUSEEVENT);
		Tme.dwFlags = TME_LEAVE;
		Tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&Tme);

		m_bMouseOverControl = true;
		auto transHot = Animator::Instance().CreateLinearTransition(0.3, 1.0);
		auto storyBoard = Animator::Instance().CreateStoryBoard();
		if (storyBoard && transHot && m_AnimVarHot)
		{
			storyBoard->AddTransition(m_AnimVarHot, transHot);
			Animator::Instance().RunStoryBoard(storyBoard, [this]()
			{
				InvalidateRect(nullptr, false);
			});
		}
	}
	CStatic::OnMouseMove(nFlags, point);
}

LRESULT CSplitterControl::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_bMouseOverControl)
	{
		auto transHot = Animator::Instance().CreateLinearTransition(0.3, 0.0);
		auto storyBoard = Animator::Instance().CreateStoryBoard();
		if (storyBoard && transHot && m_AnimVarHot)
		{
			storyBoard->AddTransition(m_AnimVarHot, transHot);
			Animator::Instance().RunStoryBoard(storyBoard, [this]()
			{
				InvalidateRect(nullptr, false);
			});
		}
	}
	m_bMouseOverControl = false;
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

	m_bIsPressed = true;
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
}

void CSplitterControl::OnLButtonUp(UINT nFlags, CPoint point)
{
	CStatic::OnLButtonUp(nFlags, point);
	m_bIsPressed = false;
	ReleaseCapture();
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

HDWP CSplitterControl::ChangeRect(HDWP hdwp, CWnd * pWnd, int dleft, int dtop, int dright, int dbottom)
{
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		rcWnd.left += dleft;
		rcWnd.top += dtop;
		rcWnd.right += dright;
		rcWnd.bottom += dbottom;

		return DeferWindowPos(hdwp, pWnd->GetSafeHwnd(), nullptr, rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER);
		//pWnd->MoveWindow(rcWnd);
	}
	return hdwp;
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
	ModifyStyle(0, SS_NOTIFY);

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

