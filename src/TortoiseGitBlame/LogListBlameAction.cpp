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

void CGitBlameLogList::ContextMenuAction(int cmd,int FirstSelect, int LastSelect,CMenu * menu)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	CString  procCmd;

	GitRev* pSelLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(indexNext));

	bool bOpenWith = false;

	procCmd += _T("/path:\"");
	procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	procCmd += _T("\" ");
	procCmd += _T(" /rev:")+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString();

	procCmd += _T(" /command:");

	switch (cmd)
	{
		case ID_GNUDIFF1:
			procCmd += _T("diff /udiff");
		break;

#if 0
		case ID_GNUDIFF2:
			{
				CString tempfile=GetTempFile();
				CString cmd;
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"),r1->m_CommitHash,r2->m_CommitHash);
				g_Git.RunLogFile(cmd,tempfile);
				CAppUtils::StartUnifiedDiffViewer(tempfile, r1->m_CommitHash.Left(g_Git.GetShortHASHLength()) + _T(":") + r2->m_CommitHash.Left(g_Git.GetShortHASHLength()));

			}
			break;
#endif
#if 0
		case ID_COMPARETWO:
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				CFileDiffDlg dlg;
				dlg.SetDiff(NULL,*r1,*r2);
				dlg.DoModal();

			}
			break;
#endif
#if 0
		case ID_COMPARE:
			{
				GitRev * r1 = &m_wcRev;
				GitRev * r2 = pSelLogEntry;
				CFileDiffDlg dlg;
				dlg.SetDiff(NULL,*r1,*r2);
				dlg.DoModal();

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
		case ID_COMPARE:
			procCmd+=CString(_T("diff \rev1:"))+CString(GIT_REV_ZERO)+CString(_T(" \rev2:"))+this->m_logEntries.GetGitRevAt(indexNext).m_CommitHash.ToString();
			break;
#endif
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

#if 0

		case ID_REVERTREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(true);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_MERGEREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString path = m_path.GetWinPathString();
				bool bGotSavePath = false;
				if ((GetSelectedCount() == 1)&&(!m_path.IsDirectory()))
				{
					bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER, true, GetSafeHwnd());
				}
				else
				{
					CBrowseFolder folderBrowser;
					folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
					bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
				}
				if (bGotSavePath)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(CTGitPath(path)));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(false);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_REVERTTOREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString msg;
				msg.Format(IDS_LOG_REVERTTOREV_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					GitRevRangeArray revarray;
					revarray.AddRevRange(GitRev::REV_HEAD, revSelected);
					dlg.SetRevisionRanges(revarray);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;

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
		case ID_BLAMETWO:
			{
				//user clicked on the menu item "compare and blame revisions"
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTGitPath(pathURL), revSelected2, CTGitPath(pathURL), revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revSelected2, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, false, false, true);
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
					int ret = 0;
					if (!bOpenWith)
						ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
					if ((ret <= HINSTANCE_ERROR)||bOpenWith)
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += tempfile.GetWinPathString() + _T(" ");
						CAppUtils::LaunchApplication(cmd, NULL, false);
					}
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
		case ID_UPDATE:
			{
				CString sCmd;
				CString url = _T("tgit:")+pathURL;
				sCmd.Format(_T("%s /command:update /path:\"%s\" /rev:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)m_path.GetWinPath(), (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
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
					m_pFindDialog = new CFindReplaceDialog();
					m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);
				}
			}
			break;
		case ID_EDITLOG:
			{
				EditLogMessage(selIndex);
			}
			break;
		case ID_EDITAUTHOR:
			{
				EditAuthor(selEntries);
			}
			break;
		case ID_REVPROPS:
			{
				CEditPropertiesDlg dlg;
				dlg.SetProjectProperties(&m_ProjectProperties);
				CTGitPathList escapedlist;
				dlg.SetPathList(CTGitPathList(CTGitPath(pathURL)));
				dlg.SetRevision(revSelected);
				dlg.RevProps(true);
				dlg.DoModal();
			}
			break;

		case ID_EXPORT:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:export /path:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_CHECKOUT:
			{
				CString sCmd;
				CString url = _T("tgit:")+pathURL;
				sCmd.Format(_T("%s /command:checkout /url:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)url, (LONG)revSelected);
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

		CCommonAppUtils::RunTortoiseProc(procCmd);
}