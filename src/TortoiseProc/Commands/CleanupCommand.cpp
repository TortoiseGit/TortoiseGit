// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011-2019 - TortoiseGit
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
#include "../Utils/UnicodeUtils.h"
#include "SysProgressDlg.h"

static CString UnescapeQuotePath(CString s)
{
	CStringA t;
	for (int i = 0; i < s.GetLength(); ++i)
	{
		if (s[i] == '\\' && i + 3 < s.GetLength())
		{
			char c = static_cast<char>((s[i + 1] - '0') * 64 + (s[i + 2] - '0') * 8 + (s[i + 3] - '0'));
			t += c;
			i += 3;
		}
		else
			t += s[i];
	}

	return CUnicodeUtils::GetUnicode(t);
}

struct SubmodulePayload
{
	STRING_VECTOR &list;
	CString basePath;
	STRING_VECTOR prefixList;
	SubmodulePayload(STRING_VECTOR& alist, CString abasePath = L"", const STRING_VECTOR& aprefixList = STRING_VECTOR())
		: list(alist)
		, basePath(abasePath)
		, prefixList(aprefixList)
	{
	}
};

static bool GetSubmodulePathList(SubmodulePayload &payload);

static int SubmoduleCallback(git_submodule *sm, const char * /*name*/, void *payload)
{
	auto spayload = reinterpret_cast<SubmodulePayload*>(payload);
	CString path = CUnicodeUtils::GetUnicode(git_submodule_path(sm));
	CString fullPath(spayload->basePath);
	fullPath += L'\\';
	fullPath += path;
	if (!PathIsDirectory(fullPath))
		return 0;
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
			CString prefix = spayload->prefixList.at(i) + L'/';
			if (CStringUtils::StartsWith(path, prefix))
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
	CAutoRepository repo(payload.basePath);
	if (!repo)
	{
		// Silence the warning message, submodule may not be initialized yet.
		return false;
	}

	if (git_submodule_foreach(repo, SubmoduleCallback, &payload))
	{
		MessageBox(GetExplorerHWND(), CGit::GetLibGit2LastErr(L"Could not get submodule list."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	return true;
}

static bool GetFilesToCleanUp(CTGitPathList& delList, const CString& baseCmd, CGit *pGit, const CString& path, const boolean quotepath, CSysProgressDlg& sysProgressDlg)
{
	CString cmd(baseCmd);
	if (!path.IsEmpty())
		cmd += L" -- \"" + path + L'"';

	CString cmdout, cmdouterr;
	if (pGit->Run(cmd, &cmdout, &cmdouterr, CP_UTF8))
	{
		if (cmdouterr.IsEmpty())
			cmdouterr.Format(IDS_GITEXEERROR_NOMESSAGE, static_cast<LPCTSTR>(cmdout));
		MessageBox(GetExplorerHWND(), cmdouterr, L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	if (sysProgressDlg.HasUserCancelled())
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_USERCANCELLED, IDS_APPNAME, MB_OK);
		return false;
	}

	int pos = 0;
	CString token = cmdout.Tokenize(L"\n", pos);
	while (!token.IsEmpty())
	{
		if (CStringUtils::StartsWith(token, L"Would remove "))
		{
			CString tempPath = token.Mid(static_cast<int>(wcslen(L"Would remove "))).TrimRight();
			if (quotepath)
				tempPath = UnescapeQuotePath(tempPath.Trim(L'"'));
			delList.AddPath(pGit->CombinePath(tempPath));
		}

		token = cmdout.Tokenize(L"\n", pos);
	}

	if (sysProgressDlg.HasUserCancelled())
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_USERCANCELLED, IDS_APPNAME, MB_OK);
		return false;
	}

	return true;
}

static bool DoCleanUp(const CTGitPathList& pathList, int cleanType, bool bDir, bool bDirUnmanagedRepos, bool bSubmodules, bool bDryRun, bool bNoRecycleBin)
{
	CString cmd;
	cmd.Format(L"git.exe clean");
	if (bDryRun || !bNoRecycleBin)
		cmd += L" -n ";
	if (bDir)
		cmd += L" -d ";
	switch (cleanType)
	{
	case 0:
		cmd += L" -fx";
		break;
	case 1:
		cmd += L" -f";
		break;
	case 2:
		cmd += L" -fX";
		break;
	}
	if (bDirUnmanagedRepos)
		cmd += L" -f";

	STRING_VECTOR submoduleList;
	if (bSubmodules)
	{
		SubmodulePayload payload(submoduleList);
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
			return false;
		std::sort(submoduleList.begin(), submoduleList.end());
	}

	if (bDryRun || bNoRecycleBin)
	{
		CProgressDlg progress;
		for (int i = 0; i < pathList.GetCount(); ++i)
		{
			CString path;
			if (pathList[i].IsDirectory())
				path = pathList[i].GetGitPathString();
			else
				path = pathList[i].GetContainingDirectory().GetGitPathString();

			progress.m_GitDirList.push_back(g_Git.m_CurrentDir);
			progress.m_GitCmdList.push_back(cmd + (path.IsEmpty() ? L"" : (L" -- \"" + path + L'"')));
		}

		for (CString dir : submoduleList)
		{
			progress.m_GitDirList.push_back(CTGitPath(dir).GetWinPathString());
			progress.m_GitCmdList.push_back(cmd);
		}

		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				postCmdList.emplace_back(IDS_MSGBOX_RETRY, [&]{ DoCleanUp(pathList, cleanType, bDir, bDirUnmanagedRepos, bSubmodules, bDryRun, bNoRecycleBin); });

			if (status || !bDryRun)
				return;

			if (bNoRecycleBin)
			{
				postCmdList.emplace_back(IDS_CLEAN_NO_RECYCLEBIN, [&]{ DoCleanUp(pathList, cleanType, bDir, bDirUnmanagedRepos, bSubmodules, FALSE, TRUE); });
				postCmdList.emplace_back(IDS_CLEAN_TO_RECYCLEBIN, [&]{ DoCleanUp(pathList, cleanType, bDir, bDirUnmanagedRepos, bSubmodules, FALSE, FALSE); });
			}
			else
			{
				postCmdList.emplace_back(IDS_CLEAN_TO_RECYCLEBIN, [&]{ DoCleanUp(pathList, cleanType, bDir, bDirUnmanagedRepos, bSubmodules, FALSE, FALSE); });
				postCmdList.emplace_back(IDS_CLEAN_NO_RECYCLEBIN, [&]{ DoCleanUp(pathList, cleanType, bDir, bDirUnmanagedRepos, bSubmodules, FALSE, TRUE); });
			}
		};

		INT_PTR result = progress.DoModal();
		return result == IDOK;
	}
	else
	{
		CSysProgressDlg sysProgressDlg;
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO1)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		sysProgressDlg.SetShowProgressBar(false);
		sysProgressDlg.ShowModeless(static_cast<HWND>(nullptr), true);

		bool quotepath = g_Git.GetConfigValueBool(L"core.quotepath");

		CTGitPathList delList;
		for (int i = 0; i < pathList.GetCount(); ++i)
		{
			CString path;
			if (pathList[i].IsDirectory())
				path = pathList[i].GetGitPathString();
			else
				path = pathList[i].GetContainingDirectory().GetGitPathString();

			if (!GetFilesToCleanUp(delList, cmd, &g_Git, path, quotepath, sysProgressDlg))
				return false;
		}

		for (CString dir : submoduleList)
		{
			CGit git;
			git.m_CurrentDir = dir;
			if (!GetFilesToCleanUp(delList, cmd, &git, L"", quotepath, sysProgressDlg))
				return false;
		}

		delList.DeleteAllFiles(true, false, true);

		sysProgressDlg.Stop();
	}

	return true;
}

bool CleanupCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = false;

	CCleanTypeDlg dlg;
	dlg.m_pathList = pathList;
	if (dlg.DoModal() == IDOK)
	{
		bRet = DoCleanUp(pathList, dlg.m_CleanType, dlg.m_bDir == BST_CHECKED, dlg.m_bDirUnmanagedRepo == BST_CHECKED, dlg.m_bSubmodules == BST_CHECKED, dlg.m_bDryRun == BST_CHECKED, dlg.m_bNoRecycleBin == BST_CHECKED);

		CShellUpdater::Instance().Flush();
	}
	return bRet;
}
