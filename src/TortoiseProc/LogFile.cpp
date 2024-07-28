// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008, 2013-2014, 2016, 2024 - TortoiseGit
// Copyright (C) 2007, 2010-2011, 2014-2016, 2024 - TortoiseSVN

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
#include "LogFile.h"
#include "PathUtils.h"

CLogFile::CLogFile(const CString& repo, DWORD maxLines)
	: m_maxLines(maxLines)
	, m_sRepo(repo)
{
}

CLogFile::~CLogFile()
{
	assert(m_newLines.empty());
}

bool CLogFile::Open()
{
	CTGitPath logfile = CTGitPath(CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt");
	return Open(logfile);
}

bool CLogFile::Open(const CTGitPath& logfile)
{
	if (m_maxLines == 0)
		return false; // do nothing if no log lines should be used.
	assert(m_newLines.empty());
	m_logFile = logfile;
	if (!logfile.Exists())
		CPathUtils::MakeSureDirectoryPathExists(logfile.GetContainingDirectory().GetWinPath());

	return true;
}

void CLogFile::AddLine(const CString& line)
{
	m_newLines.emplace_back(line);
}

bool CLogFile::Close()
{
	assert(!m_logFile.IsEmpty());
	if (m_maxLines == 0)
		return false; // do nothing if no log lines should be used.
	try
	{
		// limit log file growth
		const size_t maxLines = static_cast<DWORD>(m_maxLines);
		const size_t newLines = m_newLines.size();
		TrimFile(static_cast<DWORD>(max(maxLines, newLines) - newLines));

		// append new info
		CStdioFile file;

		int retrycounter = 10;
		// try to open the file for about two seconds - some other TSVN process might be
		// writing to the file and we just wait a little for this to finish
		while (!file.Open(m_logFile.GetWinPath(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate) && retrycounter)
		{
			retrycounter--;
			Sleep(200);
		}
		if (retrycounter == 0)
			return false;

		file.SeekToEnd();
		for (auto it = m_newLines.begin(); it != m_newLines.end(); ++it)
		{
			file.WriteString(*it);
			file.WriteString(L"\n");
		}
		file.Close();
		m_newLines.clear();
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException saving log file\n");
		pE->Delete();
		return false;
	}
	return true;
}

void CLogFile::AddTimeLine()
{
	CString sLine;
	// first add an empty line as a separator
	m_newLines.emplace_back(sLine);
	// now add the time string
	wchar_t datebuf[4096] = { 0 };
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, datebuf, 4096);
	sLine = datebuf;
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, nullptr, nullptr, datebuf, 4096);
	sLine += L" - ";
	sLine += datebuf;
	sLine += L" - ";
	sLine += m_sRepo;
	m_newLines.emplace_back(sLine);
}

void CLogFile::TrimFile(DWORD maxLines) const
{
	// find the start of the maxLines-th last line
	// (\n is always a new line - regardless of the encoding)
	CAutoFile file = ::CreateFile(m_logFile.GetWinPath(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!file)
		return;

	size_t newSize = 0;
	{
		HANDLE mapping = ::CreateFileMapping(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if (!mapping)
			return;
		SCOPE_EXIT
		{
			if (mapping)
				CloseHandle(mapping);
		};

		unsigned char* begin = static_cast<unsigned char*>(MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0));
		if (!begin)
		{
			// special case, just truncate the file (e.g., on x86 file is too big)
			CloseHandle(mapping);
			mapping = nullptr;
			SetFilePointer(file, 0, nullptr, FILE_BEGIN);
			SetEndOfFile(file);
			return;
		}
		SCOPE_EXIT { UnmapViewOfFile(begin); };

		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(file, &fileSize))
			return;
		unsigned char* end = begin + fileSize.QuadPart; // should be safe as the whole file is locked and could be mapped into memory
		if (begin >= end - 2)
			return;

		unsigned char* trimPos = begin;
		for (unsigned char* s = end; s != begin; --s)
		{
			if (*(s - 1) != '\n')
				continue;

			if (maxLines == 0)
			{
				trimPos = s;
				break;
			}

			--maxLines;
		}

		// need to remove lines from the beginning of the file?
		if (trimPos == begin)
			return;

		// remove data
		newSize = end - trimPos;
		memmove(begin, trimPos, newSize);
	}

	// truncate file
	LARGE_INTEGER fileSize;
	fileSize.QuadPart = newSize;
	SetFilePointerEx(file, fileSize, nullptr, FILE_BEGIN);
	SetEndOfFile(file);
}
