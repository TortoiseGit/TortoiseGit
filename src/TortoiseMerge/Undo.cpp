// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010-2011 - TortoiseSVN

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
	addedlines.clear();

	removedlines.clear();
	replacedlines.clear();
}

CUndo& CUndo::GetInstance()
{
	static CUndo instance;
	return instance;
}

CUndo::CUndo()
{
	m_originalstate = 0;
	m_groupCount = 0;
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const allviewstate& allstate, POINT pt)
{
	m_viewstates.push_back(allstate);
	m_caretpoints.push_back(pt);
}

bool CUndo::Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	if (!CanUndo())
		return false;

	if (m_groups.size() && m_groups.back() == m_caretpoints.size())
	{
		m_groups.pop_back();
		std::list<int>::size_type b = m_groups.back();
		m_groups.pop_back();
		while (b < m_caretpoints.size())
			UndoOne(pLeft, pRight, pBottom);
	}
	else
		UndoOne(pLeft, pRight, pBottom);

	CBaseView * pActiveView = NULL;

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
		bool bModified = m_viewstates.size() != m_originalstate;
		if (pLeft)
			pLeft->SetModified(bModified);
		if (pRight)
			pRight->SetModified(bModified);
		if (pBottom)
			pBottom->SetModified(bModified);
		pActiveView->RefreshViews();
	}

	if (m_viewstates.size() < m_originalstate)
		// Can never get back to original state now
		m_originalstate = (size_t)-1;

	return true;
}

void CUndo::UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	allviewstate allstate = m_viewstates.back();
	POINT pt = m_caretpoints.back();

	Undo(allstate.left, pLeft, pt);
	Undo(allstate.right, pRight, pt);
	Undo(allstate.bottom, pBottom, pt);

	m_viewstates.pop_back();
	m_caretpoints.pop_back();
}

void CUndo::Undo(const viewstate& state, CBaseView * pView, const POINT& pt)
{
	if (!pView)
		return;

	CViewData* viewData = pView->m_pViewData;
	if (!viewData)
		return;

	for (std::list<int>::const_reverse_iterator it = state.addedlines.rbegin(); it != state.addedlines.rend(); ++it)
	{
		viewData->RemoveData(*it);
	}
	for (std::map<int, DWORD>::const_iterator it = state.linelines.begin(); it != state.linelines.end(); ++it)
	{
		viewData->SetLineNumber(it->first, it->second);
	}
	for (std::map<int, DWORD>::const_iterator it = state.linestates.begin(); it != state.linestates.end(); ++it)
	{
		viewData->SetState(it->first, (DiffStates)it->second);
	}
	for (std::map<int, EOL>::const_iterator it = state.linesEOL.begin(); it != state.linesEOL.end(); ++it)
	{
		viewData->SetLineEnding(it->first, (EOL)it->second);
	}
	for (std::map<int, CString>::const_iterator it = state.difflines.begin(); it != state.difflines.end(); ++it)
	{
		viewData->SetLine(it->first, it->second);
	}
	for (std::map<int, viewdata>::const_iterator it = state.removedlines.begin(); it != state.removedlines.end(); ++it)
	{
		viewData->InsertData(it->first, it->second);
	}
	for (std::map<int, viewdata>::const_iterator it = state.replacedlines.begin(); it != state.replacedlines.end(); ++it)
	{
		viewData->SetData(it->first, it->second);
	}

	if (pView->IsTarget())
	{
		pView->SetCaretViewPosition(pt);
		pView->EnsureCaretVisible();
	}


}

void CUndo::Clear()
{
	m_viewstates.clear();
	m_caretpoints.clear();
	m_groups.clear();
	m_originalstate = 0;
	m_groupCount = 0;
}
