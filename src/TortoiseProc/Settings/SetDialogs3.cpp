// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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
#include "ProjectProperties.h"
#include "SetDialogs3.h"

CComboBox	CSetDialogs3::m_langCombo;

IMPLEMENT_DYNAMIC(CSetDialogs3, ISettingsPropPage)
CSetDialogs3::CSetDialogs3()
	: ISettingsPropPage(CSetDialogs3::IDD)
	, m_bNeedSave(false)
{
}

CSetDialogs3::~CSetDialogs3()
{
}

void CSetDialogs3::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGCOMBO, m_langCombo);
	DDX_Text(pDX, IDC_LOGMINSIZE, m_LogMinSize);
	DDX_Text(pDX, IDC_BORDER, m_Border);
	DDX_Control(pDX, IDC_WARN_NO_SIGNED_OFF_BY, m_cWarnNoSignedOffBy);
	GITSETTINGS_DDX
}

BEGIN_MESSAGE_MAP(CSetDialogs3, ISettingsPropPage)
	GITSETTINGS_RADIO_EVENT
	ON_CBN_SELCHANGE(IDC_LANGCOMBO, &OnChange)
	ON_CBN_SELCHANGE(IDC_WARN_NO_SIGNED_OFF_BY, &OnChange)
	ON_EN_CHANGE(IDC_LOGMINSIZE, &OnChange)
	ON_EN_CHANGE(IDC_BORDER, &OnChange)
END_MESSAGE_MAP()

// CSetDialogs2 message handlers
BOOL CSetDialogs3::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AddTrueFalseToComboBox(m_cWarnNoSignedOffBy);

	m_langCombo.AddString(_T(""));
	m_langCombo.SetItemData(0, 0);
	m_langCombo.AddString(_T("(disable)"));
	m_langCombo.SetItemData(1, (DWORD_PTR)-1);
	// fill the combo box with all available languages
	EnumSystemLocales(EnumLocalesProc, LCID_SUPPORTED);

	m_tooltips.Create(this);

	InitGitSettings(this, true, &m_tooltips);

	UpdateData(FALSE);
	return TRUE;
}

static void SelectLanguage(CComboBox &combobox, LONG langueage)
{
	for (int i = 0; i < combobox.GetCount(); ++i)
	{
		if (combobox.GetItemData(i) == langueage)
		{
			combobox.SetCurSel(i);
			break;
		}
	}
}

void CSetDialogs3::LoadDataImpl(git_config * config)
{
	{
		CString value;
		GetConfigValue(config, PROJECTPROPNAME_PROJECTLANGUAGE, value);
		if (value == _T("-1"))
			m_langCombo.SetCurSel(1);
		else if (!value.IsEmpty())
		{
			LPTSTR strEnd;
			long longValue = _tcstol(value, &strEnd, 0);
			if (longValue == 0)
			{
				if (m_iConfigSource == 0)
					SelectLanguage(m_langCombo, CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033));
				else
					m_langCombo.SetCurSel(0);
			}
			else
				SelectLanguage(m_langCombo, longValue);
		} else if (m_iConfigSource == 0)
			SelectLanguage(m_langCombo, CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033));
		else
			m_langCombo.SetCurSel(0);
	}

	{
		m_LogMinSize = _T("");
		CString value;
		GetConfigValue(config, PROJECTPROPNAME_LOGMINSIZE, value);
		if (!value.IsEmpty() || m_iConfigSource == 0)
		{
			int nMinLogSize = _ttoi(value);
			m_LogMinSize.Format(L"%d", nMinLogSize);
		}
	}

	{
		m_Border = _T("");
		CString value;
		GetConfigValue(config, PROJECTPROPNAME_LOGWIDTHLINE, value);
		if (!value.IsEmpty() || m_iConfigSource == 0)
		{
			int nLogWidthMarker = _ttoi(value);
			m_Border.Format(L"%d", nLogWidthMarker);
		}
	}

	GetBoolConfigValueComboBox(config, PROJECTPROPNAME_WARNNOSIGNEDOFFBY, m_cWarnNoSignedOffBy);

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

BOOL CSetDialogs3::SafeDataImpl(git_config * config)
{
	if (m_langCombo.GetCurSel() == 1)
	{
		if (!Save(config, PROJECTPROPNAME_PROJECTLANGUAGE, L"-1"))
			return FALSE;
	}
	else
	{
		CString value;
		char numBuf[20];
		sprintf_s(numBuf, "%ld", m_langCombo.GetItemData(m_langCombo.GetCurSel()));
		if (!Save(config, PROJECTPROPNAME_PROJECTLANGUAGE, (CString)numBuf, true, _T("0")))
			return FALSE;
	}

	if (!Save(config, PROJECTPROPNAME_LOGMINSIZE, m_LogMinSize, true, _T("0")))
		return FALSE;

	if (!Save(config, PROJECTPROPNAME_LOGWIDTHLINE, m_Border, true, _T("0")))
		return FALSE;

	{
		CString value;
		m_cWarnNoSignedOffBy.GetWindowText(value);
		if (!Save(config, PROJECTPROPNAME_WARNNOSIGNEDOFFBY, value))
			return FALSE;
	}

	return TRUE;
}

BOOL CSetDialogs3::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetDialogs3::OnChange()
{
	m_bNeedSave = true;
	SetModified();
}

BOOL CSetDialogs3::OnApply()
{
	if (!m_bNeedSave)
		return TRUE;
	UpdateData();
	if (!SafeData())
		return FALSE;
	m_bNeedSave = false;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

BOOL CSetDialogs3::EnumLocalesProc(LPTSTR lpLocaleString)
{
	DWORD langID = _tcstol(lpLocaleString, NULL, 16);

	TCHAR buf[MAX_PATH] = {0};
	GetLocaleInfo(langID, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
	CString sLang = buf;
	GetLocaleInfo(langID, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
	if (buf[0])
	{
		sLang += _T(" (");
		sLang += buf;
		sLang += _T(")");
	}

	int index = m_langCombo.AddString(sLang);
	m_langCombo.SetItemData(index, langID);

	return TRUE;
}
