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

#include "StdAfx.h"
#include "TortoiseProc.h"
#include "MassiveGitTask.h"
#include "MessageBox.h"

CMassiveGitTask::CMassiveGitTask(CString gitParameters)
	: m_bUnused(true)
	, m_NotifyCallbackInstance(NULL)
	, m_NotifyCallbackMethod(NULL)
{
	m_sParams = gitParameters;
}

CMassiveGitTask::~CMassiveGitTask(void)
{
}

void CMassiveGitTask::AddFile(CString filename)
{
	assert(m_bUnused);
	m_pathList.AddPath(filename);
}

void CMassiveGitTask::AddFile(CTGitPath filename)
{
	assert(m_bUnused);
	m_pathList.AddPath(filename);
}

bool CMassiveGitTask::ExecuteWithNotify(CTGitPathList *pathList, BOOL &cancel, git_wc_notify_action_t action, CGitProgressDlg * instance, NOTIFY_CALLBACK notifyMethod)
{
	assert(m_bUnused);
	m_bUnused = false;

	m_pathList = *pathList;
	m_NotifyCallbackInstance = instance;
	m_NotifyCallbackMethod = notifyMethod;
	m_NotifyCallbackAction = action;
	return ExecuteCommands(cancel);
}

bool CMassiveGitTask::Execute(BOOL &cancel)
{
	assert(m_bUnused);
	m_pathList.RemoveDuplicates();
	return ExecuteCommands(cancel);
}

bool CMassiveGitTask::ExecuteCommands(BOOL &cancel)
{
	m_bUnused = false;

	bool bErrorsOccurred = false;
	int maxLength = 0;
	int firstCombine = 0;
	for(int i = 0; i < m_pathList.GetCount(); i++)
	{
		if (maxLength + m_pathList[i].GetGitPathString().GetLength() > MAX_COMMANDLINE_LENGTH || i == m_pathList.GetCount() - 1 || cancel)
		{
			CString add;
			for (int j = firstCombine; j <= i; j++)
				add += _T(" \"") + m_pathList[j].GetGitPathString() + _T("\"");

			CString cmd, out;
			cmd.Format(_T("git.exe %s --%s"), m_sParams, add);
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
				bErrorsOccurred = true;
				return false;
			}

			if (m_NotifyCallbackInstance && m_NotifyCallbackMethod)
				for (int j = firstCombine; j <= i; j++)
					(*m_NotifyCallbackInstance.*m_NotifyCallbackMethod)(m_pathList[j], m_NotifyCallbackAction, 0, NULL);

			maxLength = 0;
			firstCombine = i+1;

			if (cancel)
				break;
		}
		else
		{
			maxLength += 3 + m_pathList[i].GetGitPathString().GetLength();
		}
	}
	return !bErrorsOccurred;
}
