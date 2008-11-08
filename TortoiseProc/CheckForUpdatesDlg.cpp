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
	m_sUpdateDownloadLink = _T("http://tortoisesvn.tigris.org");
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LINK, m_link);
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

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
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

	CRegString checkurluser = CRegString(_T("Software\\TortoiseSVN\\UpdateCheckURL"), _T(""));
	CRegString checkurlmachine = CRegString(_T("Software\\TortoiseSVN\\UpdateCheckURL"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString sCheckURL = checkurluser;
	if (sCheckURL.IsEmpty())
	{
		sCheckURL = checkurlmachine;
		if (sCheckURL.IsEmpty())
			sCheckURL = _T("http://tortoisesvn.tigris.org/version.txt");
	}
	HRESULT res = URLDownloadToFile(NULL, sCheckURL, tempfile, 0, NULL);
	if (res == S_OK)
	{
		try
		{
			CStdioFile file(tempfile, CFile::modeRead | CFile::shareDenyWrite);
			CString ver;
			if (file.ReadString(ver))
			{
				CString vertemp = ver;
				int major = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int minor = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int micro = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int build = _ttoi(vertemp);
				BOOL bNewer = FALSE;
				if (major > TSVN_VERMAJOR)
					bNewer = TRUE;
				else if ((minor > TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;
				else if ((micro > TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;
				else if ((build > TSVN_VERBUILD)&&(micro == TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;

				if (_ttoi(ver)!=0)
				{
					temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)ver);
					SetDlgItemText(IDC_CURRENTVERSION, temp);
					temp.Format(_T("%d.%d.%d.%d"), TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
				}

				if (_ttoi(ver)==0)
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
