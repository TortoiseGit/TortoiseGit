// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "SetMisc.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSetMisc, ISettingsPropPage)

CSetMisc::CSetMisc()
	: ISettingsPropPage(CSetMisc::IDD)
	, m_bUnversionedRecurse(FALSE)
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_bSpell(TRUE)
	, m_bCheckRepo(FALSE)
	, m_dwMaxHistory(25)
	, m_bCommitReopen(FALSE)
	, m_bAutoSelect(TRUE)
{
	m_regUnversionedRecurse = CRegDWORD(_T("Software\\TortoiseGit\\UnversionedRecurse"), TRUE);
	m_bUnversionedRecurse = (DWORD)m_regUnversionedRecurse;
	m_regAutocompletion = CRegDWORD(_T("Software\\TortoiseGit\\Autocompletion"), TRUE);
	m_bAutocompletion = (DWORD)m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(_T("Software\\TortoiseGit\\AutocompleteParseTimeout"), 5);
	m_dwAutocompletionTimeout = (DWORD)m_regAutocompletionTimeout;
	m_regSpell = CRegDWORD(_T("Software\\TortoiseGit\\Spellchecker"), FALSE);
	m_bSpell = (DWORD)m_regSpell;
	m_regCheckRepo = CRegDWORD(_T("Software\\TortoiseGit\\CheckRepo"), FALSE);
	m_bCheckRepo = (DWORD)m_regCheckRepo;
	m_regMaxHistory = CRegDWORD(_T("Software\\TortoiseGit\\MaxHistoryItems"), 25);
	m_dwMaxHistory = (DWORD)m_regMaxHistory;
	m_regCommitReopen = CRegDWORD(_T("Software\\TortoiseGit\\CommitReopen"), FALSE);
	m_bCommitReopen = (BOOL)(DWORD)m_regCommitReopen;
	m_regAutoSelect = CRegDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE);
	m_bAutoSelect = (BOOL)(DWORD)m_regAutoSelect;
}

CSetMisc::~CSetMisc()
{
}

void CSetMisc::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UNVERSIONEDRECURSE, m_bUnversionedRecurse);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Check(pDX, IDC_SPELL, m_bSpell);
	DDX_Check(pDX, IDC_REPOCHECK, m_bCheckRepo);
	DDX_Text(pDX, IDC_MAXHISTORY, m_dwMaxHistory);
	DDV_MinMaxUInt(pDX, m_dwMaxHistory, 1, 100);
	DDX_Check(pDX, IDC_REOPENCOMMIT, m_bCommitReopen);
	DDX_Check(pDX, IDC_SELECTFILESONCOMMIT, m_bAutoSelect);
}


BEGIN_MESSAGE_MAP(CSetMisc, ISettingsPropPage)
	ON_BN_CLICKED(IDC_UNVERSIONEDRECURSE, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_MAXHISTORY, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SPELL, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REPOCHECK, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REOPENCOMMIT, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SELECTFILESONCOMMIT, &CSetMisc::OnChanged)
END_MESSAGE_MAP()

void CSetMisc::OnChanged()
{
	SetModified();
}

BOOL CSetMisc::OnApply()
{
	UpdateData();

    Store (m_bUnversionedRecurse, m_regUnversionedRecurse);
	Store (m_bAutocompletion, m_regAutocompletion);
	Store (m_dwAutocompletionTimeout, m_regAutocompletionTimeout);
	Store (m_bSpell, m_regSpell);
	Store (m_bCheckRepo, m_regCheckRepo);
	Store (m_dwMaxHistory, m_regMaxHistory);
	Store (m_bCommitReopen, m_regCommitReopen);
	Store (m_bAutoSelect, m_regAutoSelect);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

BOOL CSetMisc::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_UNVERSIONEDRECURSE, IDS_SETTINGS_UNVERSIONEDRECURSE_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETION, IDS_SETTINGS_AUTOCOMPLETION_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_SPELL, IDS_SETTINGS_SPELLCHECKER_TT);
	m_tooltips.AddTool(IDC_REOPENCOMMIT, IDS_SETTINGS_COMMITREOPEN_TT);
	m_tooltips.AddTool(IDC_REPOCHECK, IDS_SETTINGS_REPOCHECK_TT);
	m_tooltips.AddTool(IDC_MAXHISTORY, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_MAXHISTORYLABEL, IDS_SETTINGS_MAXHISTORY_TT);
		m_tooltips.AddTool(IDC_SELECTFILESONCOMMIT, IDS_SETTINGS_SELECTFILESONCOMMIT_TT);
	return TRUE;
}

BOOL CSetMisc::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}
