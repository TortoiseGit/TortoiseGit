// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2019 - TortoiseGit
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
#include "GitRevLoglist.h"
#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"
#include "MainFrm.h"
#include "CommonAppUtils.h"

IMPLEMENT_DYNAMIC(CGitBlameLogList, CHintCtrl<CListCtrl>)

void CGitBlameLogList::hideUnimplementedCommands()
{
	m_ContextMenuMask |= GetContextMenuBit(ID_BLAMEPREVIOUS) | GetContextMenuBit(ID_LOG);
	hideFromContextMenu(
		GetContextMenuBit(ID_COMPAREWITHPREVIOUS) |
		GetContextMenuBit(ID_GNUDIFF1) |
		GetContextMenuBit(ID_BLAMEPREVIOUS) |
		GetContextMenuBit(ID_COPYCLIPBOARD) |
		GetContextMenuBit(ID_EXPORT) |
		GetContextMenuBit(ID_CREATE_BRANCH) |
		GetContextMenuBit(ID_CREATE_TAG) |
		GetContextMenuBit(ID_SWITCHTOREV) |
		GetContextMenuBit(ID_LOG) |
		GetContextMenuBit(ID_SHOWBRANCHES) |
		GetContextMenuBit(ID_REPOBROWSE)
		, true);
}

void CGitBlameLogList::GetParentHashes(GitRevLoglist* pRev, GIT_REV_LIST& parentHash)
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
	procCmd.Format(L"/command:%s /path:\"%s\" /rev:%s", static_cast<LPCTSTR>(command), static_cast<LPCTSTR>(path), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
	CCommonAppUtils::RunTortoiseGitProc(procCmd);
}

void CGitBlameLogList::ContextMenuAction(int cmd, int /*FirstSelect*/, int /*LastSelect*/, CMenu* /*menu*/, MAP_HASH_NAME&)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;
	auto pView = DYNAMIC_DOWNCAST(CTortoiseGitBlameView, static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->GetActiveView());

	GitRevLoglist* pRev = &this->m_logEntries.GetGitRevAt(indexNext);

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
					CString procCmd = L"/path:\"" + pView->ResolveCommitFile(parentFilenames[i]) + L"\" ";
					procCmd += L" /command:blame";
					procCmd += L" /endrev:" + parentHash.ToString();

					CCommonAppUtils::RunTortoiseGitProc(procCmd);
				}
			}
			break;
		case ID_GNUDIFF1: // fallthrough
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
					CString procCmd = L"/path:\"" + pView->ResolveCommitFile(parentFilenames[i]) + L"\" ";
					procCmd += L" /command:diff";
					procCmd += L" /startrev:" + parentHash.ToString();
					procCmd += L" /endrev:" + pRev->m_CommitHash.ToString();
					if ((cmd & 0xFFFF) == ID_GNUDIFF1)
						procCmd += L" /unified";
					if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
						procCmd += L" /alternative";

					CCommonAppUtils::RunTortoiseGitProc(procCmd);
				}
			}
			break;
		case ID_COPYCLIPBOARDFULL:
		case ID_COPYCLIPBOARDFULLNOPATHS:
		case ID_COPYCLIPBOARDHASH:
		case ID_COPYCLIPBOARDAUTHORSFULL:
		case ID_COPYCLIPBOARDAUTHORSNAME:
		case ID_COPYCLIPBOARDAUTHORSEMAIL:
		case ID_COPYCLIPBOARDSUBJECTS:
		case ID_COPYCLIPBOARDMESSAGES:
			CopySelectionToClipBoard(cmd & 0xFFFF);
			break;
		case ID_EXPORT:
			RunTortoiseGitProcWithCurrentRev(L"export", pRev);
			break;
		case ID_CREATE_BRANCH:
			RunTortoiseGitProcWithCurrentRev(L"branch", pRev);
			break;
		case ID_CREATE_TAG:
			RunTortoiseGitProcWithCurrentRev(L"tag", pRev);
			break;
		case ID_SWITCHTOREV:
			RunTortoiseGitProcWithCurrentRev(L"switch", pRev);
			break;
		case ID_LOG:
			{
				CString procCmd;
				procCmd.Format(L"/command:log /path:\"%s\" /endrev:%s /rev:%s", static_cast<LPCTSTR>(static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName()), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
				CCommonAppUtils::RunTortoiseGitProc(procCmd);
			}
			break;
		case ID_REPOBROWSE:
			RunTortoiseGitProcWithCurrentRev(L"repobrowser", pRev, static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName());
			break;
		case ID_SHOWBRANCHES:
			RunTortoiseGitProcWithCurrentRev(L"commitisonrefs", pRev);
			break;
		default:
			//CMessageBox::Show(nullptr, L"Have not implemented", L"TortoiseGit", MB_OK);
			break;
	} // switch (cmd)
}

void CGitBlameLogList::GetPaths(const CGitHash& hash, std::vector<CTGitPath>& paths)
{
	auto pView = DYNAMIC_DOWNCAST(CTortoiseGitBlameView, static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->GetActiveView());
	if (pView)
	{
		{
			std::set<CString> filenames;
			auto numberOfLines = pView->m_data.GetNumberOfLines();
			for (size_t i = 0; i < numberOfLines; ++i)
			{
				if (pView->m_data.GetHash(i) == hash)
					filenames.insert(pView->m_data.GetFilename(i));
			}
			for (auto it = filenames.cbegin(); it != filenames.cend(); ++it)
				paths.emplace_back(*it);
		}
		if (paths.empty())
		{
			// in case the hash does not exist in the blame output but it exists in the log follow only the file
			paths.push_back(pView->GetDocument()->m_GitPath);
		}
	}
}

void CGitBlameLogList::GetParentNumbers(GitRevLoglist* pRev, const std::vector<CTGitPath>& paths, std::set<int>& parentNos)
{
	if (pRev->m_ParentHash.empty())
	{
		if (pRev->GetParentFromHash(pRev->m_CommitHash))
			MessageBox(pRev->GetLastErr(), L"TortoiseGit", MB_ICONERROR);
	}

	GIT_REV_LIST allParentHash;
	CGitLogListBase::GetParentHashes(pRev, allParentHash);

	try
	{
		const CTGitPathList& files = pRev->GetFiles(nullptr);
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
							if (parentNo >= 0 && static_cast<size_t>(parentNo) < pRev->m_ParentHash.size())
								parentNos.insert(parentNo);
						}
					}
				}
			}
		}
	}
	catch (const char* msg)
	{
		MessageBox(L"Could not get files of parents.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
	}
}

void CGitBlameLogList::GetParentHash(GitRevLoglist* pRev, int index, CGitHash& parentHash, std::vector<CString>& parentFilenames)
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
		const CTGitPathList& files = pRev->GetFiles(nullptr);
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
							if (parentNo == (file.m_ParentNo & PARENT_MASK) && static_cast<size_t>(parentNo) < pRev->m_ParentHash.size())
								parentFilenames.push_back( (action & CTGitPath::LOGACTIONS_REPLACED) ? file.GetGitOldPathString() : file.GetGitPathString());
						}
					}
				}
			}
		}
	}
	catch (const char* msg)
	{
		MessageBox(L"Could not get files of parents.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
	}
}
