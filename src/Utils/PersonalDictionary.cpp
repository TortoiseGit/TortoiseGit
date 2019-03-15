// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2019 - TortoiseGit
// Copyright (C) 2003-2006, 2008, 2014, 2016, 2019 - TortoiseSVN

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
#include <fstream>
#include <codecvt>
#include "PersonalDictionary.h"
#include "PathUtils.h"

CPersonalDictionary::CPersonalDictionary(LONG lLanguage /* = 0*/)
	: m_bLoaded(false)
	, m_lLanguage(lLanguage)
{
}

CPersonalDictionary::~CPersonalDictionary()
{
}

template<class T>
static void OpenFileStream(T& file, LONG lLanguage, std::ios_base::openmode openmode = 0)
{
	TCHAR path[MAX_PATH] = { 0 };		//MAX_PATH ok here.
	swprintf_s(path, L"%s%ld.dic", static_cast<LPCTSTR>(CPathUtils::GetAppDataDirectory()), !lLanguage ? GetUserDefaultLCID() : lLanguage);

	char filepath[MAX_PATH + 1] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, path, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);

	file.open(filepath, openmode);
}

bool CPersonalDictionary::Load()
{
	CString sWord;
	char line[PDICT_MAX_WORD_LENGTH + 1];

	if (m_bLoaded)
		return true;

	std::ifstream File;
	OpenFileStream(File, m_lLanguage);
	if (!File.good())
	{
		return false;
	}
	do
	{
		File.getline(line, _countof(line));
		sWord = CUnicodeUtils::GetUnicode(line);
		sWord.TrimRight();
		if (sWord.IsEmpty())
			continue;
		dict.insert(sWord);
	} while (File.gcount() > 0);
	File.close();
	m_bLoaded = true;
	return true;
}

bool CPersonalDictionary::AddWord(const CString& sWord)
{
	if (!m_bLoaded)
		Load();
	if (sWord.GetLength() >= PDICT_MAX_WORD_LENGTH || sWord.IsEmpty())
		return false;
	dict.insert(sWord);
	return true;
}

bool CPersonalDictionary::FindWord(const CString& sWord)
{
	if (!m_bLoaded)
		Load();
	// even if the load failed for some reason, we mark it as loaded
	// and just assume an empty personal dictionary
	m_bLoaded = true;
	std::set<CString>::iterator it;
	it = dict.find(sWord);
	return (it != dict.end());
}

bool CPersonalDictionary::Save()
{
	if (!m_bLoaded)
		return false;
	std::ofstream File;
	OpenFileStream(File, m_lLanguage, std::ios::ios_base::binary);
	for (const auto& line : dict)
		File << CUnicodeUtils::GetUTF8(line) << "\n";
	File.close();
	return true;
}
