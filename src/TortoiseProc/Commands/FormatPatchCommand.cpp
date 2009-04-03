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
#include "FormatPatchCommand.h"

#include "MessageBox.h"
#include "FormatPatchDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool FormatPatchCommand::Execute()
{
	CFormatPatchDlg dlg;
//	dlg.m_bIsTag=TRUE;
	
	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString range;

		switch(dlg.m_Radio)
		{
		case IDC_RADIO_SINCE:
			range=dlg.m_Since;
			break;
		case IDC_RADIO_NUM:
			range.Format(_T("-n%d"),dlg.m_Num);
			break;
		case IDC_RADIO_RANGE:
			range.Format(_T("%s..%s"),dlg.m_From,dlg.m_To);
			break;
		}
		cmd.Format(_T("git.exe format-patch -o \"%s\" %s"),
			dlg.m_Dir,
			range
			);

		CString out;
		if(g_Git.Run(cmd,&out,CP_ACP))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
		}
		return TRUE;
		
	}
	return FALSE;
}
