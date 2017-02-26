// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2017 - TortoiseGit

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
#include "gtest/gtest.h"

int _tmain(int argc, _TCHAR* argv[])
{
	git_libgit2_init();
	// Since Google Mock depends on Google Test, InitGoogleMock() is
	// also responsible for initializing Google Test. Therefore there's
	// no need for calling testing::InitGoogleTest() separately.
	testing::InitGoogleMock(&argc, argv);
	int result = RUN_ALL_TESTS();
	git_libgit2_shutdown();
	return result;
}

CString g_sGroupingUUID;
CString sOrigCWD;
