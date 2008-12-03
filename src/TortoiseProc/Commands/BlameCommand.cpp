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
#include "BlameCommand.h"

#include "BlameDlg.h"
#include "Blame.h"
#include "SVN.h"
#include "AppUtils.h"
#include "MessageBox.h"


bool BlameCommand::Execute()
{
	bool bRet = false;
	bool bShowDialog = true;
	CBlameDlg dlg;
	CString options;
	dlg.EndRev = SVNRev::REV_HEAD;
	if (parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")))
	{
		bShowDialog = false;
		dlg.StartRev = parser.GetLongVal(_T("startrev"));
		dlg.EndRev = parser.GetLongVal(_T("endrev"));
		if (parser.HasKey(_T("ignoreeol")) || parser.HasKey(_T("ignorespaces")) || parser.HasKey(_T("ignoreallspaces")))
		{
			options = SVN::GetOptionsString(parser.HasKey(_T("ignoreeol")), parser.HasKey(_T("ignorespaces")), parser.HasKey(_T("ignoreallspaces")));
		}
	}
	if ((!bShowDialog)||(dlg.DoModal() == IDOK))
	{
		CBlame blame;
		CString tempfile;
		CString logfile;
		if (bShowDialog)
			options = SVN::GetOptionsString(dlg.m_bIgnoreEOL, dlg.m_IgnoreSpaces);
		
		tempfile = blame.BlameToTempFile(cmdLinePath, dlg.StartRev, dlg.EndRev, 
			cmdLinePath.IsUrl() ? SVNRev() : SVNRev::REV_WC, logfile, 
			options, dlg.m_bIncludeMerge, TRUE, TRUE);
		if (!tempfile.IsEmpty())
		{
			if (dlg.m_bTextView)
			{
				//open the default text editor for the result file
				bRet = !!CAppUtils::StartTextViewer(tempfile);
			}
			else
			{
				CString sVal;
				if (parser.HasVal(_T("line")))
				{
					sVal = _T("/line:");
					sVal += parser.GetVal(_T("line"));
					sVal += _T(" ");
				}
				sVal += _T("/path:\"") + cmdLinePath.GetSVNPathString() + _T("\" ");
				if (bShowDialog)
				{
					if (dlg.m_bIgnoreEOL)
						sVal += _T("/ignoreeol ");
					switch (dlg.m_IgnoreSpaces)
					{
					case svn_diff_file_ignore_space_change:
						sVal += _T("/ignorespaces ");
						break;
					case svn_diff_file_ignore_space_all:
						sVal += _T("/ignoreallspaces ");
					}
				}
				else 
				{
					if (parser.HasKey(_T("ignoreeol")))
						sVal += _T("/ignoreeol ");
					if (parser.HasKey(_T("ignorespaces")))
						sVal += _T("/ignorespaces ");
					if (parser.HasKey(_T("ignoreallspaces")))
						sVal += _T("/ignoreallspaces ");
				}

				bRet = CAppUtils::LaunchTortoiseBlame(tempfile, logfile, cmdLinePath.GetFileOrDirectoryName(), sVal);
			}
		}
		else
		{
			CMessageBox::Show(hwndExplorer, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
	}
	return bRet;
}
