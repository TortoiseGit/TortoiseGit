// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2013 - TortoiseSVN

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
#include "resource.h"
#include "AppUtils.h"

#include "LeftView.h"
#include "BottomView.h"

IMPLEMENT_DYNCREATE(CLeftView, CBaseView)

CLeftView::CLeftView(void)
{
	m_pwndLeft = this;
	m_pState = &m_AllState.left;
	m_nStatusBarID = ID_INDICATOR_LEFTVIEW;
}

CLeftView::~CLeftView(void)
{
}


void CLeftView::UseBothLeftFirst()
{
	if (!IsRightViewGood())
		return;
	int nFirstViewLine = 0; // first view line in selection
	int nLastViewLine	= 0; // last view line in selection

	if (!IsWritable())
		return;
	if (!GetViewSelection(nFirstViewLine, nLastViewLine))
		return;

	int nNextViewLine = nLastViewLine + 1; // first view line after selected block

	CUndo::GetInstance().BeginGrouping();

	// right original become added
	for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
	{
		if (!IsStateEmpty(GetViewState(viewLine)))
		{
			SetViewState(viewLine, DIFFSTATE_REMOVED);
		}
	}
	SaveUndoStep();

	// your block is done, now insert their block
	int viewindex = nNextViewLine;
	for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
	{
		viewdata line = m_pwndRight->GetViewData(viewLine);
		if (IsStateEmpty(line.state))
		{
			line.state = DIFFSTATE_EMPTY;
		}
		else
		{
			if (line.state!=DIFFSTATE_NORMAL) {
				m_pwndRight->SetViewState(viewLine, DIFFSTATE_NORMAL);
				line.state = DIFFSTATE_NORMAL;
			}
			SetModified();
		}
		InsertViewData(viewindex++, line);
	}
	SaveUndoStep();

	// now insert an empty block in right view
	int nCount = nLastViewLine - nFirstViewLine + 1;
	m_pwndRight->InsertViewEmptyLines(nFirstViewLine, nCount);
	m_pwndRight->Invalidate();
	SaveUndoStep();

	// clean up
	int nRemovedLines = CleanEmptyLines();
	SaveUndoStep();
	UpdateViewLineNumbers();
	SaveUndoStep();

	CUndo::GetInstance().EndGrouping();

	// final clean up
	ClearSelection();
	SetupAllViewSelection(nFirstViewLine, 2*nLastViewLine - nFirstViewLine - nRemovedLines + 1);
	BuildAllScreen2ViewVector();
	RefreshViews();
}

void CLeftView::UseBothRightFirst()
{
	if (!IsRightViewGood())
		return;
	int nFirstViewLine = 0; // first view line in selection
	int nLastViewLine	= 0; // last view line in selection

	if (!IsWritable())
		return;
	if (!GetViewSelection(nFirstViewLine, nLastViewLine))
		return;

	int nNextViewLine = nLastViewLine + 1; // first view line after selected block

	CUndo::GetInstance().BeginGrouping();

	// left original become removed
	for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
	{
		if (!IsStateEmpty(GetViewState(viewLine)))
		{
			SetViewState(viewLine, DIFFSTATE_THEIRSREMOVED);
		}
	}
	SaveUndoStep();

	for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
	{
		viewdata line = m_pwndRight->GetViewData(viewLine);
		if (IsStateEmpty(line.state))
		{
			line.state = DIFFSTATE_EMPTY;
		}
		else
		{
			if (line.state!=DIFFSTATE_NORMAL) {
				m_pwndRight->SetViewState(viewLine, DIFFSTATE_NORMAL);
				line.state = DIFFSTATE_NORMAL;
			}
			SetModified();
		}
		InsertViewData(viewLine, line);
	}

	// now insert an empty block in right view
	int nCount = nLastViewLine - nFirstViewLine + 1;
	m_pwndRight->InsertViewEmptyLines(nNextViewLine, nCount);
	m_pwndRight->Invalidate(); // empty lines added
	SaveUndoStep();

	// clean up
	int nRemovedLines = CleanEmptyLines();
	SaveUndoStep();
	UpdateViewLineNumbers();
	SaveUndoStep();

	CUndo::GetInstance().EndGrouping();

	// final clean up
	ClearSelection();
	SetupAllViewSelection(nFirstViewLine, 2*nLastViewLine - nFirstViewLine - nRemovedLines + 1);
	BuildAllScreen2ViewVector();
	RefreshViews();
}

void CLeftView::UseRightBlock()
{
	int nFirstViewLine = 0;
	int nLastViewLine	= 0;

	if (!IsWritable())
		return;
	if (!GetViewSelection(nFirstViewLine, nLastViewLine))
		return;

	return UseViewBlock(m_pwndRight, nFirstViewLine, nLastViewLine);
}

void CLeftView::UseRightFile()
{
	int nFirstViewLine = 0;
	int nLastViewLine = GetViewCount()-1;

	if (!IsWritable())
		return;
	ClearSelection();
	return UseViewBlock(m_pwndRight, nFirstViewLine, nLastViewLine);
}


void CLeftView::AddContextItems(CIconMenu& popup, DiffStates state)
{
	const bool bShow = HasSelection() && (state != DIFFSTATE_UNKNOWN);

	if (IsBottomViewGood())
	{
		if (bShow)
			popup.AppendMenuIcon(POPUPCOMMAND_USETHEIRBLOCK, IDS_VIEWCONTEXTMENU_USETHISBLOCK);
		popup.AppendMenuIcon(POPUPCOMMAND_USETHEIRFILE, IDS_VIEWCONTEXTMENU_USETHISFILE);
		if (bShow)
		{
			popup.AppendMenuIcon(POPUPCOMMAND_USEYOURANDTHEIRBLOCK, IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
			popup.AppendMenuIcon(POPUPCOMMAND_USETHEIRANDYOURBLOCK, IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
		}
	}
	else
	{
		if (bShow)
		{
			popup.AppendMenuIcon(POPUPCOMMAND_USELEFTBLOCK, IDS_VIEWCONTEXTMENU_USETHISBLOCK);
			popup.AppendMenuIcon(POPUPCOMMAND_USEBOTHLEFTFIRST, IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
			popup.AppendMenuIcon(POPUPCOMMAND_USEBOTHRIGHTFIRST, IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
			if (IsLeftViewGood() && !m_pwndLeft->IsReadonly())
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(POPUPCOMMAND_PREPENDFROMRIGHT, IDS_VIEWCONTEXTMENU_PREPENDRIGHT);
				popup.AppendMenuIcon(POPUPCOMMAND_REPLACEBYRIGHT, IDS_VIEWCONTEXTMENU_USERIGHT);
				popup.AppendMenuIcon(POPUPCOMMAND_APPENDFROMRIGHT, IDS_VIEWCONTEXTMENU_APPENDRIGHT);
			}
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		popup.AppendMenuIcon(POPUPCOMMAND_USELEFTFILE, IDS_VIEWCONTEXTMENU_USETHISFILE);
		if (IsLeftViewGood() && !m_pwndLeft->IsReadonly())
		{
			popup.AppendMenuIcon(POPUPCOMMAND_USERIGHTFILE, IDS_VIEWCONTEXTMENU_USEOTHERFILE);
		}
	}

	CBaseView::AddContextItems(popup, state);
}
