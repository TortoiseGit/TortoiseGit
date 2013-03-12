// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN
// Copyright (C) 2007-2011 - TortoiseGit
// Copyright (C) 2011 Sven Strickroth <email@cs-ware.de>

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
#include "DiffCommand.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "GitDiff.h"
#include "GitStatus.h"

bool DiffCommand::Execute()
{
	bool bRet = false;
	CString path2 = CPathUtils::GetLongPathname(parser.GetVal(_T("path2")));
	bool bAlternativeTool = !!parser.HasKey(_T("alternative"));
//	bool bBlame = !!parser.HasKey(_T("blame"));
	if (path2.IsEmpty())
	{
		if (this->orgCmdLinePath.IsDirectory())
		{
			CChangedDlg dlg;
			dlg.m_pathList = CTGitPathList(cmdLinePath);
			dlg.DoModal();
			bRet = true;
		}
		else
		{
			CGitDiff diff;
			//diff.SetAlternativeTool(bAlternativeTool);
			if ( parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")) )
			{
				bRet = !!diff.Diff(&cmdLinePath,&cmdLinePath,git_revnum_t(parser.GetVal(_T("startrev"))),git_revnum_t(parser.GetVal(_T("endrev"))));
			}
			else
			{
				// check if it is a newly added (but uncommitted) file
				git_wc_status_kind status = git_wc_status_none;
				CString topDir;
				if (orgCmdLinePath.HasAdminDir(&topDir))
				{
					g_Git.Run(_T("git.exe update-index -- \"") + cmdLinePath.GetGitPathString() + _T("\""), nullptr); // make sure we get the right status
					GitStatus::GetFileStatus(topDir, cmdLinePath.GetWinPathString(), &status, true);
				}
				if (status == git_wc_status_added)
				{
					if (!g_Git.IsInitRepos())
					{
						// this might be a rename, try to find original name
						BYTE_VECTOR cmdout;
						g_Git.Run(_T("git.exe diff-index --raw HEAD -M -C -z"), &cmdout);
						CTGitPathList changedFiles;
						changedFiles.ParserFromLog(cmdout);
						for (int i = 0; i < changedFiles.GetCount(); ++i)
						{
							if (changedFiles[i].GetGitPathString() == cmdLinePath.GetGitPathString())
							{
								if (!changedFiles[i].GetGitOldPathString().IsEmpty())
								{
									CTGitPath oldPath(changedFiles[i].GetGitOldPathString());
									return !!diff.Diff(&cmdLinePath, &oldPath, git_revnum_t(GIT_REV_ZERO), git_revnum_t(_T("HEAD")));
								}
								break;
							}
						}
					}
					cmdLinePath.m_Action = cmdLinePath.LOGACTIONS_ADDED;
				}

				bRet = !!diff.Diff(&cmdLinePath,&cmdLinePath,git_revnum_t(GIT_REV_ZERO),git_revnum_t(_T("HEAD")));
			}
		}
	}
	else
		bRet = CAppUtils::StartExtDiff(
			path2, orgCmdLinePath.GetWinPathString(), CString(), CString(),
			CString(), CString(), git_revnum_t(GIT_REV_ZERO), git_revnum_t(GIT_REV_ZERO),
			CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool));

	return bRet;
}
