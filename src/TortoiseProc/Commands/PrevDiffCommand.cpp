// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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
#include "PrevDiffCommand.h"
#include "GitDiff.h"
#include "GitStatus.h"
#include "MessageBox.h"
#include "ChangedDlg.h"
#include "LogDlgHelper.h"
#include "FileDiffDlg.h"

bool PrevDiffCommand::Execute()
{
	bool bRet = false;
	//bool bAlternativeTool = !!parser.HasKey(_T("alternative"));
	if (this->orgCmdLinePath.IsDirectory())
	{
		CFileDiffDlg dlg;

		dlg.m_strRev1 = GIT_REV_ZERO;
		dlg.m_strRev2 = _T("HEAD~1");
		dlg.m_sFilter = this->cmdLinePath.GetGitPathString();

		//dlg.m_pathList = CTGitPathList(cmdLinePath);
		dlg.DoModal();
		bRet = true;
	}
	else
	{
		CGitDiff diff;
//		diff.SetAlternativeTool(bAlternativeTool);
		GitStatus st;
		st.GetStatus(cmdLinePath);

		if (1)
		{
			CString hash;
			CString logout;

			CLogDataVector revs;
			CLogCache cache;
			revs.m_pLogCache=&cache;

			revs.ParserFromLog(&cmdLinePath,2,CGit::LOG_INFO_ONLY_HASH);

			if( revs.size() != 2)
			{
				CMessageBox::Show(hWndExplorer, IDS_ERR_NOPREVREVISION, IDS_APPNAME, MB_ICONERROR);
				bRet = false;
			}
			else
			{
				CGitDiff diff;
				bRet = !!diff.Diff(&cmdLinePath,&cmdLinePath, GIT_REV_ZERO, revs.GetGitRevAt(1).m_CommitHash.ToString());
			}
		}
		else
		{
			//if (st.GetLastErrorMsg().IsEmpty())
			{
				CMessageBox::Show(hWndExplorer, IDS_ERR_NOPREVREVISION, IDS_APPNAME, MB_ICONERROR);
			}
			//else
			//{
			//	CMessageBox::Show(hWndExplorer, IDS_ERR_NOSTATUS, IDS_APPNAME, MB_ICONERROR);
			//s}
		}
	}
	return bRet;
}
