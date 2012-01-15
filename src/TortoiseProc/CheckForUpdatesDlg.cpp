// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2011 - TortoiseGit

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


IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
	, m_bShowInfo(FALSE)
	, m_bVisible(FALSE)
{
	m_sUpdateDownloadLink = _T("http://code.google.com/p/tortoisegit/downloads");
	m_sUpdateChangeLogLink = _T("http://code.google.com/p/tortoisegit/wiki/ReleaseNotes");
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LINK, m_link);
	DDX_Control(pDX, IDC_LINK_CHANGE_LOG, m_ChangeLogLink);
}


BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CStandAloneDialog)
	ON_STN_CLICKED(IDC_CHECKRESULT, OnStnClickedCheckresult)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
	SetDlgItemText(IDC_YOURVERSION, temp);

	DialogEnableWindow(IDOK, FALSE);

	if (AfxBeginThread(CheckThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	SetTimer(100, 1000, NULL);
	return TRUE;
}

void CCheckForUpdatesDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;
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
			sCheckURL = _T("http://code.google.com/p/tortoisegit/downloads/list");
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

			if(file.GetLength()>100)
			{
				while(file.ReadString(ver))
				{
					ver.Trim().MakeLower();
					int start;
					while( (start=ver.Find(_T("tortoisegit-"))) > 0 )
					{
						ver = ver.Mid(start+CString(_T("tortoisegit-")).GetLength());
						unsigned int x1,x2,x3,x4;
						x1=_ttoi(ver)&0xFFFF;
						ver = ver.Mid(ver.Find('.')+1);
						x2=_ttoi(ver)&0xFFFF;
						ver = ver.Mid(ver.Find('.')+1);
						x3=_ttoi(ver)&0xFFFF;
						ver = ver.Mid(ver.Find('.')+1);
						x4=_ttoi(ver)&0xFFFF;

						unsigned __int64 newversion;
						newversion = x1;
						newversion <<= 16;
						newversion += x2;
						newversion <<= 16;
						newversion += x3;
						newversion <<=16;
						newversion += x4;

						if(newversion>version)
						{
							major=x1;
							minor=x2;
							micro=x3;
							build=x4;
							version = newversion;
						}
					}
				}

			}
			else if (file.ReadString(ver))
			{
				CString vertemp = ver;
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
					ver.Format(_T("%d.%d.%d.%d"),major,minor,micro,build);
					temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)ver);
					SetDlgItemText(IDC_CURRENTVERSION, temp);
					temp.Format(_T("%d.%d.%d.%d"), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
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
					}
					SetDlgItemText(IDC_CHECKRESULT, temp);
					m_bShowInfo = TRUE;
				}
				else
				{
					temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
					SetDlgItemText(IDC_CHECKRESULT, temp);
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
	if (!m_sUpdateDownloadLink.IsEmpty())
	{
		m_ChangeLogLink.ShowWindow(SW_SHOW);
		m_ChangeLogLink.SetURL(m_sUpdateChangeLogLink);
	}

	DeleteFile(tempfile);
	m_bThreadRunning = FALSE;
	DialogEnableWindow(IDOK, TRUE);
	return 0;
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
