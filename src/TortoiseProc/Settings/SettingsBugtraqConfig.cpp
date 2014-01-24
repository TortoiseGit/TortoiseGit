// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014 - TortoiseGit

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
// settings\SettingsBugtraqConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingsBugtraqConfig.h"
#include "ProjectProperties.h"
#include "Git.h"
#include "BugtraqRegexTestDlg.h"

// CSettingsBugtraqConfig dialog

IMPLEMENT_DYNAMIC(CSettingsBugtraqConfig, ISettingsPropPage)

CSettingsBugtraqConfig::CSettingsBugtraqConfig(CString cmdPath)
	: ISettingsPropPage(CSettingsBugtraqConfig::IDD)
	, m_URL(_T(""))
	, m_Message(_T(""))
	, m_Label(_T(""))
	, m_Logregex(_T(""))
	, m_bNeedSave(false)
	, m_bInheritURL(FALSE)
	, m_bInheritMessage(FALSE)
	, m_bInheritLabel(FALSE)
	, m_bInheritLogregex(FALSE)
{
}

CSettingsBugtraqConfig::~CSettingsBugtraqConfig()
{
}

void CSettingsBugtraqConfig::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BUGTRAQ_URL, m_URL);
	DDX_Control(pDX, IDC_BUGTRAQ_WARNINGIFNOISSUE, m_cWarningifnoissue);
	DDX_Text(pDX, IDC_BUGTRAQ_MESSAGE, m_Message);
	DDX_Control(pDX, IDC_BUGTRAQ_APPEND, m_cAppend);
	DDX_Text(pDX, IDC_BUGTRAQ_LABEL, m_Label);
	DDX_Control(pDX, IDC_BUGTRAQ_NUMBER, m_cNumber);
	DDX_Text(pDX, IDC_BUGTRAQ_LOGREGEX, m_Logregex);
	DDX_Control(pDX, IDC_BUGTRAQ_LOGREGEX, m_BugtraqRegex1);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTURL, m_bInheritURL);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTMSG, m_bInheritMessage);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTLABEL, m_bInheritLabel);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTREGEXP, m_bInheritLogregex);
	GITSETTINGS_DDX
}

BEGIN_MESSAGE_MAP(CSettingsBugtraqConfig, ISettingsPropPage)
	GITSETTINGS_RADIO_EVENT
	ON_EN_CHANGE(IDC_BUGTRAQ_URL, &CSettingsBugtraqConfig::OnChange)
	ON_CBN_SELCHANGE(IDC_BUGTRAQ_WARNINGIFNOISSUE, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_MESSAGE, &CSettingsBugtraqConfig::OnChange)
	ON_CBN_SELCHANGE(IDC_BUGTRAQ_APPEND, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_LABEL, &CSettingsBugtraqConfig::OnChange)
	ON_CBN_SELCHANGE(IDC_BUGTRAQ_NUMBER, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_LOGREGEX, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTURL, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTMSG, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTLABEL, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTREGEXP, &OnChange)
	ON_BN_CLICKED(IDC_TESTBUGTRAQREGEXBUTTON, &CSettingsBugtraqConfig::OnBnClickedTestbugtraqregexbutton)
END_MESSAGE_MAP()

BOOL CSettingsBugtraqConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AddTrueFalseToComboBox(m_cWarningifnoissue);
	AddTrueFalseToComboBox(m_cAppend);
	AddTrueFalseToComboBox(m_cNumber);

	m_tooltips.Create(this);

	InitGitSettings(this, true, &m_tooltips);

	this->UpdateData(FALSE);
	return TRUE;
}

void CSettingsBugtraqConfig::EnDisableControls()
{
	GetDlgItem(IDC_BUGTRAQ_URL)->SendMessage(EM_SETREADONLY, m_iConfigSource == 0, 0);
	GetDlgItem(IDC_BUGTRAQ_MESSAGE)->SendMessage(EM_SETREADONLY, m_iConfigSource == 0, 0);
	GetDlgItem(IDC_BUGTRAQ_LABEL)->SendMessage(EM_SETREADONLY, m_iConfigSource == 0, 0);
	GetDlgItem(IDC_BUGTRAQ_LOGREGEX)->SendMessage(EM_SETREADONLY, m_iConfigSource == 0, 0);
	GetDlgItem(IDC_BUGTRAQ_WARNINGIFNOISSUE)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_BUGTRAQ_APPEND)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_BUGTRAQ_NUMBER)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_COMBO_SETTINGS_SAFETO)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_CHECK_INHERIT_BTURL)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_CHECK_INHERIT_BTMSG)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_CHECK_INHERIT_BTLABEL)->EnableWindow(m_iConfigSource != 0);
	GetDlgItem(IDC_CHECK_INHERIT_BTREGEXP)->EnableWindow(m_iConfigSource != 0);

	GetDlgItem(IDC_BUGTRAQ_URL)->EnableWindow(!m_bInheritURL);
	GetDlgItem(IDC_BUGTRAQ_MESSAGE)->EnableWindow(!m_bInheritMessage);
	GetDlgItem(IDC_BUGTRAQ_LABEL)->EnableWindow(!m_bInheritLabel);
	GetDlgItem(IDC_BUGTRAQ_LOGREGEX)->EnableWindow(!m_bInheritLogregex);
	UpdateData(FALSE);
}

void CSettingsBugtraqConfig::OnChange()
{
	UpdateData();
	EnDisableControls();
	m_bNeedSave = true;
	SetModified();
}

void CSettingsBugtraqConfig::LoadDataImpl(git_config * config)
{
	if (m_iConfigSource == 0)
	{
		// use project properties here, so that we correctly get the default values
		ProjectProperties props;
		props.ReadProps();
		m_URL = props.sUrl;
		m_Logregex = props.sCheckRe + _T("\n") + props.sBugIDRe;
		m_Label = props.sLabel;
		m_Message = props.sMessage;

		if (props.bAppend)
			m_cAppend.SetCurSel(1);
		else
			m_cAppend.SetCurSel(2);

		if (props.bNumber)
			m_cNumber.SetCurSel(1);
		else
			m_cNumber.SetCurSel(2);

		if (props.bWarnIfNoIssue)
			m_cWarningifnoissue.SetCurSel(1);
		else
			m_cWarningifnoissue.SetCurSel(2);

		m_bInheritURL = FALSE;
		m_bInheritMessage = FALSE;
		m_bInheritLabel = FALSE;
		m_bInheritLogregex = FALSE;
	}
	else
	{
		m_bInheritURL = (GetConfigValue(config, BUGTRAQPROPNAME_URL, m_URL) == GIT_ENOTFOUND);
		m_bInheritMessage = (GetConfigValue(config, BUGTRAQPROPNAME_MESSAGE, m_Message) == GIT_ENOTFOUND);
		m_bInheritLabel = (GetConfigValue(config, BUGTRAQPROPNAME_LABEL, m_Label) == GIT_ENOTFOUND);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_NUMBER, m_cNumber);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_APPEND, m_cAppend);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_WARNIFNOISSUE, m_cWarningifnoissue);
		m_bInheritLogregex = (GetConfigValue(config, BUGTRAQPROPNAME_LOGREGEX, m_Logregex) == GIT_ENOTFOUND);
	}

	m_Logregex.Trim();
	m_Logregex.Replace(_T("\n"), _T("\r\n"));

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

BOOL CSettingsBugtraqConfig::SafeDataImpl(git_config * config)
{
	if (!Save(config, BUGTRAQPROPNAME_URL, m_URL, m_bInheritURL == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_MESSAGE, m_Message, m_bInheritMessage == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_LABEL, m_Label, m_bInheritLabel == TRUE))
		return FALSE;

	{
		CString value;
		m_cAppend.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_APPEND, value, value.IsEmpty()))
			return FALSE;
	}
	{
		CString value;
		m_cNumber.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_NUMBER, value, value.IsEmpty()))
			return FALSE;
	}
	{
		CString value;
		m_cWarningifnoissue.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_WARNIFNOISSUE, value, value.IsEmpty()))
			return FALSE;
	}
	{
		CString value(m_Logregex);
		value.Replace(_T("\r\n"),_T("\n"));
		if (!Save(config, BUGTRAQPROPNAME_LOGREGEX, value, m_bInheritLogregex == TRUE))
			return FALSE;
	}

	return TRUE;
}

BOOL CSettingsBugtraqConfig::OnApply()
{
	if (!m_bNeedSave)
		return TRUE;
	UpdateData();
	if (!SafeData())
		return FALSE;
	m_bNeedSave = false;
	SetModified(FALSE);
	return TRUE;
}

BOOL CSettingsBugtraqConfig::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsBugtraqConfig::OnBnClickedTestbugtraqregexbutton()
{
	m_tooltips.Pop(); // hide the tooltips
	CBugtraqRegexTestDlg dlg(this);
	dlg.m_sBugtraqRegex2 = m_Logregex;
	dlg.m_sBugtraqRegex2.Trim();
	dlg.m_sBugtraqRegex2.Replace(_T("\r\n"), _T("\n"));
	if (dlg.m_sBugtraqRegex2.Find('\n') >= 0)
	{
		dlg.m_sBugtraqRegex1 = dlg.m_sBugtraqRegex2.Mid(dlg.m_sBugtraqRegex2.Find('\n')).Trim();
		dlg.m_sBugtraqRegex2 = dlg.m_sBugtraqRegex2.Left(dlg.m_sBugtraqRegex2.Find('\n')).Trim();
	}
	if (dlg.DoModal() == IDOK)
	{
		m_Logregex = dlg.m_sBugtraqRegex1 + _T("\n") + dlg.m_sBugtraqRegex2;
		m_Logregex.Trim();
		m_Logregex.Replace(_T("\n"), _T("\r\n"));
		UpdateData(FALSE);
	}
}
