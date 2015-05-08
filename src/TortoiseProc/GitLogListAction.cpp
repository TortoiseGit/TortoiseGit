// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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
#include "FileDiffDlg.h"
#include "CommitDlg.h"
#include "RebaseDlg.h"
#include "GitDiff.h"
#include "../TGitCache/CacheInterface.h"

IMPLEMENT_DYNAMIC(CGitLogList, CHintListCtrl)

int CGitLogList::RevertSelectedCommits(int parent)
{
	CSysProgressDlg progress;
	int ret = -1;

#if 0
	if(!g_Git.CheckCleanWorkTree())
	{
		CMessageBox::Show(NULL, IDS_PROC_NOCLEAN, IDS_APPNAME, MB_OK);

	}
#endif

	if (this->GetSelectedCount() > 1)
	{
		progress.SetTitle(CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_REVERTCOMMIT)));
		progress.SetAnimation(IDR_MOVEANI);
		progress.SetTime(true);
		progress.ShowModeless(this);
	}

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	POSITION pos = GetFirstSelectedItemPosition();
	int i=0;
	while(pos)
	{
		int index = GetNextSelectedItem(pos);
		GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(index));

		if (progress.IsVisible())
		{
			progress.FormatNonPathLine(1, IDS_PROC_REVERTCOMMIT, r1->m_CommitHash.ToString());
			progress.FormatNonPathLine(2, _T("%s"), r1->GetSubject());
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
				CMessageBox::Show(NULL, str, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			else
			{
				if(CMessageBox::Show(NULL, str, _T("TortoiseGit"),2 , IDI_ERROR, CString(MAKEINTRESOURCE(IDS_SKIPBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
				{
					return ret;
				}
			}
		}
		else
		{
			ret =0;
		}

		if (progress.HasUserCancelled())
			break;
	}
	return ret;
}
int CGitLogList::CherryPickFrom(CString from, CString to)
{
	CLogDataVector logs(&m_LogCache);
	CString range;
	range.Format(_T("%s..%s"), from, to);
	if (logs.ParserFromLog(nullptr, -1, 0, &range))
		return -1;

	if (logs.empty())
		return 0;

	CSysProgressDlg progress;
	progress.SetTitle(CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CHERRYPICK)));
	progress.SetAnimation(IDR_MOVEANI);
	progress.SetTime(true);
	progress.ShowModeless(this);

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	for (int i = (int)logs.size() - 1; i >= 0; i--)
	{
		if (progress.IsVisible())
		{
			progress.FormatNonPathLine(1, IDS_PROC_PICK, logs.GetGitRevAt(i).m_CommitHash.ToString());
			progress.FormatNonPathLine(2, _T("%s"), logs.GetGitRevAt(i).GetSubject());
			progress.SetProgress64(logs.size() - i, logs.size());
		}
		if (progress.HasUserCancelled())
		{
			throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
		}
		CString cmd,out;
		cmd.Format(_T("git.exe cherry-pick %s"),logs.GetGitRevAt(i).m_CommitHash.ToString());
		out.Empty();
		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_CHERRYPICKFAILED)) + _T(":\r\n\r\n") + out));
		}
	}

	return 0;
}

void CGitLogList::ContextMenuAction(int cmd,int FirstSelect, int LastSelect, CMenu *popmenu)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	GitRevLoglist* pSelLogEntry = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(indexNext));

	theApp.DoWaitCursor(1);
	switch (cmd&0xFFFF)
		{
			case ID_COMMIT:
			{
				CTGitPathList pathlist;
				CTGitPathList selectedlist;
				pathlist.AddPath(this->m_Path);
				bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE));
				CString str;
				CAppUtils::Commit(CString(),false,str,
								  pathlist,selectedlist,bSelectFilesForCommit);
				//this->Refresh();
				this->GetParent()->PostMessage(WM_COMMAND,ID_LOGDLG_REFRESH,0);
			}
			break;
			case ID_MERGE_ABORT:
			{
				if (CAppUtils::MergeAbort())
					this->GetParent()->PostMessage(WM_COMMAND,ID_LOGDLG_REFRESH, 0);
			}
			break;
			case ID_GNUDIFF1: // compare with WC, unified
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				bool bMerge = false, bCombine = false;
				CString hash2;
				if(!r1->m_CommitHash.IsEmpty())
				{
					CString merge;
					cmd >>= 16;
					if( (cmd&0xFFFF) == 0xFFFF)
						bMerge = true;

					else if((cmd&0xFFFF) == 0xFFFE)
						bCombine = true;
					else
					{
						if(cmd > r1->m_ParentHash.size())
						{
							CString str;
							str.Format(IDS_PROC_NOPARENT, cmd);
							MessageBox(str, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
							return;
						}
						else
						{
							if(cmd>0)
								hash2 = r1->m_ParentHash[cmd-1].ToString();
						}
					}
					CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(), hash2, CTGitPath(), r1->m_CommitHash.ToString(), false, false, false, bMerge, bCombine);
				}
				else
					CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(), _T("HEAD"), CTGitPath(), GitRev::GetWorkingCopy(), false, false, false, bMerge, bCombine);
			}
			break;

			case ID_GNUDIFF2: // compare two revisions, unified
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(), r2->m_CommitHash.ToString(), CTGitPath(), r1->m_CommitHash.ToString());
			}
			break;

		case ID_COMPARETWO: // compare two revisions
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
					CGitDiff::DiffCommit(this->m_Path, r1,r2);
				else
				{
					CString path1 = m_Path.GetGitPathString();
					// start with 1 (0 = working copy changes)
					for (int i = 1; i < FirstSelect; ++i)
					{
						GitRevLoglist* first = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(i));
						CTGitPathList list = first->GetFiles(NULL);
						CTGitPath * file = list.LookForGitPath(path1);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path1 = file->GetGitOldPathString();
					}
					CString path2 = path1;
					for (int i = FirstSelect; i < LastSelect; ++i)
					{
						GitRevLoglist* first = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(i));
						CTGitPathList list = first->GetFiles(NULL);
						CTGitPath * file = list.LookForGitPath(path2);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path2 = file->GetGitOldPathString();
					}
					CGitDiff::DiffCommit(CTGitPath(path1), CTGitPath(path2), r1, r2);
				}

			}
			break;

		case ID_COMPARE: // compare revision with WC
			{
				GitRevLoglist* r1 = &m_wcRev;
				GitRevLoglist* r2 = pSelLogEntry;

				if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
					CGitDiff::DiffCommit(this->m_Path, r1,r2);
				else
				{
					CString path1 = m_Path.GetGitPathString();
					// start with 1 (0 = working copy changes)
					for (int i = 1; i < FirstSelect; ++i)
					{
						GitRevLoglist* first = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(i));
						CTGitPathList list = first->GetFiles(NULL);
						CTGitPath * file = list.LookForGitPath(path1);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path1 = file->GetGitOldPathString();
					}
					CGitDiff::DiffCommit(m_Path, CTGitPath(path1), r1, r2);
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
				CFileDiffDlg dlg;

				if (pSelLogEntry->m_ParentHash.empty())
				{
					if (pSelLogEntry->GetParentFromHash(pSelLogEntry->m_CommitHash))
						MessageBox(pSelLogEntry->GetLastErr(), _T("TortoiseGit"), MB_ICONERROR);
				}

				if (!pSelLogEntry->m_ParentHash.empty())
				//if(m_logEntries.m_HashMap[pSelLogEntry->m_ParentHash[0]]>=0)
				{
					cmd>>=16;
					cmd&=0xFFFF;

					if(cmd == 0)
						cmd=1;

					if (m_Path.IsDirectory() || !(m_ShowMask & CGit::LOG_INFO_FOLLOW))
						CGitDiff::DiffCommit(m_Path, pSelLogEntry->m_CommitHash.ToString(), pSelLogEntry->m_ParentHash[cmd - 1].ToString());
					else
					{
						CString path1 = m_Path.GetGitPathString();
						// start with 1 (0 = working copy changes)
						for (int i = 1; i < indexNext; ++i)
						{
							GitRevLoglist* first = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(i));
							CTGitPathList list = first->GetFiles(NULL);
							CTGitPath * file = list.LookForGitPath(path1);
							if (file && !file->GetGitOldPathString().IsEmpty())
								path1 = file->GetGitOldPathString();
						}
						CString path2 = path1;
						GitRevLoglist* first = reinterpret_cast<GitRevLoglist*>(m_arShownList.GetAt(indexNext));
						CTGitPathList list = first->GetFiles(NULL);
						CTGitPath * file = list.LookForGitPath(path2);
						if (file && !file->GetGitOldPathString().IsEmpty())
							path2 = file->GetGitOldPathString();

						CGitDiff::DiffCommit(CTGitPath(path1), CTGitPath(path2), pSelLogEntry->m_CommitHash.ToString(), pSelLogEntry->m_ParentHash[cmd - 1].ToString());
					}
				}
				else
				{
					CMessageBox::Show(NULL, IDS_PROC_NOPREVIOUSVERSION, IDS_APPNAME, MB_OK);
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
		case ID_LOG_VIEWRANGE:
		case ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE:
			{
				GitRev* pLastEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(LastSelect));

				CString sep = _T("..");
				if ((cmd & 0xFFFF) == ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE)
					sep = _T("...");

				CString cmdline;
				cmdline.Format(_T("/command:log /path:\"%s\" /range:\"%s%s%s\""),
					g_Git.CombinePath(m_Path), pLastEntry->m_CommitHash.ToString(), sep, pSelLogEntry->m_CommitHash.ToString());
				CAppUtils::RunTortoiseGitProc(cmdline);
			}
			break;
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			break;
		case ID_COPYCLIPBOARDMESSAGES:
			{
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
					CopySelectionToClipBoard(ID_COPY_SUBJECT);
				else
					CopySelectionToClipBoard(ID_COPY_MESSAGE);
			}
			break;
		case ID_COPYHASH:
			{
				CopySelectionToClipBoard(ID_COPY_HASH);
			}
			break;
		case ID_EXPORT:
			{
				CString str=pSelLogEntry->m_CommitHash.ToString();
				// try to get the tag
				for (size_t i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); ++i)
				{
					if (m_HashMap[pSelLogEntry->m_CommitHash][i].Find(_T("refs/tags/")) == 0)
					{
						str = m_HashMap[pSelLogEntry->m_CommitHash][i];
						break;
					}
				}
				CAppUtils::Export(&str, &m_Path);
			}
			break;
		case ID_CREATE_BRANCH:
		case ID_CREATE_TAG:
			{
				CString str = pSelLogEntry->m_CommitHash.ToString();
				// try to guess remote branch in order to enable tracking
				for (size_t i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); ++i)
				{
					if (m_HashMap[pSelLogEntry->m_CommitHash][i].Find(_T("refs/remotes/")) == 0)
					{
						str = m_HashMap[pSelLogEntry->m_CommitHash][i];
						break;
					}
				}
				CAppUtils::CreateBranchTag((cmd&0xFFFF) == ID_CREATE_TAG, &str);
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
				// try to guess remote branch in order to recommend good branch name and tracking
				for (size_t i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); ++i)
				{
					if (m_HashMap[pSelLogEntry->m_CommitHash][i].Find(_T("refs/remotes/")) == 0)
					{
						str = m_HashMap[pSelLogEntry->m_CommitHash][i];
						break;
					}
				}
				CAppUtils::Switch(str);
			}
			ReloadHashMap();
			Invalidate();
			::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
			break;
		case ID_SWITCHBRANCH:
			if(popmenu)
			{
				CString *branch = (CString*)((CIconMenu*)popmenu)->GetMenuItemData(cmd);
				if(branch)
				{
					CString name;
					if(branch->Find(_T("refs/heads/")) ==0 )
						name = branch->Mid(11);
					else
						name = *branch;

					CAppUtils::PerformSwitch(name);
				}
				ReloadHashMap();
				Invalidate();
				::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
			}
			break;
		case ID_RESET:
			{
				CString str = pSelLogEntry->m_CommitHash.ToString();
				if (CAppUtils::GitReset(&str))
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
				head.Format(_T("HEAD~%d"),FirstSelect-headindex);
				if (g_Git.GetHash(hashFirst, head))
				{
					MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of first selected revision.")), _T("TortoiseGit"), MB_ICONERROR);
					break;
				}

				head.Format(_T("HEAD~%d"),LastSelect-headindex);
				if (g_Git.GetHash(hashLast, head))
				{
					MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of last selected revision.")), _T("TortoiseGit"), MB_ICONERROR);
					break;
				}
			}

			GitRev* pFirstEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
			GitRev* pLastEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
			if(pFirstEntry->m_CommitHash != hashFirst || pLastEntry->m_CommitHash != hashLast)
			{
				CMessageBox::Show(NULL, IDS_PROC_CANNOTCOMBINE, IDS_APPNAME, MB_OK);
				break;
			}

			GitRev lastRevision;
			if (lastRevision.GetParentFromHash(hashLast))
			{
				MessageBox(lastRevision.GetLastErr(), _T("TortoiseGit"), MB_ICONERROR);
				break;
			}

			if (g_Git.GetHash(headhash, _T("HEAD")))
			{
				MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash.")), _T("TortoiseGit"), MB_ICONERROR);
				break;
			}

			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL, IDS_PROC_NOCLEAN, IDS_APPNAME, MB_OK);
				break;
			}
			CString cmd,out;

			//Use throw to abort this process (reset back to original HEAD)
			try
			{
				cmd.Format(_T("git.exe reset --hard %s --"), pFirstEntry->m_CommitHash.ToString());
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORSTEP1)) + _T("\r\n\r\n") + out));
				}
				cmd.Format(_T("git.exe reset --mixed %s --"), hashLast.ToString());
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORSTEP2)) + _T("\r\n\r\n")+out));
				}

				CTGitPathList PathList;
				/* don't why must add --stat to get action status*/
				/* first -z will be omitted by gitdll*/
				if(g_Git.GetDiffPath(&PathList,&pFirstEntry->m_CommitHash,&hashLast,"-z --stat -r"))
				{
					CMessageBox::Show(NULL,_T("Get Diff file list error"),_T("TortoiseGit"),MB_OK);
					throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not get changed file list aborting...\r\n\r\n")+out));
				}

				for (int i = 0; i < PathList.GetCount(); ++i)
				{
					if(PathList[i].m_Action & CTGitPath::LOGACTIONS_ADDED)
					{
						cmd.Format(_T("git.exe add -- \"%s\""), PathList[i].GetGitPathString());
						if (g_Git.Run(cmd, &out, CP_UTF8))
						{
							CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
							throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not add new file aborting...\r\n\r\n")+out));

						}
					}
					if(PathList[i].m_Action & CTGitPath::LOGACTIONS_DELETED)
					{
						cmd.Format(_T("git.exe rm -- \"%s\""), PathList[i].GetGitPathString());
						if (g_Git.Run(cmd, &out, CP_UTF8))
						{
							CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
							throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not rm file aborting...\r\n\r\n")+out));
						}
					}
				}

				CCommitDlg dlg;
				for (int i = FirstSelect; i <= LastSelect; ++i)
				{
					GitRev* pRev = reinterpret_cast<GitRev*>(m_arShownList.GetAt(i));
					dlg.m_sLogMessage+=pRev->GetSubject()+_T("\n")+pRev->GetBody();
					dlg.m_sLogMessage+=_T("\n");
				}
				dlg.m_bWholeProject=true;
				dlg.m_bSelectFilesForCommit = true;
				dlg.m_bForceCommitAmend=true;
				CTGitPathList gpl;
				gpl.AddPath(CTGitPath(g_Git.m_CurrentDir));
				dlg.m_pathList = gpl;
				if (lastRevision.ParentsCount() != 1)
				{
					CMessageBox::Show(NULL, _T("The following commit dialog can only show changes of oldest commit if it has exactly one parent. This is not the case right now."), _T("TortoiseGit"),MB_OK);
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
						if(CherryPickFrom(pFirstEntry->m_CommitHash.ToString(),headhash))
						{
							CString msg;
							msg.Format(_T("Error while cherry pick commits on top of combined commits. Aborting.\r\n\r\n"));
							throw std::exception(CUnicodeUtils::GetUTF8(msg));
						}
					}
				}
				else
					throw std::exception(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
			}
			catch(std::exception& e)
			{
				CMessageBox::Show(NULL, CUnicodeUtils::GetUnicode(CStringA(e.what())), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				cmd.Format(_T("git.exe reset --hard %s --"), headhash.ToString());
				out.Empty();
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL, CString(MAKEINTRESOURCE(IDS_PROC_COMBINE_ERRORRESETHEAD)) + _T("\r\n\r\n") + out, _T("TortoiseGit"), MB_OK);
				}
			}
			Refresh();
		}
			break;

		case ID_CHERRY_PICK:
			{
				if (m_bThreadRunning)
				{
					CMessageBox::Show(GetSafeHwnd(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
					break;
				}
				CRebaseDlg dlg;
				dlg.m_IsCherryPick = TRUE;
				dlg.m_Upstream = this->m_CurrentBranch;
				POSITION pos = GetFirstSelectedItemPosition();
				while(pos)
				{
					int indexNext = GetNextSelectedItem(pos);
					dlg.m_CommitList.m_logEntries.push_back(((GitRevLoglist*)m_arShownList[indexNext])->m_CommitHash);
					dlg.m_CommitList.m_LogCache.m_HashMap[((GitRevLoglist*)m_arShownList[indexNext])->m_CommitHash] = *(GitRevLoglist*)m_arShownList[indexNext];
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
					CMessageBox::Show(GetSafeHwnd(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
					break;
				}
				CRebaseDlg dlg;
				auto refList = m_HashMap[pSelLogEntry->m_CommitHash];
				dlg.m_Upstream = refList.empty() ? pSelLogEntry->m_CommitHash.ToString() : refList.front();
				for (auto ref : refList)
				{
					if (ref.Left(11) == _T("refs/heads/"))
					{
						// 11=len("refs/heads/")
						dlg.m_Upstream = ref.Mid(11);
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
			if (CAppUtils::StashSave())
				Refresh();
			break;

		case ID_STASH_POP:
			if (CAppUtils::StashPop())
				Refresh();
			break;

		case ID_STASH_LIST:
			CAppUtils::RunTortoiseGitProc(_T("/command:reflog /ref:refs/stash"));
			break;

		case ID_REFLOG_STASH_APPLY:
			CAppUtils::StashApply(pSelLogEntry->m_Ref);
			break;

		case ID_REFLOG_DEL:
			{
				CString str;
				if (GetSelectedCount() > 1)
					str.Format(IDS_PROC_DELETENREFS, GetSelectedCount());
				else
					str.Format(IDS_PROC_DELETEREF, pSelLogEntry->m_Ref);

				if (CMessageBox::Show(NULL, str, _T("TortoiseGit"), 1, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
					return;

				std::vector<CString> refsToDelete;
				POSITION pos = GetFirstSelectedItemPosition();
				while (pos)
				{
					CString ref = ((GitRevLoglist*)m_arShownList[GetNextSelectedItem(pos)])->m_Ref;
					if (ref.Find(_T("refs/")) == 0)
						ref = ref.Mid(5);
					int refpos = ref.ReverseFind('{');
					if (refpos > 0 && ref.Mid(refpos - 1, 2) != _T("@{"))
						ref = ref.Left(refpos) + _T("@")+ ref.Mid(refpos);
					refsToDelete.push_back(ref);
				}

				for (auto revIt = refsToDelete.rbegin(); revIt != refsToDelete.rend(); ++revIt)
				{
					CString ref = *revIt;
					CString cmd, out;
					if (ref.Find(_T("stash")) == 0)
						cmd.Format(_T("git.exe stash drop %s"), ref);
					else
						cmd.Format(_T("git.exe reflog delete %s"), ref);

					if (g_Git.Run(cmd, &out, CP_UTF8))
						CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);

					::PostMessage(this->GetParent()->m_hWnd,MSG_REFLOG_CHANGED,0,0);
				}
			}
			break;
		case ID_LOG:
			{
				CString cmd = _T("/command:log");
				cmd += _T(" /path:\"")+g_Git.m_CurrentDir+_T("\" ");
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				cmd += _T(" /endrev:")+r1->m_CommitHash.ToString();
				CAppUtils::RunTortoiseGitProc(cmd);
			}
			break;
		case ID_CREATE_PATCH:
			{
				int select=this->GetSelectedCount();
				CString cmd = _T("/command:formatpatch");
				cmd += _T(" /path:\"")+g_Git.m_CurrentDir+_T("\" ");

				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = NULL;
				if(select == 1)
				{
					cmd += _T(" /startrev:")+r1->m_CommitHash.ToString();
				}
				else
				{
					r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
					if( this->m_IsOldFirst )
					{
						cmd += _T(" /startrev:")+r1->m_CommitHash.ToString()+_T("~1");
						cmd += _T(" /endrev:")+r2->m_CommitHash.ToString();

					}
					else
					{
						cmd += _T(" /startrev:")+r2->m_CommitHash.ToString()+_T("~1");
						cmd += _T(" /endrev:")+r1->m_CommitHash.ToString();
					}

				}

				CAppUtils::RunTortoiseGitProc(cmd);
			}
			break;
		case ID_BISECTSTART:
			{
				GitRev * first = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * last = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				ASSERT(first != NULL && last != NULL);

				CString firstBad = first->m_CommitHash.ToString();
				if (!m_HashMap[first->m_CommitHash].empty())
					firstBad = m_HashMap[first->m_CommitHash].at(0);
				CString lastGood = last->m_CommitHash.ToString();
				if (!m_HashMap[last->m_CommitHash].empty())
					lastGood = m_HashMap[last->m_CommitHash].at(0);

				if (CAppUtils::BisectStart(lastGood, firstBad))
					Refresh();
			}
			break;
		case ID_REPOBROWSE:
			{
				CString sCmd;
				sCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%s"), g_Git.m_CurrentDir, pSelLogEntry->m_CommitHash.ToString());
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_PUSH:
			{
				CString guessAssociatedBranch = pSelLogEntry->m_CommitHash;
				if (!m_HashMap[pSelLogEntry->m_CommitHash].empty() && m_HashMap[pSelLogEntry->m_CommitHash].at(0).Find(_T("refs/heads/")) == 0)
					guessAssociatedBranch = m_HashMap[pSelLogEntry->m_CommitHash].at(0);
				if (CAppUtils::Push(guessAssociatedBranch))
					Refresh();
			}
			break;
		case ID_PULL:
			{
				if (CAppUtils::Pull())
					Refresh();
			}
			break;
		case ID_FETCH:
			{
				if (CAppUtils::Fetch())
					Refresh();
			}
			break;
		case ID_CLEANUP:
			{
				CString sCmd;
				sCmd.Format(_T("/command:cleanup /path:\"%s\""), g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_SUBMODULE_UPDATE:
			{
				CString sCmd;
				sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);
				CAppUtils::RunTortoiseGitProc(sCmd);
			}
			break;
		case ID_SHOWBRANCHES:
			{
				CString cmd;
				cmd.Format(_T("git.exe branch -a --contains %s"), pSelLogEntry->m_CommitHash.ToString());
				CProgressDlg progress;
				progress.m_AutoClose = AUTOCLOSE_NO;
				progress.m_GitCmd = cmd;
				progress.DoModal();
			}
			break;
		case ID_DELETE:
			{
				CString *branch = (CString*)((CIconMenu*)popmenu)->GetMenuItemData(cmd);
				if (!branch)
				{
					CMessageBox::Show(NULL,IDS_ERROR_NOREF,IDS_APPNAME,MB_OK|MB_ICONERROR);
					return;
				}
				CString shortname;
				if (CGit::GetShortName(*branch, shortname, _T("refs/remotes/")))
				{
					CString msg;
					msg.Format(IDS_PROC_DELETEREMOTEBRANCH, *branch);
					int result = CMessageBox::Show(NULL, msg, _T("TortoiseGit"), 3, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_PROC_DELETEREMOTEBRANCH_LOCALREMOTE)), CString(MAKEINTRESOURCE(IDS_PROC_DELETEREMOTEBRANCH_LOCAL)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
					if (result == 1)
					{
						CString remoteName = shortname.Left(shortname.Find('/'));
						shortname = shortname.Mid(shortname.Find('/') + 1);
						if(CAppUtils::IsSSHPutty())
							CAppUtils::LaunchPAgent(NULL, &remoteName);

						CSysProgressDlg sysProgressDlg;
						sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
						sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
						sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
						sysProgressDlg.SetShowProgressBar(false);
						sysProgressDlg.ShowModal(this, true);
						STRING_VECTOR list;
						list.push_back(_T("refs/heads/") + shortname);
						if (g_Git.DeleteRemoteRefs(remoteName, list))
							CMessageBox::Show(NULL, g_Git.GetGitLastErr(_T("Could not delete remote ref."), CGit::GIT_CMD_PUSH), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
						sysProgressDlg.Stop();
					}
					else if (result == 2)
					{
						if (g_Git.DeleteRef(*branch))
						{
							CMessageBox::Show(nullptr, g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
							return;
						}
					}
					else
						return;
				}
				else if (CGit::GetShortName(*branch, shortname, _T("refs/stash")))
				{
					if (CMessageBox::Show(NULL, IDS_PROC_DELETEALLSTASH, IDS_APPNAME, 2, IDI_QUESTION, IDS_DELETEBUTTON, IDS_ABORTBUTTON) == 1)
					{
						CString cmd;
						cmd.Format(_T("git.exe stash clear"));
						CString out;
						if (g_Git.Run(cmd, &out, CP_UTF8))
							CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
					}
					else
						return;
				}
				else
				{
					CString msg;
					msg.Format(IDS_PROC_DELETEBRANCHTAG, *branch);
					if (CMessageBox::Show(NULL, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_DELETEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 1)
					{
						if (g_Git.DeleteRef(*branch))
						{
							CMessageBox::Show(nullptr, g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
							return;
						}
					}
				}
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
				{
					break;
				}
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
				if (!m_HashMap[pSelLogEntry->m_CommitHash].empty())
					str = m_HashMap[pSelLogEntry->m_CommitHash].at(0);
				// we need an URL to complete this command, so error out if we can't get an URL
				if(CAppUtils::Merge(&str))
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
					if (parent > pSelLogEntry->m_ParentHash.size())
					{
						CString str;
						str.Format(IDS_PROC_NOPARENT, parent);
						MessageBox(str, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
						return;
					}
				}

				if (!this->RevertSelectedCommits(parent))
				{
					if (CMessageBox::Show(m_hWnd, IDS_REVREVERTED, IDS_APPNAME, 1, IDI_QUESTION, IDS_OKBUTTON, IDS_COMMITBUTTON) == 2)
					{
						CTGitPathList pathlist;
						CTGitPathList selectedlist;
						pathlist.AddPath(this->m_Path);
						bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE));
						CString str;
						CAppUtils::Commit(CString(), false, str, pathlist, selectedlist, bSelectFilesForCommit);
					}
					this->Refresh();
				}
			}
			break;
		case ID_EDITNOTE:
			{
				CAppUtils::EditNote(pSelLogEntry);
				this->SetItemState(FirstSelect,  0, LVIS_SELECTED);
				this->SetItemState(FirstSelect,  LVIS_SELECTED, LVIS_SELECTED);
			}
			break;
		default:
			//CMessageBox::Show(NULL,_T("Have not implemented"),_T("TortoiseGit"),MB_OK);
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
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
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
						SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
						EnableOKButton();
						break;
					}
					bSuccess = true;
				}
				if (bSuccess)
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
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
					tempfile = blame.BlameToTempFile(m_path, dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + m_path.GetGitPathString() + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),sParams))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
					}
				}
			}
			break;
		case ID_EXPORT:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:export /path:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseGitProc.exe")),
					(LPCTSTR)pathURL, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
			}
			break;
		case ID_VIEWPATHREV:
			{
				CString relurl = pathURL;
				CString sRoot = GetRepositoryRoot(CTGitPath(relurl));
				relurl = relurl.Mid(sRoot.GetLength());
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				url.Replace(_T("%PATH%"), relurl);
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
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
	int index;
	while(pos)
	{
		index = GetNextSelectedItem(pos);
		if (((GitRevLoglist*)m_arShownList[index])->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE) || (index == GetItemCount() - 1 && action == LOGACTIONS_REBASE_SQUASH))
			continue;
		((GitRevLoglist*)m_arShownList[index])->GetRebaseAction() = action;
		CRect rect;
		this->GetItemRect(index,&rect,LVIR_BOUNDS);
		this->InvalidateRect(rect);

	}
	GetParent()->PostMessage(CGitLogListBase::m_RebaseActionMessage);
}
void CGitLogList::ShiftSelectedRebaseAction()
{
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while(pos)
	{
		index = GetNextSelectedItem(pos);
		int *action = &((GitRevLoglist*)m_arShownList[index])->GetRebaseAction();
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
			if (index == GetItemCount() - 1)
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
