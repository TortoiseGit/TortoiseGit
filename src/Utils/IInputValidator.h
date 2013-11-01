// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010, 2013 - TortoiseSVN

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

class IInputValidator
{
public:
	/**
	 * Method is called to validate the user input, usually an edit control.
	 * Return an empty string if the input is valid. If the input is not valid,
	 * return an error string which is then shown in an error balloon.
	 * \param nID the id of the edit control, or 0 if it's not for a specific control
	 * \param input the user input
	 */
	virtual CString Validate(const int nID, const CString& input) = 0;
};
