// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2013 - TortoiseGit

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MassiveGitTask.h"
#include "MessageBox.h"

CMassiveGitTask::CMassiveGitTask(CString gitParameters, BOOL isPath)
	: m_bUnused(true)
	, m_bIsPath(isPath)
	, m_NotifyCallbackInstance(NULL)
	, m_NotifyCallbackMethod(NULL)
	, m_NotifyCallbackAction(git_wc_notify_add)
{
	m_sParams = gitParameters;
}

CMassiveGitTask::~CMassiveGitTask(void)
{
}

void CMassiveGitTask::AddFile(CString filename)
{
	assert(m_bUnused);
	if (m_bIsPath)
		m_pathList.AddPath(filename);
	else
		m_itemList.push_back(filename);
}

void CMassiveGitTask::AddFile(CTGitPath filename)
{
	assert(m_bUnused);
	if (m_bIsPath)
		m_pathList.AddPath(filename);
	else
		m_itemList.push_back(filename.GetGitPathString());
}

bool CMassiveGitTask::ExecuteWithNotify(CTGitPathList *pathList, volatile BOOL &cancel, git_wc_notify_action_t action, CGitProgressList * instance, NOTIFY_CALLBACK notifyMethod)
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

bool CMassiveGitTask::ExecuteCommands(volatile BOOL &cancel)
{
	m_bUnused = false;

	int maxLength = 0;
	int firstCombine = 0;
	for (int i = 0; i < GetListCount(); ++i)
	{
		if (maxLength + GetListItem(i).GetLength() > MAX_COMMANDLINE_LENGTH || i == GetListCount() - 1 || cancel)
		{
			CString add;
			for (int j = firstCombine; j <= i; ++j)
				add += _T(" \"") + GetListItem(j) + _T("\"");

			CString cmd, out;
			cmd.Format(_T("git.exe %s %s%s"), m_sParams, m_bIsPath ? _T("--") : _T(""), add);
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
				return false;
			}

			if (m_bIsPath)
				if (m_NotifyCallbackInstance && m_NotifyCallbackMethod)
					for (int j = firstCombine; j <= i; ++j)
						(*m_NotifyCallbackInstance.*m_NotifyCallbackMethod)(m_pathList[j], m_NotifyCallbackAction, 0, NULL);

			maxLength = 0;
			firstCombine = i+1;

			if (cancel)
				return false;
		}
		else
		{
			maxLength += 3 + GetListItem(i).GetLength();
		}
	}
	return true;
}

int CMassiveGitTask::GetListCount()
{
	return m_bIsPath ? m_pathList.GetCount() : (int)m_itemList.size();
}

CString CMassiveGitTask::GetListItem(int index)
{
	return m_bIsPath ? m_pathList[index].GetGitPathString() : m_itemList[index];
}
