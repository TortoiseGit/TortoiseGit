// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2007-2008, 2010, 2013-2014, 2017, 2020 - TortoiseSVN

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
#include "DiffColors.h"


CDiffColors& CDiffColors::GetInstance()
{
	static CDiffColors instance;
	return instance;
}


CDiffColors::CDiffColors(void)
{
	LoadRegistry();
}

CDiffColors::~CDiffColors(void)
{
}

void CDiffColors::GetColors(DiffStates state, bool darkMode, COLORREF& crBkgnd, COLORREF& crText)
{
	if ((state < DIFFSTATE_END) && (state >= 0))
	{
		if (darkMode)
		{
			crBkgnd = static_cast<DWORD>(m_regDarkBackgroundColors[static_cast<int>(state)]);
			crText = static_cast<DWORD>(m_regDarkForegroundColors[static_cast<int>(state)]);
		}
		else
		{
			crBkgnd = static_cast<DWORD>(m_regBackgroundColors[static_cast<int>(state)]);
			crText = static_cast<DWORD>(m_regForegroundColors[static_cast<int>(state)]);
		}
	}
	else
	{
		if (darkMode)
		{
			crBkgnd = CTheme::darkBkColor;
			crText = CTheme::darkTextColor;
		}
		else
		{
			crBkgnd = ::GetSysColor(COLOR_WINDOW);
			crText = ::GetSysColor(COLOR_WINDOWTEXT);
		}
	}
}

void CDiffColors::SetColors(DiffStates state, bool darkMode, const COLORREF& crBkgnd, const COLORREF& crText)
{
	if ((state < DIFFSTATE_END) && (state >= 0))
	{
		if (darkMode)
		{
			m_regDarkBackgroundColors[static_cast<int>(state)] = crBkgnd;
			m_regDarkForegroundColors[static_cast<int>(state)] = crText;
		}
		else
		{
			m_regBackgroundColors[static_cast<int>(state)] = crBkgnd;
			m_regForegroundColors[static_cast<int>(state)] = crText;
		}
	}
}

void CDiffColors::LoadRegistry()
{
	m_regForegroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorUnknownF", DIFFSTATE_UNKNOWN_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_NORMAL] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorNormalF", DIFFSTATE_NORMAL_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorRemovedF", DIFFSTATE_REMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorRemovedWhitespaceF", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorAddedF", DIFFSTATE_ADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorAddedWhitespaceF", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceF", DIFFSTATE_WHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceDiffF", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_EMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorEmptyF", DIFFSTATE_EMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedF", DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedIgnoredF", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedAddedF", DIFFSTATE_CONFLICTADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedEmptyF", DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ConflictResolvedF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ConflictResolvedEmptyF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorIdenticalRemovedF", DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorIdenticalAddedF", DIFFSTATE_IDENTICALADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorTheirsRemovedF", DIFFSTATE_THEIRSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorTheirsAddedF", DIFFSTATE_THEIRSADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorYoursRemovedF", DIFFSTATE_YOURSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorYoursAddedF", DIFFSTATE_YOURSADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_EDITED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorEditedF", DIFFSTATE_EDITED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_FILTEREDDIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorFilteredF", DIFFSTATE_EDITED_DEFAULT_FG);

	m_regBackgroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorUnknownB", DIFFSTATE_UNKNOWN_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_NORMAL] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorNormalB", DIFFSTATE_NORMAL_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorRemovedB", DIFFSTATE_REMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorRemovedWhitespaceB", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorAddedB", DIFFSTATE_ADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorAddedWhitespaceB", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceB", DIFFSTATE_WHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceDiffB", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_EMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorEmptyB", DIFFSTATE_EMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedB", DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedIgnoredB", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedAddedB", DIFFSTATE_CONFLICTADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorConflictedEmptyB", DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ConflictResolvedB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ConflictResolvedEmptyB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorIdenticalRemovedB", DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorIdenticalAddedB", DIFFSTATE_IDENTICALADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorTheirsRemovedB", DIFFSTATE_THEIRSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorTheirsAddedB", DIFFSTATE_THEIRSADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorYoursRemovedB", DIFFSTATE_YOURSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorYoursAddedB", DIFFSTATE_YOURSADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_EDITED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorEditedB", DIFFSTATE_EDITED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_FILTEREDDIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\ColorFilteredB", DIFFSTATE_FILTERED_DEFAULT_BG);

	m_regDarkForegroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorUnknownF", DIFFSTATE_UNKNOWN_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_NORMAL] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorNormalF", DIFFSTATE_NORMAL_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_REMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorRemovedF", DIFFSTATE_REMOVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorRemovedWhitespaceF", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_ADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorAddedF", DIFFSTATE_ADDED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorAddedWhitespaceF", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorWhitespaceF", DIFFSTATE_WHITESPACE_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorWhitespaceDiffF", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_EMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorEmptyF", DIFFSTATE_EMPTY_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedF", DIFFSTATE_CONFLICTED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedIgnoredF", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedAddedF", DIFFSTATE_CONFLICTADDED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedEmptyF", DIFFSTATE_CONFLICTEMPTY_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkConflictResolvedF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkConflictResolvedEmptyF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorIdenticalRemovedF", DIFFSTATE_IDENTICALREMOVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorIdenticalAddedF", DIFFSTATE_IDENTICALADDED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorTheirsRemovedF", DIFFSTATE_THEIRSREMOVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorTheirsAddedF", DIFFSTATE_THEIRSADDED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorYoursRemovedF", DIFFSTATE_YOURSREMOVED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorYoursAddedF", DIFFSTATE_YOURSADDED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_EDITED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorEditedF", DIFFSTATE_EDITED_DEFAULT_DARK_FG);
	m_regDarkForegroundColors[DIFFSTATE_FILTEREDDIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorFilteredF", DIFFSTATE_EDITED_DEFAULT_DARK_FG);

	m_regDarkBackgroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorUnknownB", DIFFSTATE_UNKNOWN_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_NORMAL] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorNormalB", DIFFSTATE_NORMAL_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_REMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorRemovedB", DIFFSTATE_REMOVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorRemovedWhitespaceB", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_ADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorAddedB", DIFFSTATE_ADDED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorAddedWhitespaceB", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorWhitespaceB", DIFFSTATE_WHITESPACE_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorWhitespaceDiffB", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_EMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorEmptyB", DIFFSTATE_EMPTY_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedB", DIFFSTATE_CONFLICTED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedIgnoredB", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedAddedB", DIFFSTATE_CONFLICTADDED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorConflictedEmptyB", DIFFSTATE_CONFLICTEMPTY_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkConflictResolvedB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkConflictResolvedEmptyB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorIdenticalRemovedB", DIFFSTATE_IDENTICALREMOVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorIdenticalAddedB", DIFFSTATE_IDENTICALADDED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorTheirsRemovedB", DIFFSTATE_THEIRSREMOVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorTheirsAddedB", DIFFSTATE_THEIRSADDED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorYoursRemovedB", DIFFSTATE_YOURSREMOVED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorYoursAddedB", DIFFSTATE_YOURSADDED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_EDITED] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorEditedB", DIFFSTATE_EDITED_DEFAULT_DARK_BG);
	m_regDarkBackgroundColors[DIFFSTATE_FILTEREDDIFF] = CRegDWORD(L"Software\\TortoiseGitMerge\\Colors\\DarkColorFilteredB", DIFFSTATE_FILTERED_DEFAULT_DARK_BG);

	for (int i=0; i<DIFFSTATE_END; ++i)
	{
		m_regForegroundColors[i].read();
		m_regBackgroundColors[i].read();
		m_regDarkForegroundColors[i].read();
		m_regDarkBackgroundColors[i].read();
	}
}

