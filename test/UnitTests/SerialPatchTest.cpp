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
#include "SerialPatch.h"
#include "StringUtils.h"
#include "Git.h"

TEST(CSerialPatch, Parse)
{
	CStringA patch1header = "From 3a3d3d4a31f093ff351dd7de53e6904d618a82d5 Mon Sep 17 00:00:00 2001\nFrom: Sven Strickroth <email@cs-ware.de>\nDate: Wed, 24 Feb 2016 22:22:49 +0100\nSubject: [PATCH] RefBrowse: Start new process for showing log\n\n";
	CStringA patch1body = "(fixes issue #2709)\n\nSigned-off-by: Sven Strickroth <email@cs-ware.de>\n---\n src/TortoiseProc/BrowseRefsDlg.cpp | 18 +++++++++---------\n 1 file changed, 9 insertions(+), 9 deletions(-)\n\ndiff --git a/src/TortoiseProc/BrowseRefsDlg.cpp b/src/TortoiseProc/BrowseRefsDlg.cpp\nindex 1c9f1a7..e8159bc 100644\n--- a/src/TortoiseProc/BrowseRefsDlg.cpp\n+++ b/src/TortoiseProc/BrowseRefsDlg.cpp\n@@ -1098,23 +1098,23 @@ void CBrowseRefsDlg::ShowContextMenu(CPoint point, HTREEITEM hTreePos, VectorPSh\n 	{\r\n 	case eCmd_ViewLog:\r\n 		{\r\n-			CLogDlg dlg;\r\n-			dlg.SetRange(g_Git.FixBranchName(selectedLeafs[0]->GetRefName()));\r\n-			dlg.DoModal();\r\n-- \n2.7.0.windows.1\n\n";
	CStringA patch1 = patch1header + patch1body;

	CString tmpfile = GetTempFile();
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, CString(patch1)));

	CSerialPatch parser;
	ASSERT_EQ(0, parser.Parse(tmpfile, false));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser.m_Author);
	ASSERT_STREQ(L"Wed, 24 Feb 2016 22:22:49 +0100", parser.m_Date);
	ASSERT_STREQ(L"[PATCH] RefBrowse: Start new process for showing log", parser.m_Subject);
	ASSERT_STREQ(patch1, parser.m_Body);
	ASSERT_TRUE(parser.m_strBody.IsEmpty());
	CSerialPatch parser2;
	ASSERT_EQ(0, parser2.Parse(tmpfile, true));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser2.m_Author);
	ASSERT_STREQ(L"Wed, 24 Feb 2016 22:22:49 +0100", parser2.m_Date);
	ASSERT_STREQ(L"[PATCH] RefBrowse: Start new process for showing log", parser2.m_Subject);
	ASSERT_STREQ(patch1, parser2.m_Body);
	ASSERT_STREQ(CString(patch1body), parser2.m_strBody);

	// long subject line
	CStringA patch2longheader = "From c445609da424a6e6229c469e01ce3df5ef099ddd Mon Sep 17 00:00:00 2001\nFrom: Sven Strickroth <email@cs-ware.de>\nDate: Sun, 27 Dec 2015 15:49:34 +0100\nSubject: [PATCH 2/3] Remove dynamic linking using GetProcAddress() for APIs\n that are available on Vista now that XP support is no longer needed\n\n";
	CStringA patch2body = patch1body;
	CStringA patch2 = patch2longheader + patch2body;
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, CString(patch2)));
	CSerialPatch parser3;
	ASSERT_EQ(0, parser3.Parse(tmpfile, false));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser3.m_Author);
	ASSERT_STREQ(L"Sun, 27 Dec 2015 15:49:34 +0100", parser3.m_Date);
	ASSERT_STREQ(L"[PATCH 2/3] Remove dynamic linking using GetProcAddress() for APIs that are available on Vista now that XP support is no longer needed", parser3.m_Subject);
	ASSERT_STREQ(patch2, parser3.m_Body);
	ASSERT_TRUE(parser3.m_strBody.IsEmpty());
	CSerialPatch parser4;
	ASSERT_EQ(0, parser4.Parse(tmpfile, true));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser4.m_Author);
	ASSERT_STREQ(L"Sun, 27 Dec 2015 15:49:34 +0100", parser4.m_Date);
	ASSERT_STREQ(L"[PATCH 2/3] Remove dynamic linking using GetProcAddress() for APIs that are available on Vista now that XP support is no longer needed", parser4.m_Subject);
	ASSERT_STREQ(patch2, parser4.m_Body);
	ASSERT_STREQ(CString(patch2body), parser4.m_strBody);

	// utf8 content
	CStringA patch3header = "From 8bf757a62dab483e738fb0ea20d2240c6d554462 Mon Sep 17 00:00:00 2001\nFrom: Sven Strickroth <email@cs-ware.de>\nDate: Thu, 25 Feb 2016 04:07:55 +0100\nSubject: [PATCH] =?UTF-8?q?=C3=B6d=C3=B6=C3=A4=C3=BC?=\nMIME-Version: 1.0\nContent-Type: text/plain; charset=UTF-8\nContent-Transfer-Encoding: 8bit\n\n";
	CStringA patch3body = patch1body;
	CStringA patch3 = patch3header + patch3body;
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, CString(patch3)));
	CSerialPatch parser5;
	ASSERT_EQ(0, parser5.Parse(tmpfile, false));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser5.m_Author);
	ASSERT_STREQ(L"Thu, 25 Feb 2016 04:07:55 +0100", parser5.m_Date);
	ASSERT_STREQ(L"[PATCH] =?UTF-8?q?=C3=B6d=C3=B6=C3=A4=C3=BC?=", parser5.m_Subject);
	ASSERT_STREQ(patch3, parser5.m_Body);
	ASSERT_TRUE(parser5.m_strBody.IsEmpty());
	CSerialPatch parser6;
	ASSERT_EQ(0, parser6.Parse(tmpfile, true));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser6.m_Author);
	ASSERT_STREQ(L"Thu, 25 Feb 2016 04:07:55 +0100", parser6.m_Date);
	ASSERT_STREQ(L"[PATCH] =?UTF-8?q?=C3=B6d=C3=B6=C3=A4=C3=BC?=", parser6.m_Subject);
	ASSERT_STREQ(patch3, parser6.m_Body);
	ASSERT_STREQ(CString(patch3body), parser6.m_strBody);

	// CRLF in headers
	patch1header.Replace("\n", "\r\n");
	patch1 = patch1header + patch1body;
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, CString(patch1)));
	CSerialPatch parser7;
	ASSERT_EQ(0, parser7.Parse(tmpfile, true));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser7.m_Author);
	ASSERT_STREQ(L"Wed, 24 Feb 2016 22:22:49 +0100", parser7.m_Date);
	ASSERT_STREQ(L"[PATCH] RefBrowse: Start new process for showing log", parser7.m_Subject);
	ASSERT_STREQ(patch1, parser7.m_Body);
	ASSERT_STREQ(CString(patch1body), parser7.m_strBody);

	// different order and additional headers
	CStringA patch4header = "From c445609da424a6e6229c469e01ce3df5ef099ddd Mon Sep 17 00:00:00 2001\nContent-Type: text/plain\nSubject: [PATCH 2/3] Remove dynamic linking using GetProcAddress()\n\tfor APIs\nSomething: else\nDate: Sun, 27 Dec 2015 15:49:34 +0100\nFrom: Sven Strickroth <email@cs-ware.de>\n\n";
	CStringA patch4body = patch1body;
	CStringA patch4 = patch4header + patch4body;
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, CString(patch4)));
	CSerialPatch parser8;
	ASSERT_EQ(0, parser8.Parse(tmpfile, false));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser8.m_Author);
	ASSERT_STREQ(L"Sun, 27 Dec 2015 15:49:34 +0100", parser8.m_Date);
	ASSERT_STREQ(L"[PATCH 2/3] Remove dynamic linking using GetProcAddress()\tfor APIs", parser8.m_Subject);
	ASSERT_STREQ(patch4, parser8.m_Body);
	ASSERT_TRUE(parser8.m_strBody.IsEmpty());
	CSerialPatch parser9;
	ASSERT_EQ(0, parser9.Parse(tmpfile, true));
	ASSERT_STREQ(L"Sven Strickroth <email@cs-ware.de>", parser9.m_Author);
	ASSERT_STREQ(L"Sun, 27 Dec 2015 15:49:34 +0100", parser9.m_Date);
	ASSERT_STREQ(L"[PATCH 2/3] Remove dynamic linking using GetProcAddress()\tfor APIs", parser9.m_Subject);
	ASSERT_STREQ(patch4, parser9.m_Body);
	ASSERT_STREQ(CString(patch4body), parser9.m_strBody);
}
