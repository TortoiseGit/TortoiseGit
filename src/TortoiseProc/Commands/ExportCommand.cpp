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
#include "ExportCommand.h"

#include "ExportDlg.h"
#include "ProgressDlg.h"
#include "GitAdminDir.h"
#include "ProgressDlg.h"
#include "BrowseFolder.h"
#include "DirFileEnum.h"
#include "MessageBox.h"
#include "GitStatus.h"

bool ExportCommand::Execute()
{
	bool bRet = false;

		// ask from where the export has to be done
	CExportDlg dlg;
	
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		cmd.Format(_T("git.exe archive --format=zip --verbose %s >\"%s\""),
					dlg.m_VersionName,
					dlg.m_strExportDirectory);
		CProgressDlg pro;
		pro.m_GitCmd=cmd;
		pro.DoModal();
		return TRUE;
	}
	return bRet;
}
