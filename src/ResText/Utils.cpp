// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2003-2006, 2012-2014 - TortoiseSVN

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
#include "Utils.h"
#include "FormatMessageWrapper.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

void CUtils::StringExtend(LPTSTR str)
{
	TCHAR * cPos = str;
	do
	{
		cPos = wcschr(cPos, '\\');
		if (cPos)
		{
			memmove(cPos + 1, cPos, (wcslen(cPos) + 1) * sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = '\\';
			cPos++;
			cPos++;
		}
	} while (cPos);
	cPos = str;
	do
	{
		cPos = wcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos + 1, cPos, (wcslen(cPos) + 1) * sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'n';
		}
	} while (cPos);
	cPos = str;
	do
	{
		cPos = wcschr(cPos, '\r');
		if (cPos)
		{
			memmove(cPos + 1, cPos, (wcslen(cPos) + 1) * sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'r';
		}
	} while (cPos);
	cPos = str;
	do
	{
		cPos = wcschr(cPos, '\t');
		if (cPos)
		{
			memmove(cPos + 1, cPos, (wcslen(cPos) + 1) * sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 't';
		}
	} while (cPos);
	cPos = str;
	do
	{
		cPos = wcschr(cPos, '"');
		if (cPos)
		{
			memmove(cPos + 1, cPos, (wcslen(cPos) + 1) * sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = '"';
			cPos++;
			cPos++;
		}
	} while (cPos);
}

void CUtils::StringCollapse(LPTSTR str)
{
	TCHAR * cPos = str;
	do
	{
		cPos = wcschr(cPos, '\\');
		if (cPos)
		{
			if (*(cPos+1) == 'n')
			{
				*cPos = '\n';
				memmove(cPos+1, cPos+2, (wcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 'r')
			{
				*cPos = '\r';
				memmove(cPos+1, cPos+2, (wcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 't')
			{
				*cPos = '\t';
				memmove(cPos+1, cPos+2, (wcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == '"')
			{
				*cPos = '"';
				memmove(cPos+1, cPos+2, (wcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == '\\')
			{
				*cPos = '\\';
				memmove(cPos+1, cPos+2, (wcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			cPos++;
		}
	} while (cPos);
}

void CUtils::Error()
{
	CFormatMessageWrapper errorDetails;
	if (errorDetails)
		_ftprintf (stderr, L"%s\n", static_cast<LPCTSTR>(errorDetails));
}

void CUtils::SearchReplace(std::wstring& str, const std::wstring& toreplace, const std::wstring& replacewith)
{
	std::wstring result;
	std::wstring::size_type pos = 0;
	for ( ; ; ) // while (true)
	{
		std::wstring::size_type next = str.find(toreplace, pos);
		result.append(str, pos, next-pos);
		if( next != std::string::npos )
		{
			result.append(replacewith);
			pos = next + toreplace.size();
		}
		else
		{
			break;  // exit loop
		}
	}
	str.swap(result);
}
