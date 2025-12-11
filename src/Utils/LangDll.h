// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2025 - TortoiseGit
// Copyright (C) 2003-2007, 2014 - TortoiseSVN

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
#pragma once
#include "SmartHandle.h"

/**
 * \ingroup Utils
 * Helper class to load language dependent resource dlls.
 */
class CLangDll
{
public:
	CLangDll() = default;
	~CLangDll() = default;

	CLangDll(const CLangDll&) = delete;
	CLangDll& operator=(const CLangDll&) = delete;

	HINSTANCE	Init(LPCWSTR appname);

	static constexpr DWORD s_defaultLang = 1033;

private:
	CAutoLibrary	m_hInstance;
	DWORD			m_langId = s_defaultLang;
};
