// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2013-2016 - TortoiseGit

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
#include "MassiveGitTask.h"
#include "ProgressDlg.h"

CMassiveGitTask::CMassiveGitTask(CString gitParameters, BOOL isPath, bool ignoreErrors)
	: CMassiveGitTaskBase(gitParameters, isPath, ignoreErrors)
	, m_NotifyCallbackInstance(nullptr)
	, m_NotifyCallbackAction(CGitProgressList::WC_File_NotificationData::git_wc_notify_add)
{
}

CMassiveGitTask::~CMassiveGitTask(void)
{
}

void CMassiveGitTask::ReportError(const CString& out, int exitCode)
{
	if (m_NotifyCallbackInstance)
		m_NotifyCallbackInstance->ReportError(out);
	else
	{
		CProgressDlg dlg;
		dlg.m_PreText = L"git.exe ";
		dlg.m_PreText += GetParams();
		dlg.m_PreText += L" [...]\r\n\r\n";
		dlg.m_PreText += out;
		dlg.m_PreFailText.Format(IDS_PROC_PROGRESS_GITUNCLEANEXIT, exitCode);
		dlg.DoModal();
	}
}

void CMassiveGitTask::ReportProgress(const CTGitPath& path, int index)
{
	if (m_NotifyCallbackInstance)
	{
		m_NotifyCallbackInstance->AddNotify(new CGitProgressList::WC_File_NotificationData(path, m_NotifyCallbackAction));
		m_NotifyCallbackInstance->SetItemProgress(index);
	}
}

void CMassiveGitTask::ReportUserCanceled()
{
	if (m_NotifyCallbackInstance)
		m_NotifyCallbackInstance->ReportUserCanceled();
}

bool CMassiveGitTask::ExecuteWithNotify(CTGitPathList* pathList, volatile BOOL& cancel, CGitProgressList::WC_File_NotificationData::git_wc_notify_action_t action, CGitProgressList* instance)
{
	SetPaths(pathList);
	m_NotifyCallbackInstance = instance;
	m_NotifyCallbackAction = action;
	return ExecuteCommands(cancel);
}

