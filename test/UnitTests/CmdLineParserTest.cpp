// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit

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
#include "CmdLineParser.h"

TEST(CCmdLineParser, Null)
{
	CCmdLineParser parser(nullptr);
	EXPECT_FALSE(parser.HasKey(L"action"));
	EXPECT_FALSE(parser.HasVal(L"action"));
	EXPECT_EQ(nullptr, parser.GetVal(L"action"));
	EXPECT_EQ(0, parser.GetLongVal(L"action"));
	EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
}

TEST(CCmdLineParser, Empty)
{
	CCmdLineParser parser(L"");
	EXPECT_FALSE(parser.HasKey(L"action"));
	EXPECT_FALSE(parser.HasVal(L"action"));
	EXPECT_EQ(nullptr, parser.GetVal(L"action"));
	EXPECT_EQ(0, parser.GetLongVal(L"action"));
	EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
}

TEST(CCmdLineParser, Text)
{
	CCmdLineParser parser(L"action");
	EXPECT_FALSE(parser.HasKey(L"action"));
	EXPECT_FALSE(parser.HasVal(L"action"));
	EXPECT_EQ(nullptr, parser.GetVal(L"action"));
	EXPECT_EQ(0, parser.GetLongVal(L"action"));
	EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
}

TEST(CCmdLineParser, SingleSimpleArgValue)
{
	CString args[] = { L"/action", L"-action" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"", parser.GetVal(L"action"));
		EXPECT_EQ(0, parser.GetLongVal(L"action"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
	}
}

TEST(CCmdLineParser, PrefixSingleSimpleArgValue)
{
	CString args[] = { L"Something /action", L"Something -action" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"", parser.GetVal(L"action"));
		EXPECT_EQ(0, parser.GetLongVal(L"action"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
	}
}

TEST(CCmdLineParser, QuotedArgs)
{
	CString args[] = { L"\"/action\"", L"\"-action\"", L"/\"action\"", L"/\"action\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_FALSE(parser.HasKey(L""));
		EXPECT_FALSE(parser.HasVal(L""));
		EXPECT_EQ(nullptr, parser.GetVal(L""));
		EXPECT_FALSE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_EQ(nullptr, parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleSimpleArgStringValue)
{
	CString args[] = { L"/action:log", L"/action log", L"-action:log", L"-action log" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_FALSE(parser.HasKey(L"log"));
		EXPECT_FALSE(parser.HasVal(L"log"));
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringSpaceValue)
{
	CString args[] = { L"/action:log 2", L"/action log 2", L"-action:log 2", L"-action log 2" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"log"));
		EXPECT_FALSE(parser.HasVal(L"log"));
		EXPECT_FALSE(parser.HasKey(L"2"));
		EXPECT_FALSE(parser.HasVal(L"2"));
	}
}

TEST(CCmdLineParser, SingleArgStringSpaceValueQuoted)
{
	CString args[] = { L"/action:\"log 2\"", L"/action \"log 2\"", L"-action:\"log 2\"", L"-action \"log 2\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log 2", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuoted)
{
	CString args[] = { L"/action:\"log\"", L"/action \"log\"", L"-action:\"log\"", L"-action \"log\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotedWithQuotes)
{
	CString args[] = { L"/action:\"lo\"\"g\"", L"/action \"lo\"\"g\"", L"-action:\"lo\"\"g\"", L"-action \"lo\"\"g\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"lo\"g", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotedWithQuotesAtEnd)
{
	CString args[] = { L"/action:\"lo\"\"\"", L"/action \"lo\"\"\"", L"-action:\"lo\"\"\"", L"-action \"lo\"\"\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"lo\"", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStartingDoubleQuote)
{
	CString args[] = { L"/action:\"\"log", L"/action \"\"log", L"-action:\"\"log", L"-action \"\"log" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuote)
{
	CString args[] = { L"/action:l\"og", L"/action l\"og", L"-action:l\"og", L"-action l\"og" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l\"og", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotes1)
{
	CString args[] = { L"/action:l\"\"og", L"/action l\"\"og", L"-action:l\"\"og", L"-action l\"\"og" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l\"\"og", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotes2)
{
	CString args[] = { L"/action:l\"og\"", L"/action l\"og\"", L"-action:l\"og\"", L"-action l\"og\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l\"og\"", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuoteAtEnd)
{
	CString args[] = { L"/action:log\"", L"/action log\"", L"-action:log\"", L"-action log\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log\"", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueMultiQuote)
{
	CString args[] = { L"/action:l\"o\"g", L"/action l\"o\"g", L"-action:l\"o\"g", L"-action l\"o\"g" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l\"o\"g", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotedWithQuote)
{
	CString args[] = { L"/action:\"l\"o\"g\"", L"/action \"l\"o\"g\"", L"-action:\"l\"o\"g\"", L"-action \"l\"o\"g\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringValueQuotedWithTwoQuotes)
{
	CString args[] = { L"/action:\"l\"\"o\"\"g\"", L"/action \"l\"\"o\"\"g\"", L"-action:\"l\"\"o\"\"g\"", L"-action \"l\"\"o\"\"g\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"l\"o\"g", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringEmptyValue)
{
	CString args[] = { L"/action:\"\"", L"/action \"\"", L"-action:\"\"", L"-action \"\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringSingleQuote)
{
	CString args[] = { L"/action:\"\"\"\"", L"/action \"\"\"\"", L"-action:\"\"\"\"", L"-action \"\"\"\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"\"", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgStringDoubleQuote)
{
	CString args[] = { L"/action:\"\"\"\"\"\"", L"/action \"\"\"\"\"\"", L"-action:\"\"\"\"\"\"", L"-action \"\"\"\"\"\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"\"\"", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgNumberValue)
{
	CString args[] = { L"/action:10", L"/action 10", L"-action:10", L"-action 10" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"10", parser.GetVal(L"action"));
		EXPECT_EQ(10, parser.GetLongVal(L"action"));
		EXPECT_EQ(10, parser.GetLongLongVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgNumberValueQuoted)
{
	CString args[] = { L"/action:\"10\"", L"/action \"10\"", L"-action:\"10\"", L"-action \"10\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"10", parser.GetVal(L"action"));
		EXPECT_EQ(10, parser.GetLongVal(L"action"));
		EXPECT_EQ(10, parser.GetLongLongVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgNumberValuePrefix)
{
	CString args[] = { L"/action:010", L"/action 010", L"-action:010", L"-action 010" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"010", parser.GetVal(L"action"));
		EXPECT_EQ(10, parser.GetLongVal(L"action"));
		EXPECT_EQ(10, parser.GetLongLongVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgQuotedArg1)
{
	CString args[] = { L"/action:\"/test\"", L"/action \"/test\"", L"-action:\"/test\"", L"-action \"/test\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"/test", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgQuotedArg2)
{
	CString args[] = { L"/action:\"-test\"", L"/action \"-test\"", L"-action:\"-test\"", L"-action \"-test\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"-test", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgBrokenQuote)
{
	CString args[] = { L"/action:\"-test", L"/action \"-test", L"-action \"-test", L"-action:\"-test" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"-test", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"test"));
	}
}

TEST(CCmdLineParser, SingleArgBrokenQuoteAtEnd)
{
	CString args[] = { L"/action:\"", L"/action \"", L"-action \"", L"-action:\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgBrokenQuoteEndDoubleQuote)
{
	CString args[] = { L"/action:\"-test\"\"", L"/action \"-test\"\"", L"-action \"-test\"\"", L"-action:\"-test\"\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"-test\"", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"test"));
	}
}

TEST(CCmdLineParser, SingleArgBrokenQuote2)
{
	CString args[] = { L"/action \"-test \"bla\"", L"/action:\"-test \"bla\"", L"-action \"-test \"bla\"", L"-action:\"-test \"bla\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"-test ", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"test"));
		EXPECT_FALSE(parser.HasKey(L"bla"));
	}
}

TEST(CCmdLineParser, SingleArgBrokenQuote3)
{
	CString args[] = { L"/action \"-test:\"bla\"", L"/action:\"-test:\"bla\"", L"-action \"-test:\"bla\"", L"-action:\"-test:\"bla\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"-test:", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"test"));
		EXPECT_FALSE(parser.HasKey(L"bla"));
	}
}

TEST(CCmdLineParser, MultiArgSimpleArgs)
{
	CString args[] = { L"/action /rev", L"/action -rev", L"-action -rev", L"-action /rev" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_EQ(0, parser.GetLongVal(L"action"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_FALSE(parser.HasVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgSimpleArgsDoubleSpace)
{
	CString args[] = { L"/action  /rev", L"/action  -rev", L"-action  -rev", L"-action  /rev" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_EQ(0, parser.GetLongVal(L"action"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_FALSE(parser.HasVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgMixedArgs1)
{
	CString args[] = { L"/action log /rev", L"/action log -rev", L"-action log -rev", L"-action log /rev" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_FALSE(parser.HasVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongVal(L"rev"));
		EXPECT_EQ(0, parser.GetLongLongVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgMixedArgs2)
{
	CString args[] = { L"/action /rev def", L"/action -rev def", L"-action -rev def", L"-action /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_FALSE(parser.HasVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceValue)
{
	CString args[] = { L"/action:log 2 /rev def", L"/action log 2 -rev def", L"-action:log 2 -rev def", L"-action log 2 /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
		EXPECT_FALSE(parser.HasKey(L"log"));
		EXPECT_FALSE(parser.HasVal(L"log"));
		EXPECT_FALSE(parser.HasKey(L"2"));
		EXPECT_FALSE(parser.HasVal(L"2"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceLeftOverQuote)
{
	CString args[] = { L"/action:log \"2 /rev def", L"/action log \"2 -rev def", L"-action:log \"2 -rev def", L"-action log \"2 /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceValueQuoted)
{
	CString args[] = { L"/action:\"log 2\" /rev def", L"/action \"log 2\" -rev def", L"-action:\"log 2\" -rev def", L"-action \"log 2\" /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log 2", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceValueQuotedWithQuote)
{
	CString args[] = { L"/action:\"log \"\"2\" /rev def", L"/action \"log \"\"2\" -rev def", L"-action:\"log \"\"2\" -rev def", L"-action \"log \"\"2\" /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log \"2", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceValueQuoted2)
{
	CString args[] = { L"/action:\" log 2 \" /rev def", L"/action \" log 2 \" -rev def", L"-action:\" log 2 \" -rev def", L"-action \" log 2 \" /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L" log 2 ", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgStringSpaceValueQuotedWithQuote2)
{
	CString args[] = { L"/action:\" log 2 \"\"\" /rev def", L"/action \" log 2 \"\"\" -rev def", L"-action:\" log 2 \"\"\" -rev def", L"-action \" log 2 \"\"\" /rev def" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L" log 2 \"", parser.GetVal(L"action"));
		EXPECT_TRUE(parser.HasKey(L"rev"));
		EXPECT_TRUE(parser.HasVal(L"rev"));
		EXPECT_STREQ(L"def", parser.GetVal(L"rev"));
	}
}

TEST(CCmdLineParser, MultiArgValueTwice)
{
	CString args[] = { L"/action:\"log\" /action:\"log2\"", L"/action \"log\" /action \"log2\"", L"-action:\"log\" -action:\"log2\"", L"-action \"log\" -action \"log2\"", L"/action:\"log\" -action:\"log2\"", L"/action \"log\" -action \"log2\"", L"-action:\"log\" /action:\"log2\"", L"-action \"log\" /action \"log2\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"action"));
		EXPECT_TRUE(parser.HasVal(L"action"));
		EXPECT_STREQ(L"log", parser.GetVal(L"action"));
	}
}

TEST(CCmdLineParser, SingleArgPath)
{
	CString args[] = { L"/path:C:\\test", L"/path C:\\test", L"-path C:\\test", L"-path C:\\test" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"path"));
		EXPECT_TRUE(parser.HasVal(L"path"));
		EXPECT_STREQ(L"C:\\test", parser.GetVal(L"path"));
	}
}

TEST(CCmdLineParser, SingleArgPathQuoted)
{
	CString args[] = { L"/path:\"C:\\test\"", L"/path \"C:\\test\"", L"-path \"C:\\test\"", L"-path \"C:\\test\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"path"));
		EXPECT_TRUE(parser.HasVal(L"path"));
		EXPECT_STREQ(L"C:\\test", parser.GetVal(L"path"));
	}
}

TEST(CCmdLineParser, SingleArgUnixPath)
{
	CString args[] = { L"/path:C:/test", L"/path C:/test", L"-path C:/test", L"-path C:/test" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"path"));
		EXPECT_TRUE(parser.HasVal(L"path"));
		EXPECT_STREQ(L"C:/test", parser.GetVal(L"path"));
		EXPECT_FALSE(parser.HasKey(L"C"));
		EXPECT_FALSE(parser.HasKey(L"C:"));
	}
}


TEST(CCmdLineParser, SingleArgUnixPathQuoted)
{
	CString args[] = { L"/path:\"C:/test\"", L"/path \"C:/test\"", L"-path \"C:/test\"", L"-path \"C:/test\"" };
	for (const CString& arg : args)
	{
		CCmdLineParser parser(arg);
		EXPECT_TRUE(parser.HasKey(L"path"));
		EXPECT_TRUE(parser.HasVal(L"path"));
		EXPECT_STREQ(L"C:/test", parser.GetVal(L"path"));
		EXPECT_FALSE(parser.HasKey(L"C"));
		EXPECT_FALSE(parser.HasKey(L"C:"));
	}
}
