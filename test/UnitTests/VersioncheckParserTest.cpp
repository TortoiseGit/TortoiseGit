// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2018 - TortoiseGit

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
#include "VersioncheckParser.h"
#include "StringUtils.h"
#include "Git.h"

TEST(CVersioncheckParser, Invalid)
{
	CString tmpfile = GetTempFile();
	CStringUtils::WriteStringToTextFile(tmpfile, L"");

	CVersioncheckParser parser;
	CString err;
	EXPECT_FALSE(parser.Load(tmpfile, err));

	CStringUtils::WriteStringToTextFile(tmpfile, L"something\nversion\nblabla");
	EXPECT_FALSE(parser.Load(tmpfile, err));

	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\nversion=def");
	EXPECT_FALSE(parser.Load(tmpfile, err));

	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\nversion=d4ef");
	EXPECT_FALSE(parser.Load(tmpfile, err));
}

TEST(CVersioncheckParser, ParseTestMinimal)
{
	CString tmpfile = GetTempFile();
	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\r\nversion=2.3.4.0\r\n");

	CVersioncheckParser parser;
	CString err;
	EXPECT_TRUE(parser.Load(tmpfile, err));

	auto version = parser.GetTortoiseGitVersion();
	EXPECT_STREQ(version.version, version.version_for_filename);
	EXPECT_EQ(2U, version.major);
	EXPECT_EQ(3U, version.minor);
	EXPECT_EQ(4U, version.micro);
	EXPECT_EQ(0U, version.build);

	EXPECT_STREQ(L"", parser.GetTortoiseGitInfoText());
	EXPECT_STREQ(L"", parser.GetTortoiseGitInfoTextURL());

	EXPECT_STREQ(L"http://updater.download.tortoisegit.org/tgit/2.3.4.0/", parser.GetTortoiseGitBaseURL());
	EXPECT_STREQ(L"https://tortoisegit.org/issue/%BUGID%", parser.GetTortoiseGitIssuesURL());
	EXPECT_STREQ(L"https://versioncheck.tortoisegit.org/changelog.txt", parser.GetTortoiseGitChangelogURL());

	EXPECT_FALSE(parser.GetTortoiseGitIsHotfix());

#if WIN64
	EXPECT_STREQ(L"TortoiseGit-2.3.4.0-64bit.msi", parser.GetTortoiseGitMainfilename());
#else
	EXPECT_STREQ(L"TortoiseGit-2.3.4.0-32bit.msi", parser.GetTortoiseGitMainfilename());
#endif

	auto langs = parser.GetTortoiseGitLanguagePacks();
	EXPECT_EQ(0U, langs.size());
}

TEST(CVersioncheckParser, ParseTestRelease)
{
	CString tmpfile = GetTempFile();
	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\nversion=2.3.4.5\ninfotext=\"Hallo\\ntest\"\ninfotexturl=someurl\nissuesurl=https://tortoisegit.org/issue/%BUGID%\r\nbaseurl=https://updater.download.tortoisegit.org/tgit/2.3.0.0/\nlangs=\"1031;de\"\nlangs=\"1046;pt_BR\"\nlangs=\"2074;sr-latin\"\nlangs=\"1028;zh_TW\"");

	CVersioncheckParser parser;
	CString err;
	EXPECT_TRUE(parser.Load(tmpfile, err));

	auto version = parser.GetTortoiseGitVersion();
	EXPECT_STREQ(version.version, version.version_for_filename);
	EXPECT_EQ(2U, version.major);
	EXPECT_EQ(3U, version.minor);
	EXPECT_EQ(4U, version.micro);
	EXPECT_EQ(5U, version.build);

	EXPECT_STREQ(L"Hallo\ntest", parser.GetTortoiseGitInfoText());
	EXPECT_STREQ(L"someurl", parser.GetTortoiseGitInfoTextURL());

	EXPECT_STREQ(L"https://updater.download.tortoisegit.org/tgit/2.3.0.0/", parser.GetTortoiseGitBaseURL());
	EXPECT_STREQ(L"https://tortoisegit.org/issue/%BUGID%", parser.GetTortoiseGitIssuesURL());
	EXPECT_STREQ(L"https://versioncheck.tortoisegit.org/changelog.txt", parser.GetTortoiseGitChangelogURL());

	EXPECT_FALSE(parser.GetTortoiseGitIsHotfix());

#if WIN64
	EXPECT_STREQ(L"TortoiseGit-2.3.4.5-64bit.msi", parser.GetTortoiseGitMainfilename());
#else
	EXPECT_STREQ(L"TortoiseGit-2.3.4.5-32bit.msi", parser.GetTortoiseGitMainfilename());
#endif

	auto langs = parser.GetTortoiseGitLanguagePacks();
	ASSERT_EQ(4U, langs.size());

	EXPECT_EQ(1031U, langs[0].m_LocaleID);
	EXPECT_EQ(1046U, langs[1].m_LocaleID);
	EXPECT_EQ(2074U, langs[2].m_LocaleID);
	EXPECT_EQ(1028U, langs[3].m_LocaleID);

	EXPECT_STREQ(L"de", langs[0].m_LangCode);
	EXPECT_STREQ(L"pt_BR", langs[1].m_LangCode);
	EXPECT_STREQ(L"sr-latin", langs[2].m_LangCode);
	EXPECT_STREQ(L"zh_TW", langs[3].m_LangCode);

#if WIN64
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-64bit-de.msi", langs[0].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-64bit-pt_BR.msi", langs[1].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-64bit-sr-latin.msi", langs[2].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-64bit-zh_TW.msi", langs[3].m_filename);
#else
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-32bit-de.msi", langs[0].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-32bit-pt_BR.msi", langs[1].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-32bit-sr-latin.msi", langs[2].m_filename);
	EXPECT_STREQ(L"TortoiseGit-LanguagePack-2.3.4.5-32bit-zh_TW.msi", langs[3].m_filename);
#endif
}

TEST(CVersioncheckParser, ParseTestPreview)
{
	CString tmpfile = GetTempFile();
	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\nversion=1.8.14.2\nversionstring=preview-1.8.14.2-20150705-92b29f6\nchangelogurl=https://versioncheck.tortoisegit.org/changelog-preview.txt\nbaseurl=http://updater.download.tortoisegit.org/tgit/previews/");

	CVersioncheckParser parser;
	CString err;
	EXPECT_TRUE(parser.Load(tmpfile, err));

	auto version = parser.GetTortoiseGitVersion();
	EXPECT_STREQ(L"preview-1.8.14.2-20150705-92b29f6", version.version_for_filename);
	EXPECT_EQ(1U, version.major);
	EXPECT_EQ(8U, version.minor);
	EXPECT_EQ(14U, version.micro);
	EXPECT_EQ(2U, version.build);

	EXPECT_STREQ(L"http://updater.download.tortoisegit.org/tgit/previews/", parser.GetTortoiseGitBaseURL());
	EXPECT_STREQ(L"https://tortoisegit.org/issue/%BUGID%", parser.GetTortoiseGitIssuesURL());
	EXPECT_STREQ(L"https://versioncheck.tortoisegit.org/changelog-preview.txt", parser.GetTortoiseGitChangelogURL());

	EXPECT_FALSE(parser.GetTortoiseGitIsHotfix());

#if WIN64
	EXPECT_STREQ(L"TortoiseGit-preview-1.8.14.2-20150705-92b29f6-64bit.msi", parser.GetTortoiseGitMainfilename());
#else
	EXPECT_STREQ(L"TortoiseGit-preview-1.8.14.2-20150705-92b29f6-32bit.msi", parser.GetTortoiseGitMainfilename());
#endif

	auto langs = parser.GetTortoiseGitLanguagePacks();
	EXPECT_EQ(0U, langs.size());
}

TEST(CVersioncheckParser, ParseTestHotfix)
{
	CString tmpfile = GetTempFile();
	CStringUtils::WriteStringToTextFile(tmpfile, L"[TortoiseGit]\nversion=1.8.14.2\nversionstring=hfüx\nmainfilename=TortoiseGit-%2!s!bit-%1!s!-hotfix.exe\nhotfix=true\nissuesurl=");

	CVersioncheckParser parser;
	CString err;
	EXPECT_TRUE(parser.Load(tmpfile, err));

	auto version = parser.GetTortoiseGitVersion();
	EXPECT_STREQ(L"hfüx", version.version_for_filename);
	EXPECT_EQ(1U, version.major);
	EXPECT_EQ(8U, version.minor);
	EXPECT_EQ(14U, version.micro);
	EXPECT_EQ(2U, version.build);

	EXPECT_STREQ(L"http://updater.download.tortoisegit.org/tgit/hfüx/", parser.GetTortoiseGitBaseURL());
	EXPECT_TRUE(parser.GetTortoiseGitIsHotfix());

	EXPECT_STREQ(L"", parser.GetTortoiseGitIssuesURL());

#if WIN64
	EXPECT_STREQ(L"TortoiseGit-64bit-hfüx-hotfix.exe", parser.GetTortoiseGitMainfilename());
#else
	EXPECT_STREQ(L"TortoiseGit-32bit-hfüx-hotfix.exe", parser.GetTortoiseGitMainfilename());
#endif

	auto langs = parser.GetTortoiseGitLanguagePacks();
	EXPECT_EQ(0U, langs.size());
}
