// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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
#include "DeleteRemoteTagDlg.h"
#include "Messagebox.h"

IMPLEMENT_DYNAMIC(CDeleteRemoteTagDlg, CHorizontalResizableStandAloneDialog)

CDeleteRemoteTagDlg::CDeleteRemoteTagDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CDeleteRemoteTagDlg::IDD, pParent)
{
}

CDeleteRemoteTagDlg::~CDeleteRemoteTagDlg()
{
}

void CDeleteRemoteTagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS, m_ctrlTags);
}

BEGIN_MESSAGE_MAP(CDeleteRemoteTagDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_TAGS, OnCbnSelchangeTags)
END_MESSAGE_MAP()

BOOL CDeleteRemoteTagDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	AdjustControlSize(IDC_STATIC);

	Refresh();

	EnableSaveRestore(_T("DeleteRemoteTagDlg"));

	return TRUE;
}

void CDeleteRemoteTagDlg::Refresh()
{
	m_ctrlTags.SetMaxHistoryItems(0x0FFFFFFF);
	m_ctrlTags.Reset();

	STRING_VECTOR list;
	g_Git.GetRemoteTags(m_sRemote, list);
	m_ctrlTags.AddString(list);

	m_ctrlTags.SetCurSel(-1);
	DialogEnableWindow(IDOK, FALSE);
}

void CDeleteRemoteTagDlg::OnBnClickedOk()
{
	this->UpdateData(TRUE);

	CString msg;
	msg.Format(IDS_PROC_DELETEBRANCHTAG, m_ctrlTags.GetString());
	if (CMessageBox::Show(m_hWnd, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
	{
		CString cmd, out;
		cmd.Format(_T("git.exe push %s :refs/tags/%s"), m_sRemote, m_ctrlTags.GetString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			MessageBox(out, _T("TortoiseGit"), MB_ICONERROR);
			return;
		}
		m_ctrlTags.RemoveSelectedItem();
		m_ctrlTags.SetCurSel(-1);
		DialogEnableWindow(IDOK, FALSE);
	}
}

void CDeleteRemoteTagDlg::OnCbnSelchangeTags()
{
	DialogEnableWindow(IDOK, m_ctrlTags.GetCurSel() != 1);
}

BOOL CDeleteRemoteTagDlg::PreTranslateMessage(MSG* pMsg)
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
