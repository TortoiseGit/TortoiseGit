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
	DDX_Control(pDX, IDC_LIST_TAGS, m_ctrlTags);
}

BEGIN_MESSAGE_MAP(CDeleteRemoteTagDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TAGS, OnSelchangeTags)
END_MESSAGE_MAP()

BOOL CDeleteRemoteTagDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_LIST_TAGS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	AdjustControlSize((UINT)IDC_STATIC);

	CString temp;
	temp.LoadString(IDS_PROC_TAG);
	m_ctrlTags.InsertColumn(0, temp, 0, -1);
	m_ctrlTags.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlTags.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);

	Refresh();

	EnableSaveRestore(_T("DeleteRemoteTagDlg"));

	return TRUE;
}

void CDeleteRemoteTagDlg::Refresh()
{
	m_ctrlTags.DeleteAllItems();
	m_taglist.clear();

	g_Git.GetRemoteTags(m_sRemote, m_taglist);

	for (int i = 0; i < (int)m_taglist.size(); i++)
	{
		m_ctrlTags.InsertItem(i, m_taglist[i]);
	}

	DialogEnableWindow(IDOK, FALSE);
}

void CDeleteRemoteTagDlg::OnBnClickedOk()
{
	if (m_ctrlTags.GetSelectedCount() > 1)
	{
		CString msg;
		msg.Format(IDS_PROC_DELETENREFS, m_ctrlTags.GetSelectedCount());
		if (CMessageBox::Show(m_hWnd, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
			return;
	}
	else // GetSelectedCount() is 1, otherwise the button is disabled
	{
		POSITION pos = m_ctrlTags.GetFirstSelectedItemPosition();
		CString msg;
		msg.Format(IDS_PROC_DELETEBRANCHTAG, m_taglist[(m_ctrlTags.GetNextSelectedItem(pos))]);
		if (CMessageBox::Show(m_hWnd, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
			return;
	}

	CString cmd, out;
	cmd.Format(_T("git.exe push %s"), m_sRemote);

	POSITION pos = m_ctrlTags.GetFirstSelectedItemPosition();
	int index;
	while ((index = m_ctrlTags.GetNextSelectedItem(pos)) >= 0)
	{
		cmd += _T(" :refs/tags/") + m_taglist[index];
	}

	if (g_Git.Run(cmd, &out, CP_UTF8))
	{
		MessageBox(out, _T("TortoiseGit"), MB_ICONERROR);
	}
	Refresh();
}

void CDeleteRemoteTagDlg::OnSelchangeTags(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	DialogEnableWindow(IDOK, m_ctrlTags.GetSelectedCount() > 0);
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
