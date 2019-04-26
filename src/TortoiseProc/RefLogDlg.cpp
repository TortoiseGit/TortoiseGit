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
// RefLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "RefLogDlg.h"
#include "Git.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"

// CRefLogDlg dialog

IMPLEMENT_DYNAMIC(CRefLogDlg, CResizableStandAloneDialog)

UINT CRefLogDlg::m_FindDialogMessage = ::RegisterWindowMessage(FINDMSGSTRING);

CRefLogDlg::CRefLogDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CRefLogDlg::IDD, pParent)
	, m_pFindDialog(nullptr)
	, m_nSearchLine(0)
{
}

CRefLogDlg::~CRefLogDlg()
{
}

void CRefLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_REF, m_ChooseRef);
	DDX_Control(pDX, IDC_REFLOG_LIST, m_RefList);
}


BEGIN_MESSAGE_MAP(CRefLogDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CRefLogDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_REFLOG_BUTTONCLEARSTASH, &CRefLogDlg::OnBnClickedClearStash)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_REF,   &CRefLogDlg::OnCbnSelchangeRef)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REFLOG_LIST, OnLvnItemchangedRefLoglist)
	ON_MESSAGE(MSG_REFLOG_CHANGED,OnRefLogChanged)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
	ON_BN_CLICKED(IDC_SEARCH, OnFind)
END_MESSAGE_MAP()

LRESULT CRefLogDlg::OnRefLogChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_RefList.m_RevCache.clear();
	OnCbnSelchangeRef();
	return 0;
}

BOOL CRefLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_REFLOG_BUTTONCLEARSTASH, BOTTOM_LEFT);
	AddAnchor(IDC_SEARCH, BOTTOM_LEFT);
	AddAnchor(IDC_REFLOG_LIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_REF, TOP_LEFT, TOP_RIGHT);

	AddOthersToAnchor();
	this->EnableSaveRestore(L"RefLogDlg");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_ChooseRef.SetMaxHistoryItems(0x7FFFFFFF);

	m_RefList.m_hasWC = !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir);

	this->m_RefList.InsertRefLogColumn();

	Refresh();

	return TRUE;
}
// CRefLogDlg message handlers

void CRefLogDlg::OnBnClickedOk()
{
	if (m_RefList.GetSelectedCount() == 1)
	{
		// get the selected row
		POSITION pos = m_RefList.GetFirstSelectedItemPosition();
		size_t selIndex = m_RefList.GetNextSelectedItem(pos);
		if (selIndex < m_RefList.m_arShownList.size())
		{
			// all ok, pick up the revision
			GitRev* pLogEntry = m_RefList.m_arShownList.SafeGetAt(selIndex);
			// extract the hash
			m_SelectedHash = pLogEntry->m_CommitHash;
		}
	}

	OnOK();
}
void CRefLogDlg::OnBnClickedClearStash()
{
	size_t count = m_RefList.m_arShownList.size();
	CString msg;
	msg.Format(IDS_PROC_DELETEALLSTASH, count);
	if (CMessageBox::Show(this->GetSafeHwnd(), msg, L"TortoiseGit", 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
	{
		CString cmdOut;
		if (g_Git.Run(L"git.exe stash clear", &cmdOut, CP_UTF8))
		{
			MessageBox(cmdOut, L"TortoiseGit", MB_ICONERROR);
			return;
		}

		m_RefList.m_RevCache.clear();

		OnCbnSelchangeRef();
	}
}

void CRefLogDlg::OnCbnSelchangeRef()
{
	m_CurrentBranch = m_ChooseRef.GetString(); // remember selected branch
	m_RefList.ClearText();

	m_RefList.SetRedraw(false);

	if (CString err; GitRevLoglist::GetRefLog(m_CurrentBranch, m_RefList.m_RevCache, err))
		MessageBox(L"Error while loading reflog.\n" + err, L"TortoiseGit", MB_ICONERROR);

	m_RefList.SetItemCountEx(static_cast<int>(m_RefList.m_RevCache.size()));

	this->m_RefList.m_arShownList.clear();

	for (unsigned int i = 0; i < m_RefList.m_RevCache.size(); ++i)
	{
		GitRevLoglist* rev = &m_RefList.m_RevCache[i];
		rev->m_IsFull = TRUE;
		this->m_RefList.m_arShownList.SafeAdd(rev);
	}

	m_RefList.SetRedraw(true);

	m_RefList.Invalidate();

	// reset search start positions
	m_RefList.m_nSearchIndex = 0;
	m_nSearchLine = 0;

	if (m_CurrentBranch == L"refs/stash")
	{
		GetDlgItem(IDC_REFLOG_BUTTONCLEARSTASH)->ShowWindow(SW_SHOW);
		BOOL enabled = !m_RefList.m_arShownList.empty();
		GetDlgItem(IDC_REFLOG_BUTTONCLEARSTASH)->EnableWindow(enabled);
		if (!enabled)
			GetDlgItem(IDOK)->SetFocus();
	}
	else
		GetDlgItem(IDC_REFLOG_BUTTONCLEARSTASH)->ShowWindow(SW_HIDE);
}

BOOL CRefLogDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F5)
		Refresh();
	else if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_F3 || (pMsg->wParam == 'F' && (GetAsyncKeyState(VK_CONTROL) & 0x8000))))
		OnFind();

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CRefLogDlg::OnLvnItemchangedRefLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem < 0)
		return;
	m_RefList.m_nSearchIndex = pNMLV->iItem;
	m_nSearchLine = pNMLV->iItem;
}

void CRefLogDlg::Refresh()
{
	STRING_VECTOR list;
	list.push_back(L"HEAD");
	if (g_Git.GetRefList(list))
		MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);

	m_ChooseRef.SetList(list);

	if (m_CurrentBranch.IsEmpty())
		m_CurrentBranch = L"HEAD";

	bool found = false;
	for (int i = 0; i < static_cast<int>(list.size()); ++i)
	{
		if (list[i] == m_CurrentBranch)
		{
			m_ChooseRef.SetCurSel(i);
			found = true;
			break;
		}
	}
	if (!found)
		m_ChooseRef.SetCurSel(0); /* Choose HEAD */

	m_RefList.m_RevCache.clear();

	OnCbnSelchangeRef();
}

void CRefLogDlg::OnFind()
{
	if (m_pFindDialog)
		return;
	m_pFindDialog = new CFindReplaceDialog();
	m_pFindDialog->Create(TRUE, L"", nullptr, FR_DOWN | FR_HIDEWHOLEWORD | FR_HIDEUPDOWN, this);
}

LRESULT CRefLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	ASSERT(m_pFindDialog);

	if (m_RefList.m_arShownList.empty())
		return 0;

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDialog->IsTerminating())
	{
			m_pFindDialog = nullptr;
			return 0;
	}

	// If the FR_FINDNEXT flag is set,
	// call the application-defined search routine
	// to search for the requested string.
	if (m_pFindDialog->FindNext())
	{
		//read data from dialog
		CString findString = m_pFindDialog->GetFindString();

		bool bFound = false;
		bool bCaseSensitive = !!(m_pFindDialog->MatchCase());

		if (!bCaseSensitive)
			findString.MakeLower();

		size_t i = m_nSearchLine;
		if (i >= m_RefList.m_arShownList.size())
		{
			i = 0;
			m_pFindDialog->FlashWindowEx(FLASHW_ALL, 2, 100);
		}

		do
		{
			GitRevLoglist* data = m_RefList.m_arShownList.SafeGetAt(i);

			CString str;
			str += data->m_Ref;
			str += L'\n';
			str += data->m_RefAction;
			str += L'\n';
			str += data->m_CommitHash.ToString();
			str += L'\n';
			str += data->GetSubject();
			str += L'\n';
			str += data->GetBody();
			str += L'\n';

			if (!bCaseSensitive)
				str.MakeLower();

			if (str.Find(findString) >= 0)
				bFound = true;

			++i;
			if(!bFound && i >= m_RefList.m_arShownList.size())
				i=0;
		} while (i != m_nSearchLine && (!bFound));

		if (bFound)
		{
			m_RefList.SetHotItem(static_cast<int>(i) - 1);
			m_RefList.EnsureVisible(static_cast<int>(i) - 1, FALSE);
			m_nSearchLine = i;
		}
		else
			MessageBox(L'"' + findString + L"\" " + CString(MAKEINTRESOURCE(IDS_NOTFOUND)), L"TortoiseGit", MB_ICONINFORMATION);
	}

	return 0;
}
