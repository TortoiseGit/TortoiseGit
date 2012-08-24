// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
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
#include "RebaseCommand.h"

#include "MessageBox.h"
//#include "SVNProgressDlg.h"
//#include "ProgressDlg.h"
#include "RebaseDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool RebaseCommand::Execute()
{
	bool bRet =false;

	if(!g_Git.CheckCleanWorkTree())
	{
		if(CMessageBox::Show(NULL,	IDS_ERROR_NOCLEAN_STASH,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return false;
			}

		}
		else
		{
			return false;
		}
	}

	while(1)
	{
		CRebaseDlg dlg;
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_PROC_RESTARTREBASE)));
		INT_PTR ret = dlg.DoModal();
		if( ret == IDOK)
		{
			bRet=true;
			return bRet;
		}
		if( ret == IDCANCEL)
		{
			bRet=false;
			return bRet;
		}
	}
	return bRet;
}
