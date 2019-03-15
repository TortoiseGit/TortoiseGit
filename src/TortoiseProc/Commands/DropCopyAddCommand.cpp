// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013-2016, 2018-2019 - TortoiseGit
// Copyright (C) 2007-2008,2010,2012 - TortoiseSVN

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
#include "DropCopyAddCommand.h"
#include "FormatMessageWrapper.h"
#include "GitProgressDlg.h"
#include "MessageBox.h"
#include "StringUtils.h"
#include "DirFileEnum.h"
#include "ProgressCommands/AddProgressCommand.h"

bool DropCopyAddCommand::Execute()
{
	bool bRet = false;
	CString droppath = parser.GetVal(L"droptarget");
	if (CTGitPath(droppath).IsAdminDir())
		return FALSE;

	if(!CTGitPath(droppath).HasAdminDir(&g_Git.m_CurrentDir))
		return FALSE;

	int worktreePathLen = g_Git.m_CurrentDir.GetLength();
	orgPathList.RemoveAdminPaths();
	CTGitPathList copiedFiles;
	for(int nPath = 0; nPath < orgPathList.GetCount(); ++nPath)
	{
		if (orgPathList[nPath].IsEquivalentTo(CTGitPath(droppath)))
			continue;

		//copy the file to the new location
		CString name = orgPathList[nPath].GetFileOrDirectoryName();
		if (::PathFileExists(droppath + L'\\' + name))
		{
			if (::PathIsDirectory(droppath + L'\\' + name))
			{
				if (orgPathList[nPath].IsDirectory())
					continue;
			}

			CString strMessage;
			strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, static_cast<LPCTSTR>(droppath + L'\\' + name));
			CString sBtn1(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_OVERWRITE));
			CString sBtn2(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_KEEP));
			CString sBtn3(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_CANCEL));
			UINT ret = CMessageBox::Show(GetExplorerHWND(), strMessage, L"TortoiseGit", 2, IDI_QUESTION, sBtn1, sBtn2, sBtn3);

			if (ret == 3)
				return FALSE; //cancel the whole operation
			if (ret == 1)
			{
				if (!::CopyFile(orgPathList[nPath].GetWinPath(), droppath + L'\\' + name, FALSE))
				{
					//the copy operation failed! Get out of here!
					ShowErrorMessage();
					return FALSE;
				}
			}
		}
		else
		{
			if (orgPathList[nPath].IsDirectory())
			{
				CString fromPath = orgPathList[nPath].GetWinPathString() + L"||";
				CString toPath = droppath + L'\\' + name + L"||";
				auto fromBuf = std::make_unique<TCHAR[]>(fromPath.GetLength() + 2);
				auto toBuf = std::make_unique<TCHAR[]>(toPath.GetLength() + 2);
				wcscpy_s(fromBuf.get(), fromPath.GetLength() + 2, fromPath);
				wcscpy_s(toBuf.get(), toPath.GetLength() + 2, toPath);
				CStringUtils::PipesToNulls(fromBuf.get(), fromPath.GetLength() + 2);
				CStringUtils::PipesToNulls(toBuf.get(), toPath.GetLength() + 2);

				SHFILEOPSTRUCT fileop = {0};
				fileop.wFunc = FO_COPY;
				fileop.pFrom = fromBuf.get();
				fileop.pTo = toBuf.get();
				fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS | FOF_SILENT;
				if (!SHFileOperation(&fileop))
				{
					// add all copied files WITH special handling for repos/submodules (folders which include a .git entry)
					CDirFileEnum finder(droppath + L'\\' + name);
					bool isDir = true;
					CString filepath;
					CString lastRepo;
					bool isRepo = false;
					while (finder.NextFile(filepath, &isDir, !isRepo)) // don't recurse into .git directories
					{
						if (!lastRepo.IsEmpty())
						{
							if (CStringUtils::StartsWith(filepath, lastRepo))
								continue;
							else
								lastRepo.Empty();
						}
						int pos = -1;
						if ((pos = filepath.Find(L'\\' + GitAdminDir::GetAdminDirName())) >= 0)
							isRepo = (pos == filepath.GetLength() - GitAdminDir::GetAdminDirName().GetLength() - 1);
						else
							isRepo = false;
						if (isRepo)
						{
							lastRepo = filepath.Left(filepath.GetLength() - GitAdminDir::GetAdminDirName().GetLength());
							CString msg;
							if (!isDir)
								msg.Format(IDS_PROC_COPY_SUBMODULE, static_cast<LPCTSTR>(lastRepo));
							else
								msg.Format(IDS_PROC_COPY_REPOSITORY, static_cast<LPCTSTR>(lastRepo));
							int ret = CMessageBox::Show(GetExplorerHWND(), msg, L"TortoiseGit", 1, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_IGNOREBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
							if (ret == 3)
								return FALSE;
							if (ret == 1)
							{
								CTGitPath(filepath).Delete(false, true);
								lastRepo.Empty();
							}
							continue;
						}
						if (!isDir)
							copiedFiles.AddPath(CTGitPath(filepath.Mid(worktreePathLen + 1))); //add the new filepath
					}
				}
				continue; // do not add a directory to copiedFiles
			}
			else if (!CopyFile(orgPathList[nPath].GetWinPath(), droppath + L'\\' + name, FALSE))
			{
				//the copy operation failed! Get out of here!
				ShowErrorMessage();
				return FALSE;
			}
		}
		CString destPath(droppath + L'\\' + name);
		copiedFiles.AddPath(CTGitPath(destPath.Mid(worktreePathLen + 1))); //add the new filepath
	}
	//now add all the newly copied files to the working copy
	CGitProgressDlg progDlg;
	theApp.m_pMainWnd = &progDlg;
	AddProgressCommand addCommand;
	progDlg.SetCommand(&addCommand);
	addCommand.SetPathList(copiedFiles);
	progDlg.DoModal();
	bRet = !progDlg.DidErrorsOccur();

	return bRet;
}

void DropCopyAddCommand::ShowErrorMessage()
{
	CFormatMessageWrapper errorDetails;
	CString strMessage;
	strMessage.Format(IDS_ERR_COPYFILES, static_cast<LPCTSTR>(errorDetails));
	MessageBox(GetExplorerHWND(), strMessage, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
}
