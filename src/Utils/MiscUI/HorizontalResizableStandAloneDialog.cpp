// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseGit

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
#include "HorizontalResizableStandAloneDialog.h"

IMPLEMENT_DYNAMIC(CHorizontalResizableStandAloneDialog, CResizableStandAloneDialog)
CHorizontalResizableStandAloneDialog::CHorizontalResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
	: CResizableStandAloneDialog(nIDTemplate, pParentWnd)
{
}

BEGIN_MESSAGE_MAP(CHorizontalResizableStandAloneDialog, CResizableStandAloneDialog)
	ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL CHorizontalResizableStandAloneDialog::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	RECT rect;
	GetWindowRect(&rect);
	m_height = rect.bottom - rect.top;

	return FALSE;
}

void CHorizontalResizableStandAloneDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + m_height;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - m_height;
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}
