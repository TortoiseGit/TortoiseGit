// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009,2012 - TortoiseGit

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

//////////////////////////////////////////////////////////////////////////

// This sets up all of the subtree command handlers

#pragma once
#include "Command.h"

class SubtreeCommand : public Command
{
public:
	// non virtual helper
	bool ExecuteSubtree(CString cmd, CString arg=_T(""));
};

class SubtreeAddCommand : public SubtreeCommand
{
public:
	virtual bool Execute();
};

class SubtreePushCommand : public SubtreeCommand
{
public:
	virtual bool Execute();
};

class SubtreePullCommand : public SubtreeCommand
{
public:
	virtual bool Execute();
};


