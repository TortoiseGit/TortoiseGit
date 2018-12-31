// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseGit
// Copyright (C) 2000 - Francis Irving
// <francis@flourish.org> - May 2000

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <windows.h>

#define MAX_LENGTH_PASSWORD 256

#ifdef __cplusplus
extern "C"
{
#endif
	BOOL DoLoginDialog(char* password, int maxlen, const char* prompt);

	HWND GetParentHwnd();

#ifdef __cplusplus
}
#endif

#endif
