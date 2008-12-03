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
#include "UnLockCommand.h"

#include "UnlockDlg.h"
#include "SVNProgressDlg.h"

bool UnLockCommand::Execute()
{
	bool bRet = false;
	CUnlockDlg unlockDlg;
	unlockDlg.m_pathList = pathList;
	if (unlockDlg.DoModal()==IDOK)
	{
		if (unlockDlg.m_pathList.GetCount() != 0)
		{
			CSVNProgressDlg progDlg;
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Unlock);
			progDlg.SetOptions(parser.HasKey(_T("force")) ? ProgOptLockForce : ProgOptNone);
			progDlg.SetPathList(unlockDlg.m_pathList);
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.DoModal();
			bRet = !progDlg.DidErrorsOccur();
		}
	}
	return bRet;
}