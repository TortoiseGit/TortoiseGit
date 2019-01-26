// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
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
#include "Debug.h"
#include <tchar.h>

#if defined(_DEBUG) || defined(DEBUG)
#include <stdlib.h>
#include <stdarg.h>
void TRACE(LPCTSTR str, ...)
{
	static TCHAR buf[20*1024];

	va_list ap;
	va_start(ap, str);

	_vstprintf_s(buf, str, ap);
	OutputDebugString(buf);
	va_end(ap);
};
#else
void TRACE(LPCTSTR str, ...) {UNREFERENCED_PARAMETER(str);}
#endif
