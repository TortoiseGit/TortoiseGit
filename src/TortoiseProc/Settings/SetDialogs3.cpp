// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017, 2019 - TortoiseGit

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
#include "AppUtils.h"

static std::vector<DWORD> g_langs;

IMPLEMENT_DYNAMIC(CSetDialogs3, ISettingsPropPage)
CSetDialogs3::CSetDialogs3()
	: ISettingsPropPage(CSetDialogs3::IDD)
	, m_bNeedSave(false)
	, m_bInheritLogMinSize(FALSE)
	, m_bInheritBorder(FALSE)
	, m_bInheritIconFile(FALSE)
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
	DDX_Text(pDX, IDC_ICONFILE, m_iconFile);
	DDX_Control(pDX, IDC_WARN_NO_SIGNED_OFF_BY, m_cWarnNoSignedOffBy);
	DDX_Check(pDX, IDC_CHECK_INHERIT_LIMIT, m_bInheritLogMinSize);
	DDX_Check(pDX, IDC_CHECK_INHERIT_BORDER, m_bInheritBorder);
	DDX_Check(pDX, IDC_CHECK_INHERIT_KEYID, m_bInheritIconFile);
	GITSETTINGS_DDX
}

BEGIN_MESSAGE_MAP(CSetDialogs3, ISettingsPropPage)
	GITSETTINGS_RADIO_EVENT
	ON_CBN_SELCHANGE(IDC_LANGCOMBO, &OnChange)
	ON_CBN_SELCHANGE(IDC_WARN_NO_SIGNED_OFF_BY, &OnChange)
	ON_EN_CHANGE(IDC_LOGMINSIZE, &OnChange)
	ON_EN_CHANGE(IDC_BORDER, &OnChange)
	ON_EN_CHANGE(IDC_ICONFILE, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_LIMIT, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_BORDER, &OnChange)
	ON_BN_CLICKED(IDC_CHECK_INHERIT_KEYID, &OnChange)
	ON_BN_CLICKED(IDC_ICONFILE_BROWSE, &OnBnClickedIconfileBrowse)
END_MESSAGE_MAP()

// CSetDialogs2 message handlers
BOOL CSetDialogs3::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_CHECK_INHERIT_LIMIT);
	AdjustControlSize(IDC_CHECK_INHERIT_BORDER);
	AdjustControlSize(IDC_CHECK_INHERIT_KEYID);
	GITSETTINGS_ADJUSTCONTROLSIZE

	m_tooltips.AddTool(IDC_CHECK_INHERIT_LIMIT, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_BORDER, IDS_SETTINGS_GITCONFIG_INHERIT_TT);
	m_tooltips.AddTool(IDC_CHECK_INHERIT_KEYID, IDS_SETTINGS_GITCONFIG_INHERIT_TT);

	AddTrueFalseToComboBox(m_cWarnNoSignedOffBy);

	m_langCombo.AddString(L"");
	m_langCombo.SetItemData(0, DWORD_PTR(-2));
	m_langCombo.AddString(L"(auto)"); // do not translate, the order matters!
	m_langCombo.SetItemData(1, 0);
	m_langCombo.AddString(L"(disable)"); // do not translate, the order matters!
	m_langCombo.SetItemData(2, DWORD_PTR(-1));
	// fill the combo box with all available languages
	g_langs.clear();
	EnumSystemLocales(EnumLocalesProc, LCID_SUPPORTED);
	for (DWORD langID : g_langs)
		AddLangToCombo(langID);

	InitGitSettings(this, true, &m_tooltips);

	UpdateData(FALSE);
	return TRUE;
}

static void SelectLanguage(CComboBox &combobox, LONG langueage)
{
	for (int i = 0; i < combobox.GetCount(); ++i)
	{
		if (combobox.GetItemData(i) == static_cast<DWORD>(langueage))
		{
			combobox.SetCurSel(i);
			break;
		}
	}
}

void CSetDialogs3::LoadDataImpl(CAutoConfig& config)
{
	{
		CString value;
		if (config.GetString(PROJECTPROPNAME_PROJECTLANGUAGE, value) == GIT_ENOTFOUND && m_iConfigSource != 0)
			m_langCombo.SetCurSel(0);
		else if (value == L"-1")
			m_langCombo.SetCurSel(2);
		else if (!value.IsEmpty())
		{
			LPTSTR strEnd;
			long longValue = wcstol(value, &strEnd, 0);
			if (longValue == 0)
			{
				if (m_iConfigSource == CFG_SRC_EFFECTIVE)
					SelectLanguage(m_langCombo, CRegDWORD(L"Software\\TortoiseGit\\LanguageID", 1033));
				else
					m_langCombo.SetCurSel(1);
			}
			else
				SelectLanguage(m_langCombo, longValue);
		}
		else if (m_iConfigSource == CFG_SRC_EFFECTIVE)
			SelectLanguage(m_langCombo, CRegDWORD(L"Software\\TortoiseGit\\LanguageID", 1033));
		else
			m_langCombo.SetCurSel(1);
	}

	{
		m_LogMinSize.Empty();
		CString value;
		m_bInheritLogMinSize = (config.GetString(PROJECTPROPNAME_LOGMINSIZE, value) == GIT_ENOTFOUND);
		if (!value.IsEmpty() || m_iConfigSource == CFG_SRC_EFFECTIVE)
		{
			int nMinLogSize = _wtoi(value);
			m_LogMinSize.Format(L"%d", nMinLogSize);
			m_bInheritLogMinSize = FALSE;
		}
	}

	{
		m_Border.Empty();
		CString value;
		m_bInheritBorder = (config.GetString(PROJECTPROPNAME_LOGWIDTHLINE, value) == GIT_ENOTFOUND);
		if (!value.IsEmpty() || m_iConfigSource == CFG_SRC_EFFECTIVE)
		{
			int nLogWidthMarker = _wtoi(value);
			m_Border.Format(L"%d", nLogWidthMarker);
			m_bInheritBorder = FALSE;
		}
	}

	GetBoolConfigValueComboBox(config, PROJECTPROPNAME_WARNNOSIGNEDOFFBY, m_cWarnNoSignedOffBy);

	m_bInheritIconFile = (config.GetString(PROJECTPROPNAME_ICON, m_iconFile) == GIT_ENOTFOUND);

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

BOOL CSetDialogs3::SafeDataImpl(CAutoConfig& config)
{
	if (m_langCombo.GetCurSel() == 2) // disable
	{
		if (!Save(config, PROJECTPROPNAME_PROJECTLANGUAGE, L"-1"))
			return FALSE;
	}
	else if (m_langCombo.GetCurSel() == 0) // inherit
	{
		if (!Save(config, PROJECTPROPNAME_PROJECTLANGUAGE, L"", true))
			return FALSE;
	}
	else
	{
		CString value;
		char numBuf[20] = { 0 };
		sprintf_s(numBuf, "%lu", static_cast<DWORD>(m_langCombo.GetItemData(m_langCombo.GetCurSel())));
		if (!Save(config, PROJECTPROPNAME_PROJECTLANGUAGE, static_cast<CString>(numBuf)))
			return FALSE;
	}

	if (!Save(config, PROJECTPROPNAME_LOGMINSIZE, m_LogMinSize, m_bInheritLogMinSize == TRUE))
		return FALSE;

	if (!Save(config, PROJECTPROPNAME_LOGWIDTHLINE, m_Border, m_bInheritBorder == TRUE))
		return FALSE;

	{
		CString value;
		m_cWarnNoSignedOffBy.GetWindowText(value);
		if (!Save(config, PROJECTPROPNAME_WARNNOSIGNEDOFFBY, value, value.IsEmpty()))
			return FALSE;
	}

	if (!Save(config, PROJECTPROPNAME_ICON, m_iconFile, m_bInheritIconFile == TRUE))
		return FALSE;

	return TRUE;
}

void CSetDialogs3::EnDisableControls()
{
	GetDlgItem(IDC_LOGMINSIZE)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_BORDER)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_LANGCOMBO)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_WARN_NO_SIGNED_OFF_BY)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_COMBO_SETTINGS_SAFETO)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_ICONFILE)->SendMessage(EM_SETREADONLY, m_iConfigSource == CFG_SRC_EFFECTIVE, 0);
	GetDlgItem(IDC_ICONFILE_BROWSE)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE && !m_bInheritIconFile);
	GetDlgItem(IDC_CHECK_INHERIT_LIMIT)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_BORDER)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);
	GetDlgItem(IDC_CHECK_INHERIT_ICONPATH)->EnableWindow(m_iConfigSource != CFG_SRC_EFFECTIVE);

	GetDlgItem(IDC_LOGMINSIZE)->EnableWindow(!m_bInheritLogMinSize);
	GetDlgItem(IDC_BORDER)->EnableWindow(!m_bInheritBorder);
	GetDlgItem(IDC_ICONFILE)->EnableWindow(!m_bInheritIconFile);
	UpdateData(FALSE);
}

void CSetDialogs3::OnChange()
{
	UpdateData();
	EnDisableControls();
	m_bNeedSave = true;
	SetModified();
}

void CSetDialogs3::OnBnClickedIconfileBrowse()
{
	UpdateData(TRUE);
	CString currentDir = g_Git.m_CurrentDir + (CStringUtils::EndsWith(g_Git.m_CurrentDir, L'\\') ? L"" : L"\\");
	CString iconFile = m_iconFile;
	if (!(iconFile.Mid(1, 1) == L":" || CStringUtils::StartsWith(iconFile, L"\\")))
		iconFile = currentDir + iconFile;
	iconFile.Replace('/', '\\');
	if (!CAppUtils::FileOpenSave(iconFile, nullptr, 0, IDS_ICONFILEFILTER, true, GetSafeHwnd()))
		return;

	if (CStringUtils::StartsWith(iconFile, currentDir))
		iconFile = iconFile.Mid(currentDir.GetLength());
	iconFile.Replace('\\', '/');
	if (m_iconFile != iconFile)
	{
		m_iconFile = iconFile;
		m_bNeedSave = true;
		SetModified();
	}
	UpdateData(FALSE);
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
	DWORD langID = wcstol(lpLocaleString, nullptr, 16);
	g_langs.push_back(langID);
	return TRUE;
}

void CSetDialogs3::AddLangToCombo(DWORD langID)
{
	TCHAR buf[MAX_PATH] = {0};
	GetLocaleInfo(langID, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
	CString sLang = buf;
	GetLocaleInfo(langID, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
	if (buf[0])
	{
		sLang += L" (";
		sLang += buf;
		sLang += L')';
	}

	int index = m_langCombo.AddString(sLang);
	m_langCombo.SetItemData(index, langID);
}
