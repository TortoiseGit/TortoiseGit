// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2012 - TortoiseSVN

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

#include "leftview.h"
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
			popup.AppendMenuIcon(POPUPCOMMAND_USELEFTBLOCK, IDS_VIEWCONTEXTMENU_USETHISBLOCK);
		popup.AppendMenuIcon(POPUPCOMMAND_USELEFTFILE, IDS_VIEWCONTEXTMENU_USETHISFILE);
		if (bShow)
		{
			popup.AppendMenuIcon(POPUPCOMMAND_USEBOTHLEFTFIRST, IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
			popup.AppendMenuIcon(POPUPCOMMAND_USEBOTHRIGHTFIRST, IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
		}
	}

	CBaseView::AddContextItems(popup, state);
}
