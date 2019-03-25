// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2019 - TortoiseGit

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
#include "../version.h"
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

#define SIGNATURE_FILE_ENDING L".rsa.asc"

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
	, m_sUpdateDownloadLink(L"https://tortoisegit.org/download")
{
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
	ON_WM_SYSCOLORCHANGE()
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
	::MapWindowPoints(nullptr, GetSafeHwnd(), reinterpret_cast<LPPOINT>(&rectOKButton), 2);
	GetDlgItem(IDOK)->MoveWindow(&rectOKButton);

	temp.LoadString(IDS_STATUSLIST_COLFILE);
	m_ctrlFiles.InsertColumn(0, temp, 0, -1);
	m_ctrlFiles.InsertColumn(1, temp, 0, -1);
	m_ctrlFiles.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
	m_ctrlFiles.SetColumnWidth(0, 350);
	m_ctrlFiles.SetColumnWidth(1, 200);

	m_cLogMessage.Init(-1);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
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
		delete reinterpret_cast<CUpdateListCtrl::Entry*>(m_ctrlFiles.GetItemData(i));

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
	return static_cast<CCheckForUpdatesDlg*>(pVoid)->CheckThread();
}

UINT CCheckForUpdatesDlg::CheckThread()
{
	m_bThreadRunning = TRUE;

	CString temp;
	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();

	bool official = false;

	CRegString checkurluser = CRegString(L"Software\\TortoiseGit\\UpdateCheckURL", L"");
	CRegString checkurlmachine = CRegString(L"Software\\TortoiseGit\\UpdateCheckURL", L"", FALSE, HKEY_LOCAL_MACHINE);
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
				sCheckURL = L"https://versioncheck.tortoisegit.org/version-preview.txt";
				SetDlgItemText(IDC_SOURCE, L"Using preview release channel");
			}
			else
				sCheckURL = L"https://versioncheck.tortoisegit.org/version.txt";
		}
	}

	if (!official && CStringUtils::StartsWith(sCheckURL, L"https://versioncheck.tortoisegit.org/"))
		official = true;

	if (!official)
		SetDlgItemText(IDC_SOURCE, L"Using (unofficial) release channel: " + sCheckURL);

	CString errorText;
	CVersioncheckParser versioncheck;
	CVersioncheckParser::Version version;
	DWORD ret = m_updateDownloader->DownloadFile(sCheckURL, tempfile, false);
	if (!ret && official)
	{
		CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		ret = m_updateDownloader->DownloadFile(sCheckURL + SIGNATURE_FILE_ENDING, signatureTempfile, false);
		if (ret || VerifyIntegrity(tempfile, signatureTempfile, m_updateDownloader))
		{
			CString error = L"Could not verify digital signature.";
			if (ret)
				error += L"\r\nError: " + GetWinINetError(ret) + L" (on " + sCheckURL + SIGNATURE_FILE_ENDING + L")";
			SetDlgItemText(IDC_CHECKRESULT, error);
			DeleteUrlCacheEntry(sCheckURL);
			DeleteUrlCacheEntry(sCheckURL + SIGNATURE_FILE_ENDING);
			goto finish;
		}
	}
	else if (ret)
	{
		DeleteUrlCacheEntry(sCheckURL);
		if (CRegDWORD(L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\GlobalUserOffline", 0))
			errorText.LoadString(IDS_OFFLINEMODE); // offline mode enabled
		else
			errorText.FormatMessage(IDS_CHECKNEWER_NETERROR_FORMAT, static_cast<LPCTSTR>(GetWinINetError(ret) + L" URL: " + sCheckURL), ret);
		SetDlgItemText(IDC_CHECKRESULT, errorText);
		goto finish;
	}

	if (!versioncheck.Load(tempfile, errorText))
	{
		if (!errorText.IsEmpty())
			SetDlgItemText(IDC_CHECKRESULT, errorText);
		else
		{
			temp.LoadString(IDS_CHECKNEWER_NETERROR);
			SetDlgItemText(IDC_CHECKRESULT, temp);
		}
		DeleteUrlCacheEntry(sCheckURL);
		goto finish;
	}

	version = versioncheck.GetTortoiseGitVersion();
	{
		BOOL bNewer = FALSE;
		if (m_bForce)
			bNewer = TRUE;
		else if (version.major > TGIT_VERMAJOR)
			bNewer = TRUE;
		else if ((version.minor > TGIT_VERMINOR) && (version.major == TGIT_VERMAJOR))
			bNewer = TRUE;
		else if ((version.micro > TGIT_VERMICRO) && (version.minor == TGIT_VERMINOR) && (version.major == TGIT_VERMAJOR))
			bNewer = TRUE;
		else if ((version.build > TGIT_VERBUILD) && (version.micro == TGIT_VERMICRO) && (version.minor == TGIT_VERMINOR) && (version.major == TGIT_VERMAJOR))
			bNewer = TRUE;

		m_sNewVersionNumber.Format(L"%u.%u.%u.%u", version.major, version.minor, version.micro, version.build);
		if (m_sNewVersionNumber != version.version_for_filename)
		{
			CString versionstr = m_sNewVersionNumber + L" (" + version.version_for_filename + L")";
			temp.Format(IDS_CHECKNEWER_CURRENTVERSION, static_cast<LPCTSTR>(versionstr));
		}
		else
			temp.Format(IDS_CHECKNEWER_CURRENTVERSION, static_cast<LPCTSTR>(m_sNewVersionNumber));
		SetDlgItemText(IDC_CURRENTVERSION, temp);

		if (bNewer)
		{
			temp = versioncheck.GetTortoiseGitInfoText();
			if (!temp.IsEmpty())
			{
				CString tempLink = versioncheck.GetTortoiseGitInfoTextURL();
				if (!tempLink.IsEmpty()) // find out the download link-URL, if any
					m_sUpdateDownloadLink = tempLink;
			}
			else
				temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
			SetDlgItemText(IDC_CHECKRESULT, temp);

			FillChangelog(versioncheck, official);
			FillDownloads(versioncheck);

			RemoveAnchor(IDC_GROUP_CHANGELOG);
			RemoveAnchor(IDC_LOGMESSAGE);
			RemoveAnchor(IDC_GROUP_DOWNLOADS);
			RemoveAnchor(IDC_LIST_DOWNLOADS);
			RemoveAnchor(IDC_PROGRESSBAR);
			RemoveAnchor(IDC_DONOTASKAGAIN);
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
			::MapWindowPoints(nullptr, GetSafeHwnd(), reinterpret_cast<LPPOINT>(&rectOKButton), 2);
			if (CRegDWORD(L"Software\\TortoiseGit\\VersionCheck", TRUE) != FALSE && !m_bForce && !m_bShowInfo)
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
			AddAnchor(IDC_DONOTASKAGAIN, BOTTOM_CENTER);
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

void CCheckForUpdatesDlg::FillDownloads(CVersioncheckParser& versioncheck)
{
	m_sFilesURL = versioncheck.GetTortoiseGitBaseURL();

	bool isHotfix = versioncheck.GetTortoiseGitIsHotfix();
	if (isHotfix)
		m_ctrlFiles.InsertItem(0, L"TortoiseGit Hotfix");
	else
		m_ctrlFiles.InsertItem(0, L"TortoiseGit");
	CString filenameMain = versioncheck.GetTortoiseGitMainfilename();
	m_ctrlFiles.SetItemData(0, reinterpret_cast<DWORD_PTR>(new CUpdateListCtrl::Entry(filenameMain, CUpdateListCtrl::STATUS_NONE)));
	m_ctrlFiles.SetCheck(0 , TRUE);

	if (isHotfix)
	{
		DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
		return;
	}

	struct LangPack
	{
		CVersioncheckParser::LanguagePack languagepack;
		bool m_Installed;
	};
	std::vector<LangPack> availableLangs;
	std::vector<DWORD> installedLangs;
	{
		// set up the language selecting combobox
		CString path = CPathUtils::GetAppParentDirectory();
		path = path + L"Languages\\";
		CSimpleFileFind finder(path, L"*.dll");
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
				CString sLoc = filename.Mid(static_cast<int>(wcslen(L"TortoiseProc")));
				sLoc = sLoc.Left(sLoc.GetLength() - static_cast<int>(wcslen(L".dll"))); // cut off ".dll"
				if (CStringUtils::StartsWith(sLoc, L"32") && (sLoc.GetLength() > 5))
					continue;
				DWORD loc = _wtoi(filename.Mid(static_cast<int>(wcslen(L"TortoiseProc"))));
				installedLangs.push_back(loc);
			}
		}
	}

	for (auto& languagepack : versioncheck.GetTortoiseGitLanguagePacks())
	{
		bool installed = std::find(installedLangs.cbegin(), installedLangs.cend(), languagepack.m_LocaleID) != installedLangs.cend();
		LangPack pack = { languagepack, installed };
		availableLangs.push_back(pack);
	}
	std::stable_sort(availableLangs.begin(), availableLangs.end(), [&](const LangPack& a, const LangPack& b) -> int
	{
		return (a.m_Installed && !b.m_Installed) ? 1 : (!a.m_Installed && b.m_Installed) ? 0 : (a.languagepack.m_PackName.Compare(b.languagepack.m_PackName) < 0);
	});
	for (const auto& langs : availableLangs)
	{
		int pos = m_ctrlFiles.InsertItem(m_ctrlFiles.GetItemCount(), langs.languagepack.m_PackName);
		m_ctrlFiles.SetItemText(pos, 1, langs.languagepack.m_LangName);
		m_ctrlFiles.SetItemData(pos, reinterpret_cast<DWORD_PTR>(new CUpdateListCtrl::Entry(langs.languagepack.m_filename, CUpdateListCtrl::STATUS_NONE)));

		if (langs.m_Installed)
			m_ctrlFiles.SetCheck(pos , TRUE);
	}
	DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
}

void CCheckForUpdatesDlg::FillChangelog(CVersioncheckParser& versioncheck, bool official)
{
	ProjectProperties pp;
	pp.lProjectLanguage = -1;
	pp.sUrl = versioncheck.GetTortoiseGitIssuesURL();
	if (!pp.sUrl.IsEmpty())
	{
		pp.SetCheckRe(L"[Ii]ssues?:?(\\s*(,|and)?\\s*#?\\d+)+");
		pp.SetBugIDRe(L"(\\d+)");
	}
	m_cLogMessage.Init(pp);

	CString sChangelogURL;
	sChangelogURL.FormatMessage(versioncheck.GetTortoiseGitChangelogURL(), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, static_cast<LPCTSTR>(m_updateDownloader->m_sWindowsPlatform), static_cast<LPCTSTR>(m_updateDownloader->m_sWindowsVersion), static_cast<LPCTSTR>(m_updateDownloader->m_sWindowsServicePack));

	CString tempchangelogfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	if (DWORD err = m_updateDownloader->DownloadFile(sChangelogURL, tempchangelogfile, false); err != ERROR_SUCCESS)
	{
		CString msg = L"Could not load changelog.\r\nError: " + GetWinINetError(err) + L" (on " + sChangelogURL + L")";
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(msg)));
		return;
	}
	if (official)
	{
		CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		if (DWORD err = m_updateDownloader->DownloadFile(sChangelogURL + SIGNATURE_FILE_ENDING, signatureTempfile, false); err != ERROR_SUCCESS || VerifyIntegrity(tempchangelogfile, signatureTempfile, m_updateDownloader))
		{
			CString error = L"Could not verify digital signature.";
			if (err)
				error += L"\r\nError: " + GetWinINetError(err) + L" (on " + sChangelogURL + SIGNATURE_FILE_ENDING + L")";
			::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(error)));
			DeleteUrlCacheEntry(sChangelogURL);
			DeleteUrlCacheEntry(sChangelogURL + SIGNATURE_FILE_ENDING);
			return;
		}
	}

	CString temp;
	CStdioFile file;
	if (file.Open(tempchangelogfile, CFile::modeRead | CFile::typeBinary))
	{
		auto buf = std::make_unique<BYTE[]>(static_cast<UINT>(file.GetLength()));
		UINT read = file.Read(buf.get(), static_cast<UINT>(file.GetLength()));
		bool skipBom = read >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF;
		CGit::StringAppend(&temp, buf.get() + (skipBom ? 3 : 0), CP_UTF8, read - (skipBom ? 3 : 0));
	}
	else
		temp = L"Could not open downloaded changelog file.";
	::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(temp)));
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
		for (int i = 0; i < m_ctrlFiles.GetItemCount(); ++i)
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
			for (int i = 0; i < m_ctrlFiles.GetItemCount(); ++i)
			{
				auto data = reinterpret_cast<CUpdateListCtrl::Entry*>(m_ctrlFiles.GetItemData(i));
				if (m_ctrlFiles.GetCheck(i) == TRUE)
					ShellExecute(GetSafeHwnd(), L"open", folder + data->m_filename, nullptr, nullptr, SW_SHOWNORMAL);
			}
			CResizableStandAloneDialog::OnOK();
		}
		else if (m_ctrlUpdate.GetCurrentEntry() == 1)
			ShellExecute(GetSafeHwnd(), L"open", folder, nullptr, nullptr, SW_SHOWNORMAL);

		m_ctrlUpdate.SetCurrentEntry(0);
	}
}

UINT CCheckForUpdatesDlg::DownloadThreadEntry(LPVOID pVoid)
{
	return static_cast<CCheckForUpdatesDlg*>(pVoid)->DownloadThread();
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
			m_sErrors.AppendFormat(L"%s: Invalid filetype found (neither executable nor MSI).\r\n", static_cast<LPCTSTR>(reportingFilename));
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": MsiGetSummaryInformation reported: %s\n", static_cast<LPCTSTR>(CFormatMessageWrapper(ret)));
			return false;
		}
		else if (sFileVer == m_sNewVersionNumber)
			return true;

		m_sErrors.AppendFormat(L"%s: Version number of downloaded file doesn't match (expected: \"%s\", got: \"%s\").\r\n", static_cast<LPCTSTR>(reportingFilename), static_cast<LPCTSTR>(m_sNewVersionNumber), static_cast<LPCTSTR>(sFileVer));
		return false;
	}
	SCOPE_EXIT{ MsiCloseHandle(hSummary); };

	UINT uiDataType = 0;
	DWORD cchValue = 4096;
	CString buffer;
	int intValue;
	if (MsiSummaryInfoGetProperty(hSummary, PID_SUBJECT, &uiDataType, &intValue, nullptr, CStrBuf(buffer, cchValue + 1), &cchValue))
	{
		m_sErrors.AppendFormat(L"%s: Error obtaining version of MSI file (%s).\r\n", static_cast<LPCTSTR>(reportingFilename), static_cast<LPCTSTR>(CFormatMessageWrapper(ret)));
		return false;
	}

	if (VT_LPSTR != uiDataType)
	{
		m_sErrors.AppendFormat(L"%s: Error obtaining version of MSI file (invalid data type returned).\r\n", static_cast<LPCTSTR>(reportingFilename));
		return false;
	}

	CString sFileVer = buffer.Right(m_sNewVersionNumber.GetLength() + 1);
	if (sFileVer != L"v" + m_sNewVersionNumber)
	{
		m_sErrors.AppendFormat(L"%s: Version number of downloaded file doesn't match (expected: \"v%s\", got: \"%s\").\r\n", static_cast<LPCTSTR>(reportingFilename), static_cast<LPCTSTR>(m_sNewVersionNumber), static_cast<LPCTSTR>(sFileVer));
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
			m_sErrors += url + SIGNATURE_FILE_ENDING + L": " + GetWinINetError(ret) + L"\r\n";
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
		m_sErrors += url + L": " + GetWinINetError(ret) + L"\r\n";
	if (!ret)
	{
		if (VerifyUpdateFile(tempfile, signatureTempfile, url))
		{
			DeleteFile(destFilename);
			DeleteFile(destFilename + SIGNATURE_FILE_ENDING);
			if (!MoveFile(tempfile, destFilename))
			{
				m_sErrors.AppendFormat(L"Could not move \"%s\" to \"%s\".\r\n", static_cast<LPCTSTR>(tempfile), static_cast<LPCTSTR>(destFilename));
				return false;
			}
			if (!MoveFile(signatureTempfile, destFilename + SIGNATURE_FILE_ENDING))
			{
				m_sErrors.AppendFormat(L"Could not move \"%s\" to \"%s\".\r\n", static_cast<LPCTSTR>(signatureTempfile), static_cast<LPCTSTR>(destFilename + SIGNATURE_FILE_ENDING));
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
	for (int i = 0; i < m_ctrlFiles.GetItemCount(); ++i)
	{
		m_ctrlFiles.EnsureVisible(i, FALSE);
		CRect rect;
		m_ctrlFiles.GetItemRect(i, &rect, LVIR_BOUNDS);
		auto data = reinterpret_cast<CUpdateListCtrl::Entry*>(m_ctrlFiles.GetItemData(i));
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
			tmp += L"\r\n\r\nErrors:\r\n" + m_sErrors;
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
	m_cLogMessage.Call(SCI_SETWRAPSTARTINDENT, 3);

	return 0;
}

CString CCheckForUpdatesDlg::GetDownloadsDirectory()
{
	if (CComHeapPtr<WCHAR> wcharPtr; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, nullptr, &wcharPtr)))
	{
		CString folder = wcharPtr;
		return folder.TrimRight(L'\\') + L'\\';
	}

	return {};
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
		for (const CString& module : { L"wininet.dll", L"urlmon.dll" })
		{
			LPTSTR buffer;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(module), err, 0, reinterpret_cast<LPTSTR>(&buffer), 0, nullptr);
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
		CRegDWORD(L"Software\\TortoiseGit\\VersionCheck") = FALSE;
		OnOK();
	}
}

void CCheckForUpdatesDlg::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_cLogMessage.SetColors(true);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
}
