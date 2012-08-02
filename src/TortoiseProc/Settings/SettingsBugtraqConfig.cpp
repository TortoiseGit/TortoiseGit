// settings\SettingsBugtraqConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "settings\SettingsBugtraqConfig.h"
#include "ProjectProperties.h"
#include "git.h"
#include "messagebox.h"
// CSettingsBugtraqConfig dialog

IMPLEMENT_DYNAMIC(CSettingsBugtraqConfig, ISettingsPropPage)

CSettingsBugtraqConfig::CSettingsBugtraqConfig(CString cmdPath)
	: ISettingsPropPage(CSettingsBugtraqConfig::IDD)
	, m_URL(_T(""))
	, m_bNWarningifnoissue(FALSE)
	, m_Message(_T(""))
	, m_bNAppend(FALSE)
	, m_Label(_T(""))
	, m_bNNumber(FALSE)
	, m_Logregex(_T(""))
{
	m_ChangeMask=0;
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

	ON_EN_CHANGE(IDC_BUGTRAQ_URL, &CSettingsBugtraqConfig::OnEnChangeBugtraqUrl)
	ON_BN_CLICKED(IDC_BUGTRAQ_WARNINGIFNOISSUE_TRUE, &CSettingsBugtraqConfig::OnBnClickedBugtraqWarningifnoissueTrue)
	ON_BN_CLICKED(IDC_BUGTRAQ_WARNINGIFNOISSUE_FALSE, &CSettingsBugtraqConfig::OnBnClickedBugtraqWarningifnoissueFalse)
	ON_EN_CHANGE(IDC_BUGTRAQ_MESSAGE, &CSettingsBugtraqConfig::OnEnChangeBugtraqMessage)
	ON_BN_CLICKED(IDC_BUGTRAQ_APPEND_TRUE, &CSettingsBugtraqConfig::OnBnClickedBugtraqAppendTrue)
	ON_BN_CLICKED(IDC_BUGTRAQ_APPEND_FALSE, &CSettingsBugtraqConfig::OnBnClickedBugtraqAppendFalse)
	ON_EN_CHANGE(IDC_BUGTRAQ_LABEL, &CSettingsBugtraqConfig::OnEnChangeBugtraqLabel)
	ON_BN_CLICKED(IDC_BUGTRAQ_NUMBER_TRUE, &CSettingsBugtraqConfig::OnBnClickedBugtraqNumberTrue)
	ON_BN_CLICKED(IDC_BUGTRAQ_NUMBER_FALSE, &CSettingsBugtraqConfig::OnBnClickedBugtraqNumberFalse)
	ON_EN_CHANGE(IDC_BUGTRAQ_LOGREGEX, &CSettingsBugtraqConfig::OnEnChangeBugtraqLogregex)
END_MESSAGE_MAP()

BOOL CSettingsBugtraqConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();
	ProjectProperties::GetStringProps(this->m_URL,_T("bugtraq.url"));
	ProjectProperties::GetStringProps(this->m_Logregex,_T("bugtraq.logregex"),false);
	ProjectProperties::GetStringProps(this->m_Label,_T("bugtraq.label"));
	ProjectProperties::GetStringProps(this->m_Message,_T("bugtraq.message"));

	ProjectProperties::GetBOOLProps(this->m_bNAppend,_T("bugtraq.append"));
	ProjectProperties::GetBOOLProps(this->m_bNNumber,_T("bugtraq.number"));
	ProjectProperties::GetBOOLProps(this->m_bNWarningifnoissue,_T("bugtraq.warnifnoissue"));

	m_Logregex.Trim();
	m_Logregex.Replace(_T("\n"),_T("\r\n"));

	m_bNAppend = !m_bNAppend;
	m_bNNumber = !m_bNNumber;
	m_bNWarningifnoissue = !m_bNWarningifnoissue;

	this->UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsBugtraqConfig::OnApply()
{
	this->UpdateData();

	CString cmd,out;
	if(m_ChangeMask & BUG_URL)
	{
		if (g_Git.SetConfigValue(_T("bugtraq.url"), m_URL,CONFIG_LOCAL, CP_UTF8))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
	}

	if(m_ChangeMask & BUG_WARNING)
	{
		if(g_Git.SetConfigValue(_T("bugtraq.warnifnoissue"),(!this->m_bNWarningifnoissue)?_T("true"):_T("false")))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}

	}

	if(m_ChangeMask & BUG_MESSAGE)
	{
		if(g_Git.SetConfigValue(_T("bugtraq.message"),m_Message,CONFIG_LOCAL,g_Git.GetGitEncode(L"i18n.commitencoding")))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
	}

	if(m_ChangeMask & BUG_APPEND )
	{
		if(g_Git.SetConfigValue(_T("bugtraq.append"),(!this->m_bNAppend)?_T("true"):_T("false")))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}

	}

	if(m_ChangeMask & BUG_LABEL )
	{
		if(g_Git.SetConfigValue(_T("bugtraq.label"),m_Label))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
	}

	if(m_ChangeMask &BUG_NUMBER )
	{
		if(g_Git.SetConfigValue(_T("bugtraq.number"),(!this->m_bNNumber)?_T("true"):_T("false")))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
	}

	if(m_ChangeMask & BUG_LOGREGEX)
	{
		m_Logregex.Replace(_T("\r\n"),_T("\n"));
		if(g_Git.SetConfigValue(_T("bugtraq.logregex"),m_Logregex))
		{
			CMessageBox::Show(NULL,_T("Fail to set config"),_T("TortoiseGit"),MB_OK);
		}
		m_Logregex.Replace(_T("\n"),_T("\r\n"));
	}

	m_ChangeMask= 0;
	return TRUE;
}
// CSettingsBugtraqConfig message handlers

void CSettingsBugtraqConfig::OnEnChangeBugtraqUrl()
{
	m_ChangeMask |= BUG_URL;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqWarningifnoissueTrue()
{
	m_ChangeMask |= BUG_WARNING;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqWarningifnoissueFalse()
{
	m_ChangeMask |= BUG_WARNING;
	SetModified();
}

void CSettingsBugtraqConfig::OnEnChangeBugtraqMessage()
{
	m_ChangeMask |= BUG_MESSAGE;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqAppendTrue()
{
	m_ChangeMask |= BUG_APPEND;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqAppendFalse()
{
	m_ChangeMask |= BUG_APPEND;
	SetModified();
}

void CSettingsBugtraqConfig::OnEnChangeBugtraqLabel()
{
	m_ChangeMask |= BUG_LABEL;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqNumberTrue()
{
	m_ChangeMask |= BUG_NUMBER;
	SetModified();
}

void CSettingsBugtraqConfig::OnBnClickedBugtraqNumberFalse()
{
	m_ChangeMask |= BUG_NUMBER;
	SetModified();
}

void CSettingsBugtraqConfig::OnEnChangeBugtraqLogregex()
{
	m_ChangeMask |= BUG_LOGREGEX;
	SetModified();
}
