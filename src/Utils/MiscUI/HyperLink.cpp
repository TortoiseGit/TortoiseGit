// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008, 2011 - TortoiseSVN

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
#include "atlconv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLTIP_ID 1


CHyperLink::CHyperLink()
{
	m_hLinkCursor		= NULL;					// No cursor as	yet
	m_crLinkColor		= RGB(	0,	 0,	238);	// Blue
	m_crHoverColor		= RGB(255,	 0,	  0);	// Red
	m_bOverControl		= FALSE;				// Cursor not yet over control
	m_nUnderline		= ulHover;				// Underline the link?
	m_strURL.Empty();
	m_nTimerID			= 100;
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
	DWORD dwStyle = GetStyle();
	::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	// By default use the label text as the URL
	if (m_strURL.IsEmpty())
		GetWindowText(m_strURL);

	CString strWndText;
	GetWindowText(strWndText);
	if (strWndText.IsEmpty())
	{
		SetWindowText(m_strURL);
	}

	CFont* pFont = GetFont();
	if (!pFont)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT) GetStockObject(ANSI_VAR_FONT);
		if (hFont)
			pFont = CFont::FromHandle(hFont);
	}
	ASSERT(pFont->GetSafeHandle());

	LOGFONT lf;
	pFont->GetLogFont(&lf);
	m_StdFont.CreateFontIndirect(&lf);
	lf.lfUnderline = (BYTE) TRUE;
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
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CHyperLink::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bOverControl)
	{
		m_bOverControl = TRUE;

		if (m_nUnderline == ulHover)
			SetFont(&m_UnderlineFont);
		Invalidate();

		SetTimer(m_nTimerID, 100, NULL);
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
	if (m_hLinkCursor == NULL)
	{
		// first try the windows hand cursor (not available on NT4)
#ifndef OCR_HAND
#	define OCR_HAND 32649
#endif
		HCURSOR hHandCursor = (HCURSOR)::LoadImage(NULL, MAKEINTRESOURCE(OCR_HAND), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		if (hHandCursor)
		{
			m_hLinkCursor = hHandCursor;
			return;
		}
		// windows cursor not available, so try to load it from winhlp32.exe
		CString strWndDir;
		GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);	// Explorer can't handle paths longer than MAX_PATH.
		strWndDir.ReleaseBuffer();

		strWndDir += _T("\\winhlp32.exe");
		// This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
		CAutoLibrary hModule = LoadLibrary(strWndDir);
		if (hModule) {
			HCURSOR hHandCursor2 = (HCURSOR)::LoadImage(hModule, MAKEINTRESOURCE(106), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
			if (hHandCursor2)
				m_hLinkCursor = CopyCursor(hHandCursor2);
		}
	}
}

HINSTANCE CHyperLink::GotoURL(LPCTSTR url)
{
	return ShellExecute(NULL, _T("open"), url, NULL,NULL, SW_SHOW);
}
