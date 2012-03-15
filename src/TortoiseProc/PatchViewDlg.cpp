// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
// PatchViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchViewDlg.h"
#include "Registry.h"
#include "CommitDlg.h"
#include "UnicodeUtils.h"
// CPatchViewDlg dialog

IMPLEMENT_DYNAMIC(CPatchViewDlg, CDialog)

CPatchViewDlg::CPatchViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchViewDlg::IDD, pParent)
{

}

CPatchViewDlg::~CPatchViewDlg()
{
}

void CPatchViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCH, m_ctrlPatchView);
}

BEGIN_MESSAGE_MAP(CPatchViewDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_MOVING()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// CPatchViewDlg message handlers

BOOL CPatchViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctrlPatchView.Init(*m_pProjectProperties);
	m_ctrlPatchView.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));

	m_ctrlPatchView.SetUDiffStyle();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPatchViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_PATCH)->MoveWindow(0, 0, cx, cy);
		CRect rect2;
		m_ctrlPatchView.GetClientRect(rect);
		m_ctrlPatchView.Call(SCI_SETSCROLLWIDTH, rect.Width() - 4);
	}
}

void CPatchViewDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
#define STICKYSIZE 5
	RECT parentRect;
	this->m_ParentCommitDlg->GetWindowRect(&parentRect);
	if (abs(parentRect.right - pRect->left) < STICKYSIZE)
	{
		int width = pRect->right - pRect->left;
		pRect->left = parentRect.right;
		pRect->right = pRect->left + width;
	}
	CDialog::OnMoving(fwSide, pRect);
}

void CPatchViewDlg::OnDestroy()
{
	CDialog::OnDestroy();

	this->m_ParentCommitDlg->ShowViewPatchText(true);
}

void CPatchViewDlg::OnClose()
{
	CDialog::OnClose();
	this->DestroyWindow();
}
