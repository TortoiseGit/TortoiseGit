// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit

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
// DeleteConflictDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DeleteConflictDlg.h"
#include "AppUtils.h"
#include "Git.h"

// CDeleteConflictDlg dialog

IMPLEMENT_DYNAMIC(CDeleteConflictDlg, CStandAloneDialog)

CDeleteConflictDlg::CDeleteConflictDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CDeleteConflictDlg::IDD, pParent)
	, m_bShowModifiedButton(FALSE)
	, m_bIsDelete(FALSE)
	, m_bDiffMine(true)
{
}

CDeleteConflictDlg::~CDeleteConflictDlg()
{
}

void CDeleteConflictDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_LOCAL_STATUS, m_LocalStatus);
	DDX_Text(pDX, IDC_REMOTE_STATUS, m_RemoteStatus);
	GetDlgItem(IDC_FROMHASH)->SetWindowText(m_LocalHash.ToString());
	GetDlgItem(IDC_TOHASH)->SetWindowText(m_RemoteHash.ToString());
}


BEGIN_MESSAGE_MAP(CDeleteConflictDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, OnBnClickedLog)
	ON_BN_CLICKED(IDC_LOG2, OnBnClickedLog2)
	ON_BN_CLICKED(IDC_DELETE, &CDeleteConflictDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_MODIFY, &CDeleteConflictDlg::OnBnClickedModify)
	ON_BN_CLICKED(IDHELP, &OnHelp)
	ON_BN_CLICKED(IDC_SHOWDIFF, &CDeleteConflictDlg::OnBnClickedShowdiff)
	ON_BN_CLICKED(IDC_SHOWDIFF2, &CDeleteConflictDlg::OnBnClickedShowdiff)
END_MESSAGE_MAP()


BOOL CDeleteConflictDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	if(this->m_bShowModifiedButton )
	{
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(CString(MAKEINTRESOURCE(IDS_SVNACTION_MODIFIED)));
		if (m_bDiffMine)
			GetDlgItem(IDC_SHOWDIFF)->ShowWindow(SW_SHOW);
		else
			GetDlgItem(IDC_SHOWDIFF2)->ShowWindow(SW_SHOW);
	}
	else
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_CREATED)));
	if (m_LocalHash.IsEmpty())
		GetDlgItem(IDC_LOG)->ShowWindow(SW_HIDE);
	if (m_RemoteHash.IsEmpty())
		GetDlgItem(IDC_LOG2)->ShowWindow(SW_HIDE);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_File), sWindowTitle);

	GetDlgItem(IDC_INFOLABEL)->SetWindowText(m_File.GetGitPathString());

	GetDlgItem(IDCANCEL)->SetFocus();

	return FALSE;
}
// CDeleteConflictDlg message handlers

void CDeleteConflictDlg::OnBnClickedLog()
{
	ShowLog(m_LocalHash.ToString());
}

void CDeleteConflictDlg::OnBnClickedLog2()
{
	ShowLog(m_RemoteHash.ToString());
}

void CDeleteConflictDlg::OnBnClickedDelete()
{
	m_bIsDelete = TRUE;
	OnOK();
}

void CDeleteConflictDlg::OnBnClickedModify()
{
	m_bIsDelete = FALSE;
	OnOK();
}

void CDeleteConflictDlg::ShowLog(CString hash)
{
	CString sCmd;
	sCmd.Format(L"/command:log /path:\"%s\" /endrev:%s", static_cast<LPCTSTR>(g_Git.CombinePath(m_File)), static_cast<LPCTSTR>(hash));
	CAppUtils::RunTortoiseGitProc(sCmd, false, false);
}

void CDeleteConflictDlg::OnBnClickedShowdiff()
{
	CString base;
	base.LoadString(IDS_PROC_DIFF_BASE);
	CAppUtils::DiffFlags flags;
	flags.bAlternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	CAppUtils::StartExtDiff(m_FileBaseVersion.GetWinPathString(), g_Git.CombinePath(m_File),
		base,
		m_bDiffMine ? m_LocalHash.ToString() : m_RemoteHash.ToString(),
		m_FileBaseVersion.GetWinPathString(),
		g_Git.CombinePath(m_File),
		m_bDiffMine ? m_LocalHash : m_RemoteHash,
		m_bDiffMine ? m_RemoteHash : m_LocalHash,
		flags);
}
