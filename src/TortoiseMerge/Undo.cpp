// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2006-2007, 2010-2011, 2013,2015, 2021-2022 - TortoiseSVN

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
#include "Undo.h"

#include "BaseView.h"

void viewstate::AddViewLineFromView(CBaseView *pView, int nViewLine, bool bAddEmptyLine)
{
	// is undo good place for this ?
	if (!pView || !pView->m_pViewData)
		return;
	replacedlines[nViewLine] = pView->m_pViewData->GetData(nViewLine);
	if (bAddEmptyLine)
	{
		addedlines.push_back(nViewLine + 1);
		pView->AddEmptyViewLine(nViewLine);
	}
}

void viewstate::Clear()
{
	difflines.clear();
	linestates.clear();
	linelines.clear();
	linesEOL.clear();
	markedlines.clear();
	addedlines.clear();

	removedlines.clear();
	replacedlines.clear();
	modifies = false;
}

void CUndo::MarkAsOriginalState(bool bLeft, bool bRight, bool bBottom)
{
	// find highest index of changing step
	if (bLeft) // left is selected for mark
		m_originalstateLeft = 0;
	if (bRight) // right is selected for mark
		m_originalstateRight = 0;
	if (bBottom) // bottom is selected for mark
		m_originalstateBottom = 0;
}

CUndo& CUndo::GetInstance()
{
	static CUndo instance;
	return instance;
}

CUndo::CUndo()
{
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const allviewstate& allstate, POINT pt)
{
	if (allstate.left.modifies)
		++m_originalstateLeft;
	if (allstate.right.modifies)
		++m_originalstateRight;
	if (allstate.bottom.modifies)
		++m_originalstateBottom;

	m_viewstates.push_back(allstate);
	m_caretpoints.push_back(pt);
	// a new action that can be undone clears the redo since
	// after this there is nothing to redo anymore
	m_redoviewstates.clear();
	m_redocaretpoints.clear();
	m_redogroups.clear();
}

bool CUndo::Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	if (!CanUndo())
		return false;

	if (m_groups.size() && m_groups.back() == m_caretpoints.size())
	{
		m_groups.pop_back();
		std::list<int>::size_type b = m_groups.back();
		m_redogroups.push_back(b);
		m_redogroups.push_back(m_caretpoints.size());
		m_groups.pop_back();
		while (b < m_caretpoints.size())
			UndoOne(pLeft, pRight, pBottom);
	}
	else
		UndoOne(pLeft, pRight, pBottom);

	updateActiveView(pLeft, pRight, pBottom);

	return true;
}

void CUndo::UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	allviewstate allstate = m_viewstates.back();
	POINT pt = m_caretpoints.back();

	if (pLeft->IsTarget())
		m_redocaretpoints.push_back(pLeft->GetCaretPosition());
	else if (pRight->IsTarget())
		m_redocaretpoints.push_back(pRight->GetCaretPosition());
	else if (pBottom->IsTarget())
		m_redocaretpoints.push_back(pBottom->GetCaretPosition());

	if (allstate.left.modifies)
		--m_originalstateLeft;
	if (allstate.right.modifies)
		--m_originalstateRight;
	if (allstate.bottom.modifies)
		--m_originalstateBottom;

	allstate.left   = Do(allstate.left, pLeft, pt);
	allstate.right  = Do(allstate.right, pRight, pt);
	allstate.bottom = Do(allstate.bottom, pBottom, pt);

	m_redoviewstates.push_back(allstate);

	m_viewstates.pop_back();
	m_caretpoints.pop_back();
}

bool CUndo::Redo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	if (!CanRedo())
		return false;

	if (m_redogroups.size() && m_redogroups.back() == m_redocaretpoints.size())
	{
		m_redogroups.pop_back();
		std::list<int>::size_type b = m_redogroups.back();
		m_groups.push_back(b);
		m_groups.push_back(m_redocaretpoints.size());
		m_redogroups.pop_back();
		while (b < m_redocaretpoints.size())
			RedoOne(pLeft, pRight, pBottom);
	}
	else
		RedoOne(pLeft, pRight, pBottom);

	updateActiveView(pLeft, pRight, pBottom);

	return true;
}

void CUndo::RedoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom)
{
	allviewstate allstate = m_redoviewstates.back();
	POINT pt = m_redocaretpoints.back();

	if (pLeft->IsTarget())
		m_caretpoints.push_back(pLeft->GetCaretPosition());
	else if (pRight->IsTarget())
		m_caretpoints.push_back(pRight->GetCaretPosition());
	else if (pBottom->IsTarget())
		m_caretpoints.push_back(pBottom->GetCaretPosition());

	if (allstate.left.modifies)
		++m_originalstateLeft;
	if (allstate.right.modifies)
		++m_originalstateRight;
	if (allstate.bottom.modifies)
		++m_originalstateBottom;

	allstate.left = Do(allstate.left, pLeft, pt);
	allstate.right = Do(allstate.right, pRight, pt);
	allstate.bottom = Do(allstate.bottom, pBottom, pt);

	m_viewstates.push_back(allstate);

	m_redoviewstates.pop_back();
	m_redocaretpoints.pop_back();
}

void CUndo::updateActiveView(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom) const
{
	CBaseView* pActiveView = nullptr;

	if (pBottom && pBottom->IsTarget())
	{
		pActiveView = pBottom;
	}
	else if (pRight && pRight->IsTarget())
	{
		pActiveView = pRight;
	}
	else
	// if (pLeft && pLeft->IsTarget())
	{
		pActiveView = pLeft;
	}

	if (pActiveView)
	{
		pActiveView->ClearSelection();
		pActiveView->BuildAllScreen2ViewVector();
		pActiveView->RecalcAllVertScrollBars();
		pActiveView->RecalcAllHorzScrollBars();
		pActiveView->EnsureCaretVisible();
		pActiveView->UpdateCaret();

		if (pLeft)
		{
			pLeft->SetModified(m_originalstateLeft != 0);
			pLeft->ClearStepModifiedMark();
		}
		if (pRight)
		{
			pRight->SetModified(m_originalstateRight != 0);
			pRight->ClearStepModifiedMark();
		}
		if (pBottom)
		{
			pBottom->SetModified(m_originalstateBottom != 0);
			pBottom->ClearStepModifiedMark();
		}
		pActiveView->RefreshViews();
	}
}
viewstate CUndo::Do(const viewstate& state, CBaseView * pView, const POINT& pt)
{
	if (!pView)
		return state;

	CViewData* viewData = pView->m_pViewData;
	if (!viewData)
		return state;

	viewstate revstate; // the reversed viewstate
	revstate.modifies = state.modifies;

	for (std::list<int>::const_reverse_iterator it = state.addedlines.rbegin(); it != state.addedlines.rend(); ++it)
	{
		revstate.removedlines[*it] = viewData->GetData(*it);
		viewData->RemoveData(*it);
	}
	for (std::map<int, DWORD>::const_iterator it = state.linelines.begin(); it != state.linelines.end(); ++it)
	{
		revstate.linelines[it->first] = viewData->GetLineNumber(it->first);
		viewData->SetLineNumber(it->first, it->second);
	}
	for (auto it = state.linestates.cbegin(); it != state.linestates.cend(); ++it)
	{
		revstate.linestates[it->first] = viewData->GetState(it->first);
		viewData->SetState(it->first, it->second);
	}
	for (std::map<int, EOL>::const_iterator it = state.linesEOL.begin(); it != state.linesEOL.end(); ++it)
	{
		revstate.linesEOL[it->first] = viewData->GetLineEnding(it->first);
		viewData->SetLineEnding(it->first, it->second);
	}
	for (std::map<int, bool>::const_iterator it = state.markedlines.begin(); it != state.markedlines.end(); ++it)
	{
		revstate.markedlines[it->first] = viewData->GetMarked(it->first);
		viewData->SetMarked(it->first, it->second);
	}
	for (std::map<int, CString>::const_iterator it = state.difflines.begin(); it != state.difflines.end(); ++it)
	{
		revstate.difflines[it->first] = viewData->GetLine(it->first);
		viewData->SetLine(it->first, it->second);
	}
	for (std::map<int, viewdata>::const_iterator it = state.removedlines.begin(); it != state.removedlines.end(); ++it)
	{
		revstate.addedlines.push_back(it->first);
		viewData->InsertData(it->first, it->second);
	}
	for (std::map<int, viewdata>::const_iterator it = state.replacedlines.begin(); it != state.replacedlines.end(); ++it)
	{
		revstate.replacedlines[it->first] = viewData->GetData(it->first);
		viewData->SetData(it->first, it->second);
	}

	if (pView->IsTarget())
	{
		pView->SetCaretViewPosition(pt);
		pView->EnsureCaretVisible();
	}
	return revstate;
}

void CUndo::Clear()
{
	m_viewstates.clear();
	m_caretpoints.clear();
	m_groups.clear();
	m_redoviewstates.clear();
	m_redocaretpoints.clear();
	m_redogroups.clear();
	m_originalstateLeft = 0;
	m_originalstateRight = 0;
	m_originalstateBottom = 0;
	m_groupCount = 0;
}
