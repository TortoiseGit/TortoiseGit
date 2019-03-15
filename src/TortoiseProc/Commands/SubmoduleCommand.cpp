// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2012-2016, 2018-2019 - TortoiseGit

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
#include "SubmoduleCommand.h"

#include "MessageBox.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "SubmoduleAddDlg.h"
#include "SubmoduleUpdateDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"

bool SubmoduleAddCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = false;
	CSubmoduleAddDlg dlg;
	dlg.m_strPath = cmdLinePath.GetDirectory().GetWinPathString();
	dlg.m_strProject = g_Git.m_CurrentDir;
	if( dlg.DoModal() == IDOK )
	{
		if (dlg.m_bAutoloadPuttyKeyFile)
			CAppUtils::LaunchPAgent(GetExplorerHWND(), &dlg.m_strPuttyKeyFile);

		CString cmd;
		if (CStringUtils::StartsWith(dlg.m_strPath, g_Git.m_CurrentDir))
			dlg.m_strPath = dlg.m_strPath.Right(dlg.m_strPath.GetLength()-g_Git.m_CurrentDir.GetLength()-1);

		CString branch;
		if(dlg.m_bBranch)
			branch.Format(L" -b %s ", static_cast<LPCTSTR>(dlg.m_strBranch));

		CString force;
		if (dlg.m_bForce)
			force = L"--force";

		dlg.m_strPath.Replace(L'\\', L'/');
		dlg.m_strRepos.Replace(L'\\', L'/');

		cmd.Format(L"git.exe submodule add %s %s -- \"%s\" \"%s\"",
						static_cast<LPCTSTR>(branch), static_cast<LPCTSTR>(force),
						static_cast<LPCTSTR>(dlg.m_strRepos), static_cast<LPCTSTR>(dlg.m_strPath));

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.DoModal();

		if (progress.m_GitStatus == 0)
		{
			if (dlg.m_bAutoloadPuttyKeyFile && !dlg.m_strPuttyKeyFile.IsEmpty())
			{
				SCOPE_EXIT { SetCurrentDirectory(g_Git.m_CurrentDir); };
				CGit subgit;
				dlg.m_strPath.Replace(L'/', L'\\');
				subgit.m_CurrentDir = PathIsRelative(dlg.m_strPath) ? g_Git.CombinePath(dlg.m_strPath) : dlg.m_strPath;
				SetCurrentDirectory(subgit.m_CurrentDir);
				if (subgit.SetConfigValue(L"remote.origin.puttykeyfile", dlg.m_strPuttyKeyFile, CONFIG_LOCAL))
				{
					CMessageBox::Show(GetExplorerHWND(), L"Fail set config remote.origin.puttykeyfile", L"TortoiseGit", MB_OK | MB_ICONERROR);
					return FALSE;
				}
			}
		}

		bRet = TRUE;
	}
	return bRet;
}

bool SubmoduleUpdateCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CString bkpath;
	if (parser.HasKey(L"bkpath"))
		bkpath = parser.GetVal(L"bkpath");
	else
	{
		bkpath = this->orgPathList[0].GetWinPathString();
		int start = bkpath.ReverseFind(L'\\');
		if (start >= 0)
			bkpath = bkpath.Left(start);
	}

	CString super = GitAdminDir::GetSuperProjectRoot(bkpath);
	if (super.IsEmpty())
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_ERR_NOTFOUND_SUPER_PRJECT, IDS_APPNAME, MB_OK | MB_ICONERROR);
		//change current project root to super project
		return false;
	}

	STRING_VECTOR pathFilterList;
	for (int i = 0; i < orgPathList.GetCount(); i++)
	{
		if (orgPathList[i].IsDirectory())
		{
			CString path = orgPathList[i].GetSubPath(CTGitPath(super)).GetGitPathString();
			if (!path.IsEmpty())
				pathFilterList.push_back(path);
		}
	}

	CSubmoduleUpdateDlg submoduleUpdateDlg;
	submoduleUpdateDlg.m_PathFilterList = pathFilterList;
	if (parser.HasKey(L"selectedpath"))
	{
		CString selectedPath = parser.GetVal(L"selectedpath");
		selectedPath.Replace(L'\\', L'/');
		submoduleUpdateDlg.m_PathList.push_back(selectedPath);
	}
	if (submoduleUpdateDlg.DoModal() != IDOK)
		return false;

	CProgressDlg progress;
	theApp.m_pMainWnd = &progress;

	g_Git.m_CurrentDir = super;

	CString params = L" --progress";
	if (submoduleUpdateDlg.m_bInit)
		params += L" --init";
	if (submoduleUpdateDlg.m_bRecursive)
		params += L" --recursive";
	if (submoduleUpdateDlg.m_bForce)
		params += L" --force";
	if (submoduleUpdateDlg.m_bNoFetch)
		params += L" --no-fetch";
	if (submoduleUpdateDlg.m_bMerge)
		params += L" --merge";
	if (submoduleUpdateDlg.m_bRebase)
		params += L" --rebase";
	if (submoduleUpdateDlg.m_bRemote)
		params += L" --remote";

	for (size_t i = 0; i < submoduleUpdateDlg.m_PathList.size(); ++i)
	{
		CString str;
		str.Format(L"git.exe submodule update%s -- \"%s\"", static_cast<LPCTSTR>(params), static_cast<LPCTSTR>(submoduleUpdateDlg.m_PathList[i]));
		progress.m_GitCmdList.push_back(str);
	}

	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.IsBisectActive())
		{
			postCmdList.emplace_back(IDI_THUMB_UP, IDS_MENUBISECTGOOD, [] { CAppUtils::RunTortoiseGitProc(L"/command:bisect /good"); });
			postCmdList.emplace_back(IDI_THUMB_DOWN, IDS_MENUBISECTBAD, [] { CAppUtils::RunTortoiseGitProc(L"/command:bisect /bad"); });
			postCmdList.emplace_back(IDI_BISECT, IDS_MENUBISECTSKIP, [] { CAppUtils::RunTortoiseGitProc(L"/command:bisect /skip"); });
			postCmdList.emplace_back(IDI_BISECT_RESET, IDS_MENUBISECTRESET, [] { CAppUtils::RunTortoiseGitProc(L"/command:bisect /reset"); });
		}
	};

	progress.DoModal();

	return !progress.m_GitStatus;
}

bool SubmoduleSyncCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CProgressDlg progress;
	theApp.m_pMainWnd = &progress;

	CString bkpath;

	if (parser.HasKey(L"bkpath"))
		bkpath=parser.GetVal(L"bkpath");
	else
	{
		bkpath=this->orgPathList[0].GetWinPathString();
		int start = bkpath.ReverseFind(L'\\');
		if( start >= 0 )
			bkpath=bkpath.Left(start);
	}

	CString super = GitAdminDir::GetSuperProjectRoot(bkpath);
	if(super.IsEmpty())
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_ERR_NOTFOUND_SUPER_PRJECT, IDS_APPNAME, MB_OK | MB_ICONERROR);
		//change current project root to super project
		return false;
	}

	g_Git.m_CurrentDir=super;

	CString str;
	for (int i = 0; i < this->orgPathList.GetCount(); ++i)
	{
		if(orgPathList[i].IsDirectory())
		{
			CString path = orgPathList[i].GetSubPath(CTGitPath(super)).GetGitPathString();
			if (path.IsEmpty())
				str = L"git.exe submodule sync";
			else
				str.Format(L"git.exe submodule sync -- \"%s\"", static_cast<LPCTSTR>(path));
			progress.m_GitCmdList.push_back(str);
		}
	}

	progress.DoModal();

	return !progress.m_GitStatus;
}
