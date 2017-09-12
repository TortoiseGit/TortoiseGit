// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseGit

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

class ResetProgressCommand : public ProgressCommand
{
private:
	CString	m_revision;
	int		m_resetType;

public:
	ResetProgressCommand()
	: m_resetType(0)
	{}

	void SetRevision(const CString& revision) { m_revision = revision; }
	void SetResetType(int resetType){ m_resetType = resetType; }
	virtual bool Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount) override;
};
