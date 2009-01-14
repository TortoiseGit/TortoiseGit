// TortoiseSVN - a Windows shell extension for easy version control

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
#include "SVNProgressDlg.h"
#include "..\version.h"
#include ".\setmainpage.h"
#include "Git.h"
#include "MessageBox.h"
#include "CommonResource.h"

IMPLEMENT_DYNAMIC(CSetMainPage, ISettingsPropPage)
CSetMainPage::CSetMainPage()
	: ISettingsPropPage(CSetMainPage::IDD)
	, m_sTempExtensions(_T(""))
	, m_bCheckNewer(TRUE)
	, m_bLastCommitTime(FALSE)
	, m_bUseDotNetHack(FALSE)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
//	CString temp(SVN_CONFIG_DEFAULT_GLOBAL_IGNORES);
//	m_regExtensions = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores"), temp);
	m_regCheckNewer = CRegDWORD(_T("Software\\TortoiseGit\\CheckNewer"), TRUE);
	m_regLastCommitTime = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times"), _T(""));
	if ((GetEnvironmentVariable(_T("SVN_ASP_DOT_NET_HACK"), NULL, 0)==0)&&(GetLastError()==ERROR_ENVVAR_NOT_FOUND))
		m_bUseDotNetHack = false;
	else
		m_bUseDotNetHack = true;
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
	DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
	DDX_Check(pDX, IDC_ASPDOTNETHACK, m_bUseDotNetHack);
}


BEGIN_MESSAGE_MAP(CSetMainPage, ISettingsPropPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnModified)
	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnModified)
	ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnModified)
	ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
	ON_BN_CLICKED(IDC_COMMITFILETIMES, OnModified)
	ON_BN_CLICKED(IDC_SOUNDS, OnBnClickedSounds)
	ON_BN_CLICKED(IDC_ASPDOTNETHACK, OnASPHACK)
END_MESSAGE_MAP()

BOOL CSetMainPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	EnableToolTips();

	m_sTempExtensions = m_regExtensions;
	m_dwLanguage = m_regLanguage;
	m_bCheckNewer = m_regCheckNewer;

	CString temp;
	temp = m_regLastCommitTime;
	m_bLastCommitTime = (temp.CompareNoCase(_T("yes"))==0);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	m_tooltips.AddTool(IDC_COMMITFILETIMES, IDS_SETTINGS_COMMITFILETIMES_TT);
	m_tooltips.AddTool(IDC_ASPDOTNETHACK, IDS_SETTINGS_DOTNETHACK_TT);

	// set up the language selecting combobox
	TCHAR buf[MAX_PATH];
	GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
	m_LanguageCombo.AddString(buf);
	m_LanguageCombo.SetItemData(0, 1033);
	CString path = CPathUtils::GetAppParentDirectory();
	path = path + _T("Languages\\");
	CSimpleFileFind finder(path, _T("*.dll"));
	int langcount = 1;
	while (finder.FindNextFileNoDirectories())
	{
		CString file = finder.GetFilePath();
		CString filename = finder.GetFileName();
		if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0)
		{
			CString sVer = _T(STRPRODUCTVER);
			sVer = sVer.Left(sVer.ReverseFind(','));
			CString sFileVer = CPathUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind(','));
			if (sFileVer.Compare(sVer)!=0)
				continue;
			DWORD loc = _tstoi(filename.Mid(12));
			TCHAR buf[MAX_PATH];
			GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
			m_LanguageCombo.AddString(buf);
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

BOOL CSetMainPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetMainPage::OnModified()
{
	SetModified();
}

void CSetMainPage::OnASPHACK()
{
	if (CMessageBox::Show(m_hWnd, IDS_SETTINGS_ASPHACKWARNING, IDS_APPNAME, MB_ICONWARNING|MB_YESNO) == IDYES)
	{
		SetModified();
	}
	else
	{
		UpdateData();
		m_bUseDotNetHack = !m_bUseDotNetHack;
		UpdateData(FALSE);
	}
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();
	Store (m_dwLanguage, m_regLanguage);
	if (m_sTempExtensions.Compare(CString(m_regExtensions)))
	{
		Store (m_sTempExtensions, m_regExtensions);
		m_restart = Restart_Cache;
	}
	Store (m_bCheckNewer, m_regCheckNewer);
	Store ((m_bLastCommitTime ? _T("yes") : _T("no")), m_regLastCommitTime);

	CRegString asphack_local(_T("System\\CurrentControlSet\\Control\\Session Manager\\Environment\\SVN_ASP_DOT_NET_HACK"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
	CRegString asphack_user(_T("Environment\\SVN_ASP_DOT_NET_HACK"));
	if (m_bUseDotNetHack)
	{
		asphack_local = _T("*");
		if (asphack_local.LastError)
			asphack_user = _T("*");
		if ((GetEnvironmentVariable(_T("SVN_ASP_DOT_NET_HACK"), NULL, 0)==0)&&(GetLastError()==ERROR_ENVVAR_NOT_FOUND))
		{
			DWORD_PTR dwRet = 0;
			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)_T("Environment"), SMTO_ABORTIFHUNG, 1000, &dwRet);
			m_restart = Restart_System;
		}
	}
	else
	{
		asphack_local.removeValue();
		asphack_user.removeValue();
		if (GetEnvironmentVariable(_T("SVN_ASP_DOT_NET_HACK"), NULL, 0)!=0)
		{
			DWORD_PTR dwRet = 0;
			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)_T("Environment"), SMTO_ABORTIFHUNG, 1000, &dwRet);
			m_restart = Restart_System;
		}
	}
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetMainPage::OnBnClickedEditconfig()
{
#if 0
	TCHAR buf[MAX_PATH];
	SVN::EnsureConfigFile();
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	CString path = buf;
	path += _T("\\Subversion\\config");
	CAppUtils::StartTextViewer(path);
#endif
}

void CSetMainPage::OnBnClickedChecknewerbutton()
{
	TCHAR com[MAX_PATH+100];
	GetModuleFileName(NULL, com, MAX_PATH);
	_tcscat_s(com, MAX_PATH+100, _T(" /command:updatecheck /visible"));

	CAppUtils::LaunchApplication(com, 0, false);
}

void CSetMainPage::OnBnClickedSounds()
{
	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	
	if (fullver >= 0x0600)
		CAppUtils::LaunchApplication(_T("RUNDLL32 Shell32,Control_RunDLL mmsys.cpl,,2"), NULL, false);
	else
		CAppUtils::LaunchApplication(_T("RUNDLL32 Shell32,Control_RunDLL mmsys.cpl,,1"), NULL, false);
}







