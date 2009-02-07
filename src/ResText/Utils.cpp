// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "StdAfx.h"
#include ".\utils.h"

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
		cPos = _tcschr(cPos, '\\');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = '\\';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'n';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\r');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'r';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\t');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 't';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '"');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = '"';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}

void CUtils::StringCollapse(LPTSTR str)
{
	TCHAR * cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\\');
		if (cPos)
		{
			if (*(cPos+1) == 'n')
			{
				*cPos = '\n';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 'r')
			{
				*cPos = '\r';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 't')
			{
				*cPos = '\t';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == '"')
			{
				*cPos = '"';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == '\\')
			{
				*cPos = '\\';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			cPos++;
		}
	} while (cPos != NULL);
}

void CUtils::Error()
{
	LPVOID lpMsgBuf;
	if (!FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL ))
	{
		return;
	}

	// Display the string.
	_ftprintf(stderr, _T("%s\n"), (LPCTSTR)lpMsgBuf);

	// Free the buffer.
	LocalFree( lpMsgBuf );
}