// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 - Sven Strickroth <email@cs-ware.de>

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
#include "GitProgressDlg.h"
#include "TGitPath.h"

#define MAX_COMMANDLINE_LENGTH 30000

typedef BOOL (CGitProgressDlg::*NOTIFY_CALLBACK)(const CTGitPath& path, git_wc_notify_action_t action, int status, CString *strErr);

class MassiveGitTask
{
public:
	MassiveGitTask(CString params);
	~MassiveGitTask(void);

	void					AddFile(CString filename);
	void					AddFile(CTGitPath filename);
	bool					ExecuteWithNotify(CTGitPathList *pathList, BOOL &cancel, git_wc_notify_action_t action, CGitProgressDlg * instance, NOTIFY_CALLBACK m_NotifyCallbackMethod);
	bool					Execute(BOOL &cancel);

private:
	bool					ExecuteCommands(BOOL &cancel);
	bool					m_bUnused;
	CString					m_sParams;
	CTGitPathList			m_pathList;
	CGitProgressDlg *		m_NotifyCallbackInstance;
	NOTIFY_CALLBACK			m_NotifyCallbackMethod;
	git_wc_notify_action_t	m_NotifyCallbackAction;
};
