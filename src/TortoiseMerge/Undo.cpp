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

#include "StdAfx.h"
#include "Undo.h"

#include "BaseView.h"

void viewstate::AddLineFormView(CBaseView *pView, int nLine, bool bAddEmptyLine)
{
	if (!pView || !pView->m_pViewData)
		return;
	difflines[nLine] = pView->m_pViewData->GetLine(nLine);
	linestates[nLine] = pView->m_pViewData->GetState(nLine);
	linesEOL[nLine] = pView->m_pViewData->GetLineEnding(nLine);
	if (bAddEmptyLine)
	{
		addedlines.push_back(nLine + 1);
		pView->AddEmptyLine(nLine);
	}
}

CUndo& CUndo::GetInstance()
{
	static CUndo instance;
	return instance;
}

CUndo::CUndo()
{
	m_originalstate = 0;
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const viewstate& leftstate, const viewstate& rightstate, const viewstate& bottomstate, POINT pt)
{
	m_viewstates.push_back(bottomstate);
	m_viewstates.push_back(rightstate);
	m_viewstates.push_back(leftstate);
	m_caretpoints.push_back(pt);
}

bool CUndo::Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	if (!CanUndo())
		return false;

	if (!m_groups.empty() && m_groups.back() == m_caretpoints.size())
	{
		m_groups.pop_back();
		std::list<int>::size_type b = m_groups.back();
		m_groups.pop_back();
		while (b < m_caretpoints.size())
			UndoOne(pLeft, pRight, pBottom);
	}
	else
		UndoOne(pLeft, pRight, pBottom);

	CBaseView * pActiveView = pLeft;

	if (pRight && pRight->HasCaret())
		pActiveView = pRight;

	if (pBottom && pBottom->HasCaret())
		pActiveView = pBottom;

	if (pActiveView) {
		pActiveView->ClearSelection();
		pActiveView->EnsureCaretVisible();
		pActiveView->UpdateCaret();
		pActiveView->SetModified(m_viewstates.size() != m_originalstate);
		pActiveView->RefreshViews();
	}

	if (m_viewstates.size() < m_originalstate)
		// Can never get back to original state now 
		m_originalstate = 1; // size() is always a multiple of 3

	return true;
}

void CUndo::UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	viewstate state = m_viewstates.back();
	Undo(state, pLeft);
	m_viewstates.pop_back();
	state = m_viewstates.back();
	Undo(state, pRight);
	m_viewstates.pop_back();
	state = m_viewstates.back();
	Undo(state, pBottom);
	m_viewstates.pop_back();
	if ((pLeft)&&(pLeft->HasCaret()))
	{
		pLeft->SetCaretPosition(m_caretpoints.back());
		pLeft->EnsureCaretVisible();
	}
	if ((pRight)&&(pRight->HasCaret()))
	{
		pRight->SetCaretPosition(m_caretpoints.back());
		pRight->EnsureCaretVisible();
	}
	if ((pBottom)&&(pBottom->HasCaret()))
	{
		pBottom->SetCaretPosition(m_caretpoints.back());
		pBottom->EnsureCaretVisible();
	}
	m_caretpoints.pop_back();
}

void CUndo::Undo(const viewstate& state, CBaseView * pView)
{
	if (!pView)
		return;

	for (std::list<int>::const_iterator it = state.addedlines.begin(); it != state.addedlines.end(); ++it)
	{
		if (pView->m_pViewData)
			pView->m_pViewData->RemoveData(*it);
	}
	for (std::map<int, DWORD>::const_iterator it = state.linelines.begin(); it != state.linelines.end(); ++it)
	{
		if (pView->m_pViewData)
		{
			pView->m_pViewData->SetLineNumber(it->first, it->second);
		}
	}
	for (std::map<int, DWORD>::const_iterator it = state.linestates.begin(); it != state.linestates.end(); ++it)
	{
		if (pView->m_pViewData)
		{
			pView->m_pViewData->SetState(it->first, (DiffStates)it->second);
		}
	}
	for (std::map<int, EOL>::const_iterator it = state.linesEOL.begin(); it != state.linesEOL.end(); ++it)
	{
		if (pView->m_pViewData)
		{
			pView->m_pViewData->SetLineEnding(it->first, (EOL)it->second);
		}
	}
	for (std::map<int, CString>::const_iterator it = state.difflines.begin(); it != state.difflines.end(); ++it)
	{
		if (pView->m_pViewData)
		{
			pView->m_pViewData->SetLine(it->first, it->second);
		}
	}
	for (std::map<int, viewdata>::const_iterator it = state.removedlines.begin(); it != state.removedlines.end(); ++it)
	{
		if (pView->m_pViewData)
		{
			pView->m_pViewData->InsertData(it->first, it->second.sLine, it->second.state, it->second.linenumber, it->second.ending);
		}
	}
}


void CUndo::Clear()
{
	m_viewstates.clear();
	m_caretpoints.clear();
	m_groups.clear();
	m_originalstate = 0;
}
