// TortoiseGit - a Windows shell extension for easy version control

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
#include "StdAfx.h"
#include "RenameCommand.h"

#include "MessageBox.h"
//#include "SVNProgressDlg.h"
//#include "ProgressDlg.h"
#include "RenameDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool RenameCommand::Execute()
{
	bool bRet = false;
	CString filename = cmdLinePath.GetFileOrDirectoryName();
	CString basePath = cmdLinePath.GetContainingDirectory().GetGitPathString();
	//::SetCurrentDirectory(basePath);

	// show the rename dialog until the user either cancels or enters a new
	// name (one that's different to the original name
	CString sNewName;
	do
	{
		CRenameDlg dlg;
		dlg.m_name = filename;
		if (dlg.DoModal() != IDOK)
			return FALSE;
		sNewName = dlg.m_name;
	} while(PathIsRelative(sNewName) && !PathIsURL(sNewName) && (sNewName.IsEmpty() || (sNewName.Compare(filename)==0)));

	if(!basePath.IsEmpty())
		sNewName=basePath+"/"+sNewName;

	CString cmd;
	CString output;
	cmd.Format(_T("git.exe mv -- \"%s\" \"%s\""),
					cmdLinePath.GetGitPathString(),
					sNewName);

	if (g_Git.Run(cmd, &output, CP_UTF8))
	{
		CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_OK);
	}

	CTGitPath newpath;
	newpath.SetFromGit(sNewName);

	CShellUpdater::Instance().AddPathForUpdate(newpath);
#if 0
	TRACE(_T("rename file %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)sNewName);
	CTSVNPath destinationPath(basePath);
	if (PathIsRelative(sNewName) && !PathIsURL(sNewName))
		destinationPath.AppendPathString(sNewName);
	else
		destinationPath.SetFromWin(sNewName);
	// check if a rename just with case is requested: that's not possible on windows file systems
	// and we have to show an error.
	if (cmdLinePath.GetWinPathString().CompareNoCase(destinationPath.GetWinPathString())==0)
	{
		//rename to the same file!
		CString sHelpPath = theApp.m_pszHelpFilePath;
		sHelpPath += _T("::/tsvn-dug-rename.html#tsvn-dug-renameincase");
		CMessageBox::Show(hwndExplorer, IDS_PROC_CASERENAME, IDS_APPNAME, MB_OK|MB_HELP, sHelpPath);
	}
	else
	{
		CString sMsg;
		if (SVN::PathIsURL(cmdLinePath))
		{
			// rename an URL.
			// Ask for a commit message, then rename directly in
			// the repository
			CInputLogDlg input;
			CString sUUID;
			SVN svn;
			svn.GetRepositoryRootAndUUID(cmdLinePath, sUUID);
			input.SetUUID(sUUID);
			CString sHint;
			sHint.Format(IDS_INPUT_MOVE, (LPCTSTR)cmdLinePath.GetSVNPathString(), (LPCTSTR)destinationPath.GetSVNPathString());
			input.SetActionText(sHint);
			if (input.DoModal() == IDOK)
			{
				sMsg = input.GetLogMessage();
			}
			else
			{
				return FALSE;
			}
		}
		if ((cmdLinePath.IsDirectory())||(pathList.GetCount() > 1))
		{
			// renaming a directory can take a while: use the
			// progress dialog to show the progress of the renaming
			// operation.
			CSVNProgressDlg progDlg;
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Rename);
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.SetPathList(pathList);
			progDlg.SetUrl(destinationPath.GetWinPathString());
			progDlg.SetCommitMessage(sMsg);
			progDlg.SetRevision(SVNRev::REV_WC);
			progDlg.DoModal();
			bRet = !progDlg.DidErrorsOccur();
		}
		else
		{
			SVN svn;
			CString sFilemask = cmdLinePath.GetFilename();
			if (sFilemask.ReverseFind('.')>=0)
			{
				sFilemask = sFilemask.Left(sFilemask.ReverseFind('.'));
			}
			else
				sFilemask.Empty();
			CString sNewMask = sNewName;
			if (sNewMask.ReverseFind('.'>=0))
			{
				sNewMask = sNewMask.Left(sNewMask.ReverseFind('.'));
			}
			else
				sNewMask.Empty();

			if (((!sFilemask.IsEmpty()) && (parser.HasKey(_T("noquestion")))) ||
				(cmdLinePath.GetFileExtension().Compare(destinationPath.GetFileExtension())!=0))
			{
				if (!svn.Move(CTSVNPathList(cmdLinePath), destinationPath, TRUE, sMsg))
				{
					TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
					CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
				}
				else
					bRet = true;
			}
			else
			{
				// when refactoring, multiple files have to be renamed
				// at once because those files belong together.
				// e.g. file.aspx, file.aspx.cs, file.aspx.resx
				CTSVNPathList renlist;
				CSimpleFileFind filefind(cmdLinePath.GetDirectory().GetWinPathString(), sFilemask+_T(".*"));
				while (filefind.FindNextFileNoDots())
				{
					if (!filefind.IsDirectory())
						renlist.AddPath(CTSVNPath(filefind.GetFilePath()));
				}
				if (renlist.GetCount()<=1)
				{
					// we couldn't find any other matching files
					// just do the default...
					if (!svn.Move(CTSVNPathList(cmdLinePath), destinationPath, TRUE, sMsg))
					{
						TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
						CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
					}
					else
					{
						bRet = true;
						CShellUpdater::Instance().AddPathForUpdate(destinationPath);
					}
				}
				else
				{
					std::map<CString, CString> renmap;
					CString sTemp;
					CString sRenList;
					for (int i=0; i<renlist.GetCount(); ++i)
					{
						CString sFilename = renlist[i].GetFilename();
						CString sNewFilename = sNewMask + sFilename.Mid(sFilemask.GetLength());
						sTemp.Format(_T("\n%s -> %s"), (LPCTSTR)sFilename, (LPCTSTR)sNewFilename);
						if (!renlist[i].IsEquivalentTo(cmdLinePath))
							sRenList += sTemp;
						renmap[renlist[i].GetWinPathString()] = renlist[i].GetContainingDirectory().GetWinPathString()+_T("\\")+sNewFilename;
					}
					CString sRenameMultipleQuestion;
					sRenameMultipleQuestion.Format(IDS_PROC_MULTIRENAME, (LPCTSTR)sRenList);
					UINT idret = CMessageBox::Show(hwndExplorer, sRenameMultipleQuestion, _T("TortoiseGit"), MB_ICONQUESTION|MB_YESNOCANCEL);
					if (idret == IDYES)
					{
						CProgressDlg progress;
						progress.SetTitle(IDS_PROC_MOVING);
						progress.SetAnimation(IDR_MOVEANI);
						progress.SetTime(true);
						progress.ShowModeless(CWnd::FromHandle(hwndExplorer));
						DWORD count = 1;
						for (std::map<CString, CString>::iterator it=renmap.begin(); it != renmap.end(); ++it)
						{
							progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, (LPCTSTR)it->first);
							progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, (LPCTSTR)it->second);
							progress.SetProgress(count, renmap.size());
							if (!svn.Move(CTSVNPathList(CTSVNPath(it->first)), CTSVNPath(it->second), TRUE, sMsg))
							{
								if (svn.Err->apr_err == SVN_ERR_ENTRY_NOT_FOUND)
								{
									bRet = !!MoveFile(it->first, it->second);
								}
								else
								{
									TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
									CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
									bRet = false;
								}
							}
							else
							{
								bRet = true;
								CShellUpdater::Instance().AddPathForUpdate(CTSVNPath(it->second));
							}
						}
						progress.Stop();
					}
					else if (idret == IDNO)
					{
						// no, user wants to just rename the file he selected
						if (!svn.Move(CTSVNPathList(cmdLinePath), destinationPath, TRUE, sMsg))
						{
							TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
							CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
						}
						else
						{
							bRet = true;
							CShellUpdater::Instance().AddPathForUpdate(destinationPath);
						}
					}
					else if (idret == IDCANCEL)
					{
						// nothing
					}
				}
			}
		}
	}
#endif
	CShellUpdater::Instance().Flush();
	return bRet;
}
