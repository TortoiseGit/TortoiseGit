// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN
// Copyright (C) 2008-2011 - TortoiseGit

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
#include "LogDlgHelper.h"
#include "LogDlg.h"

CStoreSelection::CStoreSelection(CLogDlg* /* dlg */)
{
#if 0
	m_logdlg = dlg;

	int selIndex = m_logdlg->m_LogList.GetSelectionMark();
	if ( selIndex>=0 )
	{
		POSITION pos = m_logdlg->m_LogList.GetFirstSelectedItemPosition();
		int nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
		if ( nIndex!=-1 && nIndex < m_logdlg->m_arShownList.GetSize() )
		{
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(nIndex));
			m_SetSelectedRevisions.insert(pLogEntry->Rev);
			while (pos)
			{
				nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
				if ( nIndex!=-1 && nIndex < m_logdlg->m_arShownList.GetSize() )
				{
					pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(nIndex));
					m_SetSelectedRevisions.insert(pLogEntry->Rev);
				}
			}
		}
	}
#endif
}

CStoreSelection::~CStoreSelection()
{
#if 0
	if ( m_SetSelectedRevisions.size()>0 )
	{
		for (int i=0; i<m_logdlg->m_arShownList.GetCount(); ++i)
		{
			LONG nRevision = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(i))->Rev;
			if ( m_SetSelectedRevisions.find(nRevision)!=m_SetSelectedRevisions.end() )
			{
				m_logdlg->m_LogList.SetSelectionMark(i);
				m_logdlg->m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_logdlg->m_LogList.EnsureVisible(i, FALSE);
			}
		}
	}
#endif
}

