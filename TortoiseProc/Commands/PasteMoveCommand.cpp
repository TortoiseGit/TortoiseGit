// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "PasteMoveCommand.h"

#include "ProgressDlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "SVNStatus.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

bool PasteMoveCommand::Execute()
{
	CString sDroppath = parser.GetVal(_T("droptarget"));
	CTSVNPath dropPath(sDroppath);
	ProjectProperties props;
	props.ReadProps(dropPath);
	if (dropPath.IsAdminDir())
		return FALSE;
	SVN svn;
	SVNStatus status;
	unsigned long count = 0;
	pathList.RemoveAdminPaths();
	CString sNewName;
	CProgressDlg progress;
	progress.SetTitle(IDS_PROC_MOVING);
	progress.SetAnimation(IDR_MOVEANI);
	progress.SetTime(true);
	progress.ShowModeless(CWnd::FromHandle(hwndExplorer));
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		CTSVNPath destPath;
		if (sNewName.IsEmpty())
			destPath = CTSVNPath(sDroppath+_T("\\")+pathList[nPath].GetFileOrDirectoryName());
		else
			destPath = CTSVNPath(sDroppath+_T("\\")+sNewName);
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
			destPath.SetFromWin(sDroppath+_T("\\")+dlg.m_name);
		}
		svn_wc_status_kind s = status.GetAllStatus(pathList[nPath]);
		if ((s == svn_wc_status_none)||(s == svn_wc_status_unversioned)||(s == svn_wc_status_ignored))
		{
			// source file is unversioned: move the file to the target, then add it
			MoveFile(pathList[nPath].GetWinPath(), destPath.GetWinPath());
			if (!svn.Add(CTSVNPathList(destPath), &props, svn_depth_infinity, true, false, true))
			{
				TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
				CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return FALSE;		//get out of here
			}
			CShellUpdater::Instance().AddPathForUpdate(destPath);
		}
		else
		{
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
		}
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