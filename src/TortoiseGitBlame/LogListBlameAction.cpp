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

void RunTortoiseGitProcWithCurrentRev(const CString& command, const GitRev* pRev, const CString &path = g_Git.m_CurrentDir)
{
	ASSERT(pRev);
	CString  procCmd;
	procCmd.Format(L"/command:%s /path:\"%s\" /rev:%s", command, path, pRev->m_CommitHash.ToString());
	CCommonAppUtils::RunTortoiseGitProc(procCmd);
}

void CGitBlameLogList::ContextMenuAction(int cmd, int /*FirstSelect*/, int /*LastSelect*/, CMenu * /*menu*/)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;
	CTortoiseGitBlameView *pView = DYNAMIC_DOWNCAST(CTortoiseGitBlameView,((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView());

	GitRev *pRev = &this->m_logEntries.GetGitRevAt(indexNext);

	switch (cmd & 0xFFFF)
	{
		case ID_BLAMEPREVIOUS:
			{
				int index = (cmd >> 16) & 0xFFFF;
				if (index > 0)
					index -= 1;

				CGitHash parentHash;
				std::vector<CString> parentFilenames;
				GetParentHash(pRev, index, parentHash, parentFilenames);
				for (size_t i = 0; i < parentFilenames.size(); ++i)
				{
					CString procCmd = _T("/path:\"") + pView->ResolveCommitFile(parentFilenames[i]) + _T("\" ");
					procCmd += _T(" /command:blame");
					procCmd += _T(" /endrev:") + parentHash.ToString();

					CCommonAppUtils::RunTortoiseGitProc(procCmd);
				}
			}
			break;
		case ID_COMPAREWITHPREVIOUS:
			{
				int index = (cmd >> 16) & 0xFFFF;
				if (index > 0)
					index -= 1;

				CGitHash parentHash;
				std::vector<CString> parentFilenames;
				GetParentHash(pRev, index, parentHash, parentFilenames);
				for (size_t i = 0; i < parentFilenames.size(); ++i)
				{
					CString procCmd = _T("/path:\"") + pView->ResolveCommitFile(parentFilenames[i]) + _T("\" ");
					procCmd += _T(" /command:diff");
					procCmd += _T(" /startrev:") + pRev->m_CommitHash.ToString();
					procCmd += _T(" /endrev:") + parentHash.ToString();

					CCommonAppUtils::RunTortoiseGitProc(procCmd);
				}
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
			RunTortoiseGitProcWithCurrentRev(_T("export"), pRev);
			break;
		case ID_CREATE_BRANCH:
			RunTortoiseGitProcWithCurrentRev(_T("branch"), pRev);
			break;
		case ID_CREATE_TAG:
			RunTortoiseGitProcWithCurrentRev(_T("tag"), pRev);
			break;
		case ID_SWITCHTOREV:
			RunTortoiseGitProcWithCurrentRev(_T("switch"), pRev);
			break;
		case ID_LOG:
			RunTortoiseGitProcWithCurrentRev(_T("log"), pRev, ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName());
			break;
		case ID_REPOBROWSE:
			RunTortoiseGitProcWithCurrentRev(_T("repobrowser"), pRev, ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName());
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
		if (paths.empty())
		{
			// in case the hash does not exist in the blame output but it exists in the log follow only the file
			paths.push_back(pView->GetDocument()->m_GitPath);
		}
	}
}

void CGitBlameLogList::GetParentNumbers(GitRev *pRev, const std::vector<CTGitPath>& paths, std::set<int> &parentNos)
{
	if (pRev->m_ParentHash.empty())
	{
		try
		{
			pRev->GetParentFromHash(pRev->m_CommitHash);
		}
		catch (const char* msg)
		{
			MessageBox(_T("Could not get parent.\nlibgit reports:\n") + CString(msg), _T("TortoiseGit"), MB_ICONERROR);
		}
	}

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
							if (parentNo >= 0 && (size_t)parentNo < pRev->m_ParentHash.size())
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

void CGitBlameLogList::GetParentHash(GitRev *pRev, int index, CGitHash &parentHash, std::vector<CString>& parentFilenames)
{
	std::vector<CTGitPath> paths;
	GetPaths(pRev->m_CommitHash, paths);

	std::set<int> parentNos;
	GetParentNumbers(pRev, paths, parentNos);

	int parentNo = 0;
	{
		int i = 0;
		for (auto it = parentNos.cbegin(); it != parentNos.cend(); ++it, ++i)
		{
			if (i == index)
			{
				parentNo = *it;
				break;
			}
		}
	}
	parentHash = pRev->m_ParentHash[parentNo];

	try
	{
		const CTGitPathList& files = pRev->GetFiles(NULL);
		for (int j = 0, j_size = files.GetCount(); j < j_size; ++j)
		{
			const CTGitPath &file =  files[j];
			for (auto it = paths.cbegin(); it != paths.cend(); ++it)
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
							if (parentNo == (file.m_ParentNo & PARENT_MASK))
							{
								if (parentNo >= 0 && (size_t)parentNo < pRev->m_ParentHash.size())
									parentFilenames.push_back( (action & CTGitPath::LOGACTIONS_REPLACED) ? file.GetGitOldPathString() : file.GetGitPathString());
							}
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
