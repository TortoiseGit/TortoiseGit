// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2017, 2019-2020 - TortoiseGit

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

#include "Git.h"
#include "AppUtils.h"
#include "TortoiseProc.h"
#include "SelectRemoteRefDlg.h"
#include "MessageBox.h"
#include "MassiveGitTask.h"
#include "SysProgressDlg.h"

IMPLEMENT_DYNAMIC(CSelectRemoteRefDlg, CHorizontalResizableStandAloneDialog)

CSelectRemoteRefDlg::CSelectRemoteRefDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CSelectRemoteRefDlg::IDD, pParent)
{
}

CSelectRemoteRefDlg::~CSelectRemoteRefDlg()
{
}

void CSelectRemoteRefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_BRANCH, m_ctrlRefs);
}

BEGIN_MESSAGE_MAP(CSelectRemoteRefDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CSelectRemoteRefDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(static_cast<UINT>(IDC_STATIC));

	AddAnchor(IDC_EDIT_REMOTE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	m_ctrlRefs.SetCaseSensitive(TRUE);
	m_ctrlRefs.SetMaxHistoryItems(0x7FFFFFFF);

	GetDlgItem(IDC_EDIT_REMOTE)->SetWindowText(m_sRemote);

	Refresh();

	EnableSaveRestore(L"SelectRemoteRefDlg");

	return TRUE;
}

void CSelectRemoteRefDlg::Refresh()
{
	m_ctrlRefs.Clear();

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_LOADING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.ShowModal(this, true);
	REF_VECTOR refs;
	if (g_Git.GetRemoteRefs(m_sRemote, refs, false, true))
	{
		sysProgressDlg.Stop();
		MessageBox(g_Git.GetGitLastErr(L"Could not retrieve remote refs.", CGit::GIT_CMD_FETCH), L"TortoiseGit", MB_ICONERROR);
	}
	sysProgressDlg.Stop();
	BringWindowToTop();

	for (int i = 0; i < static_cast<int>(refs.size()); ++i)
	{
		m_ctrlRefs.AddString(refs[i].name);
	}

	if (m_ctrlRefs.GetCount() > 0)
		m_ctrlRefs.SetCurSel(0);
	DialogEnableWindow(IDOK, m_ctrlRefs.GetCount() > 0 ? TRUE : FALSE);
}

void CSelectRemoteRefDlg::OnBnClickedOk()
{
	m_sRemoteBranch = m_ctrlRefs.GetString();
	OnOK();
}

BOOL CSelectRemoteRefDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}

	return CHorizontalResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
