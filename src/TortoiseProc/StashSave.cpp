// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2017 - TortoiseGit

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
#include "TortoiseProc.h"
#include "StashSave.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "Git.h"

IMPLEMENT_DYNAMIC(CStashSaveDlg, CHorizontalResizableStandAloneDialog)

CStashSaveDlg::CStashSaveDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CStashSaveDlg::IDD, pParent)
	, m_bIncludeUntracked(FALSE)
	, m_bAll(FALSE)
{
}

CStashSaveDlg::~CStashSaveDlg()
{
}

void CStashSaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STASHMESSAGE, m_sMessage);
	DDX_Check(pDX, IDC_CHECK_UNTRACKED, m_bIncludeUntracked);
	DDX_Check(pDX, IDC_CHECK_ALL, m_bAll);
}

BEGIN_MESSAGE_MAP(CStashSaveDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CStashSaveDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_UNTRACKED, &CStashSaveDlg::OnBnClickedCheckUntracked)
	ON_BN_CLICKED(IDC_CHECK_ALL, &CStashSaveDlg::OnBnClickedCheckAll)
END_MESSAGE_MAP()

BOOL CStashSaveDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_CHECK_UNTRACKED);
	AdjustControlSize(IDC_CHECK_ALL);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP_STASHMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STASHMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	this->UpdateData(false);

	return TRUE;
}

void CStashSaveDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	if (m_bIncludeUntracked)
	{
		if (CMessageBox::ShowCheck(GetSafeHwnd(), IDS_STASHSAVE_INCLUDEUNTRACKED, IDS_APPNAME, 2, IDI_WARNING, IDS_CONTINUEBUTTON, IDS_ABORTBUTTON, NULL, L"NoStashIncludeUntrackedWarning", IDS_PROC_NOTSHOWAGAINCONTINUE) == 2)
		{
			CMessageBox::RemoveRegistryKey(L"NoStashIncludeUntrackedWarning"); // only store answer if it is "Continue"
			return;
		}
	}

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CStashSaveDlg::OnBnClickedCheckUntracked()
{
	UpdateData();
	DialogEnableWindow(IDC_CHECK_ALL, !m_bIncludeUntracked);
}

void CStashSaveDlg::OnBnClickedCheckAll()
{
	UpdateData();
	DialogEnableWindow(IDC_CHECK_UNTRACKED, !m_bAll);
}
