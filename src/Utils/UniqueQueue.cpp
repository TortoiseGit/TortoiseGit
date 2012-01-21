// TortoiseGit - a Windows shell extension for easy version control

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
#include "StdAfx.h"
#include "UniqueQueue.h"


#if defined(_DEBUG)
// Some test cases for these classes
static class UniqueQueueTests
{
public:
	UniqueQueueTests()
	{
		UniqueQueue<CString> myQueue;

		myQueue.Push(CString(L"one"));
		ATLASSERT(myQueue.size() == 1);
		myQueue.Push(CString(L"two"));
		ATLASSERT(myQueue.size() == 2);
		myQueue.Push(CString(L"one"));
		ATLASSERT(myQueue.size() == 2);
		myQueue.Push(CString(L"three"));
		ATLASSERT(myQueue.size() == 3);
		myQueue.Push(CString(L"three"));
		ATLASSERT(myQueue.size() == 3);
		myQueue.erase(CString(L"three"));
		ATLASSERT(myQueue.size() == 2);
		myQueue.Push(CString(L"three"));
		ATLASSERT(myQueue.size() == 3);

		ATLASSERT(myQueue.Pop().Compare(L"two") == 0);
		ATLASSERT(myQueue.Pop().Compare(L"one") == 0);
		ATLASSERT(myQueue.Pop().Compare(L"three") == 0);
	}
} UniqueQueueTestsObject;

#endif // _DEBUG
