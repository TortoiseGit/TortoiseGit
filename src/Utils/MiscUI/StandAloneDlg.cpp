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
#include "Resource.h"
#include "StandAloneDlg.h"

IMPLEMENT_DYNAMIC(CResizableStandAloneDialog, CDialog)
CResizableStandAloneDialog::CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
	: CStandAloneDialogTmpl<CResizableDialog>(nIDTemplate, pParentWnd)
	, m_bVertical(false)
	, m_bHorizontal(false)
{
}

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
	ON_WM_SIZING()
	ON_WM_MOVING()
	ON_WM_NCMBUTTONUP()
	ON_WM_NCRBUTTONUP()
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()

void CResizableStandAloneDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	m_bVertical = m_bVertical && (fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT);
	m_bHorizontal = m_bHorizontal && (fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM);
	CStandAloneDialogTmpl<CResizableDialog>::OnSizing(fwSide, pRect);
}

void CResizableStandAloneDialog::OnMoving(UINT fwSide, LPRECT pRect)
{
	m_bVertical = m_bHorizontal = false;
	CStandAloneDialogTmpl<CResizableDialog>::OnMoving(fwSide, pRect);
}

void CResizableStandAloneDialog::OnNcMButtonUp(UINT nHitTest, CPoint point) 
{
	WINDOWPLACEMENT windowPlacement;
	if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
	{
		CRect rcWorkArea, rcWindowRect;
		GetWindowRect(&rcWindowRect);
		if (m_bVertical)
		{
			rcWindowRect.top = m_rcOrgWindowRect.top;
			rcWindowRect.bottom = m_rcOrgWindowRect.bottom;
		}
		else if (SystemParametersInfo(SPI_GETWORKAREA, 0U, &rcWorkArea, 0U))
		{
			m_rcOrgWindowRect.top = rcWindowRect.top;
			m_rcOrgWindowRect.bottom = rcWindowRect.bottom;
			rcWindowRect.top = rcWorkArea.top;
			rcWindowRect.bottom = rcWorkArea.bottom;
		}
		m_bVertical = !m_bVertical;
		m_bHorizontal = m_bHorizontal;
		MoveWindow(&rcWindowRect);
	}
	CStandAloneDialogTmpl<CResizableDialog>::OnNcMButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnNcRButtonUp(UINT nHitTest, CPoint point) 
{
	WINDOWPLACEMENT windowPlacement;
	if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
	{
		CRect rcWorkArea, rcWindowRect;
		GetWindowRect(&rcWindowRect);
		if (m_bHorizontal)
		{
			rcWindowRect.left = m_rcOrgWindowRect.left;
			rcWindowRect.right = m_rcOrgWindowRect.right;
		}
		else if (SystemParametersInfo(SPI_GETWORKAREA, 0U, &rcWorkArea, 0U))
		{
			m_rcOrgWindowRect.left = rcWindowRect.left;
			m_rcOrgWindowRect.right = rcWindowRect.right;
			rcWindowRect.left = rcWorkArea.left;
			rcWindowRect.right = rcWorkArea.right;
		}
		m_bVertical = m_bVertical;
		m_bHorizontal = !m_bHorizontal;
		MoveWindow(&rcWindowRect);
		// WORKAROUND
		// for some reasons, when the window is resized horizontally, its menu size is not get adjusted.
		// so, we force it to happen.
		SetMenu(GetMenu());
	}
	CStandAloneDialogTmpl<CResizableDialog>::OnNcRButtonUp(nHitTest, point);
}
