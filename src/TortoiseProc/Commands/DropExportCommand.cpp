// TortoiseSVN - a Windows shell extension for easy version control

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
#include "DropExportCommand.h"
#include "MessageBox.h"
#include "Git.h"
#include "GitAdminDir.h"
#include "DirFileEnum.h"
#include "SysProgressDlg.h"

bool DropExportCommand::Execute()
{
	bool bRet = true;
#if 0
	CString droppath = parser.GetVal(_T("droptarget"));
	if (CTGitPath(droppath).IsAdminDir())
		return false;

	SVN svn;
	if ((pathList.GetCount() == 1)&&
		(pathList[0].IsEquivalentTo(CTSVNPath(droppath))))
	{
		// exporting to itself:
		// remove all svn admin dirs, effectively unversion the 'exported' folder.
		CString msg;
		msg.Format(IDS_PROC_EXPORTUNVERSION, (LPCTSTR)droppath);
		if (CMessageBox::Show(hwndExplorer, msg, _T("TortoiseGit"), MB_ICONQUESTION|MB_YESNO) == IDYES)
		{
			CProgressDlg progress;
			progress.SetTitle(IDS_PROC_UNVERSION);
			progress.SetAnimation(IDR_MOVEANI);
			progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
			progress.SetTime(true);
			progress.ShowModeless(hwndExplorer);
			std::vector<CTSVNPath> removeVector;

			CDirFileEnum lister(droppath);
			CString srcFile;
			bool bFolder = false;
			while (lister.NextFile(srcFile, &bFolder))
			{
				CTSVNPath item(srcFile);
				if ((bFolder)&&(g_SVNAdminDir.IsAdminDirName(item.GetFileOrDirectoryName())))
				{
					removeVector.push_back(item);
				}
			}
			DWORD count = 0;
			for (std::vector<CTSVNPath>::iterator it = removeVector.begin(); (it != removeVector.end()) && (!progress.HasUserCancelled()); ++it)
			{
				progress.FormatPathLine(1, IDS_SVNPROGRESS_UNVERSION, (LPCTSTR)it->GetWinPath());
				progress.SetProgress(count, removeVector.size());
				count++;
				it->Delete(false);
			}
			progress.Stop();
		}
		else
			return false;
	}
	else
	{
		for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
		{
			CString dropper = droppath + _T("\\") + pathList[nPath].GetFileOrDirectoryName();
			if (PathFileExists(dropper))
			{
				CString sMsg;
				CString sBtn1(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_OVERWRITE));
				CString sBtn2(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_RENAME));
				CString sBtn3(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_CANCEL));
				sMsg.Format(IDS_PROC_OVERWRITEEXPORT, (LPCTSTR)dropper);
				UINT ret = CMessageBox::Show(hwndExplorer, sMsg, _T("TortoiseGit"), MB_DEFBUTTON1, IDI_QUESTION, sBtn1, sBtn2, sBtn3);
				if (ret==2)
				{
					dropper.Format(IDS_PROC_EXPORTFOLDERNAME, (LPCTSTR)droppath, (LPCTSTR)pathList[nPath].GetFileOrDirectoryName());
					int exportcount = 1;
					while (PathFileExists(dropper))
					{
						dropper.Format(IDS_PROC_EXPORTFOLDERNAME2, (LPCTSTR)droppath, exportcount++, (LPCTSTR)pathList[nPath].GetFileOrDirectoryName());
					}
				}
				else if (ret == 3)
					return false;
			}
			if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC ,SVNRev::REV_WC, FALSE, FALSE, svn_depth_infinity, hwndExplorer, parser.HasKey(_T("extended"))))
			{
				CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				bRet = false;
			}
		}
	}
#endif
	return bRet;
}
