// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2014 - TortoiseSVN
// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2015 - TortoiseSI

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
#include "ShellUpdater.h"

/**
 * \ingroup TortoiseShell
 * Displays and updates all controls on the property page. The property
 * page itself is shown by explorer.
 */
class CGitPropertyPage
{
public:
	CGitPropertyPage(const std::vector<stdstring> &filenames);
	virtual ~CGitPropertyPage();

	/**
	 * Sets the window handle.
	 * \param hwnd the handle.
	 */
	virtual void SetHwnd(HWND hwnd);
	/**
	 * Callback function which receives the window messages of the
	 * property page. See the Win32 API for PropertySheets for details.
	 */
	virtual BOOL PageProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	const static UINT m_UpdateLastCommit;

protected:
	/**
	 * Initializes the property page.
	 */
//	virtual void InitWorkfileView();
//	void DisplayCommit(const git_commit* commit, UINT hashLabel, UINT subjectLabel, UINT authorLabel, UINT dateLabel);
	static void LogThreadEntry(void *param);
//	int LogThread();
	void Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen) const;
	void PageProcOnCommand(WPARAM wParam);
	void RunCommand(const tstring& command);

	HWND m_hwnd;
	std::vector<stdstring> filenames;
	/**
	 * Were executable, assumeValid or skip-worktree flags changes
	 */
	bool m_bChanged;
};


