// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2016, 2019 - TortoiseGit
// Copyright (C) 2003-2006,2008, 2011, 2018 - TortoiseSVN

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
#include "HyperLink.h"
#include "SmartHandle.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLTIP_ID 1


CHyperLink::CHyperLink()
	: m_hLinkCursor(nullptr)
	, m_crLinkColor(GetSysColor(COLOR_HOTLIGHT))
	, m_crHoverColor(RGB(255, 0, 0)) // Red
	, m_bOverControl(FALSE)
	, m_nUnderline(ulHover)
	, m_nTimerID(100)
{
}

CHyperLink::~CHyperLink()
{
	m_UnderlineFont.DeleteObject();
}

BOOL CHyperLink::DestroyWindow()
{
	KillTimer(m_nTimerID);

	return CStatic::DestroyWindow();
}

BOOL CHyperLink::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CStatic::PreTranslateMessage(pMsg);
}


void CHyperLink::PreSubclassWindow()
{
	// Enable notifications - CStatic has this disabled by default
	ModifyStyle(0, SS_NOTIFY);

	// By default use the label text as the URL
	if (m_strURL.IsEmpty())
		GetWindowText(m_strURL);

	CString strWndText;
	GetWindowText(strWndText);
	if (strWndText.IsEmpty())
	{
		SetWindowText(m_strURL);
	}

	LOGFONT lf;
	CFont* pFont = GetFont();
	if (pFont)
		pFont->GetObject(sizeof(lf), &lf);
	else
	{
		NONCLIENTMETRICS metrics = { 0 };
		metrics.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
		memcpy_s(&lf, sizeof(LOGFONT), &metrics.lfMessageFont, sizeof(LOGFONT));
	}
	m_StdFont.CreateFontIndirect(&lf);
	lf.lfUnderline = TRUE;
	m_UnderlineFont.CreateFontIndirect(&lf);

	SetDefaultCursor(); // try loading a "hand" cursor
	SetUnderline();

	CRect rect;
	GetClientRect(rect);
	m_ToolTip.Create(this);
	m_ToolTip.AddTool(this, m_strURL, rect, TOOLTIP_ID);

	CStatic::PreSubclassWindow();
}

BEGIN_MESSAGE_MAP(CHyperLink, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()


void CHyperLink::OnClicked()
{
	if(!m_strURL.IsEmpty())
	{
		GotoURL(m_strURL);
	}
	else
	{
		::SendMessage(this->GetParent()->m_hWnd,WM_COMMAND,this->GetDlgCtrlID(),0);
	}
}

HBRUSH CHyperLink::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	if (m_bOverControl)
		pDC->SetTextColor(m_crHoverColor);
	else
		pDC->SetTextColor(m_crLinkColor);

	// draw transparent
	pDC->SetBkMode(TRANSPARENT);
	return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
}

void CHyperLink::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bOverControl)
	{
		m_bOverControl = TRUE;

		if (m_nUnderline == ulHover)
			SetFont(&m_UnderlineFont);
		Invalidate();

		SetTimer(m_nTimerID, 100, nullptr);
	}
	CStatic::OnMouseMove(nFlags, point);
}

void CHyperLink::OnTimer(UINT_PTR nIDEvent)
{
	CPoint p(GetMessagePos());
	ScreenToClient(&p);

	CRect rect;
	GetClientRect(rect);
	if (!rect.PtInRect(p))
	{
		m_bOverControl = FALSE;
		KillTimer(m_nTimerID);

		if (m_nUnderline != ulAlways)
			SetFont(&m_StdFont);
		rect.bottom+=10;
		InvalidateRect(rect);
	}

	CStatic::OnTimer(nIDEvent);
}

BOOL CHyperLink::OnSetCursor(CWnd* /*pWnd*/, UINT /*nHitTest*/, UINT /*message*/)
{
	if (m_hLinkCursor)
	{
		::SetCursor(m_hLinkCursor);
		return TRUE;
	}
	return FALSE;
}

BOOL CHyperLink::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	return TRUE;
}

void CHyperLink::SetURL(CString strURL)
{
	m_strURL = strURL;

	if (::IsWindow(GetSafeHwnd()))
	{
		m_ToolTip.UpdateTipText(strURL, this, TOOLTIP_ID);
	}
}

CString CHyperLink::GetURL() const
{
	return m_strURL;
}

void CHyperLink::SetColors(COLORREF crLinkColor, COLORREF crHoverColor)
{
	m_crLinkColor = crLinkColor;

	if (crHoverColor == -1)
		m_crHoverColor = ::GetSysColor(COLOR_HIGHLIGHT);
	else
		m_crHoverColor = crHoverColor;

	if (::IsWindow(m_hWnd))
		Invalidate();
}

COLORREF CHyperLink::GetLinkColor() const
{
	return m_crLinkColor;
}

COLORREF CHyperLink::GetHoverColor() const
{
	return m_crHoverColor;
}

void CHyperLink::SetUnderline(int nUnderline /*=ulHover*/)
{
	if (m_nUnderline == nUnderline)
		return;

	if (::IsWindow(GetSafeHwnd()))
	{
		if (nUnderline == ulAlways)
			SetFont(&m_UnderlineFont);
		else
			SetFont(&m_StdFont);

		Invalidate();
	}

	m_nUnderline = nUnderline;
}

int CHyperLink::GetUnderline() const
{
	return m_nUnderline;
}

// The following appeared in Paul DiLascia's Jan 1998 MSJ articles.
// It loads a "hand" cursor from the winhlp32.exe module
void CHyperLink::SetDefaultCursor()
{
	if (!m_hLinkCursor)
	{
		// first try the windows hand cursor (not available on NT4)
#ifndef OCR_HAND
#	define OCR_HAND 32649
#endif
		auto hHandCursor = static_cast<HCURSOR>(::LoadImage(nullptr, MAKEINTRESOURCE(OCR_HAND), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		if (hHandCursor)
		{
			m_hLinkCursor = hHandCursor;
			return;
		}
		// windows cursor not available, so try to load it from winhlp32.exe
		CString strWndDir;
		GetWindowsDirectory(CStrBuf(strWndDir, MAX_PATH), MAX_PATH);	// Explorer can't handle paths longer than MAX_PATH.
		strWndDir += L"\\winhlp32.exe";
		// This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
		CAutoLibrary hModule = LoadLibrary(strWndDir);
		if (hModule) {
			auto hHandCursor2 = static_cast<HCURSOR>(::LoadImage(hModule, MAKEINTRESOURCE(106), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
			if (hHandCursor2)
				m_hLinkCursor = CopyCursor(hHandCursor2);
		}
	}
}

HINSTANCE CHyperLink::GotoURL(LPCTSTR url)
{
	return ShellExecute(nullptr, L"open", url, nullptr, nullptr, SW_SHOW);
}

void CHyperLink::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_crLinkColor = GetSysColor(COLOR_HOTLIGHT);
	Invalidate();
}
