// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2009-2010, 2012 - TortoiseSVN

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
#include "ScrollTool.h"


CScrollTool::CScrollTool()
	: m_bInitCalled(false)
	, m_bRightAligned(false)
{
	SecureZeroMemory(&ti, sizeof(ti));
}

CScrollTool::~CScrollTool()
{
}


BEGIN_MESSAGE_MAP(CScrollTool, CWnd)
END_MESSAGE_MAP()


bool CScrollTool::Init(LPPOINT pos, bool bRightAligned /* = false */)
{
	if (!m_bInitCalled)
	{
		// create the tooltip window
		if (!CreateEx(NULL,
					 TOOLTIPS_CLASS,
					 nullptr,
					 TTS_NOPREFIX | TTS_ALWAYSTIP,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 nullptr,
					 nullptr,
					 nullptr))
		{
			return false;
		}

		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_TRACK;
		ti.hwnd = nullptr;
		ti.hinst = nullptr;
		ti.uId = 0;
		ti.lpszText = L" ";

		// ToolTip control will cover the whole window
		ti.rect.left = 0;
		ti.rect.top = 0;
		ti.rect.right = 0;
		ti.rect.bottom = 0;

		CPoint point;
		::GetCursorPos(&point);

		SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

		SendMessage(TTM_TRACKPOSITION, 0, static_cast<DWORD>(MAKELONG(point.x, point.y)));
		SendMessage(TTM_TRACKACTIVATE, true, reinterpret_cast<LPARAM>(&ti));
		SendMessage(TTM_TRACKPOSITION, 0, MAKELONG(pos->x, pos->y));
		m_bRightAligned = bRightAligned;
		m_bInitCalled = true;
	}
	return true;
}

void CScrollTool::SetText(LPPOINT pos, const TCHAR * fmt, ...)
{
	CString s;
	va_list marker;

	va_start( marker, fmt );
	s.FormatV(fmt, marker);
	va_end( marker );

	CSize textsize(0);
	if (m_bRightAligned)
	{
		CDC *pDC = GetDC();
		textsize = pDC->GetTextExtent(s);
		ReleaseDC(pDC);
	}

	ti.lpszText = s.GetBuffer();
	SendMessage(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
	SendMessage(TTM_TRACKPOSITION, 0, MAKELONG(pos->x-textsize.cx, pos->y));
	s.ReleaseBuffer();
}

void CScrollTool::Clear()
{
	if (m_bInitCalled)
	{
		SendMessage(TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
		DestroyWindow();
	}
	m_bInitCalled = false;
}

LONG CScrollTool::GetTextWidth(LPCTSTR szText)
{
	CDC *pDC = GetDC();
	CSize textsize = pDC->GetTextExtent(szText, static_cast<int>(wcslen(szText)));
	ReleaseDC(pDC);
	return textsize.cx;
}
