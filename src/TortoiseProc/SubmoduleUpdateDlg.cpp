// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 Sven Strickroth, <email@cs-ware.de>

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

IMPLEMENT_DYNAMIC(CSubmoduleUpdateDlg, CStandAloneDialog)

CSubmoduleUpdateDlg::CSubmoduleUpdateDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CSubmoduleUpdateDlg::IDD, pParent)
	, m_bInit(true)
	, m_bRecursive(FALSE)
	, m_bForce(FALSE)
	, m_bNoFetch(FALSE)
	, m_bMerge(FALSE)
	, m_bRebase(FALSE)
{
}

CSubmoduleUpdateDlg::~CSubmoduleUpdateDlg()
{
}

void CSubmoduleUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATH, m_PathListBox);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_INIT, m_bInit);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_RECURSIVE, m_bRecursive);
	DDX_Check(pDX, IDC_FORCE, m_bForce);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_NOFETCH, m_bNoFetch);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_MERGE, m_bMerge);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_REBASE, m_bRebase);
}


BEGIN_MESSAGE_MAP(CSubmoduleUpdateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CSubmoduleUpdateDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDHELP, &CSubmoduleUpdateDlg::OnBnClickedHelp)
	ON_LBN_SELCHANGE(IDC_LIST_PATH, &CSubmoduleUpdateDlg::OnLbnSelchangeListPath)
END_MESSAGE_MAP()

static int SubmoduleCallback(git_submodule *sm, const char * /*name*/, void *payload)
{
	STRING_VECTOR *list = *(STRING_VECTOR **)payload;
	STRING_VECTOR *prefixList = *((STRING_VECTOR **)payload + 1);
	CString path = CUnicodeUtils::GetUnicode(git_submodule_path(sm));
	if (prefixList->empty())
	{
		list->push_back(path);
	}
	else
	{
		for (size_t i = 0; i < prefixList->size(); ++i)
		{
			CString prefix = prefixList->at(i) + _T("/");
			if (path.Left(prefix.GetLength()) == prefix)
				list->push_back(path);
		}
	}
	return 0;
}

static void GetSubmodulePathList(STRING_VECTOR &list, STRING_VECTOR &prefixList)
{
	git_repository *repo;
	if (git_repository_open(&repo, CUnicodeUtils::GetUTF8(g_Git.m_CurrentDir)))
	{
		MessageBox(NULL, CGit::GetLibGit2LastErr(_T("Could not open repository.")), _T("TortoiseGit"), MB_ICONERROR);
		return;
	}

	STRING_VECTOR *listParams[] = { &list, &prefixList };
	if (git_submodule_foreach(repo, SubmoduleCallback, &listParams))
	{
		MessageBox(NULL, CGit::GetLibGit2LastErr(_T("Could not get submodule list.")), _T("TortoiseGit"), MB_ICONERROR);
		return;
	}

	git_repository_free(repo);
	std::sort(list.begin(), list.end());
}

BOOL CSubmoduleUpdateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AdjustControlSize(IDC_CHECK_SUBMODULE_INIT);
	AdjustControlSize(IDC_CHECK_SUBMODULE_RECURSIVE);
	AdjustControlSize(IDC_CHECK_SUBMODULE_NOFETCH);
	AdjustControlSize(IDC_CHECK_SUBMODULE_MERGE);
	AdjustControlSize(IDC_CHECK_SUBMODULE_REBASE);

	CString WorkingDir = g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'), _T('_'));

	m_regPath = CRegString(CString(_T("Software\\TortoiseGit\\History\\SubmoduleUpdatePath\\") + WorkingDir));
	CString path = m_regPath;
	STRING_VECTOR list;
	GetSubmodulePathList(list, m_PathFilterList);
	STRING_VECTOR selected;
	int pos = 0;
	while (pos >= 0)
	{
		CString part = path.Tokenize(_T("|"), pos);
		if (!part.IsEmpty())
			selected.push_back(part);
	}
	for (size_t i = 0; i < list.size(); ++i)
	{
		m_PathListBox.AddString(list[i]);
		if (selected.size() == 0)
			m_PathListBox.SetSel((int)i);
		else
		{
			for (int j = 0; j < selected.size(); ++j)
			{
				if (selected[j] == list[i])
					m_PathListBox.SetSel((int)i);
			}
		}
	}

	UpdateData(FALSE);

	return TRUE;
}

void CSubmoduleUpdateDlg::OnBnClickedOk()
{
	CStandAloneDialog::UpdateData(TRUE);
	m_PathList.clear();
	CString selected;
	for (int i = 0; i < m_PathListBox.GetCount(); ++i)
	{
		if (m_PathListBox.GetSel(i))
		{
			if (!selected.IsEmpty())
				selected.Append(_T("|"));
			CString text;
			m_PathListBox.GetText(i, text);
			m_PathList.push_back(text);
			selected.Append(text);
		}
	}
	m_regPath = selected;

	CStandAloneDialog::OnOK();
}

void CSubmoduleUpdateDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CSubmoduleUpdateDlg::OnLbnSelchangeListPath()
{
	GetDlgItem(IDOK)->EnableWindow(m_PathListBox.GetSelCount() > 0 ? TRUE : FALSE);
}
