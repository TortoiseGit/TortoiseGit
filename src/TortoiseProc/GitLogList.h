// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2014-2015, 2017, 2019 - TortoiseGit

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
#include "GitLoglistBase.h"

class CGitLogListBase;

class CGitLogList : public CGitLogListBase
{
	DECLARE_DYNAMIC(CGitLogList)

public:
	void ShiftSelectedRebaseAction();
	void SetSelectedRebaseAction(int action);
	void SetUnselectedRebaseAction(int action);

protected:
	int	 CherryPickFrom(CString from, CString to);
	int  RevertSelectedCommits(int parent);
	void ContextMenuAction(int cmd, int FirstSelect, int LastSelect, CMenu* menu, MAP_HASH_NAME& hashMap) override;
};
