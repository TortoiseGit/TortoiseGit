// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2016 - TortoiseGit
// Copyright (C) 2003-2008,2011 - TortoiseSVN

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

IMPLEMENT_DYNAMIC(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
CStandAloneDialog::CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= nullptr*/)
: CStandAloneDialogTmpl<CDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
CStateStandAloneDialog::CStateStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= nullptr*/)
: CStandAloneDialogTmpl<CStateDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CResizableStandAloneDialog, CDialog)
CResizableStandAloneDialog::CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= nullptr*/)
	: CStandAloneDialogTmpl<CResizableDialog>(nIDTemplate, pParentWnd)
	, m_bVertical(false)
	, m_bHorizontal(false)
	, m_nResizeBlock(0)
	, m_height(0)
	, m_width(0)
{
}

BOOL CResizableStandAloneDialog::OnInitDialog()
{
	__super::OnInitDialog();

	RECT rect;
	GetWindowRect(&rect);
	m_height = rect.bottom - rect.top;
	m_width = rect.right - rect.left;

	return FALSE;
}

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
	ON_WM_SIZING()
	ON_WM_MOVING()
	ON_WM_NCMBUTTONUP()
	ON_WM_NCRBUTTONUP()
	ON_WM_NCHITTEST()
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()

void CResizableStandAloneDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	m_bVertical = m_bVertical && (fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT);
	m_bHorizontal = m_bHorizontal && (fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM);
	if (m_nResizeBlock & DIALOG_BLOCKVERTICAL)
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
	}
	if (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL)
	{
		// don't allow the dialog to be changed in width
		switch (fwSide)
		{
		case WMSZ_RIGHT:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOMRIGHT:
			pRect->right = pRect->left + m_width;
			break;
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT:
			pRect->left = pRect->right - m_width;
			break;
		}
	}
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
		m_bHorizontal = !m_bHorizontal;
		MoveWindow(&rcWindowRect);
		// WORKAROUND
		// for some reasons, when the window is resized horizontally, its menu size is not get adjusted.
		// so, we force it to happen.
		SetMenu(GetMenu());
	}
	CStandAloneDialogTmpl<CResizableDialog>::OnNcRButtonUp(nHitTest, point);
}

LRESULT CResizableStandAloneDialog::OnNcHitTest(CPoint point)
{
	if (m_nResizeBlock == 0)
		return __super::OnNcHitTest(point);

	// when resizing is blocked in a direction, don't return
	// a hit code that would allow that resizing.
	// Using the OnNcHitTest handler tells Windows to
	// not show a resizing mouse pointer if it's not possible
	auto ht = __super::OnNcHitTest(point);
	if (m_nResizeBlock & DIALOG_BLOCKVERTICAL)
	{
		switch (ht)
		{
		case HTBOTTOMLEFT:  ht = HTLEFT;   break;
		case HTBOTTOMRIGHT: ht = HTRIGHT;  break;
		case HTTOPLEFT:     ht = HTLEFT;   break;
		case HTTOPRIGHT:    ht = HTRIGHT;  break;
		case HTTOP:         ht = HTBORDER; break;
		case HTBOTTOM:      ht = HTBORDER; break;
		}
	}
	if (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL)
	{
		switch (ht)
		{
		case HTBOTTOMLEFT:  ht = HTBOTTOM; break;
		case HTBOTTOMRIGHT: ht = HTBOTTOM; break;
		case HTTOPLEFT:     ht = HTTOP;    break;
		case HTTOPRIGHT:    ht = HTTOP;    break;
		case HTLEFT:        ht = HTBORDER; break;
		case HTRIGHT:       ht = HTBORDER; break;
		}
	}

	return ht;
}

BEGIN_MESSAGE_MAP(CStateDialog, CDialog)
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()
