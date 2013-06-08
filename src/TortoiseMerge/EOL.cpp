// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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
#include "EOL.h"


const wchar_t * GetEolName(EOL eEol)
{
	switch(eEol)
	{
	case EOL_LF:
		return L"LF";
	case EOL_CRLF:
		return L"CRLF";
	case EOL_LFCR:
		return L"LFCR";
	case EOL_CR:
		return L"CR";
	case EOL_VT:
		return L"VT";
	case EOL_FF:
		return L"FF";
	case EOL_NEL:
		return L"NEL";
	case EOL_LS:
		return L"LS";
	case EOL_PS:
		return L"PS";
	case EOL_AUTOLINE:
		return L"AEOL";
	}
	return L"";
}
