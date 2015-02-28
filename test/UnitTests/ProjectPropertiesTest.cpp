// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit
// Copyright (C) 2003-2011 - TortoiseSVN

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

#include "stdafx.h"
#include "ProjectProperties.h"

TEST(ProjectPropertiesTest, ParseBugIDs)
{
	ProjectProperties props;
	props.sCheckRe = _T("PAF-[0-9]+");
	props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
	CString sRet = props.FindBugID(_T("This is a test for PAF-88"));
	ASSERT_TRUE(sRet.IsEmpty());
	props.sCheckRe = _T("[Ii]ssue #?(\\d+)");
	props.regExNeedUpdate = true;
	sRet = props.FindBugID(_T("Testing issue #99"));
	sRet.Trim();
	ASSERT_STREQ(_T("99"), sRet);
	props.sCheckRe = _T("[Ii]ssues?:?(\\s*(,|and)?\\s*#\\d+)+");
	props.sBugIDRe = _T("(\\d+)");
	props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
	props.regExNeedUpdate = true;
	sRet = props.FindBugID(_T("This is a test for Issue #7463,#666"));
	ASSERT_TRUE(props.HasBugID(_T("This is a test for Issue #7463,#666")));
	ASSERT_FALSE(props.HasBugID(_T("This is a test for Issue 7463,666")));
	sRet.Trim();
	ASSERT_STREQ(_T("666 7463"), sRet);
	sRet = props.FindBugID(_T("This is a test for Issue #850,#1234,#1345"));
	sRet.Trim();
	ASSERT_STREQ(_T("850 1234 1345"), sRet);
	props.sCheckRe = _T("^\\[(\\d+)\\].*");
	props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
	props.regExNeedUpdate = true;
	sRet = props.FindBugID(_T("[000815] some stupid programming error fixed"));
	sRet.Trim();
	ASSERT_STREQ(_T("000815"), sRet);
	props.sCheckRe = _T("\\[\\[(\\d+)\\]\\]\\]");
	props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
	props.regExNeedUpdate = true;
	sRet = props.FindBugID(_T("test test [[000815]]] some stupid programming error fixed"));
	sRet.Trim();
	ASSERT_STREQ(_T("000815"), sRet);
	ASSERT_TRUE(props.HasBugID(_T("test test [[000815]]] some stupid programming error fixed")));
	ASSERT_FALSE(props.HasBugID(_T("test test [000815]] some stupid programming error fixed")));
}
