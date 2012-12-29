// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2011-2012 Sven Strickroth <email@cs-ware.de>

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
#include "GitBlameLogList.h"
#include "GitRev.h"
#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"
#include "MainFrm.h"
#include "CommonAppUtils.h"

IMPLEMENT_DYNAMIC(CGitBlameLogList, CHintListCtrl)

void CGitBlameLogList::hideUnimplementedCommands()
{
	hideFromContextMenu(
		GetContextMenuBit(ID_COMPAREWITHPREVIOUS) |
		GetContextMenuBit(ID_COPYCLIPBOARD) |
		GetContextMenuBit(ID_COPYHASH) |
		GetContextMenuBit(ID_EXPORT) |
		GetContextMenuBit(ID_CREATE_BRANCH) |
		GetContextMenuBit(ID_CREATE_TAG) |
		GetContextMenuBit(ID_SWITCHTOREV)
		, true);
	m_ContextMenuMask |= GetContextMenuBit(ID_LOG) | GetContextMenuBit(ID_BLAME) | GetContextMenuBit(ID_REPOBROWSE);
}

void CGitBlameLogList::ContextMenuAction(int cmd, int /*FirstSelect*/, int /*LastSelect*/, CMenu * /*menu*/)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	CString  procCmd = _T("/path:\"");
	procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	procCmd += _T("\" ");
	procCmd += _T(" /rev:")+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString();

	procCmd += _T(" /command:");

	switch (cmd)
	{
		case ID_GNUDIFF1:
			procCmd += _T("diff /udiff");
		break;
		case ID_COMPAREWITHPREVIOUS:
			if (indexNext + 1 < m_logEntries.size()) // cannot diff previous revision in first revision
			{
				procCmd+=CString(_T("diff /startrev:"))+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString()+CString(_T(" /endrev:"))+this->m_logEntries.GetGitRevAt(indexNext+1).m_CommitHash.ToString();
			}
			else
			{
				return;
			}
			break;
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			return;
		case ID_COPYHASH:
			{
				CopySelectionToClipBoard(TRUE);
			}
			return;
		case ID_EXPORT:
			procCmd += _T("export");
			break;
		case ID_CREATE_BRANCH:
			procCmd += _T("branch");
			break;
		case ID_CREATE_TAG:
			procCmd += _T("tag");
			break;
		case ID_SWITCHTOREV:
			procCmd += _T("switch");
			break;
		case ID_BLAME:
			procCmd += _T("blame");
			procCmd += _T(" /endrev:") + this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString();
			break;
		case ID_LOG:
			procCmd += _T("log");
			break;
		case ID_REPOBROWSE:
			procCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%s"), g_Git.m_CurrentDir, this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString());
			break;
		default:
			//CMessageBox::Show(NULL,_T("Have not implemented"),_T("TortoiseGit"),MB_OK);
			return;
		} // switch (cmd)

		CCommonAppUtils::RunTortoiseGitProc(procCmd);
}