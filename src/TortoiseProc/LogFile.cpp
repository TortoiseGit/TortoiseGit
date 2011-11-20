// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "LogFile.h"
#include "PathUtils.h"

CLogFile::CLogFile(void)
{
	m_maxlines = CRegStdDWORD(_T("Software\\TortoiseGit\\MaxLinesInLogfile"), 4000);
}

CLogFile::~CLogFile(void)
{
}

bool CLogFile::Open()
{
	CTGitPath logfile = CTGitPath(CPathUtils::GetAppDataDirectory() + _T("\\logfile.txt"));
	return Open(logfile);
}

bool CLogFile::Open(const CTGitPath& logfile)
{
	m_lines.clear();
	m_logfile = logfile;
	if (!logfile.Exists())
	{
		CPathUtils::MakeSureDirectoryPathExists(logfile.GetContainingDirectory().GetWinPath());
		return true;
	}

	try
	{
		CString strLine;
		CStdioFile file;
		int retrycounter = 10;
		// try to open the file for about two seconds - some other TSVN process might be
		// writing to the file and we just wait a little for this to finish
		while (!file.Open(logfile.GetWinPath(), CFile::typeText | CFile::modeRead | CFile::shareDenyWrite) && retrycounter)
		{
			retrycounter--;
			Sleep(200);
		}
		if (retrycounter == 0)
			return false;

		while (file.ReadString(strLine))
		{
			m_lines.push_back(strLine);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading autolist regex file\n");
		pE->Delete();
		return false;
	}
	return true;
}

bool CLogFile::AddLine(const CString& line)
{
	m_lines.push_back(line);
	return true;
}

bool CLogFile::Close()
{
	AdjustSize();
	try
	{
		CString strLine;
		CStdioFile file;

		int retrycounter = 10;
		// try to open the file for about two seconds - some other TSVN process might be
		// writing to the file and we just wait a little for this to finish
		while (!file.Open(m_logfile.GetWinPath(), CFile::typeText | CFile::modeWrite | CFile::modeCreate) && retrycounter)
		{
			retrycounter--;
			Sleep(200);
		}
		if (retrycounter == 0)
			return false;

		for (std::list<CString>::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it)
		{
			file.WriteString(*it);
			file.WriteString(_T("\n"));
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading autolist regex file\n");
		pE->Delete();
		return false;
	}
	return true;
}

bool CLogFile::AddTimeLine()
{
	CString sLine;
	// first add an empty line as a separator
	m_lines.push_back(sLine);
	// now add the time string
	TCHAR datebuf[4096] = {0};
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, datebuf, 4096);
	sLine = datebuf;
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, datebuf, 4096);
	sLine += _T(" - ");
	sLine += datebuf;
	m_lines.push_back(sLine);
	return true;
}

void CLogFile::AdjustSize()
{
	DWORD maxlines = m_maxlines;

	while (m_lines.size() > maxlines)
	{
		m_lines.pop_front();
	}
}