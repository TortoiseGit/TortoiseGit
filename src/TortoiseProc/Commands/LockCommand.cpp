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
#include "LockCommand.h"

#include "LockDlg.h"
#include "SVNProgressDlg.h"

bool LockCommand::Execute()
{
	bool bRet = false;
	CLockDlg lockDlg;
	lockDlg.m_pathList = pathList;
	ProjectProperties props;
	props.ReadPropsPathList(pathList);
	lockDlg.SetProjectProperties(&props);
	if (pathList.AreAllPathsFiles() && !DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\ShowLockDlg"), TRUE)) && !props.nMinLockMsgSize)
	{
		// just lock the requested files
		CSVNProgressDlg progDlg;
		progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Lock);
		progDlg.SetPathList(pathList);
		if (parser.HasVal(_T("closeonend")))
			progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
		progDlg.DoModal();
		bRet = !progDlg.DidErrorsOccur();
	}
	else if (lockDlg.DoModal()==IDOK)
	{
		if (lockDlg.m_pathList.GetCount() != 0)
		{
			CSVNProgressDlg progDlg;
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Lock);
			progDlg.SetOptions(lockDlg.m_bStealLocks ? ProgOptLockForce : ProgOptNone);
			progDlg.SetPathList(lockDlg.m_pathList);
			progDlg.SetCommitMessage(lockDlg.m_sLockMessage);
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.DoModal();
			bRet = !progDlg.DidErrorsOccur();
		}
	}
	return bRet;
}