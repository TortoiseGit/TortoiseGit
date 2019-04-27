// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2005-2007 Marco Costalba

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
// GitLogList.cpp : implementation file
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitLogList.h"
#include "GitRev.h"
#include "IconMenu.h"
#include "cursor.h"
#include "GitProgressDlg.h"
#include "ProgressDlg.h"
#include "SysProgressDlg.h"
#include "LogDlg.h"
#include "MessageBox.h"
#include "registry.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
#include "CommitDlg.h"
#include "RebaseDlg.h"
#include "CommitIsOnRefsDlg.h"
#include "GitDiff.h"
#include "../TGitCache/CacheInterface.h"

IMPLEMENT_DYNAMIC(CGitLogList, CHintCtrl<CListCtrl>)

static bool GetFirstEntryStartingWith(STRING_VECTOR& heystack, const CString& needle, CString& result)
{
	auto it = std::find_if(heystack.cbegin(), heystack.cend(), [&needle](const CString& entry) { return CStringUtils::StartsWith(entry, needle); });
	if (it == heystack.cend())
		return false;
	result = *it;
	return true;
}

int CGitLogList::RevertSelectedCommits(int parent)
{
	CSysProgressDlg progress;
	int ret = -1;

#if 0
	if(!g_Git.CheckCleanWorkTree())
		CMessageBox::Show(nullptr, IDS_PROC_NOCLEAN, IDS_APPNAME, MB_OK);
#endif

	if (this->GetSelectedCount() > 1)
	{
		progress.SetTitle(CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_REVERTCOMMIT)));
		progress.SetTime(true);
		progress.ShowModeless(this);
	}

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	POSITION pos = GetFirstSelectedItemPosition();
	int i=0;
	while(pos)
	{
		int index = GetNextSelectedItem(pos);
		GitRev* r1 = m_arShownList.SafeGetAt(index);

		if (progress.IsVisible())
		{
			progress.FormatNonPathLine(1, IDS_PROC_REVERTCOMMIT, static_cast<LPCTSTR>(r1->m_CommitHash.ToString()));
			progress.FormatNonPathLine(2, L"%s", static_cast<LPCTSTR>(r1->GetSubject()));
			progress.SetProgress(i, this->GetSelectedCount());
		}
		++i;

		if(r1->m_CommitHash.IsEmpty())
			continue;

		if (g_Git.GitRevert(parent, r1->m_CommitHash))
		{
			CString str;
			str.LoadString(IDS_SVNACTION_FAILEDREVERT);
			str = g_Git.GetGitLastErr(str, CGit::GIT_CMD_REVERT);
			if( GetSelectedCount() == 1)
				CMessageBox::Show(GetParentHWND(), str, L"TortoiseGit", MB_OK | MB_ICONERROR);
			else if (CMessageBox::Show(GetParentHWND(), str, L"TortoiseGit", 2, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_SKIPBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
				return ret;
		}
		else
			ret =0;

		if (progress.HasUserCancelled())
			break;
	}
	return ret;
}
int CGitLogList::CherryPickFrom(CString from, CString to)
{
	CLogDataVector logs(&m_LogCache);
	CString range;
	range.Format(L"%s..%s", static_cast<LPCTSTR>(from), static_cast<LPCTSTR>(to));
	if (logs.ParserFromLog(nullptr, 0, 0, &range))
		return -1;

	if (logs.empty())
		return 0;

	CSysProgressDlg progress;
	progress.SetTitle(CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CHERRYPICK)));
	progress.SetTime(true);
	progress.ShowModeless(this);

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	for (int i = static_cast<int>(logs.size()) - 1; i >= 0; --i)
	{
		if (progress.IsVisible())
		{
			progress.FormatNonPathLine(1, IDS_PROC_PICK, static_cast<LPCTSTR>(logs.GetGitRevAt(i).m_CommitHash.ToString()));
			progress.FormatNonPathLine(2, L"%s", static_cast<LPCTSTR>(logs.GetGitRevAt(i).GetSubject()));
			progress.SetProgress64(logs.size() - i, logs.size());
		}
		if (progress.HasUserCancelled())
			throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_USERCANCELLED))));
		CString cmd,out;
		cmd.Format(L"git.exe cherry-pick %s", static_cast<LPCTSTR>(logs.GetGitRevAt(i).m_CommitHash.ToString()));
		if(g_Git.Run(cmd,&out,CP_UTF8))
			throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_CHERRYPICKFAILED)) + L":\r\n\r\n" + out));
	}

	return 0;
}

void CGitLogList::ContextMenuAction(int cmd, int FirstSelect, int LastSelect, CMenu* popmenu, MAP_HASH_NAME& hashMap)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	GitRevLoglist* pSelLogEntry = m_arShownList.SafeGetAt(indexNext);

	bool bShiftPressed = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	theApp.DoWaitCursor(1);
	switch (cmd&0xFFFF)
		{
			case ID_COMMIT:
			{
				CTGitPathList pathlist;
				pathlist.AddPath(this->m_Path);
				bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE));
				CString str;
				CAppUtils::Commit(GetParentHWND(), CString(), false, str,
								  pathlist, bSelectFilesForCommit);
				//this->Refresh();
				this->GetParent()->PostMessage(WM_COMMAND,ID_LOGDLG_REFRESH,0);
			}
			break;
			case ID_MERGE_ABORT:
			{
				if (CAppUtils::MergeAbort(GetParentHWND()))
					this->GetParent()->PostMessage(WM_COMMAND,ID_LOGDLG_REFRESH, 0);
			}
			break;
			case ID_GNUDIFF1: // compare with WC, unified
			{
				GitRev* r1 = m_arShownList.SafeGetAt(FirstSelect);
				if(!r1->m_CommitHash.IsEmpty())
				{
					CString hash2;
					bool bMerge = false, bCombine = false;
					CString merge;
					cmd >>= 16;
					if( (cmd&0xFFFF) == 0xFFFF)
						bMerge = true;
					else if((cmd&0xFFFF) == 0xFFFE)
						bCombine = true;
					else if ((cmd & 0xFFFF) == 0xFFFD)
					{
						CString tempfile = GetTempFile();
						CString gitcmd = L"git.exe diff-tree --cc " + r1->m_CommitHash.ToString();
						CString lastErr;
						if (g_Git.RunLogFile(gitcmd, tempfile, &lastErr))
						{
							MessageBox(lastErr, L"TortoiseGit", MB_ICONERROR);
							break;
						}

						try
						{
							CStdioFile file(tempfile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
							CString strLine;
							bool isHash = file.ReadString(strLine) && r1->m_CommitHash.ToString() == strLine;
							bool more = isHash && file.ReadString(strLine) && !strLine.IsEmpty();
							if (!more)
							{
								CMessageBox::Show(GetParentHWND(), IDS_NOCHANGEAFTERMERGE, IDS_APPNAME, MB_OK);
								break;
							}
						}
						catch (CFileException* e)
						{
							e->Delete();
						}

						CAppUtils::StartUnifiedDiffViewer(tempfile, r1->m_CommitHash.ToString());
						break;
					}
					else
					{
						if (cmd == 0)
							cmd = 1;
						if (static_cast<size_t>(cmd) > r1->m_ParentHash.size())
						{
							CString str;
							str.Format(IDS_PROC_NOPARENT, cmd);
							MessageBox(str, L"TortoiseGit", MB_OK | MB_ICONERROR);
							return;
						}
						else
						{
							if(cmd>0)
								hash2 = r1->m_ParentHash[cmd-1].ToString();
						}
					}
					if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
						CAppUtils::StartShowUnifiedDiff(nullptr, m_Path, hash2, m_Path, r1->m_CommitHash.ToString(), bShiftPressed, false, false, bMerge, bCombine);
					else
					{
						CString path = m_Path.GetGitPathString();
						// start with 1 (0 = working copy changes)
						for (int i = m_bShowWC ? 1 : 0; i < FirstSelect; ++i)
						{
							GitRevLoglist* first = m_arShownList.SafeGetAt(i);
							CTGitPathList list = first->GetFiles(nullptr);
							const CTGitPath* file = list.LookForGitPath(path);
							if (file && !file->GetGitOldPathString().IsEmpty())
								path = file->GetGitOldPathString();
						}
						CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(path), hash2, CTGitPath(path), r1->m_CommitHash.ToString(), bShiftPressed, false, false, bMerge, bCombine);
					}
				}
				else
					CAppUtils::StartShowUnifiedDiff(nullptr, m_Path, L"HEAD", m_Path, GitRev::GetWorkingCopy(), bShiftPressed);
			}
			break;

			case ID_GNUDIFF2: // compare two revisions, unified
			{
				GitRev* r1 = m_arShownList.SafeGetAt(FirstSelect);
				GitRev* r2 = m_arShownList.SafeGetAt(LastSelect);
				if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
					CAppUtils::StartShowUnifiedDiff(nullptr, m_Path, r2->m_CommitHash.ToString(), m_Path, r1->m_CommitHash.ToString(), bShiftPressed);
				else
				{
					CString path = m_Path.GetGitPathString();
					// start with 1 (0 = working copy changes)
					for (int i = m_bShowWC ? 1 : 0; i < FirstSelect; ++i)
					{
						GitRevLoglist* first = m_arShownList.SafeGetAt(i);
						CTGitPathList list = first->GetFiles(nullptr);
						const CTGitPath* file = list.LookForGitPath(path);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path = file->GetGitOldPathString();
					}
					CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(path), r2->m_CommitHash.ToString(), CTGitPath(path), r1->m_CommitHash.ToString(), bShiftPressed);
				}
			}
			break;

		case ID_COMPARETWO: // compare two revisions
			{
				GitRev* r1 = m_arShownList.SafeGetAt(FirstSelect);
				GitRev* r2 = m_arShownList.SafeGetAt(LastSelect);
				if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
					CGitDiff::DiffCommit(GetParentHWND(), m_Path, r1, r2, bShiftPressed);
				else
				{
					CString path1 = m_Path.GetGitPathString();
					// start with 1 (0 = working copy changes)
					for (int i = m_bShowWC ? 1 : 0; i < FirstSelect; ++i)
					{
						GitRevLoglist* first = m_arShownList.SafeGetAt(i);
						CTGitPathList list = first->GetFiles(nullptr);
						const CTGitPath* file = list.LookForGitPath(path1);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path1 = file->GetGitOldPathString();
					}
					CString path2 = path1;
					for (int i = FirstSelect; i < LastSelect; ++i)
					{
						GitRevLoglist* first = m_arShownList.SafeGetAt(i);
						CTGitPathList list = first->GetFiles(nullptr);
						const CTGitPath* file = list.LookForGitPath(path2);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path2 = file->GetGitOldPathString();
					}
					CGitDiff::DiffCommit(GetParentHWND(), CTGitPath(path1), CTGitPath(path2), r1, r2, bShiftPressed);
				}

			}
			break;

		case ID_COMPARE: // compare revision with WC
			{
				GitRevLoglist* r1 = &m_wcRev;
				GitRevLoglist* r2 = pSelLogEntry;

				if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
					CGitDiff::DiffCommit(GetParentHWND(), m_Path, r1, r2, bShiftPressed);
				else
				{
					CString path1 = m_Path.GetGitPathString();
					// start with 1 (0 = working copy changes)
					for (int i = m_bShowWC ? 1 : 0; i < FirstSelect; ++i)
					{
						GitRevLoglist* first = m_arShownList.SafeGetAt(i);
						CTGitPathList list = first->GetFiles(nullptr);
						const CTGitPath* file = list.LookForGitPath(path1);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path1 = file->GetGitOldPathString();
					}
					CGitDiff::DiffCommit(GetParentHWND(), m_Path, CTGitPath(path1), r1, r2, bShiftPressed);
				}

				//user clicked on the menu item "compare with working copy"
				//if (PromptShown())
				//{
				//	GitDiff diff(this, m_hWnd, true);
				//	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				//	diff.SetHEADPeg(m_LogRevision);
				//	diff.ShowCompare(m_path, GitRev::REV_WC, m_path, revSelected);
				//}
				//else
				//	CAppUtils::StartShowCompare(m_hWnd, m_path, GitRev::REV_WC, m_path, revSelected, GitRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;

		case ID_COMPAREWITHPREVIOUS:
			{
				if (pSelLogEntry->m_ParentHash.empty())
				{
					if (pSelLogEntry->GetParentFromHash(pSelLogEntry->m_CommitHash))
						MessageBox(pSelLogEntry->GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				}

				if (!pSelLogEntry->m_ParentHash.empty())
				//if(m_logEntries.m_HashMap[pSelLogEntry->m_ParentHash[0]]>=0)
				{
					cmd>>=16;
					cmd&=0xFFFF;

					if(cmd == 0)
						cmd=1;

					if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
						CGitDiff::DiffCommit(GetParentHWND(), m_Path, pSelLogEntry->m_CommitHash.ToString(), pSelLogEntry->m_ParentHash[cmd - 1].ToString(), bShiftPressed);
					else
					{
						CString path1 = m_Path.GetGitPathString();
						// start with 1 (0 = working copy changes)
						for (int i = m_bShowWC ? 1 : 0; i < indexNext; ++i)
						{
							GitRevLoglist* first = m_arShownList.SafeGetAt(i);
							CTGitPathList list = first->GetFiles(nullptr);
							const CTGitPath* file = list.LookForGitPath(path1);
							if (file && !file->GetGitOldPathString().IsEmpty())
								path1 = file->GetGitOldPathString();
						}
						CString path2 = path1;
						GitRevLoglist* first = m_arShownList.SafeGetAt(indexNext);
						CTGitPathList list = first->GetFiles(nullptr);
						const CTGitPath* file = list.LookForGitPath(path2);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path2 = file->GetGitOldPathString();

						CGitDiff::DiffCommit(GetParentHWND(), CTGitPath(path1), CTGitPath(path2), pSelLogEntry->m_CommitHash.ToString(), pSelLogEntry->m_ParentHash[cmd - 1].ToString(), bShiftPressed);
					}
				}
				else
				{
					CMessageBox::Show(GetParentHWND(), IDS_PROC_NOPREVIOUSVERSION, IDS_APPNAME, MB_OK);
				}
				//if (PromptShown())
				//{
				//	GitDiff diff(this, m_hWnd, true);
				//	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				//	diff.SetHEADPeg(m_LogRevision);
				//	diff.ShowCompare(CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected);
				//}
				//else
				//	CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;
		case ID_COMPARETWOCOMMITCHANGES:
			{
				auto pFirstEntry = m_arShownList.SafeGetAt(FirstSelect);
				auto pLastEntry = m_arShownList.SafeGetAt(LastSelect);
				CString patch1 = GetTempFile();
				CString patch2 = GetTempFile();
				if (CString err; g_Git.RunLogFile(L"git.exe format-patch --stdout " + pFirstEntry->m_CommitHash.ToString() + L"~1.." + pFirstEntry->m_CommitHash.ToString() + L"", patch1, &err))
				{
					MessageBox(L"Could not generate patch for commit " + pFirstEntry->m_CommitHash.ToString() + L".\n" + err, L"TortoiseGit", MB_ICONERROR);
					break;
				}
				if (CString err; g_Git.RunLogFile(L"git.exe format-patch --stdout " + pLastEntry->m_CommitHash.ToString() + L"~1.." + pLastEntry->m_CommitHash.ToString() + L"", patch2, &err))
				{
					MessageBox(L"Could not generate patch for commit " + pLastEntry->m_CommitHash.ToString() + L".\n" + err, L"TortoiseGit", MB_ICONERROR);
					break;
				}
				CAppUtils::DiffFlags flags;
				CAppUtils::StartExtDiff(patch1, patch2, pFirstEntry->m_CommitHash.ToString(), pLastEntry->m_CommitHash.ToString(), pFirstEntry->m_CommitHash.ToString() + L".patch", pLastEntry->m_CommitHash.ToString() + L".patch", pFirstEntry->m_CommitHash, pLastEntry->m_CommitHash, flags);
			}
			break;
		case ID_LOG_VIEWRANGE:
		case ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE:
			{
				GitRev* pLastEntry = m_arShownList.SafeGetAt(LastSelect);

				CString sep = L"..";
				if ((cmd & 0xFFFF) == ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE)
					sep = L"...";

				CString cmdline;
				cmdline.Format(L"/command:log /path:\"%s\" /range:\"%s%s%s\"",
					static_cast<LPCTSTR>(g_Git.CombinePath(m_Path)), static_cast<LPCTSTR>(pLastEntry->m_CommitHash.ToString()), static_cast<LPCTSTR>(sep), static_cast<LPCTSTR>(pSelLogEntry->m_CommitHash.ToString()));
				CAppUtils::RunTortoiseGitProc(cmdline);
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
			{
				CopySelectionToClipBoard(cmd & 0xFFFF);
			}
			break;
		case ID_COPYCLIPBOARDBRANCHTAG:
			{
				if (!popmenu)
					break;
				auto selectedBranch = reinterpret_cast<const CString*>(reinterpret_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd));
				CString sClipboard;
				if (selectedBranch)
				{
					if (CStringUtils::StartsWith(*selectedBranch, L"refs/tags/"))
					{
						if (CStringUtils::EndsWith(*selectedBranch, L"^{}"))
							sClipboard = selectedBranch->Mid(static_cast<int>(wcslen(L"refs/tags/")), selectedBranch->GetLength() - static_cast<int>(wcslen(L"refs/tags/")) - static_cast<int>(wcslen(L"^{}")));
						else
							sClipboard = selectedBranch->Mid(static_cast<int>(wcslen(L"refs/tags/")));
					}
					else
						sClipboard = CGit::StripRefName(*selectedBranch);
				}
				else if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend())
				{
					for (const auto& ref : hashMap[pSelLogEntry->m_CommitHash])
					{
						if (CStringUtils::StartsWith(ref, L"refs/tags/") && CStringUtils::EndsWith(ref, L"^{}"))
							sClipboard += ref.Left(ref.GetLength() - static_cast<int>(wcslen(L"^{}")));
						else
							sClipboard += ref;
						sClipboard += L"\r\n";
					}
				}
				CStringUtils::WriteAsciiStringToClipboard(sClipboard, GetSafeHwnd());
			}
			break;
		case ID_EXPORT:
			{
				CString str=pSelLogEntry->m_CommitHash.ToString();
				// try to get the tag
				if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend())
					GetFirstEntryStartingWith(hashMap[pSelLogEntry->m_CommitHash], L"refs/tags/", str);
				CAppUtils::Export(GetParentHWND(), &str, &m_Path);
			}
			break;
		case ID_CREATE_BRANCH:
		case ID_CREATE_TAG:
			{
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd & 0xFFFF)) : nullptr;
				CString str = pSelLogEntry->m_CommitHash.ToString();
				if (branch)
					str = *branch;
				else if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend()) // try to guess remote branch in order to enable tracking
					GetFirstEntryStartingWith(hashMap[pSelLogEntry->m_CommitHash], L"refs/remotes/", str);

				CAppUtils::CreateBranchTag(GetParentHWND(), (cmd & 0xFFFF) == ID_CREATE_TAG, &str);
				ReloadHashMap();
				if (m_pFindDialog)
					m_pFindDialog->RefreshList();
				Invalidate();
				::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
			}
			break;
		case ID_SWITCHTOREV:
			{
				CString str = pSelLogEntry->m_CommitHash.ToString();
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd & 0xFFFF)) : nullptr;
				if (branch)
					str = *branch;
				else if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend()) // try to guess remote branch in order to recommend good branch name and tracking
					GetFirstEntryStartingWith(hashMap[pSelLogEntry->m_CommitHash], L"refs/remotes/", str);

				CAppUtils::Switch(GetParentHWND(), str);
			}
			ReloadHashMap();
			Invalidate();
			::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
			break;
		case ID_SWITCHBRANCH:
			if(popmenu)
			{
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd)) : nullptr;
				if(branch)
				{
					CString name = *branch;
					CGit::GetShortName(*branch, name, L"refs/heads/");
					CAppUtils::PerformSwitch(GetParentHWND(), name);
				}
				ReloadHashMap();
				Invalidate();
				::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
			}
			break;
		case ID_RESET:
			{
				if (CAppUtils::GitReset(GetParentHWND(), pSelLogEntry->m_CommitHash.ToString()))
				{
					ResetWcRev(true);
					ReloadHashMap();
					Invalidate();
				}
			}
			break;
		case ID_REBASE_PICK:
			SetSelectedRebaseAction(LOGACTIONS_REBASE_PICK);
			break;
		case ID_REBASE_EDIT:
			SetSelectedRebaseAction(LOGACTIONS_REBASE_EDIT);
			break;
		case ID_REBASE_SQUASH:
			SetSelectedRebaseAction(LOGACTIONS_REBASE_SQUASH);
			break;
		case ID_REBASE_SKIP:
			SetSelectedRebaseAction(LOGACTIONS_REBASE_SKIP);
			break;
		case ID_COMBINE_COMMIT:
		{
			CString head;
			CGitHash headhash;
			CGitHash hashFirst,hashLast;

			int headindex=GetHeadIndex();
			if(headindex>=0) //incase show all branch, head is not the first commits.
			{
				head.Format(L"HEAD~%d", FirstSelect - headindex);
				if (g_Git.GetHash(hashFirst, head))
				{
					MessageBox(g_Git.GetGitLastErr(L"Could not get hash of first selected revision."), L"TortoiseGit", MB_ICONERROR);
					break;
				}

				head.Format(L"HEAD~%d", LastSelect - headindex);
				if (g_Git.GetHash(hashLast, head))
				{
					MessageBox(g_Git.GetGitLastErr(L"Could not get hash of last selected revision."), L"TortoiseGit", MB_ICONERROR);
					break;
				}
			}

			GitRev* pFirstEntry = m_arShownList.SafeGetAt(FirstSelect);
			GitRev* pLastEntry = m_arShownList.SafeGetAt(LastSelect);
			if(pFirstEntry->m_CommitHash != hashFirst || pLastEntry->m_CommitHash != hashLast)
			{
				CMessageBox::Show(GetParentHWND(), IDS_PROC_CANNOTCOMBINE, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
				break;
			}

			GitRev lastRevision;
			if (lastRevision.GetParentFromHash(hashLast))
			{
				MessageBox(lastRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				break;
			}

			if (g_Git.GetHash(headhash, L"HEAD"))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
				break;
			}

			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(GetParentHWND(), IDS_PROC_NOCLEAN, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			CString sCmd, out;

			//Use throw to abort this process (reset back to original HEAD)
			try
			{
				sCmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(pFirstEntry->m_CommitHash.ToString()));
				if(g_Git.Run(sCmd, &out, CP_UTF8))
				{
					MessageBox(out, L"TortoiseGit", MB_OK | MB_ICONERROR);
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORSTEP1)) + L"\r\n\r\n" + out));
				}
				sCmd.Format(L"git.exe reset --soft %s --", static_cast<LPCTSTR>(hashLast.ToString()));
				if(g_Git.Run(sCmd, &out, CP_UTF8))
				{
					MessageBox(out, L"TortoiseGit", MB_OK | MB_ICONERROR);
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORSTEP2)) + L"\r\n\r\n"+out));
				}

				CCommitDlg dlg;
				for (int i = FirstSelect; i <= LastSelect; ++i)
				{
					GitRev* pRev = m_arShownList.SafeGetAt(i);
					dlg.m_sLogMessage += pRev->GetSubject() + L'\n' + pRev->GetBody();
					dlg.m_sLogMessage += L'\n';
				}
				dlg.m_bWholeProject=true;
				dlg.m_bSelectFilesForCommit = true;
				dlg.m_bForceCommitAmend=true;
				int squashDate = CRegDWORD(L"Software\\TortoiseGit\\SquashDate", 0);
				if (squashDate == 1)
					dlg.SetTime(m_arShownList.SafeGetAt(FirstSelect)->GetAuthorDate());
				else if (squashDate == 2)
					dlg.SetTime(CTime::GetCurrentTime());
				else
					dlg.SetTime(m_arShownList.SafeGetAt(LastSelect)->GetAuthorDate());
				CTGitPathList gpl;
				gpl.AddPath(CTGitPath(g_Git.m_CurrentDir));
				dlg.m_pathList = gpl;
				if (lastRevision.ParentsCount() != 1)
				{
					MessageBox(L"The following commit dialog can only show changes of oldest commit if it has exactly one parent. This is not the case right now.", L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
					dlg.m_bAmendDiffToLastCommit = TRUE;
				}
				else
					dlg.m_bAmendDiffToLastCommit = FALSE;
				dlg.m_bNoPostActions=true;
				dlg.m_AmendStr=dlg.m_sLogMessage;

				if (dlg.DoModal() == IDOK)
				{
					if(pFirstEntry->m_CommitHash!=headhash)
					{
						if (CherryPickFrom(pFirstEntry->m_CommitHash.ToString(), headhash.ToString()))
						{
							CString msg;
							msg.Format(L"Error while cherry pick commits on top of combined commits. Aborting.\r\n\r\n");
							throw std::exception(CUnicodeUtils::GetUTF8(msg));
						}
					}
				}
				else
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_USERCANCELLED))));
			}
			catch(std::exception& e)
			{
				CMessageBox::Show(GetParentHWND(), CUnicodeUtils::GetUnicode(CStringA(e.what())), L"TortoiseGit", MB_OK | MB_ICONERROR);
				sCmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(headhash.ToString()));
				out.Empty();
				if(g_Git.Run(sCmd, &out, CP_UTF8))
					MessageBox(CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORRESETHEAD)) + L"\r\n\r\n" + out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			}
			Refresh();
		}
			break;

		case ID_CHERRY_PICK:
			{
				if (m_bThreadRunning)
				{
					CMessageBox::Show(GetParentHWND(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
					break;
				}
				CRebaseDlg dlg;
				dlg.m_IsCherryPick = TRUE;
				dlg.m_Upstream = this->m_CurrentBranch;
				POSITION pos2 = GetFirstSelectedItemPosition();
				while(pos2)
				{
					int indexNext2 = GetNextSelectedItem(pos2);
					dlg.m_CommitList.m_logEntries.push_back(m_arShownList.SafeGetAt(indexNext2)->m_CommitHash);
					dlg.m_CommitList.m_LogCache.m_HashMap[m_arShownList.SafeGetAt(indexNext2)->m_CommitHash] = *m_arShownList.SafeGetAt(indexNext2);
					dlg.m_CommitList.m_logEntries.GetGitRevAt(dlg.m_CommitList.m_logEntries.size() - 1).GetRebaseAction() |= LOGACTIONS_REBASE_PICK;
				}

				if(dlg.DoModal() == IDOK)
				{
					Refresh();
				}
			}
			break;
		case ID_REBASE_TO_VERSION:
			{
				if (m_bThreadRunning)
				{
					CMessageBox::Show(GetParentHWND(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
					break;
				}
				CRebaseDlg dlg;
				dlg.m_Upstream = pSelLogEntry->m_CommitHash.ToString();
				// try to guess a branch, optimally a local branch
				if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend())
				{
					const auto& refList = hashMap[pSelLogEntry->m_CommitHash];
					if (!refList.empty())
						dlg.m_Upstream = refList.front();
					for (const auto& ref : refList)
					{
						if (CGit::GetShortName(ref, dlg.m_Upstream, L"refs/heads/"))
							break;
					}
				}

				if(dlg.DoModal() == IDOK)
				{
					Refresh();
				}
			}

			break;

		case ID_STASH_SAVE:
			if (CAppUtils::StashSave(GetParentHWND()))
				Refresh();
			break;

		case ID_STASH_POP:
			if (CAppUtils::StashPop(GetParentHWND()))
				Refresh();
			break;

		case ID_STASH_LIST:
			CAppUtils::RunTortoiseGitProc(L"/command:reflog /ref:refs/stash");
			break;

		case ID_REFLOG_STASH_APPLY:
			CAppUtils::StashApply(GetParentHWND(), pSelLogEntry->m_Ref);
			break;

		case ID_REFLOG_DEL:
			{
				CString str;
				if (GetSelectedCount() > 1)
					str.Format(IDS_PROC_DELETENREFS, GetSelectedCount());
				else
					str.Format(IDS_PROC_DELETEREF, static_cast<LPCTSTR>(pSelLogEntry->m_Ref));

				if (CMessageBox::Show(GetParentHWND(), str, L"TortoiseGit", 1, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
					return;

				std::vector<CString> refsToDelete;
				POSITION pos2 = GetFirstSelectedItemPosition();
				while (pos2)
				{
					CString ref = m_arShownList.SafeGetAt(GetNextSelectedItem(pos2))->m_Ref;
					if (CStringUtils::StartsWith(ref, L"refs/"))
						ref = ref.Mid(static_cast<int>(wcslen(L"refs/")));
					int refpos = ref.ReverseFind(L'{');
					if (refpos > 0 && ref.Mid(refpos - 1, 2) != L"@{")
						ref = ref.Left(refpos) + L'@'+ ref.Mid(refpos);
					refsToDelete.push_back(ref);
				}

				for (auto revIt = refsToDelete.crbegin(); revIt != refsToDelete.crend(); ++revIt)
				{
					CString ref = *revIt;
					CString sCmd, out;
					if (CStringUtils::StartsWith(ref, L"stash"))
						sCmd.Format(L"git.exe stash drop %s", static_cast<LPCTSTR>(ref));
					else
						sCmd.Format(L"git.exe reflog delete %s", static_cast<LPCTSTR>(ref));

					if (g_Git.Run(sCmd, &out, CP_UTF8))
						MessageBox(out, L"TortoiseGit", MB_OK | MB_ICONERROR);

					::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
				}
			}
			break;
		case ID_LOG:
			{
				CString sCmd = L"/command:log";
				sCmd += L" /path:\"" + g_Git.m_CurrentDir + L"\" ";
				GitRev* r1 = m_arShownList.SafeGetAt(FirstSelect);
				sCmd += L" /endrev:" + r1->m_CommitHash.ToString();
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_CREATE_PATCH:
			{
				int select=this->GetSelectedCount();
				CString sCmd = L"/command:formatpatch";
				sCmd += L" /path:\"" + g_Git.m_CurrentDir + L"\" ";

				GitRev* r1 = m_arShownList.SafeGetAt(FirstSelect);
				GitRev* r2 = nullptr;
				if(select == 1)
				{
					sCmd += L" /startrev:" + r1->m_CommitHash.ToString();
				}
				else
				{
					r2 = m_arShownList.SafeGetAt(LastSelect);
					if( this->m_IsOldFirst )
					{
						sCmd += L" /startrev:" + r1->m_CommitHash.ToString() + L"~1";
						sCmd += L" /endrev:" + r2->m_CommitHash.ToString();

					}
					else
					{
						sCmd += L" /startrev:" + r2->m_CommitHash.ToString() + L"~1";
						sCmd += L" /endrev:" + r1->m_CommitHash.ToString();
					}

				}

				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_BISECTSTART:
			{
				GitRev* first = m_arShownList.SafeGetAt(FirstSelect);
				GitRev* last = m_arShownList.SafeGetAt(LastSelect);
				ASSERT(first && last);

				CString firstBad = first->m_CommitHash.ToString();
				if (hashMap.find(first->m_CommitHash) != hashMap.cend() && !hashMap[first->m_CommitHash].empty())
					firstBad = hashMap[first->m_CommitHash].at(0);
				CString lastGood = last->m_CommitHash.ToString();
				if (hashMap.find(last->m_CommitHash) != hashMap.cend() && !hashMap[last->m_CommitHash].empty())
					lastGood = hashMap[last->m_CommitHash].at(0);

				if (CAppUtils::BisectStart(GetParentHWND(), lastGood, firstBad))
					Refresh();
			}
			break;
		case ID_BISECTGOOD:
			{
				GitRev* first = m_arShownList.SafeGetAt(FirstSelect);
				if (CAppUtils::BisectOperation(GetParentHWND(), L"good", !first->m_CommitHash.IsEmpty() ? first->m_CommitHash.ToString() : L""))
					Refresh();
			}
			break;
		case ID_BISECTBAD:
			{
				GitRev* first = m_arShownList.SafeGetAt(FirstSelect);
				if (CAppUtils::BisectOperation(GetParentHWND(), L"bad", !first->m_CommitHash.IsEmpty() ? first->m_CommitHash.ToString() : L""))
					Refresh();
			}
			break;
		case ID_BISECTSKIP:
		{
			CString refs;
			POSITION pos2 = GetFirstSelectedItemPosition();
			while (pos2)
			{
				int indexNext2 = GetNextSelectedItem(pos2);
				auto rev = m_arShownList.SafeGetAt(indexNext2);
				if (!rev->m_CommitHash.IsEmpty())
					refs.AppendFormat(L" %s", static_cast<LPCTSTR>(rev->m_CommitHash.ToString()));
			}
			if (CAppUtils::BisectOperation(GetParentHWND(), L"skip", refs))
				Refresh();
		}
		break;
		case ID_BISECTRESET:
			{
				if (CAppUtils::BisectOperation(GetParentHWND(), L"reset"))
					Refresh();
			}
			break;
		case ID_REPOBROWSE:
			{
				CString sCmd;
				sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%s", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(pSelLogEntry->m_CommitHash.ToString()));
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_PUSH:
			{
				CString guessAssociatedBranch = pSelLogEntry->m_CommitHash.ToString();
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd)) : nullptr;
				if (branch && !CStringUtils::StartsWith(*branch, L"refs/remotes/"))
					guessAssociatedBranch = *branch;
				else if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend())
				{
					if (!GetFirstEntryStartingWith(hashMap[pSelLogEntry->m_CommitHash], L"refs/heads/", guessAssociatedBranch))
						GetFirstEntryStartingWith(hashMap[pSelLogEntry->m_CommitHash], L"refs/tags/", guessAssociatedBranch);
				}

				if (CStringUtils::EndsWith(guessAssociatedBranch, L"^{}"))
					guessAssociatedBranch.Truncate(guessAssociatedBranch.GetLength() - static_cast<int>(wcslen(L"^{}")));

				if (CAppUtils::Push(GetParentHWND(), guessAssociatedBranch))
					Refresh();
			}
			break;
		case ID_PULL:
			{
				if (CAppUtils::Pull(GetParentHWND()))
					Refresh();
			}
			break;
		case ID_FETCH:
			{
				if (CAppUtils::Fetch(GetParentHWND()))
					Refresh();
			}
			break;
		case ID_SVNDCOMMIT:
		{
			if (CAppUtils::SVNDCommit(GetParentHWND()))
				Refresh();
		}
		break;
		case ID_CLEANUP:
			{
				CString sCmd;
				sCmd.Format(L"/command:cleanup /path:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_SUBMODULE_UPDATE:
			{
				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_SHOWBRANCHES:
			{
				CCommitIsOnRefsDlg* dlg = new CCommitIsOnRefsDlg(this);
				dlg->m_Rev = static_cast<LPCTSTR>(pSelLogEntry->m_CommitHash.ToString());
				dlg->Create(this);
				// pointer won't leak as it is destroyed within PostNcDestroy()
			}
			break;
		case ID_DELETE:
			{
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd)) : nullptr;
				if (!branch)
				{
					CMessageBox::Show(GetParentHWND(), IDS_ERROR_NOREF, IDS_APPNAME, MB_OK | MB_ICONERROR);
					return;
				}
				CString shortname;
				if (branch == reinterpret_cast<CString*>(MAKEINTRESOURCE(IDS_ALL)))
				{
					if (hashMap.find(pSelLogEntry->m_CommitHash) == hashMap.cend())
						return;
					CString currentBranch = L"refs/heads/" + m_CurrentBranch;
					bool nothingDeleted = true;
					for (const auto& ref : hashMap[pSelLogEntry->m_CommitHash])
					{
						if (ref == currentBranch)
							continue;
						if (!CAppUtils::DeleteRef(this, ref))
							break;
						nothingDeleted = false;
					}
					if (nothingDeleted)
						return;
				}
				else if (!CAppUtils::DeleteRef(this, *branch))
					return;
				this->ReloadHashMap();
				if (m_pFindDialog)
					m_pFindDialog->RefreshList();
				CRect rect;
				this->GetItemRect(FirstSelect,&rect,LVIR_BOUNDS);
				this->InvalidateRect(rect);
			}
			break;

		case ID_FINDENTRY:
			{
				m_nSearchIndex = GetSelectionMark();
				if (m_nSearchIndex < 0)
					m_nSearchIndex = 0;
				if (m_pFindDialog)
					break;
				else
				{
					m_pFindDialog = new CFindDlg();
					m_pFindDialog->Create(this);
				}
			}
			break;
		case ID_MERGEREV:
			{
				CString str = pSelLogEntry->m_CommitHash.ToString();
				auto branch = popmenu ? reinterpret_cast<const CString*>(static_cast<CIconMenu*>(popmenu)->GetMenuItemData(cmd & 0xFFFF)) : nullptr;
				if (branch)
					str = *branch;
				else if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend() && !hashMap[pSelLogEntry->m_CommitHash].empty())
					str = hashMap[pSelLogEntry->m_CommitHash].at(0);
				// we need an URL to complete this command, so error out if we can't get an URL
				if (CAppUtils::Merge(GetParentHWND(), &str))
				{
					this->Refresh();
				}
			}
		break;
		case ID_REVERTREV:
			{
				int parent = 0;
				if (GetSelectedCount() == 1)
				{
					parent = cmd >> 16;
					if (static_cast<size_t>(parent) > pSelLogEntry->m_ParentHash.size())
					{
						CString str;
						str.Format(IDS_PROC_NOPARENT, parent);
						MessageBox(str, L"TortoiseGit", MB_OK | MB_ICONERROR);
						return;
					}
				}

				if (!this->RevertSelectedCommits(parent))
				{
					if (CMessageBox::Show(m_hWnd, IDS_REVREVERTED, IDS_APPNAME, 1, IDI_QUESTION, IDS_OKBUTTON, IDS_COMMITBUTTON) == 2)
					{
						CTGitPathList pathlist;
						pathlist.AddPath(this->m_Path);
						bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE));
						CString str;
						CAppUtils::Commit(GetParentHWND(), CString(), false, str, pathlist, bSelectFilesForCommit);
					}
					this->Refresh();
				}
			}
			break;
		case ID_EDITNOTE:
			{
				CAppUtils::EditNote(GetParentHWND(), pSelLogEntry, &m_ProjectProperties);
				this->SetItemState(FirstSelect,  0, LVIS_SELECTED);
				this->SetItemState(FirstSelect,  LVIS_SELECTED, LVIS_SELECTED);
			}
			break;
		default:
			//CMessageBox::Show(nullptr, L"Have not implemented", L"TortoiseGit", MB_OK);
			break;
#if 0

		case ID_BLAMECOMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				//now first get the revision which is selected
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(m_path, GitRev::REV_BASE, m_path, revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, m_path, GitRev::REV_BASE, m_path, revSelected, GitRev(), m_LogRevision, false, false, true);
			}
			break;
		case ID_BLAMEWITHPREVIOUS:
			{
				//user clicked on the menu item "Compare and Blame with previous revision"
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, false, false, true);
			}
			break;

		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				CProgressDlg progDlg;
				progDlg.SetTitle(IDS_APPNAME);
				progDlg.SetAnimation(IDR_DOWNLOAD);
				CString sInfoLine;
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), static_cast<LPCTSTR>(revSelected.ToString()));
				progDlg.SetLine(1, sInfoLine, true);
				SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				CTGitPath tempfile = CTempFiles::Instance().GetTempFilePath(false, m_path, revSelected);
				bool bSuccess = true;
				if (!Cat(m_path, GitRev(GitRev::REV_HEAD), revSelected, tempfile))
				{
					bSuccess = false;
					// try again, but with the selected revision as the peg revision
					if (!Cat(m_path, revSelected, revSelected, tempfile))
					{
						progDlg.Stop();
						SetAndClearProgressInfo(nullptr);
						CMessageBox::Show(GetSafeHwnd(), GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
						EnableOKButton();
						break;
					}
					bSuccess = true;
				}
				if (bSuccess)
				{
					progDlg.Stop();
					SetAndClearProgressInfo(nullptr);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					if (!bOpenWith)
						CAppUtils::ShellOpen(tempfile.GetWinPath(), GetSafeHwnd());
					else
						CAppUtils::ShowOpenWithDialog(tempfile.GetWinPathString(), GetSafeHwnd());
				}
			}
			break;
		case ID_BLAME:
			{
				CBlameDlg dlg;
				dlg.EndRev = revSelected;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(m_path, dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, L"", dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = L"/path:\"" + m_path.GetGitPathString() + L"\" ";
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),sParams))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(GetSafeHwnd(), blame.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
					}
				}
			}
			break;
		case ID_EXPORT:
			{
				CString sCmd;
				sCmd.Format(L"%s /command:export /path:\"%s\" /revision:%ld",
					static_cast<LPCTSTR>(CPathUtils::GetAppDirectory() + L"TortoiseGitProc.exe"),
					static_cast<LPCTSTR>(pathURL), static_cast<LONG>(revSelected));
				CAppUtils::LaunchApplication(sCmd, nullptr, false);
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(L"%REVISION%", revSelected.ToString());
				if (!url.IsEmpty())
					ShellExecute(GetSafeHwnd(), L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
			}
			break;
		case ID_VIEWPATHREV:
			{
				CString relurl = pathURL;
				CString sRoot = GetRepositoryRoot(CTGitPath(relurl));
				relurl = relurl.Mid(sRoot.GetLength());
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(L"%REVISION%", revSelected.ToString());
				url.Replace(L"%PATH%", relurl);
				if (!url.IsEmpty())
					ShellExecute(GetSafeHwnd(), L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
			}
			break;
#endif

		} // switch (cmd)

		theApp.DoWaitCursor(-1);
}

void CGitLogList::SetSelectedRebaseAction(int action)
{
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos) return;
	while(pos)
	{
		auto index = GetNextSelectedItem(pos);
		if (m_arShownList.SafeGetAt(index)->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE) || (index == GetItemCount() - 1 && action == LOGACTIONS_REBASE_SQUASH))
			continue;
		if (!m_bIsCherryPick && m_arShownList.SafeGetAt(index)->ParentsCount() > 1 && action == LOGACTIONS_REBASE_SQUASH)
			continue;
		m_arShownList.SafeGetAt(index)->GetRebaseAction() = action;
		CRect rect;
		this->GetItemRect(index,&rect,LVIR_BOUNDS);
		this->InvalidateRect(rect);

	}
	GetParent()->PostMessage(CGitLogListBase::m_RebaseActionMessage);
}

void CGitLogList::SetUnselectedRebaseAction(int action)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int index = pos ? GetNextSelectedItem(pos) : -1;
	for (int i = 0; i < GetItemCount(); i++)
	{
		if (i == index)
		{
			index = pos ? GetNextSelectedItem(pos) : -1;
			continue;
		}

		if (m_arShownList.SafeGetAt(i)->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE) || (i == GetItemCount() - 1 && action == LOGACTIONS_REBASE_SQUASH) || (!m_bIsCherryPick && action == LOGACTIONS_REBASE_SQUASH && m_arShownList.SafeGetAt(i)->ParentsCount() != 1))
			continue;
		m_arShownList.SafeGetAt(i)->GetRebaseAction() = action;
		CRect rect;
		this->GetItemRect(i, &rect, LVIR_BOUNDS);
		this->InvalidateRect(rect);
	}

	GetParent()->PostMessage(CGitLogListBase::m_RebaseActionMessage);
}

void CGitLogList::ShiftSelectedRebaseAction()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while(pos)
	{
		auto index = GetNextSelectedItem(pos);
		int* action = &(m_arShownList.SafeGetAt(index))->GetRebaseAction();
		switch (*action)
		{
		case LOGACTIONS_REBASE_PICK:
			*action = LOGACTIONS_REBASE_SKIP;
			break;
		case LOGACTIONS_REBASE_SKIP:
			*action= LOGACTIONS_REBASE_EDIT;
			break;
		case LOGACTIONS_REBASE_EDIT:
			*action = LOGACTIONS_REBASE_SQUASH;
			if (index == GetItemCount() - 1 && (m_bIsCherryPick || m_arShownList.SafeGetAt(index)->m_ParentHash.size() == 1))
				*action = LOGACTIONS_REBASE_PICK;
			break;
		case LOGACTIONS_REBASE_SQUASH:
			*action= LOGACTIONS_REBASE_PICK;
			break;
		}
		CRect rect;
		this->GetItemRect(index, &rect, LVIR_BOUNDS);
		this->InvalidateRect(rect);
	}
}
