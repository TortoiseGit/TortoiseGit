// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng
// Copyright (C) 2012-2016, 2018-2019 - TortoiseGit

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
#include "RevGraphFilterDlg.h"
#include "gittype.h"
#include "Git.h"
#include "BrowseRefsDlg.h"
#include "StringUtils.h"

IMPLEMENT_DYNAMIC(CRevGraphFilterDlg, CDialog)

CRevGraphFilterDlg::CRevGraphFilterDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CRevGraphFilterDlg::IDD, pParent)
	, m_bCurrentBranch(FALSE)
	, m_bLocalBranches(FALSE)
{
}

CRevGraphFilterDlg::~CRevGraphFilterDlg()
{
}

void CRevGraphFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FROMREV, m_sFromRev);
	DDX_Text(pDX, IDC_TOREV, m_sToRev);
	DDX_Check(pDX, IDC_CURRENT_BRANCH, m_bCurrentBranch);
	DDX_Check(pDX, IDC_LOCAL_BRANCHES, m_bLocalBranches);
	DDX_Control(pDX, IDC_FROMREV, m_ctrlFromRev);
	DDX_Control(pDX, IDC_TOREV, m_ctrlToRev);
}

BEGIN_MESSAGE_MAP(CRevGraphFilterDlg, CDialog)
	ON_BN_CLICKED(IDC_REV1BTN1, &CRevGraphFilterDlg::OnBnClickedRev1btn1)
	ON_BN_CLICKED(IDC_REV1BTN2, &CRevGraphFilterDlg::OnBnClickedRev1btn2)
	ON_BN_CLICKED(IDC_RESETFILTER, &CRevGraphFilterDlg::OnBnClickedResetfilter)
	ON_BN_CLICKED(IDC_CURRENT_BRANCH, &CRevGraphFilterDlg::OnBnClickedCurrentBranch)
	ON_BN_CLICKED(IDC_LOCAL_BRANCHES, &CRevGraphFilterDlg::OnBnClickedLocalBranches)
END_MESSAGE_MAP()

BOOL CRevGraphFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	this->m_ctrlFromRev.Init();
	this->m_ctrlToRev.Init();

	STRING_VECTOR list;
	if (g_Git.GetRefList(list))
		MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);

	m_ctrlFromRev.AddSearchString(L"HEAD");
	m_ctrlToRev.AddSearchString(L"HEAD");

	for (size_t i = 0; i < list.size(); ++i)
	{
		CString str=list[i];

		m_ctrlFromRev.AddSearchString(list[i]);
		m_ctrlToRev.AddSearchString(list[i]);

		if (CStringUtils::StartsWith(str, L"refs/"))
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/"))));
			m_ctrlToRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/"))));
		}

		if (CStringUtils::StartsWith(str, L"refs/heads/"))
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/heads/"))));
			m_ctrlToRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/heads/"))));
		}

		if (CStringUtils::StartsWith(str, L"refs/remotes/"))
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/remotes/"))));
			m_ctrlToRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/remotes/"))));
		}

		if (CStringUtils::StartsWith(str, L"refs/tags/"))
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/tags/"))));
			m_ctrlToRev.AddSearchString(list[i].Mid(static_cast<int>(wcslen(L"refs/tags/"))));
		}
	}

	OnBnClickedCurrentBranch();
	OnBnClickedLocalBranches();

	return TRUE;
}


void CRevGraphFilterDlg::GetRevisionRange(CString& minrev, CString& maxrev)
{
	minrev = m_sFromRev;
	maxrev = m_sToRev;
}

void CRevGraphFilterDlg::SetRevisionRange (CString minrev, CString maxrev)
{
	m_sFromRev = minrev;
	m_sToRev = maxrev;
}

void CRevGraphFilterDlg::OnOK()
{
	CDialog::OnOK();
}

void CRevGraphFilterDlg::OnBnClickedRev1btn1()
{
	CString str = CBrowseRefsDlg::PickRef();
	if(str.IsEmpty())
		return;

	m_ctrlFromRev.SetWindowText(str);
}

void CRevGraphFilterDlg::OnBnClickedRev1btn2()
{
	CString str = CBrowseRefsDlg::PickRef();
	if(str.IsEmpty())
		return;

	m_ctrlToRev.SetWindowText(str);
}

void CRevGraphFilterDlg::OnBnClickedResetfilter()
{
	m_sFromRev.Empty();
	m_sToRev.Empty();
	m_bCurrentBranch = FALSE;
	m_bLocalBranches = FALSE;
	UpdateData(FALSE);
	CDialog::OnOK();
}

void CRevGraphFilterDlg::OnBnClickedCurrentBranch()
{
	UpdateData();
	if (m_bCurrentBranch)
	{
		m_bLocalBranches = FALSE;
		GetDlgItem(IDC_LOCAL_BRANCHES)->EnableWindow(FALSE);
		UpdateData(FALSE);
	}
	else
		GetDlgItem(IDC_LOCAL_BRANCHES)->EnableWindow(TRUE);
}

void CRevGraphFilterDlg::OnBnClickedLocalBranches()
{
	UpdateData();
	if (m_bLocalBranches)
	{
		m_bCurrentBranch = FALSE;
		GetDlgItem(IDC_CURRENT_BRANCH)->EnableWindow(FALSE);
		UpdateData(FALSE);
	}
	else
		GetDlgItem(IDC_CURRENT_BRANCH)->EnableWindow(TRUE);
}
