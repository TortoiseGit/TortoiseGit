// GitLogList.cpp : implementation file
//
/*
	Description: qgit revision list view

	Author: Marco Costalba (C) 2005-2007

	Copyright: See COPYING file that comes with this distribution

*/
#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitLogList.h"
#include "GitRev.h"
//#include "VssStyle.h"
#include "IconMenu.h"
// CGitLogList
#include "cursor.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
//#include "RepositoryBrowser.h"
//#include "CopyDlg.h"
//#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
//#include "GitInfo.h"
//#include "GitDiff.h"
#include "IconMenu.h"
//#include "RevisionRangeDlg.h"
//#include "BrowseFolder.h"
//#include "BlameDlg.h"
//#include "Blame.h"
//#include "GitHelpers.h"
#include "GitStatus.h"
//#include "LogDlgHelper.h"
//#include "CachedLogInfo.h"
//#include "RepositoryInfo.h"
//#include "EditPropertiesDlg.h"
#include "FileDiffDlg.h"
#include "CommitDlg.h"
#include "RebaseDlg.h"

IMPLEMENT_DYNAMIC(CGitLogList, CHintListCtrl)

void CGitLogList::ContextMenuAction(int cmd,int FirstSelect, int LastSelect)
{	
	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	GitRev* pSelLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(indexNext));

	theApp.DoWaitCursor(1);
	bool bOpenWith = false;
	switch (cmd)
		{
			case ID_GNUDIFF1:
			{
				CString tempfile=GetTempFile();
				CString cmd;
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				cmd.Format(_T("git.exe diff-tree -r -p --stat %s"),r1->m_CommitHash);
				g_Git.RunLogFile(cmd,tempfile);
				CAppUtils::StartUnifiedDiffViewer(tempfile,r1->m_CommitHash.Left(6)+_T(":")+r1->m_Subject);
			}
			break;

			case ID_GNUDIFF2:
			{
				CString tempfile=GetTempFile();
				CString cmd;
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"),r1->m_CommitHash,r2->m_CommitHash);
				g_Git.RunLogFile(cmd,tempfile);
				CAppUtils::StartUnifiedDiffViewer(tempfile,r1->m_CommitHash.Left(6)+_T(":")+r2->m_CommitHash.Left(6));

			}
			break;

		case ID_COMPARETWO:
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				CFileDiffDlg dlg;
				dlg.SetDiff(NULL,*r1,*r2);
				dlg.DoModal();
				
			}
			break;
		

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

		case ID_COMPAREWITHPREVIOUS:
			{

				CFileDiffDlg dlg;
				
				if(pSelLogEntry->m_ParentHash.size()>0)
				//if(m_logEntries.m_HashMap[pSelLogEntry->m_ParentHash[0]]>=0)
				{
					dlg.SetDiff(NULL,pSelLogEntry->m_CommitHash,pSelLogEntry->m_ParentHash[0]);
					dlg.DoModal();
				}else
				{
					CMessageBox::Show(NULL,_T("No previous version"),_T("TortoiseGit"),MB_OK);	
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
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			break;
		case ID_COPYHASH:
			{
				CopySelectionToClipBoard(TRUE);
			}
			break;
		case ID_EXPORT:
			CAppUtils::Export(&pSelLogEntry->m_CommitHash);
			break;
		case ID_CREATE_BRANCH:
			CAppUtils::CreateBranchTag(FALSE,&pSelLogEntry->m_CommitHash);
			ReloadHashMap();
			Invalidate();			
			break;
		case ID_CREATE_TAG:
			CAppUtils::CreateBranchTag(TRUE,&pSelLogEntry->m_CommitHash);
			ReloadHashMap();
			Invalidate();
			break;
		case ID_SWITCHTOREV:
			CAppUtils::Switch(&pSelLogEntry->m_CommitHash);
			ReloadHashMap();
			Invalidate();
			break;
		case ID_RESET:
			CAppUtils::GitReset(&pSelLogEntry->m_CommitHash);
			ReloadHashMap();
			Invalidate();
			break;
		case ID_REBASE_PICK:
			SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_PICK);
			break;
		case ID_REBASE_EDIT:
			SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_EDIT);
			break;
		case ID_REBASE_SQUASH:
			SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_SQUASH);
			break;
		case ID_REBASE_SKIP:
			SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_SKIP);
			break;
		case ID_COMBINE_COMMIT:
		{
			CString head;
			CString headhash;
			
			head.Format(_T("HEAD~%d"),FirstSelect);
			CString hashFirst=g_Git.GetHash(head);

			head.Format(_T("HEAD~%d"),LastSelect);
			CString hashLast=g_Git.GetHash(head);
			
			headhash=g_Git.GetHash(CString(_T("HEAD")));
			
			GitRev* pFirstEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
			GitRev* pLastEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
			if(pFirstEntry->m_CommitHash != hashFirst || pLastEntry->m_CommitHash != hashLast)
			{
				CMessageBox::Show(NULL,_T(
					"Cannot combine commits now.\r\n\
					Make sure you are viewing the log of your current branch and \
					no filters are applied."),_T("TortoiseGit"),MB_OK);
				break;
			}
			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL,_T("Combine needs a clean work tree"),_T("TortoiseGit"),MB_OK);
				break;
			}
			CString cmd,out;

			//Use throw to abort this process (reset back to original HEAD)
			try
			{
				cmd.Format(_T("git.exe reset --hard  %s"),pFirstEntry->m_CommitHash);
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not reset to first commit (first step) aborting...\r\n\r\n")+out));
				}
				cmd.Format(_T("git.exe reset --mixed  %s"),hashLast);
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
					throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not reset to last commit (second step) aborting...\r\n\r\n")+out));
				}
				CCommitDlg dlg;
				for(int i=FirstSelect;i<=LastSelect;i++)
				{
					GitRev* pRev = reinterpret_cast<GitRev*>(m_arShownList.GetAt(i));
					dlg.m_sLogMessage+=pRev->m_Subject+_T("\n")+pRev->m_Body;
					dlg.m_sLogMessage+=_T("\n");
				}
				dlg.m_bWholeProject=true;
				dlg.m_bSelectFilesForCommit = true;
				dlg.m_bCommitAmend=true;
				dlg.m_AmendStr=dlg.m_sLogMessage;

				bool abort=false;
				if (dlg.DoModal() == IDOK)
				{
					if(pFirstEntry->m_CommitHash!=headhash)
					{
						//Commitrange firstEntry..headhash (from top of combine to original head) needs to be 'cherry-picked'
						//on top of new commit.
						//Use the rebase --onto command for it.
						//
						//All this can be done in one step using the following command:
						//cmd.Format(_T("git.exe format-patch --stdout --binary --full-index -k %s..%s | git am -k -3"),
						//	pFirstEntry->m_CommitHash,
						//	headhash);
						//But I am not sure if a '|' is going to work in a CreateProcess() call.
						//
						//Later the progress dialog could be used to execute these steps.

						CString currentBranch=g_Git.GetCurrentBranch();
						cmd.Format(_T("git.exe rebase --onto \"%s\" %s %s"),
							currentBranch,
							pFirstEntry->m_CommitHash,
							headhash);
						if(g_Git.Run(cmd,&out,CP_UTF8)!=0)
						{
							CString msg;
							msg.Format(_T("Error while rebasing commits on top of combined commits. Aborting.\r\n\r\n%s"),out);
//							CMessageBox::Show(NULL,msg,_T("TortoiseGit"),MB_OK);
							g_Git.Run(_T("git.exe rebase --abort"),&out,CP_UTF8);
							throw std::exception(CUnicodeUtils::GetUTF8(msg));
						}

						//HEAD is now on <no branch>. 
						//The following steps are to get HEAD back on the original branch and reset the branch to the new HEAD
						//To avoid 2 working copy changes, we could use git branch -f <original branch> <hash new head> 
						//And then git checkout <original branch>
						//But I don't know if 'git branch -f' removes tracking options. So for now, do a checkout and a reset.
						
						//Store new HEAD
						CString newHead=g_Git.GetHash(CString(_T("HEAD")));

						//Checkout working branch
						cmd.Format(_T("git.exe checkout -f \"%s\""),currentBranch);
						if(g_Git.Run(cmd,&out,CP_UTF8))
							throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not checkout original branch. Aborting...\r\n\r\n")+out));

						//Reset to new HEAD
						cmd.Format(_T("git.exe reset --hard  %s"),newHead);
						if(g_Git.Run(cmd,&out,CP_UTF8))
							throw std::exception(CUnicodeUtils::GetUTF8(_T("Could not reset to new head. Aborting...\r\n\r\n")+out));
					}
				}
				else
					throw std::exception("User aborted the combine process");
			}
			catch(std::exception& e)
			{
				CMessageBox::Show(NULL,CUnicodeUtils::GetUnicode(CStringA(e.what())),_T("TortoiseGit: Combine error"),MB_OK|MB_ICONERROR);
				cmd.Format(_T("git.exe reset --hard  %s"),headhash);
				out.Empty();
				if(g_Git.Run(cmd,&out,CP_UTF8))
				{
					CMessageBox::Show(NULL,_T("Could not reset to original HEAD\r\n\r\n")+out,_T("TortoiseGit"),MB_OK);
				}
			}
			Refresh();
		}
			break;

		case ID_CHERRY_PICK:
			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL,_T("Cherry Pick Require Clean Working Tree"),_T("TortoiseGit"),MB_OK);
			
			}else
			{
				CRebaseDlg dlg;
				dlg.m_IsCherryPick = TRUE;
				dlg.m_Upstream = this->m_CurrentBranch;
				POSITION pos = GetFirstSelectedItemPosition();
				while(pos)
				{
					int indexNext = GetNextSelectedItem(pos);
					dlg.m_CommitList.m_logEntries.push_back(*(GitRev*)m_arShownList[indexNext]);
					dlg.m_CommitList.m_logEntries.at(dlg.m_CommitList.m_logEntries.size()-1).m_Action |= CTGitPath::LOGACTIONS_REBASE_PICK;
				}

				if(dlg.DoModal() == IDOK)
				{
					Refresh();
				}
			}
			break;
		case ID_REBASE_TO_VERSION:
			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL,_T("Rebase Require Clean Working Tree"),_T("TortoiseGit"),MB_OK);
			
			}else
			{
				CRebaseDlg dlg;
				dlg.m_Upstream = pSelLogEntry->m_CommitHash;

				if(dlg.DoModal() == IDOK)
				{
					Refresh();
				}
			}

			break;

		default:
			//CMessageBox::Show(NULL,_T("Have not implemented"),_T("TortoiseGit"),MB_OK);
			break;
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
		case ID_REPOBROWSE:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:repobrowser /path:\"%s\" /rev:%s"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LPCTSTR)revSelected.ToString());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
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

		theApp.DoWaitCursor(-1);
}

void CGitLogList::SetSelectedAction(int action)
{
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while(pos)
	{
		index = GetNextSelectedItem(pos);
		((GitRev*)m_arShownList[index])->m_Action = action;
		CRect rect;
		this->GetItemRect(index,&rect,LVIR_BOUNDS);
		this->InvalidateRect(rect);

	}

}