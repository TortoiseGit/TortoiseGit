// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022 - TortoiseGit

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
#include "AppUtils.h"

TEST(CAppUtils, FindWarningsErrors)
{
	{
		std::vector<CHARRANGE> rangeErrors;
		std::vector<CHARRANGE> rangeWarnings;
		EXPECT_FALSE(CAppUtils::FindWarningsErrors(L"", rangeErrors, rangeWarnings));
	}

	{
		std::vector<CHARRANGE> rangeErrors;
		std::vector<CHARRANGE> rangeWarnings;
		EXPECT_TRUE(CAppUtils::FindWarningsErrors(L"warning: something", rangeErrors, rangeWarnings));
		ASSERT_EQ(0u, rangeErrors.size());
		ASSERT_EQ(1u, rangeWarnings.size());

		EXPECT_EQ(0, rangeWarnings[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"warning: ")), rangeWarnings[0].cpMax);
	}

	{
		std::vector<CHARRANGE> rangeErrors;
		std::vector<CHARRANGE> rangeWarnings;
		EXPECT_TRUE(CAppUtils::FindWarningsErrors(L"\nwarning: something", rangeErrors, rangeWarnings));
		ASSERT_EQ(0u, rangeErrors.size());
		ASSERT_EQ(1u, rangeWarnings.size());

		EXPECT_EQ(1, rangeWarnings[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"warning: ") + 1), rangeWarnings[0].cpMax);
	}

	{
		std::vector<CHARRANGE> rangeErrors;
		std::vector<CHARRANGE> rangeWarnings;
		EXPECT_TRUE(CAppUtils::FindWarningsErrors(L"\nwarning: something\n error: fjfjf\nerror: \n", rangeErrors, rangeWarnings));
		ASSERT_EQ(1u, rangeErrors.size());
		ASSERT_EQ(1u, rangeWarnings.size());

		EXPECT_EQ(1, rangeWarnings[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"warning: ") + 1), rangeWarnings[0].cpMax);

		EXPECT_EQ(34, rangeErrors[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"error: ") + 34), rangeErrors[0].cpMax);
	}

	{
		std::vector<CHARRANGE> rangeErrors;
		std::vector<CHARRANGE> rangeWarnings;
		EXPECT_TRUE(CAppUtils::FindWarningsErrors(L"\n\nfatal: something\nwarning: ", rangeErrors, rangeWarnings));
		ASSERT_EQ(1u, rangeErrors.size());
		ASSERT_EQ(1u, rangeWarnings.size());

		EXPECT_EQ(19, rangeWarnings[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"warning: ") + 19), rangeWarnings[0].cpMax);

		EXPECT_EQ(2, rangeErrors[0].cpMin);
		EXPECT_EQ(static_cast<int>(wcslen(L"fatal: ") + 2), rangeErrors[0].cpMax);
	}
}

TEST(CAppUtils, FindURLMatches)
{
	{
		auto ranges = CAppUtils::FindURLMatches(L"");
		EXPECT_TRUE(ranges.empty());
	}

	{
		auto ranges = CAppUtils::FindURLMatches(L"https://tortoisegit.org");
		ASSERT_EQ(1u, ranges.size());

		EXPECT_EQ(0, ranges[0].cpMin);
		EXPECT_EQ(23, ranges[0].cpMax);
	}

	{
		CString text = L"here https://user:pw@tortoisegit.org/~user/file_name-123.html?param=val+ue&another=val%20ue2#anchor more http:// text mailto:local_part@sub-domain.example.com text mail@example.com separator text file://c:some/path text";
		auto ranges = CAppUtils::FindURLMatches(text);
		ASSERT_EQ(4u, ranges.size());

		EXPECT_EQ(5, ranges[0].cpMin);
		EXPECT_EQ(99, ranges[0].cpMax);
		EXPECT_STREQ(L"https://user:pw@tortoisegit.org/~user/file_name-123.html?param=val+ue&another=val%20ue2#anchor", text.Mid(ranges[0].cpMin, ranges[0].cpMax - ranges[0].cpMin));

		EXPECT_EQ(118, ranges[1].cpMin);
		EXPECT_EQ(158, ranges[1].cpMax);
		EXPECT_STREQ(L"mailto:local_part@sub-domain.example.com", text.Mid(ranges[1].cpMin, ranges[1].cpMax - ranges[1].cpMin));

		EXPECT_EQ(164, ranges[2].cpMin);
		EXPECT_EQ(180, ranges[2].cpMax);
		EXPECT_STREQ(L"mail@example.com", text.Mid(ranges[2].cpMin, ranges[2].cpMax - ranges[2].cpMin));

		EXPECT_EQ(196, ranges[3].cpMin);
		EXPECT_EQ(214, ranges[3].cpMax);
		EXPECT_STREQ(L"file://c:some/path", text.Mid(ranges[3].cpMin, ranges[3].cpMax - ranges[3].cpMin));
	}

	{
		CString text = L"here <https://tortoisegit.org?param=val ue> text http://tortoisegit.org? text <http://tortoisegit.org?> text https://example.com/;sesionid=747re7fucbd text ftp://example.com/some!string text \\\\unc\\path\\somewhere text";
		auto ranges = CAppUtils::FindURLMatches(text);
		ASSERT_EQ(5u, ranges.size());

		EXPECT_EQ(6, ranges[0].cpMin);
		EXPECT_EQ(42, ranges[0].cpMax);
		EXPECT_STREQ(L"https://tortoisegit.org?param=val ue", text.Mid(ranges[0].cpMin, ranges[0].cpMax - ranges[0].cpMin));

		EXPECT_EQ(49, ranges[1].cpMin);
		EXPECT_EQ(71, ranges[1].cpMax);
		EXPECT_STREQ(L"http://tortoisegit.org", text.Mid(ranges[1].cpMin, ranges[1].cpMax - ranges[1].cpMin));

		EXPECT_EQ(79, ranges[2].cpMin);
		EXPECT_EQ(102, ranges[2].cpMax);
		EXPECT_STREQ(L"http://tortoisegit.org?", text.Mid(ranges[2].cpMin, ranges[2].cpMax - ranges[2].cpMin));

		EXPECT_EQ(109, ranges[3].cpMin);
		EXPECT_EQ(150, ranges[3].cpMax);
		EXPECT_STREQ(L"https://example.com/;sesionid=747re7fucbd", text.Mid(ranges[3].cpMin, ranges[3].cpMax - ranges[3].cpMin));

		EXPECT_EQ(156, ranges[4].cpMin);
		EXPECT_EQ(185, ranges[4].cpMax);
		EXPECT_STREQ(L"ftp://example.com/some!string", text.Mid(ranges[4].cpMin, ranges[4].cpMax - ranges[4].cpMin));
	}
}
