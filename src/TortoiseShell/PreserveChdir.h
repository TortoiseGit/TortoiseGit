// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2006,2008 - Stefan Kueng

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
#include <memory>

/**
 * \ingroup TortoiseShell
 * Helper class to keep the current directory.
 * When created, the current directory is saved and restored
 * as soon as the object is destroyed.
 * Since some of the SVN functions change the current directory
 * which when in the explorer process space causes problems / crashes,
 * this class is used to reset the current directory after calling
 * those functions.
 */
class PreserveChdir
{
public:
	PreserveChdir();
	~PreserveChdir();

private:
	std::unique_ptr<TCHAR[]> m_originalCurrentDirectory;
};

