// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
#include "WindowsCredentialsStore.h"

#define CREDENDIALSTORETESTENTRY L"TGitUnitTest-TestEntry"

TEST(WindowsCredentialsStore, GetSetOverrideDelete)
{
	EXPECT_EQ(-1, CWindowsCredentialsStore::DeleteCredential(CREDENDIALSTORETESTENTRY));
	CString username, password;
	EXPECT_EQ(-1, CWindowsCredentialsStore::GetCredential(CREDENDIALSTORETESTENTRY, username, password));
	username = L"someusername";
	password = L"somepassword";
	EXPECT_EQ(0, CWindowsCredentialsStore::SaveCredential(CREDENDIALSTORETESTENTRY, username, password));
	username.Empty();
	password.Empty();
	EXPECT_EQ(0, CWindowsCredentialsStore::GetCredential(CREDENDIALSTORETESTENTRY, username, password));
	EXPECT_STREQ(L"someusername", username);
	EXPECT_STREQ(L"somepassword", password);
	username = L"some-other-username";
	password = L"some-other-passwordä";
	EXPECT_EQ(0, CWindowsCredentialsStore::SaveCredential(CREDENDIALSTORETESTENTRY, username, password));
	username.Empty();
	password.Empty();
	EXPECT_EQ(0, CWindowsCredentialsStore::GetCredential(CREDENDIALSTORETESTENTRY, username, password));
	EXPECT_STREQ(L"some-other-username", username);
	EXPECT_STREQ(L"some-other-passwordä", password);
	EXPECT_EQ(0, CWindowsCredentialsStore::DeleteCredential(CREDENDIALSTORETESTENTRY));
}
