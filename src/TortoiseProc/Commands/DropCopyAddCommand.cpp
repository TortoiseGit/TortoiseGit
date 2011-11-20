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
#include "DropCopyAddCommand.h"

#include "GITProgressDlg.h"
#include "MessageBox.h"

bool DropCopyAddCommand::Execute()
{
	bool bRet = false;

	CString droppath = parser.GetVal(_T("droptarget"));
	if (CTGitPath(droppath).IsAdminDir())
		return FALSE;

	if(!CTGitPath(droppath).HasAdminDir(&g_Git.m_CurrentDir))
		return FALSE;

	orgPathList.RemoveAdminPaths();
	CTGitPathList copiedFiles;
	for(int nPath = 0; nPath < orgPathList.GetCount(); nPath++)
	{
		if (!orgPathList[nPath].IsEquivalentTo(CTGitPath(droppath)))
		{
			//copy the file to the new location
			CString name = orgPathList[nPath].GetFileOrDirectoryName();
			if (::PathFileExists(droppath+_T("\\")+name))
			{
				CString strMessage;
				strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, (LPCTSTR)(droppath+_T("\\")+name));
				int ret = CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseGit"), MB_YESNOCANCEL | MB_ICONQUESTION);
				if (ret == IDYES)
				{
					if (!::CopyFile(orgPathList[nPath].GetWinPath(), droppath+_T("\\")+name, FALSE))
					{
						//the copy operation failed! Get out of here!
						LPVOID lpMsgBuf;
						FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPTSTR) &lpMsgBuf,
							0,
							NULL 
							);
						CString strMessage;
						strMessage.Format(IDS_ERR_COPYFILES, (LPTSTR)lpMsgBuf);
						CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
						LocalFree( lpMsgBuf );
						return FALSE;
					}
				}
				if (ret == IDCANCEL)
				{
					return FALSE;		//cancel the whole operation
				}
			}
			else if (!CopyFile(orgPathList[nPath].GetWinPath(), droppath+_T("\\")+name, FALSE))
			{
				//the copy operation failed! Get out of here!
				LPVOID lpMsgBuf;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
					);
				CString strMessage;
				strMessage.Format(IDS_ERR_COPYFILES, lpMsgBuf);
				CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
				LocalFree( lpMsgBuf );
				return FALSE;
			}
			copiedFiles.AddPath(CTGitPath(droppath+_T("\\")+name));		//add the new filepath
		}
	}
	//now add all the newly copied files to the working copy
	CGitProgressDlg progDlg;
	theApp.m_pMainWnd = &progDlg;
	progDlg.SetCommand(CGitProgressDlg::GitProgress_Add);
	if (parser.HasVal(_T("closeonend")))
		progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
	progDlg.SetPathList(copiedFiles);
	ProjectProperties props;
	props.ReadPropsPathList(copiedFiles);
	progDlg.SetProjectProperties(props);
	progDlg.DoModal();
	bRet = !progDlg.DidErrorsOccur();

	return bRet;
}
