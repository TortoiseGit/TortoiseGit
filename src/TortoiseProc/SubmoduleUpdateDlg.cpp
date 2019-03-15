// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2017, 2019 - TortoiseGit

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
#include "SubmoduleUpdateDlg.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"

IMPLEMENT_DYNAMIC(CSubmoduleUpdateDlg, CResizableStandAloneDialog)

bool CSubmoduleUpdateDlg::s_bSortLogical = true;

CSubmoduleUpdateDlg::CSubmoduleUpdateDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CSubmoduleUpdateDlg::IDD, pParent)
	, m_bInit(TRUE)
	, m_bRecursive(FALSE)
	, m_bForce(FALSE)
	, m_bNoFetch(FALSE)
	, m_bMerge(FALSE)
	, m_bRebase(FALSE)
	, m_bRemote(FALSE)
	, m_bWholeProject(FALSE)
{
	s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (s_bSortLogical)
		s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
}

CSubmoduleUpdateDlg::~CSubmoduleUpdateDlg()
{
}

void CSubmoduleUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATH, m_PathListBox);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Check(pDX, IDC_WHOLE_PROJECT, m_bWholeProject);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_INIT, m_bInit);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_RECURSIVE, m_bRecursive);
	DDX_Check(pDX, IDC_FORCE, m_bForce);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_NOFETCH, m_bNoFetch);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_MERGE, m_bMerge);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_REBASE, m_bRebase);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_REMOTE, m_bRemote);
}

BEGIN_MESSAGE_MAP(CSubmoduleUpdateDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_WHOLE_PROJECT, OnBnClickedShowWholeProject)
	ON_BN_CLICKED(IDOK, &CSubmoduleUpdateDlg::OnBnClickedOk)
	ON_LBN_SELCHANGE(IDC_LIST_PATH, &CSubmoduleUpdateDlg::OnLbnSelchangeListPath)
END_MESSAGE_MAP()

static int SubmoduleCallback(git_submodule *sm, const char * /*name*/, void *payload)
{
	auto list = *static_cast<STRING_VECTOR**>(payload);
	auto prefixList = *(static_cast<STRING_VECTOR**>(payload) + 1);
	CString path = CUnicodeUtils::GetUnicode(git_submodule_path(sm));
	if (prefixList->empty())
		list->push_back(path);
	else
	{
		for (size_t i = 0; i < prefixList->size(); ++i)
		{
			CString prefix = prefixList->at(i) + L'/';
			if (CStringUtils::StartsWith(path, prefix))
				list->push_back(path);
		}
	}
	return 0;
}

int LogicalComparePredicate(const CString &left, const CString &right)
{
	if (CSubmoduleUpdateDlg::s_bSortLogical)
		return StrCmpLogicalW(left, right) < 0;
	return StrCmpI(left, right) < 0;
}

static void GetSubmodulePathList(STRING_VECTOR &list, STRING_VECTOR &prefixList)
{
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		MessageBox(nullptr, CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	STRING_VECTOR *listParams[] = { &list, &prefixList };
	if (git_submodule_foreach(repo, SubmoduleCallback, &listParams))
	{
		MessageBox(nullptr, CGit::GetLibGit2LastErr(L"Could not get submodule list."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	std::sort(list.begin(), list.end(), LogicalComparePredicate);
}

BOOL CSubmoduleUpdateDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_CHECK_SUBMODULE_INIT);
	AdjustControlSize(IDC_CHECK_SUBMODULE_RECURSIVE);
	AdjustControlSize(IDC_CHECK_SUBMODULE_NOFETCH);
	AdjustControlSize(IDC_CHECK_SUBMODULE_MERGE);
	AdjustControlSize(IDC_CHECK_SUBMODULE_REBASE);

	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(static_cast<UINT>(IDC_STATIC), TOP_LEFT);
	AddAnchor(IDC_LIST_PATH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_WHOLE_PROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_GROUP_INFO, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECK_SUBMODULE_INIT, BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_SUBMODULE_RECURSIVE, BOTTOM_LEFT);
	AddAnchor(IDC_FORCE, BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_SUBMODULE_REMOTE, BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_SUBMODULE_NOFETCH, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECK_SUBMODULE_MERGE, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECK_SUBMODULE_REBASE, BOTTOM_RIGHT);

	CString str(g_Git.m_CurrentDir);
	str.Replace(L':', L'_');
	m_regShowWholeProject = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ShowWholeProject\\" + str, FALSE);
	m_bWholeProject = m_regShowWholeProject;

	m_regInit = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\init", TRUE);
	m_bInit = m_regInit;
	m_regRecursive = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\recursive", FALSE);
	m_bRecursive = m_regRecursive;
	m_regForce = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\force", FALSE);
	m_bForce = m_regForce;
	m_regRemote = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\remote", FALSE);
	m_bRemote = m_regRemote;
	m_regNoFetch = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\nofetch", FALSE);
	m_bNoFetch = m_regNoFetch;
	m_regMerge = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\merge", FALSE);
	m_bMerge = m_regMerge;
	m_regRebase = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SubmoduleUpdate\\" + str + L"\\rebase", FALSE);
	m_bRebase = m_regRebase;

	DialogEnableWindow(IDC_WHOLE_PROJECT, !(m_PathFilterList.empty() || (m_PathFilterList.size() == 1 && m_PathFilterList[0].IsEmpty())));

	SetDlgTitle();

	EnableSaveRestore(L"SubmoduleUpdateDlg");

	Refresh();
	UpdateData(FALSE);

	return TRUE;
}

void CSubmoduleUpdateDlg::SetDlgTitle()
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);
	CString dir = g_Git.m_CurrentDir;
	if (!m_bWholeProject)
	{
		if (!m_PathFilterList.empty())
			dir += (CStringUtils::EndsWith(g_Git.m_CurrentDir, L'\\') ? L"" : L"\\") + CTGitPath(m_PathFilterList[0]).GetWinPathString();
		if (m_PathFilterList.size() > 1)
			dir += L", ...";
	}
	CAppUtils::SetWindowTitle(m_hWnd, dir, m_sTitle);
}

void CSubmoduleUpdateDlg::OnBnClickedOk()
{
	CResizableStandAloneDialog::UpdateData(TRUE);
	m_PathList.clear();

	CString selected;
	for (int i = 0; i < m_PathListBox.GetCount(); ++i)
	{
		if (m_PathListBox.GetSel(i))
		{
			if (!selected.IsEmpty())
				selected.AppendChar(L'|');
			CString text;
			m_PathListBox.GetText(i, text);
			m_PathList.push_back(text);
			selected.Append(text);
		}
	}
	m_regPath = selected;

	m_regInit = m_bInit;
	m_regRecursive = m_bRecursive;
	m_regForce = m_bForce;
	m_regRemote = m_bRemote;
	m_regNoFetch = m_bNoFetch;
	m_regMerge = m_bMerge;
	m_regRebase = m_bRebase;

	CResizableStandAloneDialog::OnOK();
}

void CSubmoduleUpdateDlg::OnLbnSelchangeListPath()
{
	GetDlgItem(IDOK)->EnableWindow(m_PathListBox.GetSelCount() > 0 ? TRUE : FALSE);
	if (m_PathListBox.GetSelCount() == 0)
		m_SelectAll.SetCheck(BST_UNCHECKED);
	else if (m_PathListBox.GetSelCount() < m_PathListBox.GetCount())
		m_SelectAll.SetCheck(BST_INDETERMINATE);
	else
		m_SelectAll.SetCheck(BST_CHECKED);
}

void CSubmoduleUpdateDlg::OnBnClickedSelectall()
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
		for (int i = 0; i < m_PathListBox.GetCount(); ++i)
			m_PathListBox.SetSel(i, FALSE);
	}
	else
	{
		for (int i = 0; i < m_PathListBox.GetCount(); ++i)
			m_PathListBox.SetSel(i, TRUE);
	}
	OnLbnSelchangeListPath();
}

void CSubmoduleUpdateDlg::OnBnClickedShowWholeProject()
{
	UpdateData();
	m_regShowWholeProject = m_bWholeProject;
	SetDlgTitle();
	Refresh();
}

void CSubmoduleUpdateDlg::Refresh()
{
	while (m_PathListBox.GetCount() > 0)
		m_PathListBox.DeleteString(m_PathListBox.GetCount() - 1);

	CString WorkingDir = g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');

	m_regPath = CRegString(L"Software\\TortoiseGit\\History\\SubmoduleUpdatePath\\" + WorkingDir);
	CString path = m_regPath;
	STRING_VECTOR emptylist;
	STRING_VECTOR list;
	GetSubmodulePathList(list, m_bWholeProject ? emptylist : m_PathFilterList);
	STRING_VECTOR selected;
	if (m_PathList.empty())
	{
		int pos = 0;
		while (pos >= 0)
		{
			CString part = path.Tokenize(L"|", pos);
			if (!part.IsEmpty())
				selected.push_back(part);
		}
	}
	else
	{
		for (size_t i = 0; i < m_PathList.size(); ++i)
			selected.push_back(m_PathList[i]);
	}

	for (size_t i = 0; i < list.size(); ++i)
	{
		m_PathListBox.AddString(list[i]);
		if (selected.size() == 0)
			m_PathListBox.SetSel(static_cast<int>(i));
		else
		{
			for (size_t j = 0; j < selected.size(); ++j)
			{
				if (selected[j] == list[i])
					m_PathListBox.SetSel(static_cast<int>(i));
			}
		}
	}

	OnLbnSelchangeListPath();
}
