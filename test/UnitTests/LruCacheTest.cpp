// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2016 - TortoiseSVN

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
#include "LruCache.h"

TEST(LruCache, InsertGet)
{
	LruCache<int, int> cache(2);
	cache.insert_or_assign(1, 100);
	cache.insert_or_assign(2, 200);
	// Emulate access to key '1'
	cache.try_get(1);

	// Add third entry. Key '2' should be evicted/
	cache.insert_or_assign(3, 300);

	EXPECT_NE(nullptr, cache.try_get(1));
	EXPECT_EQ(100, *cache.try_get(1));

	EXPECT_EQ(nullptr, cache.try_get(2));

	EXPECT_NE(nullptr, cache.try_get(3));
	EXPECT_EQ(300, *cache.try_get(3));

	// Test LruCache.clear()
	cache.clear();
	EXPECT_EQ(nullptr, cache.try_get(1));
	EXPECT_EQ(nullptr, cache.try_get(2));
	EXPECT_EQ(nullptr, cache.try_get(3));

	cache.insert_or_assign(1, 100);
	cache.insert_or_assign(3, 300);

	EXPECT_NE(nullptr, cache.try_get(1));
	EXPECT_EQ(100, *cache.try_get(1));
	EXPECT_NE(nullptr, cache.try_get(3));
	EXPECT_EQ(300, *cache.try_get(3));

	// Replace value associated with key '1'.
	cache.insert_or_assign(1, 101);
	EXPECT_NE(nullptr, cache.try_get(1));

	EXPECT_EQ(101, *cache.try_get(1));
	EXPECT_NE(nullptr, cache.try_get(3));
	EXPECT_EQ(300, *cache.try_get(3));
}

TEST(LruCache, EmptyCache)
{
	LruCache<int, int> cache(5);
	EXPECT_EQ(nullptr, cache.try_get(1));
	EXPECT_EQ(nullptr, cache.try_get(2));
	EXPECT_EQ(nullptr, cache.try_get(3));
}
