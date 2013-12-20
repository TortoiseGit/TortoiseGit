// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2013 - TortoiseGit

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
#include "Win7.h"

#define SIGNATURE_FILE_ENDING _T(".asc")

#define WM_USER_DISPLAYSTATUS	(WM_USER + 1)
#define WM_USER_ENDDOWNLOAD		(WM_USER + 2)
#define WM_USER_FILLCHANGELOG	(WM_USER + 3)

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
	, m_bShowInfo(FALSE)
	, m_bForce(FALSE)
	, m_bVisible(FALSE)
	, m_pDownloadThread(NULL)
	, m_bThreadRunning(FALSE)
{
	m_sUpdateDownloadLink = _T("http://redir.tortoisegit.org/download");
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LINK, m_link);
	DDX_Control(pDX, IDC_PROGRESSBAR, m_progress);
	DDX_Control(pDX, IDC_LIST_DOWNLOADS, m_ctrlFiles);
	DDX_Control(pDX, IDC_BUTTON_UPDATE, m_ctrlUpdate);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
}

BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CStandAloneDialog)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnBnClickedButtonUpdate)
	ON_MESSAGE(WM_USER_DISPLAYSTATUS, OnDisplayStatus)
	ON_MESSAGE(WM_USER_ENDDOWNLOAD, OnEndDownload)
	ON_MESSAGE(WM_USER_FILLCHANGELOG, OnFillChangelog)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::DownloadFile(const CString& url, const CString& dest, bool showProgress)
{
	CString hostname;
	CString urlpath;
	URL_COMPONENTS urlComponents = {0};
	urlComponents.dwStructSize = sizeof(urlComponents);
	urlComponents.lpszHostName = hostname.GetBufferSetLength(INTERNET_MAX_HOST_NAME_LENGTH);
	urlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
	urlComponents.lpszUrlPath = urlpath.GetBufferSetLength(INTERNET_MAX_PATH_LENGTH);
	urlComponents.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
	if (!InternetCrackUrl(url, url.GetLength(), 0, &urlComponents))
		return GetLastError();
	hostname.ReleaseBuffer();
	urlpath.ReleaseBuffer();

	if (m_bForce)
		DeleteUrlCacheEntry(url);

	OSVERSIONINFOEX inf = {0};
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);

	CString userAgent;
	userAgent.Format(L"TortoiseGit %s; %s; Windows%s %d.%d", _T(STRFILEVER), _T(TGIT_PLATFORM), (inf.dwPlatformId == VER_PLATFORM_WIN32_NT) ? _T(" NT") : _T(""), inf.dwMajorVersion, inf.dwMinorVersion);

	HINTERNET hOpenHandle = InternetOpen(userAgent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	HINTERNET hConnectHandle = InternetConnect(hOpenHandle, hostname, urlComponents.nPort, nullptr, nullptr, urlComponents.nScheme, 0, 0);
	if (!hConnectHandle)
	{
		DWORD err = GetLastError();
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s failed on InternetConnect: %d\n"), url, err);
		InternetCloseHandle(hOpenHandle);
		return err;
	}
	HINTERNET hResourceHandle = HttpOpenRequest(hConnectHandle, nullptr, urlpath, nullptr, nullptr, nullptr, INTERNET_FLAG_KEEP_CONNECTION, 0);
	if (!hResourceHandle)
	{
		DWORD err = GetLastError();
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s failed on HttpOpenRequest: %d\n"), url, err);
		InternetCloseHandle(hConnectHandle);
		InternetCloseHandle(hOpenHandle);
		return err;
	}

	{
resend:
		BOOL httpsendrequest = HttpSendRequest(hResourceHandle, nullptr, 0, nullptr, 0);

		DWORD dwError = InternetErrorDlg(GetSafeHwnd(), hResourceHandle, ERROR_SUCCESS, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA, nullptr);

		if (dwError == ERROR_INTERNET_FORCE_RETRY)
			goto resend;
		else if (!httpsendrequest)
		{
			DWORD err = GetLastError();
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s failed: %d, %d\n"), url, httpsendrequest, err);
			InternetCloseHandle(hResourceHandle);
			InternetCloseHandle(hConnectHandle);
			InternetCloseHandle(hOpenHandle);
			return err;
		}
	}

	DWORD contentLength = 0;
	{
		DWORD length = sizeof(contentLength);
		HttpQueryInfo(hResourceHandle, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&contentLength, &length, NULL);
	}
	{
		DWORD statusCode = 0;
		DWORD length = sizeof(statusCode);
		if (!HttpQueryInfo(hResourceHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&statusCode, &length, NULL) || statusCode != 200)
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s returned %d\n"), url, statusCode);
			InternetCloseHandle(hResourceHandle);
			InternetCloseHandle(hConnectHandle);
			InternetCloseHandle(hOpenHandle);
			if (statusCode == 404)
				return ERROR_FILE_NOT_FOUND;
			else if (statusCode == 403)
				return ERROR_ACCESS_DENIED;
			return INET_E_DOWNLOAD_FAILURE;
		}
	}

	CFile destinationFile(dest, CFile::modeCreate | CFile::modeWrite);
	DWORD downloadedSum = 0; // sum of bytes downloaded so far
	do
	{
		DWORD size; // size of the data available
		if (!InternetQueryDataAvailable(hResourceHandle, &size, 0, 0))
		{
			DWORD err = GetLastError();
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s failed on InternetQueryDataAvailable: %d\n"), url, err);
			InternetCloseHandle(hResourceHandle);
			InternetCloseHandle(hConnectHandle);
			InternetCloseHandle(hOpenHandle);
			return err;
		}

		DWORD downloaded; // size of the downloaded data
		LPTSTR lpszData = new TCHAR[size + 1];
		if (!InternetReadFile(hResourceHandle, (LPVOID)lpszData, size, &downloaded))
		{
			delete[] lpszData;
			DWORD err = GetLastError();
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download of %s failed on InternetReadFile: %d\n"), url, err);
			InternetCloseHandle(hResourceHandle);
			InternetCloseHandle(hConnectHandle);
			InternetCloseHandle(hOpenHandle);
			return err;
		}

		if (downloaded == 0)
		{
			delete[] lpszData;
			break;
		}

		lpszData[downloaded] = '\0';
		destinationFile.Write(lpszData, downloaded);
		delete[] lpszData;

		downloadedSum += downloaded;

		if (!showProgress)
			continue;

		if (contentLength == 0) // got no content-length from webserver
		{
			DOWNLOADSTATUS downloadStatus = { 0, 1 + 1 }; // + 1 for download of signature file
			::SendMessage(m_hWnd, WM_USER_DISPLAYSTATUS, 0, reinterpret_cast<LPARAM>(&downloadStatus));
		}
		else
		{
			if (downloadedSum > contentLength)
				downloadedSum = contentLength - 1;
			DOWNLOADSTATUS downloadStatus = { downloadedSum, contentLength + 1 }; // + 1 for download of signature file
			::SendMessage(m_hWnd, WM_USER_DISPLAYSTATUS, 0, reinterpret_cast<LPARAM>(&downloadStatus));
		}

		if (::WaitForSingleObject(m_eventStop, 0) == WAIT_OBJECT_0)
		{
			InternetCloseHandle(hResourceHandle);
			InternetCloseHandle(hConnectHandle);
			InternetCloseHandle(hOpenHandle);
			return E_ABORT; // canceled by the user
		}
	}
	while (true);
	destinationFile.Close();
	InternetCloseHandle(hResourceHandle);
	InternetCloseHandle(hConnectHandle);
	InternetCloseHandle(hOpenHandle);
	if (downloadedSum == 0)
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Download size of %s was zero.\n"), url);
		return INET_E_DOWNLOAD_FAILURE;
	}
	return 0;
}

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
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
	MoveWindow(&rectWindow);
	::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
	GetDlgItem(IDOK)->MoveWindow(&rectOKButton);

	temp.LoadString(IDS_STATUSLIST_COLFILE);
	m_ctrlFiles.InsertColumn(0, temp, 0, -1);
	m_ctrlFiles.InsertColumn(1, temp, 0, -1);
	m_ctrlFiles.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
	m_ctrlFiles.SetColumnWidth(0, 350);
	m_ctrlFiles.SetColumnWidth(1, 200);

	ProjectProperties pp;
	pp.SetCheckRe(_T("[Ii]ssues?:?(\\s*(,|and)?\\s*#?\\d+)+"));
	pp.SetBugIDRe(_T("(\\d+)"));
	pp.lProjectLanguage = -1;
	pp.sUrl = _T("http://code.google.com/p/tortoisegit/issues/detail?id=%BUGID%");
	m_cLogMessage.Init(pp);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));
	m_cLogMessage.Call(SCI_SETREADONLY, TRUE);

	if (AfxBeginThread(CheckThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	SetTimer(100, 1000, NULL);
	return TRUE;
}

void CCheckForUpdatesDlg::OnDestroy()
{
	for (int i = 0; i < m_ctrlFiles.GetItemCount(); ++i)
		delete (CUpdateListCtrl::Entry *)m_ctrlFiles.GetItemData(i);

	CStandAloneDialog::OnDestroy();
}

void CCheckForUpdatesDlg::OnOK()
{
	if (m_bThreadRunning || m_pDownloadThread != NULL)
		return; // Don't exit while downloading

	CStandAloneDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
	if (m_bThreadRunning || m_pDownloadThread != NULL)
		return; // Don't exit while downloading
	CStandAloneDialog::OnCancel();
}

UINT CCheckForUpdatesDlg::CheckThreadEntry(LPVOID pVoid)
{
	return ((CCheckForUpdatesDlg*)pVoid)->CheckThread();
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
				sCheckURL = _T("http://version.tortoisegit.googlecode.com/git/version-preview.txt");
				SetDlgItemText(IDC_SOURCE, _T("Using preview release channel"));
			}
			else
				sCheckURL = _T("http://version.tortoisegit.googlecode.com/git/version.txt");
		}
	}

	if (!official)
		SetDlgItemText(IDC_SOURCE, _T("Using (unofficial) release channel: ") + sCheckURL);

	CString errorText;
	BOOL ret = DownloadFile(sCheckURL, tempfile, false);
	if (!ret && official)
	{
		CString signatureTempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		ret = DownloadFile(sCheckURL + SIGNATURE_FILE_ENDING, signatureTempfile, false);
		if (!ret && VerifyIntegrity(tempfile, signatureTempfile))
		{
			SetDlgItemText(IDC_CHECKRESULT, _T("Could not verify digital signature."));
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
			errorText.Format(IDS_CHECKNEWER_NETERROR_FORMAT, ret);
	}
	if (!ret)
	{
		try
		{
			CStdioFile file(tempfile, CFile::modeRead | CFile::shareDenyWrite);
			CString ver;
			unsigned int major,minor,micro,build;
			major=minor=micro=build=0;
			unsigned __int64 version=0;

			if (file.ReadString(ver))
			{
				CString vertemp = ver;
				// another versionstring for the filename can be provided after a semicolon
				// this is needed for preview releases
				int differentFilenamePos = vertemp.Find(_T(";"));
				if (differentFilenamePos > 0)
				{
					vertemp = vertemp.Left(differentFilenamePos);
					ver = ver.Mid(differentFilenamePos + 1);
				}

				major = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				minor = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				micro = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				build = _ttoi(vertemp);
				version = major;
				version <<= 16;
				version += minor;
				version <<= 16;
				version += micro;
				version <<= 16;
				version += build;
			}

			{
				BOOL bNewer = FALSE;
				if (m_bForce)
					bNewer = TRUE;
				if (major > TGIT_VERMAJOR)
					bNewer = TRUE;
				else if ((minor > TGIT_VERMINOR)&&(major == TGIT_VERMAJOR))
					bNewer = TRUE;
				else if ((micro > TGIT_VERMICRO)&&(minor == TGIT_VERMINOR)&&(major == TGIT_VERMAJOR))
					bNewer = TRUE;
				else if ((build > TGIT_VERBUILD)&&(micro == TGIT_VERMICRO)&&(minor == TGIT_VERMINOR)&&(major == TGIT_VERMAJOR))
					bNewer = TRUE;

				if (version != 0)
				{
					CString version;
					version.Format(_T("%u.%u.%u.%u"),major,minor,micro,build);
					if (version != ver)
						version += _T(" (") + ver + _T(")");
					temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)version);
					SetDlgItemText(IDC_CURRENTVERSION, temp);
				}

				if (version == 0)
				{
					temp.LoadString(IDS_CHECKNEWER_NETERROR);
					SetDlgItemText(IDC_CHECKRESULT, temp);
				}
				else if (bNewer)
				{
					if(file.ReadString(temp) && !temp.IsEmpty())
					{	// Read the next line, it could contain a message for the user
						CString tempLink;
						if(file.ReadString(tempLink) && !tempLink.IsEmpty())
						{	// Read another line to find out the download link-URL, if any
							m_sUpdateDownloadLink = tempLink;
						}
					}
					else
					{
						temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
						CString tempLink;
						file.ReadString(tempLink);
					}
					SetDlgItemText(IDC_CHECKRESULT, temp);
					m_bShowInfo = TRUE;

					FillChangelog(file);
					FillDownloads(file, ver);

					// Show download controls
					RECT rectWindow, rectProgress, rectGroupDownloads, rectOKButton;
					GetWindowRect(&rectWindow);
					m_progress.GetWindowRect(&rectProgress);
					GetDlgItem(IDC_GROUP_DOWNLOADS)->GetWindowRect(&rectGroupDownloads);
					GetDlgItem(IDOK)->GetWindowRect(&rectOKButton);
					LONG bottomDistance = rectWindow.bottom - rectOKButton.bottom;
					OffsetRect(&rectOKButton, 0, (rectGroupDownloads.bottom + (rectGroupDownloads.bottom - rectProgress.bottom)) - rectOKButton.top);
					rectWindow.bottom = rectOKButton.bottom + bottomDistance;
					MoveWindow(&rectWindow);
					::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&rectOKButton, 2);
					GetDlgItem(IDOK)->MoveWindow(&rectOKButton);
					m_ctrlFiles.ShowWindow(SW_SHOW);
					GetDlgItem(IDC_GROUP_DOWNLOADS)->ShowWindow(SW_SHOW);
					CenterWindow();
				}
				else
				{
					temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
					SetDlgItemText(IDC_CHECKRESULT, temp);
					file.ReadString(temp);
					file.ReadString(temp);
					FillChangelog(file);
				}
			}
		}
		catch (CException * e)
		{
			e->Delete();
			temp.LoadString(IDS_CHECKNEWER_NETERROR);
			SetDlgItemText(IDC_CHECKRESULT, temp);
		}
	}
	else
	{
		SetDlgItemText(IDC_CHECKRESULT, errorText);
	}
	if (!m_sUpdateDownloadLink.IsEmpty())
	{
		m_link.ShowWindow(SW_SHOW);
		m_link.SetURL(m_sUpdateDownloadLink);
	}

finish:
	m_bThreadRunning = FALSE;
	DialogEnableWindow(IDOK, TRUE);
	return 0;
}

void CCheckForUpdatesDlg::FillDownloads(CStdioFile &file, CString version)
{
#if WIN64
	const CString x86x64 = _T("64");
#else
	const CString x86x64 = _T("32");
#endif

	if (!file.ReadString(m_sFilesURL) || m_sFilesURL.IsEmpty())
		m_sFilesURL = _T("http://tortoisegit.googlecode.com/files/");

	m_ctrlFiles.InsertItem(0, _T("TortoiseGit"));
	CString filename;
	filename.Format(_T("TortoiseGit-%s-%sbit.msi"), version, x86x64);
	m_ctrlFiles.SetItemData(0, (DWORD_PTR)(new CUpdateListCtrl::Entry(filename, CUpdateListCtrl::STATUS_NONE)));
	m_ctrlFiles.SetCheck(0 , TRUE);

	std::vector<DWORD> m_installedLangs;
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
				m_installedLangs.push_back(loc);
			}
		}
	}

	CString langs;
	while (file.ReadString(langs) && !langs.IsEmpty())
	{
		CString sLang = _T("TortoiseGit Language Pack ") + langs.Mid(5);

		DWORD loc = _tstoi(langs.Mid(0, 4));
		TCHAR buf[MAX_PATH];
		GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		CString sLang2(buf);
		GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
		if (buf[0])
		{
			sLang2 += _T(" (");
			sLang2 += buf;
			sLang2 += _T(")");
		}

		int pos = m_ctrlFiles.InsertItem(m_ctrlFiles.GetItemCount(), sLang);
		m_ctrlFiles.SetItemText(pos, 1, sLang2);

		CString filename;
		filename.Format(_T("TortoiseGit-LanguagePack-%s-%sbit-%s.msi"), version, x86x64, langs.Mid(5));
		m_ctrlFiles.SetItemData(pos, (DWORD_PTR)(new CUpdateListCtrl::Entry(filename, CUpdateListCtrl::STATUS_NONE)));

		if (std::find(m_installedLangs.begin(), m_installedLangs.end(), loc) != m_installedLangs.end())
			m_ctrlFiles.SetCheck(pos , TRUE);
	}
	DialogEnableWindow(IDC_BUTTON_UPDATE, TRUE);
}

void CCheckForUpdatesDlg::FillChangelog(CStdioFile &file)
{
	CString sChangelogURL;
	if (!file.ReadString(sChangelogURL) || sChangelogURL.IsEmpty())
		sChangelogURL = _T("http://tortoisegit.googlecode.com/git/src/Changelog.txt");

	CString tempchangelogfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	if (!DownloadFile(sChangelogURL, tempchangelogfile, false))
	{
		CString temp;
		CStdioFile file(tempchangelogfile, CFile::modeRead|CFile::typeText);
		CString str;
		bool first = true;
		while (file.ReadString(str))
		{
			if (first)
			{
				first = false;
				if (str.GetLength() > 2 && str.GetAt(0) == 0xEF && str.GetAt(1) == 0xBB)
				{
					if (str.GetAt(2) == 0xBF)
					{
						str = str.Mid(3);
					}
					else
					{
						str = str.Mid(2);
					}
				}
			}
			str = CUnicodeUtils::GetUnicode(CStringA(str), CP_UTF8);
			temp += str + _T("\n");
		}
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(temp.GetBuffer()));
	}
	else
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(_T("Could not load changelog.")));
}

void CCheckForUpdatesDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 100)
	{
		if (m_bThreadRunning == FALSE)
		{
			if (m_bShowInfo)
			{
				m_bVisible = TRUE;
				ShowWindow(SW_SHOWNORMAL);
			}
			else
			{
				EndDialog(0);
			}
		}
	}
	CStandAloneDialog::OnTimer(nIDEvent);
}

void CCheckForUpdatesDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CStandAloneDialog::OnWindowPosChanging(lpwndpos);
	if (m_bVisible == FALSE)
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
}

BOOL CCheckForUpdatesDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CStandAloneDialogTmpl<CDialog>::OnSetCursor(pWnd, nHitTest, message);
}

void CCheckForUpdatesDlg::OnBnClickedButtonUpdate()
{
	CString title;
	m_ctrlUpdate.GetWindowText(title);
	if (m_pDownloadThread == NULL && title == CString(MAKEINTRESOURCE(IDS_PROC_DOWNLOAD)))
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
		if (m_pDownloadThread != NULL)
		{
			m_pDownloadThread->m_bAutoDelete = FALSE;
			m_pDownloadThread->ResumeThread();

			GetDlgItem(IDC_BUTTON_UPDATE)->SetWindowText(CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
		}
		else
		{
			CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
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
					ShellExecute(NULL, _T("open"), folder + data->m_filename, NULL, NULL, SW_SHOWNORMAL);
			}
			CStandAloneDialog::OnOK();
		}
		else if (m_ctrlUpdate.GetCurrentEntry() == 1)
		{
			ShellExecute(NULL, _T("open"), folder, NULL, NULL, SW_SHOWNORMAL);
		}

		m_ctrlUpdate.SetCurrentEntry(0);
	}
}

UINT CCheckForUpdatesDlg::DownloadThreadEntry(LPVOID pVoid)
{
	return ((CCheckForUpdatesDlg*)pVoid)->DownloadThread();
}

bool CCheckForUpdatesDlg::Download(CString filename)
{
	CString url = m_sFilesURL + filename;
	CString destFilename = GetDownloadsDirectory() + filename;
	if (PathFileExists(destFilename) && PathFileExists(destFilename + SIGNATURE_FILE_ENDING))
	{
		if (VerifyIntegrity(destFilename, destFilename + SIGNATURE_FILE_ENDING) == 0)
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
	BOOL ret = DownloadFile(url, tempfile, true);
	if (!ret)
	{
		ret = DownloadFile(url + SIGNATURE_FILE_ENDING, signatureTempfile, true);
		m_progress.SetPos(m_progress.GetPos() + 1);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			int minValue, maxValue;
			m_progress.GetRange(minValue, maxValue);
			m_pTaskbarList->SetProgressValue(m_hWnd, m_progress.GetPos(), maxValue);
		}
	}
	if (!ret)
	{
		if (VerifyIntegrity(tempfile, signatureTempfile) == 0)
		{
			DeleteFile(destFilename);
			DeleteFile(destFilename + SIGNATURE_FILE_ENDING);
			MoveFile(tempfile, destFilename);
			MoveFile(signatureTempfile, destFilename + SIGNATURE_FILE_ENDING);
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
	ASSERT(m_pDownloadThread != NULL);

	// wait until the thread terminates
	DWORD dwExitCode;
	if (::GetExitCodeThread(m_pDownloadThread->m_hThread, &dwExitCode) && dwExitCode == STILL_ACTIVE)
		::WaitForSingleObject(m_pDownloadThread->m_hThread, INFINITE);

	// make sure we always have the correct exit code
	::GetExitCodeThread(m_pDownloadThread->m_hThread, &dwExitCode);

	delete m_pDownloadThread;
	m_pDownloadThread = NULL;

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
		CMessageBox::Show(NULL, IDS_ERR_FAILEDUPDATEDOWNLOAD, IDS_APPNAME, MB_ICONERROR);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}

	return 0;
}

LRESULT CCheckForUpdatesDlg::OnFillChangelog(WPARAM, LPARAM lParam)
{
	ASSERT(lParam != NULL);

	TCHAR * changelog = reinterpret_cast<TCHAR *>(lParam);
	m_cLogMessage.Call(SCI_SETREADONLY, FALSE);
	m_cLogMessage.SetText(changelog);
	m_cLogMessage.Call(SCI_SETREADONLY, TRUE);
	m_cLogMessage.Call(SCI_GOTOPOS, 0);

	return 0;
}

CString CCheckForUpdatesDlg::GetDownloadsDirectory()
{
	CString folder;

	if (SysInfo::Instance().IsVistaOrLater())
	{
		CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(_T("shell32.dll"));
		if (hShell)
		{
			typedef HRESULT STDAPICALLTYPE SHGetKnownFolderPathFN(__in REFKNOWNFOLDERID rfid, __in DWORD dwFlags, __in_opt HANDLE hToken, __deref_out PWSTR *ppszPath);
			SHGetKnownFolderPathFN *pfnSHGetKnownFolderPath = (SHGetKnownFolderPathFN*)GetProcAddress(hShell, "SHGetKnownFolderPath");
			if (pfnSHGetKnownFolderPath)
			{
				wchar_t * wcharPtr = 0;
				HRESULT hr = pfnSHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, NULL, &wcharPtr);
				if (SUCCEEDED(hr))
				{
					folder = wcharPtr;
					CoTaskMemFree(static_cast<void*>(wcharPtr));
					return folder.TrimRight(_T("\\")) + _T("\\");
				}
			}
		}
	}

	TCHAR szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szPath)))
		folder = szPath;
	CString downloads = folder.TrimRight(_T("\\")) + _T("\\Downloads\\");
	if ((PathFileExists(downloads) && PathIsDirectory(downloads)) || (!PathFileExists(downloads) && CreateDirectory(downloads, NULL)))
		return downloads;

	return folder;
}

LRESULT CCheckForUpdatesDlg::OnDisplayStatus(WPARAM, LPARAM lParam)
{
	const DOWNLOADSTATUS *const pDownloadStatus = reinterpret_cast<DOWNLOADSTATUS *>(lParam);
	if (pDownloadStatus != NULL)
	{
		ASSERT(::AfxIsValidAddress(pDownloadStatus, sizeof(DOWNLOADSTATUS)));

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
