// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009, 2013 - TortoiseSVN

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

/**
 * \ingroup TortoiseShell
 * Represents a list of directory elements like folders and files.
 */
class ItemIDList
{
public:
	ItemIDList(PCUITEMID_CHILD item, PCUIDLIST_RELATIVE parent);
	ItemIDList(PCIDLIST_ABSOLUTE item);

	int size() const;
	LPCSHITEMID get(int index) const;
	virtual ~ItemIDList();

	tstring toString(bool resolveLibraries = true);

	PCUITEMID_CHILD operator& ();
private:
	PCUITEMID_CHILD item_;
	PCUIDLIST_RELATIVE parent_;
	mutable int count_;
};

