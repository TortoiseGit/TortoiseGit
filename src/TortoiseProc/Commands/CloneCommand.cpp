// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019, 2021-2026 - TortoiseGit
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
#include "ProgressCommands/CloneProgressCommand.h"
#include "CmdLineParser.h"

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

	configName.Format(L"remote.%s.puttykeyfile", static_cast<LPCWSTR>(remote));

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
			cloneDirectory.SetFromWin(CPathUtils::GetCWD(), true);
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
		CString args;
		if(dlg.m_bRecursive)
			args += L" --recursive";

		if(dlg.m_bBare)
			args += L" --bare";

		if (dlg.m_bNoCheckout)
			args += L" --no-checkout";

		try
		{
			if (dlg.m_bBranch)
				args += L" --branch " + CGit::QuoteParameter(dlg.m_strBranch);

			if (dlg.m_bOrigin && !dlg.m_bSVN)
				args += L" --origin " + CGit::QuoteParameter(dlg.m_strOrigin);
		}
		catch (illegal_git_parameter& e)
		{
			MessageBox(GetExplorerHWND(), e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}

		if(dlg.m_bAutoloadPuttyKeyFile)
			CAppUtils::LaunchPAgent(GetExplorerHWND(), &dlg.m_strPuttyKeyFile);

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

		if (dlg.m_bDepth)
			args.AppendFormat(L" --depth %d", dlg.m_nDepth);

		CString cmd;
		try
		{
			cmd.Format(L"git.exe clone --progress%s -v -- %s %s",
					   static_cast<LPCWSTR>(args),
					   static_cast<LPCWSTR>(CGit::QuoteParameter(url)),
					   static_cast<LPCWSTR>(CGit::QuoteParameter(dir)));
		}
		catch (illegal_git_parameter& e)
		{
			MessageBox(GetExplorerHWND(), e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}

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
				StorePuttyKey(dlg.m_Directory, dlg.m_bOrigin && !dlg.m_strOrigin.IsEmpty() ? dlg.m_strOrigin : CString(L"origin"), dlg.m_strPuttyKeyFile);

			postCmdList.emplace_back(IDI_LOG, IDS_MENULOG, [&]
			{
				CString cmd = L"/command:log";
				cmd += L" /path:" + CCmdLineParser::EscapeValue(dlg.m_Directory);
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
			cmd = L"git.exe svn clone";
			try
			{
				if (dlg.m_bOrigin)
				{
					if (dlg.m_strOrigin.IsEmpty())
						cmd += L" --prefix " + CGit::QuoteParameter(L"");
					else
						cmd.AppendFormat(L" --prefix %s", static_cast<LPCWSTR>(CGit::QuoteParameter(dlg.m_strOrigin + L"/")));
				}

				if (dlg.m_bSVNTrunk)
					cmd += L" -T " + CGit::QuoteParameter(dlg.m_strSVNTrunk);

				if (dlg.m_bSVNBranch)
					cmd += L" -b " + CGit::QuoteParameter(dlg.m_strSVNBranchs);

				if (dlg.m_bSVNTags)
					cmd += L" -t " + CGit::QuoteParameter(dlg.m_strSVNTags);

				if (dlg.m_bSVNFrom)
					cmd.AppendFormat(L" -r %d:HEAD", dlg.m_nSVNFrom);

				if (dlg.m_bSVNUserName)
				{
					cmd += L" --username ";
					cmd += CGit::QuoteParameter(dlg.m_strUserName);
				}

				cmd.AppendFormat(L" -- %s %s", static_cast<LPCWSTR>(CGit::QuoteParameter(url)), static_cast<LPCWSTR>(CGit::QuoteParameter(dlg.m_Directory)));
			}
			catch (illegal_git_parameter& e)
			{
				MessageBox(GetExplorerHWND(), e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
		}
		else
		{
			if (g_Git.UsingLibGit2(CGit::GIT_CMD_CLONE))
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
