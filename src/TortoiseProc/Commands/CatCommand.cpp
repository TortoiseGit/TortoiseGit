// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009,2011-2015 - TortoiseGit

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
#include "CatCommand.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "Git.h"
#include "MessageBox.h"
#include "SmartHandle.h"

bool CatCommand::Execute()
{
	CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
	CString revision = parser.GetVal(_T("revision"));

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_GETONEFILE))
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
		{
			::DeleteFile(savepath);
			CMessageBox::Show(hwndExplorer, g_Git.GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
			return false;
		}

		CAutoObject obj;
		if (git_revparse_single(obj.GetPointer(), repo, CUnicodeUtils::GetUTF8(revision)))
		{
			::DeleteFile(savepath);
			CMessageBox::Show(hwndExplorer, g_Git.GetLibGit2LastErr(L"Could not parse revision."), L"TortoiseGit", MB_ICONERROR);
			return false;
		}

		if (git_object_type(obj) == GIT_OBJ_BLOB)
		{
			CAutoFILE file = _tfsopen(savepath, _T("w"), SH_DENYRW);
			if (file == nullptr)
			{
				::DeleteFile(savepath);
				CMessageBox::Show(hwndExplorer, L"Could not open file for writing.", L"TortoiseGit", MB_ICONERROR);
				return false;
			}

			CAutoBuf buf;
			if (git_blob_filtered_content(buf, (git_blob *)(git_object *)obj, CUnicodeUtils::GetUTF8(cmdLinePath.GetGitPathString()), 0))
			{
				::DeleteFile(savepath);
				CMessageBox::Show(hwndExplorer, g_Git.GetLibGit2LastErr(L"Could not get filtered content."), L"TortoiseGit", MB_ICONERROR);
				return false;
			}
			if (fwrite(buf->ptr, sizeof(char), buf->size, file) != buf->size)
			{
				::DeleteFile(savepath);
				CString err = CFormatMessageWrapper();
				CMessageBox::Show(hwndExplorer, _T("Could not write to file: ") + err, L"TortoiseGit", MB_ICONERROR);
				return false;
			}
			return true;
		}

		if (g_Git.GetOneFile(revision, cmdLinePath, savepath))
		{
			CMessageBox::Show(hwndExplorer, g_Git.GetGitLastErr(L"Could get file.", CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_ICONERROR);
			return false;
		}
		return true;
	}

	CString cmd, output, err;
	cmd.Format(_T("git.exe cat-file -t %s"), (LPCTSTR)revision);

	if (g_Git.Run(cmd, &output, &err, CP_UTF8))
	{
		::DeleteFile(savepath);
		CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	if(output.Find(_T("blob")) == 0)
	{
		cmd.Format(_T("git.exe cat-file -p %s"), (LPCTSTR)revision);
	}
	else
	{
		cmd.Format(_T("git.exe show %s -- \"%s\""), (LPCTSTR)revision, (LPCTSTR)this->cmdLinePath.GetWinPathString());
	}

	if (g_Git.RunLogFile(cmd, savepath, &err))
	{
		::DeleteFile(savepath);
		CMessageBox::Show(hwndExplorer, _T("Cat file failed:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}
	return true;
}
