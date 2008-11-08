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
#include "UpdateCommand.h"

#include "UpdateDlg.h"
#include "SVNProgressDlg.h"
#include "Hooks.h"
#include "MessageBox.h"

bool UpdateCommand::Execute()
{
	SVNRev rev = SVNRev(_T("HEAD"));
	int options = 0;
	svn_depth_t depth = svn_depth_unknown;
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().StartUpdate(pathList, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			CMessageBox::Show(hwndExplorer, temp, _T("TortoiseSVN"), MB_ICONERROR);
			return FALSE;
		}
	}
	if ((parser.HasKey(_T("rev")))&&(!parser.HasVal(_T("rev"))))
	{
		CUpdateDlg dlg;
		if (pathList.GetCount()>0)
			dlg.m_wcPath = pathList[0];
		if (dlg.DoModal() == IDOK)
		{
			rev = dlg.Revision;
			depth = dlg.m_depth;
			options |= dlg.m_bNoExternals ? ProgOptIgnoreExternals : 0;
		}
		else 
			return FALSE;
	}
	else
	{
		if (parser.HasVal(_T("rev")))
			rev = SVNRev(parser.GetVal(_T("rev")));
		if (parser.HasKey(_T("nonrecursive")))
			depth = svn_depth_empty;
		if (parser.HasKey(_T("ignoreexternals")))
			options |= ProgOptIgnoreExternals;
	}

	CSVNProgressDlg progDlg;
	theApp.m_pMainWnd = &progDlg;
	progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Update);
	if (parser.HasVal(_T("closeonend")))
		progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
	progDlg.SetOptions(options);
	progDlg.SetPathList(pathList);
	progDlg.SetRevision(rev);
	progDlg.SetDepth(depth);
	progDlg.DoModal();
	return !progDlg.DidErrorsOccur();
}
