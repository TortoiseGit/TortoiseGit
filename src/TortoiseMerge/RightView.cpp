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
#include ".\rightview.h"

IMPLEMENT_DYNCREATE(CRightView, CBaseView)

CRightView::CRightView(void)
{
	m_pwndRight = this;
	m_nStatusBarID = ID_INDICATOR_RIGHTVIEW;
}

CRightView::~CRightView(void)
{
}

bool CRightView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
	if (!this->IsWindowVisible())
		return false;

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
#define ID_USEBLOCK 1
#define ID_USEFILE 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
#define ID_USEBOTHTHISFIRST 5
#define ID_USEBOTHTHISLAST 6

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

		if (!m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERBLOCK);
		}
		else
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBLOCK, temp);

		if (!m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERFILE);
		}
		else
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);

		if (m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
			popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURANDTHEIRBLOCK, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
			popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRANDYOURBLOCK, temp);
		}
		else
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
			popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBOTHTHISFIRST, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
			popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBOTHTHISLAST, temp);
		}

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
		case ID_EDIT_COPY:
			OnEditCopy();
			return true;
		case ID_EDIT_CUT:
			OnEditCopy();
			RemoveSelectedText();
			return false;
		case ID_EDIT_PASTE:
			PasteText();
			return false;
		case ID_USEFILE:
			{
				UseFile(false);
			} 
			break;
		case ID_USEBLOCK:
			{
				UseBlock(false);
			} 
		break;
		case ID_USEYOURANDTHEIRBLOCK:
			{
				UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
				CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			}
			break;
		case ID_USETHEIRANDYOURBLOCK:
			{
				UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
				CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			}
			break;
		case ID_USEBOTHTHISFIRST:
			{
				UseBothRightFirst(rightstate, leftstate);
				CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			}
			break;
		case ID_USEBOTHTHISLAST:
			{
				UseBothLeftFirst(rightstate, leftstate);
				CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
			}
			break;
		default:
			return false;
		} // switch (cmd) 
	} // if (popup.CreatePopupMenu()) 
	return false;
}

void CRightView::UseFile(bool refreshViews /* = true */)
{
	viewstate rightstate;
	viewstate bottomstate;
	viewstate leftstate;
	if (m_pwndBottom->IsWindowVisible())
	{
		for (int i=0; i<GetLineCount(); i++)
		{
			bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
			m_pwndBottom->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
			bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
			m_pwndBottom->m_pViewData->SetState(i, m_pViewData->GetState(i));
			m_pwndBottom->m_pViewData->SetLineEnding(i, m_pViewData->GetLineEnding(i));
			if (m_pwndBottom->IsLineConflicted(i))
				m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
		}
		m_pwndBottom->SetModified();
	}
	else
	{
		for (int i=0; i<GetLineCount(); i++)
		{
			rightstate.difflines[i] = m_pViewData->GetLine(i);
			m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
			m_pViewData->SetLineEnding(i, m_pwndLeft->m_pViewData->GetLineEnding(i));
			DiffStates state = m_pwndLeft->m_pViewData->GetState(i);
			switch (state)
			{
			case DIFFSTATE_CONFLICTEMPTY:
			case DIFFSTATE_UNKNOWN:
			case DIFFSTATE_EMPTY:
				rightstate.linestates[i] = m_pViewData->GetState(i);
				m_pViewData->SetState(i, state);
				break;
			case DIFFSTATE_YOURSADDED:
			case DIFFSTATE_IDENTICALADDED:
			case DIFFSTATE_NORMAL:
			case DIFFSTATE_THEIRSADDED:
			case DIFFSTATE_ADDED:
			case DIFFSTATE_CONFLICTADDED:
			case DIFFSTATE_CONFLICTED:
			case DIFFSTATE_CONFLICTED_IGNORED:
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
				rightstate.linestates[i] = m_pViewData->GetState(i);
				m_pViewData->SetState(i, DIFFSTATE_NORMAL);
				leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
				m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_NORMAL);
				break;
			default:
				break;
			}
			SetModified();
			if (m_pwndLocator)
				m_pwndLocator->DocumentUpdated();
		}
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	if (refreshViews)
		RefreshViews();
}

void CRightView::UseBlock(bool refreshViews /* = true */)
{
	viewstate rightstate;
	viewstate bottomstate;
	viewstate leftstate;

	if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
		return;
	if (m_pwndBottom->IsWindowVisible())
	{
		for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
		{
			bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
			m_pwndBottom->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
			bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
			m_pwndBottom->m_pViewData->SetState(i, m_pViewData->GetState(i));
			m_pwndBottom->m_pViewData->SetLineEnding(i, lineendings);
			if (m_pwndBottom->IsLineConflicted(i))
			{
				if (m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
					m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
				else
					m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
			}
		}
		m_pwndBottom->SetModified();
	}
	else
	{
		for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
		{
			rightstate.difflines[i] = m_pViewData->GetLine(i);
			m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
			m_pViewData->SetLineEnding(i, lineendings);
			DiffStates state = m_pwndLeft->m_pViewData->GetState(i);
			switch (state)
			{
			case DIFFSTATE_ADDED:
			case DIFFSTATE_CONFLICTADDED:
			case DIFFSTATE_CONFLICTED:
			case DIFFSTATE_CONFLICTED_IGNORED:
			case DIFFSTATE_CONFLICTEMPTY:
			case DIFFSTATE_IDENTICALADDED:
			case DIFFSTATE_NORMAL:
			case DIFFSTATE_THEIRSADDED:
			case DIFFSTATE_UNKNOWN:
			case DIFFSTATE_YOURSADDED:
			case DIFFSTATE_EMPTY:
				rightstate.linestates[i] = m_pViewData->GetState(i);
				m_pViewData->SetState(i, state);
				break;
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
				rightstate.linestates[i] = m_pViewData->GetState(i);
				m_pViewData->SetState(i, DIFFSTATE_ADDED);
				break;
			default:
				break;
			}
		}
		SetModified();
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	if (refreshViews)
		RefreshViews();
}

void CRightView::UseLeftBeforeRight(bool refreshViews /* = true */)
{
	viewstate rightstate;
	viewstate bottomstate;
	viewstate leftstate;
	UseBothLeftFirst(rightstate, leftstate);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	if (refreshViews)
		RefreshViews();
}

void CRightView::UseRightBeforeLeft(bool refreshViews /* = true */)
{
	viewstate rightstate;
	viewstate bottomstate;
	viewstate leftstate;
	UseBothRightFirst(rightstate, leftstate);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	if (refreshViews)
		RefreshViews();
}
