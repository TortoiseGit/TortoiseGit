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
#include "RepositoryBrowserCommand.h"

#include "RepositoryBrowser.h"
#include "URLDlg.h"
#include "SVN.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"

bool RepositoryBrowserCommand::Execute()
{
	CString url;
	BOOL bFile = FALSE;
	SVN svn;
	if (!cmdLinePath.IsEmpty())
	{
		if (cmdLinePath.GetSVNPathString().Left(4).CompareNoCase(_T("svn:"))==0)
		{
			// If the path starts with "svn:" and there is another protocol
			// found in the path (a "://" found after the "svn:") then
			// remove "svn:" from the beginning of the path.
			if (cmdLinePath.GetSVNPathString().Find(_T("://"), 4)>=0)
				cmdLinePath.SetFromSVN(cmdLinePath.GetSVNPathString().Mid(4));
		}

		url = svn.GetURLFromPath(cmdLinePath);

		if (url.IsEmpty())
		{
			if (SVN::PathIsURL(cmdLinePath))
				url = cmdLinePath.GetSVNPathString();
			else if (svn.IsRepository(cmdLinePath))
			{
				// The path points to a local repository.
				// Add 'file:///' so the repository browser recognizes
				// it as an URL to the local repository.
				if (cmdLinePath.GetWinPathString().GetAt(0) == '\\')	// starts with '\' means an UNC path
				{
					CString p = cmdLinePath.GetWinPathString();
					p.TrimLeft('\\');
					if (CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(p)).Find('%') >= 0)
					{
						// the path has special chars which will get escaped!
						url = _T("file:///\\")+p;
					}
					else
						url = _T("file://")+p;
				}
				else
					url = _T("file:///")+cmdLinePath.GetWinPathString();
				url.Replace('\\', '/');
			}
		}
	}
	if (cmdLinePath.GetUIPathString().Left(7).CompareNoCase(_T("file://"))==0)
	{
		cmdLinePath.SetFromUnknown(cmdLinePath.GetUIPathString().Mid(7));
	}
	bFile = PathFileExists(cmdLinePath.GetWinPath()) ? !cmdLinePath.IsDirectory() : FALSE;

	if (url.IsEmpty())
	{
		CURLDlg urldlg;
		if (urldlg.DoModal() != IDOK)
		{
			return false;
		}
		url = urldlg.m_url;
	}

	CString val = parser.GetVal(_T("rev"));
	SVNRev rev(val);
	CRepositoryBrowser dlg(url, rev);
	if (!cmdLinePath.IsUrl())
		dlg.m_ProjectProperties.ReadProps(cmdLinePath);
	else
	{
		if (parser.HasVal(_T("projectpropertiespath")))
		{
			dlg.m_ProjectProperties.ReadProps(CTSVNPath(parser.GetVal(_T("projectpropertiespath"))));
		}
	}
	dlg.m_path = cmdLinePath;
	dlg.DoModal();
	return true;
}