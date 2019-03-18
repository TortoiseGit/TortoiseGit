// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "../version.h"
#include "Git.h"
#include "MessageBox.h"
#include "GitForWindows.h"
#include "Libraries.h"

IMPLEMENT_DYNAMIC(CSetMainPage, ISettingsPropPage)
CSetMainPage::CSetMainPage()
	: ISettingsPropPage(CSetMainPage::IDD)
	, m_bCheckNewer(TRUE)
	, m_dwLanguage(0)
{
	m_regLanguage = CRegDWORD(L"Software\\TortoiseGit\\LanguageID", 1033);

	m_regMsysGitPath = CRegString(REG_MSYSGIT_PATH);
	m_regMsysGitExtranPath =CRegString(REG_MSYSGIT_EXTRA_PATH);

	m_sMsysGitPath = m_regMsysGitPath;
	m_sMsysGitExtranPath = m_regMsysGitExtranPath;

	m_regCheckNewer = CRegDWORD(L"Software\\TortoiseGit\\VersionCheck", TRUE);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	m_dwLanguage = static_cast<DWORD>(m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel()));
	DDX_Text(pDX, IDC_MSYSGIT_PATH, m_sMsysGitPath);
	DDX_Text(pDX, IDC_MSYSGIT_EXTERN_PATH, m_sMsysGitExtranPath);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
}


BEGIN_MESSAGE_MAP(CSetMainPage, ISettingsPropPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnModified)
//	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnModified)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnClickVersioncheck)
	ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
	ON_BN_CLICKED(IDC_MSYSGIT_BROWSE,OnBrowseDir)
	ON_BN_CLICKED(IDC_MSYSGIT_CHECK,OnCheck)
	ON_EN_CHANGE(IDC_MSYSGIT_PATH, OnMsysGitPathModify)
	ON_EN_CHANGE(IDC_MSYSGIT_EXTERN_PATH, OnModified)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_ENV, &CSetMainPage::OnBnClickedButtonShowEnv)
	ON_BN_CLICKED(IDC_CREATELIB, &CSetMainPage::OnBnClickedCreatelib)
	ON_BN_CLICKED(IDC_RUNFIRSTSTARTWIZARD, &CSetMainPage::OnBnClickedRunfirststartwizard)
END_MESSAGE_MAP()

BOOL CSetMainPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	EnableToolTips();
	AdjustControlSize(IDC_CHECKNEWERVERSION);

	m_dwLanguage = m_regLanguage;
	m_bCheckNewer = m_regCheckNewer;

	m_tooltips.AddTool(IDC_MSYSGIT_PATH,IDS_MSYSGIT_PATH_TT);
	m_tooltips.AddTool(IDC_MSYSGIT_EXTERN_PATH, IDS_EXTRAPATH_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	m_tooltips.AddTool(IDC_CREATELIB, IDS_SETTINGS_CREATELIB_TT);

	SHAutoComplete(GetDlgItem(IDC_MSYSGIT_PATH)->m_hWnd, SHACF_FILESYSTEM);

	// set up the language selecting combobox
	TCHAR buf[MAX_PATH] = {0};
	GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
	m_LanguageCombo.AddString(buf);
	m_LanguageCombo.SetItemData(0, 1033);
	CString path = CPathUtils::GetAppParentDirectory();
	path = path + L"Languages\\";
	CSimpleFileFind finder(path, L"*.dll");
	int langcount = 1;
	while (finder.FindNextFileNoDirectories())
	{
		CString file = finder.GetFilePath();
		CString filename = finder.GetFileName();
		if (CStringUtils::StartsWithI(filename, L"TortoiseProc"))
		{
			CString sVer = _T(STRPRODUCTVER);
			sVer = sVer.Left(sVer.ReverseFind('.'));
			CString sFileVer = CPathUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
			if (sFileVer.Compare(sVer)!=0)
				continue;
			CString sLoc = filename.Mid(static_cast<int>(wcslen(L"TortoiseProc")));
			sLoc = sLoc.Left(sLoc.GetLength() - static_cast<int>(wcslen(L".dll"))); // cut off ".dll"
			if (CStringUtils::StartsWith(sLoc, L"32") && (sLoc.GetLength() > 5))
				continue;
			DWORD loc = _wtoi(filename.Mid(static_cast<int>(wcslen(L"TortoiseProc"))));
			if (loc == 0)
				continue;
			buf[0] = 0;
			GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
			CString sLang = buf;
			buf[0] = 0;
			GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
			if (buf[0])
			{
				sLang += L" (";
				sLang += buf;
				sLang += L')';
			}
			m_LanguageCombo.AddString(sLang);
			m_LanguageCombo.SetItemData(langcount++, loc);
		}
	}

	for (int i=0; i<m_LanguageCombo.GetCount(); i++)
	{
		if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
			m_LanguageCombo.SetCurSel(i);
	}

	UpdateData(FALSE);
	return TRUE;
}

void CSetMainPage::OnClickVersioncheck()
{
	if (m_bCheckNewer && CMessageBox::Show(GetSafeHwnd(), IDS_DISABLEUPDATECHECKS, IDS_APPNAME, 2, IDI_QUESTION, IDS_DISABLEUPDATECHECKSBUTTON, IDS_ABORTBUTTON) != 1)
		return;
	m_bCheckNewer = !m_bCheckNewer;
	UpdateData(FALSE);
	SetModified();
}

void CSetMainPage::OnModified()
{
	SetModified();
}

void CSetMainPage::OnMsysGitPathModify()
{
	this->UpdateData();
	if (GuessExtraPath(m_sMsysGitPath, m_sMsysGitExtranPath))
		UpdateData(FALSE);
	SetModified();
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();

	PerformCommonGitPathCleanup(m_sMsysGitPath);
	UpdateData(FALSE);

	Store(m_dwLanguage, m_regLanguage);
	if (m_sMsysGitPath.Compare(CString(m_regMsysGitPath)) ||
		this->m_sMsysGitExtranPath.Compare(CString(m_regMsysGitExtranPath)))
	{
		Store(m_sMsysGitPath, m_regMsysGitPath);
		Store(m_sMsysGitExtranPath, m_regMsysGitExtranPath);
		m_restart = Restart_Cache;
	}
	Store(m_bCheckNewer, m_regCheckNewer);

	// only complete if the msysgit directory is ok
	if (!CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); }))
		return 0;

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetMainPage::OnBnClickedChecknewerbutton()
{
	CAppUtils::RunTortoiseGitProc(L"/command:updatecheck /visible", false, false);
}

void CSetMainPage::OnBrowseDir()
{
	UpdateData(TRUE);

	if (!SelectFolder(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath))
		return;

	UpdateData(FALSE);
	SetModified(TRUE);
}

void CSetMainPage::OnCheck()
{
	UpdateData(TRUE);

	CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); });

	UpdateData(FALSE);
}

void CSetMainPage::OnBnClickedButtonShowEnv()
{
	CString err;
	CString tempfile=::GetTempFile();

	CString cmd = L"cmd /c set";
	if (g_Git.RunLogFile(cmd, tempfile, &err))
	{
		CMessageBox::Show(GetSafeHwnd(), L"Could not get environment variables:\n" + err, L"TortoiseGit", MB_OK);
		return;
	}
	::SetFileAttributes(tempfile, FILE_ATTRIBUTE_READONLY);
	CAppUtils::LaunchAlternativeEditor(tempfile);
}

void CSetMainPage::OnBnClickedCreatelib()
{
	CoInitialize(nullptr);
	EnsureGitLibrary();
	CoUninitialize();
}

void CSetMainPage::OnBnClickedRunfirststartwizard()
{
	CAppUtils::RunTortoiseGitProc(L"/command:firststart", false, false);
}
