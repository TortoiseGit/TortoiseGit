// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014, 2016, 2018-2019 - TortoiseGit

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

class AddProgressCommand : public ProgressCommand
{
protected:
	bool m_bShowCommitButtonAfterAdd;
	bool m_bExecutable;
	bool m_bSymlink;

	bool SetFileMode(uint32_t mode);

public:
	virtual bool Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount) override;

	AddProgressCommand()
		: m_bShowCommitButtonAfterAdd(true)
		, m_bExecutable(false)
		, m_bSymlink(false)
	{}

	void SetShowCommitButtonAfterAdd(bool b) { m_bShowCommitButtonAfterAdd = b; }
	void SetExecutable(bool b = true)
	{
		m_bExecutable = b;
		ATLASSERT(!(m_bExecutable && m_bSymlink));
	}
	void SetSymlink(bool b = true)
	{
		m_bSymlink = b;
		ATLASSERT(!(m_bExecutable && m_bSymlink));
	}
};
