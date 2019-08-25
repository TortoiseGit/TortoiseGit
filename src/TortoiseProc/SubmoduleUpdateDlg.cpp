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

struct SubmoduleItem
{
	CString path;
	git_submodule_status_t status;
	CString rev;

	CString GetStatusText()
	{
		if (status & GIT_SUBMODULE_STATUS_WD_UNINITIALIZED)
			return L"Empty";
		if (status & GIT_SUBMODULE_STATUS_WD_ADDED)
			return L"Not in index";
		if (status & GIT_SUBMODULE_STATUS_WD_DELETED)
			return L"Not in workdir";
		if (status & GIT_SUBMODULE_STATUS_INDEX_ADDED)
			return L"Not in head";
		if (status & GIT_SUBMODULE_STATUS_INDEX_DELETED)
			return L"Not in index";
		if (status & GIT_SUBMODULE_STATUS_INDEX_MODIFIED)
			return L"Index != HEAD";
		if (!(status & GIT_SUBMODULE_STATUS_IN_CONFIG))
			return L"Not in .gitmodules";
		if (!(status & GIT_SUBMODULE_STATUS_IN_INDEX))
			return L"Not in index";
		if (!(status & GIT_SUBMODULE_STATUS_IN_HEAD))
			return L"Not in head";
		if (!(status & GIT_SUBMODULE_STATUS_IN_WD))
			return L"Not in workdir";
		if (status & GIT_SUBMODULE_STATUS_WD_MODIFIED)
			return L"Not Match";
		if ((status & GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED)
			|| (status & GIT_SUBMODULE_STATUS_WD_WD_MODIFIED)
			|| (status & GIT_SUBMODULE_STATUS_WD_UNTRACKED))
			return L"Dirty";
		return L"Normal";
	}
};

struct SubmodulePayload
{
	std::vector<SubmoduleItem>* list;
	STRING_VECTOR* prefixList;
	git_repository* repo;
};

static int SubmoduleCallback(git_submodule* sm, const char* name, void* payload)
{
	auto payload1 = static_cast<SubmodulePayload*>(payload);
	auto list = payload1->list;
	auto prefixList = *(static_cast<STRING_VECTOR**>(payload) + 1);
	unsigned int status = 0;
	git_submodule_status(&status, payload1->repo, name, GIT_SUBMODULE_IGNORE_UNSPECIFIED);
	CAutoRepository smRepo;
	git_repository_open(smRepo.GetPointer(), git_submodule_path(sm));
	auto headId = git_submodule_head_id(sm);
	CString rev = headId ? CGitHash(headId).ToString() : L"";
	if (smRepo)
	{
		CAutoReference headRef;
		git_repository_head(headRef.GetPointer(), smRepo);
		CAutoObject commit;
		git_object_lookup(commit.GetPointer(), smRepo, git_reference_target(headRef), GIT_OBJECT_COMMIT);
		CAutoDescribeResult describe;
		git_describe_options describe_options = GIT_DESCRIBE_OPTIONS_INIT;
		git_describe_commit(describe.GetPointer(), commit, &describe_options);
		CAutoBuf describe_buf;
		git_describe_format_options format_options = GIT_DESCRIBE_FORMAT_OPTIONS_INIT;
		format_options.always_use_long_format = 1;
		if (describe && !git_describe_format(describe_buf, describe, &format_options))
			rev = CUnicodeUtils::GetUnicode(describe_buf->ptr);
	}
	CString path = CUnicodeUtils::GetUnicode(git_submodule_path(sm));
	SubmoduleItem item = { path, (git_submodule_status_t)status, rev };
	if (prefixList->empty())
		list->push_back(item);
	else
	{
		for (size_t i = 0; i < prefixList->size(); ++i)
		{
			CString prefix = prefixList->at(i) + L'/';
			if (CStringUtils::StartsWith(path, prefix))
				list->push_back(item);
		}
	}
	return 0;
}

static int LogicalComparePredicate(const SubmoduleItem& left, const SubmoduleItem& right)
{
	if (CSubmoduleUpdateDlg::s_bSortLogical)
		return StrCmpLogicalW(left.path, right.path) < 0;
	return StrCmpI(left.path, right.path) < 0;
}

static void GetSubmodulePathList(std::vector<SubmoduleItem>& list, STRING_VECTOR& prefixList)
{
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		MessageBox(nullptr, CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	SubmodulePayload payload = { &list, &prefixList, repo };
	if (git_submodule_foreach(repo, SubmoduleCallback, &payload))
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

	static UINT columnNames[] = { IDS_STATUSLIST_COLFILE, IDS_STATUSLIST_COLSTATUS, IDS_STATUSLIST_COLREVISION };
	static int columnWidths[] = { 150, 100, 150 };
	DWORD dwDefaultColumns = (1 << eCol_Path) | (1 << eCol_Status) | (1 << eCol_Version);
	m_PathListBox.SetExtendedStyle(m_PathListBox.GetExtendedStyle() | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	m_PathListBox.m_bAllowHiding = false;
	m_PathListBox.Init();
	m_PathListBox.m_ColumnManager.SetNames(columnNames, _countof(columnNames));
	m_PathListBox.m_ColumnManager.ReadSettings(dwDefaultColumns, 0, L"SubmoduleUpdate", _countof(columnNames), columnWidths);
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
	auto pos = m_PathListBox.GetFirstSelectedItemPosition();
	while (pos)
	{
		auto index = m_PathListBox.GetNextSelectedItem(pos);
		{
			if (!selected.IsEmpty())
				selected.AppendChar(L'|');
			auto text = m_PathListBox.GetItemText(index, 0);
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
	GetDlgItem(IDOK)->EnableWindow(m_PathListBox.GetSelectedCount() > 0 ? TRUE : FALSE);
	if (m_PathListBox.GetSelectedCount() == 0)
		m_SelectAll.SetCheck(BST_UNCHECKED);
	else if ((int)m_PathListBox.GetSelectedCount() < m_PathListBox.GetItemCount())
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
		for (int i = 0; i < m_PathListBox.GetItemCount(); ++i)
		{
			m_PathListBox.SetItemState(i, 0, LVIS_SELECTED);
			m_PathListBox.EnsureVisible(i, FALSE);
		}
	}
	else
	{
		for (int i = 0; i < m_PathListBox.GetItemCount(); ++i)
		{
			m_PathListBox.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_PathListBox.EnsureVisible(i, FALSE);
		}
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
	m_PathListBox.DeleteAllItems();

	CString WorkingDir = g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');

	m_regPath = CRegString(L"Software\\TortoiseGit\\History\\SubmoduleUpdatePath\\" + WorkingDir);
	CString path = m_regPath;
	STRING_VECTOR emptylist;
	std::vector<SubmoduleItem> list;
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

	for (int i = 0; i < (int)list.size(); ++i)
	{
		int indexItem = m_PathListBox.InsertItem(m_PathListBox.GetItemCount(), list[i].path);
		m_PathListBox.SetItemText(indexItem, eCol_Path, list[i].path);
		m_PathListBox.SetItemText(indexItem, eCol_Status, list[i].GetStatusText());
		m_PathListBox.SetItemText(indexItem, eCol_Version, list[i].rev);
		if (selected.size() == 0)
		{
			m_PathListBox.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_PathListBox.EnsureVisible(i, FALSE);
		}
		else
		{
			for (size_t j = 0; j < selected.size(); ++j)
			{
				if (selected[j] == list[i].path)
				{
					m_PathListBox.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
					m_PathListBox.EnsureVisible(i, FALSE);
				}
			}
		}
	}

	OnLbnSelchangeListPath();
}
