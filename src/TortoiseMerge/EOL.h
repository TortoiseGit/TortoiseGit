// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2007, 2013 - TortoiseSVN

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

/**
 * \ingroup TortoiseMerge
 * the different EOL styles a line can have.
 */
enum EOL
{
	EOL_AUTOLINE,
	// MS native
	EOL_CRLF,  ///< CR (U+000D) followed by LF (U+000A)
	// foregin
	EOL_LF,    ///< Line Feed, U+000A
	EOL_CR,    ///< Carriage Return, U+000D
	// exotic - diff needs conversion
	EOL_LFCR,
	EOL_VT,    ///< Vertical Tab, U+000B
	EOL_FF,    ///< Form Feed, U+000C
	EOL_NEL,   ///< Next Line, U+0085
	EOL_LS,    ///< Line Separator, U+2028
	EOL_PS,    ///< Paragraph Separator, U+2029
	EOL_NOENDING,

	EOL__COUNT
};

extern const wchar_t * GetEolName(EOL eEol);
