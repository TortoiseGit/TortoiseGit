// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2020 - TortoiseGit

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
#include "GitMailmap.h"
#include "Git.h"

CGitMailmap::CGitMailmap()
{
	if (ShouldLoadMailmap())
	{
		try
		{
			CAutoLocker lock(g_Git.m_critGitDllSec);
			g_Git.CheckAndInitDll();
		}
		catch (char* msg)
		{
			CString err(msg);
			MessageBox(nullptr, L"Could not initialize libgit. Disabling Mailmap support.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
			return;
		}
		git_read_mailmap(&m_pMailmap);
	}
}

CGitMailmap::~CGitMailmap()
{
	git_free_mailmap(m_pMailmap);
}

bool CGitMailmap::ShouldLoadMailmap()
{
	return CRegDWORD(L"Software\\TortoiseGit\\UseMailmap", TRUE) == TRUE;
}

void CGitMailmap::Translate(CString& name, CString& email) const
{
	ASSERT(m_pMailmap);
	struct payload_struct
	{
		const CString* name;
		const char* authorName;
	} payload = { &name, nullptr };
	const char* author1 = nullptr;
	const char* email1 = nullptr;
	git_lookup_mailmap(m_pMailmap, &email1, &author1, CUnicodeUtils::GetUTF8(email), &payload,
					   [](void* payload) -> const char* { return reinterpret_cast<payload_struct*>(payload)->authorName = _strdup(CUnicodeUtils::GetUTF8(*reinterpret_cast<payload_struct*>(payload)->name)); });
	free((void*)payload.authorName);
	if (email1)
		email = CUnicodeUtils::GetUnicode(email1);
	if (author1)
		name = CUnicodeUtils::GetUnicode(author1);
}

static bool GetMailmapMapping(GIT_MAILMAP mailmap, const CString& email, const CString& name, bool returnEmail, CString& ret)
{
	struct payload_struct
	{
		const CString* name;
		const char* authorName;
	} payload = { &name, nullptr };
	const char* author1 = nullptr;
	const char* email1 = nullptr;
	if (git_lookup_mailmap(mailmap, &email1, &author1, CUnicodeUtils::GetUTF8(email), &payload,
						   [](void* payload) -> const char* { return reinterpret_cast<payload_struct*>(payload)->authorName = _strdup(CUnicodeUtils::GetUTF8(*reinterpret_cast<payload_struct*>(payload)->name)); }) == -1)
		return false;
	free((void*)payload.authorName);
	if (returnEmail && email1)
		ret = CUnicodeUtils::GetUnicode(email1);
	else if (returnEmail)
		return false;
	else if (author1)
		ret = CUnicodeUtils::GetUnicode(author1);
	else
		return false;
	return true;
}

const CString CGitMailmap::TranslateAuthor(const CString& name, const CString& email) const
{
	ASSERT(m_pMailmap);
	CString ret;
	if (GetMailmapMapping(m_pMailmap, email, name, false, ret))
		return ret;
	return name;
}

const CString CGitMailmap::TranslateEmail(const CString& name, const CString& email) const
{
	ASSERT(m_pMailmap);
	CString ret;
	if (GetMailmapMapping(m_pMailmap, email, name, true, ret))
		return ret;
	return email;
}
