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
#include "DropMoveCommand.h"

#include "ProgressDlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

bool DropMoveCommand::Execute()
{
	CString droppath = parser.GetVal(_T("droptarget"));
	if (CTSVNPath(droppath).IsAdminDir())
		return FALSE;
	SVN svn;
	unsigned long count = 0;
	pathList.RemoveAdminPaths();
	CString sNewName;
	if ((parser.HasKey(_T("rename")))&&(pathList.GetCount()==1))
	{
		// ask for a new name of the source item
		do 
		{
			CRenameDlg renDlg;
			renDlg.m_windowtitle.LoadString(IDS_PROC_MOVERENAME);
			renDlg.m_name = pathList[0].GetFileOrDirectoryName();
			if (renDlg.DoModal() != IDOK)
			{
				return FALSE;
			}
			sNewName = renDlg.m_name;
		} while(sNewName.IsEmpty() || PathFileExists(droppath+_T("\\")+sNewName));
	}
	CProgressDlg progress;
	if (progress.IsValid())
	{
		progress.SetTitle(IDS_PROC_MOVING);
		progress.SetAnimation(IDR_MOVEANI);
		progress.SetTime(true);
		progress.ShowModeless(CWnd::FromHandle(hwndExplorer));
	}
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		CTSVNPath destPath;
		if (sNewName.IsEmpty())
			destPath = CTSVNPath(droppath+_T("\\")+pathList[nPath].GetFileOrDirectoryName());
		else
			destPath = CTSVNPath(droppath+_T("\\")+sNewName);
		if (destPath.Exists())
		{
			CString name = pathList[nPath].GetFileOrDirectoryName();
			if (!sNewName.IsEmpty())
				name = sNewName;
			progress.Stop();
			CRenameDlg dlg;
			dlg.m_name = name;
			dlg.m_windowtitle.Format(IDS_PROC_NEWNAMEMOVE, (LPCTSTR)name);
			if (dlg.DoModal() != IDOK)
			{
				return FALSE;
			}
			destPath.SetFromWin(droppath+_T("\\")+dlg.m_name);
		} 
		if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath, FALSE))
		{
			if (svn.Err && (svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE ||
				svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
			{
				// file/folder seems to have local modifications. Ask the user if
				// a force is requested.
				CString temp = svn.GetLastErrorMessage();
				CString sQuestion(MAKEINTRESOURCE(IDS_PROC_FORCEMOVE));
				temp += _T("\n") + sQuestion;
				if (CMessageBox::Show(hwndExplorer, temp, _T("TortoiseSVN"), MB_YESNO)==IDYES)
				{
					if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath, TRUE))
					{
						CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return FALSE;		//get out of here
					}
					CShellUpdater::Instance().AddPathForUpdate(destPath);
				}
			}
			else
			{
				TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
				CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return FALSE;		//get out of here
			}
		} 
		else
			CShellUpdater::Instance().AddPathForUpdate(destPath);
		count++;
		if (progress.IsValid())
		{
			progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
			progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
			progress.SetProgress(count, pathList.GetCount());
		}
		if ((progress.IsValid())&&(progress.HasUserCancelled()))
		{
			CMessageBox::Show(hwndExplorer, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
			return FALSE;
		}
	}
	return true;
}
