// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009,2011-2014 - TortoiseGit

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

bool CatCommand::Execute()
{
	CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
	CString revision = parser.GetVal(_T("revision"));

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_GETONEFILE))
	{
		git_repository *repo = nullptr;
		if (git_repository_open(&repo, CUnicodeUtils::GetUTF8(CTGitPath(g_Git.m_CurrentDir).GetGitPathString())))
		{
			::DeleteFile(savepath);
			CMessageBox::Show(hwndExplorer, g_Git.GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
			return false;
		}

		git_object *obj = nullptr;
		if (git_revparse_single(&obj, repo, CUnicodeUtils::GetUTF8(revision)))
		{
			::DeleteFile(savepath);
			git_repository_free(repo);
			CMessageBox::Show(hwndExplorer, g_Git.GetLibGit2LastErr(L"Could not parse revision."), L"TortoiseGit", MB_ICONERROR);
			return false;
		}

		if (git_object_type(obj) == GIT_OBJ_BLOB)
		{
			FILE *file = nullptr;
			_tfopen_s(&file, savepath, _T("w"));
			if (file == nullptr)
			{
				::DeleteFile(savepath);
				CMessageBox::Show(hwndExplorer, L"Could not open file for writing.", L"TortoiseGit", MB_ICONERROR);
				git_object_free(obj);
				git_repository_free(repo);
				return false;
			}
			if (fwrite(git_blob_rawcontent((const git_blob *)obj), 1, git_blob_rawsize((const git_blob *)obj), file) != (size_t)git_blob_rawsize((const git_blob *)obj)) // TODO: need to apply git_blob_filtered_content?
			{
				::DeleteFile(savepath);
				CString err = CFormatMessageWrapper();
				CMessageBox::Show(hwndExplorer, _T("Could not write to file: ") + err, L"TortoiseGit", MB_ICONERROR);
				fclose(file);
				git_object_free(obj);
				git_repository_free(repo);
				return false;
			}
			fclose(file);
			git_object_free(obj);
			git_repository_free(repo);
			return true;
		}

		git_object_free(obj);
		git_repository_free(repo);

		if (g_Git.GetOneFile(revision, cmdLinePath, savepath))
		{
			CMessageBox::Show(hwndExplorer, g_Git.GetGitLastErr(L"Could get file.", CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_ICONERROR);
			return false;
		}
		return true;
	}

	CString cmd, output, err;
	cmd.Format(_T("git.exe cat-file -t %s"),revision);

	if (g_Git.Run(cmd, &output, &err, CP_UTF8))
	{
		::DeleteFile(savepath);
		CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	if(output.Find(_T("blob")) == 0)
	{
		cmd.Format(_T("git.exe cat-file -p %s"),revision);
	}
	else
	{
		cmd.Format(_T("git.exe show %s -- \"%s\""),revision,this->cmdLinePath);
	}

	if (g_Git.RunLogFile(cmd, savepath, &err))
	{
		::DeleteFile(savepath);
		CMessageBox::Show(hwndExplorer, _T("Cat file failed:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}
	return true;
}
