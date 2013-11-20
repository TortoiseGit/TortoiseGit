// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013 - TortoiseGit

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

CRefLogDlg::CRefLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRefLogDlg::IDD, pParent)
	, m_pFindDialog(NULL)
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
	ON_MESSAGE(MSG_REFLOG_CHANGED,OnRefLogChanged)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
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
	AddAnchor(IDC_REFLOG_LIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_REF, TOP_LEFT, TOP_RIGHT);

	AddOthersToAnchor();
	this->EnableSaveRestore(_T("RefLogDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_ChooseRef.SetMaxHistoryItems(0x7FFFFFFF);

	m_RefList.m_hasWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);

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
		int selIndex = m_RefList.GetNextSelectedItem(pos);
		if (selIndex < m_RefList.m_arShownList.GetCount())
		{
			// all ok, pick up the revision
			GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_RefList.m_arShownList.GetAt(selIndex));
			// extract the hash
			m_SelectedHash = pLogEntry->m_CommitHash;
		}
	}

	OnOK();
}
void CRefLogDlg::OnBnClickedClearStash()
{
	if (CMessageBox::Show(this->GetSafeHwnd(), IDS_PROC_DELETEALLSTASH, IDS_APPNAME, 2, IDI_QUESTION, IDS_DELETEBUTTON, IDS_ABORTBUTTON) == 1)
	{
		CString cmdOut;
		if (g_Git.Run(_T("git.exe stash clear"), &cmdOut, CP_UTF8))
		{
			MessageBox(cmdOut, _T("TortoiseGit"), MB_ICONERROR);
			return;
		}

		m_RefList.m_RevCache.clear();

		OnCbnSelchangeRef();
	}
}

int AddToRefLoglist(unsigned char * /*osha1*/, unsigned char *nsha1, const char * /*name*/, unsigned long /*time*/, int /*sz*/, const char *msg, void *data)
{
	std::vector<GitRev> *vector = (std::vector<GitRev> *)data;
	GitRev rev;
	rev.m_CommitHash = (char *)nsha1;

	CString one;
	g_Git.StringAppend(&one, (BYTE *)msg);

	int message = one.Find(_T(":"), 0);
	if (message > 0)
	{
		rev.m_RefAction = one.Left(message);
		rev.GetSubject() = one.Mid(message + 1);
	}

	vector->insert(vector->begin(), rev); 

	return 0;
}

int ParserFromRefLog(CString ref, std::vector<GitRev> &refloglist)
{
	refloglist.clear();
	if (g_Git.m_IsUseGitDLL)
	{
		git_for_each_reflog_ent(CUnicodeUtils::GetUTF8(ref), AddToRefLoglist, &refloglist);
		for (size_t i = 0; i < refloglist.size(); ++i)
			refloglist[i].m_Ref.Format(_T("%s@{%d}"), ref, i);
	}
	else
	{
		CString cmd, out;
		GitRev rev;
		cmd.Format(_T("git.exe reflog show %s"), ref);
		if (g_Git.Run(cmd, &out, NULL, CP_UTF8))
			return -1;

		int pos = 0;
		while (pos >= 0)
		{
			CString one = out.Tokenize(_T("\n"), pos);
			int ref = one.Find(_T(' '), 0);
			if (ref < 0)
				continue;

			rev.Clear();

			if (g_Git.GetHash(rev.m_CommitHash, one.Left(ref)))
			{
				MessageBox(NULL, g_Git.GetGitLastErr(_T("Could not get hash of ") + one.Left(ref) + _T(".")), _T("TortoiseGit"), MB_ICONERROR);
				return -1;
			}
			int action = one.Find(_T(' '), ref + 1);
			if (action > 0)
			{
				rev.m_Ref = one.Mid(ref + 1, action - ref - 2);
				int message = one.Find(_T(":"), action);
				if (message > 0)
				{
					rev.m_RefAction = one.Mid(action + 1, message - action - 1);
					rev.GetSubject() = one.Right(one.GetLength() - message - 1);
				}
			}

			refloglist.push_back(rev);
		}
	}
	return 0;
}


void CRefLogDlg::OnCbnSelchangeRef()
{
	CString ref=m_ChooseRef.GetString();
	m_RefList.ClearText();

	//this->m_logEntries.ParserFromLog();
	m_RefList.SetRedraw(false);

	ParserFromRefLog(ref, m_RefList.m_RevCache);

	m_RefList.SetItemCountEx((int)m_RefList.m_RevCache.size());

	this->m_RefList.m_arShownList.RemoveAll();

	for (unsigned int i = 0; i < m_RefList.m_RevCache.size(); ++i)
	{
		GitRev *rev = &m_RefList.m_RevCache[i];
		rev->m_IsFull = TRUE;
		this->m_RefList.m_arShownList.Add(rev);
	}

	m_RefList.SetRedraw(true);

	m_RefList.Invalidate();

	if (ref == _T("refs/stash"))
	{
		GetDlgItem(IDC_REFLOG_BUTTONCLEARSTASH)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_REFLOG_BUTTONCLEARSTASH)->EnableWindow((m_RefList.m_arShownList.GetSize() > 0));
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

void CRefLogDlg::Refresh()
{
	STRING_VECTOR list;
	list.push_back(_T("HEAD"));
	if (g_Git.GetRefList(list))
		MessageBox(g_Git.GetGitLastErr(_T("Could not get all refs.")), _T("TortoiseGit"), MB_ICONERROR);

	m_ChooseRef.AddString(list);

	if (m_CurrentBranch.IsEmpty())
	{
		m_CurrentBranch.Format(_T("refs/heads/%s"), g_Git.GetCurrentBranch());
		m_ChooseRef.SetCurSel(0); /* Choose HEAD */
	}
	else
	{
		bool found = false;
		for (int i = 0; i < list.size(); ++i)
		{
			if(list[i] == m_CurrentBranch)
			{
				m_ChooseRef.SetCurSel(i);
				found = true;
				break;
			}
		}
		if (!found)
			m_ChooseRef.SetCurSel(0);
	}

	m_RefList.m_RevCache.clear();

	OnCbnSelchangeRef();
}

void CRefLogDlg::OnFind()
{
	m_nSearchLine = 0;
	m_pFindDialog = new CFindReplaceDialog();
	m_pFindDialog->Create(TRUE, _T(""), NULL, FR_DOWN | FR_HIDEWHOLEWORD | FR_HIDEUPDOWN, this);
}

LRESULT CRefLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	ASSERT(m_pFindDialog != NULL);

	if (m_RefList.m_arShownList.IsEmpty())
		return 0;

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDialog->IsTerminating())
	{
			m_pFindDialog = NULL;
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
		bool bCaseSensitive = !!(m_pFindDialog->m_nFlags & FR_MATCHCASE);

		if (!bCaseSensitive)
			findString.MakeLower();

		int i = m_nSearchLine;
		if (i < 0 || i >= m_RefList.m_arShownList.GetCount())
			i = 0;

		do
		{
			GitRev * data = (GitRev*)m_RefList.m_arShownList.SafeGetAt(i);

			CString str;
			str += data->m_Ref;
			str += _T("\n");
			str += data->m_RefAction;
			str += _T("\n");
			str += data->m_CommitHash.ToString();
			str += _T("\n");
			str += data->GetSubject();
			str += _T("\n");
			str += data->GetBody();
			str += _T("\n");

			if (!bCaseSensitive)
				str.MakeLower();

			if (str.Find(findString) >= 0)
				bFound = true;

			++i;
			if(!bFound && i >= m_RefList.m_arShownList.GetCount())
				i=0;
		} while (i != m_nSearchLine && (!bFound));

		if (bFound)
		{
			m_RefList.SetHotItem(i - 1);
			m_RefList.EnsureVisible(i - 1, FALSE);
			m_nSearchLine = i;
		}
		else
			MessageBox(_T("\"") + findString + _T("\" ") + CString(MAKEINTRESOURCE(IDS_NOTFOUND)), _T("TortoiseGit"), MB_ICONINFORMATION);
	}

	return 0;
}
