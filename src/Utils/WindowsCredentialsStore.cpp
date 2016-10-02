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
#include <wincred.h>

#pragma comment(lib, "Advapi32.lib")

int CWindowsCredentialsStore::GetCredential(const CString& entryName, CString& username, CString& password)
{
	PCREDENTIAL cred = nullptr;
	if (CredRead(entryName, 1, 0, &cred) != TRUE || !cred)
		return -1;

	username = cred->UserName;
	password = CString((TCHAR*)cred->CredentialBlob, cred->CredentialBlobSize / sizeof(TCHAR));
	return 0;
}

int CWindowsCredentialsStore::SaveCredential(const CString& entryName, const CString& username, const CString& password)
{
	CREDENTIAL cred = { 0 };
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = const_cast<LPTSTR>(static_cast<LPCTSTR>(entryName));
	cred.UserName = const_cast<LPTSTR>(static_cast<LPCTSTR>(username));
	cred.CredentialBlob = (LPBYTE)static_cast<LPCTSTR>(password);
	cred.CredentialBlobSize = password.GetLength() * sizeof(TCHAR);
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	return CredWrite(&cred, 0) == TRUE ? 0 : -1;
}

int CWindowsCredentialsStore::DeleteCredential(const CString& entryName)
{
	return CredDelete(entryName, CRED_TYPE_GENERIC, 0) == TRUE ? 0 : -1;
}
