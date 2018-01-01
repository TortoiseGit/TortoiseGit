// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015, 2018 - TortoiseGit
// Copyright (C) 2010 - TortoiseSVN

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
#include "UniqueQueue.h"

TEST(UniqueQueue, CString)
{
	UniqueQueue<CString> myQueue;
	EXPECT_EQ(0U, myQueue.size());
	EXPECT_TRUE(myQueue.Pop().IsEmpty());
	EXPECT_EQ(0U, myQueue.erase(L"doesnotexist"));
	myQueue.Push(CString(L"one"));
	EXPECT_EQ(1U, myQueue.size());
	myQueue.Push(CString(L"two"));
	EXPECT_EQ(2U, myQueue.size());
	myQueue.Push(CString(L"one"));
	EXPECT_EQ(2U, myQueue.size());
	myQueue.Push(CString(L"three"));
	EXPECT_EQ(3U, myQueue.size());
	EXPECT_EQ(3U, myQueue.erase(L"doesnotexist"));
	myQueue.Push(CString(L"three"));
	EXPECT_EQ(3U, myQueue.size());
	EXPECT_EQ(2U, myQueue.erase(CString(L"three")));
	EXPECT_EQ(2U, myQueue.size());
	myQueue.Push(CString(L"three"));
	EXPECT_EQ(3U, myQueue.size());

	EXPECT_TRUE(myQueue.Pop().Compare(L"two") == 0);
	EXPECT_EQ(2U, myQueue.size());
	EXPECT_TRUE(myQueue.Pop().Compare(L"one") == 0);
	EXPECT_EQ(1U, myQueue.size());
	EXPECT_TRUE(myQueue.Pop().Compare(L"three") == 0);
	EXPECT_EQ(0U, myQueue.size());
	EXPECT_TRUE(myQueue.Pop().IsEmpty());
}
