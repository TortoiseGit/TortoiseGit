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
	, m_bNWarningifnoissue(FALSE)
	, m_Message(_T(""))
	, m_bNAppend(FALSE)
	, m_Label(_T(""))
	, m_bNNumber(TRUE)
	, m_Logregex(_T(""))
{
}

CSettingsBugtraqConfig::~CSettingsBugtraqConfig()
{
}

void CSettingsBugtraqConfig::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BUGTRAQ_URL, m_URL);
	DDX_Radio(pDX, IDC_BUGTRAQ_WARNINGIFNOISSUE_TRUE, m_bNWarningifnoissue);
	DDX_Text(pDX, IDC_BUGTRAQ_MESSAGE, m_Message);
	DDX_Radio(pDX, IDC_BUGTRAQ_APPEND_TRUE, m_bNAppend);
	DDX_Text(pDX, IDC_BUGTRAQ_LABEL, m_Label);
	DDX_Radio(pDX, IDC_BUGTRAQ_NUMBER_TRUE, m_bNNumber);
	DDX_Text(pDX, IDC_BUGTRAQ_LOGREGEX, m_Logregex);
	DDX_Control(pDX, IDC_BUGTRAQ_LOGREGEX, m_BugtraqRegex1);
}


BEGIN_MESSAGE_MAP(CSettingsBugtraqConfig, ISettingsPropPage)

	ON_EN_CHANGE(IDC_BUGTRAQ_URL, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_WARNINGIFNOISSUE_TRUE, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_WARNINGIFNOISSUE_FALSE, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_MESSAGE, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_APPEND_TRUE, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_APPEND_FALSE, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_LABEL, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_NUMBER_TRUE, &CSettingsBugtraqConfig::OnChange)
	ON_BN_CLICKED(IDC_BUGTRAQ_NUMBER_FALSE, &CSettingsBugtraqConfig::OnChange)
	ON_EN_CHANGE(IDC_BUGTRAQ_LOGREGEX, &CSettingsBugtraqConfig::OnChange)
END_MESSAGE_MAP()

BOOL CSettingsBugtraqConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();
	ProjectProperties props;
	props.ReadProps(g_Git.m_CurrentDir);
	m_URL = props.sUrl;
	m_Logregex = props.sCheckRe + _T("\n") + props.sBugIDRe;
	m_Label = props.sLabel;
	m_Message = props.sMessage;

	m_bNAppend = props.bAppend;
	m_bNNumber = props.bNumber;
	m_bNWarningifnoissue = props.bWarnIfNoIssue;

	m_Logregex.Trim();
	m_Logregex.Replace(_T("\n"),_T("\r\n"));

	m_bNAppend = !m_bNAppend;
	m_bNNumber = !m_bNNumber;
	m_bNWarningifnoissue = !m_bNWarningifnoissue;

	this->UpdateData(FALSE);
	return TRUE;
}

void CSettingsBugtraqConfig::OnChange()
{
	SetModified();
}

BOOL CSettingsBugtraqConfig::OnApply()
{
	this->UpdateData();

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_URL, m_URL, CONFIG_LOCAL))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_WARNIFNOISSUE, (!this->m_bNWarningifnoissue) ? _T("true") : _T("false")))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_MESSAGE, m_Message, CONFIG_LOCAL, g_Git.GetGitEncode(L"i18n.commitencoding")))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_APPEND, (!this->m_bNAppend) ? _T("true") : _T("false")))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_LABEL, m_Label))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	if (g_Git.SetConfigValue(BUGTRAQPROPNAME_NUMBER, (!this->m_bNNumber) ? _T("true") : _T("false")))
	{
		CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
	}

	{
		m_Logregex.Replace(_T("\r\n"),_T("\n"));
		if (g_Git.SetConfigValue(BUGTRAQPROPNAME_LOGREGEX, m_Logregex))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
		m_Logregex.Replace(_T("\n"),_T("\r\n"));
	}

	return TRUE;
}
