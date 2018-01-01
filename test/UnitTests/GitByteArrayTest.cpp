// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2018 - TortoiseGit

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
#include "gittype.h"

TEST(CGitByteArray, Empty)
{
	CGitByteArray byteArray;
	EXPECT_TRUE(byteArray.empty());
	EXPECT_EQ(0U, byteArray.size());
}

TEST(CGitByteArray, AppendByteArray)
{
	BYTE inputByteArray[] = { "12345789" };
	CGitByteArray byteArray;
	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(2 * sizeof(inputByteArray), byteArray.size());

	byteArray.clear();
	EXPECT_TRUE(byteArray.empty());
	EXPECT_EQ(0U, byteArray.size());
}

TEST(CGitByteArray, AppendByteArrayWithNulls)
{
	BYTE inputByteArray[] = { "1234\0""5789\0" };
	CGitByteArray byteArray;

	byteArray.append(inputByteArray, 0);
	EXPECT_EQ(0U, byteArray.size());

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(2 * sizeof(inputByteArray), byteArray.size());
}

TEST(CGitByteArray, AppendByteArrayRange)
{
	BYTE inputByteArray[] = { "1234\0""5789\0" };
	CGitByteArray byteArray1;
	byteArray1.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray1.size());

	CGitByteArray byteArray2;
	EXPECT_EQ(0U, byteArray2.append(byteArray1, 0));
	EXPECT_EQ(byteArray1.size(), byteArray2.size());

	EXPECT_EQ(0U, byteArray2.append(byteArray1, 4, 3));
	EXPECT_EQ(sizeof(inputByteArray), byteArray1.size());
	EXPECT_EQ(byteArray1.size(), byteArray2.size());

	EXPECT_EQ(0U, byteArray2.append(byteArray1, 4, 4));
	EXPECT_EQ(sizeof(inputByteArray), byteArray1.size());
	EXPECT_EQ(byteArray1.size(), byteArray2.size());

	EXPECT_EQ(0U, byteArray2.append(byteArray1, 4, 7));
	EXPECT_EQ(sizeof(inputByteArray), byteArray1.size());
	EXPECT_EQ(byteArray1.size() + 3, byteArray2.size());
}

TEST(CGitByteArray, Find)
{
	BYTE inputByteArray[] = { "1234\0""5789\0" };
	CGitByteArray byteArray;

	EXPECT_EQ(CGitByteArray::npos, byteArray.find(0));
	EXPECT_EQ(CGitByteArray::npos, byteArray.find('3'));
	EXPECT_EQ(CGitByteArray::npos, byteArray.find(0, 15));

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	EXPECT_EQ(4U, byteArray.find(0));
	EXPECT_EQ(4U, byteArray.find(0, 4));
	EXPECT_EQ(9U, byteArray.find(0, 5));
	EXPECT_EQ(CGitByteArray::npos, byteArray.find(0, 15));

	EXPECT_EQ(0U, byteArray.find('1'));
	EXPECT_EQ(1U, byteArray.find('2'));

	EXPECT_EQ(CGitByteArray::npos, byteArray.find('F'));

	byteArray.append((const BYTE*)"1", 1);
	EXPECT_EQ(0U, byteArray.find('1'));
	EXPECT_EQ(11U, byteArray.find('1', 2));

	EXPECT_EQ(4U, byteArray.find(0));
	EXPECT_EQ(4U, byteArray.find(0, 4));
	EXPECT_EQ(9U, byteArray.find(0, 5));
	EXPECT_EQ(9U, byteArray.find(0, 9));
	EXPECT_EQ(10U, byteArray.find(0, 10));
	EXPECT_EQ(CGitByteArray::npos, byteArray.find(0, 11));
	EXPECT_EQ(CGitByteArray::npos, byteArray.find(0, 15));
}

TEST(CGitByteArray, RevertFind)
{
	BYTE inputByteArray[] = { "1234\0""5789\0" };
	CGitByteArray byteArray;

	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind(0));
	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind('3'));

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	EXPECT_EQ(10U, byteArray.RevertFind(0));
	EXPECT_EQ(10U, byteArray.RevertFind(0, 10));
	EXPECT_EQ(9U, byteArray.RevertFind(0, 9));
	EXPECT_EQ(4U, byteArray.RevertFind(0, 4));
	EXPECT_EQ(4U, byteArray.RevertFind(0, 5));
	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind(0, 2));

	EXPECT_EQ(0U, byteArray.RevertFind('1'));
	EXPECT_EQ(1U, byteArray.RevertFind('2'));

	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind('F'));

	byteArray.append((const BYTE*)"1", 1);
	EXPECT_EQ(11U, byteArray.RevertFind('1'));
	EXPECT_EQ(11U, byteArray.RevertFind('1', 11));
	EXPECT_EQ(0U, byteArray.RevertFind('1', 10));
	EXPECT_EQ(0U, byteArray.RevertFind('1', 2));
	EXPECT_EQ(0U, byteArray.RevertFind('1', 1));
	EXPECT_EQ(0U, byteArray.RevertFind('1', 0));

	EXPECT_EQ(10U, byteArray.RevertFind(0));
	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind(0, 0));
	EXPECT_EQ(CGitByteArray::npos, byteArray.RevertFind(0, 2));
	EXPECT_EQ(4U, byteArray.RevertFind(0, 4));
	EXPECT_EQ(4U, byteArray.RevertFind(0, 5));
	EXPECT_EQ(9U, byteArray.RevertFind(0, 9));
	EXPECT_EQ(10U, byteArray.RevertFind(0, 10));
	EXPECT_EQ(10U, byteArray.RevertFind(0, 11));
}

TEST(CGitByteArray, FindNextString)
{
	BYTE inputByteArray[] = { "1234\0""5789\0" };
	CGitByteArray byteArray;

	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(0));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(5));

	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	EXPECT_EQ(5U, byteArray.findNextString(0));
	EXPECT_EQ(5U, byteArray.findNextString(2));
	EXPECT_EQ(5U, byteArray.findNextString(4));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(5));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(9));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(10));

	byteArray.append((const BYTE*)"0", 1);
	EXPECT_EQ(5U, byteArray.findNextString(0));
	EXPECT_EQ(5U, byteArray.findNextString(2));
	EXPECT_EQ(5U, byteArray.findNextString(4));
	EXPECT_EQ(11U, byteArray.findNextString(5));
	EXPECT_EQ(11U, byteArray.findNextString(9));
	EXPECT_EQ(11U, byteArray.findNextString(10));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(11));

	byteArray.clear();
	byteArray.append(inputByteArray, sizeof(inputByteArray));
	EXPECT_EQ(sizeof(inputByteArray), byteArray.size());

	byteArray.append((const BYTE*)"1", 1);
	EXPECT_EQ(5U, byteArray.findNextString(0));
	EXPECT_EQ(5U, byteArray.findNextString(2));
	EXPECT_EQ(5U, byteArray.findNextString(4));
	EXPECT_EQ(11U, byteArray.findNextString(5));
	EXPECT_EQ(11U, byteArray.findNextString(9));
	EXPECT_EQ(11U, byteArray.findNextString(10));
	EXPECT_EQ(CGitByteArray::npos, byteArray.findNextString(11));
}
