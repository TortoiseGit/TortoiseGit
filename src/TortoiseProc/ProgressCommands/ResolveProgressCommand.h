// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014, 2023 - TortoiseGit

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

#include "GitProgressList.h"

enum class ResolveWith
{
	Current,
	Mine,
	Theirs,
};

class ResolveProgressCommand : public ProgressCommand
{
public:
	ResolveProgressCommand(ResolveWith resolveWith)
		: m_resolveWith(resolveWith)
	{
	};
	bool Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount) override;
	bool ShowInfo(CString& info) override;

private:
	ResolveWith m_resolveWith = ResolveWith::Current;
};
