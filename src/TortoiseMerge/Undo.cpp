// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010-2011, 2013,2015 - TortoiseSVN

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
	// TODO reduce code duplication
	// find highest index of changing step
	if (bLeft) // left is selected for mark
	{
		m_originalstateLeft = m_viewstates.size();
		std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
		while (i != m_viewstates.rend() && !i->left.modifies)
		{
			++i;
			--m_originalstateLeft;
		}
	}
	if (bRight) // right is selected for mark
	{
		m_originalstateRight = m_viewstates.size();
		std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
		while (i != m_viewstates.rend() && !i->right.modifies)
		{
			++i;
			--m_originalstateRight;
		}
	}
	if (bBottom) // bottom is selected for mark
	{
		m_originalstateBottom = m_viewstates.size();
		std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
		while (i != m_viewstates.rend() && !i->bottom.modifies)
		{
			++i;
			--m_originalstateBottom;
		}
	}
}

CUndo& CUndo::GetInstance()
{
	static CUndo instance;
	return instance;
}

CUndo::CUndo()
{
	Clear();
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const allviewstate& allstate, POINT pt)
{
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

	CBaseView* pActiveView = nullptr;

	if (pBottom && pBottom->IsTarget())
	{
		pActiveView = pBottom;
	}
	else
	if (pRight && pRight->IsTarget())
	{
		pActiveView = pRight;
	}
	else
	//if (pLeft && pLeft->IsTarget())
	{
		pActiveView = pLeft;
	}


	if (pActiveView) {
		pActiveView->ClearSelection();
		pActiveView->BuildAllScreen2ViewVector();
		pActiveView->RecalcAllVertScrollBars();
		pActiveView->RecalcAllHorzScrollBars();
		pActiveView->EnsureCaretVisible();
		pActiveView->UpdateCaret();

		// TODO reduce code duplication
		if (m_viewstates.size() < m_originalstateLeft)
		{
			// Left can never get back to original state now
			m_originalstateLeft = static_cast<size_t>(-1);
		}
		if (pLeft)
		{
			bool bModified = (m_originalstateLeft == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_viewstates.begin();
				std::advance(i, m_originalstateLeft);
				for (; i!=m_viewstates.end(); ++i)
				{
					if (i->left.modifies)
					{
						bModified = true;
						break;
					}
				}
			}
			pLeft->SetModified(bModified);
			pLeft->ClearStepModifiedMark();
		}
		if (m_viewstates.size() < m_originalstateRight)
		{
			// Right can never get back to original state now
			m_originalstateRight = static_cast<size_t>(-1);
		}
		if (pRight)
		{
			bool bModified = (m_originalstateRight == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_viewstates.begin();
				std::advance(i, m_originalstateRight);
				for (; i!=m_viewstates.end() && !i->right.modifies; ++i) ;
				bModified = i!=m_viewstates.end();
			}
			pRight->SetModified(bModified);
			pRight->ClearStepModifiedMark();
		}
		if (m_viewstates.size() < m_originalstateBottom)
		{
			// Bottom can never get back to original state now
			m_originalstateBottom = static_cast<size_t>(-1);
		}
		if (pBottom)
		{
			bool bModified = (m_originalstateBottom == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_viewstates.begin();
				std::advance(i, m_originalstateBottom);
				for (; i!=m_viewstates.end(); ++i)
				{
					if (i->bottom.modifies)
					{
						bModified = true;
						break;
					}
				}
			}
			pBottom->SetModified(bModified);
			pBottom->ClearStepModifiedMark();
		}
		pActiveView->RefreshViews();
	}

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

	CBaseView* pActiveView = nullptr;

	if (pBottom && pBottom->IsTarget())
	{
		pActiveView = pBottom;
	}
	else
		if (pRight && pRight->IsTarget())
		{
			pActiveView = pRight;
		}
		else
			//if (pLeft && pLeft->IsTarget())
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

		// TODO reduce code duplication
		if (m_redoviewstates.size() < m_originalstateLeft)
		{
			// Left can never get back to original state now
			m_originalstateLeft = static_cast<size_t>(-1);
		}
		if (pLeft)
		{
			bool bModified = (m_originalstateLeft == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_redoviewstates.begin();
				std::advance(i, m_originalstateLeft);
				for (; i != m_redoviewstates.end(); ++i)
				{
					if (i->left.modifies)
					{
						bModified = true;
						break;
					}
				}
			}
			pLeft->SetModified(bModified);
			pLeft->ClearStepModifiedMark();
		}
		if (m_redoviewstates.size() < m_originalstateRight)
		{
			// Right can never get back to original state now
			m_originalstateRight = static_cast<size_t>(-1);
		}
		if (pRight)
		{
			bool bModified = (m_originalstateRight == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_redoviewstates.begin();
				std::advance(i, m_originalstateRight);
				for (; i != m_redoviewstates.end() && !i->right.modifies; ++i);
				bModified = i != m_redoviewstates.end();
			}
			pRight->SetModified(bModified);
			pRight->ClearStepModifiedMark();
		}
		if (m_redoviewstates.size() < m_originalstateBottom)
		{
			// Bottom can never get back to original state now
			m_originalstateBottom = static_cast<size_t>(-1);
		}
		if (pBottom)
		{
			bool bModified = (m_originalstateBottom == static_cast<size_t>(-1));
			if (!bModified)
			{
				std::list<allviewstate>::iterator i = m_redoviewstates.begin();
				std::advance(i, m_originalstateBottom);
				for (; i != m_redoviewstates.end(); ++i)
				{
					if (i->bottom.modifies)
					{
						bModified = true;
						break;
					}
				}
			}
			pBottom->SetModified(bModified);
			pBottom->ClearStepModifiedMark();
		}
		pActiveView->RefreshViews();
	}

	return true;
}

void CUndo::RedoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	allviewstate allstate = m_redoviewstates.back();
	POINT pt = m_redocaretpoints.back();

	if (pLeft->IsTarget())
		m_caretpoints.push_back(pLeft->GetCaretPosition());
	else if (pRight->IsTarget())
		m_caretpoints.push_back(pRight->GetCaretPosition());
	else if (pBottom->IsTarget())
		m_caretpoints.push_back(pBottom->GetCaretPosition());

	allstate.left   = Do(allstate.left, pLeft, pt);
	allstate.right  = Do(allstate.right, pRight, pt);
	allstate.bottom = Do(allstate.bottom, pBottom, pt);

	m_viewstates.push_back(allstate);

	m_redoviewstates.pop_back();
	m_redocaretpoints.pop_back();
}
viewstate CUndo::Do(const viewstate& state, CBaseView * pView, const POINT& pt)
{
	if (!pView)
		return state;

	CViewData* viewData = pView->m_pViewData;
	if (!viewData)
		return state;

	viewstate revstate; // the reversed viewstate

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
	for (std::map<int, DWORD>::const_iterator it = state.linestates.begin(); it != state.linestates.end(); ++it)
	{
		revstate.linestates[it->first] = viewData->GetState(it->first);
		viewData->SetState(it->first, static_cast<DiffStates>(it->second));
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
