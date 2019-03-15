// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2019 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "SetMainPage.h"
#include "AppUtils.h"
#include "ProgressDlg.h"
#include "SetDialogs2.h"

IMPLEMENT_DYNAMIC(CSetDialogs2, ISettingsPropPage)
CSetDialogs2::CSetDialogs2()
	: ISettingsPropPage(CSetDialogs2::IDD)
	, m_dwAutoCloseGitProgress(AUTOCLOSE_NO)
	, m_bUseRecycleBin(TRUE)
	, m_bConfirmKillProcess(FALSE)
	, m_bSyncDialogRandomPos(FALSE)
	, m_bRefCompareHideUnchanged(FALSE)
	, m_bSortTagsReversed(FALSE)
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_dwMaxHistory(25)
	, m_bAutoSelect(TRUE)
	, m_bShowGitexeTimings(TRUE)
	, m_bNoSounds(FALSE)
	, m_bBranchesIncludeFetchHead(TRUE)
	, m_bNoAutoselectMissing(FALSE)
{
	m_regAutoCloseGitProgress = CRegDWORD(L"Software\\TortoiseGit\\AutoCloseGitProgress");
	m_regUseRecycleBin = CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE);
	m_regConfirmKillProcess = CRegDWORD(L"Software\\TortoiseGit\\ConfirmKillProcess", FALSE);
	m_bConfirmKillProcess = m_regConfirmKillProcess;
	m_regSyncDialogRandomPos = CRegDWORD(L"Software\\TortoiseGit\\SyncDialogRandomPos", FALSE);
	m_bSyncDialogRandomPos = m_regSyncDialogRandomPos;
	m_regRefCompareHideUnchanged = CRegDWORD(L"Software\\TortoiseGit\\RefCompareHideUnchanged", FALSE);
	m_bRefCompareHideUnchanged = m_regRefCompareHideUnchanged;
	m_regSortTagsReversed = CRegDWORD(L"Software\\TortoiseGit\\SortTagsReversed", FALSE);
	m_bSortTagsReversed = m_regSortTagsReversed;
	m_regAutocompletion = CRegDWORD(L"Software\\TortoiseGit\\Autocompletion", TRUE);
	m_bAutocompletion = m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(L"Software\\TortoiseGit\\AutocompleteParseTimeout", 5);
	m_dwAutocompletionTimeout = m_regAutocompletionTimeout;
	m_regMaxHistory = CRegDWORD(L"Software\\TortoiseGit\\MaxHistoryItems", 25);
	m_dwMaxHistory = m_regMaxHistory;
	m_regAutoSelect = CRegDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE);
	m_bAutoSelect = m_regAutoSelect;
	m_regStripCommentedLines = CRegDWORD(L"Software\\TortoiseGit\\StripCommentedLines", FALSE);
	m_bStripCommentedLines = m_regStripCommentedLines;
	m_regShowGitexeTimings = CRegDWORD(L"Software\\TortoiseGit\\ShowGitexeTimings", TRUE);
	m_bShowGitexeTimings = m_regShowGitexeTimings;
	m_regNoSounds = CRegDWORD(L"Software\\TortoiseGit\\NoSounds", FALSE);
	m_bNoSounds = m_regNoSounds;
	m_regBranchesIncludeFetchHead = CRegDWORD(L"Software\\TortoiseGit\\BranchesIncludeFetchHead", TRUE);
	m_bBranchesIncludeFetchHead = m_regBranchesIncludeFetchHead;
	m_regNoAutoselectMissing = CRegDWORD(L"Software\\TortoiseGit\\AutoselectMissingFiles", FALSE);
	m_bNoAutoselectMissing = m_regNoAutoselectMissing;
}

CSetDialogs2::~CSetDialogs2()
{
}

void CSetDialogs2::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AUTOCLOSECOMBO, m_cAutoCloseGitProgress);
	DDX_Check(pDX, IDC_USERECYCLEBIN, m_bUseRecycleBin);
	DDX_Check(pDX, IDC_CONFIRMKILLPROCESS, m_bConfirmKillProcess);
	DDX_Check(pDX, IDC_SYNCDIALOGRANDOMPOS, m_bSyncDialogRandomPos);
	DDX_Check(pDX, IDC_REFCOMPAREHIDEUNCHANGED, m_bRefCompareHideUnchanged);
	DDX_Check(pDX, IDC_SORTTAGSREVERSED, m_bSortTagsReversed);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Text(pDX, IDC_MAXHISTORY, m_dwMaxHistory);
	DDV_MinMaxUInt(pDX, m_dwMaxHistory, 1, 100);
	DDX_Check(pDX, IDC_SELECTFILESONCOMMIT, m_bAutoSelect);
	DDX_Check(pDX, IDC_STRIPCOMMENTEDLINES, m_bStripCommentedLines);
	DDX_Check(pDX, IDC_PROGRESSDLG_SHOW_TIMES, m_bShowGitexeTimings);
	DDX_Check(pDX, IDC_NOSOUNDS, m_bNoSounds);
	DDX_Check(pDX, IDC_BRANCHESINCLUDEFETCHHEAD, m_bBranchesIncludeFetchHead);
	DDX_Check(pDX, IDC_NOAUTOSELECTMISSING, m_bNoAutoselectMissing);
}

BEGIN_MESSAGE_MAP(CSetDialogs2, ISettingsPropPage)
	ON_CBN_SELCHANGE(IDC_AUTOCLOSECOMBO, OnCbnSelchangeAutoclosecombo)
	ON_BN_CLICKED(IDC_USERECYCLEBIN, OnChange)
	ON_BN_CLICKED(IDC_CONFIRMKILLPROCESS, OnChange)
	ON_BN_CLICKED(IDC_SYNCDIALOGRANDOMPOS, OnChange)
	ON_BN_CLICKED(IDC_REFCOMPAREHIDEUNCHANGED, OnChange)
	ON_BN_CLICKED(IDC_SORTTAGSREVERSED, OnChange)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, OnChange)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, OnChange)
	ON_EN_CHANGE(IDC_MAXHISTORY, OnChange)
	ON_BN_CLICKED(IDC_SELECTFILESONCOMMIT, OnChange)
	ON_BN_CLICKED(IDC_STRIPCOMMENTEDLINES, OnChange)
	ON_BN_CLICKED(IDC_PROGRESSDLG_SHOW_TIMES, OnChange)
	ON_BN_CLICKED(IDC_NOSOUNDS, OnChange)
	ON_BN_CLICKED(IDC_BRANCHESINCLUDEFETCHHEAD, OnChange)
	ON_BN_CLICKED(IDC_NOAUTOSELECTMISSING, OnChange)
END_MESSAGE_MAP()

// CSetDialogs2 message handlers
BOOL CSetDialogs2::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_USERECYCLEBIN);
	AdjustControlSize(IDC_CONFIRMKILLPROCESS);
	AdjustControlSize(IDC_SYNCDIALOGRANDOMPOS);
	AdjustControlSize(IDC_REFCOMPAREHIDEUNCHANGED);
	AdjustControlSize(IDC_SORTTAGSREVERSED);
	AdjustControlSize(IDC_AUTOCOMPLETION);
	AdjustControlSize(IDC_SELECTFILESONCOMMIT);
	AdjustControlSize(IDC_STRIPCOMMENTEDLINES);
	AdjustControlSize(IDC_PROGRESSDLG_SHOW_TIMES);
	AdjustControlSize(IDC_NOSOUNDS);
	AdjustControlSize(IDC_BRANCHESINCLUDEFETCHHEAD);
	AdjustControlSize(IDC_NOAUTOSELECTMISSING);

	EnableToolTips();

	int ind = m_cAutoCloseGitProgress.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_MANUAL)));
	m_cAutoCloseGitProgress.SetItemData(ind, AUTOCLOSE_NO);
	ind = m_cAutoCloseGitProgress.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOPTIONS)));
	m_cAutoCloseGitProgress.SetItemData(ind, AUTOCLOSE_IF_NO_OPTIONS);
	ind = m_cAutoCloseGitProgress.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOERROR)));
	m_cAutoCloseGitProgress.SetItemData(ind, AUTOCLOSE_IF_NO_ERRORS);

	m_dwAutoCloseGitProgress = m_regAutoCloseGitProgress;
	m_bUseRecycleBin = m_regUseRecycleBin;

	for (int i = 0; i < m_cAutoCloseGitProgress.GetCount(); ++i)
		if (m_cAutoCloseGitProgress.GetItemData(i) == m_dwAutoCloseGitProgress)
			m_cAutoCloseGitProgress.SetCurSel(i);

	CString temp;

	m_tooltips.AddTool(IDC_AUTOCLOSECOMBO, IDS_SETTINGS_AUTOCLOSE_TT);
	m_tooltips.AddTool(IDC_USERECYCLEBIN, IDS_SETTINGS_USERECYCLEBIN_TT);
	m_tooltips.AddTool(IDC_CONFIRMKILLPROCESS, IDS_SETTINGS_CONFIRMKILLPROCESS_TT);
	m_tooltips.AddTool(IDC_SYNCDIALOGRANDOMPOS, IDS_SYNCDIALOGRANDOMPOS_TT);
	m_tooltips.AddTool(IDC_REFCOMPAREHIDEUNCHANGED, IDS_REFCOMPAREHIDEUNCHANGED_TT);
	m_tooltips.AddTool(IDC_SORTTAGSREVERSED, IDS_SORTTAGSREVERSED_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETION, IDS_SETTINGS_AUTOCOMPLETION_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_MAXHISTORY, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_MAXHISTORYLABEL, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_SELECTFILESONCOMMIT, IDS_SETTINGS_SELECTFILESONCOMMIT_TT);

	UpdateData(FALSE);
	return TRUE;
}

void CSetDialogs2::OnChange()
{
	SetModified();
}

BOOL CSetDialogs2::OnApply()
{
	UpdateData();

	Store(static_cast<DWORD>(m_dwAutoCloseGitProgress), m_regAutoCloseGitProgress);
	Store(m_bUseRecycleBin, m_regUseRecycleBin);
	Store(m_bConfirmKillProcess, m_regConfirmKillProcess);
	Store(m_bSyncDialogRandomPos, m_regSyncDialogRandomPos);
	Store(m_bRefCompareHideUnchanged, m_regRefCompareHideUnchanged);
	Store(m_bSortTagsReversed, m_regSortTagsReversed);

	Store(m_bAutocompletion, m_regAutocompletion);
	Store(m_dwAutocompletionTimeout, m_regAutocompletionTimeout);
	Store(m_dwMaxHistory, m_regMaxHistory);
	Store(m_bAutoSelect, m_regAutoSelect);
	Store(m_bStripCommentedLines, m_regStripCommentedLines);
	Store(m_bShowGitexeTimings, m_regShowGitexeTimings);
	Store(m_bNoSounds, m_regNoSounds);
	Store(m_bBranchesIncludeFetchHead, m_regBranchesIncludeFetchHead);
	Store(m_bNoAutoselectMissing, m_regNoAutoselectMissing);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetDialogs2::OnCbnSelchangeAutoclosecombo()
{
	if (m_cAutoCloseGitProgress.GetCurSel() != CB_ERR)
	{
		m_dwAutoCloseGitProgress = m_cAutoCloseGitProgress.GetItemData(m_cAutoCloseGitProgress.GetCurSel());
	}
	SetModified();
}
