// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN
// Copyright (C) 2007-2011, 2013-2015 - TortoiseGit

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
#include "../TGitCache/CacheInterface.h"
#include "../Utils/UnicodeUtils.h"

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
			if (cmdLinePath.IsEmpty())
				return false;
			CGitDiff diff;
			//diff.SetAlternativeTool(bAlternativeTool);
			if ( parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")) )
			{
				if (parser.HasKey(_T("unified")))
					bRet = !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, git_revnum_t(parser.GetVal(_T("endrev"))), cmdLinePath, git_revnum_t(parser.GetVal(_T("startrev"))));
				else
					bRet = !!diff.Diff(&cmdLinePath, &cmdLinePath, git_revnum_t(parser.GetVal(_T("startrev"))), git_revnum_t(parser.GetVal(_T("endrev"))), false, parser.HasKey(_T("unified")) == TRUE, parser.GetLongVal(_T("line")));
			}
			else
			{
				// check if it is a newly added (but uncommitted) file
				git_wc_status_kind status = git_wc_status_none;
				CString topDir;
				if (orgCmdLinePath.HasAdminDir(&topDir))
				{
					CBlockCacheForPath cacheBlock(topDir);
					CAutoIndex index;
					CString adminDir;
					GitAdminDir::GetAdminDirPath(topDir, adminDir);
					if (!git_index_open(index.GetPointer(), CUnicodeUtils::GetUTF8(adminDir + _T("index"))))
						g_Git.Run(_T("git.exe update-index -- \"") + cmdLinePath.GetGitPathString() + _T("\""), nullptr); // make sure we get the right status
					GitStatus::GetFileStatus(topDir, cmdLinePath.GetWinPathString(), &status, true);
					if (index)
						git_index_write(index);
				}
				if (status == git_wc_status_added)
				{
					if (!g_Git.IsInitRepos())
					{
						// this might be a rename, try to find original name
						BYTE_VECTOR cmdout;
						g_Git.Run(_T("git.exe diff-index --raw HEAD -M -C -z --"), &cmdout);
						CTGitPathList changedFiles;
						changedFiles.ParserFromLog(cmdout);
						for (int i = 0; i < changedFiles.GetCount(); ++i)
						{
							if (changedFiles[i].GetGitPathString() == cmdLinePath.GetGitPathString())
							{
								if (!changedFiles[i].GetGitOldPathString().IsEmpty())
								{
									CTGitPath oldPath(changedFiles[i].GetGitOldPathString());
									if (parser.HasKey(_T("unified")))
										return !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, git_revnum_t(_T("HEAD")), cmdLinePath, git_revnum_t(GIT_REV_ZERO));
									return !!diff.Diff(&cmdLinePath, &oldPath, git_revnum_t(GIT_REV_ZERO), git_revnum_t(_T("HEAD")), false, parser.HasKey(_T("unified")) == TRUE, parser.GetLongVal(_T("line")));
								}
								break;
							}
						}
					}
					cmdLinePath.m_Action = cmdLinePath.LOGACTIONS_ADDED;
				}

				if (parser.HasKey(_T("unified")))
					bRet = !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, git_revnum_t(_T("HEAD")), cmdLinePath, git_revnum_t(GIT_REV_ZERO));
				else
					bRet = !!diff.Diff(&cmdLinePath, &cmdLinePath, git_revnum_t(GIT_REV_ZERO), git_revnum_t(_T("HEAD")), false, parser.HasKey(_T("unified")) == TRUE, parser.GetLongVal(_T("line")));
			}
		}
	}
	else
	{
		CGitDiff diff;
		if ( parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")) && path2.Left(g_Git.m_CurrentDir.GetLength() + 1) == g_Git.m_CurrentDir + _T("\\"))
		{
			CTGitPath tgitPath2 = path2.Mid(g_Git.m_CurrentDir.GetLength() + 1);
			bRet = !!diff.Diff(&tgitPath2, &cmdLinePath, git_revnum_t(parser.GetVal(_T("startrev"))), git_revnum_t(parser.GetVal(_T("endrev"))), false, parser.HasKey(_T("unified")) == TRUE, parser.GetLongVal(_T("line")));
		}
		else
		{
			bRet = CAppUtils::StartExtDiff(
				path2, orgCmdLinePath.GetWinPathString(), CString(), CString(),
				CString(), CString(), git_revnum_t(GIT_REV_ZERO), git_revnum_t(GIT_REV_ZERO),
				CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool), parser.GetLongVal(_T("line")));
		}
	}

	return bRet;
}
