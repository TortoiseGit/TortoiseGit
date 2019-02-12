// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2009, 2012 - TortoiseSVN

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
#include "ShellObjects.h"


ShellObjects::ShellObjects()
{
}

ShellObjects::~ShellObjects()
{
}

void ShellObjects::Insert(CShellExt * obj)
{
	AutoLocker lock(m_critSec);
	m_exts.insert(obj);
}

void ShellObjects::Erase(CShellExt * obj)
{
	AutoLocker lock(m_critSec);
	m_exts.erase(obj);
}

void ShellObjects::DeleteAll()
{
	AutoLocker lock(m_critSec);
	if (!m_exts.empty())
	{
		std::set<CShellExt *>::iterator it = m_exts.begin();
		while (it != m_exts.end())
		{
			delete *it;
			it = m_exts.begin();
		}
	}
}
