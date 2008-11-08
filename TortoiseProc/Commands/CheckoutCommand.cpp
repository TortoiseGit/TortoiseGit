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
#include "CheckoutCommand.h"

#include "CheckoutDlg.h"
#include "SVNProgressDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"

bool CheckoutCommand::Execute()
{
	bool bRet = false;
	// Get the directory supplied in the command line. If there isn't
	// one then we should use first the default checkout path
	// specified in the settings dialog, and fall back to the current 
	// working directory instead if no such path was specified.
	CTSVNPath checkoutDirectory;
	CRegString regDefCheckoutPath(_T("Software\\TortoiseSVN\\DefaultCheckoutPath"));
	if (cmdLinePath.IsEmpty())
	{
		if (CString(regDefCheckoutPath).IsEmpty())
		{
			checkoutDirectory.SetFromWin(sOrigCWD, true);
			DWORD len = ::GetTempPath(0, NULL);
			TCHAR * tszPath = new TCHAR[len];
			::GetTempPath(len, tszPath);
			if (_tcsncicmp(checkoutDirectory.GetWinPath(), tszPath, len-2 /* \\ and \0 */) == 0)
			{
				// if the current directory is set to a temp directory,
				// we don't use that but leave it empty instead.
				checkoutDirectory.Reset();
			}
			delete [] tszPath;
		}
		else
		{
			checkoutDirectory.SetFromWin(CString(regDefCheckoutPath));
		}
	}
	else
	{
		checkoutDirectory = cmdLinePath;
	}

	CCheckoutDlg dlg;
	dlg.m_strCheckoutDirectory = checkoutDirectory.GetWinPathString();
	dlg.m_URL = parser.GetVal(_T("url"));
	// if there is no url specified on the command line, check if there's one
	// specified in the settings dialog to use as the default and use that
	CRegString regDefCheckoutUrl(_T("Software\\TortoiseSVN\\DefaultCheckoutUrl"));
	if (!CString(regDefCheckoutUrl).IsEmpty())
	{
		// if the URL specified is a child of the default URL, we also
		// adjust the default checkout path
		// e.g.
		// Url specified on command line: http://server.com/repos/project/trunk/folder
		// Url specified as default     : http://server.com/repos/project/trunk
		// checkout path specified      : c:\work\project
		// -->
		// checkout path adjusted       : c:\work\project\folder
		CTSVNPath clurl = CTSVNPath(dlg.m_URL);
		CTSVNPath defurl = CTSVNPath(CString(regDefCheckoutUrl));
		if (defurl.IsAncestorOf(clurl))
		{
			// the default url is the parent of the specified url
			if (CTSVNPath::CheckChild(CTSVNPath(CString(regDefCheckoutPath)), CTSVNPath(dlg.m_strCheckoutDirectory)))
			{
				dlg.m_strCheckoutDirectory = CString(regDefCheckoutPath) + clurl.GetWinPathString().Mid(defurl.GetWinPathString().GetLength());
				dlg.m_strCheckoutDirectory.Replace(_T("\\\\"), _T("\\"));
			}
		}
		if (dlg.m_URL.IsEmpty())
			dlg.m_URL = regDefCheckoutUrl;
	}
	if (dlg.m_URL.Left(5).Compare(_T("tsvn:"))==0)
	{
		dlg.m_URL = dlg.m_URL.Mid(5);
		if (dlg.m_URL.Find('?') >= 0)
		{
			dlg.Revision = SVNRev(dlg.m_URL.Mid(dlg.m_URL.Find('?')+1));
			dlg.m_URL = dlg.m_URL.Left(dlg.m_URL.Find('?'));
		}
	}
	if (parser.HasKey(_T("revision")))
	{
		SVNRev Rev = SVNRev(parser.GetVal(_T("revision")));
		dlg.Revision = Rev;
	}
	if (dlg.m_URL.Find('*')>=0)
	{
		// multiple URL's specified
		// ask where to check them out to
		CBrowseFolder foldbrowse;
		foldbrowse.SetInfo(CString(MAKEINTRESOURCE(IDS_PROC_CHECKOUTTO)));
		foldbrowse.SetCheckBoxText(CString(MAKEINTRESOURCE(IDS_PROC_CHECKOUTTOPONLY)));
		foldbrowse.SetCheckBoxText2(CString(MAKEINTRESOURCE(IDS_PROC_CHECKOUTNOEXTERNALS)));
		foldbrowse.m_style = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE;
		TCHAR checkoutpath[MAX_PATH];
		if (foldbrowse.Show(hwndExplorer, checkoutpath, MAX_PATH, CString(regDefCheckoutPath))==CBrowseFolder::OK)
		{
			CSVNProgressDlg progDlg;
			theApp.m_pMainWnd = &progDlg;
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Checkout);
			progDlg.SetOptions(foldbrowse.m_bCheck2 ? ProgOptIgnoreExternals : ProgOptNone);
			progDlg.SetPathList(CTSVNPathList(CTSVNPath(CString(checkoutpath))));
			progDlg.SetUrl(dlg.m_URL);
			progDlg.SetRevision(dlg.Revision);
			progDlg.SetDepth(foldbrowse.m_bCheck ? svn_depth_empty : svn_depth_infinity);
			progDlg.DoModal();
			bRet = !progDlg.DidErrorsOccur();
		}
	}
	else if (dlg.DoModal() == IDOK)
	{
		checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

		CSVNProgressDlg progDlg;
		theApp.m_pMainWnd = &progDlg;
		progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Checkout);
		if (parser.HasVal(_T("closeonend")))
			progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
		progDlg.SetOptions(dlg.m_bNoExternals ? ProgOptIgnoreExternals : ProgOptNone);
		progDlg.SetPathList(CTSVNPathList(checkoutDirectory));
		progDlg.SetUrl(dlg.m_URL);
		progDlg.SetRevision(dlg.Revision);
		progDlg.SetDepth(dlg.m_depth);
		progDlg.DoModal();
		bRet = !progDlg.DidErrorsOccur();
	}
	return bRet;
}
