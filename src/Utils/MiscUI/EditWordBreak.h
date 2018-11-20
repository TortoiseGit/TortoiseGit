// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseSVN

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
 * \ingroup Utils
 * Implements an edit control word break procedure which
 * also breaks at forward slashes.
 * use
 * \code
 * SendMessage(hEdit, EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(&UrlWordBreakProc));
 * \endcode
 * to initialize the edit control
 */
int CALLBACK UrlWordBreakProc(LPCWSTR edit_text, int current_pos, int length, int action);

 /**
  * Goes through the whole window and sets the UrlWordBreakProc to all
  * child edit controls. If \b includeComboboxes is true, then
  * the edit controls of comboboxes have the UrlWordBreakProc set too
  * \return the number of controls for which the WordBreakProc has been set
  */
int SetUrlWordBreakProcToChildWindows(HWND hParent, bool includeComboboxes);
