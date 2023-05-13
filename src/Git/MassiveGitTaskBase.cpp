// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2019-2020, 2022-2023 - TortoiseGit

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

CMassiveGitTaskBase::~CMassiveGitTaskBase()
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

bool CMassiveGitTaskBase::Execute(volatile BOOL& cancel)
{
	assert(m_bUnused);
	m_pathList.RemoveDuplicates();
	return ExecuteCommands(cancel);
}

template <typename... T>
bool startsWithOrIsParam(const CString& parameters, const CString& supportedParameter, T... tail)
{
	return startsWithOrIsParam(parameters, supportedParameter) || startsWithOrIsParam(parameters, tail...);
}
template<>
bool startsWithOrIsParam(const CString& parameters, const CString& supportedParameter)
{
	return parameters == supportedParameter || CStringUtils::StartsWith(parameters, supportedParameter + L' ');
}

bool CMassiveGitTaskBase::ExecuteCommands(volatile BOOL& cancel)
{
	m_bUnused = false;

	if (IsListEmpty())
		return true;

	if (m_bIsPath && CGit::ms_LastMsysGitVersion >= ConvertVersionToInt(2, 26, 0) && startsWithOrIsParam(m_sParams, L"add", L"rm", L"reset", L"checkout", L"restore", L"stash"))
	{
		CString tempFilename = GetTempFile();
		if (tempFilename.IsEmpty())
		{
			ReportError(L"Error creating temp file", -1);
			return false;
		}
		SCOPE_EXIT { ::DeleteFile(tempFilename); };

		if (!m_pathList.WriteToPathSpecFile(tempFilename))
		{
			ReportError(L"Error writing to temp file", -1);
			return false;
		}

		CString cmd, out;
		cmd.Format(L"git.exe %s --pathspec-from-file=\"%s\" --pathspec-file-nul", static_cast<LPCWSTR>(m_sParams), static_cast<LPCWSTR>(tempFilename));
		int exitCode = g_Git.Run(cmd, &out, CP_UTF8);
		if (exitCode && !m_bIgnoreErrors)
		{
			ReportError(out, exitCode);
			return false;
		}

		for (int i = 0; i < GetListCount(); ++i)
			ReportProgress(m_pathList[i], i);

		if (cancel)
		{
			ReportUserCanceled();
			return false;
		}
		return true;
	}

	int max_command_line_length = 30000;
	int quotes_length = 2;
	if (CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) // see issue https://tortoisegit.org/issue/3542
	{
		max_command_line_length = 3500;
		quotes_length = 4;
	}

	int maxLength = 0;
	int firstCombine = 0;
	for (int i = 0; i < GetListCount(); ++i)
	{
		if (maxLength + GetListItem(i).GetLength() > max_command_line_length || i == GetListCount() - 1 || cancel)
		{
			CString add;
			for (int j = firstCombine; j <= i; ++j)
			{
				add += L" \"";
				add += GetListItem(j);
				add += L'"';
			}

			CString cmd, out;
			cmd.Format(L"git.exe %s %s%s", static_cast<LPCWSTR>(m_sParams), m_bIsPath ? L"--" : L"", static_cast<LPCWSTR>(add));
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
			maxLength += 1 + quotes_length + GetListItem(i).GetLength();
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

void CMassiveGitTaskBase::ConvertToCmdList(CString params, const STRING_VECTOR& pathList, STRING_VECTOR& cmdList)
{
	if (pathList.empty())
		return;

	// see issue https://tortoisegit.org/issue/3542
	const int max_command_line_length{ (CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) ? 3500 : 30000 };
	const int quotes_length{ (CGit::ms_bCygwinGit || CGit::ms_bMsys2Git) ? 4 : 2 };

	CString cmd;
	cmd.Format(L"git.exe %s --", static_cast<LPCWSTR>(params));

	bool noCmdYet{ true };
	for (const auto& filename : pathList)
	{
		// add new command if no command yet or last command will exceed max length
		if (noCmdYet || (cmdList.back().GetLength() + 1 + quotes_length + filename.GetLength()) > max_command_line_length)
		{
			noCmdYet = false;
			cmdList.push_back(cmd);
		}

		// update last commmand of list
		cmdList.back().AppendFormat(L" \"%s\"", static_cast<LPCWSTR>(filename));
	}
}
