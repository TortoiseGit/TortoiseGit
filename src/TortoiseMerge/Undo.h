// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2007,2009-2015 - TortoiseSVN
// Copyright (C) 2011 Sven Strickroth <email@cs-ware.de>

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
#include "ViewData.h"
#include <map>
#include <list>

class CBaseView;

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge.
 */
class viewstate
{
public:
	viewstate()
	: modifies(false)
	{}

	std::map<int, CString>	difflines;
	std::map<int, DWORD>	linestates;
	std::map<int, DWORD>	linelines;
	std::map<int, EOL>		linesEOL;
	std::map<int, bool>		markedlines;
	std::list<int>			addedlines;

	std::map<int, viewdata> removedlines;
	std::map<int, viewdata> replacedlines;
	bool					modifies; ///< this step modifies view (save before and after save differs)

	void	AddViewLineFromView(CBaseView *pView, int nViewLine, bool bAddEmptyLine);
	void	Clear();
	bool	IsEmpty() const { return difflines.empty() && linestates.empty() && linelines.empty() && linesEOL.empty() && markedlines.empty() && addedlines.empty() && removedlines.empty() && replacedlines.empty(); }
};

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge for all(3) views.
 */
struct allviewstate
{
	viewstate right;
	viewstate bottom;
	viewstate left;

	void	Clear() { right.Clear(); bottom.Clear(); left.Clear(); }
	bool	IsEmpty() const { return right.IsEmpty() && bottom.IsEmpty() && left.IsEmpty(); }
};

/**
 * \ingroup TortoiseMerge
 * Holds all the information of previous changes made to a view content.
 * Of course, can undo those changes.
 */
class CUndo
{
public:
	static CUndo& GetInstance();

	bool Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	bool Redo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	void AddState(const allviewstate& allstate, POINT pt);
	bool CanUndo() const {return !m_viewstates.empty();}
	bool CanRedo() const { return !m_redoviewstates.empty(); }

	bool IsGrouping() const { return m_groups.size() % 2 == 1; }
	bool IsRedoGrouping() const { return m_redogroups.size() % 2 == 1; }
	void BeginGrouping() { if (m_groupCount==0) m_groups.push_back(m_caretpoints.size()); m_groupCount++; }
	void EndGrouping(){ m_groupCount--; if (m_groupCount==0) m_groups.push_back(m_caretpoints.size()); }
	void Clear();
	void MarkAllAsOriginalState() { MarkAsOriginalState(true, true, true); }
	void MarkAsOriginalState(bool Left, bool Right, bool Bottom);
protected:
	viewstate Do(const viewstate& state, CBaseView * pView, const POINT& pt);
	void UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	void RedoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	std::list<allviewstate> m_viewstates;
	std::list<POINT> m_caretpoints;
	std::list< std::list<int>::size_type > m_groups;
	size_t m_originalstateLeft;
	size_t m_originalstateRight;
	size_t m_originalstateBottom;
	int m_groupCount;

	std::list<allviewstate> m_redoviewstates;
	std::list<POINT> m_redocaretpoints;
	std::list< std::list<int>::size_type > m_redogroups;

private:
	CUndo();
	~CUndo();
};
