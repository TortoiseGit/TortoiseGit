// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014-2019 TortoiseGit

// with code of PullFetchDlg.cpp

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
#include "BisectStartDlg.h"
#include "Git.h"
#include "LogDlg.h"
#include "AppUtils.h"
#include "StringUtils.h"

IMPLEMENT_DYNAMIC(CBisectStartDlg, CHorizontalResizableStandAloneDialog)

CBisectStartDlg::CBisectStartDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CBisectStartDlg::IDD, pParent)
{
}

CBisectStartDlg::~CBisectStartDlg()
{
}

void CBisectStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_GOOD, m_cLastGoodRevision);
	DDX_Control(pDX, IDC_COMBOBOXEX_BAD, m_cFirstBadRevision);
}

BEGIN_MESSAGE_MAP(CBisectStartDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CBisectStartDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_GOOD, &CBisectStartDlg::OnBnClickedButtonGood)
	ON_BN_CLICKED(IDC_BUTTON_BAD, &CBisectStartDlg::OnBnClickedButtonBad)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_GOOD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BAD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_GOOD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_BAD, &CBisectStartDlg::OnChangedRevision)
END_MESSAGE_MAP()

static void uniqueMergeLists(STRING_VECTOR& list, const STRING_VECTOR& listToMerge)
{
	std::map<CString, int> map;
	for (CString entry : list)
		map[entry] = 1;

	for (CString entry : listToMerge)
	{
		if (map.find(entry) == map.end())
			list.push_back(entry);
	}
}

BOOL CBisectStartDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddAnchor(IDC_STATIC_LOCAL_BRANCH, TOP_LEFT);
	AddAnchor(IDC_STATIC_REMOTE_BRANCH, TOP_LEFT);

	AddAnchor(IDC_BUTTON_GOOD, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BAD, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_GOOD, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_BAD, TOP_LEFT, TOP_RIGHT);

	EnableSaveRestore(L"BisectStartDlg");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	STRING_VECTOR list;
	int current = -1;
	g_Git.GetBranchList(list, &current, CGit::BRANCH_ALL);
	m_cLastGoodRevision.SetMaxHistoryItems(0x7FFFFFFF);
	m_cFirstBadRevision.SetMaxHistoryItems(0x7FFFFFFF);
	STRING_VECTOR tagsList;
	g_Git.GetTagList(tagsList);
	uniqueMergeLists(list, tagsList);
	m_cLastGoodRevision.SetList(list);
	m_cFirstBadRevision.SetList(list);
	if (m_sLastGood.IsEmpty())
		m_cLastGoodRevision.SetCurSel(-1);
	else
		m_cLastGoodRevision.SetWindowTextW(m_sLastGood);
	if (!m_sFirstBad.IsEmpty())
		m_cFirstBadRevision.SetWindowTextW(m_sFirstBad);
	else if (current >= 0)
		m_cFirstBadRevision.SetCurSel(current);
	else
		m_cFirstBadRevision.SetWindowTextW(L"HEAD");
	this->UpdateData(FALSE);

	// EnDisable OK Button
	OnChangedRevision();

	return TRUE;
}

void CBisectStartDlg::OnChangedRevision()
{
	this->GetDlgItem(IDOK)->EnableWindow(!(m_cLastGoodRevision.GetString().Trim().IsEmpty() || m_cFirstBadRevision.GetString().Trim().IsEmpty()));
}

void CBisectStartDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	m_LastGoodRevision = m_cLastGoodRevision.GetString().Trim();
	m_FirstBadRevision = m_cFirstBadRevision.GetString().Trim();

	if (CStringUtils::StartsWith(m_FirstBadRevision, L"remotes/"))
		m_FirstBadRevision = m_FirstBadRevision.Mid(static_cast<int>(wcslen(L"remotes/")));

	if (CStringUtils::StartsWith(m_FirstBadRevision, L"remotes/"))
		m_FirstBadRevision = m_FirstBadRevision.Mid(static_cast<int>(wcslen(L"remotes/")));

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CBisectStartDlg::OnBnClickedButtonGood()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	CString revision;
	m_cLastGoodRevision.GetWindowText(revision);
	dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	dlg.ShowWorkingTreeChanges(false);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
	{
		m_cLastGoodRevision.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
		OnChangedRevision();
	}
}

void CBisectStartDlg::OnBnClickedButtonBad()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	CString revision;
	m_cFirstBadRevision.GetWindowText(revision);
	dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	dlg.ShowWorkingTreeChanges(false);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
	{
		m_cFirstBadRevision.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
		OnChangedRevision();
	}
}
