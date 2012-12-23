// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "DiffColors.h"


CDiffColors& CDiffColors::GetInstance()
{
	static CDiffColors instance;
	return instance;
}


CDiffColors::CDiffColors(void)
{
	m_regForegroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorUnknownF"), DIFFSTATE_UNKNOWN_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_NORMAL] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorNormalF"), DIFFSTATE_NORMAL_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorRemovedF"), DIFFSTATE_REMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorRemovedWhitespaceF"), DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorAddedF"), DIFFSTATE_ADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorAddedWhitespaceF"), DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceF"), DIFFSTATE_WHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceDiffF"), DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_EMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorEmptyF"), DIFFSTATE_EMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedF"), DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedIgnoredF"), DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedAddedF"), DIFFSTATE_CONFLICTADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedEmptyF"), DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ConflictResolvedF"), DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ConflictResolvedEmptyF"), DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorIdenticalRemovedF"), DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorIdenticalAddedF"), DIFFSTATE_IDENTICALADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorTheirsRemovedF"), DIFFSTATE_THEIRSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorTheirsAddedF"), DIFFSTATE_THEIRSADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorYoursRemovedF"), DIFFSTATE_YOURSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorYoursAddedF"), DIFFSTATE_YOURSADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_EDITED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorEditedF"), DIFFSTATE_EDITED_DEFAULT_FG);

	m_regBackgroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorUnknownB"), DIFFSTATE_UNKNOWN_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_NORMAL] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorNormalB"), DIFFSTATE_NORMAL_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorRemovedB"), DIFFSTATE_REMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorRemovedWhitespaceB"), DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorAddedB"), DIFFSTATE_ADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorAddedWhitespaceB"), DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceB"), DIFFSTATE_WHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorWhitespaceDiffB"), DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_EMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorEmptyB"), DIFFSTATE_EMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedB"), DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTED_IGNORED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedIgnoredB"), DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedAddedB"), DIFFSTATE_CONFLICTADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorConflictedEmptyB"), DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ConflictResolvedB"), DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ConflictResolvedEmptyB"), DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorIdenticalRemovedB"), DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorIdenticalAddedB"), DIFFSTATE_IDENTICALADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorTheirsRemovedB"), DIFFSTATE_THEIRSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorTheirsAddedB"), DIFFSTATE_THEIRSADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorYoursRemovedB"), DIFFSTATE_YOURSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorYoursAddedB"), DIFFSTATE_YOURSADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_EDITED] = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorEditedB"), DIFFSTATE_EDITED_DEFAULT_BG);
}

CDiffColors::~CDiffColors(void)
{
}

void CDiffColors::GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText)
{
	if ((state < DIFFSTATE_END)&&(state >= 0))
	{
		crBkgnd = (COLORREF)(DWORD)m_regBackgroundColors[(int)state];
		crText = (COLORREF)(DWORD)m_regForegroundColors[(int)state];
	}
	else
	{
		crBkgnd = ::GetSysColor(COLOR_WINDOW);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
	}
}

void CDiffColors::SetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText)
{
	if ((state < DIFFSTATE_END)&&(state >= 0))
	{
		m_regBackgroundColors[(int)state] = crBkgnd;
		m_regForegroundColors[(int)state] = crText;
	}
}

void CDiffColors::LoadRegistry()
{
	for (int i=0; i<DIFFSTATE_END; ++i)
	{
		m_regForegroundColors[i].read();
		m_regBackgroundColors[i].read();
	}
}

