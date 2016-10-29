// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2011, 2015-2016 - TortoiseSVN

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
#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <tuple>

typedef struct tagResourceEntry
{
	WORD                        menuID;
	std::vector<std::wstring>   translatorcomments;
	std::vector<std::wstring>   automaticcomments;
	std::set<std::wstring>      resourceIDs;
	std::wstring                flag;
	std::wstring                msgstr;
	std::wstring                headerfile;
} RESOURCEENTRY, * LPRESOURCEENTRY;

/**
 * \ingroup ResText
 * Class to handle po-files. Inherits from an std::map which assigns
 * string IDs to additional information, including the translated strings.
 *
 * Provides methods to load and save a po-file with the translation information
 * we need for ResText.
 */
class CPOFile : public std::map<std::wstring, RESOURCEENTRY>
{
public:
	CPOFile();
	~CPOFile(void);

	BOOL ParseFile(LPCTSTR szPath, BOOL bUpdateExisting, bool bAdjustEOLs);
	BOOL SaveFile(LPCTSTR szPath, LPCTSTR lpszHeaderFile);
	void SetQuiet(BOOL bQuiet = TRUE) {m_bQuiet = bQuiet;}

	std::vector<std::tuple<std::wstring, std::wstring>> m_regexes;

private:
	void AdjustEOLs(std::wstring& str);
	BOOL m_bQuiet;
	bool m_bAdjustEOLs;
};
