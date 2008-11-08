// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "RelocateCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RelocateDlg.h"
#include "SVN.h"
#include "MessageBox.h"
#include "PathUtils.h"

bool RelocateCommand::Execute()
{
	bool bRet = false;
	SVN svn;
	CRelocateDlg dlg;
	dlg.m_sFromUrl = CPathUtils::PathUnescape(svn.GetURLFromPath(cmdLinePath));
	dlg.m_sToUrl = dlg.m_sFromUrl;

	if (dlg.DoModal() == IDOK)
	{
		TRACE(_T("relocate from %s to %s\n"), (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
		// crack the urls into their components
		TCHAR urlpath1[INTERNET_MAX_URL_LENGTH+1];
		TCHAR scheme1[INTERNET_MAX_URL_LENGTH+1];
		TCHAR hostname1[INTERNET_MAX_URL_LENGTH+1];
		TCHAR username1[INTERNET_MAX_URL_LENGTH+1];
		TCHAR password1[INTERNET_MAX_URL_LENGTH+1];
		TCHAR urlpath2[INTERNET_MAX_URL_LENGTH+1];
		TCHAR scheme2[INTERNET_MAX_URL_LENGTH+1];
		TCHAR hostname2[INTERNET_MAX_URL_LENGTH+1];
		TCHAR username2[INTERNET_MAX_URL_LENGTH+1];
		TCHAR password2[INTERNET_MAX_URL_LENGTH+1];
		URL_COMPONENTS components1 = {0};
		URL_COMPONENTS components2 = {0};
		components1.dwStructSize = sizeof(URL_COMPONENTS);
		components1.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
		components1.lpszUrlPath = urlpath1;
		components1.lpszScheme = scheme1;
		components1.dwSchemeLength = INTERNET_MAX_URL_LENGTH;
		components1.lpszHostName = hostname1;
		components1.dwHostNameLength = INTERNET_MAX_URL_LENGTH;
		components1.lpszUserName = username1;
		components1.dwUserNameLength = INTERNET_MAX_URL_LENGTH;
		components1.lpszPassword = password1;
		components1.dwPasswordLength = INTERNET_MAX_URL_LENGTH;
		components2.dwStructSize = sizeof(URL_COMPONENTS);
		components2.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
		components2.lpszUrlPath = urlpath2;
		components2.lpszScheme = scheme2;
		components2.dwSchemeLength = INTERNET_MAX_URL_LENGTH;
		components2.lpszHostName = hostname2;
		components2.dwHostNameLength = INTERNET_MAX_URL_LENGTH;
		components2.lpszUserName = username2;
		components2.dwUserNameLength = INTERNET_MAX_URL_LENGTH;
		components2.lpszPassword = password2;
		components2.dwPasswordLength = INTERNET_MAX_URL_LENGTH;
		CString sTempUrl = dlg.m_sFromUrl;
		if (sTempUrl.Left(8).Compare(_T("file:///\\"))==0)
			sTempUrl.Replace(_T("file:///\\"), _T("file://"));
		InternetCrackUrl((LPCTSTR)sTempUrl, sTempUrl.GetLength(), 0, &components1);
		sTempUrl = dlg.m_sToUrl;
		if (sTempUrl.Left(8).Compare(_T("file:///\\"))==0)
			sTempUrl.Replace(_T("file:///\\"), _T("file://"));
		InternetCrackUrl((LPCTSTR)sTempUrl, sTempUrl.GetLength(), 0, &components2);
		// now compare the url components.
		// If the 'main' parts differ (e.g. hostname, port, scheme, ...) then a relocate is
		// necessary and we don't show a warning. But if only the path part of the url
		// changed, we assume the user really wants to switch and show the warning.
		bool bPossibleSwitch = true;
		if (components1.dwSchemeLength != components2.dwSchemeLength)
			bPossibleSwitch = false;
		else if (_tcsncmp(components1.lpszScheme, components2.lpszScheme, components1.dwSchemeLength))
			bPossibleSwitch = false;
		if (components1.dwHostNameLength != components2.dwHostNameLength)
			bPossibleSwitch = false;
		else if (_tcsncmp(components1.lpszHostName, components2.lpszHostName, components1.dwHostNameLength))
			bPossibleSwitch = false;
		if (components1.dwUserNameLength != components2.dwUserNameLength)
			bPossibleSwitch = false;
		else if (_tcsncmp(components1.lpszUserName, components2.lpszUserName, components1.dwUserNameLength))
			bPossibleSwitch = false;
		if (components1.dwPasswordLength != components2.dwPasswordLength)
			bPossibleSwitch = false;
		else if (_tcsncmp(components1.lpszPassword, components2.lpszPassword, components1.dwPasswordLength))
			bPossibleSwitch = false;
		if (components1.nPort != components2.nPort)
			bPossibleSwitch = false;
		CString sWarning, sWarningTitle, sHelpPath;
		sWarning.Format(IDS_WARN_RELOCATEREALLY, (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
		sWarningTitle.LoadString(IDS_WARN_RELOCATEREALLYTITLE);
		sHelpPath = theApp.m_pszHelpFilePath;
		sHelpPath += _T("::/tsvn-dug-relocate.html");
		if ((!bPossibleSwitch)||(CMessageBox::Show((hwndExplorer), sWarning, sWarningTitle, MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2|MB_HELP, sHelpPath)==IDYES))
		{
			SVN s;

			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_RELOCATING);
				progress.ShowModeless(hwndExplorer);
			}
			if (!s.Relocate(cmdLinePath, CTSVNPath(dlg.m_sFromUrl), CTSVNPath(dlg.m_sToUrl), TRUE))
			{
				progress.Stop();
				TRACE(_T("%s\n"), (LPCTSTR)s.GetLastErrorMessage());
				CMessageBox::Show(hwndExplorer, s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				progress.Stop();
				CString strMessage;
				strMessage.Format(IDS_PROC_RELOCATEFINISHED, (LPCTSTR)dlg.m_sToUrl);
				CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseSVN"), MB_ICONINFORMATION);
				bRet = true;
			}
		}
	}
	return bRet;
}
