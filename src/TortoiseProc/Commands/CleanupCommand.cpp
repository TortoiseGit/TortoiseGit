// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009,2011-2012 - TortoiseGit
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
#include "stdafx.h"
#include "CleanupCommand.h"

#include "MessageBox.h"
#include "ProgressDlg.h"
#include "ShellUpdater.h"
#include "CleanTypeDlg.h"
#include "..\Utils\UnicodeUtils.h"
#include "ProjectProperties.h"
#include "SysProgressDlg.h"

static CString UnescapeQuotePath(CString s)
{
	CStringA t;
	for (int i = 0; i < s.GetLength(); i++)
	{
		if (s[i] == '\\' && i + 3 < s.GetLength())
		{
			char c = (char)((s[i + 1] - '0') * 64 + (s[i + 2] - '0') * 8 + (s[i + 3] - '0'));
			t += c;
			i += 3;
		}
		else
		{
			t += s[i];
		}
	}

	return CUnicodeUtils::GetUnicode(t);
}

bool CleanupCommand::Execute()
{
	bool bRet = false;

	CCleanTypeDlg dlg;
	if( dlg.DoModal() == IDOK)
	{
		ProjectProperties pp;
		BOOL quotepath = TRUE;
		pp.GetBOOLProps(quotepath, _T("core.quotepath"));

		CString cmd;
		cmd.Format(_T("git clean"));
		if (dlg.m_bDryRun || !dlg.m_bNoRecycleBin)
			cmd += _T(" -n ");
		if(dlg.m_bDir)
			cmd += _T(" -d ");
		switch(dlg.m_CleanType)
		{
		case 0:
			cmd += _T(" -fx");
			break;
		case 1:
			cmd += _T(" -f");
			break;
		case 2:
			cmd += _T(" -fX");
			break;
		}

		if (dlg.m_bDryRun || dlg.m_bNoRecycleBin)
		{
			CProgressDlg progress;
			for (int i = 0; i < this->pathList.GetCount(); i++)
			{
				CString path;
				if (this->pathList[i].IsDirectory())
					path = pathList[i].GetGitPathString();
				else
					path = pathList[i].GetContainingDirectory().GetGitPathString();

				progress.m_GitCmdList.push_back(cmd + _T(" \"") + path + _T("\""));
			}
			if (progress.DoModal()==IDOK)
				return TRUE;
		}
		else
		{
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetAnimation(IDR_CLEANUPANI);
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO1)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.ShowModeless((HWND)NULL, true);

			CString cmdout, cmdouterr;
			if (g_Git.Run(cmd, &cmdout, &cmdouterr, CP_UTF8)) {
				MessageBox(NULL, cmdouterr, _T("TortoiseGit"), MB_ICONERROR);
				return FALSE;
			}

			if (sysProgressDlg.HasUserCancelled())
			{
				CMessageBox::Show(NULL, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_OK);
				return FALSE;
			}

			int pos = 0;
			CString token = cmdout.Tokenize(_T("\n"), pos);
			CTGitPathList delList;
			while (!token.IsEmpty())
			{
				if (token.Mid(0, 13) == _T("Would remove "))
				{
					CString tempPath = token.Mid(13).TrimRight();
					if (quotepath)
					{
						tempPath = UnescapeQuotePath(tempPath.Trim(_T('"')));
					}
					delList.AddPath(CTGitPath(tempPath));
				}

				token = cmdout.Tokenize(_T("\n"), pos);
			}

			if (sysProgressDlg.HasUserCancelled())
			{
				CMessageBox::Show(NULL, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_OK);
				return FALSE;
			}

			delList.DeleteAllFiles(true, false);

			sysProgressDlg.Stop();
		}
	}
#if 0
	CProgressDlg progress;
	progress.SetTitle(IDS_PROC_CLEANUP);
	progress.SetAnimation(IDR_CLEANUPANI);
	progress.SetShowProgressBar(false);
	progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO1)));
	progress.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
	progress.ShowModeless(hwndExplorer);

	CString strSuccessfullPaths, strFailedPaths;
	for (int i=0; i<pathList.GetCount(); ++i)
	{
		SVN svn;
		if (!svn.CleanUp(pathList[i]))
		{
			strFailedPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");
			strFailedPaths += svn.GetLastErrorMessage() + _T("\n\n");
		}
		else
		{
			strSuccessfullPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");

			// after the cleanup has finished, crawl the path downwards and send a change
			// notification for every directory to the shell. This will update the
			// overlays in the left tree view of the explorer.
			CDirFileEnum crawler(pathList[i].GetWinPathString());
			CString sPath;
			bool bDir = false;
			CTSVNPathList updateList;
			while (crawler.NextFile(sPath, &bDir))
			{
				if ((bDir) && (!g_SVNAdminDir.IsAdminDirPath(sPath)))
				{
					updateList.AddPath(CTSVNPath(sPath));
				}
			}
			updateList.AddPath(pathList[i]);
			CShellUpdater::Instance().AddPathsForUpdate(updateList);
			CShellUpdater::Instance().Flush();
			updateList.SortByPathname(true);
			for (INT_PTR i=0; i<updateList.GetCount(); ++i)
			{
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, updateList[i].GetWinPath(), NULL);
				ATLTRACE(_T("notify change for path %s\n"), updateList[i].GetWinPath());
			}
		}
	}
	progress.Stop();

	CString strMessage;
	if ( !strSuccessfullPaths.IsEmpty() )
	{
		CString tmp;
		tmp.Format(IDS_PROC_CLEANUPFINISHED, (LPCTSTR)strSuccessfullPaths);
		strMessage += tmp;
		bRet = true;
	}
	if ( !strFailedPaths.IsEmpty() )
	{
		if (!strMessage.IsEmpty())
			strMessage += _T("\n");
		CString tmp;
		tmp.Format(IDS_PROC_CLEANUPFINISHED_FAILED, (LPCTSTR)strFailedPaths);
		strMessage += tmp;
		bRet = false;
	}
	CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseGit"), MB_OK | (strFailedPaths.IsEmpty()?MB_ICONINFORMATION:MB_ICONERROR));
#endif
	CShellUpdater::Instance().Flush();
	return bRet;
}
