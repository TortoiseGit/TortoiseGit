// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011 - TortoiseGit

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
#include "git.h"
#include "AppUtils.h"

// CRefLogDlg dialog

IMPLEMENT_DYNAMIC(CRefLogDlg, CResizableStandAloneDialog)

CRefLogDlg::CRefLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRefLogDlg::IDD, pParent)
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
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_REF,   &CRefLogDlg::OnCbnSelchangeRef)
	ON_MESSAGE(MSG_REFLOG_CHANGED,OnRefLogChanged)
END_MESSAGE_MAP()

LRESULT CRefLogDlg::OnRefLogChanged(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	m_RefList.m_RefMap.clear();
	OnCbnSelchangeRef();
	return 0;
}

BOOL CRefLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	AddAnchor(IDC_REFLOG_LIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();
	this->EnableSaveRestore(_T("RefLogDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	STRING_VECTOR list;
	list.push_back(_T("HEAD"));
	g_Git.GetRefList(list);

	m_RefList.m_hasWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);

	m_ChooseRef.SetMaxHistoryItems(0x7FFFFFFF);
	this->m_ChooseRef.AddString(list);

	this->m_RefList.InsertRefLogColumn();
	//m_RefList.m_logEntries.ParserFromRefLog(_T("master"));
	if(this->m_CurrentBranch.IsEmpty())
	{
		m_CurrentBranch.Format(_T("refs/heads/%s"),g_Git.GetCurrentBranch());
		m_ChooseRef.SetCurSel(0); /* Choose HEAD */
	}
	else
	{
		for(int i=0;i<list.size();i++)
		{
			if(list[i] == m_CurrentBranch)
			{
				m_ChooseRef.SetCurSel(i);
				break;
			}
		}
	}


	OnCbnSelchangeRef();

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

void CRefLogDlg::OnCbnSelchangeRef()
{
	CString ref=m_ChooseRef.GetString();
	if(m_RefList.m_RefMap.find(ref) == m_RefList.m_RefMap.end())
	{
		m_RefList.m_RefMap[ref].m_pLogCache = &m_RefList.m_LogCache;
		m_RefList.m_RefMap[ref].ParserFromRefLog(ref);
	}
	m_RefList.ClearText();

	//this->m_logEntries.ParserFromLog();
	m_RefList.SetRedraw(false);

	CLogDataVector *plog;
	plog = &m_RefList.m_RefMap[ref];

	m_RefList.SetItemCountEx(plog->size());

	this->m_RefList.m_arShownList.RemoveAll();

	for(unsigned int i=0;i<m_RefList.m_RefMap[ref].size();i++)
	{
		plog->GetGitRevAt(i).m_IsFull=TRUE;
		this->m_RefList.m_arShownList.Add(&(plog->GetGitRevAt(i)));

	}

	m_RefList.SetRedraw(true);

	m_RefList.Invalidate();

}