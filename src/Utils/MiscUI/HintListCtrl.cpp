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
#include "MyMemDC.h"
#include "HintListCtrl.h"

CHintListCtrl::CHintListCtrl() : CListCtrl()
{
}

CHintListCtrl::~CHintListCtrl()
{
}

void CHintListCtrl::ShowText(const CString& sText, bool forceupdate /* = false*/)
{
	m_sText = sText;
	Invalidate();
	if (forceupdate)
		UpdateWindow();
}

void CHintListCtrl::ClearText()
{
	m_sText.Empty();
	Invalidate();
}

IMPLEMENT_DYNAMIC(CHintListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CHintListCtrl, CListCtrl)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CHintListCtrl::OnPaint()
{
	Default();
	if (!m_sText.IsEmpty())
	{
		COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
		COLORREF clrTextBk;
		if (IsWindowEnabled())
			clrTextBk = ::GetSysColor(COLOR_WINDOW);
		else
			clrTextBk = ::GetSysColor(COLOR_3DFACE);

		CRect rc;
		GetClientRect(&rc);
		CHeaderCtrl* pHC;
		pHC = GetHeaderCtrl();
		if (pHC != NULL)
		{
			CRect rcH;
			rcH.SetRectEmpty();
			pHC->GetItemRect(0, &rcH);
			rc.top += rcH.bottom;
		}
		CDC* pDC = GetDC();
		{
			CMyMemDC memDC(pDC, &rc);

			memDC.SetTextColor(clrText);
			memDC.SetBkColor(clrTextBk);
			memDC.FillSolidRect(rc, clrTextBk);
			rc.top += 10;
			CGdiObject * oldfont = memDC.SelectStockObject(DEFAULT_GUI_FONT);
			memDC.DrawText(m_sText, rc, DT_CENTER | DT_VCENTER |
				DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
			memDC.SelectObject(oldfont);
		}
		ReleaseDC(pDC);
	}
}
