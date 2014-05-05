// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009,2011-2014 - TortoiseGit
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
#include "SysProgressDlg.h"

static CString UnescapeQuotePath(CString s)
{
	CStringA t;
	for (int i = 0; i < s.GetLength(); ++i)
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

struct SubmodulePayload
{
	STRING_VECTOR &list;
	CString basePath;
	STRING_VECTOR prefixList;
	SubmodulePayload(STRING_VECTOR &alist, CString abasePath = _T(""), STRING_VECTOR aprefixList = STRING_VECTOR())
		: list(alist)
		, basePath(abasePath)
		, prefixList(aprefixList)
	{
	}
};

static bool GetSubmodulePathList(SubmodulePayload &payload);

static int SubmoduleCallback(git_submodule *sm, const char * /*name*/, void *payload)
{
	auto spayload = (SubmodulePayload *)payload;
	CString path = CUnicodeUtils::GetUnicode(git_submodule_path(sm));
	if (spayload->prefixList.empty())
	{
		CTGitPath subPath(spayload->basePath);
		subPath.AppendPathString(path);
		spayload->list.push_back(subPath.GetGitPathString());
		SubmodulePayload tpayload(spayload->list, subPath.GetGitPathString());
		GetSubmodulePathList(tpayload);
	}
	else
	{
		for (size_t i = 0; i < spayload->prefixList.size(); ++i)
		{
			CString prefix = spayload->prefixList.at(i) + _T("/");
			if (path.Left(prefix.GetLength()) == prefix)
			{
				CTGitPath subPath(spayload->basePath);
				subPath.AppendPathString(path);
				spayload->list.push_back(subPath.GetGitPathString());
				SubmodulePayload tpayload(spayload->list, subPath.GetGitPathString());
				GetSubmodulePathList(tpayload);
			}
		}
	}
	return 0;
}

static bool GetSubmodulePathList(SubmodulePayload &payload)
{
	git_repository *repo;
	if (git_repository_open(&repo, CUnicodeUtils::GetUTF8(payload.basePath)))
	{
		// Silence the warning message, submodule may not be initialized yet.
		return false;
	}

	if (git_submodule_foreach(repo, SubmoduleCallback, &payload))
	{
		git_repository_free(repo);
		MessageBox(nullptr, CGit::GetLibGit2LastErr(_T("Could not get submodule list.")), _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	git_repository_free(repo);
	return true;
}

bool CleanupCommand::Execute()
{
	bool bRet = false;

	CCleanTypeDlg dlg;
	if( dlg.DoModal() == IDOK)
	{
		bool quotepath = g_Git.GetConfigValueBool(_T("core.quotepath"));

		CString cmd;
		cmd.Format(_T("git.exe clean"));
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

		STRING_VECTOR submoduleList;
		SubmodulePayload payload(submoduleList);
		if (dlg.m_bSubmodules)
		{
			payload.basePath = CTGitPath(g_Git.m_CurrentDir).GetGitPathString();
			if (pathList.GetCount() != 1 || pathList.GetCount() == 1 && !pathList[0].IsEmpty())
			{
				for (int i = 0; i < pathList.GetCount(); ++i)
				{
					CString path;
					if (pathList[i].IsDirectory())
						payload.prefixList.push_back(pathList[i].GetGitPathString());
					else
						payload.prefixList.push_back(pathList[i].GetContainingDirectory().GetGitPathString());
				}
			}
			if (!GetSubmodulePathList(payload))
				return FALSE;
			std::sort(submoduleList.begin(), submoduleList.end());
		}

		if (dlg.m_bDryRun || dlg.m_bNoRecycleBin)
		{
			while (true)
			{
				CProgressDlg progress;
				for (int i = 0; i < this->pathList.GetCount(); ++i)
				{
					CString path;
					if (this->pathList[i].IsDirectory())
						path = pathList[i].GetGitPathString();
					else
						path = pathList[i].GetContainingDirectory().GetGitPathString();

					progress.m_GitDirList.push_back(g_Git.m_CurrentDir);
					progress.m_GitCmdList.push_back(cmd + _T(" \"") + path + _T("\""));
				}

				if (dlg.m_bSubmodules)
				{
					for (CString dir : submoduleList)
					{
						progress.m_GitDirList.push_back(CTGitPath(dir).GetWinPathString());
						progress.m_GitCmdList.push_back(cmd);
					}
				}

				INT_PTR idRetry = -1;
				if (!dlg.m_bDryRun)
					idRetry = progress.m_PostFailCmdList.Add(CString(MAKEINTRESOURCE(IDS_MSGBOX_RETRY)));
				INT_PTR result = progress.DoModal();
				if (result == IDOK)
					return TRUE;
				if (progress.m_GitStatus && result == IDC_PROGRESS_BUTTON1 + idRetry)
					continue;
				break;
			}
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

			CTGitPathList delList;
			for (size_t i = 0; i <= submoduleList.size(); ++i)
			{
				CGit git;
				CGit *pGit;
				if (i == 0)
					pGit = &g_Git;
				else
				{
					git.m_CurrentDir = submoduleList[i - 1];
					pGit = &git;
				}
				CString cmdout, cmdouterr;
				if (pGit->Run(cmd, &cmdout, &cmdouterr, CP_UTF8))
				{
					MessageBox(nullptr, cmdouterr, _T("TortoiseGit"), MB_ICONERROR);
					return FALSE;
				}

				if (sysProgressDlg.HasUserCancelled())
				{
					CMessageBox::Show(nullptr, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_OK);
					return FALSE;
				}

				int pos = 0;
				CString token = cmdout.Tokenize(_T("\n"), pos);
				while (!token.IsEmpty())
				{
					if (token.Mid(0, 13) == _T("Would remove "))
					{
						CString tempPath = token.Mid(13).TrimRight();
						if (quotepath)
						{
							tempPath = UnescapeQuotePath(tempPath.Trim(_T('"')));
						}
						if (i == 0)
							delList.AddPath(CTGitPath(tempPath));
						else
							delList.AddPath(CTGitPath(submoduleList[i - 1] + "/" + tempPath));
					}

					token = cmdout.Tokenize(_T("\n"), pos);
				}

				if (sysProgressDlg.HasUserCancelled())
				{
					CMessageBox::Show(nullptr, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_OK);
					return FALSE;
				}
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
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": notify change for path %s\n"), updateList[i].GetWinPath());
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
