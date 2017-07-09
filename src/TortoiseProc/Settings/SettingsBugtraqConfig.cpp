// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014, 2016-2017 - TortoiseGit

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
#include "BugTraqAssociations.h"

// CSettingsBugtraqConfig dialog

IMPLEMENT_DYNAMIC(CSettingsBugtraqConfig, ISettingsPropPage)

CSettingsBugtraqConfig::CSettingsBugtraqConfig()
: ISettingsPropPage(CSettingsBugtraqConfig::IDD)
, m_bNeedSave(false)
, m_bInheritURL(FALSE)
, m_bInheritMessage(FALSE)
, m_bInheritLabel(FALSE)
, m_bInheritLogregex(FALSE)
, m_bInheritUUID32(FALSE)
, m_bInheritUUID64(FALSE)
, m_bInheritParams(FALSE)
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
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTUUID32, m_bInheritUUID32);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTUUID64, m_bInheritUUID64);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BTPARAMS, m_bInheritParams);
	DDX_Text(pDX, IDC_UUID32, m_UUID32);
	DDX_Text(pDX, IDC_UUID64, m_UUID64);
	DDX_Text(pDX, IDC_PARAMS, m_Params);
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
	ON_EN_CHANGE(IDC_UUID32, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_UUID64, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_PARAMS, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTURL, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTMSG, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTLABEL, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTREGEXP, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTUUID32, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTUUID64, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BTPARAMS, &OnChange)
	ON_BN_CLICKED(IDC_TESTBUGTRAQREGEXBUTTON, &CSettingsBugtraqConfig::OnBnClickedTestbugtraqregexbutton)
END_MESSAGE_MAP()

BOOL CSettingsBugtraqConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_CHECK_INHERIT_BTURL);
	AdjustControlSize(IDC_CHECK_INHERIT_BTMSG);
	AdjustControlSize(IDC_CHECK_INHERIT_BTLABEL);
	AdjustControlSize(IDC_CHECK_INHERIT_BTREGEXP);
	AdjustControlSize(IDC_CHECK_INHERIT_BTUUID32);
	AdjustControlSize(IDC_CHECK_INHERIT_BTUUID64);
	AdjustControlSize(IDC_CHECK_INHERIT_BTPARAMS);
	GITSETTINGS_ADJUSTCONTROLSIZE

	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTURL, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTMSG, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTLABEL, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTREGEXP, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTUUID32, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTUUID64, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BTPARAMS, IDS_SETTINGS_GITCONFIG_INHERIT_TT);

	AddTrueFalseToComboBox(m_cWarningifnoissue);
	AddTrueFalseToComboBox(m_cAppend);
	AddTrueFalseToComboBox(m_cNumber);

	InitGitSettings(this, true, &m_tooltips);

	this->UpdateData(FALSE);
	return TRUE;
}

void CSettingsBugtraqConfig::EnDisableControls()
{
	GetDlgItem(IDC_BUGTRAQ_URL)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_BUGTRAQ_MESSAGE)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_BUGTRAQ_LABEL)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_BUGTRAQ_LOGREGEX)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_UUID32)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_UUID64)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_PARAMS)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);

	GetDlgItem(IDC_BUGTRAQ_WARNINGIFNOISSUE)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_BUGTRAQ_APPEND)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_BUGTRAQ_NUMBER)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_COMBO_SETTINGS_SAFETO)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTURL)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTMSG)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTLABEL)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTREGEXP)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTUUID32)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTUUID64)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BTPARAMS)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_TESTBUGTRAQREGEXBUTTON)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE && !m_bInheritLogregex);

	GetDlgItem(IDC_BUGTRAQ_URL)->EnableWindow(!m_bInheritURL);
	GetDlgItem(IDC_BUGTRAQ_MESSAGE)->EnableWindow(!m_bInheritMessage);
	GetDlgItem(IDC_BUGTRAQ_LABEL)->EnableWindow(!m_bInheritLabel);
	GetDlgItem(IDC_BUGTRAQ_LOGREGEX)->EnableWindow(!m_bInheritLogregex);
	GetDlgItem(IDC_UUID32)->EnableWindow(!m_bInheritUUID32);
	GetDlgItem(IDC_UUID64)->EnableWindow(!m_bInheritUUID64);
	GetDlgItem(IDC_PARAMS)->EnableWindow(!m_bInheritParams);
	UpdateData(FALSE);
}

void CSettingsBugtraqConfig::OnChange()
{
	UpdateData();
	EnDisableControls();
	m_bNeedSave = true;
	SetModified();
}

void CSettingsBugtraqConfig::LoadDataImpl(CAutoConfig& config)
{
	if (m_iConfigSource == CFG_SRC_EFFECTIVE)
	{
		// use project properties here, so that we correctly get the default values
		ProjectProperties props;
		props.ReadProps();
		m_URL = props.sUrl;
		m_Logregex = props.sCheckRe + L'\n' + props.sBugIDRe;
		m_Label = props.sLabel;
		m_Message = props.sMessage;
		m_UUID32 = props.sProviderUuid;
		m_UUID64 = props.sProviderUuid64;
		m_Params = props.sProviderParams;
		// read legacy registry values
		CBugTraqAssociations bugtraq_associations;
		bugtraq_associations.Load(props.GetProviderUUID(), props.sProviderParams);
		CBugTraqAssociation bugtraq_association;
		if (bugtraq_associations.FindProvider(g_Git.m_CurrentDir, &bugtraq_association))
		{
#if _WIN64
			m_UUID64 = bugtraq_association.GetProviderClassAsString();
#else
			m_UUID32 = bugtraq_association.GetProviderClassAsString();
			if (m_UUID64.IsEmpty())
				m_UUID64 = m_UUID32;
#endif
			m_Params = bugtraq_association.GetParameters();
		}

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
		m_bInheritParams = FALSE;
		m_bInheritUUID32 = FALSE;
		m_bInheritUUID64 = FALSE;
	}
	else
	{
		m_bInheritURL = (config.GetString(BUGTRAQPROPNAME_URL, m_URL) == GIT_ENOTFOUND);
		m_bInheritMessage = (config.GetString(BUGTRAQPROPNAME_MESSAGE, m_Message) == GIT_ENOTFOUND);
		m_bInheritLabel = (config.GetString(BUGTRAQPROPNAME_LABEL, m_Label) == GIT_ENOTFOUND);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_NUMBER, m_cNumber);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_APPEND, m_cAppend);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_WARNIFNOISSUE, m_cWarningifnoissue);
		m_bInheritLogregex = (config.GetString(BUGTRAQPROPNAME_LOGREGEX, m_Logregex) == GIT_ENOTFOUND);
		m_bInheritParams = (config.GetString(BUGTRAQPROPNAME_PROVIDERPARAMS, m_Params) == GIT_ENOTFOUND);
		m_bInheritUUID32 = (config.GetString(BUGTRAQPROPNAME_PROVIDERUUID, m_UUID32) == GIT_ENOTFOUND);
		m_bInheritUUID64 = (config.GetString(BUGTRAQPROPNAME_PROVIDERUUID64, m_UUID64) == GIT_ENOTFOUND);
	}

	m_Logregex.Trim();
	m_Logregex.Replace(L"\n", L"\r\n");

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

BOOL CSettingsBugtraqConfig::SafeDataImpl(CAutoConfig& config)
{
	if (!Save(config, BUGTRAQPROPNAME_URL, m_URL, m_bInheritURL == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_MESSAGE, m_Message, m_bInheritMessage == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_LABEL, m_Label, m_bInheritLabel == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_PROVIDERPARAMS, m_Params, m_bInheritParams == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_PROVIDERUUID, m_UUID32, m_bInheritUUID32 == TRUE))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_PROVIDERUUID64, m_UUID64, m_bInheritUUID64 == TRUE))
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
		value.Replace(L"\r\n",L"\n");
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

void CSettingsBugtraqConfig::OnBnClickedTestbugtraqregexbutton()
{
	m_tooltips.Pop(); // hide the tooltips
	CBugtraqRegexTestDlg dlg(this);
	dlg.m_sBugtraqRegex2 = m_Logregex;
	dlg.m_sBugtraqRegex2.Trim();
	dlg.m_sBugtraqRegex2.Replace(L"\r\n", L"\n");
	if (dlg.m_sBugtraqRegex2.Find('\n') >= 0)
	{
		dlg.m_sBugtraqRegex1 = dlg.m_sBugtraqRegex2.Mid(dlg.m_sBugtraqRegex2.Find('\n')).Trim();
		dlg.m_sBugtraqRegex2 = dlg.m_sBugtraqRegex2.Left(dlg.m_sBugtraqRegex2.Find('\n')).Trim();
	}
	if (dlg.DoModal() == IDOK)
	{
		m_Logregex = dlg.m_sBugtraqRegex2 + L'\n' + dlg.m_sBugtraqRegex1;
		m_Logregex.Trim();
		m_Logregex.Replace(L"\n", L"\r\n");
		UpdateData(FALSE);
	}
}
