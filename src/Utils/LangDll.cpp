﻿// TortoiseGit - a Windows shell extension for easy version control

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

CLangDll::CLangDll()
	: m_hInstance(nullptr)
{
}

CLangDll::~CLangDll()
{
	Close();
}

HINSTANCE CLangDll::Init(LPCWSTR appname, unsigned long langID)
{
	wchar_t langpath[MAX_PATH] = { 0 };
	wchar_t langdllpath[MAX_PATH] = { 0 };
	wchar_t sVer[MAX_PATH] = { 0 };
	wcscpy_s(sVer, TEXT(STRPRODUCTVER));
	GetModuleFileName(nullptr, langpath, _countof(langpath));
	wchar_t* pSlash = wcsrchr(langpath, L'\\');
	if (!pSlash)
		return m_hInstance;

	*pSlash = 0;
	pSlash = wcsrchr(langpath, L'\\');
	if (!pSlash)
		return m_hInstance;

	*pSlash = 0;
	wcscat_s(langpath, L"\\Languages\\");
	assert(m_hInstance == nullptr);
	do
	{
		swprintf_s(langdllpath, L"%s%s%lu.dll", langpath, appname, langID);

		m_hInstance = ::LoadLibraryEx(langdllpath, nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);

		if (!DoVersionStringsMatch(sVer, langdllpath))
		{
			if (m_hInstance)
				FreeLibrary(m_hInstance);
			m_hInstance = nullptr;
		}
		if (!m_hInstance)
		{
			DWORD lid = SUBLANGID(langID);
			lid--;
			if (lid > 0)
				langID = MAKELANGID(PRIMARYLANGID(langID), lid);
			else
				langID = 0;
		}
	} while (!m_hInstance && (langID != 0));

	return m_hInstance;
}

void CLangDll::Close()
{
	if (!m_hInstance)
		return;

	FreeLibrary(m_hInstance);
	m_hInstance = nullptr;
}

bool CLangDll::DoVersionStringsMatch(LPCWSTR sVer, LPCWSTR langDll) const
{
	return CI18NHelper::DoVersionStringsMatch(CPathUtils::GetVersionFromFile(langDll), sVer);
}
