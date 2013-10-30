// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
// Copyright (C) 2011-2013 Sven Strickroth <email@cs-ware.de>

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
	m_ContextMenuMask |= GetContextMenuBit(ID_BLAMEPREVIOUS);
	hideFromContextMenu(
		GetContextMenuBit(ID_COMPAREWITHPREVIOUS) |
		GetContextMenuBit(ID_BLAMEPREVIOUS) |
		GetContextMenuBit(ID_COPYCLIPBOARD) |
		GetContextMenuBit(ID_COPYHASH) |
		GetContextMenuBit(ID_EXPORT) |
		GetContextMenuBit(ID_CREATE_BRANCH) |
		GetContextMenuBit(ID_CREATE_TAG) |
		GetContextMenuBit(ID_SWITCHTOREV) |
		GetContextMenuBit(ID_LOG) |
		GetContextMenuBit(ID_REPOBROWSE)
		, true);
}

void CGitBlameLogList::GetParentHashes(GitRev *pRev, GIT_REV_LIST &parentHash)
{
	std::vector<CTGitPath> paths;
	GetPaths(pRev->m_CommitHash, paths);

	std::set<int> parentNos;
	GetParentNumbers(pRev, paths, parentNos);

	for (auto it = parentNos.cbegin(); it != parentNos.cend(); ++it)
	{
		int parentNo = *it;
		parentHash.push_back(pRev->m_ParentHash[parentNo]);
	}
}

void CGitBlameLogList::ContextMenuAction(int cmd, int /*FirstSelect*/, int /*LastSelect*/, CMenu * /*menu*/)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	GitRev *pRev = &this->m_logEntries.GetGitRevAt(indexNext);

	switch (cmd)
	{
		case ID_COMPAREWITHPREVIOUS:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\" ");
				procCmd += _T(" /rev:")+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString();

				procCmd += _T(" /command:");
				procCmd+=CString(_T("diff /startrev:"))+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString()+CString(_T(" /endrev:"))+this->m_logEntries.GetGitRevAt(indexNext+1).m_CommitHash.ToString();
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			break;
		case ID_COPYHASH:
			{
				CopySelectionToClipBoard(ID_COPY_HASH);
			}
			break;
		case ID_EXPORT:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\"");
				procCmd += _T(" /rev:")+pRev->m_CommitHash.ToString();

				procCmd += _T(" /command:export");
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_CREATE_BRANCH:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\"");
				procCmd += _T(" /rev:")+pRev->m_CommitHash.ToString();

				procCmd += _T(" /command:branch");
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_CREATE_TAG:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\"");
				procCmd += _T(" /rev:")+pRev->m_CommitHash.ToString();

				procCmd += _T(" /command:tag");
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_SWITCHTOREV:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\"");
				procCmd += _T(" /rev:")+pRev->m_CommitHash.ToString();

				procCmd += _T(" /command:switch");
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_LOG:
			{
				CString  procCmd = _T("/path:\"");
				procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
				procCmd += _T("\"");
				procCmd += _T(" /rev:")+pRev->m_CommitHash.ToString();

				procCmd += _T(" /command:log");
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_REPOBROWSE:
			{
				CString  procCmd;
				procCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%s"), g_Git.m_CurrentDir, pRev->m_CommitHash.ToString());
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		default:
			//CMessageBox::Show(NULL,_T("Have not implemented"),_T("TortoiseGit"),MB_OK);
			break;
	} // switch (cmd)
}

void CGitBlameLogList::GetPaths(const CGitHash& hash, std::vector<CTGitPath>& paths)
{
	CTortoiseGitBlameView *pView = DYNAMIC_DOWNCAST(CTortoiseGitBlameView,((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView());
	if (pView)
	{
		{
			std::set<CString> filenames;
			int numberOfLines = pView->m_data.GetNumberOfLines();
			for (int i = 0; i < numberOfLines; ++i)
			{
				if (pView->m_data.GetHash(i) == hash)
				{
					filenames.insert(pView->m_data.GetFilename(i));
				}
			}
			for (auto it = filenames.cbegin(); it != filenames.cend(); ++it)
			{
				paths.push_back(CTGitPath(*it));
			}
		}
	}
}

void CGitBlameLogList::GetParentNumbers(GitRev *pRev, const std::vector<CTGitPath>& paths, std::set<int> &parentNos)
{
	GIT_REV_LIST allParentHash;
	CGitLogListBase::GetParentHashes(pRev, allParentHash);

	try
	{
		const CTGitPathList& files = pRev->GetFiles(NULL);
		for (int j=0, j_size = files.GetCount(); j < j_size; ++j)
		{
			const CTGitPath &file =  files[j];
			for (auto it=paths.cbegin(); it != paths.cend(); ++it)
			{
				const CTGitPath& path = *it;
				if (file.IsEquivalentTo(path))
				{
					if (!(file.m_ParentNo & MERGE_MASK))
					{
						int action = file.m_Action;
						// ignore (action & CTGitPath::LOGACTIONS_ADDED), as then there is nothing to blame/diff
						// ignore (action & CTGitPath::LOGACTIONS_DELETED), should never happen as the file must exist
						if (action & (CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED))
						{
							int parentNo = file.m_ParentNo & PARENT_MASK;
							parentNos.insert(parentNo);
						}
					}
				}
			}
		}
	}
	catch (const char* msg)
	{
		MessageBox(_T("Could not get files of parents.\nlibgit reports:\n") + CString(msg), _T("TortoiseGit"), MB_ICONERROR);
	}
}
