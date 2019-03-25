// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2019 - TortoiseGit
// Copyright (C) 2003-2008,2014 - TortoiseSVN

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
#include "registry.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "SetSavedDataPage.h"
#include "MessageBox.h"
#include "StringUtils.h"
#include "Git.h"

IMPLEMENT_DYNAMIC(CSetSavedDataPage, ISettingsPropPage)

CSetSavedDataPage::CSetSavedDataPage()
	: ISettingsPropPage(CSetSavedDataPage::IDD)
	, m_maxLines(0)
{
	m_regMaxLines = CRegDWORD(L"Software\\TortoiseGit\\MaxLinesInLogfile", 4000);
	m_maxLines = m_regMaxLines;
}

CSetSavedDataPage::~CSetSavedDataPage()
{
}

void CSetSavedDataPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLHISTCLEAR, m_btnUrlHistClear);
	DDX_Control(pDX, IDC_LOGHISTCLEAR, m_btnLogHistClear);
	DDX_Control(pDX, IDC_RESIZABLEHISTCLEAR, m_btnResizableHistClear);
	DDX_Control(pDX, IDC_AUTHHISTCLEAR, m_btnAuthHistClear);
	DDX_Control(pDX, IDC_REPOLOGCLEAR, m_btnRepoLogClear);
	DDX_Text(pDX, IDC_MAXLINES, m_maxLines);
	DDX_Control(pDX, IDC_ACTIONLOGSHOW, m_btnActionLogShow);
	DDX_Control(pDX, IDC_ACTIONLOGCLEAR, m_btnActionLogClear);
}

BOOL CSetSavedDataPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	// find out how many log messages and URLs we've stored
	int nLogHistWC = 0;
	INT_PTR nLogHistMsg = 0;
	int nUrlHistWC = 0;
	INT_PTR nUrlHistItems = 0;
	int nLogHistRepo = 0;
	CRegistryKey regloghist(L"Software\\TortoiseGit\\History");
	CStringList loghistlist;
	regloghist.getSubKeys(loghistlist);
	for (POSITION pos = loghistlist.GetHeadPosition(); pos; )
	{
		CString sHistName = loghistlist.GetNext(pos);
		if (CStringUtils::StartsWithI(sHistName, L"commit") || CStringUtils::StartsWithI(sHistName, L"merge"))
		{
			nLogHistWC++;
			CRegistryKey regloghistwc(L"Software\\TortoiseGit\\History\\"+sHistName);
			CStringList loghistlistwc;
			regloghistwc.getValues(loghistlistwc);
			nLogHistMsg += loghistlistwc.GetCount();
		}
		else
		{
			// repoURLs
			CStringList urlhistlistmain;
			CStringList urlhistlistmainvalues;
			CRegistryKey regurlhistlist(L"Software\\TortoiseGit\\History\\repoURLS");
			regurlhistlist.getSubKeys(urlhistlistmain);
			regurlhistlist.getValues(urlhistlistmainvalues);
			nUrlHistItems += urlhistlistmainvalues.GetCount();
			for (POSITION urlpos = urlhistlistmain.GetHeadPosition(); urlpos; )
			{
				CString sWCUID = urlhistlistmain.GetNext(urlpos);
				nUrlHistWC++;
				CStringList urlhistlistwc;
				CRegistryKey regurlhistlistwc(L"Software\\TortoiseGit\\History\\repoURLS\\"+sWCUID);
				regurlhistlistwc.getValues(urlhistlistwc);
				nUrlHistItems += urlhistlistwc.GetCount();
			}
		}
	}

	// find out how many dialog sizes / positions we've stored
	INT_PTR nResizableDialogs = 0;
	CRegistryKey regResizable(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState");
	CStringList resizablelist;
	regResizable.getValues(resizablelist);
	nResizableDialogs += resizablelist.GetCount();

	// find out how many auth data we've stored
	int nSimple = 0;
	int nSSL = 0;
	int nUsername = 0;

	CString sFile;
	bool bIsDir = false;

	if (CComHeapPtr<WCHAR> pszPath; SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
	{
		CString path = pszPath;
		path += L"\\Subversion\\auth\\";

		CString sSimple = path + L"svn.simple";
		CString sSSL = path + L"svn.ssl.server";
		CString sUsername = path + L"svn.username";
		CDirFileEnum simpleenum(sSimple);
		while (simpleenum.NextFile(sFile, &bIsDir))
			nSimple++;
		CDirFileEnum sslenum(sSSL);
		while (sslenum.NextFile(sFile, &bIsDir))
			nSSL++;
		CDirFileEnum userenum(sUsername);
		while (userenum.NextFile(sFile, &bIsDir))
			nUsername++;
	}

	CDirFileEnum logenum(CPathUtils::GetAppDataDirectory() + L"logcache");
	while (logenum.NextFile(sFile, &bIsDir))
		nLogHistRepo++;
	// the "Repositories.dat" is not a cache file
	nLogHistRepo--;

	BOOL bActionLog = PathFileExists(CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt");

	m_btnLogHistClear.EnableWindow(nLogHistMsg || nLogHistWC);
	m_btnUrlHistClear.EnableWindow(nUrlHistItems || nUrlHistWC);
	m_btnResizableHistClear.EnableWindow(nResizableDialogs > 0);
	m_btnAuthHistClear.EnableWindow(nSimple || nSSL || nUsername);
	m_btnRepoLogClear.EnableWindow(nLogHistRepo >= 0);
	m_btnActionLogClear.EnableWindow(bActionLog);
	m_btnActionLogShow.EnableWindow(bActionLog);

	EnableToolTips();

	CString sTT;
	sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_LOGHIST_TT, nLogHistMsg, nLogHistWC);
	m_tooltips.AddTool(IDC_LOGHISTORY, sTT);
	m_tooltips.AddTool(IDC_LOGHISTCLEAR, sTT);
	sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_URLHIST_TT, nUrlHistItems, nUrlHistWC);
	m_tooltips.AddTool(IDC_URLHISTORY, sTT);
	m_tooltips.AddTool(IDC_URLHISTCLEAR, sTT);
	sTT.Format(IDS_SETTINGS_SAVEDDATA_RESIZABLE_TT, nResizableDialogs);
	m_tooltips.AddTool(IDC_RESIZABLEHISTORY, sTT);
	m_tooltips.AddTool(IDC_RESIZABLEHISTCLEAR, sTT);
	sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_AUTH_TT, nSimple, nSSL, nUsername);
	m_tooltips.AddTool(IDC_AUTHHISTORY, sTT);
	m_tooltips.AddTool(IDC_AUTHHISTCLEAR, sTT);
	sTT.Format(IDS_SETTINGS_SAVEDDATA_REPOLOGHIST_TT, nLogHistRepo);
	m_tooltips.AddTool(IDC_REPOLOG, sTT);
	m_tooltips.AddTool(IDC_REPOLOGCLEAR, sTT);
	sTT.LoadString(IDS_SETTINGS_SHOWACTIONLOG_TT);
	m_tooltips.AddTool(IDC_ACTIONLOGSHOW, sTT);
	sTT.LoadString(IDS_SETTINGS_MAXACTIONLOGLINES_TT);
	m_tooltips.AddTool(IDC_MAXLINES, sTT);
	sTT.LoadString(IDS_SETTINGS_CLEARACTIONLOG_TT);
	m_tooltips.AddTool(IDC_ACTIONLOGCLEAR, sTT);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CSetSavedDataPage, ISettingsPropPage)
	ON_BN_CLICKED(IDC_URLHISTCLEAR, &CSetSavedDataPage::OnBnClickedUrlhistclear)
	ON_BN_CLICKED(IDC_LOGHISTCLEAR, &CSetSavedDataPage::OnBnClickedLoghistclear)
	ON_BN_CLICKED(IDC_RESIZABLEHISTCLEAR, &CSetSavedDataPage::OnBnClickedResizablehistclear)
	ON_BN_CLICKED(IDC_AUTHHISTCLEAR, &CSetSavedDataPage::OnBnClickedAuthhistclear)
	ON_BN_CLICKED(IDC_REPOLOGCLEAR, &CSetSavedDataPage::OnBnClickedRepologclear)
	ON_BN_CLICKED(IDC_ACTIONLOGSHOW, &CSetSavedDataPage::OnBnClickedActionlogshow)
	ON_BN_CLICKED(IDC_ACTIONLOGCLEAR, &CSetSavedDataPage::OnBnClickedActionlogclear)
	ON_BN_CLICKED(IDC_TEMPFILESCLEAR, &CSetSavedDataPage::OnBnClickedTempfileclear)
	ON_EN_CHANGE(IDC_MAXLINES, OnModified)
	ON_BN_CLICKED(IDC_STOREDDECISIONSCLEAR, &CSetSavedDataPage::OnBnClickedStoreddecisionsclear)
END_MESSAGE_MAP()

void CSetSavedDataPage::OnBnClickedUrlhistclear()
{
	CRegistryKey reg(L"Software\\TortoiseGit\\History\\repoURLS");
	reg.removeKey();
	m_btnUrlHistClear.EnableWindow(FALSE);
	m_tooltips.DelTool(GetDlgItem(IDC_URLHISTCLEAR));
	m_tooltips.DelTool(GetDlgItem(IDC_URLHISTORY));
}

void CSetSavedDataPage::OnBnClickedLoghistclear()
{
	CRegistryKey reg(L"Software\\TortoiseGit\\History");
	CStringList histlist;
	reg.getSubKeys(histlist);
	for (POSITION pos = histlist.GetHeadPosition(); pos; )
	{
		CString sHist = histlist.GetNext(pos);
		if (CStringUtils::StartsWithI(sHist, L"commit"))
		{
			CRegistryKey regkey(L"Software\\TortoiseGit\\History\\"+sHist);
			regkey.removeKey();
		}
	}

	m_btnLogHistClear.EnableWindow(FALSE);
	m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
	m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedResizablehistclear()
{
	CRegistryKey reg(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState");
	reg.removeKey();
	m_btnResizableHistClear.EnableWindow(FALSE);
	m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
	m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedAuthhistclear()
{
	CRegStdString auth = CRegStdString(L"Software\\TortoiseGit\\Auth\\");
	auth.removeKey();
	if (CComHeapPtr<WCHAR> pszPath; SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
	{
		CString path = pszPath;
		path += L"\\Subversion\\auth\\";
		DeleteViaShell(path, IDS_SETTINGS_DELFILE);
	}
	m_btnAuthHistClear.EnableWindow(FALSE);
	m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTCLEAR));
	m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTORY));
}

void CSetSavedDataPage::OnBnClickedRepologclear()
{
	DeleteViaShell(CPathUtils::GetAppDataDirectory() + L"logcache", IDS_SETTINGS_DELCACHE);

	m_btnRepoLogClear.EnableWindow(FALSE);
	m_tooltips.DelTool(GetDlgItem(IDC_REPOLOG));
	m_tooltips.DelTool(GetDlgItem(IDC_REPOLOGCLEAR));
}

void CSetSavedDataPage::OnBnClickedActionlogshow()
{
	CString logfile = CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt";
	CAppUtils::StartTextViewer(logfile);
}

void CSetSavedDataPage::OnBnClickedActionlogclear()
{
	CString logfile = CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt";
	DeleteFile(logfile);
	m_btnActionLogClear.EnableWindow(FALSE);
	m_btnActionLogShow.EnableWindow(FALSE);
}

void CSetSavedDataPage::OnBnClickedTempfileclear()
{
	if (CMessageBox::Show(m_hWnd, IDS_PROC_WARNCLEARTEMP, IDS_APPNAME, 1, IDI_QUESTION, IDS_ABORTBUTTON, IDS_PROCEEDBUTTON) == 1)
		return;

	int count = 0;
	DWORD len = GetTortoiseGitTempPath(0, nullptr);
	auto path = std::make_unique<TCHAR[]>(len + 100);
	len = GetTortoiseGitTempPath(len + 100, path.get());
	if (len != 0)
	{
		int lastcount;
		do
		{
			lastcount = count;
			count = 0;
			CDirFileEnum finder(path.get());
			bool isDir;
			CString filepath;
			while (finder.NextFile(filepath, &isDir))
			{
				::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
				if (isDir)
				{
					if (!::RemoveDirectory(filepath))
						count++;
				}
				else
				{
					if (!::DeleteFile(filepath))
						count++;
				}
			}
		} while (lastcount != count);
	}

	if (count == 0)
		GetDlgItem(IDC_TEMPFILESCLEAR)->EnableWindow(FALSE);
}

void CSetSavedDataPage::OnModified()
{
	SetModified();
}

BOOL CSetSavedDataPage::OnApply()
{
	Store(m_maxLines, m_regMaxLines);
	return ISettingsPropPage::OnApply();
}

void CSetSavedDataPage::DeleteViaShell(LPCTSTR path, UINT progressText)
{
	CString p(path);
	p += L"||";
	int len = p.GetLength();
	auto buf = std::make_unique<TCHAR[]>(len + 2);
	wcscpy_s(buf.get(), len + 2, p);
	CStringUtils::PipesToNulls(buf.get(), len);

	CString progText(MAKEINTRESOURCE(progressText));
	SHFILEOPSTRUCT fileop;
	fileop.hwnd = m_hWnd;
	fileop.wFunc = FO_DELETE;
	fileop.pFrom = buf.get();
	fileop.pTo = nullptr;
	fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;
	fileop.lpszProgressTitle = progText;
	SHFileOperation(&fileop);
}

void CSetSavedDataPage::OnBnClickedStoreddecisionsclear()
{
	static const CString tgitvalues[] = {
		L"OldMsysgitVersionWarning",
		L"OpenRebaseRemoteBranchEqualsHEAD",
		L"OpenRebaseRemoteBranchUnchanged",
		L"OpenRebaseRemoteBranchFastForwards",
		L"DaemonNoSecurityWarning",
		L"NothingToCommitShowUnversioned",
		L"NoJumpNotFoundWarning",
		L"HintHierarchicalConfig",
		L"TagOptNoTagsWarning",
		L"NoStashIncludeUntrackedWarning",
		L"CommitMergeHint",
		L"AskSetTrackedBranch",
		L"StashPopShowChanges",
		L"StashPopShowConflictChanges",
		L"CommitWarnOnUnresolved",
		L"CommitAskBeforeCancel",
		L"PushAllBranches",
		L"CommitMessageContainsConflictHint",
		L"MergeConflictsNeedsCommit",
	};
	for (const auto& value : tgitvalues)
	{
		CRegDWORD regkey(L"Software\\TortoiseGit\\" + value);
		regkey.removeValue();
	}

	static const CString tmergevalues[] = {
		L"DeleteFileWhenEmpty",
	};
	for (const auto& value : tmergevalues)
	{
		CRegDWORD regkey(L"Software\\TortoiseGitMerge\\" + value);
		regkey.removeValue();
	}

	// remove stored hook decisions
	CRegistryKey reg(L"Software\\TortoiseGit\\approvedhooks");
	reg.removeKey();
}
