// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "Balloon.h"

tagBALLOON_INFO::tagBALLOON_INFO()
    : hIcon(NULL),			
	  sBalloonTip(),
	  nMask(0),
	  nStyles(0),
	  nDirection(0),
	  nEffect(0),
	  nBehaviour(0),
	  crBegin(0),
	  crMid(0),
	  crEnd(0)
{
}

CBalloon::CBalloon()
    : m_nStyles (0)
{
	m_pParentWnd = NULL;
	m_hCurrentWnd = NULL;
	m_hDisplayedWnd = NULL;

	m_rgnShadow.CreateRectRgn(0, 0, 1, 1);
	m_rgnBalloon.CreateRectRgn(0, 0, 1, 1);

	m_ptOriginal.x = -1;
	m_ptOriginal.y = -1;

	SetDelayTime(TTDT_INITIAL, 500);
	SetDelayTime(TTDT_AUTOPOP, 30000);
	SetNotify(FALSE);
	SetDirection();
	SetBehaviour();
	SetDefaultStyles();
	SetDefaultColors();
	SetDefaultSizes();
	SetEffectBk(BALLOON_EFFECT_SOLID);
	RemoveAllTools();
	m_bButtonPushed = FALSE;

	// Register the window class if it has not already been registered.
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
	if(!(::GetClassInfo(hInst, BALLOON_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style			= CS_SAVEBITS;
		wndcls.lpfnWndProc		= ::DefWindowProc;
		wndcls.cbClsExtra		= wndcls.cbWndExtra = 0;
		wndcls.hInstance		= hInst;
		wndcls.hIcon			= NULL;
		wndcls.hCursor			= LoadCursor(hInst, IDC_ARROW );
		wndcls.hbrBackground	= NULL;
		wndcls.lpszMenuName		= NULL;
		wndcls.lpszClassName	= BALLOON_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}
}

CBalloon::~CBalloon()
{
	RemoveAllTools();

	m_rgnBalloon.DeleteObject();
	m_rgnShadow.DeleteObject();

	if (IsWindow(m_hWnd))
        DestroyWindow();
}


BEGIN_MESSAGE_MAP(CBalloon, CWnd)
	//{{AFX_MSG_MAP(CBalloon)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBalloon message handlers

BOOL CBalloon::Create(CWnd* pParentWnd) 
{
	DWORD dwStyle = WS_POPUP; 
	DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;

	m_pParentWnd = pParentWnd;

	if (!CreateEx(dwExStyle, BALLOON_CLASSNAME, NULL, dwStyle, 0, 0, 0, 0, m_pParentWnd->GetSafeHwnd(), NULL, NULL))
	{
		return FALSE;
	}
	SetDefaultFont();
	
	return TRUE;
}

void CBalloon::OnDestroy() 
{
	TRACE("OnDestroy()\n");
	KillTimers();
	
	CWnd::OnDestroy();
}



void CBalloon::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);
	Pop();
}


BOOL CBalloon::PreTranslateMessage(MSG* pMsg) 
{
	RelayEvent(pMsg);

	return CWnd::PreTranslateMessage(pMsg);
}

LRESULT CBalloon::SendNotify(CWnd * pWnd, CPoint * pt, BALLOON_INFO & bi)
{
	//make sure this is a valid window
	if (!IsWindow(GetSafeHwnd()))
		return 0L;

	//see if the user wants to be notified
	if (!GetNotify())
		return 0L;

	NM_BALLOON_DISPLAY lpnm;
	
	lpnm.pWnd		  = pWnd;
	lpnm.pt			  = pt;
	lpnm.bi			  = &bi;
	lpnm.hdr.hwndFrom = m_hWnd;
    lpnm.hdr.idFrom   = GetDlgCtrlID();
    lpnm.hdr.code     = UDM_TOOLTIP_DISPLAY;
	
	::SendMessage(m_hNotifyWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);

	return 0L;
}

void CBalloon::OnPaint() 
{
	//if (!m_pCurrentWnd)
	//	return;

	m_hDisplayedWnd = m_hCurrentWnd;

	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(&rect);
	rect.DeflateRect(0, 0, 1, 1);

	//create a memory device-context. This is done to help reduce
	//screen flicker, since we will paint the entire control to the
	//off screen device context first.CDC memDC;
	CDC memDC;
	CBitmap bitmap;
	memDC.CreateCompatibleDC(&dc);
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap); 
	
	memDC.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY);
	
	//draw the tooltip
	OnDraw(&memDC, rect);

	//Copy the memory device context back into the original DC.
	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, 0,0, SRCCOPY);
	
	//Cleanup resources.
	memDC.SelectObject(pOldBitmap);
	memDC.DeleteDC();
	bitmap.DeleteObject(); 
}

void CBalloon::OnDraw(CDC * pDC, CRect rect)
{
	CBrush brBackground(m_crColor [BALLOON_COLOR_BK_BEGIN]);
	CBrush brShadow(m_crColor [BALLOON_COLOR_SHADOW]);
	CBrush brBorder(m_crColor [BALLOON_COLOR_BORDER]);
	
	pDC->SetBkMode(TRANSPARENT); 
	pDC->SetTextColor(m_crColor [BALLOON_COLOR_FG]);
	//set clip region of the tooltip and draw the shadow if needed
	if (m_pToolInfo.nStyles & BALLOON_SHADOW)
	{
		//draw the shadow for the tooltip
		int nRop2Mode = pDC->SetROP2(R2_MASKPEN);
		pDC->FillRgn(&m_rgnShadow, &brShadow);
		pDC->SetROP2(nRop2Mode);
		rect.DeflateRect(0, 0, m_nSizes[XBLSZ_SHADOW_CX], m_nSizes[XBLSZ_SHADOW_CY]);
	}
	pDC->SelectClipRgn(&m_rgnBalloon);

	OnDrawBackground(pDC, &rect);

	//draw the main region's border of the tooltip
	pDC->FrameRgn(&m_rgnBalloon, &brBorder, m_nSizes[XBLSZ_BORDER_CX], m_nSizes[XBLSZ_BORDER_CY]);

	if ((m_nLastDirection == BALLOON_RIGHT_BOTTOM) || (m_nLastDirection == BALLOON_LEFT_BOTTOM))
		rect.top += m_nSizes[XBLSZ_HEIGHT_ANCHOR];
	else
		rect.bottom -= m_nSizes[XBLSZ_HEIGHT_ANCHOR];

	if (m_pToolInfo.nStyles & BALLOON_CLOSEBUTTON)
	{
		m_rtCloseButton = CRect(
			rect.right - m_szCloseButton.cx - m_nSizes[XBLSZ_BUTTON_MARGIN_CX] , rect.top + m_nSizes[XBLSZ_BUTTON_MARGIN_CY], 
			rect.right - m_nSizes[XBLSZ_BUTTON_MARGIN_CX], rect.top + m_szCloseButton.cy + m_nSizes[XBLSZ_BUTTON_MARGIN_CY]);
		pDC->DrawFrameControl(m_rtCloseButton, DFC_CAPTION, DFCS_CAPTIONCLOSE|DFCS_FLAT|DFCS_TRANSPARENT);
		rect.right -= (m_szCloseButton.cx + m_nSizes[XBLSZ_BUTTON_MARGIN_CX]);
	}

	//get the rectangle to draw the tooltip text
	rect.DeflateRect(m_nSizes[XBLSZ_MARGIN_CX], m_nSizes[XBLSZ_MARGIN_CY]);

	//draw the icon
	if (m_pToolInfo.hIcon != NULL)
	{
		DrawIconEx(pDC->m_hDC, m_nSizes[XBLSZ_MARGIN_CX], rect.top + (rect.Height() - m_szBalloonIcon.cy) / 2, 
			m_pToolInfo.hIcon, m_szBalloonIcon.cx, m_szBalloonIcon.cy, 0, NULL, DI_NORMAL);

		rect.left += m_szBalloonIcon.cx + m_nSizes[XBLSZ_MARGIN_CX]; 
	}


	//aligns tool tip's text
	if (m_pToolInfo.nStyles & BALLOON_BOTTOM_ALIGN)
		rect.top = rect.bottom - m_szBalloonText.cy;
	else if (m_pToolInfo.nStyles & BALLOON_VCENTER_ALIGN)
		rect.top += (rect.Height() - m_szBalloonText.cy) / 2;

	//prints the tool tip's text
	DrawHTML(pDC, rect, m_pToolInfo.sBalloonTip, m_LogFont, FALSE);

	//free resources
	brBackground.DeleteObject();
	brShadow.DeleteObject();
	brBorder.DeleteObject();
}

void CBalloon::OnDrawBackground(CDC * pDC, CRect * pRect)
{
#ifdef USE_GDI_GRADIENT
	#define	DRAW CGradient::DrawGDI
#else
	#define DRAW CGradient::Draw
#endif
	switch (m_pToolInfo.nEffect)
	{
	case BALLOON_EFFECT_HGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_END]);
		break;
	case BALLOON_EFFECT_VGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_END], FALSE);
		break;
	case BALLOON_EFFECT_HCGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_END], m_crColor[BALLOON_COLOR_BK_BEGIN]);
		break;
	case BALLOON_EFFECT_VCGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_END], m_crColor[BALLOON_COLOR_BK_BEGIN], FALSE);
		break;
	case BALLOON_EFFECT_3HGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_MID], m_crColor[BALLOON_COLOR_BK_END]);
		break;
	case BALLOON_EFFECT_3VGRADIENT:
		DRAW(pDC, pRect, m_crColor[BALLOON_COLOR_BK_BEGIN], m_crColor[BALLOON_COLOR_BK_MID], m_crColor[BALLOON_COLOR_BK_END], FALSE);
		break;
#undef DRAW
	default:
		pDC->FillSolidRect(pRect, m_crColor[BALLOON_COLOR_BK_BEGIN]);
		break;
	}
}

CRect CBalloon::GetWindowRegion(CRgn * rgn, CSize sz, CPoint pt) const
{
	CRect rect;
	rect.SetRect(0, 0, sz.cx, sz.cy);
	CRgn rgnRect;
	CRgn rgnAnchor;
	CPoint ptAnchor [3];
	ptAnchor [0] = pt;
	ScreenToClient(&ptAnchor [0]);

	switch (m_nLastDirection)
	{
	case BALLOON_LEFT_TOP:
	case BALLOON_RIGHT_TOP:
		rect.bottom -= m_nSizes[XBLSZ_HEIGHT_ANCHOR];
		ptAnchor [1].y = ptAnchor [2].y = rect.bottom;
		break;
	case BALLOON_LEFT_BOTTOM:
	case BALLOON_RIGHT_BOTTOM:
		rect.top += m_nSizes[XBLSZ_HEIGHT_ANCHOR];
		ptAnchor [1].y = ptAnchor [2].y = rect.top;
		break;
	}

	//get the region for rectangle with the text
	if (m_pToolInfo.nStyles & BALLOON_ROUNDED)
		rgnRect.CreateRoundRectRgn(rect.left, rect.top, rect.right + 1, rect.bottom + 1, 
			m_nSizes[XBLSZ_ROUNDED_CX], m_nSizes[XBLSZ_ROUNDED_CY]);
	else rgnRect.CreateRectRgn(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

	//gets the region for the anchor
	if (m_pToolInfo.nStyles & BALLOON_ANCHOR)
	{
		switch (m_nLastDirection)
		{
		case BALLOON_LEFT_TOP:
		case BALLOON_LEFT_BOTTOM:
			ptAnchor [1].x = rect.right - m_nSizes[XBLSZ_MARGIN_ANCHOR];
			ptAnchor [2].x = ptAnchor [1].x - m_nSizes[XBLSZ_WIDTH_ANCHOR];
			break;
		case BALLOON_RIGHT_TOP:
		case BALLOON_RIGHT_BOTTOM:
			ptAnchor [1].x = rect.left + m_nSizes[XBLSZ_MARGIN_ANCHOR];
			ptAnchor [2].x = ptAnchor [1].x + m_nSizes[XBLSZ_WIDTH_ANCHOR];
			break;
		}
		rgnAnchor.CreatePolygonRgn(ptAnchor, 3, ALTERNATE);
	}
	else
		rgnAnchor.CreateRectRgn(0, 0, 0, 0);
	
	rgn->CreateRectRgn(0, 0, 1, 1);
	rgn->CombineRgn(&rgnRect, &rgnAnchor, RGN_OR);
	
	rgnAnchor.DeleteObject();
	rgnRect.DeleteObject();

	return rect;
}

void CBalloon::RelayEvent(MSG* pMsg)
{
	HWND hWnd = NULL;
	CPoint pt;
	CString str;
	CRect rect;

	BALLOON_INFO  Info;
		
	switch(pMsg->message)
	{
	case WM_MOUSEMOVE:
		if (m_ptOriginal == pMsg->pt)
			return;

		m_ptOriginal = pMsg->pt; 
		
		//get the real window under the mouse pointer
		pt = pMsg->pt;
		if (m_pParentWnd)
			m_pParentWnd->ScreenToClient(&pt);
		hWnd = GetChildWindowFromPoint(pt);

		if (!hWnd)
		{
			if (!(GetBehaviour() & BALLOON_DIALOG))
			{
				Pop();
				m_hCurrentWnd = NULL;
				m_hDisplayedWnd = NULL;
				return;
			}
		}
		else
		{
			UINT behaviour = GetBehaviour(CWnd::FromHandle(hWnd));
			if (hWnd == m_hDisplayedWnd)
			{
				if (IsWindowVisible())
				{
					if ((behaviour & BALLOON_TRACK_MOUSE)&&(!(behaviour & BALLOON_DIALOG)))
					{
						//mouse moved, so move the tooltip too
						CRect rect;
						GetWindowRect(rect);
						CalculateInfoBoxRect(&m_ptOriginal, &rect);
						SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
					}
					else
						return;
				}
				else if ((behaviour & BALLOON_MULTIPLE_SHOW)&&(!(behaviour & BALLOON_DIALOG)))
				{
					SetNewToolTip(CWnd::FromHandle(hWnd));
				}
				else
					Pop();
			}
			else
			{
				SetNewToolTip(CWnd::FromHandle(hWnd));
			}
		}
		break;
	}
}

void CBalloon::SetNewToolTip(CWnd * pWnd)
{
	m_hDisplayedWnd = NULL;
	Pop();

	if (!pWnd->IsWindowEnabled())
		return;

	if (!GetTool(pWnd, m_pToolInfo))
		return;

	m_hCurrentWnd = pWnd->GetSafeHwnd();

	SetTimer(BALLOON_SHOW, m_nTimeInitial, NULL);
}

void CBalloon::OnTimer(UINT_PTR nIDEvent) 
{
	CPoint pt, point;
	CString str;
	HWND hWnd;

	switch (nIDEvent)
	{
	case BALLOON_SHOW:
		KillTimers(BALLOON_SHOW);
		//check if mouse pointer is still over the right window
		GetCursorPos(&pt);
		point = pt;
		if (m_pParentWnd)
			m_pParentWnd->ScreenToClient(&point);
		hWnd = GetChildWindowFromPoint(point);
		if (hWnd == m_hCurrentWnd)
		{
			DisplayToolTip(&pt);
			SetTimer(BALLOON_HIDE, m_nTimeAutoPop, NULL);
		}
		break;
	case BALLOON_HIDE:
		KillTimers(BALLOON_HIDE);
		Pop();
		if (GetBehaviour() & BALLOON_DIALOG_DESTROY)
		{
			CWnd::OnTimer(nIDEvent);
			DestroyWindow();
			return;
		}
		break;
	}
	
	CWnd::OnTimer(nIDEvent);
}

HWND CBalloon::GetChildWindowFromPoint(CPoint & point) const
{
	if (!m_pParentWnd)
		return NULL;
    CPoint pt = point;
    m_pParentWnd->ClientToScreen(&pt);
    HWND hWnd = ::WindowFromPoint(pt);

    //::WindowFromPoint misses disabled windows and such - go for a more
    //comprehensive search in this case.
    if (hWnd == m_pParentWnd->GetSafeHwnd())
        hWnd = m_pParentWnd->ChildWindowFromPoint(point, CWP_ALL)->GetSafeHwnd();

    //check that we aren't over the parent or out of client area
    if (!hWnd || hWnd == m_pParentWnd->GetSafeHwnd())
        return NULL;

    //if it's not part of the main parent window hierarchy, then we are
    //not interested
    if (!::IsChild(m_pParentWnd->GetSafeHwnd(), hWnd))
        return NULL;

    return hWnd;
}

BOOL CBalloon::IsCursorInToolTip() const
{
    if (!IsVisible() || !IsWindow(m_hWnd))
	   return FALSE;

    CPoint pt;
    GetCursorPos(&pt);

	CBalloon * pWnd = (CBalloon*)WindowFromPoint(pt);

	return (pWnd == this);
}

void CBalloon::KillTimers(UINT nIDTimer /* = NULL */)
{
	if (nIDTimer == NULL)
	{
		KillTimer(BALLOON_SHOW);
		KillTimer(BALLOON_HIDE);
	}
	else if (nIDTimer == BALLOON_SHOW)
		KillTimer(BALLOON_SHOW);
	else if (nIDTimer == BALLOON_HIDE)
		KillTimer(BALLOON_HIDE);
}

void CBalloon::DisplayToolTip(CPoint * pt /* = NULL */)
{
	if(!GetTool(CWnd::FromHandle(m_hCurrentWnd), m_pToolInfo) || m_pToolInfo.sBalloonTip.IsEmpty())
		return;
	//if a mask is set then use the default values for the tooltip
	if (!(m_pToolInfo.nMask & BALLOON_MASK_STYLES))
		m_pToolInfo.nStyles = m_nStyles;
	if (!(m_pToolInfo.nMask & BALLOON_MASK_EFFECT))
	{
		m_pToolInfo.nEffect = m_nEffect;
	}
	if (!(m_pToolInfo.nMask & BALLOON_MASK_COLORS))
	{
		m_pToolInfo.crBegin = m_crColor[BALLOON_COLOR_BK_BEGIN];
		m_pToolInfo.crMid = m_crColor[BALLOON_COLOR_BK_MID];
		m_pToolInfo.crEnd = m_crColor[BALLOON_COLOR_BK_END];
	}
	if (!(m_pToolInfo.nMask & BALLOON_MASK_DIRECTION))
		m_pToolInfo.nDirection = m_nDirection;

	//send notification
	SendNotify(CWnd::FromHandle(m_hCurrentWnd), pt, m_pToolInfo);

	//calculate the width and height of the box dynamically
    CSize sz = GetTooltipSize(m_pToolInfo.sBalloonTip);
	m_szBalloonText = sz;
	
	//get the size of the current icon
	m_szBalloonIcon = GetSizeIcon(m_pToolInfo.hIcon);
	if (m_szBalloonIcon.cx || m_szBalloonIcon.cy)
	{
		sz.cx += m_szBalloonIcon.cx + m_nSizes[XBLSZ_MARGIN_CX];
		sz.cy = max(m_szBalloonIcon.cy, sz.cy);
	}

	//get the size of the close button
	m_szCloseButton = CSize(::GetSystemMetrics(SM_CXMENUSIZE), ::GetSystemMetrics(SM_CYMENUSIZE));
	if (m_pToolInfo.nStyles & BALLOON_CLOSEBUTTON)
	{
		sz.cx += m_szCloseButton.cx + m_nSizes[XBLSZ_BUTTON_MARGIN_CX];
	}

	//get size of the tooltip with margins
	sz.cx += m_nSizes[XBLSZ_MARGIN_CX] * 2;
	sz.cy += m_nSizes[XBLSZ_MARGIN_CY] * 2 + m_nSizes[XBLSZ_HEIGHT_ANCHOR];
	if (m_pToolInfo.nStyles & BALLOON_SHADOW)
	{
		sz.cx += m_nSizes[XBLSZ_SHADOW_CX];
		sz.cy += m_nSizes[XBLSZ_SHADOW_CY];
	}
	
	CRect rect (0, 0, sz.cx, sz.cy);
	
	DisplayToolTip(pt, &rect);
}

void CBalloon::DisplayToolTip(CPoint * pt, CRect * rect)
{
	CalculateInfoBoxRect(pt, rect);

	SetWindowPos(
		NULL, rect->left, rect->top, rect->Width() + 2, rect->Height() + 2,
		SWP_SHOWWINDOW|SWP_NOCOPYBITS|SWP_NOACTIVATE|SWP_NOZORDER);

	CRgn rgn;
	rgn.CreateRectRgn(0, 0, 1, 1);
	if (m_pToolInfo.nStyles & BALLOON_SHADOW)
	{
		rect->right -= m_nSizes[XBLSZ_SHADOW_CX];
		rect->bottom -= m_nSizes[XBLSZ_SHADOW_CY];
	}

	m_rgnBalloon.DeleteObject();
	GetWindowRegion(&m_rgnBalloon, CSize (rect->Width(), rect->Height()), *pt);
	rgn.CopyRgn(&m_rgnBalloon);
	if (m_pToolInfo.nStyles & BALLOON_SHADOW)
	{
		m_rgnShadow.DeleteObject();
		m_rgnShadow.CreateRectRgn(0, 0, 1, 1);
		m_rgnShadow.CopyRgn(&m_rgnBalloon);
		m_rgnShadow.OffsetRgn(m_nSizes[XBLSZ_SHADOW_CX], m_nSizes[XBLSZ_SHADOW_CY]);
		rgn.CombineRgn(&rgn, &m_rgnShadow, RGN_OR);
	}
	SetWindowRgn(rgn, FALSE);

	rgn.DeleteObject();
}

void CBalloon::Pop()
{
	KillTimers();	
	ShowWindow(SW_HIDE);
	m_bButtonPushed = FALSE;
}

CSize CBalloon::GetTooltipSize(const CString& str)
{
	CRect rect;
	GetWindowRect(&rect);

	CDC * pDC = GetDC();

	CDC memDC;
	CBitmap bitmap;
	memDC.CreateCompatibleDC(pDC);
	bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

	//get the minimum size of the rectangle of the tooltip
	CSize sz = DrawHTML(&memDC, rect, str, m_LogFont, TRUE);

	memDC.SelectObject(pOldBitmap);
	memDC.DeleteDC();
	bitmap.DeleteObject();

	ReleaseDC(pDC);

	return sz;
}

CSize CBalloon::GetSizeIcon(HICON hIcon) const
{
	ICONINFO ii;
	CSize sz (0, 0);

	if (hIcon != NULL)
	{
		//get icon dimensions
		::SecureZeroMemory(&ii, sizeof(ICONINFO));
		if (::GetIconInfo(hIcon, &ii))
		{
			sz.cx = (DWORD)(ii.xHotspot * 2);
			sz.cy = (DWORD)(ii.yHotspot * 2);
			//release icon mask bitmaps
			if(ii.hbmMask)
				::DeleteObject(ii.hbmMask);
			if(ii.hbmColor)
				::DeleteObject(ii.hbmColor);
		}
	}
	return sz;
}

void CBalloon::CalculateInfoBoxRect(CPoint * pt, CRect * rect)
{
	CRect monitorRect;
	GetMonitorWorkArea(*pt, monitorRect);

	CPoint ptEnd;
	m_nLastDirection = m_pToolInfo.nDirection;
	BOOL horzAdjusted = TestHorizDirection(pt->x, rect->Width(), monitorRect, m_nLastDirection, rect);
	if (!horzAdjusted)
	{
		m_nLastDirection = GetNextHorizDirection(m_nLastDirection);
		horzAdjusted = TestHorizDirection(pt->x, rect->Width(), monitorRect, m_nLastDirection, rect);
	}
	BOOL vertAdjusted = TestVertDirection(pt->y, rect->Height(), monitorRect, m_nLastDirection, rect);
	if (!vertAdjusted)
	{
		m_nLastDirection = GetNextVertDirection(m_nLastDirection);
		vertAdjusted = TestVertDirection(pt->y, rect->Height(), monitorRect, m_nLastDirection, rect);
	}
	// in case the rectangle wasn't adjusted which can happen if the tooltip is
	// larger than half the monitor size, we center the tooltip around the mouse pointer
	if (!horzAdjusted)
	{
		int cx = rect->Width() / 2;
		rect->right = pt->x + cx;
		rect->left = pt->x - cx;
	}
	if (!vertAdjusted)
	{
		int cy = rect->Height() / 2;
		rect->bottom = pt->y + cy;
		rect->top = pt->y - cy;
	}
	if ((m_pToolInfo.nStyles & BALLOON_SHADOW) && 
		((m_nLastDirection == BALLOON_LEFT_TOP) || (m_nLastDirection == BALLOON_LEFT_BOTTOM)))
		rect->OffsetRect(m_nSizes[XBLSZ_SHADOW_CX], m_nSizes[XBLSZ_SHADOW_CY]);
}


int CBalloon::GetNextHorizDirection(int nDirection) const
{
	switch (nDirection)
	{
	case BALLOON_LEFT_TOP:
		nDirection = BALLOON_RIGHT_TOP;
		break;
	case BALLOON_RIGHT_TOP:
		nDirection = BALLOON_LEFT_TOP;
		break;
	case BALLOON_LEFT_BOTTOM:
		nDirection = BALLOON_RIGHT_BOTTOM;
		break;
	case BALLOON_RIGHT_BOTTOM:
		nDirection = BALLOON_LEFT_BOTTOM;
		break;
	}
	return nDirection;
}

int CBalloon::GetNextVertDirection(int nDirection) const
{
	switch (nDirection)
	{
	case BALLOON_LEFT_TOP:
		nDirection = BALLOON_LEFT_BOTTOM;
		break;
	case BALLOON_LEFT_BOTTOM:
		nDirection = BALLOON_LEFT_TOP;
		break;
	case BALLOON_RIGHT_TOP:
		nDirection = BALLOON_RIGHT_BOTTOM;
		break;
	case BALLOON_RIGHT_BOTTOM:
		nDirection = BALLOON_RIGHT_TOP;
		break;
	}
	return nDirection;
}

BOOL CBalloon::TestHorizDirection(int x, int cx, const CRect& monitorRect,
								  int nDirection, LPRECT rect)
{
	int left = 0;
	int right = 0;
	int anchorMarginSize = (int)m_nSizes[XBLSZ_MARGIN_ANCHOR];

	switch (nDirection)
	{
	case BALLOON_LEFT_TOP:
	case BALLOON_LEFT_BOTTOM:
		right = ((x + anchorMarginSize) > monitorRect.right) ? monitorRect.right : (x + anchorMarginSize);
		left = right - cx;
		break;
	case BALLOON_RIGHT_TOP:
	case BALLOON_RIGHT_BOTTOM:
		left = (x - anchorMarginSize)<monitorRect.left ? monitorRect.left : (x - anchorMarginSize);
		right = left + cx;
		break;
	}

	BOOL bTestOk = ((left >= monitorRect.left) && (right <= monitorRect.right)) ? TRUE : FALSE;
	if (bTestOk)
	{
		rect->left = left;
		rect->right = right;
	}

	return bTestOk;
}

BOOL CBalloon::TestVertDirection(int y, int cy, const CRect& monitorRect,
								 int nDirection, LPRECT rect)
{
	int top = 0;
	int bottom = 0;

	switch (nDirection)
	{
	case BALLOON_LEFT_TOP:
	case BALLOON_RIGHT_TOP:
		bottom = y;
		top = bottom - cy;
		break;
	case BALLOON_LEFT_BOTTOM:
	case BALLOON_RIGHT_BOTTOM:
		top = y;
		bottom = top + cy;
		break;
	}

	BOOL bTestOk = ((top >= monitorRect.top) && (bottom <= monitorRect.bottom)) ? TRUE : FALSE;
	if (bTestOk)
	{
		rect->top = top;
		rect->bottom = bottom;
	}

	return bTestOk;
}

LPLOGFONT CBalloon::GetSystemToolTipFont() const
{
    static LOGFONT LogFont;

    NONCLIENTMETRICS ncm;
	OSVERSIONINFO vers;
	
	memset(&vers, 0, sizeof(OSVERSIONINFO));
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	vers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		
	if(!GetVersionEx(&vers))
		return NULL;

	if(vers.dwMajorVersion < 6)
	{
		ncm.cbSize -= sizeof(ncm.iPaddedBorderWidth);
	}

    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
        return NULL;

    memcpy(&LogFont, &(ncm.lfStatusFont), sizeof(LOGFONT));

    return &LogFont; 
}


void CBalloon::Redraw(BOOL /*bRedraw*/ /* = TRUE */)
{
}

void CBalloon::SetStyles(DWORD nStyles, CWnd * pWnd /* = NULL */)
{
	ModifyStyles(nStyles, (DWORD)-1, pWnd);
}

void CBalloon::ModifyStyles(DWORD nAddStyles, DWORD nRemoveStyles, CWnd * pWnd /* = NULL */)
{
	if (!pWnd)
	{
		m_nStyles &= ~nRemoveStyles;
		m_nStyles |= nAddStyles;
	}
	else
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (!(bi.nMask & BALLOON_MASK_STYLES))
				bi.nStyles = m_nStyles;
			bi.nStyles &= ~nRemoveStyles;
			bi.nStyles |= nAddStyles;
			bi.nMask |= BALLOON_MASK_STYLES;
			AddTool(pWnd, bi);
		}
	}
} 

DWORD CBalloon::GetStyles(CWnd * pWnd /* = NULL */) const
{
	if (pWnd)
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (bi.nMask & BALLOON_MASK_STYLES)
				return bi.nStyles;
		}
	}
	return m_nStyles;
} 

void CBalloon::SetDefaultStyles(CWnd * pWnd /* = NULL */)
{
	SetStyles(BALLOON_RSA, pWnd);
}

void CBalloon::SetColor(int nIndex, COLORREF crColor)
{
	if (nIndex >= BALLOON_MAX_COLORS)
		return;

	m_crColor [nIndex] = crColor;
}

COLORREF CBalloon::GetColor(int nIndex) const
{
	if (nIndex >= BALLOON_MAX_COLORS)
		nIndex = BALLOON_COLOR_FG;

	return m_crColor [nIndex];
}

void CBalloon::SetDefaultColors()
{
	SetColor(BALLOON_COLOR_FG, ::GetSysColor(COLOR_INFOTEXT));
	SetColor(BALLOON_COLOR_BK_BEGIN, ::GetSysColor(COLOR_INFOBK));
	SetColor(BALLOON_COLOR_BK_MID, ::GetSysColor(COLOR_INFOBK));
	SetColor(BALLOON_COLOR_BK_END, ::GetSysColor(COLOR_INFOBK));
	SetColor(BALLOON_COLOR_SHADOW, ::GetSysColor(COLOR_3DSHADOW));
	SetColor(BALLOON_COLOR_BORDER, ::GetSysColor(COLOR_WINDOWFRAME));
}

void CBalloon::SetGradientColors(COLORREF crBegin, COLORREF crMid, COLORREF crEnd, CWnd * pWnd /* = NULL */)
{
	if (!pWnd)
	{
		SetColor(BALLOON_COLOR_BK_BEGIN, crBegin);
		SetColor(BALLOON_COLOR_BK_MID, crMid);
		SetColor(BALLOON_COLOR_BK_END, crEnd);
	}
	else
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			bi.crBegin = crBegin;
			bi.crMid = crMid;
			bi.crEnd = crEnd;
			bi.nMask |= BALLOON_MASK_COLORS;
			AddTool(pWnd, bi);
		}
	}
}

void CBalloon::GetGradientColors(COLORREF & crBegin, COLORREF & crMid, COLORREF & crEnd, CWnd * pWnd /* = NULL */) const
{
	if (pWnd)
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (bi.nMask & BALLOON_MASK_COLORS)
			{
				crBegin = bi.crBegin;
				crMid = bi.crMid;
				crEnd = bi.crEnd;
				return;
			}
		}
	}
	crBegin = GetColor(BALLOON_COLOR_BK_BEGIN);
	crMid = GetColor(BALLOON_COLOR_BK_MID);
	crEnd = GetColor(BALLOON_COLOR_BK_END);
} 

void CBalloon::ShowBalloon(CWnd * pWnd, CPoint pt, UINT nIdText, BOOL showCloseButton, LPCTSTR szIcon)
{
	CString str;
	str.LoadString(nIdText);
	HICON hIcon = (HICON)::LoadImage(NULL, szIcon, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	ShowBalloon(pWnd, pt, str, showCloseButton, hIcon);
}

void CBalloon::ShowBalloon(
	CWnd * pWnd, CPoint pt, const CString& sText, BOOL showCloseButton, HICON hIcon, 
	UINT nDirection, UINT nEffect, COLORREF crStart, COLORREF crMid, COLORREF crEnd)
{
	BALLOON_INFO Info;
	Info.hIcon = hIcon;
	Info.sBalloonTip = sText;
	Info.nMask = 0;

	CBalloon * pSB = new CBalloon();
	if (pWnd == NULL)
		pWnd = GetDesktopWindow();
	pSB->Create(pWnd);
	pSB->AddTool(pWnd, Info);
	pSB->m_hCurrentWnd = pWnd->GetSafeHwnd();
	pSB->SetDirection(nDirection);
	pSB->SetEffectBk(nEffect);
	if (crStart == NULL)
		crStart = ::GetSysColor(COLOR_INFOBK);
	if (crMid == NULL)
		crMid = ::GetSysColor(COLOR_INFOBK);
	if (crEnd == NULL)
		crEnd = ::GetSysColor(COLOR_INFOBK);
	pSB->SetGradientColors(crStart, crMid, crEnd);
	pSB->SetBehaviour(BALLOON_DIALOG | BALLOON_DIALOG_DESTROY);
	if (showCloseButton)
	{
		pSB->ModifyStyles(BALLOON_CLOSEBUTTON, 0);
	}
	pSB->DisplayToolTip(&pt);
	if (!showCloseButton)
		pSB->SetTimer(BALLOON_HIDE, 5000, NULL); //auto close the dialog if no close button is shown
}

void CBalloon::AddTool(int nIdWnd, UINT nIdText, HICON hIcon/* = NULL*/)
{
	AddTool(m_pParentWnd->GetDlgItem(nIdWnd), nIdText, hIcon);
}
void CBalloon::AddTool(int nIdWnd, UINT nIdText, UINT nIdIcon)
{
	AddTool(m_pParentWnd->GetDlgItem(nIdWnd), nIdText, nIdIcon);
}
void CBalloon::AddTool(int nIdWnd, const CString& sBalloonTipText, HICON hIcon/* = NULL*/)
{
	AddTool(m_pParentWnd->GetDlgItem(nIdWnd), sBalloonTipText, hIcon);
}
void CBalloon::AddTool(int nIdWnd, const CString& sBalloonTipText, UINT nIdIcon)
{
	AddTool(m_pParentWnd->GetDlgItem(nIdWnd), sBalloonTipText, nIdIcon);
}

void CBalloon::AddTool(CWnd * pWnd, UINT nIdText, HICON hIcon /* = NULL */)
{
	CString str;
    str.LoadString(nIdText);
	AddTool(pWnd, str, hIcon);
}

void CBalloon::AddTool(CWnd * pWnd, UINT nIdText, UINT nIdIcon)
{
	CString str;
    str.LoadString(nIdText);
	AddTool(pWnd, str, nIdIcon);
} 

void CBalloon::AddTool(CWnd * pWnd, const CString& sBalloonTipText, UINT nIdIcon)
{
	HICON hIcon	= NULL;
	HINSTANCE hInstResource	= NULL;

	if (nIdIcon >= 0)
	{
		hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nIdIcon), RT_GROUP_ICON);
		
		hIcon = (HICON)::LoadImage(hInstResource, MAKEINTRESOURCE(nIdIcon), IMAGE_ICON, 0, 0, 0);
	}

	AddTool(pWnd, sBalloonTipText, hIcon);
} 

void CBalloon::AddTool(CWnd * pWnd, const CString& sBalloonTipText, HICON hIcon /* = NULL */)
{
	//store the tool information
	BALLOON_INFO Info;
	Info.hIcon = hIcon;
	Info.sBalloonTip = sBalloonTipText;
	Info.nMask = 0;

	AddTool(pWnd, Info);
}

void CBalloon::AddTool(CWnd * pWnd, BALLOON_INFO & bi)
{
	if (pWnd)
		m_ToolMap.SetAt(pWnd->m_hWnd, bi);
	else
		m_ToolMap.SetAt(NULL, bi);
} 

BOOL CBalloon::GetTool(CWnd * pWnd, CString & sBalloonTipText, HICON & hIcon) const
{
	BALLOON_INFO bi;
	BOOL bFound = GetTool(pWnd, bi);
	if (bFound)
	{
		sBalloonTipText = bi.sBalloonTip;
		hIcon = bi.hIcon;
	}

	return bFound;
}

BOOL CBalloon::GetTool(CWnd * pWnd, BALLOON_INFO & bi) const
{
	if (pWnd)
		return m_ToolMap.Lookup(pWnd->m_hWnd, bi);
	return m_ToolMap.Lookup(NULL, bi);
}

void CBalloon::RemoveTool(CWnd * pWnd)
{
	if (pWnd)
		m_ToolMap.RemoveKey(pWnd->m_hWnd);
	m_ToolMap.RemoveKey(NULL);
}

void CBalloon::RemoveAllTools()
{
	m_ToolMap.RemoveAll();
}

void CBalloon::SetMaskTool(CWnd * pWnd, UINT nMask /* = 0 */)
{
	ModifyMaskTool(pWnd, nMask, (UINT)-1);
}

void CBalloon::ModifyMaskTool(CWnd * pWnd, UINT nAddMask, UINT nRemoveMask)
{
	ASSERT(pWnd);

	BALLOON_INFO bi;
	if (GetTool(pWnd, bi))
	{
		bi.nMask &= ~nRemoveMask;
		bi.nMask |= nAddMask;
		AddTool(pWnd, bi);
	}
}

UINT CBalloon::GetMaskTool(CWnd * pWnd) const
{
	ASSERT(pWnd);

	UINT nMask = 0;
	BALLOON_INFO bi;
	if (GetTool(pWnd, bi))
		nMask = bi.nMask;
	return nMask;
}

void CBalloon::SetEffectBk(UINT nEffect, CWnd * pWnd /* = NULL */)
{
	if (!pWnd)
	{
		m_nEffect = nEffect;
	}
	else
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			bi.nEffect = nEffect;
			bi.nMask |= BALLOON_MASK_EFFECT;
			AddTool(pWnd, bi);
		}
	}
}

UINT CBalloon::GetEffectBk(CWnd * pWnd /* = NULL */) const
{
	if (pWnd)
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (bi.nMask & BALLOON_MASK_EFFECT)
			{
				return bi.nEffect;
			}
		}
	}
	return m_nEffect;
}

void CBalloon::SetNotify(BOOL bParentNotify /* = TRUE */)
{
	HWND hWnd = NULL;

	if (bParentNotify)
		hWnd = m_pParentWnd->GetSafeHwnd();

	SetNotify(hWnd);
}

void CBalloon::SetNotify(HWND hWnd)
{
	m_hNotifyWnd = hWnd;
}

BOOL CBalloon::GetNotify() const
{
	return (m_hNotifyWnd != NULL);
} 

void CBalloon::SetDelayTime(DWORD dwDuration, UINT nTime)
{
	switch (dwDuration)
	{
	case TTDT_AUTOPOP:
		m_nTimeAutoPop = nTime;
		break;
	case TTDT_INITIAL :
		m_nTimeInitial = nTime;
		break;
	}
}

UINT CBalloon::GetDelayTime(DWORD dwDuration) const
{
	UINT nTime = 0;
	switch (dwDuration)
	{
	case TTDT_AUTOPOP:
		nTime = m_nTimeAutoPop;
		break;
	case TTDT_INITIAL:
		nTime = m_nTimeInitial;
		break;
	}

	return nTime;
} 

void CBalloon::SetSize(int nSizeIndex, UINT nValue)
{
	if (nSizeIndex >= XBLSZ_MAX_SIZES)
		return;

	m_nSizes [nSizeIndex] = nValue;
}

UINT CBalloon::GetSize(int nSizeIndex) const
{
	if (nSizeIndex >= XBLSZ_MAX_SIZES)
		return 0;

	return m_nSizes [nSizeIndex];
}

void CBalloon::SetDefaultSizes()
{
	SetSize(XBLSZ_ROUNDED_CX, 16);
	SetSize(XBLSZ_ROUNDED_CY, 16);
	SetSize(XBLSZ_MARGIN_CX, 12);
	SetSize(XBLSZ_MARGIN_CY, 12);
	SetSize(XBLSZ_SHADOW_CX, 4);
	SetSize(XBLSZ_SHADOW_CY, 4);
	SetSize(XBLSZ_WIDTH_ANCHOR, 12);
	SetSize(XBLSZ_HEIGHT_ANCHOR, 16);
	SetSize(XBLSZ_MARGIN_ANCHOR, 16);
	SetSize(XBLSZ_BORDER_CX, 1);
	SetSize(XBLSZ_BORDER_CY, 1);
	SetSize(XBLSZ_BUTTON_MARGIN_CX, 5);
	SetSize(XBLSZ_BUTTON_MARGIN_CY, 5);
}

void CBalloon::SetDirection(UINT nDirection /* = BALLOON_RIGHT_TOP */, CWnd * pWnd /* = NULL */)
{
	if (nDirection >= BALLOON_MAX_DIRECTIONS)
		return;

	if (!pWnd)
	{
		m_nDirection = nDirection;
	}
	else
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			bi.nDirection = nDirection;
			bi.nMask |= BALLOON_MASK_DIRECTION;
			AddTool(pWnd, bi);
		}
	}
}

UINT CBalloon::GetDirection(CWnd * pWnd /* = NULL */) const
{
	if (pWnd)
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (bi.nMask & BALLOON_MASK_DIRECTION)
				return bi.nDirection;
		}
	}
	return m_nDirection;
}

void CBalloon::SetBehaviour(UINT nBehaviour /* = 0 */, CWnd * pWnd /* = NULL */)
{
	if (!pWnd)
	{
		m_nBehaviour = nBehaviour;
	}
	else
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			bi.nBehaviour = nBehaviour;
			bi.nMask |= BALLOON_MASK_BEHAVIOUR;
			AddTool(pWnd, bi);
		}
	}
}

UINT CBalloon::GetBehaviour(CWnd * pWnd /* = NULL */) const
{
	if (pWnd)
	{
		BALLOON_INFO bi;
		if (GetTool(pWnd, bi))
		{
			if (bi.nMask & BALLOON_MASK_BEHAVIOUR)
				return bi.nBehaviour;
		}
	}
	return m_nBehaviour;
}

BOOL CBalloon::SetFont(CFont & font)
{
	LOGFONT lf;
	font.GetLogFont (&lf);

	return SetFont(&lf);
}

BOOL CBalloon::SetFont(LPLOGFONT lf)
{
    memcpy(&m_LogFont, lf, sizeof(LOGFONT));

	return TRUE; 
}

BOOL CBalloon::SetFont(LPCTSTR lpszFaceName, int nSizePoints /* = 8 */,
									BOOL bUnderline /* = FALSE */, BOOL bBold /* = FALSE */,
									BOOL bStrikeOut /* = FALSE */, BOOL bItalic /* = FALSE */)
{
	CDC* pDC = GetDC();
	LOGFONT lf;
	memset (&lf, 0, sizeof(LOGFONT));

	_tcscpy_s (lf.lfFaceName, 32, lpszFaceName);
	lf.lfHeight = -MulDiv (nSizePoints, GetDeviceCaps (pDC->m_hDC, LOGPIXELSY), 72);
	lf.lfUnderline = (BYTE)bUnderline;
	lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
	lf.lfStrikeOut = (BYTE)bStrikeOut;
	lf.lfItalic = (BYTE)bItalic;

	if (pDC)
		ReleaseDC(pDC);

	return SetFont(&lf);
}

void CBalloon::SetDefaultFont()
{
	LPLOGFONT lpSysFont = GetSystemToolTipFont();
	if(lpSysFont)
		SetFont(lpSysFont);
} 

void CBalloon::GetFont(CFont & font) const
{
	font.CreateFontIndirect(&m_LogFont);
}

void CBalloon::GetFont(LPLOGFONT lf) const
{
	memcpy(lf, &m_LogFont, sizeof(LOGFONT));
}

void CBalloon::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pToolInfo.nStyles & BALLOON_CLOSEBUTTON)
	{
		UINT nState = DFCS_CAPTIONCLOSE | DFCS_FLAT | DFCS_TRANSPARENT;
		if (m_rtCloseButton.PtInRect(point))
		{
			nState |= DFCS_HOT;
			if (m_bButtonPushed)
				nState |= DFCS_PUSHED;
		}
		CClientDC dc(this);
		dc.DrawFrameControl(m_rtCloseButton, DFC_CAPTION, nState);

		if (IsPointOverALink(point))
			m_Cursor.SetCursor(IDC_HAND);
		else
			m_Cursor.Restore();	
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CBalloon::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_pToolInfo.nStyles & BALLOON_CLOSEBUTTON) && m_rtCloseButton.PtInRect(point))
	{
		m_bButtonPushed = TRUE;
		OnMouseMove(0, point);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CBalloon::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (IsPointOverALink(point))
	{
		CString url = GetLinkForPoint(point);
		ShellExecute(NULL, _T("open"), url, NULL,NULL, 0);
	}
	else if (
		// Dialog has close button, but user has clicked somewhere else.
		(m_pToolInfo.nStyles & BALLOON_CLOSEBUTTON) &&
		(!m_rtCloseButton.PtInRect(point) || !m_bButtonPushed))
	{
		m_bButtonPushed =  FALSE;
	}
	else
	{
		Pop();
		if (GetBehaviour() & BALLOON_DIALOG_DESTROY)
		{
			CWnd::OnLButtonUp(nFlags, point);
			DestroyWindow();
			return;
		}
	}
	CWnd::OnLButtonUp(nFlags, point);
}

void CBalloon::PostNcDestroy()
{
	CWnd::PostNcDestroy();
	TRACE("CBalloon: PostNcDestroy()\n");

	if (GetBehaviour() & BALLOON_DIALOG_DESTROY)
	{
		TRACE("CBalloon: object deleted\n");
		delete this;
	}
}

void CBalloon::GetMonitorWorkArea(const CPoint& sourcePoint, CRect& monitorRect) const
{
	// identify the monitor that contains the sourcePoint
	// and return the work area (the portion of the screen 
	// not obscured by the system task bar or by application 
	// desktop tool bars) of that monitor
	OSVERSIONINFOEX VersionInformation;
	SecureZeroMemory(&VersionInformation, sizeof(OSVERSIONINFOEX));
	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&VersionInformation);

	::GetWindowRect(GetDesktopWindow()->m_hWnd, &monitorRect);
	
	if (VersionInformation.dwMajorVersion >= 5)
	{
		MONITORINFO mi;

		//
		// get the work area
		//
		mi.cbSize = sizeof(mi);
		HMODULE hUser32 = ::GetModuleHandle (_T("USER32.DLL"));
		if (hUser32 != NULL)
		{
			typedef HMONITOR (WINAPI *FN_MonitorFromPoint) (POINT pt, DWORD dwFlags);
			typedef BOOL (WINAPI *FN_GetMonitorInfo) (HMONITOR hMonitor, LPMONITORINFO lpmi);
			FN_MonitorFromPoint pfnMonitorFromPoint = (FN_MonitorFromPoint)
				::GetProcAddress (hUser32, "MonitorFromPoint");
			FN_GetMonitorInfo pfnGetMonitorInfo = (FN_GetMonitorInfo)
				::GetProcAddress (hUser32, "GetMonitorInfoW");
			if (pfnMonitorFromPoint != NULL && pfnGetMonitorInfo != NULL)
			{
				MONITORINFO mi;
				HMONITOR hMonitor = pfnMonitorFromPoint (sourcePoint, 
					MONITOR_DEFAULTTONEAREST);
				mi.cbSize = sizeof (mi);
				pfnGetMonitorInfo (hMonitor, &mi);
				monitorRect = mi.rcWork;
			}
		}
	}

}

CPoint 
CBalloon::GetCtrlCentre(CWnd* pDlgWnd, UINT ctrlId)
{
	CWnd* pCtrl = pDlgWnd->GetDlgItem(ctrlId);
	if(pCtrl == NULL)
	{
		ASSERT(FALSE);
		return CPoint(200,200);
	}
	CRect ctrlRect;
	pCtrl->GetWindowRect(ctrlRect);
	return ctrlRect.CenterPoint();
}
