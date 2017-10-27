// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseGit
// Copyright (C) 2003-2008, 2011-2016 - TortoiseSVN

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

CPOFile::~CPOFile(void)
{
}

static bool StartsWith(const wchar_t* heystacl, const wchar_t* needle)
{
	return wcsncmp(heystacl, needle, wcslen(needle)) == 0;
}

BOOL CPOFile::ParseFile(LPCTSTR szPath, BOOL bUpdateExisting, bool bAdjustEOLs)
{
	if (!PathFileExists(szPath))
		return FALSE;

	m_bAdjustEOLs = bAdjustEOLs;

	if (!m_bQuiet)
		_ftprintf(stdout, L"parsing file %s...\n", szPath);

	int nEntries = 0;
	int nDeleted = 0;
	int nTranslated = 0;
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH + 1] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, szPath, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);

	std::wifstream File;
	File.imbue(std::locale(std::locale(), new utf8_conversion()));
	File.open(filepath);
	if (!File.good())
	{
		_ftprintf(stderr, L"can't open input file %s\n", szPath);
		return FALSE;
	}
	auto line = std::make_unique<TCHAR[]>(2 * MAX_STRING_LENGTH);
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
			entry.push_back(line.get());
		}
	} while (File.gcount() > 0);
	printf("%s", File.getloc().name().c_str());
	File.close();
	RESOURCEENTRY emptyentry = {0};
	(*this)[std::wstring(L"")] = emptyentry;
	if (!m_bQuiet)
		_ftprintf(stdout, L"%d Entries found, %d were already translated and %d got deleted\n", nEntries, nTranslated, nDeleted);
	return TRUE;
}

BOOL CPOFile::SaveFile(LPCTSTR szPath, LPCTSTR lpszHeaderFile)
{
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH + 1] = { 0 };
	int nEntries = 0;
	WideCharToMultiByte(CP_ACP, 0, szPath, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);

	std::wofstream File;
	File.imbue(std::locale(std::locale(), new utf8_conversion()));
	File.open(filepath, std::ios_base::binary);

	if ((lpszHeaderFile)&&(lpszHeaderFile[0])&&(PathFileExists(lpszHeaderFile)))
	{
		// read the header file and save it to the top of the pot file
		std::wifstream inFile;
		inFile.imbue(std::locale(std::locale(), new utf8_conversion()));
		inFile.open(lpszHeaderFile, std::ios_base::binary);

		wchar_t ch;
		while(inFile && inFile.get(ch))
			File.put(ch);
		inFile.close();
	}
	else
	{
		File << L"# SOME DESCRIPTIVE TITLE.\n";
		File << L"# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER\n";
		File << L"# This file is distributed under the same license as the PACKAGE package.\n";
		File << L"# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n";
		File << L"#\n";
		File << L"#, fuzzy\n";
		File << L"msgid \"\"\n";
		File << L"msgstr \"\"\n";
		File << L"\"Project-Id-Version: PACKAGE VERSION\\n\"\n";
		File << L"\"Report-Msgid-Bugs-To: \\n\"\n";
		File << L"\"POT-Creation-Date: 1900-01-01 00:00+0000\\n\"\n";
		File << L"\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
		File << L"\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n";
		File << L"\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n";
		File << L"\"MIME-Version: 1.0\\n\"\n";
		File << L"\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
		File << L"\"Content-Transfer-Encoding: 8bit\\n\"\n\n";
	}
	File << L"\n";
	File << L"# msgid/msgstr fields for Accelerator keys\n";
	File << L"# Format is: \"ID:xxxxxx:VACS+X\" where:\n";
	File << L"#    ID:xxxxx = the menu ID corresponding to the accelerator\n";
	File << L"#    V = Virtual key (or blank if not used) - nearly always set!\n";
	File << L"#    A = Alt key     (or blank if not used)\n";
	File << L"#    C = Ctrl key    (or blank if not used)\n";
	File << L"#    S = Shift key   (or blank if not used)\n";
	File << L"#    X = upper case character\n";
	File << L"# e.g. \"V CS+Q\" == Ctrl + Shift + 'Q'\n";
	File << L"\n";
	File << L"# ONLY Accelerator Keys with corresponding alphanumeric characters can be\n";
	File << L"# updated i.e. function keys (F2), special keys (Delete, HoMe) etc. will not.\n";
	File << L"\n";
	File << L"# ONLY change the msgstr field. Do NOT change any other.\n";
	File << L"# If you do not want to change an Accelerator Key, copy msgid to msgstr\n";
	File << L"\n";

	for (auto I = this->cbegin(); I != this->cend(); ++I)
	{
		std::wstring s = I->first;
		s.erase(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](const auto& c) { return !iswspace(c); }));
		if (s.empty())
			continue;

		RESOURCEENTRY entry = I->second;
		for (auto II = entry.automaticcomments.cbegin(); II != entry.automaticcomments.cend(); ++II)
		{
			File << II->c_str() << L"\n";
		}
		for (auto II = entry.translatorcomments.cbegin(); II != entry.translatorcomments.cend(); ++II)
		{
			File << II->c_str() << L"\n";
		}
		if (!I->second.resourceIDs.empty())
		{
			File << L"#. Resource IDs: (";

			auto II = I->second.resourceIDs.begin();
			File << (*II);
			++II;
			while (II != I->second.resourceIDs.end())
			{
				File << L", ";
				File << (*II);
				++II;
			};
			File << L")\n";
		}
		if (I->second.flag.length() > 0)
			File << (I->second.flag.c_str()) << L"\n";
		File << (L"msgid \"") << (I->first.c_str()) << L"\"\n";
		File << (L"msgstr \"") << (I->second.msgstr.c_str()) << L"\"\n\n";
		nEntries++;
	}
	File.close();
	if (!m_bQuiet)
		_ftprintf(stdout, L"File %s saved, containing %d entries\n", szPath, nEntries);
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
