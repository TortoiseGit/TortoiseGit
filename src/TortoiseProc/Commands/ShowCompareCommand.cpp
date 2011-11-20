// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "ShowCompareCommand.h"
//#include "SVNDiff.h"


bool ShowCompareCommand::Execute()
{
#if 0
	bool		bRet = false;
	SVNDiff		diff(NULL, hwndExplorer);

	SVNRev		rev1;
	SVNRev		rev2;
	SVNRev		pegrev;
	SVNRev		headpeg;

	CTSVNPath	url1 = CTSVNPath(parser.GetVal(_T("url1")));
	CTSVNPath	url2 = CTSVNPath(parser.GetVal(_T("url2")));
	bool		ignoreancestry = !!parser.HasKey(_T("ignoreancestry"));
	bool		blame = !!parser.HasKey(_T("blame"));
	bool		unified = !!parser.HasKey(_T("unified"));

	if (parser.HasVal(_T("revision1")))
		rev1 = SVNRev(parser.GetVal(_T("revision1")));
	if (parser.HasVal(_T("revision2")))
		rev2 = SVNRev(parser.GetVal(_T("revision2")));
	if (parser.HasVal(_T("pegrevision")))
		pegrev = SVNRev(parser.GetVal(_T("pegrevision")));
	if (parser.HasVal(_T("headpegrevision")))
		diff.SetHEADPeg(SVNRev(parser.GetVal(_T("headpegrevision"))));
	diff.SetAlternativeTool(!!parser.HasKey(_T("alternatediff")));

	if (unified)
		bRet = diff.ShowUnifiedDiff(url1, rev1, url2, rev2, pegrev, ignoreancestry);
	else
		bRet = diff.ShowCompare(url1, rev1, url2, rev2, pegrev, ignoreancestry, blame);

	return bRet;
#endif 
	return 0;
}
