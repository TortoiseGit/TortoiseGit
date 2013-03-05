// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseGit

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
#include "MassiveGitTask.h"
#include "SysProgressDlg.h"

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
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Control(pDX, IDC_LIST_TAGS, m_ctrlTags);
}

BEGIN_MESSAGE_MAP(CDeleteRemoteTagDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TAGS, OnSelchangeTags)
END_MESSAGE_MAP()

BOOL CDeleteRemoteTagDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_LIST_TAGS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_RIGHT);
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
	m_SelectAll.SetCheck(BST_UNCHECKED);
	m_taglist.clear();

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_LOADING)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.ShowModal(this, true);
	g_Git.GetRemoteTags(m_sRemote, m_taglist);
	sysProgressDlg.Stop();

	for (int i = 0; i < (int)m_taglist.size(); ++i)
	{
		m_ctrlTags.InsertItem(i, m_taglist[i]);
	}

	DialogEnableWindow(IDOK, FALSE);
}

void CDeleteRemoteTagDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	if (state == BST_UNCHECKED)
	{
		m_ctrlTags.SetItemState(-1, 0, LVIS_SELECTED);
	}
	else
	{
		for (int i = 0; i < m_ctrlTags.GetItemCount(); ++i)
			m_ctrlTags.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}
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

	CMassiveGitTask mgtPush(_T("push ") + m_sRemote, FALSE);

	POSITION pos = m_ctrlTags.GetFirstSelectedItemPosition();
	int index;
	while ((index = m_ctrlTags.GetNextSelectedItem(pos)) >= 0)
	{
		mgtPush.AddFile(_T(":refs/tags/") + m_taglist[index]);
	}

	CSysProgressDlg sysProgressDlg;
	sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
	sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
	sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
	sysProgressDlg.SetShowProgressBar(false);
	sysProgressDlg.ShowModal(this, true);
	BOOL cancel = FALSE;
	mgtPush.Execute(cancel);
	sysProgressDlg.Stop();
	Refresh();
}

void CDeleteRemoteTagDlg::OnSelchangeTags(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	DialogEnableWindow(IDOK, m_ctrlTags.GetSelectedCount() > 0);
	if (m_ctrlTags.GetSelectedCount() == 0)
		m_SelectAll.SetCheck(BST_UNCHECKED);
	else if ((int)m_ctrlTags.GetSelectedCount() < m_ctrlTags.GetItemCount())
		m_SelectAll.SetCheck(BST_INDETERMINATE);
	else
		m_SelectAll.SetCheck(BST_CHECKED);
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
