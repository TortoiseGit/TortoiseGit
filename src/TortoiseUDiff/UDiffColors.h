// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014, 2020 - TortoiseSVN

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

#define UDIFF_COLORFORECOMMAND  RGB(0x0A, 0x24, 0x36)
#define UDIFF_COLORFOREPOSITION RGB(0xFF, 0, 0)
#define UDIFF_COLORFOREHEADER   RGB(0x80, 0, 0)
#define UDIFF_COLORFORECOMMENT  RGB(0, 0x80, 0)
#define UDIFF_COLORFOREADDED    ::GetSysColor(COLOR_WINDOWTEXT)
#define UDIFF_COLORFOREREMOVED  ::GetSysColor(COLOR_WINDOWTEXT)

#define UDIFF_COLORBACKCOMMAND  ::GetSysColor(COLOR_WINDOW)
#define UDIFF_COLORBACKPOSITION ::GetSysColor(COLOR_WINDOW)
#define UDIFF_COLORBACKHEADER   RGB(0xFF, 0xFF, 0x80)
#define UDIFF_COLORBACKCOMMENT  ::GetSysColor(COLOR_WINDOW)
#define UDIFF_COLORBACKADDED    RGB(0xCC, 0xFF, 0xCC)
#define UDIFF_COLORBACKREMOVED  RGB(0xFF, 0xDD, 0xDD)

const COLORREF UDiffTextColorDark = 0xDDDDDD;
const COLORREF UDiffBackColorDark = 0x202020; // cf. Theme.h

#define UDIFF_COLORFORECOMMAND_DARK RGB(201,226,245)
#define UDIFF_COLORFOREPOSITION_DARK RGB(0xFF, 0x20, 0x20)
#define UDIFF_COLORFOREHEADER_DARK RGB(0xC0, 0, 0)
#define UDIFF_COLORFORECOMMENT_DARK RGB(0, 0x80, 0)
#define UDIFF_COLORFOREADDED_DARK UDiffTextColorDark
#define UDIFF_COLORFOREREMOVED_DARK UDiffTextColorDark

#define UDIFF_COLORBACKCOMMAND_DARK UDiffBackColorDark
#define UDIFF_COLORBACKPOSITION_DARK UDiffBackColorDark
#define UDIFF_COLORBACKHEADER_DARK RGB(0x30, 0x30, 0x00)
#define UDIFF_COLORBACKCOMMENT_DARK UDiffBackColorDark
#define UDIFF_COLORBACKADDED_DARK RGB(0x10, 0x40, 0x10)
#define UDIFF_COLORBACKREMOVED_DARK RGB(0x40, 0x20, 0x20)
