// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2016 - TortoiseGit

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
#include "Git.h"
#include "LoglistCommonResource.h"
#include "..\version.h"
#include "MessageBox.h"
#include "CheckForUpdatesDlg.h"
#include "registry.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SmartHandle.h"
#include "SysInfo.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "UnicodeUtils.h"
#include "UpdateCrypto.h"
#include <MsiDefs.h>
#include <MsiQuery.h>

#pragma comment(lib, "msi.lib")

#define SIGNATURE_FILE_ENDING _T(".rsa.asc")

#define WM_USER_DISPLAYSTATUS	(WM_USER + 1)
#define WM_USER_ENDDOWNLOAD		(WM_USER + 2)
#define WM_USER_FILLCHANGELOG	(WM_USER + 3)

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CResizableStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
	, m_bShowInfo(FALSE)
	, m_bForce(FALSE)
	, m_bVisible(FALSE)
	, m_pDownloadThread(nullptr)
	, m_bThreadRunning(FALSE)
	, m_updateDownloader(nullptr)
{
	m_sUpdateDownloadLink = _T("https://tortoisegit.org/download");
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LINK, m_link);
	DDX_Control(pDX, IDC_PROGRESSBAR, m_progress);
	DDX_Control(pDX, IDC_LIST_DOWNLOADS, m_ctrlFiles);
	DDX_Control(pDX, IDC_BUTTON_UPDATE, m_ctrlUpdate);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
}

BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CResizableStandAloneDialog)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnBnClickedButtonUpdate)
	ON_MESSAGE(WM_USER_DISPLAYSTATUS, OnDisplayStatus)
	ON_MESSAGE(WM_USER_ENDDOWNLOAD, OnEndDownload)
	ON_MESSAGE(WM_USER_FILLCHANGELOG, OnFillChangelog)
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarBtnCreated)
	ON_BN_CLICKED(IDC_DONOTASKAGAIN, &CCheckForUpdatesDlg::OnBnClickedDonotaskagain)
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
	SetDlgItemText(IDC_YOURVERSION, temp);

	DialogEnableWindow(IDOK, FALSE);

	m_pTaskbarList.Release();
	if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
		m_pTaskbarList = nullptr;

	// hide download controls
	m_ctrlFiles.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_GROUP_DOWNLOADS)->ShowWindow(SW_HIDE);
	RECT rectWindow, rectGroupDownloads, rectOKButton;
	GetWindowRect(&rectWindow);
	GetDlgItem(IDC_GROUP_DOWNLOADS)->GetWindowRect(&rectGroupDownloads);
	GetDlgItem(IDOK)->GetWindowRect(&rectOKButton);
	LONG bottomDistance = rectWindow.bottom - rectOKButton.bottom;
	OffsetRect(&rectOKButton, 0, rectGroupDownloads.top - rectOKButton.top);
	rectWindow.bottom = rectOKButton.bottom + bottomDistance;
	SetMinTrackSize(CSize(rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top));
	MoveWindow(&rectWindow);
	::MapWindowPoints(nullptr, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
	GetDlgItem(IDOK)->MoveWindow(&rectOKButton);

	temp.LoadString(IDS_STATUSLIST_COLFILE);
	m_ctrlFiles.InsertColumn(0, temp, 0, -1);
	m_ctrlFiles.InsertColumn(1, temp, 0, -1);
	m_ctrlFiles.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
	m_ctrlFiles.SetColumnWidth(0, 350);
	m_ctrlFiles.SetColumnWidth(1, 200);

	m_cLogMessage.Init(-1);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));
	m_cLogMessage.Call(SCI_SETREADONLY, TRUE);

	m_updateDownloader = new CUpdateDownloader(GetSafeHwnd(), m_bForce == TRUE, WM_USER_DISPLAYSTATUS, &m_eventStop);

	if (!AfxBeginThread(CheckThreadEntry, this))
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	AddAnchor(IDC_YOURVERSION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CURRENTVERSION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHECKRESULT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LINK, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_CHANGELOG, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP_DOWNLOADS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LIST_DOWNLOADS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_UPDATE, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);

	SetTimer(100, 1000, nullptr);
	return TRUE;
}

void CCheckForUpdatesDlg::OnDestroy()
{
	for (int i = 0; i < m_ctrlFiles.GetItemCount(); ++i)
		delete (CUpdateListCtrl::Entry *)m_ctrlFiles.GetItemData(i);

	delete m_updateDownloader;

	CResizableStandAloneDialog::OnDestroy();
}

void CCheckForUpdatesDlg::OnOK()
{
	if (m_bThreadRunning || m_pDownloadThread)
		return; // Don't exit while downloading

	CResizableStandAloneDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
	if (m_bThreadRunning || m_pDownloadThread)
		return; // Don't exit while downloading
	CResizableStandAloneDialog::OnCancel();
}

UINT CCheckForUpdatesDlg::CheckThreadEntry(LPVOID pVoid)
{
	return reinterpret_cast<CCheckForUpdatesDlg*>(pVoid)->CheckThread();
}

UINT CCheckForUpdatesDlg::CheckThread()
{
	m_bThreadRunning = TRUE;

	CString temp;
	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();

	bool official = false;

	CRegString checkurluser = CRegString(_T("Software\\TortoiseGit\\UpdateCheckURL"), _T(""));
	CRegString checkurlmachine = CRegString(_T("Software\\TortoiseGit\\UpdateCheckURL"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString sCheckURL = checkurluser;
	if (sCheckURL.IsEmpty())
	{
		sCheckURL = checkurlmachine;
		if (sCheckURL.IsEmpty())
		{
			official = true;
			bool checkPreview = false;
#if PREVIEW
			checkPreview = true;
#else
			CRegStdDWORD regCheckPreview(L"Software\\TortoiseGit\\VersionCheckPreview", FALSE);
			if (DWORD(regCheckPreview))
				checkPreview = true;
#endif
			if (checkPreview)
			{
				sCheckURL = _T("https://versioncheck.tortoisegit.org/version-preview.txt");
				SetDlgItemText(IDC_SOURCE, _T("Using preview release channel"));
			}
			else
				sCheckURL = _T("https://versioncheck.tortoisegit.org/version.txt");
		}
	}

	if (!official && sCheckURL.Find(_T("://versioncheck.tortoisegit.org/")) > 0)
		official = true;

	if (!official)
		SetDlgItemText(IDC_SOURCE, _T("Using (unofficial) release channel: ") + sCheckURL);

	CString ver;
	CAutoConfig versioncheck(true);
	CString errorText;
	DWORD ret = m_updateDownloader->DownloadFile(sCheckURL, tempfile, false);
	if (!ret && official)
	{
		CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		ret = m_updateDownloader->DownloadFile(sCheckURL + SIGNATURE_FILE_ENDING, signatureTempfile, false);
		if (ret || VerifyIntegrity(tempfile, signatureTempfile, m_updateDownloader))
		{
			CString error = _T("Could not verify digital signature.");
			if (ret)
				error += _T("\r\nError: ") + GetWinINetError(ret) + _T(" (on ") + sCheckURL + SIGNATURE_FILE_ENDING + _T(")");
			SetDlgItemText(IDC_CHECKRESULT, error);
			DeleteUrlCacheEntry(sCheckURL);
			DeleteUrlCacheEntry(sCheckURL + SIGNATURE_FILE_ENDING);
			goto finish;
		}
	}
	else if (ret)
	{
		DeleteUrlCacheEntry(sCheckURL);
		if (CRegDWORD(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\GlobalUserOffline"), 0))
			errorText.LoadString(IDS_OFFLINEMODE); // offline mode enabled
		else
			errorText.Format(IDS_CHECKNEWER_NETERROR_FORMAT, (LPCTSTR)(GetWinINetError(ret) + _T(" URL: ") + sCheckURL), ret);
		SetDlgItemText(IDC_CHECKRESULT, errorText);
		goto finish;
	}

	git_config_add_file_ondisk(versioncheck, CUnicodeUtils::GetUTF8(tempfile), GIT_CONFIG_LEVEL_GLOBAL, 0);

	unsigned int major, minor, micro, build;
	major = minor = micro = build = 0;
	unsigned __int64 version = 0;

	if (!versioncheck.GetString(_T("tortoisegit.version"), ver))
	{
		CString vertemp = ver;
		major = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		minor = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		micro = _ttoi(vertemp);
		vertemp = vertemp.Mid(vertemp.Find('.') + 1);
		build = _ttoi(vertemp);
		version = major;
		version <<= 16;
		version += minor;
		version <<= 16;
		version += micro;
		version <<= 16;
		version += build;

		if (version == 0)
		{
			temp.LoadString(IDS_CHECKNEWER_NETERROR);
			SetDlgItemText(IDC_CHECKRESULT, temp);
			goto finish;
		}

		// another versionstring for the filename can be provided
		// this is needed for preview releases
		vertemp.Empty();
		versioncheck.GetString(_T("tortoisegit.versionstring"), vertemp);
		if (!vertemp.IsEmpty())
			ver = vertemp;
	}
	else
	{
		errorText = _T("Could not parse version check file: ") + g_Git.GetLibGit2LastErr();
		SetDlgItemText(IDC_CHECKRESULT, errorText);
		DeleteUrlCacheEntry(sCheckURL);
		goto finish;
	}

	{
		BOOL bNewer = FALSE;
		if (m_bForce)
			bNewer = TRUE;
		else if (major > TGIT_VERMAJOR)
			bNewer = TRUE;
		else if ((minor > TGIT_VERMINOR) && (major == TGIT_VERMAJOR))
			bNewer = TRUE;
		else if ((micro > TGIT_VERMICRO) && (minor == TGIT_VERMINOR) && (major == TGIT_VERMAJOR))
			bNewer = TRUE;
		else if ((build > TGIT_VERBUILD) && (micro == TGIT_VERMICRO) && (minor == TGIT_VERMINOR) && (major == TGIT_VERMAJOR))
			bNewer = TRUE;

		m_sNewVersionNumber.Format(L"%u.%u.%u.%u", major, minor, micro, build);
		if (m_sNewVersionNumber != ver)
		{
			CString versionstr = m_sNewVersionNumber + L" (" + ver + ")";
			temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)versionstr);
		}
		else
			temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)m_sNewVersionNumber);
		SetDlgItemText(IDC_CURRENTVERSION, temp);

		if (bNewer)
		{
			versioncheck.GetString(_T("tortoisegit.infotext"), temp);
			if (!temp.IsEmpty())
			{
				CString tempLink;
				versioncheck.GetString(_T("tortoisegit.infotexturl"), tempLink);
				if (!tempLink.IsEmpty()) // find out the download link-URL, if any
					m_sUpdateDownloadLink = tempLink;
			}
			else
				temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
			SetDlgItemText(IDC_CHECKRESULT, temp);

			FillChangelog(versioncheck, official);
			FillDownloads(versioncheck, ver);

			RemoveAnchor(IDC_GROUP_CHANGELOG);
			RemoveAnchor(IDC_LOGMESSAGE);
			RemoveAnchor(IDC_GROUP_DOWNLOADS);
			RemoveAnchor(IDC_LIST_DOWNLOADS);
			RemoveAnchor(IDC_PROGRESSBAR);
			RemoveAnchor(IDC_BUTTON_UPDATE);
			RemoveAnchor(IDOK);

			// Show download controls
			RECT rectWindow, rectProgress, rectGroupDownloads, rectOKButton;
			GetWindowRect(&rectWindow);
			m_progress.GetWindowRect(&rectProgress);
			GetDlgItem(IDC_GROUP_DOWNLOADS)->GetWindowRect(&rectGroupDownloads);
			GetDlgItem(IDOK)->GetWindowRect(&rectOKButton);
			LONG bottomDistance = rectWindow.bottom - rectOKButton.bottom;
			OffsetRect(&rectOKButton, 0, (rectGroupDownloads.bottom + (rectGroupDownloads.bottom - rectProgress.bottom)) - rectOKButton.top);
			rectWindow.bottom = rectOKButton.bottom + bottomDistance;
			SetMinTrackSize(CSize(rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top));
			MoveWindow(&rectWindow);
			::MapWindowPoints(nullptr, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
			if (CRegDWORD(_T("Software\\TortoiseGit\\VersionCheck"), TRUE) != FALSE && !m_bForce && !m_bShowInfo)
			{
				GetDlgItem(IDC_DONOTASKAGAIN)->ShowWindow(SW_SHOW);
				rectOKButton.left += 60;
				temp.LoadString(IDS_REMINDMELATER);
				GetDlgItem(IDOK)->SetWindowText(temp);
				rectOKButton.right += 160;
			}
			GetDlgItem(IDOK)->MoveWindow(&rectOKButton);
			AddAnchor(IDC_GROUP_CHANGELOG, TOP_LEFT, BOTTOM_RIGHT);
			AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, BOTTOM_RIGHT);
			AddAnchor(IDC_GROUP_DOWNLOADS, BOTTOM_LEFT, BOTTOM_RIGHT);
			AddAnchor(IDC_LIST_DOWNLOADS, BOTTOM_LEFT, BOTTOM_RIGHT);
			AddAnchor(IDC_PROGRESSBAR, BOTTOM_LEFT, BOTTOM_RIGHT);
			AddAnchor(IDC_BUTTON_UPDATE, BOTTOM_RIGHT);
			AddAnchor(IDOK, BOTTOM_CENTER);
			m_ctrlFiles.ShowWindow(SW_SHOW);
			GetDlgItem(IDC_GROUP_DOWNLOADS)->ShowWindow(SW_SHOW);
			CenterWindow();
			m_bShowInfo = TRUE;
		}
		else if (m_bShowInfo)
		{
			temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
			SetDlgItemText(IDC_CHECKRESULT, temp);
			FillChangelog(versioncheck, official);
		}
	}

finish:
	if (!m_sUpdateDownloadLink.IsEmpty())
	{
		m_link.ShowWindow(SW_SHOW);
		m_link.SetURL(m_sUpdateDownloadLink);
	}
	m_bThreadRunning = FALSE;
	DialogEnableWindow(IDOK, TRUE);
	return 0;
}

void CCheckForUpdatesDlg::FillDownloads(CAutoConfig& versioncheck, const CString version)
{
#if WIN64
	const CString x86x64 = _T("64");
#else
	const CString x86x64 = _T("32");
#endif

	versioncheck.GetString(_T("tortoisegit.baseurl"), m_sFilesURL);
	if (m_sFilesURL.IsEmpty())
		m_sFilesURL.Format(_T("http://updater.download.tortoisegit.org/tgit/%s/"), (LPCTSTR)version);

	bool isHotfix = false;
	versioncheck.GetBool(_T("tortoisegit.hotfix"), isHotfix);

	if (isHotfix)
		m_ctrlFiles.InsertItem(0, _T("TortoiseGit Hotfix"));
	else
		m_ctrlFiles.InsertItem(0, _T("TortoiseGit"));
	CString filenameMain, filenamePattern;
	versioncheck.GetString(_T("tortoisegit.mainfilename"), filenamePattern);
	if (filenamePattern.IsEmpty())
		filenamePattern = _T("TortoiseGit-%1!s!-%2!s!bit.msi");
	filenameMain.FormatMessage(filenamePattern, version, x86x64);
	m_ctrlFiles.SetItemData(0, (DWORD_PTR)(new CUpdateListCtrl::Entry(filenameMain, CUpdateListCtrl::STATUS_NONE)));
	m_ctrlFiles.SetCheck(0 , TRUE);

	if (isHotfix)
	{
		DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
		return;
	}

	struct LangPack
	{
		CString m_PackName;
		CString m_LangName;
		DWORD m_LocaleID;
		CString m_LangCode;
		bool m_Installed;
	};
	struct LanguagePacks
	{
		std::vector<LangPack> availableLangs;
		std::vector<DWORD> installedLangs;
	} languagePacks;
	{
		// set up the language selecting combobox
		CString path = CPathUtils::GetAppParentDirectory();
		path = path + _T("Languages\\");
		CSimpleFileFind finder(path, _T("*.dll"));
		while (finder.FindNextFileNoDirectories())
		{
			CString file = finder.GetFilePath();
			CString filename = finder.GetFileName();
			if (filename.Left(12).CompareNoCase(_T("TortoiseProc")) == 0)
			{
				CString sVer = _T(STRPRODUCTVER);
				sVer = sVer.Left(sVer.ReverseFind('.'));
				CString sFileVer = CPathUtils::GetVersionFromFile(file);
				sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
				CString sLoc = filename.Mid(12);
				sLoc = sLoc.Left(sLoc.GetLength() - 4); // cut off ".dll"
				if ((sLoc.Left(2) == L"32") && (sLoc.GetLength() > 5))
					continue;
				DWORD loc = _tstoi(filename.Mid(12));
				languagePacks.installedLangs.push_back(loc);
			}
		}
	}

	git_config_get_multivar_foreach(versioncheck, "tortoisegit.langs", nullptr, [](const git_config_entry* configentry, void* payload) -> int
	{
		LanguagePacks* languagePacks = (LanguagePacks*)payload;
		CString langs = CUnicodeUtils::GetUnicode(configentry->value);

		int nextTokenPos = langs.Find(_T(";"), 5); // be extensible for the future
		if (nextTokenPos > 0)
			langs = langs.Left(nextTokenPos);
		CString sLang = _T("TortoiseGit Language Pack ") + langs.Mid(5);

		DWORD loc = _tstoi(langs.Mid(0, 4));
		TCHAR buf[MAX_PATH] = { 0 };
		GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		CString sLang2(buf);
		GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
		if (buf[0])
		{
			sLang2 += _T(" (");
			sLang2 += buf;
			sLang2 += _T(")");
		}

		bool installed = std::find(languagePacks->installedLangs.cbegin(), languagePacks->installedLangs.cend(), loc) != languagePacks->installedLangs.cend();
		LangPack pack = { sLang, sLang2, loc, langs.Mid(5), installed };
		languagePacks->availableLangs.push_back(pack);

		return 0;
	}, &languagePacks);
	std::stable_sort(languagePacks.availableLangs.begin(), languagePacks.availableLangs.end(), [&](const LangPack& a, const LangPack& b) -> int
	{
		return (a.m_Installed && !b.m_Installed) ? 1 : (!a.m_Installed && b.m_Installed) ? 0 : (a.m_PackName.Compare(b.m_PackName) < 0);
	});
	filenamePattern.Empty();
	versioncheck.GetString(_T("tortoisegit.languagepackfilename"), filenamePattern);
	if (filenamePattern.IsEmpty())
		filenamePattern = _T("TortoiseGit-LanguagePack-%1!s!-%2!s!bit-%3!s!.msi");
	for (const auto& langs : languagePacks.availableLangs)
	{
		int pos = m_ctrlFiles.InsertItem(m_ctrlFiles.GetItemCount(), langs.m_PackName);
		m_ctrlFiles.SetItemText(pos, 1, langs.m_LangName);

		CString filename;
		filename.FormatMessage(filenamePattern, version, x86x64, langs.m_LangCode, langs.m_LocaleID);
		m_ctrlFiles.SetItemData(pos, (DWORD_PTR)(new CUpdateListCtrl::Entry(filename, CUpdateListCtrl::STATUS_NONE)));

		if (langs.m_Installed)
			m_ctrlFiles.SetCheck(pos , TRUE);
	}
	DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
}

void CCheckForUpdatesDlg::FillChangelog(CAutoConfig& versioncheck, bool official)
{
	ProjectProperties pp;
	pp.lProjectLanguage = -1;
	if (versioncheck.GetString(_T("tortoisegit.issuesurl"), pp.sUrl))
		pp.sUrl = _T("https://tortoisegit.org/issue/%BUGID%");
	if (!pp.sUrl.IsEmpty())
	{
		pp.SetCheckRe(_T("[Ii]ssues?:?(\\s*(,|and)?\\s*#?\\d+)+"));
		pp.SetBugIDRe(_T("(\\d+)"));
	}
	m_cLogMessage.Init(pp);

	CString sChangelogURL;
	versioncheck.GetString(_T("TortoiseGit.changelogurl"), sChangelogURL);
	if (sChangelogURL.IsEmpty())
		sChangelogURL = _T("https://versioncheck.tortoisegit.org/changelog.txt");
	else
	{
		CString tmp(sChangelogURL);
		sChangelogURL.FormatMessage(tmp, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, m_updateDownloader->m_sWindowsPlatform, m_updateDownloader->m_sWindowsVersion, m_updateDownloader->m_sWindowsServicePack);
	}

	CString tempchangelogfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	DWORD err;
	if ((err = m_updateDownloader->DownloadFile(sChangelogURL, tempchangelogfile, false)) != ERROR_SUCCESS)
	{
		CString msg = _T("Could not load changelog.\r\nError: ") + GetWinINetError(err) + _T(" (on ") + sChangelogURL + _T(")");
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>((LPCTSTR)msg));
		return;
	}
	if (official)
	{
		CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		if ((err = m_updateDownloader->DownloadFile(sChangelogURL + SIGNATURE_FILE_ENDING, signatureTempfile, false)) != ERROR_SUCCESS || VerifyIntegrity(tempchangelogfile, signatureTempfile, m_updateDownloader))
		{
			CString error = _T("Could not verify digital signature.");
			if (err)
				error += _T("\r\nError: ") + GetWinINetError(err) + _T(" (on ") + sChangelogURL + SIGNATURE_FILE_ENDING + _T(")");
			::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>((LPCTSTR)error));
			DeleteUrlCacheEntry(sChangelogURL);
			DeleteUrlCacheEntry(sChangelogURL + SIGNATURE_FILE_ENDING);
			return;
		}
	}

	CString temp;
	CStdioFile file;
	if (file.Open(tempchangelogfile, CFile::modeRead | CFile::typeBinary))
	{
		auto buf = std::make_unique<BYTE[]>((UINT)file.GetLength());
		UINT read = file.Read(buf.get(), (UINT)file.GetLength());
		bool skipBom = read >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF;
		CGit::StringAppend(&temp, buf.get() + (skipBom ? 3 : 0), CP_UTF8, read - (skipBom ? 3 : 0));
	}
	else
		temp = _T("Could not open downloaded changelog file.");
	::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>((LPCTSTR)temp));
}

void CCheckForUpdatesDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 100)
	{
		if (m_bThreadRunning == FALSE)
		{
			KillTimer(100);
			if (m_bShowInfo)
			{
				m_bVisible = TRUE;
				ShowWindow(SW_SHOWNORMAL);
			}
			else
				EndDialog(0);
		}
	}
	CResizableStandAloneDialog::OnTimer(nIDEvent);
}

void CCheckForUpdatesDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CResizableStandAloneDialog::OnWindowPosChanging(lpwndpos);
	if (m_bVisible == FALSE)
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
}

BOOL CCheckForUpdatesDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CCheckForUpdatesDlg::OnBnClickedButtonUpdate()
{
	CString title;
	m_ctrlUpdate.GetWindowText(title);
	if (!m_pDownloadThread && title == CString(MAKEINTRESOURCE(IDS_PROC_DOWNLOAD)))
	{
		bool isOneSelected = false;
		for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); ++i)
		{
			if (m_ctrlFiles.GetCheck(i))
			{
				isOneSelected = true;
				break;
			}
		}
		if (!isOneSelected)
			return;

		m_eventStop.ResetEvent();

		m_pDownloadThread = ::AfxBeginThread(DownloadThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if (m_pDownloadThread)
		{
			m_pDownloadThread->m_bAutoDelete = FALSE;
			m_pDownloadThread->ResumeThread();

			GetDlgItem(IDC_BUTTON_UPDATE)->SetWindowText(CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
		}
		else
			CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	else if (title == CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)))
	{
		// Abort
		m_eventStop.SetEvent();
	}
	else
	{
		CString folder = GetDownloadsDirectory();
		if (m_ctrlUpdate.GetCurrentEntry() == 0)
		{
			for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); ++i)
			{
				CUpdateListCtrl::Entry *data = (CUpdateListCtrl::Entry *)m_ctrlFiles.GetItemData(i);
				if (m_ctrlFiles.GetCheck(i) == TRUE)
					ShellExecute(GetSafeHwnd(), _T("open"), folder + data->m_filename, nullptr, nullptr, SW_SHOWNORMAL);
			}
			CResizableStandAloneDialog::OnOK();
		}
		else if (m_ctrlUpdate.GetCurrentEntry() == 1)
			ShellExecute(GetSafeHwnd(), _T("open"), folder, nullptr, nullptr, SW_SHOWNORMAL);

		m_ctrlUpdate.SetCurrentEntry(0);
	}
}

UINT CCheckForUpdatesDlg::DownloadThreadEntry(LPVOID pVoid)
{
	return reinterpret_cast<CCheckForUpdatesDlg*>(pVoid)->DownloadThread();
}

bool CCheckForUpdatesDlg::VerifyUpdateFile(const CString& filename, const CString& filenameSignature, const CString& reportingFilename)
{
	if (VerifyIntegrity(filename, filenameSignature, m_updateDownloader) != 0)
	{
		m_sErrors += reportingFilename + SIGNATURE_FILE_ENDING + L": Invalid digital signature.\r\n";
		return false;
	}

	MSIHANDLE hSummary;
	DWORD ret = 0;
	if ((ret = MsiGetSummaryInformation(NULL, filename, 0, &hSummary)) != 0)
	{
		CString sFileVer = CPathUtils::GetVersionFromFile(filename);
		sFileVer.Trim();
		if (sFileVer.IsEmpty())
		{
			m_sErrors.AppendFormat(L"%s: Invalid filetype found (neither executable nor MSI).\r\n", (LPCTSTR)reportingFilename);
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": MsiGetSummaryInformation reported: %s\n"), (LPCTSTR)CFormatMessageWrapper(ret));
			return false;
		}
		else if (sFileVer == m_sNewVersionNumber)
			return true;

		m_sErrors.AppendFormat(L"%s: Version number of downloaded file doesn't match (expected: \"%s\", got: \"%s\").\r\n", (LPCTSTR)reportingFilename, (LPCTSTR)m_sNewVersionNumber, (LPCTSTR)sFileVer);
		return false;
	}
	SCOPE_EXIT{ MsiCloseHandle(hSummary); };

	UINT uiDataType = 0;
	DWORD cchValue = 4096;
	CString buffer;
	if (MsiSummaryInfoGetProperty(hSummary, PID_SUBJECT, &uiDataType, nullptr, nullptr, CStrBuf(buffer, cchValue + 1), &cchValue))
	{
		m_sErrors.AppendFormat(L"%s: Error obtaining version of MSI file (%s).\r\n", (LPCTSTR)reportingFilename, (LPCTSTR)CFormatMessageWrapper(ret));
		return false;
	}

	if (VT_LPSTR != uiDataType)
	{
		m_sErrors.AppendFormat(L"%s: Error obtaining version of MSI file (invalid data type returned).\r\n", (LPCTSTR)reportingFilename);
		return false;
	}

	CString sFileVer = buffer.Right(m_sNewVersionNumber.GetLength() + 1);
	if (sFileVer != L"v" + m_sNewVersionNumber)
	{
		m_sErrors.AppendFormat(L"%s: Version number of downloaded file doesn't match (expected: \"v%s\", got: \"%s\").\r\n", (LPCTSTR)reportingFilename, (LPCTSTR)m_sNewVersionNumber, (LPCTSTR)sFileVer);
		return false;
	}

	return true;
}

bool CCheckForUpdatesDlg::Download(CString filename)
{
	CString url = m_sFilesURL + filename;
	CString destFilename = GetDownloadsDirectory() + filename;
	if (PathFileExists(destFilename) && PathFileExists(destFilename + SIGNATURE_FILE_ENDING))
	{
		if (VerifyUpdateFile(destFilename, destFilename + SIGNATURE_FILE_ENDING, destFilename))
			return true;
		else
		{
			DeleteFile(destFilename);
			DeleteFile(destFilename + SIGNATURE_FILE_ENDING);
			DeleteUrlCacheEntry(url);
			DeleteUrlCacheEntry(url + SIGNATURE_FILE_ENDING);
		}
	}

	m_progress.SetRange32(0, 1);
	m_progress.SetPos(0);
	m_progress.ShowWindow(SW_SHOW);
	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
		m_pTaskbarList->SetProgressValue(m_hWnd, 0, 1);
	}

	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	DWORD ret = m_updateDownloader->DownloadFile(url, tempfile, true);
	if (!ret)
	{
		ret = m_updateDownloader->DownloadFile(url + SIGNATURE_FILE_ENDING, signatureTempfile, true);
		if (ret)
			m_sErrors += url + SIGNATURE_FILE_ENDING + _T(": ") + GetWinINetError(ret) + _T("\r\n");
		m_progress.SetPos(m_progress.GetPos() + 1);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			int minValue, maxValue;
			m_progress.GetRange(minValue, maxValue);
			m_pTaskbarList->SetProgressValue(m_hWnd, m_progress.GetPos(), maxValue);
		}
	}
	else
		m_sErrors += url + _T(": ") + GetWinINetError(ret) + _T("\r\n");
	if (!ret)
	{
		if (VerifyUpdateFile(tempfile, signatureTempfile, url))
		{
			DeleteFile(destFilename);
			DeleteFile(destFilename + SIGNATURE_FILE_ENDING);
			if (!MoveFile(tempfile, destFilename))
			{
				m_sErrors.AppendFormat(L"Could not move \"%s\" to \"%s\".\r\n", (LPCTSTR)filename, (LPCTSTR)tempfile, (LPCTSTR)destFilename);
				return false;
			}
			if (!MoveFile(signatureTempfile, destFilename + SIGNATURE_FILE_ENDING))
			{
				m_sErrors.AppendFormat(L"Could not move \"%s\" to \"%s\".\r\n", (LPCTSTR)filename, (LPCTSTR)tempfile, (LPCTSTR)destFilename);
				return false;
			}
			return true;
		}
		DeleteUrlCacheEntry(url);
		DeleteUrlCacheEntry(url + SIGNATURE_FILE_ENDING);
	}
	return false;
}

UINT CCheckForUpdatesDlg::DownloadThread()
{
	m_ctrlFiles.SetExtendedStyle(m_ctrlFiles.GetExtendedStyle() & ~LVS_EX_CHECKBOXES);
	m_sErrors.Empty();
	BOOL result = TRUE;
	for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); ++i)
	{
		m_ctrlFiles.EnsureVisible(i, FALSE);
		CRect rect;
		m_ctrlFiles.GetItemRect(i, &rect, LVIR_BOUNDS);
		CUpdateListCtrl::Entry *data = (CUpdateListCtrl::Entry *)m_ctrlFiles.GetItemData(i);
		if (m_ctrlFiles.GetCheck(i) == TRUE)
		{
			data->m_status = CUpdateListCtrl::STATUS_DOWNLOADING;
			m_ctrlFiles.InvalidateRect(rect);
			if (Download(data->m_filename))
				data->m_status = CUpdateListCtrl::STATUS_SUCCESS;
			else
			{
				data->m_status = CUpdateListCtrl::STATUS_FAIL;
				result = FALSE;
			}
		}
		else
			data->m_status = CUpdateListCtrl::STATUS_IGNORE;
		m_ctrlFiles.InvalidateRect(rect);
	}

	::PostMessage(GetSafeHwnd(), WM_USER_ENDDOWNLOAD, 0, 0);

	return result;
}

LRESULT CCheckForUpdatesDlg::OnEndDownload(WPARAM, LPARAM)
{
	ASSERT(m_pDownloadThread);

	// wait until the thread terminates
	DWORD dwExitCode;
	if (::GetExitCodeThread(m_pDownloadThread->m_hThread, &dwExitCode) && dwExitCode == STILL_ACTIVE)
		::WaitForSingleObject(m_pDownloadThread->m_hThread, INFINITE);

	// make sure we always have the correct exit code
	::GetExitCodeThread(m_pDownloadThread->m_hThread, &dwExitCode);

	delete m_pDownloadThread;
	m_pDownloadThread = nullptr;

	m_progress.ShowWindow(SW_HIDE);

	if (dwExitCode == TRUE)
	{
		m_ctrlUpdate.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_INSTALL)));
		m_ctrlUpdate.AddEntry(CString(MAKEINTRESOURCE(IDS_CHECKUPDATE_DESTFOLDER)));
		m_ctrlUpdate.Invalidate();
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}
	else
	{
		m_ctrlUpdate.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_DOWNLOAD)));
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
		CString tmp;
		tmp.LoadString(IDS_ERR_FAILEDUPDATEDOWNLOAD);
		if (!m_sErrors.IsEmpty())
			tmp += _T("\r\n\r\nErrors:\r\n") + m_sErrors;
		CMessageBox::Show(GetSafeHwnd(), tmp, L"TortoiseGit", MB_ICONERROR);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}

	return 0;
}

LRESULT CCheckForUpdatesDlg::OnFillChangelog(WPARAM, LPARAM lParam)
{
	ASSERT(lParam);

	LPCTSTR changelog = reinterpret_cast<LPCTSTR>(lParam);
	m_cLogMessage.Call(SCI_SETREADONLY, FALSE);
	m_cLogMessage.SetText(changelog);
	m_cLogMessage.Call(SCI_SETREADONLY, TRUE);
	m_cLogMessage.Call(SCI_GOTOPOS, 0);

	return 0;
}

CString CCheckForUpdatesDlg::GetDownloadsDirectory()
{
	CString folder;

	PWSTR wcharPtr = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, nullptr, &wcharPtr)))
	{
		folder = wcharPtr;
		CoTaskMemFree(wcharPtr);
		return folder.TrimRight(_T("\\")) + _T("\\");
	}

	return folder;
}

LRESULT CCheckForUpdatesDlg::OnDisplayStatus(WPARAM, LPARAM lParam)
{
	const CUpdateDownloader::DOWNLOADSTATUS *const pDownloadStatus = reinterpret_cast<CUpdateDownloader::DOWNLOADSTATUS *>(lParam);
	if (pDownloadStatus)
	{
		ASSERT(::AfxIsValidAddress(pDownloadStatus, sizeof(CUpdateDownloader::DOWNLOADSTATUS)));

		m_progress.SetRange32(0, pDownloadStatus->ulProgressMax);
		m_progress.SetPos(pDownloadStatus->ulProgress);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, pDownloadStatus->ulProgress, pDownloadStatus->ulProgressMax);
		}
	}

	return 0;
}

LRESULT CCheckForUpdatesDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return 0;
}

CString CCheckForUpdatesDlg::GetWinINetError(DWORD err)
{
	CString readableError = CFormatMessageWrapper(err);
	if (readableError.IsEmpty())
	{
		for (const CString& module : { _T("wininet.dll"), _T("urlmon.dll") })
		{
			LPTSTR buffer;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(module), err, 0, (LPTSTR)&buffer, 0, nullptr);
			readableError = buffer;
			LocalFree(buffer);
			if (!readableError.IsEmpty())
				break;
		}
	}
	return readableError.Trim();
}

void CCheckForUpdatesDlg::OnBnClickedDonotaskagain()
{
	if (CMessageBox::Show(GetSafeHwnd(), IDS_DISABLEUPDATECHECKS, IDS_APPNAME, 2, IDI_QUESTION, IDS_DISABLEUPDATECHECKSBUTTON, IDS_ABORTBUTTON) == 1)
	{
		CRegDWORD(_T("Software\\TortoiseGit\\VersionCheck")) = FALSE;
		OnOK();
	}
}
