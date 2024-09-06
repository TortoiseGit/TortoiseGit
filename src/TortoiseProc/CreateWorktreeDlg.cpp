// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022, 2024 - TortoiseGit

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
// CreateWorktreeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Git.h"
#include "TortoiseProc.h"
#include "CreateWorktreeDlg.h"
#include "AppUtils.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AutoCloakWindow.h"

// CCreateWorktreeDlg dialog

IMPLEMENT_DYNAMIC(CCreateWorktreeDlg, CResizableStandAloneDialog)

CCreateWorktreeDlg::CCreateWorktreeDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CCreateWorktreeDlg::IDD, pParent)
	, CChooseVersion(this)
{
}

CCreateWorktreeDlg::~CCreateWorktreeDlg()
{
}

void CCreateWorktreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CHOOSE_VERSION_DDX;

	DDX_Text(pDX, IDC_WORKTREE_DIR, m_sWorktreePath);
	DDX_Check(pDX, IDC_CHECK_CHECKOUT, m_bCheckout);
	DDX_Check(pDX, IDC_CHECK_FORCE, m_bForce);
	DDX_Check(pDX, IDC_CHECK_DETACH, m_bDetach);
	DDX_Check(pDX, IDC_CHECK_BRANCH, m_bBranch);
	DDX_Text(pDX, IDC_EDIT_BRANCH, m_sNewBranch);
}

BEGIN_MESSAGE_MAP(CCreateWorktreeDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CCreateWorktreeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_DIR, &CCreateWorktreeDlg::OnBnClickedWorkdirDirBrowse)
	ON_BN_CLICKED(IDC_CHECK_BRANCH, OnBnClickedCheckBranch)
	ON_BN_CLICKED(IDC_CHECK_DETACH, OnBnClickedCheckDetach)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, OnCbnEditchangeComboboxexVersion)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_TAGS, OnCbnEditchangeComboboxexVersion)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_VERSION, OnCbnEditchangeComboboxexVersion)
	ON_EN_CHANGE(IDC_WORKTREE_DIR, OnCbnEditchangeComboboxexVersion)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CCreateWorktreeDlg::OnInitDialog()
{
	CAutoCloakWindow window_cloaker{ GetSafeHwnd() };
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CAppUtils::SetWindowTitle(*this, g_Git.m_CurrentDir);

	SetDefaultChoose(IDC_RADIO_HEAD);

	InitChooseVersion();

	auto pHead = GetDlgItem(IDC_RADIO_HEAD);
	CString HeadText;
	pHead->GetWindowText(HeadText);
	pHead->SetWindowText(HeadText + " (" + g_Git.GetCurrentBranch() + ")");

	AdjustControlSize(IDC_RADIO_HEAD);
	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);
	AdjustControlSize(IDC_CHECK_CHECKOUT);
	AdjustControlSize(IDC_CHECK_FORCE);
	AdjustControlSize(IDC_CHECK_DETACH);
	AdjustControlSize(IDC_CHECK_BRANCH);

	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_BUTTON_DIR, TOP_RIGHT);
	AddAnchor(IDC_WORKTREE_DIR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDIT_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();

	EnableSaveRestore(L"CreateWorktreeDlg");

	// Add tooltips
	m_tooltips.AddTool(GetDlgItem(IDC_CHECK_FORCE), CString(MAKEINTRESOURCE(IDS_PROC_NEWBRANCHTAG_FORCE_TT)));
	m_tooltips.Activate(TRUE);

	if (m_sWorktreePath.IsEmpty())
	{
		if (CStringUtils::EndsWith(g_Git.m_CurrentDir, L".git"))
			m_sWorktreePath = g_Git.m_CurrentDir.Left(g_Git.m_CurrentDir.GetLength() - static_cast<int>(wcslen(L".git")));
		else
			m_sWorktreePath = g_Git.m_CurrentDir + L"-worktree";
	}

	SHAutoComplete(GetDlgItem(IDC_WORKTREE_DIR)->m_hWnd, SHACF_DEFAULT);

	UpdateData(FALSE);

	SetTheme(CTheme::Instance().IsDarkTheme());
	return TRUE;
}

// CCreateWorktreeDlg message handlers

void CCreateWorktreeDlg::OnBnClickedOk()
{
	UpdateData(TRUE);

	if (m_sWorktreePath.IsEmpty())
	{
		ShowEditBalloon(IDC_WORKTREE_DIR, IDS_ERR_MISSINGVALUE, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	if (m_bBranch && !CGit::IsBranchNameValid(m_sNewBranch))
	{
		ShowEditBalloon(IDC_EDIT_BRANCH, IDS_B_T_NOTEMPTY, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	CTGitPath path(m_sWorktreePath);
	if (!path.IsValidOnWindows())
	{
		ShowEditBalloon(IDC_WORKTREE_DIR, IDS_WARN_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	UpdateRevsionName();

	OnOK();
}

void CCreateWorktreeDlg::OnVersionChanged()
{
	UpdateData(TRUE);
	UpdateRevsionName();

	const int radio = GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION);
	CString version = m_VersionName;

	int start = -1;
	if (CStringUtils::StartsWith(version, L"origin/"))
		start = version.Find(L'/', static_cast<int>(wcslen(L"origin/")) + 1);
	else if (CStringUtils::StartsWith(version, L"remotes/"))
		start = version.Find(L'/', static_cast<int>(wcslen(L"remotes/")) + 1);

	if (radio == IDC_RADIO_HEAD)
	{
		if (CString name = CTGitPath(m_sWorktreePath).GetUIFileOrDirectoryName(); !name.IsEmpty())
			m_sNewBranch = name;
		m_bBranch = FALSE;
	}
	else if (start >= 0)
	{
		version = version.Mid(start + 1);
		m_sNewBranch = version;

		m_bBranch = TRUE;
	}
	else
	{
		if (CStringUtils::StartsWith(m_VersionName, L"refs/heads/"))
			version = m_VersionName.Mid(static_cast<int>(wcslen(L"refs/heads/")));
		else if (radio == IDC_RADIO_VERSION)
			version = version.Left(g_Git.GetShortHASHLength());
		m_sNewBranch = L"Branch_" + version;

		m_bBranch = FALSE;
	}
	if (radio == IDC_RADIO_VERSION || radio == IDC_RADIO_TAGS)
		m_bBranch = TRUE;

	UpdateData(FALSE);

	OnBnClickedCheckBranch();
}

void CCreateWorktreeDlg::OnBnClickedWorkdirDirBrowse()
{
	UpdateData(TRUE);
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strDirectory = m_sWorktreePath;
	if (browseFolder.Show(GetSafeHwnd(), strDirectory) == CBrowseFolder::OK)
	{
		m_sWorktreePath = strDirectory;
		UpdateData(FALSE);
	}
}

void CCreateWorktreeDlg::OnBnClickedCheckBranch()
{
	UpdateData();
	GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(m_bBranch);

	const int radio = GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION);
	if (!m_bBranch && (radio == IDC_RADIO_VERSION || radio == IDC_RADIO_TAGS || radio == IDC_RADIO_BRANCH && CStringUtils::StartsWith(m_VersionName, L"remotes/")))
		m_bDetach = TRUE;
	else
		m_bDetach = FALSE;
	GetDlgItem(IDC_CHECK_DETACH)->EnableWindow(!m_bDetach);
	UpdateData(FALSE);
}

void CCreateWorktreeDlg::OnBnClickedCheckDetach()
{
	UpdateData();

	if (!m_bDetach)
		return;

	m_bBranch = FALSE;
	UpdateData(FALSE);
	GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(m_bBranch);
	const int radio = GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION);
	if (radio == IDC_RADIO_VERSION || radio == IDC_RADIO_TAGS || radio == IDC_RADIO_BRANCH && CStringUtils::StartsWith(m_VersionName, L"remotes/"))
		GetDlgItem(IDC_CHECK_DETACH)->EnableWindow(FALSE);
}

void CCreateWorktreeDlg::OnCbnEditchangeComboboxexVersion()
{
	OnVersionChanged();
}

void CCreateWorktreeDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}
