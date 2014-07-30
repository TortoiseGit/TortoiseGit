// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
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
			return path.Left(index);
		path = path.Left(index);
		index = path.ReverseFind('\\');
	}
	GetTempPath(path);
	return path;
}

static void StorePuttyKey(const CString& repoRoot, const CString& keyFile)
{
	CAutoRepository repo(repoRoot);
	CAutoConfig config;
	if (!repo)
		goto error;

	if (git_repository_config(config.GetPointer(), repo))
		goto error;

	if (git_config_set_string(config, "remote.origin.puttykeyfile", CUnicodeUtils::GetUTF8(keyFile)))
		goto error;

	return;

error:
	MessageBox(hWndExplorer, CGit::GetLibGit2LastErr(L"Could not open repository"), _T("TortoiseGit"), MB_ICONERROR);
}

bool CloneCommand::Execute()
{
	CTGitPath cloneDirectory;
	if (!parser.HasKey(_T("hasurlhandler")))
	{
		if (orgCmdLinePath.IsEmpty())
		{
			cloneDirectory.SetFromWin(sOrigCWD, true);
			DWORD len = ::GetTempPath(0, NULL);
			std::unique_ptr<TCHAR[]> tszPath(new TCHAR[len]);
			::GetTempPath(len, tszPath.get());
			if (_tcsncicmp(cloneDirectory.GetWinPath(), tszPath.get(), len-2 /* \\ and \0 */) == 0)
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

	if (parser.HasKey(_T("url")))
		dlg.m_URL = parser.GetVal(_T("url"));
	if (parser.HasKey(_T("exactpath")))
		dlg.m_bExactPath = TRUE;

	if(dlg.DoModal()==IDOK)
	{
		CString recursiveStr;
		if(dlg.m_bRecursive)
			recursiveStr = _T("--recursive");
		else
			recursiveStr = _T("");

		CString bareStr;
		if(dlg.m_bBare)
			bareStr = _T("--bare");
		else
			bareStr = _T("");

		CString nocheckoutStr;
		if (dlg.m_bNoCheckout)
			nocheckoutStr = _T("--no-checkout");

		CString branchStr;
		if (dlg.m_bBranch)
			branchStr = _T("--branch ") + dlg.m_strBranch;

		CString originStr;
		if (dlg.m_bOrigin)
			originStr = _T("--origin ") + dlg.m_strOrigin;

		if(dlg.m_bAutoloadPuttyKeyFile)
		{
			CAppUtils::LaunchPAgent(&dlg.m_strPuttyKeyFile);
		}

		CAppUtils::RemoveTrailSlash(dlg.m_Directory);
		if (!dlg.m_bSVN)
			CAppUtils::RemoveTrailSlash(dlg.m_URL);

		CString dir=dlg.m_Directory;
		CString url=dlg.m_URL;

		// is this a windows format UNC path, ie starts with \\?
		if (url.Find(_T("\\\\")) == 0)
		{
			// yes, change all \ to /
			// this should not be necessary but msysgit does not support the use \ here yet
			int atSign = url.Find(_T('@'));
			if (atSign > 0)
			{
				CString path = url.Mid(atSign);
				path.Replace(_T('\\'), _T('/'));
				url = url.Mid(0, atSign) + path;
			}
			else
				url.Replace( _T('\\'), _T('/'));
		}

		CString depth;
		if (dlg.m_bDepth)
		{
			depth.Format(_T(" --depth %d"),dlg.m_nDepth);
		}

		g_Git.m_CurrentDir = GetExistingDirectoryForClone(dlg.m_Directory);

		CString cmd;
		CString progressarg;

		int ver = CAppUtils::GetMsysgitVersion();

		if(ver >= 0x01070002) //above 1.7.0.2
			progressarg = _T("--progress");

		cmd.Format(_T("git.exe clone %s %s %s %s %s %s -v %s \"%s\" \"%s\""),
						nocheckoutStr,
						recursiveStr,
						bareStr,
						branchStr,
						originStr,
						progressarg,
						depth,
						url,
						dir);

		// Handle Git SVN-clone
		if(dlg.m_bSVN)
		{
			//g_Git.m_CurrentDir=dlg.m_Directory;
			cmd.Format(_T("git.exe svn clone \"%s\"  \"%s\""),
				url,dlg.m_Directory);

			if(dlg.m_bSVNTrunk)
				cmd+=_T(" -T ")+dlg.m_strSVNTrunk;

			if(dlg.m_bSVNBranch)
				cmd+=_T(" -b ")+dlg.m_strSVNBranchs;

			if(dlg.m_bSVNTags)
				cmd+=_T(" -t ")+dlg.m_strSVNTags;

			if(dlg.m_bSVNFrom)
			{
				CString str;
				str.Format(_T("%d:HEAD"),dlg.m_nSVNFrom);
				cmd+=_T(" -r ")+str;
			}

			if(dlg.m_bSVNUserName)
			{
				cmd+= _T(" --username ");
				cmd+=dlg.m_strUserName;
			}
		}
		else
		{
			if (g_Git.UsingLibGit2(CGit::GIT_CMD_CLONE))
			{
				CGitProgressDlg GitDlg;
				CTGitPathList list;
				g_Git.m_CurrentDir = dir;
				list.AddPath(CTGitPath(dir));
				CloneProgressCommand cloneProgressCommand;
				GitDlg.SetCommand(&cloneProgressCommand);
				cloneProgressCommand.SetUrl(url);
				cloneProgressCommand.SetPathList(list);
				cloneProgressCommand.SetIsBare(dlg.m_bBare == TRUE);
				if (dlg.m_bBranch)
					cloneProgressCommand.SetRefSpec(dlg.m_strBranch);
				if (dlg.m_bOrigin)
					cloneProgressCommand.SetRemote(dlg.m_strOrigin);
				cloneProgressCommand.SetNoCheckout(dlg.m_bNoCheckout == TRUE);
				GitDlg.DoModal();
				return !GitDlg.DidErrorsOccur();
			}
		}
		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status)
				return;

			if (dlg.m_bAutoloadPuttyKeyFile) // do this here, since it might be needed for actions performed in Log
				StorePuttyKey(dlg.m_Directory, dlg.m_strPuttyKeyFile);

			postCmdList.push_back(PostCmd(IDI_LOG, IDS_MENULOG, [&]
			{
				CString cmd = _T("/command:log");
				cmd += _T(" /path:\"") + dlg.m_Directory + _T("\"");
				CAppUtils::RunTortoiseGitProc(cmd);
			}));

			postCmdList.push_back(PostCmd(IDI_EXPLORER, IDS_STATUSLIST_CONTEXT_EXPLORE, [&]{ CAppUtils::ExploreTo(hWndExplorer, dlg.m_Directory); }));
		};
		INT_PTR ret = progress.DoModal();

		if (dlg.m_bSVN)
			::DeleteFile(g_Git.m_CurrentDir + _T("\\sys$command"));

		return ret == IDOK;
	}
	return FALSE;
}
