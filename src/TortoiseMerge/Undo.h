// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN
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
typedef struct viewstate
{
	std::map<int, CString> difflines;
	std::map<int, DWORD> linestates;
	std::map<int, DWORD> linelines;
	std::map<int, EOL> linesEOL;
	std::list<int> addedlines;

	std::map<int, viewdata> removedlines;

	void	AddLineFormView(CBaseView *pView, int nLine, bool bAddEmptyLine);
} viewstate;

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
	void AddState(const viewstate& leftstate, const viewstate& rightstate, const viewstate& bottomstate, POINT pt);
	bool CanUndo() { return (!m_viewstates.empty()); }

	bool IsGrouping() { return m_groups.size() % 2 == 1; }
	void BeginGrouping() { ASSERT(!IsGrouping()); m_groups.push_back(m_caretpoints.size()); }
	void EndGrouping(){ ASSERT(IsGrouping()); m_groups.push_back(m_caretpoints.size()); }
	void Clear();
	void MarkAsOriginalState() { m_originalstate = (unsigned int)m_viewstates.size(); }
protected:
	void Undo(const viewstate& state, CBaseView * pView);
	void UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	std::list<viewstate> m_viewstates;
	std::list<POINT> m_caretpoints;
	std::list< std::list<int>::size_type > m_groups;
	unsigned int m_originalstate;
private:
	CUndo();
	~CUndo();
};
