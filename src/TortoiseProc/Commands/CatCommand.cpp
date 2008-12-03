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
#include "CatCommand.h"

#include "PathUtils.h"
#include "SVN.h"

bool CatCommand::Execute()
{
	CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
	CString revision = parser.GetVal(_T("revision"));
	CString pegrevision = parser.GetVal(_T("pegrevision"));
	SVNRev rev = SVNRev(revision);
	if (!rev.IsValid())
		rev = SVNRev::REV_HEAD;
	SVNRev pegrev = SVNRev(pegrevision);
	if (!pegrev.IsValid())
		pegrev = SVNRev::REV_HEAD;
	SVN svn;
	if (!svn.Cat(cmdLinePath, pegrev, rev, CTSVNPath(savepath)))
	{
		::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		::DeleteFile(savepath);
		return false;
	} 
	return true;
}
