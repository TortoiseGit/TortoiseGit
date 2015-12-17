// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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
#include "GitProgressDlg.h"
#include "..\version.h"
#include "Git.h"
#include "MessageBox.h"
#include "GitForWindows.h"
#include "BrowseFolder.h"
#include "Libraries.h"

IMPLEMENT_DYNAMIC(CSetMainPage, ISettingsPropPage)
CSetMainPage::CSetMainPage()
	: ISettingsPropPage(CSetMainPage::IDD)
	, m_bCheckNewer(TRUE)
	, m_dwLanguage(0)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	CString temp = CRegString(REG_MSYSGIT_INSTALL, _T(""), FALSE, HKEY_LOCAL_MACHINE);
	if(!temp.IsEmpty())
		temp+=_T("bin");
	m_regMsysGitPath = CRegString(REG_MSYSGIT_PATH,temp,FALSE);

	m_regMsysGitExtranPath =CRegString(REG_MSYSGIT_EXTRA_PATH);

	m_sMsysGitPath = m_regMsysGitPath;
	m_sMsysGitExtranPath = m_regMsysGitExtranPath;

	m_regCheckNewer = CRegDWORD(_T("Software\\TortoiseGit\\VersionCheck"), TRUE);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	DDX_Text(pDX, IDC_MSYSGIT_PATH, m_sMsysGitPath);
	DDX_Text(pDX, IDC_MSYSGIT_EXTERN_PATH, m_sMsysGitExtranPath);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
}


BEGIN_MESSAGE_MAP(CSetMainPage, ISettingsPropPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnModified)
//	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnModified)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnModified)
	ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
	ON_BN_CLICKED(IDC_MSYSGIT_BROWSE,OnBrowseDir)
	ON_BN_CLICKED(IDC_MSYSGIT_CHECK,OnCheck)
	ON_EN_CHANGE(IDC_MSYSGIT_PATH, OnMsysGitPathModify)
	ON_EN_CHANGE(IDC_MSYSGIT_EXTERN_PATH, OnModified)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_ENV, &CSetMainPage::OnBnClickedButtonShowEnv)
	ON_BN_CLICKED(IDC_CREATELIB, &CSetMainPage::OnBnClickedCreatelib)
END_MESSAGE_MAP()

BOOL CSetMainPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	EnableToolTips();
	AdjustControlSize(IDC_CHECKNEWERVERSION);

	m_dwLanguage = m_regLanguage;
	m_bCheckNewer = m_regCheckNewer;

	m_tooltips.AddTool(IDC_MSYSGIT_PATH,IDS_MSYSGIT_PATH_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	m_tooltips.AddTool(IDC_CREATELIB, IDS_SETTINGS_CREATELIB_TT);

	SHAutoComplete(GetDlgItem(IDC_MSYSGIT_PATH)->m_hWnd, SHACF_FILESYSTEM);

	// set up the language selecting combobox
	TCHAR buf[MAX_PATH] = {0};
	GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
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
			sVer = sVer.Left(sVer.ReverseFind('.'));
			CString sFileVer = CPathUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
			if (sFileVer.Compare(sVer)!=0)
				continue;
			CString sLoc = filename.Mid(12);
			sLoc = sLoc.Left(sLoc.GetLength()-4); // cut off ".dll"
			if ((sLoc.Left(2) == L"32")&&(sLoc.GetLength() > 5))
				continue;
			DWORD loc = _tstoi(filename.Mid(12));
			GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
			CString sLang = buf;
			GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
			if (buf[0])
			{
				sLang += _T(" (");
				sLang += buf;
				sLang += _T(")");
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

void CSetMainPage::OnModified()
{
	SetModified();
}

static void PerformCommonGitPathCleanup(CString &path)
{
	path.Trim(L"\"'");

	if (path.Find(L"%") >= 0)
	{
		int neededSize = ExpandEnvironmentStrings(path, nullptr, 0);
		CString origPath(path);
		ExpandEnvironmentStrings(origPath, path.GetBufferSetLength(neededSize), neededSize);
		path.ReleaseBuffer();
	}

	path.Replace(L"/", L"\\");
	path.Replace(L"\\\\", L"\\");

	if (path.GetLength() > 7 && path.Right(7) == _T("git.exe"))
		path = path.Left(path.GetLength() - 7);

	path.TrimRight(L"\\");

	// prefer git.exe in cmd-directory for Git for Windows based on msys2
	if (path.GetLength() > 12 && (path.Right(12) == _T("\\mingw32\\bin") || path.Right(12) == _T("\\mingw64\\bin")) && PathFileExists(path.Left(path.GetLength() - 12) + _T("\\cmd\\git.exe")))
		path = path.Left(path.GetLength() - 12) + _T("\\cmd");

	// prefer git.exe in bin-directory, see https://github.com/msysgit/msysgit/issues/103
	if (path.GetLength() > 5 && path.Right(4) == _T("\\cmd") && PathFileExists(path.Left(path.GetLength() - 4) + _T("\\bin\\git.exe")))
		path = path.Left(path.GetLength() - 4) + _T("\\bin");
}

void CSetMainPage::OnMsysGitPathModify()
{
	this->UpdateData();
	CString str=this->m_sMsysGitPath;
	if(!str.IsEmpty())
	{
		PerformCommonGitPathCleanup(str);

		if(str.GetLength()>=3 && str.Find(_T("bin"), str.GetLength()-3) >=0)
		{
			str=str.Left(str.GetLength()-3);
			str += _T("mingw\\bin");
			if(::PathFileExists(str))
			{
				str+=_T(";");
				if(this->m_sMsysGitExtranPath.Find(str)<0)
				{
					m_sMsysGitExtranPath = str + m_sMsysGitExtranPath;
					this->UpdateData(FALSE);
				}
			}
		}
	}
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
	g_Git.m_bInitialized = FALSE;
	if (g_Git.CheckMsysGitDir(FALSE))
	{
		SetModified(FALSE);
		return ISettingsPropPage::OnApply();
	}
	else
	{
		if (CMessageBox::Show(NULL, _T("Invalid git.exe path.\nCheck help file for \"Git.exe Path\"."), _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
			OnHelp();
		return 0;
	}
}

void CSetMainPage::OnBnClickedChecknewerbutton()
{
	CAppUtils::RunTortoiseGitProc(_T("/command:updatecheck /visible"), false, false);
}

void CSetMainPage::OnBrowseDir()
{
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString dir;
	this->UpdateData(TRUE);
	dir=this->m_sMsysGitPath;
	if (browseFolder.Show(GetSafeHwnd(), dir) == CBrowseFolder::OK)
	{
		m_sMsysGitPath=dir;
		PerformCommonGitPathCleanup(m_sMsysGitPath);
		this->UpdateData(FALSE);
		OnMsysGitPathModify();
	}
	SetModified(TRUE);
}

void CSetMainPage::OnCheck()
{
	GetDlgItem(IDC_MSYSGIT_VER)->SetWindowText(L"");

	this->UpdateData(TRUE);

	PerformCommonGitPathCleanup(m_sMsysGitPath);
	UpdateData(FALSE);

	OnMsysGitPathModify();

	CString oldpath = m_regMsysGitPath;
	CString oldextranpath = this->m_regMsysGitExtranPath;

	Store(m_sMsysGitPath, m_regMsysGitPath);
	Store(m_sMsysGitExtranPath, m_regMsysGitExtranPath);

	g_Git.m_bInitialized = false;

	if (g_Git.CheckMsysGitDir(FALSE))
	{
		CString cmd;
		CString out;
		cmd=_T("git.exe --version");
		int ret = g_Git.Run(cmd,&out,CP_UTF8);
		this->GetDlgItem(IDC_MSYSGIT_VER)->SetWindowText(out);
		if (out.IsEmpty())
		{
			if (ret == 0xC0000135 && CMessageBox::Show(NULL, _T("Could not start git.exe. A dynamic library (dll) is missing.\nCheck help file for \"Extern DLL Path\"."), _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
				OnHelp();
			else if (CMessageBox::Show(NULL, _T("Could not get read version information from git.exe.\nCheck help file for \"Git.exe Path\"."),_T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
				OnHelp();
		}
		else if (!(CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) && out.Find(_T("msysgit")) == -1 && out.Find(_T("windows")) == -1 && CMessageBox::Show(NULL, _T("Could not find \"msysgit\" or \"windows\" in versionstring of git.exe.\nIf you are using git of the cygwin or msys2 environment please read the help file for the keyword \"cygwin git\" or \"msys2 git\"."), _T("TortoiseGit"), 1, IDI_INFORMATION, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
			OnHelp();
	}
	else
	{
		if (CMessageBox::Show(NULL, _T("Invalid git.exe path.\nCheck help file for \"Git.exe Path\"."), _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)), CString(MAKEINTRESOURCE(IDS_MSGBOX_HELP))) == 2)
			OnHelp();
	}

	Store(oldpath, m_regMsysGitPath);
	Store(oldextranpath, m_regMsysGitExtranPath);
}

void CSetMainPage::OnBnClickedButtonShowEnv()
{
	CString cmd, err;
	CString tempfile=::GetTempFile();

	cmd=_T("cmd /c set");
	if (g_Git.RunLogFile(cmd, tempfile, &err))
	{
		CMessageBox::Show(GetSafeHwnd(), _T("Could not get environment variables:\n") + err, _T("TortoiseGit"), MB_OK);
		return;
	}
	CAppUtils::LaunchAlternativeEditor(tempfile);
}

void CSetMainPage::OnBnClickedCreatelib()
{
	CoInitialize(NULL);
	EnsureGitLibrary();
	CoUninitialize();
}
