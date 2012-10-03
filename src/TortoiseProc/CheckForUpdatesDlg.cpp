// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include ".\checkforupdatesdlg.h"
#include "Registry.h"
#include "AppUtils.h"
#include "TempFile.h"
#include <wintrust.h>
#include <Softpub.h>
#include "SmartHandle.h"
#include "SysInfo.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "ProjectProperties.h"

// Link with Wintrust.lib
#pragma comment (lib, "wintrust")

#define WM_USER_DISPLAYSTATUS	(WM_USER + 1)
#define WM_USER_ENDDOWNLOAD		(WM_USER + 2)
#define WM_USER_FILLCHANGELOG	(WM_USER + 3)

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
	, m_bShowInfo(FALSE)
	, m_bVisible(FALSE)
	, m_pDownloadThread(NULL)
{
	m_sUpdateDownloadLink = _T("http://code.google.com/p/tortoisegit/wiki/Download?tm=2");
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
	ON_STN_CLICKED(IDC_CHECKRESULT, OnStnClickedCheckresult)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnBnClickedButtonUpdate)
	ON_MESSAGE(WM_USER_DISPLAYSTATUS, OnDisplayStatus)
	ON_MESSAGE(WM_USER_ENDDOWNLOAD, OnEndDownload)
	ON_MESSAGE(WM_USER_FILLCHANGELOG, OnFillChangelog)
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
	SetDlgItemText(IDC_YOURVERSION, temp);

	DialogEnableWindow(IDOK, FALSE);

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
	pp.lProjectLanguage = 0;
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

	CRegString checkurluser = CRegString(_T("Software\\TortoiseGit\\UpdateCheckURL"), _T(""));
	CRegString checkurlmachine = CRegString(_T("Software\\TortoiseGit\\UpdateCheckURL"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString sCheckURL = checkurluser;
	if (sCheckURL.IsEmpty())
	{
		sCheckURL = checkurlmachine;
		if (sCheckURL.IsEmpty())
		{
			bool checkPreview = false;
#if PREVIEW
			checkPreview = true;
#else
			CRegStdDWORD regCheckPreview(L"Software\\TortoiseGit\\VersionCheckPreview", FALSE);
			if (DWORD(regCheckPreview))
				checkPreview = true;
#endif
			if (checkPreview)
				sCheckURL = _T("http://version.tortoisegit.googlecode.com/git/version-preview.txt");
			else
				sCheckURL = _T("http://version.tortoisegit.googlecode.com/git/version.txt");
		}
	}
	CoInitialize(NULL);
	HRESULT res = URLDownloadToFile(NULL, sCheckURL, tempfile, 0, NULL);
	if (res == S_OK)
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
					version.Format(_T("%d.%d.%d.%d"),major,minor,micro,build);
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
		// Try to cache web page;

		temp.LoadString(IDS_CHECKNEWER_NETERROR);
		SetDlgItemText(IDC_CHECKRESULT, temp);
	}
	if (!m_sUpdateDownloadLink.IsEmpty())
	{
		m_link.ShowWindow(SW_SHOW);
		m_link.SetURL(m_sUpdateDownloadLink);
	}

	DeleteFile(tempfile);
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
				sVer = sVer.Left(sVer.ReverseFind(','));
				CString sFileVer = CPathUtils::GetVersionFromFile(file);
				sFileVer = sFileVer.Left(sFileVer.ReverseFind(','));
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
	HRESULT res = URLDownloadToFile(NULL, sChangelogURL, tempchangelogfile, 0, NULL);
	if (SUCCEEDED(res))
	{
		CString temp;
		CStdioFile file(tempchangelogfile, CFile::modeRead|CFile::typeText);
		CString str;
		while (file.ReadString(str))
		{
			temp += str + _T("\n");
		}
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(temp.GetBuffer()));
	}
	else
		::SendMessage(m_hWnd, WM_USER_FILLCHANGELOG, 0, reinterpret_cast<LPARAM>(_T("Could not load changelog.")));
}

void CCheckForUpdatesDlg::OnStnClickedCheckresult()
{
	// user clicked on the label, start the browser with our web page
	HINSTANCE result = ShellExecute(NULL, _T("opennew"), m_sUpdateDownloadLink, NULL,NULL, SW_SHOWNORMAL);
	if ((UINT)result <= HINSTANCE_ERROR)
	{
		result = ShellExecute(NULL, _T("open"), m_sUpdateDownloadLink, NULL,NULL, SW_SHOWNORMAL);
	}
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
	if ((!m_sUpdateDownloadLink.IsEmpty())&&(pWnd)&&(pWnd == GetDlgItem(IDC_CHECKRESULT)))
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
		SetCursor(hCur);
		return TRUE;
	}

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
		for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); i++)
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
			for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); i++)
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
	CString destFilename = GetDownloadsDirectory() + filename;
	if (PathFileExists(destFilename))
	{
		if (VerifySignature(destFilename))
			return true;
		else
			DeleteFile(destFilename);
	}

	CBSCallbackImpl bsc(this->GetSafeHwnd(), m_eventStop);

	CString url = m_sFilesURL + filename;

	m_progress.SetRange32(0, 1);
	m_progress.SetPos(0);
	m_progress.ShowWindow(SW_SHOW);

	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	HRESULT res = URLDownloadToFile(NULL, url, tempfile, 0, &bsc);
	if (res == S_OK)
	{
		if (VerifySignature(tempfile))
		{
			if (PathFileExists(destFilename))
				DeleteFile(destFilename);
			MoveFile(tempfile, destFilename);
			return true;
		}
	}
	return false;
}

UINT CCheckForUpdatesDlg::DownloadThread()
{
	m_ctrlFiles.SetExtendedStyle(m_ctrlFiles.GetExtendedStyle() & ~LVS_EX_CHECKBOXES);

	BOOL result = TRUE;
	for (int i = 0; i < (int)m_ctrlFiles.GetItemCount(); i++)
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
	}
	else
	{
		m_ctrlUpdate.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_DOWNLOAD)));
		CMessageBox::Show(NULL, IDS_ERR_FAILEDUPDATEDOWNLOAD, IDS_APPNAME, MB_ICONERROR);
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
		CAutoLibrary hShell = ::LoadLibrary(_T("shell32.dll"));
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
	}

	return 0;
}

CBSCallbackImpl::CBSCallbackImpl(HWND hWnd, HANDLE hEventStop)
{
	m_hWnd = hWnd;
	m_hEventStop = hEventStop;
	m_ulObjRefCount = 1;
}

// IUnknown
STDMETHODIMP CBSCallbackImpl::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;
	
	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
		*ppvObject = this;

	// IBindStatusCallback
	else if (::IsEqualIID(riid, __uuidof(IBindStatusCallback)))
		*ppvObject = static_cast<IBindStatusCallback *>(this);

	if (*ppvObject)
	{
		(*reinterpret_cast<LPUNKNOWN *>(ppvObject))->AddRef();
		return S_OK;
	}
	
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CBSCallbackImpl::AddRef()
{
	return ++m_ulObjRefCount;
}

STDMETHODIMP_(ULONG) CBSCallbackImpl::Release()
{
	return --m_ulObjRefCount;
}

// IBindStatusCallback
STDMETHODIMP CBSCallbackImpl::OnStartBinding(DWORD, IBinding *)
{
	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::GetPriority(LONG *)
{
	return E_NOTIMPL;
}
STDMETHODIMP CBSCallbackImpl::OnLowResource(DWORD)
{
	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR /* szStatusText */)
{
	TRACE(_T("IBindStatusCallback::OnProgress\n"));
	TRACE(_T("ulProgress: %lu, ulProgressMax: %lu\n"), ulProgress, ulProgressMax);
	UNREFERENCED_PARAMETER(ulStatusCode);
	TRACE(_T("ulStatusCode: %lu "), ulStatusCode);

	if (m_hWnd != NULL)
	{
		// inform the dialog box to display current status,
		// don't use PostMessage
		CCheckForUpdatesDlg::DOWNLOADSTATUS downloadStatus = { ulProgress, ulProgressMax };
		::SendMessage(m_hWnd, WM_USER_DISPLAYSTATUS, 0, reinterpret_cast<LPARAM>(&downloadStatus));
	}

	if (m_hEventStop != NULL && ::WaitForSingleObject(m_hEventStop, 0) == WAIT_OBJECT_0)
	{
		return E_ABORT;  // canceled by the user
	}

	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::OnStopBinding(HRESULT, LPCWSTR)
{
	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::GetBindInfo(DWORD *, BINDINFO *)
{
	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::OnDataAvailable(DWORD, DWORD, FORMATETC *, STGMEDIUM *)
{
	return S_OK;
}

STDMETHODIMP CBSCallbackImpl::OnObjectAvailable(REFIID, IUnknown *)
{
	return S_OK;
}

BOOL CCheckForUpdatesDlg::VerifySignature(CString fileName)
{
	WINTRUST_FILE_INFO fileInfo;
	memset(&fileInfo, 0, sizeof(fileInfo));
	fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
	fileInfo.pcwszFilePath = fileName;

	WINTRUST_DATA data;
	memset(&data, 0, sizeof(data));
	data.cbStruct = sizeof(data);
	
	data.dwUIChoice = WTD_UI_NONE;
	data.fdwRevocationChecks = WTD_REVOKE_NONE; 
	data.dwUnionChoice = WTD_CHOICE_FILE;
	data.pFile = &fileInfo;

	GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	return (WinVerifyTrust(NULL, &policyGUID, &data) == ERROR_SUCCESS);
}
