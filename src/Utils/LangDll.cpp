// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017, 2019-2021, 2025 - TortoiseGit
// Copyright (C) 2003-2006, 2008, 2013-2015 - TortoiseSVN

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
#include <assert.h>
#include "LangDll.h"
#include "../version.h"
#include "I18NHelper.h"
#include "PathUtils.h"
#include "registry.h"
#include <format>

HINSTANCE CLangDll::Init(LPCWSTR appname)
{
	assert(!m_hInstance);

	CRegStdDWORD loc = CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", m_langId);
	DWORD langID = loc;
	if (langID == s_defaultLang)
		return nullptr;

	wchar_t langpath[MAX_PATH]{};
	const std::wstring sVer{ TEXT(STRPRODUCTVER) };
	GetModuleFileName(nullptr, langpath, _countof(langpath));
	wchar_t* pSlash = wcsrchr(langpath, L'\\');
	if (!pSlash)
		return nullptr;

	*pSlash = '\0';
	pSlash = wcsrchr(langpath, L'\\');
	if (!pSlash)
		return nullptr;

	*++pSlash = '\0';

	do
	{
		const std::wstring langdllpath = std::format(L"{}Languages\\{}{}.dll", langpath, appname, langID);
		if (CI18NHelper::DoVersionStringsMatch(CPathUtils::GetVersionFromFile(langdllpath.data()), sVer))
		{
			m_hInstance = ::LoadLibrary(langdllpath.data());

			if (m_hInstance)
				break;
		}

		DWORD lid = SUBLANGID(langID);
		lid--;
		if (lid > 0)
			langID = MAKELANGID(PRIMARYLANGID(langID), lid);
		else
			langID = 0;
	} while (langID != 0);

	m_langId = langID;

	return m_hInstance;
}
