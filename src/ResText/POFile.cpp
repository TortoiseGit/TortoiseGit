// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseGit
// Copyright (C) 2003-2008, 2011-2016, 2024 - TortoiseSVN

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
#include "stdafx.h"
#include <Shlwapi.h>
#include <fstream>
#include "codecvt.h"
#include "Utils.h"
#include "ResModule.h"
#include "POFile.h"

#include "../Utils/UnicodeUtils.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <functional>

#define MYERROR {CUtils::Error(); return FALSE;}

CPOFile::CPOFile()
	: m_bQuiet(false)
	, m_bAdjustEOLs(false)
{
}

CPOFile::~CPOFile()
{
}

static bool StartsWith(const wchar_t* heystacl, const wchar_t* needle)
{
	return wcsncmp(heystacl, needle, wcslen(needle)) == 0;
}

BOOL CPOFile::ParseFile(LPCWSTR szPath, BOOL bUpdateExisting, bool bAdjustEOLs)
{
	if (!PathFileExists(szPath))
		return FALSE;

	m_bAdjustEOLs = bAdjustEOLs;

	if (!m_bQuiet)
		fwprintf(stdout, L"parsing file %s...\n", szPath);

	int nEntries = 0;
	int nDeleted = 0;
	int nTranslated = 0;
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH + 1] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, szPath, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);

	std::ifstream File;
	File.open(filepath);
	if (!File.good())
	{
		fwprintf(stderr, L"can't open input file %s\n", szPath);
		return FALSE;
	}
	auto line = std::make_unique<char[]>(2 * MAX_STRING_LENGTH);
	std::vector<std::wstring> entry;
	do
	{
		File.getline(line.get(), 2*MAX_STRING_LENGTH);
		if (line[0] == 0)
		{
			//empty line means end of entry!
			RESOURCEENTRY resEntry = {0};
			std::wstring msgid;
			std::wstring regexsearch, regexreplace;
			int type = 0;
			for (auto I = entry.cbegin(); I != entry.cend(); ++I)
			{
				if (StartsWith(I->c_str(), L"# "))
				{
					//user comment
					if (StartsWith(I->c_str(), L"# regexsearch="))
						regexsearch = I->substr(14);
					else if (StartsWith(I->c_str(), L"# regexreplace="))
						regexreplace = I->substr(15);
					else
						resEntry.translatorcomments.push_back(I->c_str());
					if (!regexsearch.empty() && !regexreplace.empty())
					{
						m_regexes.push_back(std::make_tuple(regexsearch, regexreplace));
						regexsearch.clear();
						regexreplace.clear();
					}
					type = 0;
				}
				if (StartsWith(I->c_str(), L"#."))
				{
					//automatic comments
					resEntry.automaticcomments.push_back(I->c_str());
					type = 0;
				}
				if (StartsWith(I->c_str(), L"#,"))
				{
					//flag
					resEntry.flag = I->c_str();
					type = 0;
				}
				if (StartsWith(I->c_str(), L"msgid"))
				{
					//message id
					msgid = I->c_str();
					msgid = std::wstring(msgid.substr(7, msgid.size() - 8));

					std::wstring s = msgid;
					s.erase(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](const auto& c) { return !iswspace(c); }));
					if (s.size())
						nEntries++;
					type = 1;
				}
				if (StartsWith(I->c_str(), L"msgstr"))
				{
					//message string
					resEntry.msgstr = I->c_str();
					resEntry.msgstr = resEntry.msgstr.substr(8, resEntry.msgstr.length() - 9);
					if (!resEntry.msgstr.empty())
						nTranslated++;
					type = 2;
				}
				if (StartsWith(I->c_str(), L"\""))
				{
					if (type == 1)
					{
						std::wstring temp = I->c_str();
						temp = temp.substr(1, temp.length()-2);
						msgid += temp;
					}
					if (type == 2)
					{
						if (resEntry.msgstr.empty())
							nTranslated++;
						std::wstring temp = I->c_str();
						temp = temp.substr(1, temp.length()-2);
						resEntry.msgstr += temp;
					}
				}
			}
			entry.clear();
			if ((bUpdateExisting)&&(this->count(msgid) == 0))
				nDeleted++;
			else
			{
				if ((m_bAdjustEOLs)&&(msgid.find(L"\\r\\n") != std::string::npos))
				{
					AdjustEOLs(resEntry.msgstr);
				}
				// always use the new data for generated comments/flags
				auto newEntry = (*this)[msgid];
				resEntry.automaticcomments = newEntry.automaticcomments;
				resEntry.flag = newEntry.flag;
				resEntry.resourceIDs = newEntry.resourceIDs;

				(*this)[msgid] = resEntry;
			}
			msgid.clear();
		}
		else
		{
			entry.push_back(CUnicodeUtils::StdGetUnicode(line.get()));
		}
	} while (File.gcount() > 0);
	printf("%s", File.getloc().name().c_str());
	File.close();
	RESOURCEENTRY emptyentry = {0};
	(*this)[std::wstring(L"")] = emptyentry;
	if (!m_bQuiet)
		fwprintf(stdout, L"%d Entries found, %d were already translated and %d got deleted\n", nEntries, nTranslated, nDeleted);
	return TRUE;
}

BOOL CPOFile::SaveFile(LPCWSTR szPath, LPCWSTR lpszHeaderFile)
{
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH + 1] = { 0 };
	int nEntries = 0;
	WideCharToMultiByte(CP_ACP, 0, szPath, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);

	std::ofstream file;
	file.open(filepath, std::ios_base::binary);

	if ((lpszHeaderFile)&&(lpszHeaderFile[0])&&(PathFileExists(lpszHeaderFile)))
	{
		// read the header file and save it to the top of the pot file
		std::ifstream inFile;
		inFile.open(lpszHeaderFile, std::ios_base::binary);

		char ch;
		while(inFile && inFile.get(ch))
			file.put(ch);
		inFile.close();
	}
	else
	{
		file << "# SOME DESCRIPTIVE TITLE.\n";
		file << "# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER\n";
		file << "# This file is distributed under the same license as the PACKAGE package.\n";
		file << "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n";
		file << "#\n";
		file << "#, fuzzy\n";
		file << "msgid \"\"\n";
		file << "msgstr \"\"\n";
		file << "\"Project-Id-Version: PACKAGE VERSION\\n\"\n";
		file << "\"Report-Msgid-Bugs-To: \\n\"\n";
		file << "\"POT-Creation-Date: 1900-01-01 00:00+0000\\n\"\n";
		file << "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
		file << "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n";
		file << "\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n";
		file << "\"MIME-Version: 1.0\\n\"\n";
		file << "\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
		file << "\"Content-Transfer-Encoding: 8bit\\n\"\n\n";
	}
	file << "\n";
	file << "# msgid/msgstr fields for Accelerator keys\n";
	file << "# Format is: \"ID:xxxxxx:VACS+X\" where:\n";
	file << "#    ID:xxxxx = the menu ID corresponding to the accelerator\n";
	file << "#    V = Virtual key (or blank if not used) - nearly always set!\n";
	file << "#    A = Alt key     (or blank if not used)\n";
	file << "#    C = Ctrl key    (or blank if not used)\n";
	file << "#    S = Shift key   (or blank if not used)\n";
	file << "#    X = upper case character\n";
	file << "# e.g. \"V CS+Q\" == Ctrl + Shift + 'Q'\n";
	file << "\n";
	file << "# ONLY Accelerator Keys with corresponding alphanumeric characters can be\n";
	file << "# updated i.e. function keys (F2), special keys (Delete, HoMe) etc. will not.\n";
	file << "\n";
	file << "# ONLY change the msgstr field. Do NOT change any other.\n";
	file << "# If you do not want to change an Accelerator Key, copy msgid to msgstr\n";
	file << "\n";

	for (auto I = this->cbegin(); I != this->cend(); ++I)
	{
		std::wstring s = I->first;
		s.erase(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](const auto& c) { return !iswspace(c); }));
		if (s.empty())
			continue;

		RESOURCEENTRY entry = I->second;
		for (auto II = entry.automaticcomments.cbegin(); II != entry.automaticcomments.cend(); ++II)
		{
			file << CUnicodeUtils::StdGetUTF8(*II) << "\n";
		}
		for (auto II = entry.translatorcomments.cbegin(); II != entry.translatorcomments.cend(); ++II)
		{
			file << CUnicodeUtils::StdGetUTF8(*II) << "\n";
		}
		if (!I->second.resourceIDs.empty())
		{
			file << "#. Resource IDs: (";

			auto II = I->second.resourceIDs.begin();
			file << CUnicodeUtils::StdGetUTF8(*II);
			++II;
			while (II != I->second.resourceIDs.end())
			{
				file << ", ";
				file << CUnicodeUtils::StdGetUTF8(*II);
				++II;
			};
			file << ")\n";
		}
		if (I->second.flag.length() > 0)
			file << CUnicodeUtils::StdGetUTF8(I->second.flag) << "\n";
		file << ("msgid \"") << CUnicodeUtils::StdGetUTF8(I->first) << "\"\n";
		file << ("msgstr \"") << CUnicodeUtils::StdGetUTF8(I->second.msgstr) << "\"\n\n";
		nEntries++;
	}
	file.close();
	if (!m_bQuiet)
		fwprintf(stdout, L"File %s saved, containing %d entries\n", szPath, nEntries);
	return TRUE;
}

void CPOFile::AdjustEOLs(std::wstring& str)
{
	std::wstring result;
	std::wstring::size_type pos = 0;
	for ( ; ; ) // while (true)
	{
		std::wstring::size_type next = str.find(L"\\r\\n", pos);
		result.append(str, pos, next-pos);
		if( next != std::string::npos )
		{
			result.append(L"\\n");
			pos = next + 4; // 4 = sizeof("\\r\\n")
		}
		else
		{
			break;  // exit loop
		}
	}
	str.swap(result);
	result.clear();
	pos = 0;

	for ( ; ; ) // while (true)
	{
		std::wstring::size_type next = str.find(L"\\n", pos);
		result.append(str, pos, next-pos);
		if( next != std::string::npos )
		{
			result.append(L"\\r\\n");
			pos = next + 2; // 2 = sizeof("\\n")
		}
		else
		{
			break;  // exit loop
		}
	}
	str.swap(result);
}
