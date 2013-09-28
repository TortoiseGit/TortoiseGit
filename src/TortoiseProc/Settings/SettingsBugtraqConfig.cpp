// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013 - TortoiseGit

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
// CSettingsBugtraqConfig dialog

IMPLEMENT_DYNAMIC(CSettingsBugtraqConfig, ISettingsPropPage)

CSettingsBugtraqConfig::CSettingsBugtraqConfig(CString cmdPath)
	: ISettingsPropPage(CSettingsBugtraqConfig::IDD)
	, m_URL(_T(""))
	, m_Message(_T(""))
	, m_Label(_T(""))
	, m_Logregex(_T(""))
	, m_bNeedSave(false)
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
END_MESSAGE_MAP()

BOOL CSettingsBugtraqConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AddTrueFalseToComboBox(m_cWarningifnoissue);
	AddTrueFalseToComboBox(m_cAppend);
	AddTrueFalseToComboBox(m_cNumber);

	InitGitSettings(this, true);

	this->UpdateData(FALSE);
	return TRUE;
}

void CSettingsBugtraqConfig::OnChange()
{
	m_bNeedSave = true;
	SetModified();
}

void CSettingsBugtraqConfig::LoadDataImpl(git_config * config)
{
	if (m_iConfigSource == 0)
	{
		// use project properties here, so that we correctly get the default values
		ProjectProperties props;
		props.ReadProps(g_Git.m_CurrentDir);
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
	}
	else
	{
		GetConfigValue(config, BUGTRAQPROPNAME_URL, m_URL);
		GetConfigValue(config, BUGTRAQPROPNAME_MESSAGE, m_Message);
		GetConfigValue(config, BUGTRAQPROPNAME_LABEL, m_Label);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_NUMBER, m_cNumber);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_APPEND, m_cAppend);
		GetBoolConfigValueComboBox(config, BUGTRAQPROPNAME_WARNIFNOISSUE, m_cWarningifnoissue);
		GetConfigValue(config, BUGTRAQPROPNAME_LOGREGEX, m_Logregex);
	}

	m_Logregex.Trim();
	m_Logregex.Replace(_T("\n"), _T("\r\n"));

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

BOOL CSettingsBugtraqConfig::SafeDataImpl(git_config * config)
{
	if (!Save(config, BUGTRAQPROPNAME_URL, m_URL))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_MESSAGE, m_Message))
		return FALSE;

	if (!Save(config, BUGTRAQPROPNAME_LABEL, m_Label))
		return FALSE;

	{
		CString value;
		m_cAppend.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_APPEND, value))
			return FALSE;
	}
	{
		CString value;
		m_cNumber.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_NUMBER, value))
			return FALSE;
	}
	{
		CString value;
		m_cWarningifnoissue.GetWindowText(value);
		if (!Save(config, BUGTRAQPROPNAME_WARNIFNOISSUE, value))
			return FALSE;
	}
	{
		CString value(m_Logregex);
		value.Replace(_T("\r\n"),_T("\n"));
		if (!Save(config, BUGTRAQPROPNAME_LOGREGEX, value))
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
