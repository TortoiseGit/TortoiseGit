// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "PreserveChdir.h"

PreserveChdir::PreserveChdir()
{
	DWORD len = GetCurrentDirectory(0, nullptr);
	if (!len)
		return;

	m_originalCurrentDirectory = std::make_unique<TCHAR[]>(len);
	if (GetCurrentDirectory(len, m_originalCurrentDirectory.get()) == 0)
		m_originalCurrentDirectory.reset();
}

PreserveChdir::~PreserveChdir()
{
	if (!m_originalCurrentDirectory)
		return;

	DWORD len = GetCurrentDirectory(0, nullptr);
	auto currentDirectory = std::make_unique<TCHAR[]>(len);

	// _tchdir is an expensive function - don't call it unless we really have to
	GetCurrentDirectory(len, currentDirectory.get());
	if (wcscmp(currentDirectory.get(), m_originalCurrentDirectory.get()) != 0)
		SetCurrentDirectory(m_originalCurrentDirectory.get());
}
