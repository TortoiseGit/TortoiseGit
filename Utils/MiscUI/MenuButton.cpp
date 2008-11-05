// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008 - Stefan Kueng

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
#include "MenuButton.h"

#include "MyMemDC.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


const int g_ciArrowSizeX = 4 ;
const int g_ciArrowSizeY = 2 ;

IMPLEMENT_DYNCREATE(CMenuButton, _Inherited)

CMenuButton::CMenuButton(void):
	_Inherited(),
	m_bMouseOver(FALSE),
	m_currentEntry(-1),
	m_bUseThemes(true)
{
}

CMenuButton::~CMenuButton(void)
{
}

void CMenuButton::UseThemes(bool bUseThemes)
{
	m_bUseThemes = bUseThemes;
	Invalidate();
}

bool CMenuButton::SetCurrentEntry(INT_PTR entry)
{
	if (entry >= m_sEntries.GetCount())
		return false;
	m_currentEntry = entry;
	SetWindowText(m_sEntries[m_currentEntry]);
	return true;
}

INT_PTR CMenuButton::AddEntry(const CString& sEntry)
{
	INT_PTR ret = m_sEntries.Add(sEntry);
	m_currentEntry = 0;
	SetWindowText(m_sEntries[0]);
	Invalidate();
	return ret;
}

INT_PTR CMenuButton::AddEntries(const CStringArray& sEntries)
{
	INT_PTR ret = m_sEntries.Append(sEntries);
	m_currentEntry = 0;
	SetWindowText(m_sEntries[0]);
	Invalidate();
	return ret;
}

BEGIN_MESSAGE_MAP(CMenuButton, _Inherited)
	ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


int CMenuButton::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (_Inherited::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

bool CMenuButton::ShowMenu()
{
	CRect rDraw;
	GetWindowRect(rDraw);
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		for (int index=0; index<m_sEntries.GetCount(); ++index)
		{
			popup.AppendMenu(MF_ENABLED | MF_STRING, index+1, m_sEntries.GetAt(index));
		}
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, rDraw.left, rDraw.bottom-1, this);
		if (cmd <= 0)
			return false;
		if (cmd > m_sEntries.GetCount())
			return false;
		m_currentEntry = cmd-1;
		SetWindowText(m_sEntries.GetAt(cmd-1));
		return true;
	}
	return false;
}

BOOL CMenuButton::OnClicked()
{
	CRect rDraw;
	GetWindowRect(rDraw);

	CPoint point;
	DWORD ptW = GetMessagePos();
	point.x = GET_X_LPARAM(ptW);
	point.y = GET_Y_LPARAM(ptW);
	if (rDraw.PtInRect(point))
	{
		// check if the user clicked on the arrow or the button part
		CRect rArrow;
		rArrow = rDraw;
		CPoint temp(m_SeparatorX, 0);
		ClientToScreen(&temp);
		rArrow.left = temp.x;
		if (rArrow.PtInRect(point))
		{
			if (ShowMenu())
				return FALSE;
			return TRUE;
		}
	}
	// click on button or user pressed enter/space.
	// let the parent handle the message the usual way
	return FALSE;
}

void CMenuButton::OnNMThemeChanged(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if (m_bUseThemes)
		m_xpButton.Open(GetSafeHwnd(), L"BUTTON");
	Invalidate(FALSE);
	*pResult = 0;
}

void CMenuButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bMouseOver)
	{
		m_bMouseOver = TRUE;
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		_TrackMouseEvent(&tme);
		Invalidate(FALSE);
	}

	_Inherited::OnMouseMove(nFlags, point);
}

LRESULT CMenuButton::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_bMouseOver)
	{
		m_bMouseOver = FALSE;
		Invalidate(FALSE);
	}
	return 0;
}

void CMenuButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ASSERT(lpDrawItemStruct);

	CDC*    pDC      = CDC::FromHandle(lpDrawItemStruct->hDC);
	CMyMemDC  dcMem(pDC);
	UINT    state    = lpDrawItemStruct->itemState;
	CRect   rDraw    = lpDrawItemStruct->rcItem;
	CRect	rArrow;

	SendMessage(WM_ERASEBKGND, (WPARAM)dcMem.GetSafeHdc());

	if (state & ODS_FOCUS)
		state |= ODS_DEFAULT;

	if(!m_xpButton)
		m_xpButton.Open(GetSafeHwnd(), L"BUTTON");

	// Draw Outer Edge
	if ((m_xpButton.IsAppThemed())&&(m_bUseThemes))
	{
		int nFrameState = 0;
		if ((state & ODS_SELECTED) != 0)
			nFrameState |= PBS_PRESSED;
		if ((state & ODS_DISABLED) != 0)
			nFrameState |= PBS_DISABLED;
		if ((state & ODS_HOTLIGHT) != 0 || m_bMouseOver)
			nFrameState |= PBS_HOT;
		else if ((state & ODS_DEFAULT) != 0)
			nFrameState |= PBS_DEFAULTED;

		m_xpButton.DrawBackground(dcMem.GetSafeHdc(), BP_PUSHBUTTON, nFrameState, &rDraw, NULL);
		m_xpButton.GetBackgroundContentRect(dcMem.GetSafeHdc(), BP_PUSHBUTTON, nFrameState, &rDraw, &rDraw);
	}
	else
	{
		UINT uFrameState = DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;

		if (state & ODS_SELECTED)
			uFrameState |= DFCS_PUSHED;

		if (state & ODS_DISABLED)
			uFrameState |= DFCS_INACTIVE;

		dcMem.DrawFrameControl(&rDraw, DFC_BUTTON, uFrameState);

		if (state & ODS_SELECTED)
			rDraw.OffsetRect(1,1);
	}


	// Draw Focus
	if (IsDefault()) 
	{
		CRect rFocus(rDraw.left, rDraw.top, rDraw.right - 1, rDraw.bottom);
		dcMem.DrawFocusRect(&rFocus);
	}

	rDraw.DeflateRect(::GetSystemMetrics(SM_CXEDGE), ::GetSystemMetrics(SM_CYEDGE));
	rDraw.DeflateRect(0, 0, 1, 0);

	// Draw Arrow
	rArrow.left		= rDraw.right - g_ciArrowSizeX - ::GetSystemMetrics(SM_CXEDGE) /2;
	rArrow.right	= rArrow.left + g_ciArrowSizeX;
	rArrow.top		= (rDraw.bottom + rDraw.top)/2 - g_ciArrowSizeY / 2;
	rArrow.bottom	= (rDraw.bottom + rDraw.top)/2 + g_ciArrowSizeY / 2;

	DrawArrow(&dcMem, &rArrow, (state & ODS_DISABLED) ? ::GetSysColor(COLOR_GRAYTEXT) : ::GetSysColor(COLOR_BTNTEXT));
	rDraw.right = rArrow.left - ::GetSystemMetrics(SM_CXEDGE) / 2 - 2;

	// Draw Separator
	dcMem.DrawEdge(&rDraw, EDGE_ETCHED,	BF_RIGHT);
	m_SeparatorX = rDraw.right;
	rDraw.right -= (::GetSystemMetrics(SM_CXEDGE) * 2) + 1 ;

	// Draw Text
	CString szTemp;
	GetWindowText(szTemp);

	CFont *pOldFont = dcMem.SelectObject(GetFont());
	dcMem.SetTextColor((state & ODS_DISABLED) ? ::GetSysColor(COLOR_GRAYTEXT) : ::GetSysColor(COLOR_BTNTEXT));

	dcMem.SetBkMode(TRANSPARENT);

	// DrawText() ignores the DT_VCENTER if DT_WORDBREAK
	// is specified. Therefore we center the text ourselves.
	CRect rectText(rDraw);
	rectText.bottom=0;
	dcMem.DrawText(szTemp,rectText,DT_WORDBREAK | DT_CALCRECT);
	if (rDraw.Height()>rectText.Height())
		rectText.OffsetRect(0,(rDraw.Height()-rectText.Height())/2);
	rectText.left = rDraw.left;
	rectText.right = rDraw.right;

	// check the button styles and draw the text accordingly
	DWORD style = GetStyle();
	UINT format = DT_CENTER | DT_VCENTER;
	if (style & BS_BOTTOM)
		format |= DT_BOTTOM;
	if (style & BS_LEFT)
		format |= DT_LEFT;
	if (style & BS_MULTILINE)
		format |= DT_WORDBREAK;
	if (style & BS_RIGHT)
		format |= DT_RIGHT;
	if (style & BS_TOP)
		format |= DT_TOP;
	format |= GetStyle()&
	dcMem.DrawText(szTemp, rectText, format);

	dcMem.SelectObject(pOldFont);
}


void CMenuButton::DrawArrow(CDC* pDC, 
							RECT* pRect, 
							COLORREF clrArrow)
{
	POINT ptsArrow[3];

	ptsArrow[0].x = pRect->left;
	ptsArrow[0].y = pRect->top;
	ptsArrow[1].x = pRect->right;
	ptsArrow[1].y = pRect->top;
	ptsArrow[2].x = (pRect->left + pRect->right)/2;
	ptsArrow[2].y = pRect->bottom;
	CBrush brsArrow(clrArrow);
	CPen penArrow(PS_SOLID, 1 , clrArrow);

	CBrush* pOldBrush = pDC->SelectObject(&brsArrow);
	CPen*   pOldPen   = pDC->SelectObject(&penArrow);

	pDC->SetPolyFillMode(WINDING);
	pDC->Polygon(ptsArrow, 3);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

BOOL CMenuButton::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_DOWN:
		case VK_F4:
			ShowMenu();			// Activate pop-up when F4-key or down-arrow is hit
			return TRUE;
		case VK_UP:
			return TRUE;		// Disable up-arrow
		}
	}

	return _Inherited::PreTranslateMessage(pMsg);
}

void CMenuButton::OnDestroy()
{
	m_sEntries.RemoveAll();

	_Inherited::OnDestroy();
}
