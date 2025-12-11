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
#include "LangDll.h"
#include "../version.h"
#include "I18NHelper.h"
#include "PathUtils.h"
#include "registry.h"
#include "CrashReport.h"
#include <format>
#if defined(TORTOISEGITPROC)
#include "StringUtils.h"
#include "DirFileEnum.h"
#endif

HINSTANCE CLangDll::Init(LPCWSTR appname)
{
	CRegStdDWORD loc = CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", m_langId);
	return Init(appname, nullptr, loc);
}

HINSTANCE CLangDll::Init(LPCWSTR appname, HMODULE hModule, DWORD langID)
{
	m_hInstance.CloseHandle(); // close any existing handle

	if (langID == s_defaultLang)
		return nullptr;

	wchar_t langpath[MAX_PATH]{};
	const std::wstring sVer{ TEXT(STRPRODUCTVER) };
	GetModuleFileName(hModule, langpath, _countof(langpath));
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
		const std::wstring langdllpath = std::format(L"{}{}{}{}.dll", langpath, s_languagesfolder, appname, langID);
		if (CI18NHelper::DoVersionStringsMatch(CPathUtils::GetVersionFromFile(langdllpath.data()), sVer))
		{
			if (hModule)
				m_hInstance = ::LoadLibraryEx(langdllpath.data(), nullptr, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
			else
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
#if ENABLE_CRASHHANLDER && !_M_ARM64 && !TORTOISESHELL
	CCrashReport::Instance().AddUserInfoToReport(L"LanguageID", std::to_wstring(langID).data());
#endif

	return m_hInstance;
}
#if defined(TORTOISEGITPROC)
std::vector<std::pair<CString, DWORD>> CLangDll::GetInstalledLanguages(bool includeNative /* false */, bool checkVersion /* true */)
{
	std::vector<std::pair<CString, DWORD>> langs;

	wchar_t buf[MAX_PATH]{};
	if (includeNative)
	{
		GetLocaleInfo(CLangDll::s_defaultLang, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		langs.emplace_back(std::make_pair<>(buf, CLangDll::s_defaultLang));
	}
	CString path = CPathUtils::GetAppParentDirectory();
	const std::wstring sVer{ TEXT(STRPRODUCTVER) };
	path += s_languagesfolder.data();
	CSimpleFileFind finder(path, L"*.dll");
	while (finder.FindNextFileNoDirectories())
	{
		CString filename = finder.GetFileName();
		if (!CStringUtils::StartsWithI(filename, L"TortoiseProc"))
			continue;

		CString file = finder.GetFilePath();
		if (checkVersion && !CI18NHelper::DoVersionStringsMatch(CPathUtils::GetVersionFromFile(file), sVer))
			continue;
		CString sLoc = filename.Mid(static_cast<int>(wcslen(L"TortoiseProc")));
		sLoc = sLoc.Left(sLoc.GetLength() - static_cast<int>(wcslen(L".dll"))); // cut off ".dll"
		if (CStringUtils::StartsWith(sLoc, L"32") && (sLoc.GetLength() > 5))
			continue;
		const DWORD loc = _wtoi(filename.Mid(static_cast<int>(wcslen(L"TortoiseProc"))));
		if (!loc)
			continue;
		GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		CString sLang = buf;
		GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
		if (buf[0])
		{
			sLang += L" (";
			sLang += buf;
			sLang += L')';
		}
		langs.emplace_back(std::make_pair<>(sLang, loc));
	}
	return langs;
}
#endif
