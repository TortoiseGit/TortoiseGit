// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2019 - TortoiseGit

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
#include "MassiveGitTaskBase.h"
#include "Git.h"
#include <assert.h>

CMassiveGitTaskBase::CMassiveGitTaskBase(CString gitParameters, BOOL isPath, bool ignoreErrors)
	: m_bUnused(true)
	, m_bIsPath(isPath)
	, m_bIgnoreErrors(ignoreErrors)
	, m_sParams(gitParameters)
{
}

CMassiveGitTaskBase::~CMassiveGitTaskBase(void)
{
}

void CMassiveGitTaskBase::AddFile(const CString& filename)
{
	assert(m_bUnused);
	if (m_bIsPath)
		m_pathList.AddPath(filename);
	else
		m_itemList.push_back(filename);
}

void CMassiveGitTaskBase::AddFile(const CTGitPath& filename)
{
	assert(m_bUnused);
	if (m_bIsPath)
		m_pathList.AddPath(filename);
	else
		m_itemList.push_back(filename.GetGitPathString());
}

void CMassiveGitTaskBase::SetPaths(const CTGitPathList* pathList)
{
	assert(m_bUnused);
	m_bIsPath = true;
	m_pathList.Clear();
	m_pathList = *pathList;
}

bool CMassiveGitTaskBase::Execute(BOOL& cancel)
{
	assert(m_bUnused);
	m_pathList.RemoveDuplicates();
	return ExecuteCommands(cancel);
}

bool CMassiveGitTaskBase::ExecuteCommands(volatile BOOL& cancel)
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
			{
				add += L" \"";
				add += GetListItem(j);
				add += L'"';
			}

			CString cmd, out;
			cmd.Format(L"git.exe %s %s%s", static_cast<LPCTSTR>(m_sParams), m_bIsPath ? L"--" : L"", static_cast<LPCTSTR>(add));
			int exitCode = g_Git.Run(cmd, &out, CP_UTF8);
			if (exitCode && !m_bIgnoreErrors)
			{
				ReportError(out, exitCode);
				return false;
			}

			if (m_bIsPath)
			{
				for (int j = firstCombine; j <= i; ++j)
					ReportProgress(m_pathList[j], j);
			}

			maxLength = 0;
			firstCombine = i+1;

			if (cancel)
			{
				ReportUserCanceled();
				return false;
			}
		}
		else
			maxLength += 3 + GetListItem(i).GetLength();
	}
	return true;
}

void CMassiveGitTaskBase::ReportError(const CString& out, int /*exitCode*/)
{
	MessageBox(nullptr, out, L"TortoiseGit", MB_OK | MB_ICONERROR);
}

int CMassiveGitTaskBase::GetListCount() const
{
	return m_bIsPath ? m_pathList.GetCount() : static_cast<int>(m_itemList.size());
}

bool CMassiveGitTaskBase::IsListEmpty() const
{
	return m_bIsPath ? m_pathList.IsEmpty() : m_itemList.empty();
}

CString CMassiveGitTaskBase::GetListItem(int index) const
{
	return m_bIsPath ? m_pathList[index].GetGitPathString() : m_itemList[index];
}
