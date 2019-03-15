// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2012 - TortoiseSVN

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
#include "CloneCommand.h"

#include "GitProgressDlg.h"
#include "StringUtils.h"
#include "CloneDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"
#include "SysProgressDlg.h"
#include "ProgressCommands/CloneProgressCommand.h"

static CString GetExistingDirectoryForClone(CString path)
{
	if (PathFileExists(path))
		return path;
	int index = path.ReverseFind('\\');
	while (index >= 0 && path.GetLength() >= 3)
	{
		if (PathFileExists(path.Left(index)))
		{
			if (index == 2 && path[1] == L':')
				return path.Left(index + 1);
			return path.Left(index);
		}
		path = path.Left(index);
		index = path.ReverseFind('\\');
	}
	GetTempPath(path);
	return path;
}

static void StorePuttyKey(const CString& repoRoot, const CString& remote, const CString& keyFile)
{
	CAutoRepository repo(repoRoot);
	CAutoConfig config;
	CString configName;
	if (!repo)
		goto error;

	if (git_repository_config(config.GetPointer(), repo))
		goto error;

	configName.Format(L"remote.%s.puttykeyfile", static_cast<LPCTSTR>(remote));

	if (git_config_set_string(config, CUnicodeUtils::GetUTF8(configName), CUnicodeUtils::GetUTF8(keyFile)))
		goto error;

	return;

error:
	MessageBox(GetExplorerHWND(), CGit::GetLibGit2LastErr(L"Could not open repository"), L"TortoiseGit", MB_ICONERROR);
}

bool CloneCommand::Execute()
{
	CTGitPath cloneDirectory;
	if (!parser.HasKey(L"hasurlhandler"))
	{
		if (orgCmdLinePath.IsEmpty())
		{
			cloneDirectory.SetFromWin(sOrigCWD, true);
			DWORD len = ::GetTempPath(0, nullptr);
			auto tszPath = std::make_unique<TCHAR[]>(len);
			::GetTempPath(len, tszPath.get());
			if (_wcsnicmp(cloneDirectory.GetWinPath(), tszPath.get(), len - 2 /* \\ and \0 */) == 0)
			{
				// if the current directory is set to a temp directory,
				// we don't use that but leave it empty instead.
				cloneDirectory.Reset();
			}
		}
		else
			cloneDirectory = orgCmdLinePath;
	}

	CCloneDlg dlg;
	dlg.m_Directory = cloneDirectory.GetWinPathString();

	if (parser.HasKey(L"url"))
		dlg.m_URL = parser.GetVal(L"url");
	if (parser.HasKey(L"exactpath"))
		dlg.m_bExactPath = TRUE;

	if(dlg.DoModal()==IDOK)
	{
		CString recursiveStr;
		if(dlg.m_bRecursive)
			recursiveStr = L" --recursive";

		CString bareStr;
		if(dlg.m_bBare)
			bareStr = L" --bare";

		CString nocheckoutStr;
		if (dlg.m_bNoCheckout)
			nocheckoutStr = L" --no-checkout";

		CString branchStr;
		if (dlg.m_bBranch)
			branchStr = L" --branch " + dlg.m_strBranch;

		CString originStr;
		if (dlg.m_bOrigin && !dlg.m_bSVN)
			originStr = L" --origin " + dlg.m_strOrigin;

		if(dlg.m_bAutoloadPuttyKeyFile)
			CAppUtils::LaunchPAgent(GetExplorerHWND(), &dlg.m_strPuttyKeyFile);

		CAppUtils::RemoveTrailSlash(dlg.m_Directory);
		if (!dlg.m_bSVN)
			CAppUtils::RemoveTrailSlash(dlg.m_URL);

		CString dir=dlg.m_Directory;
		CString url=dlg.m_URL;

		// is this a windows format UNC path, ie starts with \\?
		if (CStringUtils::StartsWith(url, L"\\\\"))
		{
			// yes, change all \ to /
			// this should not be necessary but msysgit does not support the use \ here yet
			int atSign = url.Find(L'@');
			if (atSign > 0)
			{
				CString path = url.Mid(atSign);
				path.Replace(L'\\', L'/');
				url = url.Left(atSign) + path;
			}
			else
				url.Replace( L'\\', L'/');
		}

		CString depth;
		if (dlg.m_bDepth)
			depth.Format(L" --depth %d", dlg.m_nDepth);

		CString cmd;
		cmd.Format(L"git.exe %sclone --progress%s%s%s%s%s -v%s \"%s\" \"%s\"",
						dlg.m_bUseLFS ? L"lfs " : L"",
						static_cast<LPCTSTR>(nocheckoutStr),
						static_cast<LPCTSTR>(recursiveStr),
						static_cast<LPCTSTR>(bareStr),
						static_cast<LPCTSTR>(branchStr),
						static_cast<LPCTSTR>(originStr),
						static_cast<LPCTSTR>(depth),
						static_cast<LPCTSTR>(url),
						static_cast<LPCTSTR>(dir));

		bool retry = false;
		auto postCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
			{
				postCmdList.emplace_back(IDI_REFRESH, IDS_MSGBOX_RETRY, [&]{ retry = true; });
				return;
			}

			if (dlg.m_bBare)
				CAppUtils::SetupBareRepoIcon(dir);

			// After cloning, change current directory to the cloned directory
			g_Git.m_CurrentDir = dlg.m_Directory;
			if (dlg.m_bAutoloadPuttyKeyFile) // do this here, since it might be needed for actions performed in Log
				StorePuttyKey(dlg.m_Directory, dlg.m_bOrigin && !dlg.m_strOrigin.IsEmpty() ? dlg.m_strOrigin : L"origin", dlg.m_strPuttyKeyFile);

			postCmdList.emplace_back(IDI_LOG, IDS_MENULOG, [&]
			{
				CString cmd = L"/command:log";
				cmd += L" /path:\"" + dlg.m_Directory + L'"';
				CAppUtils::RunTortoiseGitProc(cmd);
			});

			postCmdList.emplace_back(IDI_EXPLORER, IDS_STATUSLIST_CONTEXT_EXPLORE, [&]{ CAppUtils::ExploreTo(GetExplorerHWND(), dlg.m_Directory); });
		};

		// Handle Git SVN-clone
		if(dlg.m_bSVN)
		{
			// git-svn requires some mangling: \ -> /
			if (!PathIsURL(url))
			{
				url.Replace(L'\\', L'/');
				if (PathIsUNC(url))
					url = L"file:" + url;
				else if (PathIsDirectory(url))
				{
					// prefix: file:///, and no colon after drive letter for normal paths
					if (url.GetLength() > 2 && url.GetAt(1) == L':')
						url.Delete(1, 1);
					url = L"file:///" + url;
				}
			}

			//g_Git.m_CurrentDir=dlg.m_Directory;
			cmd.Format(L"git.exe svn clone \"%s\" \"%s\"",
				static_cast<LPCTSTR>(url), static_cast<LPCTSTR>(dlg.m_Directory));

			if (dlg.m_bOrigin)
			{
				CString str;
				if (dlg.m_strOrigin.IsEmpty())
					str = L" --prefix \"\"";
				else
					str.Format(L" --prefix \"%s/\"", static_cast<LPCTSTR>(dlg.m_strOrigin));
				cmd += str;
			}

			if(dlg.m_bSVNTrunk)
				cmd += L" -T " + dlg.m_strSVNTrunk;

			if(dlg.m_bSVNBranch)
				cmd += L" -b " + dlg.m_strSVNBranchs;

			if(dlg.m_bSVNTags)
				cmd += L" -t " + dlg.m_strSVNTags;

			if(dlg.m_bSVNFrom)
				cmd.AppendFormat(L" -r %d:HEAD", dlg.m_nSVNFrom);

			if(dlg.m_bSVNUserName)
			{
				cmd += L" --username ";
				cmd+=dlg.m_strUserName;
			}
		}
		else
		{
			if (g_Git.UsingLibGit2(CGit::GIT_CMD_CLONE) && !dlg.m_bUseLFS)
			{
				while (true)
				{
					retry = false;
					CGitProgressDlg GitDlg;
					CTGitPathList list;
					g_Git.m_CurrentDir = GetExistingDirectoryForClone(dlg.m_Directory);
					list.AddPath(CTGitPath(dir));
					CloneProgressCommand cloneProgressCommand;
					GitDlg.SetCommand(&cloneProgressCommand);
					cloneProgressCommand.m_PostCmdCallback = postCmdCallback;
					cloneProgressCommand.SetUrl(url);
					cloneProgressCommand.SetPathList(list);
					cloneProgressCommand.SetIsBare(dlg.m_bBare == TRUE);
					if (dlg.m_bBranch)
						cloneProgressCommand.SetRefSpec(dlg.m_strBranch);
					if (dlg.m_bOrigin)
						cloneProgressCommand.SetRemote(dlg.m_strOrigin);
					cloneProgressCommand.SetNoCheckout(dlg.m_bNoCheckout == TRUE);
					GitDlg.DoModal();
					if (!retry)
						return !GitDlg.DidErrorsOccur();
				}
			}
		}

		while (true)
		{
			retry = false;
			g_Git.m_CurrentDir = GetExistingDirectoryForClone(dlg.m_Directory);
			CProgressDlg progress;
			progress.m_GitCmd=cmd;
			progress.m_PostCmdCallback = postCmdCallback;
			INT_PTR ret = progress.DoModal();

			if (!retry)
				return ret == IDOK;
		}
	}
	return FALSE;
}
