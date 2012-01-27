// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2009,2011 - TortoiseSVN

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
#include "Resource.h"
#include "AppUtils.h"
#include ".\bottomview.h"

IMPLEMENT_DYNCREATE(CBottomView, CBaseView)

CBottomView::CBottomView(void)
{
	m_pwndBottom = this;
	m_nStatusBarID = ID_INDICATOR_BOTTOMVIEW;
}

CBottomView::~CBottomView(void)
{
}

bool CBottomView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
	if (!this->IsWindowVisible())
		return false;

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
#define ID_USETHEIRBLOCK 1
#define ID_USEYOURBLOCK 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
		UINT uEnabled = MF_ENABLED;
		if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
			uEnabled |= MF_DISABLED | MF_GRAYED;
		CString temp;

		bool bImportantBlock = true;
		switch (state)
		{
		case DIFFSTATE_UNKNOWN:
			bImportantBlock = false;
			break;
		}

		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURANDTHEIRBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRANDYOURBLOCK, temp);

		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_EDIT_COPY);
		popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_COPY, temp);
		if (!m_bCaretHidden)
		{
			temp.LoadString(IDS_EDIT_CUT);
			popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_CUT, temp);
			temp.LoadString(IDS_EDIT_PASTE);
			popup.AppendMenu(MF_STRING | (CAppUtils::HasClipboardFormat(CF_UNICODETEXT)||CAppUtils::HasClipboardFormat(CF_TEXT) ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_PASTE, temp);
		}

		// if the context menu is invoked through the keyboard, we have to use
		// a calculated position on where to anchor the menu on
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetWindowRect(&rect);
			point = rect.CenterPoint();
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		viewstate rightstate;
		viewstate bottomstate;
		viewstate leftstate;
		switch (cmd)
		{
		case ID_USETHEIRBLOCK:
			UseTheirTextBlock();
			break;
		case ID_USEYOURBLOCK:
			UseMyTextBlock();
			break;
		case ID_USEYOURANDTHEIRBLOCK:
			UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
			CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			break;
		case ID_USETHEIRANDYOURBLOCK:
			UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
			CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			break;
		case ID_EDIT_COPY:
			OnEditCopy();
			return true;
		case ID_EDIT_CUT:
			OnEditCopy();
			RemoveSelectedText();
			break;
		case ID_EDIT_PASTE:
			PasteText();
			break;
		}
	}
	return false;
}

void CBottomView::UseTheirTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndLeft->m_pViewData->GetState(i));
		m_pViewData->SetLineEnding(i, lineendings);
		if (IsLineConflicted(i))
		{
			if (m_pwndLeft->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
		}
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	SetModified();
	RefreshViews();
}

void CBottomView::UseMyTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndRight->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndRight->m_pViewData->GetState(i));
		m_pViewData->SetLineEnding(i, lineendings);
		{
			if (m_pwndRight->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
		}
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	SetModified();
	RefreshViews();
}

void CBottomView::UseTheirThenMyTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	RefreshViews();
}

void CBottomView::UseMyThenTheirTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	RefreshViews();
}
