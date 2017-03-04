// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2017 - TortoiseGit
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
#include "StringUtils.h"

TEST(CStringUtils, WordWrap)
{
	CString longline = L"this is a test of how a string can be splitted into several lines";
	CString splittedline = CStringUtils::WordWrap(longline, 10, true, false, 4);
	EXPECT_STREQ(L"this is a\n test of\n how a\n string\n can be\n splitted\n into\n several\n lines", splittedline);
	longline = L"c:\\this_is_a_very_long\\path_on_windows and of course some other words added to make the line longer";
	splittedline = CStringUtils::WordWrap(longline, 10, true, false, 4);
	EXPECT_STREQ(L"...\\pat...\n and of\n course\n some\n other\n words\n added to\n make the\n line\n longer", splittedline);
	longline = L"Forced failure in https://myserver.com/a_long_url_to_split PROPFIND error";
	splittedline = CStringUtils::WordWrap(longline, 20, true, false, 4);
	EXPECT_STREQ(L"Forced failure in\n https://myserver.com/a_long_url_to_split\n PROPFIND error", splittedline);
	longline = L"Forced\nfailure in https://myserver.com/a_long_url_to_split PROPFIND\nerror";
	splittedline = CStringUtils::WordWrap(longline, 40, true, false, 4);
	EXPECT_STREQ(L"Forced\nfailure in\n https://myserver.com/a_long_url_to_split\n PROPFIND\nerror", splittedline);
	longline = L"Failed to add file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO2.java";
	splittedline = CStringUtils::WordWrap(longline, 80, true, false, 4);
	EXPECT_STREQ(L"Failed to add\n file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthing\n", splittedline);
}

TEST(CStringUtils, LinesWrap)
{
	CString longline = L"this is a test of how a string can be splitted into several lines";
	CString splittedline = CStringUtils::LinesWrap(longline, 10, true);
	EXPECT_STREQ(L"this is a\n test of\n how a\n string\n can be\n splitted\n into\n several\n lines", splittedline);
	longline = L"c:\\this_is_a_very_long\\path_on_windows and of course some other words added to make the line longer";
	splittedline = CStringUtils::LinesWrap(longline, 10);
	EXPECT_STREQ(L"c:\\this_is_a_very_long\\path_on_windows\n and of\n course\n some\n other\n words\n added to\n make the\n line\n longer", splittedline);
	longline = L"Forced failure in https://myserver.com/a_long_url_to_split PROPFIND error";
	splittedline = CStringUtils::LinesWrap(longline, 20, true);
	EXPECT_STREQ(L"Forced failure in\n https://myserver.com/a_long_url_to_split\n PROPFIND error", splittedline);
	longline = L"Forced\nfailure in https://myserver.com/a_long_url_to_split PROPFIND\nerror";
	splittedline = CStringUtils::LinesWrap(longline, 40);
	EXPECT_STREQ(L"Forced\nfailure in\n https://myserver.com/a_long_url_to_split\n PROPFIND\nerror", splittedline);
	longline = L"Failed to add file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO2.java";
	splittedline = CStringUtils::LinesWrap(longline);
	EXPECT_STREQ(L"Failed to add file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\n\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO2.java", splittedline);
	longline = L"The commit comment is not properly formatted.\nFormat:\n  Field 1 : Field 2 : Field 3\nWhere:\nField 1 - Team Name|Triage|Merge|Goal\nField 2 - V1 Backlog Item ID|Triage Number|SVNBranch|Goal Name\nField 3 - Description of change\nExamples:\n\nTeam Gamma : B-12345 : Changed some code\n  Triage : 123 : Fixed production release bug\n  Merge : sprint0812 : Merged sprint0812 into prod\n  Goal : Implement Pre-Commit Hook : Commit message hook impl";
	splittedline = CStringUtils::LinesWrap(longline, 80);
	EXPECT_STREQ(L"The commit comment is not properly formatted.\nFormat:\n  Field 1 : Field 2 : Field 3\nWhere:\nField 1 - Team Name|Triage|Merge|Goal\nField 2 - V1 Backlog Item ID|Triage Number|SVNBranch|Goal Name\nField 3 - Description of change\nExamples:\n\nTeam Gamma : B-12345 : Changed some code\n  Triage : 123 : Fixed production release bug\n  Merge : sprint0812 : Merged sprint0812 into prod\n  Goal : Implement Pre-Commit Hook : Commit message hook impl", splittedline);
}

TEST(CStringUtils, RemoveAccelerators)
{
	CString empty;
	CStringUtils::RemoveAccelerators(empty);
	EXPECT_STREQ(L"", empty);

	CString text1 = L"&Accellerator";
	CStringUtils::RemoveAccelerators(text1);
	EXPECT_STREQ(L"Accellerator", text1);

	CString text1a = L"Ac&cellerator";
	CStringUtils::RemoveAccelerators(text1a);
	EXPECT_STREQ(L"Accellerator", text1a);

	CString text2 = L"Accellerator&";
	CStringUtils::RemoveAccelerators(text2);
	EXPECT_STREQ(L"Accellerator", text2);

	CString text3 = L"Some & text";
	CStringUtils::RemoveAccelerators(text3);
	EXPECT_STREQ(L"Some & text", text3);

	CString text4 = L"&&Accellerator";
	CStringUtils::RemoveAccelerators(text4);
	EXPECT_STREQ(L"&Accellerator", text4);

	CString text5 = L"Acce&&&llerator";
	CStringUtils::RemoveAccelerators(text5);
	EXPECT_STREQ(L"Acce&llerator", text5);

	CString text6 = L"Some & te&xt";
	CStringUtils::RemoveAccelerators(text6);
	EXPECT_STREQ(L"Some & text", text6);
}

TEST(CStringUtils, ParseEmailAddress)
{
	CString mail, name;
	CStringUtils::ParseEmailAddress(L"", mail, &name);
	EXPECT_STREQ(L"", mail);
	EXPECT_STREQ(L"", name);

	CStringUtils::ParseEmailAddress(L" ", mail, &name);
	EXPECT_STREQ(L"", mail);
	EXPECT_STREQ(L"", name);

	mail.Empty();
	CStringUtils::ParseEmailAddress(L"test@example.com ", mail);
	EXPECT_STREQ(L"test@example.com", mail);

	mail.Empty();
	CStringUtils::ParseEmailAddress(L" test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", mail);

	mail.Empty();
	CStringUtils::ParseEmailAddress(L"test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", mail);

	mail.Empty();
	CStringUtils::ParseEmailAddress(L"John Doe <johndoe>", mail);
	EXPECT_STREQ(L"johndoe", mail);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"test@example.com", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"<test@example.com>", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"John Doe <test@example.com>", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"John Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"John Doe\" <test@example.com>", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"John Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"<test@example.com", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"test@example.com>", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"test@example.com", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"John Doe <johndoe>", mail, &name);
	EXPECT_STREQ(L"johndoe", mail);
	EXPECT_STREQ(L"John Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"john.doe@example.com (John Doe)", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"John Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"John (zzz) Doe <john.doe@example.com> (Comment)", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"John (zzz) Doe (Comment)", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"john.doe@example.com >> John Doe\" <john.doe@example.com>", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"john.doe@example.com >> John Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"John<something> Doe\" <john.doe@example.com>", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"John<something> Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"John<something@example.com> Doe\" <john.doe@example.com>", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"John<something@example.com> Doe", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"something@example.com\" <john.doe@example.com>", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"something@example.com", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"\"John D\\\"o\\\"e\" <john.doe@example.com>", mail, &name);
	EXPECT_STREQ(L"john.doe@example.com", mail);
	EXPECT_STREQ(L"John D\"o\"e", name);

	mail.Empty();
	name.Empty();
	CStringUtils::ParseEmailAddress(L"<test@example.com> \"John Doe\"", mail, &name);
	EXPECT_STREQ(L"test@example.com", mail);
	EXPECT_STREQ(L"John Doe", name);
}

TEST(CStringUtils, IsPlainReadableASCII)
{
	EXPECT_TRUE(CStringUtils::IsPlainReadableASCII(L""));
	EXPECT_TRUE(CStringUtils::IsPlainReadableASCII(L" 1234,#+*.:-&!\\<>|\"$%&/()=567890abcdefSBDUDB[](!}~"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"\n"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"\u2302"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"é"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"\u2550"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"ä"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"€"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"\u570B"));
	EXPECT_FALSE(CStringUtils::IsPlainReadableASCII(L"\u7ACB"));
}

TEST(CStringUtils, StartsWith)
{
	EXPECT_TRUE(CStringUtils::StartsWith(L"", L""));
	EXPECT_FALSE(CStringUtils::StartsWith(L"", L"sometest"));

	CString heystack = L"sometest";
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, L"sometest"));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, L""));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, L"sometes"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, L"sometEs"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, L"someteste"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, L"sometess"));

	CString empty;
	CString sometest = L"sometest";
	CString sometes = L"sometes";
	CString sometEs = L"sometEs";
	CString someteste = L"someteste";
	CString sometess = L"sometess";
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, sometest));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, empty));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, sometes));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, sometEs));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, someteste));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, sometess));
	EXPECT_FALSE(CStringUtils::StartsWith(empty, sometess));
}

TEST(CStringUtils, StartsWithI)
{
	EXPECT_TRUE(CStringUtils::StartsWithI(L"", L""));
	EXPECT_FALSE(CStringUtils::StartsWithI(L"", L"sometest"));

	CString heystack = L"someTest";
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, L"sometest"));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, L"someTest"));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, L"sOmetest"));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, L""));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, L"sometEs"));
	EXPECT_FALSE(CStringUtils::StartsWithI(heystack, L"sometesTe"));
	EXPECT_FALSE(CStringUtils::StartsWithI(heystack, L"someteSs"));

	CString empty;
	CString sometest = L"someteSt";
	CString sometes = L"soMetes";
	CString someteste = L"sOmeteste";
	CString sometess = L"sometesS";
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, sometest));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, empty));
	EXPECT_TRUE(CStringUtils::StartsWithI(heystack, sometes));
	EXPECT_FALSE(CStringUtils::StartsWithI(heystack, someteste));
	EXPECT_FALSE(CStringUtils::StartsWithI(heystack, sometess));
	EXPECT_FALSE(CStringUtils::StartsWithI(empty, sometess));
}

TEST(CStringUtils, StartsWithA)
{
	EXPECT_TRUE(CStringUtils::StartsWith("", ""));
	EXPECT_FALSE(CStringUtils::StartsWith("", "sometest"));

	CStringA heystack = L"sometest";
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, "sometest"));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, ""));
	EXPECT_TRUE(CStringUtils::StartsWith(heystack, "sometes"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, "someTe"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, "someteste"));
	EXPECT_FALSE(CStringUtils::StartsWith(heystack, "sometess"));
}

TEST(CStringUtils, EndsWith)
{
	EXPECT_TRUE(CStringUtils::EndsWith(L"", L""));
	EXPECT_FALSE(CStringUtils::EndsWith(L"", L"sometest"));

	CString heystack = L"sometest";
	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L"sometest"));
	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L"test"));
	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L"st"));
	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L"t"));
	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L""));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L"teSt"));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L"esometest"));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L"someteste"));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L"text"));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L"xt"));

	EXPECT_TRUE(CStringUtils::EndsWith(heystack, L't'));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L'T'));
	EXPECT_FALSE(CStringUtils::EndsWith(heystack, L'x'));
}

TEST(CStringUtils, EndsWithI)
{
	EXPECT_TRUE(CStringUtils::EndsWithI(L"", L""));
	EXPECT_FALSE(CStringUtils::EndsWithI(L"", L"sometest"));

	CString heystack = L"sometest";
	EXPECT_TRUE(CStringUtils::EndsWithI(heystack, L"someteSt"));
	EXPECT_TRUE(CStringUtils::EndsWithI(heystack, L"tEst"));
	EXPECT_TRUE(CStringUtils::EndsWithI(heystack, L"sT"));
	EXPECT_TRUE(CStringUtils::EndsWithI(heystack, L"T"));
	EXPECT_TRUE(CStringUtils::EndsWithI(heystack, L""));
	EXPECT_FALSE(CStringUtils::EndsWithI(heystack, L"esometEst"));
	EXPECT_FALSE(CStringUtils::EndsWithI(heystack, L"someteSte"));
	EXPECT_FALSE(CStringUtils::EndsWithI(heystack, L"text"));
	EXPECT_FALSE(CStringUtils::EndsWithI(heystack, L"xt"));
}
