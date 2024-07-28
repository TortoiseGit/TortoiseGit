// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2024 - TortoiseGit
// Copyright (C) 2007-2008, 2010, 2021 - TortoiseSVN

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
#include "registry.h"
#include "TGitPath.h"
#include <deque>

/**
 * \ingroup TortoiseProc
 * CLogFile implements a log file with a limited size.
 * The log file consists of multiple lines. The amount of lines can
 * be limited with the registry entry Software\\TortoiseGit\\MaxLinesInLogfile
 * and defaults to 4000 lines.
 */
class CLogFile
{
public:
	CLogFile(const CString& repo, DWORD maxLines = CRegStdDWORD(L"Software\\TortoiseGit\\MaxLinesInLogfile", 4000));
	~CLogFile();

	/**
	 * Opens the log file and reads its contents
	 */
	bool	Open(const CTGitPath& logfile);
	/**
	 * Opens the default log file for TortoiseSVN and reads its contents
	 */
	bool	Open();
	/**
	 * Adds one line to the log file. The file is *not* yet written back to disk.
	 */
	void	AddLine(const CString& line);
	/**
	 * Writes the contents to the disk.
	 */
	bool	Close();
	/**
	 * Inserts a line with the current time and date to the log file. The file is *not* yet written back to disk.
	 */
	void	AddTimeLine();

private:
	void TrimFile(DWORD maxLines) const;

	CString					m_sRepo;
	DWORD					m_maxLines;
	CTGitPath				m_logFile;
	std::deque<CString>		m_newLines;
};
