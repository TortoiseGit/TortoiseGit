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
#include "PrevDiffCommand.h"
#include "ChangedDlg.h"
#include "SVNDiff.h"
#include "SVNStatus.h"
#include "MessageBox.h"

bool PrevDiffCommand::Execute()
{
	bool bRet = false;
	bool bAlternativeTool = !!parser.HasKey(_T("alternative"));
	if (cmdLinePath.IsDirectory())
	{
		CChangedDlg dlg;
		dlg.m_pathList = CTSVNPathList(cmdLinePath);
		dlg.DoModal();
		bRet = true;
	}
	else
	{
		SVNDiff diff;
		diff.SetAlternativeTool(bAlternativeTool);
		SVNStatus st;
		st.GetStatus(cmdLinePath);
		if (st.status && st.status->entry && st.status->entry->cmt_rev)
		{
			SVNDiff diff(NULL, hWndExplorer);
			bRet = diff.ShowCompare(cmdLinePath, SVNRev::REV_WC, cmdLinePath, st.status->entry->cmt_rev - 1, st.status->entry->cmt_rev);
		}
		else
		{
			if (st.GetLastErrorMsg().IsEmpty())
			{
				CMessageBox::Show(hWndExplorer, IDS_ERR_NOPREVREVISION, IDS_APPNAME, MB_ICONERROR);
			}
			else
			{
				CMessageBox::Show(hWndExplorer, IDS_ERR_NOSTATUS, IDS_APPNAME, MB_ICONERROR);
			}
		}
	}
	return bRet;
}
