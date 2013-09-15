// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
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
#include "GitProgressDlg.h"
#include "SetDialogs2.h"

IMPLEMENT_DYNAMIC(CSetDialogs2, ISettingsPropPage)
CSetDialogs2::CSetDialogs2()
	: ISettingsPropPage(CSetDialogs2::IDD)
	, m_dwAutoClose(0)
	, m_bUseRecycleBin(TRUE)
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_dwMaxHistory(25)
	, m_bAutoSelect(TRUE)
{
	m_regAutoClose = CRegDWORD(_T("Software\\TortoiseGit\\AutoClose"));
	m_regUseRecycleBin = CRegDWORD(_T("Software\\TortoiseGit\\RevertWithRecycleBin"), TRUE);
	m_regAutocompletion = CRegDWORD(_T("Software\\TortoiseGit\\Autocompletion"), TRUE);
	m_bAutocompletion = (DWORD)m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(_T("Software\\TortoiseGit\\AutocompleteParseTimeout"), 5);
	m_dwAutocompletionTimeout = (DWORD)m_regAutocompletionTimeout;
	m_regMaxHistory = CRegDWORD(_T("Software\\TortoiseGit\\MaxHistoryItems"), 25);
	m_dwMaxHistory = (DWORD)m_regMaxHistory;
	m_regAutoSelect = CRegDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE);
	m_bAutoSelect = (BOOL)(DWORD)m_regAutoSelect;
}

CSetDialogs2::~CSetDialogs2()
{
}

void CSetDialogs2::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AUTOCLOSECOMBO, m_cAutoClose);
	DDX_Check(pDX, IDC_USERECYCLEBIN, m_bUseRecycleBin);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Text(pDX, IDC_MAXHISTORY, m_dwMaxHistory);
	DDV_MinMaxUInt(pDX, m_dwMaxHistory, 1, 100);
	DDX_Check(pDX, IDC_SELECTFILESONCOMMIT, m_bAutoSelect);
}

BEGIN_MESSAGE_MAP(CSetDialogs2, ISettingsPropPage)
	ON_CBN_SELCHANGE(IDC_AUTOCLOSECOMBO, OnCbnSelchangeAutoclosecombo)
	ON_BN_CLICKED(IDC_USERECYCLEBIN, OnChange)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, OnChange)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, OnChange)
	ON_EN_CHANGE(IDC_MAXHISTORY, OnChange)
	ON_BN_CLICKED(IDC_SELECTFILESONCOMMIT, OnChange)
END_MESSAGE_MAP()

// CSetDialogs2 message handlers
BOOL CSetDialogs2::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	ISettingsPropPage::OnInitDialog();

	EnableToolTips();

	int ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_MANUAL)));
	m_cAutoClose.SetItemData(ind, CLOSE_MANUAL);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOMERGES)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOMERGES);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOCONFLICTS)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOCONFLICTS);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOERROR)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOERRORS);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_LOCAL)));
	m_cAutoClose.SetItemData(ind, CLOSE_LOCAL);

	m_dwAutoClose = m_regAutoClose;
	m_bUseRecycleBin = m_regUseRecycleBin;

	for (int i=0; i<m_cAutoClose.GetCount(); ++i)
		if (m_cAutoClose.GetItemData(i)==m_dwAutoClose)
			m_cAutoClose.SetCurSel(i);

	CString temp;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_AUTOCLOSECOMBO, IDS_SETTINGS_AUTOCLOSE_TT);
	m_tooltips.AddTool(IDC_USERECYCLEBIN, IDS_SETTINGS_USERECYCLEBIN_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETION, IDS_SETTINGS_AUTOCOMPLETION_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_MAXHISTORY, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_MAXHISTORYLABEL, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_SELECTFILESONCOMMIT, IDS_SETTINGS_SELECTFILESONCOMMIT_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSetDialogs2::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetDialogs2::OnChange()
{
	SetModified();
}

BOOL CSetDialogs2::OnApply()
{
	UpdateData();

	Store ((DWORD)m_dwAutoClose, m_regAutoClose);
	Store (m_bUseRecycleBin, m_regUseRecycleBin);

	Store (m_bAutocompletion, m_regAutocompletion);
	Store (m_dwAutocompletionTimeout, m_regAutocompletionTimeout);
	Store (m_dwMaxHistory, m_regMaxHistory);
	Store (m_bAutoSelect, m_regAutoSelect);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetDialogs2::OnCbnSelchangeAutoclosecombo()
{
	if (m_cAutoClose.GetCurSel() != CB_ERR)
	{
		m_dwAutoClose = m_cAutoClose.GetItemData(m_cAutoClose.GetCurSel());
	}
	SetModified();
}
