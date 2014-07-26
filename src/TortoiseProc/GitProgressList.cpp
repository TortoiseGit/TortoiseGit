// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "stdafx.h"
#include "GitProgressList.h"
#include "TortoiseProc.h"
#include "MessageBox.h"
#include "GitProgressDlg.h"
#include "LogDlg.h"
#include "registry.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "SoundUtils.h"
#include "LogFile.h"
#include "ShellUpdater.h"
#include "IconMenu.h"
#include "Patch.h"
#include "MassiveGitTask.h"
#include "LoglistUtils.h"

BOOL	CGitProgressList::m_bAscending = FALSE;
int		CGitProgressList::m_nSortedColumn = -1;

#define TRANSFERTIMER	100
#define VISIBLETIMER	101
// CGitProgressList

enum GITProgressDlgContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1,
	ID_EDITCONFLICT,
	ID_CONFLICTRESOLVE,
	ID_CONFLICTUSETHEIRS,
	ID_CONFLICTUSEMINE,
	ID_LOG,
	ID_OPEN,
	ID_OPENWITH,
	ID_EXPLORE,
	ID_COPY
};

IMPLEMENT_DYNAMIC(CGitProgressList, CListCtrl)

class CSmartAnimation
{
	CAnimateCtrl* m_pAnimate;

public:
	CSmartAnimation(CAnimateCtrl* pAnimate)
	{
		m_pAnimate = pAnimate;
		if (m_pAnimate)
		{
			m_pAnimate->ShowWindow(SW_SHOW);
			m_pAnimate->Play(0, INT_MAX, INT_MAX);
		}
	}
	~CSmartAnimation()
	{
		if (m_pAnimate)
		{
			m_pAnimate->Stop();
			m_pAnimate->ShowWindow(SW_HIDE);
		}
	}
};

CGitProgressList::CGitProgressList():CListCtrl()
	, m_bCancelled(FALSE)
	, m_pThread(NULL)
	, m_bErrorsOccurred(false)
	, m_bBare(false)
	, m_bNoCheckout(false)
	, m_AutoTag(GIT_REMOTE_DOWNLOAD_TAGS_AUTO)
	, m_resetType(0)
	, m_options(ProgOptNone)
	, m_bSetTitle(false)
	, m_pTaskbarList(nullptr)
	, m_SendMail(nullptr)
	, m_Command(GitProgress_none)
	, m_bThreadRunning(FALSE)
	, m_keepchangelist(false)
	, m_nConflicts(0)
	, m_bMergesAddsDeletesOccurred(FALSE)
	, iFirstResized(0)
	, bSecondResized(false)
	, nEnsureVisibleCount(0)
	, m_TotalBytesTransferred(0)
	, m_bFinishedItemAdded(false)
	, m_bLastVisible(false)
	, m_itemCount(-1)
	, m_itemCountTotal(-1)
{
	m_pInfoCtrl = nullptr;
	m_pAnimate = nullptr;
	m_pProgControl = nullptr;
	m_pProgressLabelCtrl = nullptr;
	m_pPostWnd = nullptr;
	m_columnbuf[0] = 0;
}

CGitProgressList::~CGitProgressList()
{
	for (size_t i = 0; i < m_arData.size(); ++i)
	{
		delete m_arData[i];
	}
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}


BEGIN_MESSAGE_MAP(CGitProgressList, CListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdrawSvnprogress)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkSvnprogress)
	ON_NOTIFY_REFLECT(HDN_ITEMCLICK, OnHdnItemclickSvnprogress)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnLvnBegindragSvnprogress)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfoSvnprogress)
	ON_MESSAGE(WM_SHOWCONFLICTRESOLVER, OnShowConflictResolver)
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CGitProgressList::Cancel()
{
	m_bCancelled = TRUE;
}



// CGitProgressList message handlers


LRESULT CGitProgressList::OnShowConflictResolver(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
#if 0
	CConflictResolveDlg dlg(this);
	const svn_wc_conflict_description_t *description = (svn_wc_conflict_description_t *)lParam;
	if (description)
	{
		dlg.SetConflictDescription(description);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		}
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.GetResult() == svn_wc_conflict_choose_postpone)
			{
				// if the result is conflicted and the dialog returned IDOK,
				// that means we should not ask again in case of a conflict
				m_AlwaysConflicted = true;
				::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		m_mergedfile = dlg.GetMergedFile();
		m_bCancelled = dlg.IsCancelled();
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		return dlg.GetResult();
	}

	return svn_wc_conflict_choose_postpone;
#endif
	return 0;
}
#if 0
svn_wc_conflict_choice_t CGitProgressList::ConflictResolveCallback(const svn_wc_conflict_description_t *description, CString& mergedfile)
{
	// we only bother the user when merging
	if (((m_Command == GitProgress_Merge)||(m_Command == GitProgress_MergeAll)||(m_Command == GitProgress_MergeReintegrate))&&(!m_AlwaysConflicted)&&(description))
	{
		// we're in a worker thread here. That means we must not show a dialog from the thread
		// but let the UI thread do it.
		// To do that, we send a message to the UI thread and let it show the conflict resolver dialog.
		LRESULT dlgResult = ::SendMessage(GetSafeHwnd(), WM_SHOWCONFLICTRESOLVER, 0, (LPARAM)description);
		mergedfile = m_mergedfile;
		return (svn_wc_conflict_choice_t)dlgResult;
	}

	return svn_wc_conflict_choose_postpone;
}
#endif
void CGitProgressList::AddItemToList()
{
	int totalcount = GetItemCount();

	SetItemCountEx(totalcount+1, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);
	// make columns width fit
	if (iFirstResized < 30)
	{
		// only resize the columns for the first 30 or so entries.
		// after that, don't resize them anymore because that's an
		// expensive function call and the columns will be sized
		// close enough already.
		ResizeColumns();
		++iFirstResized;
	}

	// Make sure the item is *entirely* visible even if the horizontal
	// scroll bar is visible.
	int count = GetCountPerPage();
	if (totalcount <= (GetTopIndex() + count + nEnsureVisibleCount + 2))
	{
		++nEnsureVisibleCount;
		m_bLastVisible = true;
	}
	else
	{
		nEnsureVisibleCount = 0;
		if (IsIconic() == 0)
			m_bLastVisible = false;
	}
}


BOOL CGitProgressList::Notify(const CTGitPath& path, git_wc_notify_action_t action)
{
	bool bNoNotify = false;
	bool bDoAddData = true;
	NotificationData * data = new NotificationData();
	data->path = path;
	data->action = action;
	data->sPathColumnText=path.GetGitPathString();
	data->bAuxItem = false;

	if (this->m_pAnimate)
		this->m_pAnimate->ShowWindow(SW_HIDE);

	switch (data->action)
	{
	case git_wc_notify_add:
		data->sActionColumnText.LoadString(IDS_SVNACTION_ADD);
		data->color = m_Colors.GetColor(CColors::Added);
		break;
	case git_wc_notify_sendmail:
		data->sActionColumnText.LoadString(IDS_SVNACTION_SENDMAIL_START);
		data->color = m_Colors.GetColor(CColors::Modified);
		break;

	case git_wc_notify_resolved:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
		break;

	case git_wc_notify_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REVERT);
		break;

	case git_wc_notify_checkout:
		data->sActionColumnText.LoadString(IDS_PROGRS_CMD_CHECKOUT);
		data->color = m_Colors.GetColor(CColors::Added);
		data->bAuxItem = false;
		break;
	default:
		break;
	} // switch (data->action)

	if (bNoNotify)
		delete data;
	else
	{
		if (bDoAddData)
		{
			m_arData.push_back(data);
			AddItemToList();
			if ((!data->bAuxItem) && (m_itemCount > 0))
			{
				if (m_pProgControl)
				{
					m_pProgControl->ShowWindow(SW_SHOW);
					m_pProgControl->SetPos(m_itemCount);
					m_pProgControl->SetRange32(0, m_itemCountTotal);
				}
				if (m_pTaskbarList && m_pPostWnd)
				{
					m_pTaskbarList->SetProgressState(m_pPostWnd->GetSafeHwnd(), TBPF_NORMAL);
					m_pTaskbarList->SetProgressValue(m_pPostWnd->GetSafeHwnd(), m_itemCount, m_itemCountTotal);
				}
			}
		}
		//if ((action == svn_wc_notify_commit_postfix_txdelta)&&(bSecondResized == FALSE))
		//{
		//	ResizeColumns();
		//	bSecondResized = TRUE;
		//}
	}

	return TRUE;
}


CString CGitProgressList::BuildInfoString()
{
	CString infotext;
	if(this->m_Command == GitProgress_Resolve)
		infotext = _T("You need commit your change after resolve conflict");
#if 0

	CString temp;
	int added = 0;
	int copied = 0;
	int deleted = 0;
	int restored = 0;
	int reverted = 0;
	int resolved = 0;
	int conflicted = 0;
	int updated = 0;
	int merged = 0;
	int modified = 0;
	int skipped = 0;
	int replaced = 0;

	for (size_t i=0; i<m_arData.size(); ++i)
	{
		const NotificationData * dat = m_arData[i];
		switch (dat->action)
		{
		case svn_wc_notify_add:
		case svn_wc_notify_update_add:
		case svn_wc_notify_commit_added:
			if (dat->bConflictedActionItem)
				++conflicted;
			else
				++added;
			break;
		case svn_wc_notify_copy:
			++copied;
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			++deleted;
			break;
		case svn_wc_notify_restore:
			++restored;
			break;
		case svn_wc_notify_revert:
			++reverted;
			break;
		case svn_wc_notify_resolved:
			++resolved;
			break;
		case svn_wc_notify_update_update:
			if (dat->bConflictedActionItem)
				++conflicted;
			else if ((dat->content_state == svn_wc_notify_state_merged) || (dat->prop_state == svn_wc_notify_state_merged))
				++merged;
			else
				++updated;
			break;
		case svn_wc_notify_commit_modified:
			++modified;
			break;
		case svn_wc_notify_skip:
			++skipped;
			break;
		case svn_wc_notify_commit_replaced:
			++replaced;
			break;
		}
	}
	if (conflicted)
	{
		temp.LoadString(IDS_SVNACTION_CONFLICTED);
		infotext += temp;
		temp.Format(_T(":%d "), conflicted);
		infotext += temp;
	}
	if (skipped)
	{
		temp.LoadString(IDS_SVNACTION_SKIP);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), skipped);
	}
	if (merged)
	{
		temp.LoadString(IDS_SVNACTION_MERGED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), merged);
	}
	if (added)
	{
		temp.LoadString(IDS_SVNACTION_ADD);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), added);
	}
	if (deleted)
	{
		temp.LoadString(IDS_SVNACTION_DELETE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), deleted);
	}
	if (modified)
	{
		temp.LoadString(IDS_SVNACTION_MODIFIED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), modified);
	}
	if (copied)
	{
		temp.LoadString(IDS_SVNACTION_COPY);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), copied);
	}
	if (replaced)
	{
		temp.LoadString(IDS_SVNACTION_REPLACED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), replaced);
	}
	if (updated)
	{
		temp.LoadString(IDS_SVNACTION_UPDATE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), updated);
	}
	if (restored)
	{
		temp.LoadString(IDS_SVNACTION_RESTORE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), restored);
	}
	if (reverted)
	{
		temp.LoadString(IDS_SVNACTION_REVERT);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), reverted);
	}
	if (resolved)
	{
		temp.LoadString(IDS_SVNACTION_RESOLVE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), resolved);
	}
#endif
	return infotext;
}

void CGitProgressList::SetSelectedList(const CTGitPathList& selPaths)
{
	m_selectedPaths = selPaths;
}

void CGitProgressList::ResizeColumns()
{
	SetRedraw(FALSE);

	TCHAR textbuf[MAX_PATH] = {0};

	CHeaderCtrl * pHeaderCtrl = (CHeaderCtrl*)(GetDlgItem(0));
	if (pHeaderCtrl)
	{
		int maxcol = pHeaderCtrl->GetItemCount()-1;
		for (int col = 0; col <= maxcol; ++col)
		{
			// find the longest width of all items
			int count = GetItemCount();
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = _countof(textbuf);
			pHeaderCtrl->GetItem(col, &hdi);
			int cx = GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin

			for (int index = 0; index<count; ++index)
			{
				// get the width of the string and add 12 pixels for the column separator and margins
				int linewidth = cx;
				switch (col)
				{
				case 0:
					linewidth = GetStringWidth(m_arData[index]->sActionColumnText) + 12;
					break;
				case 1:
					linewidth = GetStringWidth(m_arData[index]->sPathColumnText) + 12;
					break;
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			SetColumnWidth(col, cx);
		}
	}

	SetRedraw(TRUE);
}

bool CGitProgressList::SetBackgroundImage(UINT nID)
{
	return CAppUtils::SetListCtrlBackgroundImage(GetSafeHwnd(), nID);
}

void CGitProgressList::ReportGitError()
{
	ReportError(CGit::GetLibGit2LastErr());
}

void CGitProgressList::ReportUserCanceled()
{
	ReportError(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED)));
}

void CGitProgressList::ReportError(const CString& sError)
{
	CSoundUtils::PlayTGitError();
	ReportString(sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), m_Colors.GetColor(CColors::Conflict));
	m_bErrorsOccurred = true;
}

void CGitProgressList::ReportWarning(const CString& sWarning)
{
	CSoundUtils::PlayTGitWarning();
	ReportString(sWarning, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)), m_Colors.GetColor(CColors::Conflict));
}

void CGitProgressList::ReportNotification(const CString& sNotification)
{
	CSoundUtils::PlayTGitNotification();
	ReportString(sNotification, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
}

void CGitProgressList::ReportCmd(const CString& sCmd)
{
	ReportString(sCmd, CString(MAKEINTRESOURCE(IDS_PROGRS_CMDINFO)), m_Colors.GetColor(CColors::Cmd));
}

void CGitProgressList::ReportString(CString sMessage, const CString& sMsgKind, COLORREF color)
{
	// instead of showing a dialog box with the error message or notification,
	// just insert the error text into the list control.
	// that way the user isn't 'interrupted' by a dialog box popping up!

	// the message may be split up into different lines
	// so add a new entry for each line of the message
	while (!sMessage.IsEmpty())
	{
		NotificationData * data = new NotificationData();
		data->bAuxItem = true;
		data->sActionColumnText = sMsgKind;
		if (sMessage.Find('\n')>=0)
			data->sPathColumnText = sMessage.Left(sMessage.Find('\n'));
		else
			data->sPathColumnText = sMessage;
		data->sPathColumnText.Trim(_T("\n\r"));
		data->color = color;
		if (sMessage.Find('\n')>=0)
		{
			sMessage = sMessage.Mid(sMessage.Find('\n'));
			sMessage.Trim(_T("\n\r"));
		}
		else
			sMessage.Empty();
		m_arData.push_back(data);
		AddItemToList();
	}
}

UINT CGitProgressList::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CGitProgressList*)pVoid)->ProgressThread();
}

UINT CGitProgressList::ProgressThread()
{
	// The SetParams function should have loaded something for us

	CString temp;
	CString sWindowTitle;
	bool localoperation = false;
	bool bSuccess = false;

	if(m_pPostWnd)
		m_pPostWnd->PostMessage(WM_PROG_CMD_START, m_Command);

	if(m_pProgressLabelCtrl)
	{
		m_pProgressLabelCtrl->ShowWindow(SW_SHOW);
		m_pProgressLabelCtrl->SetWindowText(_T(""));
	}

//	SetAndClearProgressInfo(m_hWnd);
	m_itemCount = m_itemCountTotal;

	InterlockedExchange(&m_bThreadRunning, TRUE);
	iFirstResized = 0;
	bSecondResized = FALSE;
	m_bFinishedItemAdded = false;
	DWORD startTime = GetCurrentTime();

	if (m_pTaskbarList && m_pPostWnd)
		m_pTaskbarList->SetProgressState(m_pPostWnd->GetSafeHwnd(), TBPF_INDETERMINATE);

	switch (m_Command)
	{
	case GitProgress_Add:
		bSuccess = CmdAdd(sWindowTitle, localoperation);
		break;
	case GitProgress_Resolve:
		bSuccess = CmdResolve(sWindowTitle, localoperation);
		break;
	case GitProgress_Revert:
		bSuccess = CmdRevert(sWindowTitle, localoperation);
		break;
	case GitProgress_SendMail:
		bSuccess = CmdSendMail(sWindowTitle, localoperation);
		break;
	case GitProgress_Clone:
		bSuccess = CmdClone(sWindowTitle, localoperation);
		break;
	case GitProgress_Fetch:
		bSuccess = CmdFetch(sWindowTitle, localoperation);
		break;
	case GitProgress_Reset:
		bSuccess = CmdReset(sWindowTitle, localoperation);
		break;
	}
	if (!bSuccess)
		temp.LoadString(IDS_PROGRS_TITLEFAILED);
	else
		temp.LoadString(IDS_PROGRS_TITLEFIN);
	sWindowTitle = sWindowTitle + _T(" ") + temp;
	if (m_bSetTitle && m_pPostWnd)
		::SetWindowText(m_pPostWnd->GetSafeHwnd(), sWindowTitle);

	KillTimer(TRANSFERTIMER);
	KillTimer(VISIBLETIMER);

	if (m_pTaskbarList && m_pPostWnd)
	{
		if (DidErrorsOccur())
		{
			m_pTaskbarList->SetProgressState(m_pPostWnd->GetSafeHwnd(), TBPF_ERROR);
			m_pTaskbarList->SetProgressValue(m_pPostWnd->GetSafeHwnd(), 100, 100);
		}
		else
			m_pTaskbarList->SetProgressState(m_pPostWnd->GetSafeHwnd(), TBPF_NOPROGRESS);
	}

	CString info = BuildInfoString();
	if (!bSuccess)
		info.LoadString(IDS_PROGRS_INFOFAILED);
	if (m_pInfoCtrl)
		m_pInfoCtrl->SetWindowText(info);
	
	ResizeColumns();

	DWORD time = GetCurrentTime() - startTime;

	CString sFinalInfo;
	if (!m_sTotalBytesTransferred.IsEmpty())
	{
		temp.Format(IDS_PROGRS_TIME, (time / 1000) / 60, (time / 1000) % 60);
		sFinalInfo.Format(IDS_PROGRS_FINALINFO, m_sTotalBytesTransferred, (LPCTSTR)temp);
		if (m_pProgressLabelCtrl)
			m_pProgressLabelCtrl->SetWindowText(sFinalInfo);
	}
	else
	{
		if (m_pProgressLabelCtrl)
			m_pProgressLabelCtrl->ShowWindow(SW_HIDE);
	}

	if (m_pProgControl)
		m_pProgControl->ShowWindow(SW_HIDE);

	if (!m_bFinishedItemAdded)
	{
		CString log, str;
		if (bSuccess)
			str.LoadString(IDS_SUCCESS);
		else
			str.LoadString(IDS_FAIL);
		log.Format(_T("%s (%lu ms @ %s)"), str, time,  CLoglistUtils::FormatDateAndTime(CTime::GetCurrentTime(), DATE_SHORTDATE, true, false));

		// there's no "finished: xxx" line at the end. We add one here to make
		// sure the user sees that the command is actually finished.
		ReportString(log, CString(MAKEINTRESOURCE(IDS_PROGRS_FINISHED)), bSuccess? RGB(0,0,255) : RGB(255,0,0));
	}

	int count = GetItemCount();
	if ((count > 0)&&(m_bLastVisible))
		EnsureVisible(count-1, FALSE);

	CLogFile logfile(g_Git.m_CurrentDir);
	if (logfile.Open())
	{
		logfile.AddTimeLine();
		for (size_t i = 0; i < m_arData.size(); ++i)
		{
			NotificationData * data = m_arData[i];
			temp.Format(_T("%-20s : %s"), (LPCTSTR)data->sActionColumnText, (LPCTSTR)data->sPathColumnText);
			logfile.AddLine(temp);
		}
		if (!sFinalInfo.IsEmpty())
			logfile.AddLine(sFinalInfo);
		logfile.Close();
	}

	m_bCancelled = TRUE;
	InterlockedExchange(&m_bThreadRunning, FALSE);
#if 0 //need
	RefreshCursor();
#endif

	if (m_pPostWnd)
		m_pPostWnd->PostMessage(WM_PROG_CMD_FINISH, this->m_Command, 0L);

	//Don't do anything here which might cause messages to be sent to the window
	//The window thread is probably now blocked in OnOK if we've done an auto close
	return 0;
}

void CGitProgressList::OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo)
	{
		if (pDispInfo->item.mask & LVIF_TEXT)
		{
			if (pDispInfo->item.iItem < (int)m_arData.size())
			{
				const NotificationData * data = m_arData[pDispInfo->item.iItem];
				switch (pDispInfo->item.iSubItem)
				{
				case 0:
					lstrcpyn(m_columnbuf, data->sActionColumnText, MAX_PATH);
					break;
				case 1:
					lstrcpyn(m_columnbuf, data->sPathColumnText, pDispInfo->item.cchTextMax);
					if (!data->bAuxItem)
					{
						int cWidth = GetColumnWidth(1);
						cWidth = max(12, cWidth-12);
						CDC * pDC = GetDC();
						if (pDC != NULL)
						{
							CFont * pFont = pDC->SelectObject(GetFont());
							PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
							pDC->SelectObject(pFont);
							ReleaseDC(pDC);
						}
					}
					break;
				default:
					m_columnbuf[0] = 0;
				}
				pDispInfo->item.pszText = m_columnbuf;
			}
		}
	}
	*pResult = 0;
}

void CGitProgressList::OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		ASSERT(pLVCD->nmcd.dwItemSpec <  m_arData.size());
		if(pLVCD->nmcd.dwItemSpec >= m_arData.size())
		{
			return;
		}
		const NotificationData * data = m_arData[pLVCD->nmcd.dwItemSpec];
		ASSERT(data != NULL);
		if (data == NULL)
			return;

		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = data->color;
	}
}

void CGitProgressList::OnNMDblclkSvnprogress(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
{
#if 0
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem < 0)
		return;
	if (m_options & ProgOptDryRun)
		return;	//don't do anything in a dry-run.

	const NotificationData * data = m_arData[pNMLV->iItem];
	if (data == NULL)
		return;

	if (data->bConflictedActionItem)
	{
		// We've double-clicked on a conflicted item - do a three-way merge on it
		SVNDiff::StartConflictEditor(data->path);
	}
	else if ((data->action == svn_wc_notify_update_update) && ((data->content_state == svn_wc_notify_state_merged)||(GitProgress_Merge == m_Command)) || (data->action == svn_wc_notify_resolved))
	{
		// This is a modified file which has been merged on update. Diff it against base
		CTGitPath temporaryFile;
		SVNDiff diff(this, this->m_hWnd, true);
		diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		svn_revnum_t baseRev = 0;
		diff.DiffFileAgainstBase(data->path, baseRev);
	}
	else if ((!data->bAuxItem)&&(data->path.Exists())&&(!data->path.IsDirectory()))
	{
		bool bOpenWith = false;
		int ret = (int)ShellExecute(m_hWnd, NULL, data->path.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
		if (ret <= HINSTANCE_ERROR)
			bOpenWith = true;
		if (bOpenWith)
		{
			CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
			cmd += data->path.GetWinPathString() + _T(" ");
			CAppUtils::LaunchApplication(cmd, NULL, false);
		}
	}
#endif
}

void CGitProgressList::OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (m_bThreadRunning)
		return;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Sort();

	CString temp;
	SetRedraw(FALSE);
	DeleteAllItems();
	SetItemCountEx (static_cast<int>(m_arData.size()));

	SetRedraw(TRUE);

	*pResult = 0;
}

bool CGitProgressList::NotificationDataIsAux(const NotificationData* pData)
{
	return pData->bAuxItem;
}
BOOL CGitProgressList::Notify(const git_wc_notify_action_t action, CString str, const git_oid *a, const git_oid *b)
{
	NotificationData * data = new NotificationData();
	data->action = action;
	data->bAuxItem = false;

	if (action == git_wc_notify_update_ref)
	{
		data->m_NewHash = b->id;
		data->m_OldHash = a->id;
		data->sActionColumnText.LoadString(IDS_GITACTION_UPDATE_REF);
		data->sPathColumnText.Format(_T("%s\t %s -> %s"), str, 
				data->m_OldHash.ToString().Left(g_Git.GetShortHASHLength()),
				data->m_NewHash.ToString().Left(g_Git.GetShortHASHLength()));
	}

	m_arData.push_back(data);
	AddItemToList();

	if (m_pAnimate)
		m_pAnimate->Stop();
	if (m_pAnimate)
		m_pAnimate->ShowWindow(SW_HIDE);
	return TRUE;
}
BOOL CGitProgressList::Notify(const git_wc_notify_action_t /*action*/, const git_transfer_progress *stat)
{
	static unsigned int start = 0;
	unsigned int dt = GetCurrentTime() - start;
	double speed = 0;
	
	if (m_bCancelled)
		return FALSE;

	if (dt > 100)
	{
		start = GetCurrentTime();
		size_t ds = stat->received_bytes - m_TotalBytesTransferred;
		speed = ds * 1000.0/dt;
		m_TotalBytesTransferred = stat->received_bytes;
	}
	else
	{
		return TRUE;
	}

	int progress;
	progress = stat->received_objects + stat->indexed_objects;

	if ((stat->total_objects > 1000) && m_pProgControl && (!m_pProgControl->IsWindowVisible()))
	{
		if (m_pProgControl)
			m_pProgControl->ShowWindow(SW_SHOW);
	}

	if (m_pProgressLabelCtrl && m_pProgressLabelCtrl->IsWindowVisible())
		m_pProgressLabelCtrl->ShowWindow(SW_SHOW);

	if (m_pProgControl)
	{
		m_pProgControl->SetPos(progress);
		m_pProgControl->SetRange32(0, 2 * stat->total_objects);
	}
	if (m_pTaskbarList && m_pPostWnd)
	{
		m_pTaskbarList->SetProgressState(m_pPostWnd->GetSafeHwnd(), TBPF_NORMAL);
		m_pTaskbarList->SetProgressValue(m_pPostWnd->GetSafeHwnd(), progress, 2 * stat->total_objects);
	}

	CString progText;
	if (stat->received_bytes < 1024)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, (int64_t)stat->received_bytes);
	else if (stat->received_bytes < 1200000)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, (int64_t)stat->received_bytes / 1024);
	else
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)stat->received_bytes / 1024000.0));

	CString str;
	if(speed < 1024)
		str.Format(_T("%fB/s"), speed);
	else if(speed < 1024 * 1024)
		str.Format(_T("%.2fKB/s"), speed / 1024);
	else
		str.Format(_T("%.2fMB/s"), speed / 1024000.0);

	progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)str);
	if (m_pProgressLabelCtrl)
		m_pProgressLabelCtrl->SetWindowText(progText);

	return TRUE;
}

void CGitProgressList::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TRANSFERTIMER)
	{
		CString progText;
		CString progSpeed;
		progSpeed.Format(IDS_SVN_PROGRESS_BYTES_SEC, 0);
		progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)progSpeed);
		if (m_pProgressLabelCtrl)
			m_pProgressLabelCtrl->SetWindowText(progText);

		KillTimer(TRANSFERTIMER);
	}
	if (nIDEvent == VISIBLETIMER)
	{
		if (nEnsureVisibleCount)
			EnsureVisible(GetItemCount()-1, false);
		nEnsureVisibleCount = 0;
	}
}

void CGitProgressList::Sort()
{
	if(m_arData.size() < 2)
	{
		return;
	}

	// We need to sort the blocks which lie between the auxiliary entries
	// This is so that any aux data stays where it was
	NotificationDataVect::iterator actionBlockBegin;
	NotificationDataVect::iterator actionBlockEnd = m_arData.begin();	// We start searching from here

	for(;;)
	{
		// Search to the start of the non-aux entry in the next block
		actionBlockBegin = std::find_if(actionBlockEnd, m_arData.end(), std::not1(std::ptr_fun(&CGitProgressList::NotificationDataIsAux)));
		if(actionBlockBegin == m_arData.end())
		{
			// There are no more actions
			break;
		}
		// Now search to find the end of the block
		actionBlockEnd = std::find_if(actionBlockBegin+1, m_arData.end(), std::ptr_fun(&CGitProgressList::NotificationDataIsAux));
		// Now sort the block
		std::sort(actionBlockBegin, actionBlockEnd, &CGitProgressList::SortCompare);
	}
}

bool CGitProgressList::SortCompare(const NotificationData * pData1, const NotificationData * pData2)
{
	int result = 0;
	switch (m_nSortedColumn)
	{
	case 0:		//action column
		result = pData1->sActionColumnText.Compare(pData2->sActionColumnText);
		break;
	case 1:		//path column
		// Compare happens after switch()
		break;
	default:
		break;
	}

	// Sort by path if everything else is equal
	if (result == 0)
	{
		result = CTGitPath::Compare(pData1->path, pData2->path);
	}

	if (!m_bAscending)
		result = -result;
	return result < 0;
}

void CGitProgressList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (m_options & ProgOptDryRun)
		return;	// don't do anything in a dry-run.

	if (pWnd == this)
	{
		int selIndex = GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			// Menu was invoked from the keyboard rather than by right-clicking
			CRect rect;
			GetItemRect(selIndex, &rect, LVIR_LABEL);
			ClientToScreen(&rect);
			point = rect.CenterPoint();
		}

		if ((selIndex >= 0)&&(!m_bThreadRunning))
		{
			// entry is selected, thread has finished with updating so show the popup menu
			CIconMenu popup;
			if (popup.CreatePopupMenu())
			{
				bool bAdded = false;
				NotificationData * data = m_arData[selIndex];
				if ((data)&&(!data->path.IsDirectory()))
				{
/*
					if (data->action == svn_wc_notify_update_update || data->action == svn_wc_notify_resolved)
					{
						if (GetSelectedCount() == 1)
						{
							popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
							bAdded = true;
						}
					}

						if (data->bConflictedActionItem)
						{
							if (GetSelectedCount() == 1)
							{
								popup.AppendMenuIcon(ID_EDITCONFLICT, IDS_MENUCONFLICT,IDI_CONFLICT);
								popup.SetDefaultItem(ID_EDITCONFLICT, FALSE);
								popup.AppendMenuIcon(ID_CONFLICTRESOLVE, IDS_SVNPROGRESS_MENUMARKASRESOLVED,IDI_RESOLVE);
							}
							popup.AppendMenuIcon(ID_CONFLICTUSETHEIRS, IDS_SVNPROGRESS_MENUUSETHEIRS,IDI_RESOLVE);
							popup.AppendMenuIcon(ID_CONFLICTUSEMINE, IDS_SVNPROGRESS_MENUUSEMINE,IDI_RESOLVE);
						}
						else if ((data->content_state == svn_wc_notify_state_merged)||(GitProgress_Merge == m_Command)||(data->action == svn_wc_notify_resolved))
							popup.SetDefaultItem(ID_COMPARE, FALSE);
*/
					if (GetSelectedCount() == 1)
					{
						if ((data->action == git_wc_notify_add) ||
							(data->action == git_wc_notify_revert) ||
							(data->action == git_wc_notify_resolved) ||
							(data->action == git_wc_notify_checkout) ||
							(data->action == git_wc_notify_update_ref))
						{
							popup.AppendMenuIcon(ID_LOG, IDS_MENULOG, IDI_LOG);
						}

						if ((data->action == git_wc_notify_add) ||
							(data->action == git_wc_notify_revert) ||
							(data->action == git_wc_notify_resolved) ||
							(data->action == git_wc_notify_checkout))
						{
							popup.AppendMenu(MF_SEPARATOR, NULL);
							popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
							popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
							bAdded = true;
						}
					}
				} // if ((data)&&(!data->path.IsDirectory()))
				if (GetSelectedCount() == 1)
				{
					if (data)
					{
						CString sPath = GetPathFromColumnText(data->sPathColumnText);
						CTGitPath path = CTGitPath(sPath);
						if (!sPath.IsEmpty())
						{
							if (path.GetDirectory().Exists())
							{
								popup.AppendMenuIcon(ID_EXPLORE, IDS_SVNPROGRESS_MENUOPENPARENT, IDI_EXPLORER);
								bAdded = true;
							}
						}
					}
				}
				if (GetSelectedCount() > 0)
				{
					if (bAdded)
						popup.AppendMenu(MF_SEPARATOR, NULL);
					popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPYTOCLIPBOARD,IDI_COPYCLIP);
					bAdded = true;
				}
				if (bAdded)
				{
					int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
#if 0//need
					DialogEnableWindow(IDOK, FALSE);
#endif
					//this->SetPromptApp(&theApp);
					theApp.DoWaitCursor(1);
					bool bOpenWith = false;
					switch (cmd)
					{
					case ID_COPY:
						{
							CString sLines;
							POSITION pos = GetFirstSelectedItemPosition();
							while (pos)
							{
								int nItem = GetNextSelectedItem(pos);
								NotificationData * data = m_arData[nItem];
								if (data)
								{
									sLines += data->sPathColumnText;
									sLines += _T("\r\n");
								}
							}
							sLines.TrimRight();
							if (!sLines.IsEmpty())
							{
								CStringUtils::WriteAsciiStringToClipboard(sLines, GetSafeHwnd());
							}
						}
						break;
					case ID_EXPLORE:
						{
							if (!data)
								return;
							CString p = GetPathFromColumnText(data->sPathColumnText);
							if (PathFileExists(p))
							{
								ITEMIDLIST __unaligned * pidl = ILCreateFromPath(p);
								if (pidl)
								{
									SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
									ILFree(pidl);
								}
								break;
							}
							// if filepath does not exist any more, navigate to closest matching folder
							do
							{
								int pos = p.ReverseFind(_T('\\'));
								if (pos <= 3)
									break;
								p = p.Left(pos);
							} while (!PathFileExists(p));
							ShellExecute(GetSafeHwnd(), _T("explore"), p, nullptr, nullptr, SW_SHOW);
						}
						break;
#if 0
					case ID_COMPARE:
						{
							svn_revnum_t rev = -1;
							StringRevMap::iterator it = m_UpdateStartRevMap.end();
							if (data->basepath.IsEmpty())
								it = m_UpdateStartRevMap.begin();
							else
								it = m_UpdateStartRevMap.find(data->basepath.GetSVNApiPath(pool));
							if (it != m_UpdateStartRevMap.end())
								rev = it->second;
							// if the file was merged during update, do a three way diff between OLD, MINE, THEIRS
							if (data->content_state == svn_wc_notify_state_merged)
							{
								CTGitPath basefile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
								CTGitPath newfile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_HEAD);
								SVN svn;
								if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, basefile))
								{
									CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
									DialogEnableWindow(IDOK, TRUE);
									break;
								}
								// If necessary, convert the line-endings on the file before diffing
								if ((DWORD)CRegDWORD(_T("Software\\TortoiseGit\\ConvertBase"), TRUE))
								{
									CTGitPath temporaryFile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_BASE);
									if (!svn.Cat(data->path, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
									{
										temporaryFile.Reset();
										break;
									}
									else
									{
										newfile = temporaryFile;
									}
								}

								SetFileAttributes(newfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
								SetFileAttributes(basefile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
								CString revname, wcname, basename;
								revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
								wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
								basename.Format(IDS_DIFF_BASENAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
								CAppUtils::StartExtMerge(basefile, newfile, data->path, data->path, basename, revname, wcname, CString(), true);
							}
							else
							{
								CTGitPath tempfile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
								SVN svn;
								if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, tempfile))
								{
									CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
									DialogEnableWindow(IDOK, TRUE);
									break;
								}
								else
								{
									SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
									CString revname, wcname;
									revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
									wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
									CAppUtils::StartExtDiff(
										tempfile, data->path, revname, wcname,
										CAppUtils::DiffFlags().AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000)));
								}
							}
						}
						break;
					case ID_EDITCONFLICT:
						{
							CString sPath = GetPathFromColumnText(data->sPathColumnText);
							SVNDiff::StartConflictEditor(CTGitPath(sPath));
						}
						break;
					case ID_CONFLICTUSETHEIRS:
					case ID_CONFLICTUSEMINE:
					case ID_CONFLICTRESOLVE:
						{
							svn_wc_conflict_choice_t result = svn_wc_conflict_choose_merged;
							switch (cmd)
							{
							case ID_CONFLICTUSETHEIRS:
								result = svn_wc_conflict_choose_theirs_full;
								break;
							case ID_CONFLICTUSEMINE:
								result = svn_wc_conflict_choose_mine_full;
								break;
							case ID_CONFLICTRESOLVE:
								result = svn_wc_conflict_choose_merged;
								break;
							}
							SVN svn;
							POSITION pos = GetFirstSelectedItemPosition();
							CString sResolvedPaths;
							while (pos)
							{
								int nItem = GetNextSelectedItem(pos);
								NotificationData * data = m_arData[nItem];
								if (data)
								{
									if (data->bConflictedActionItem)
									{
										if (!svn.Resolve(data->path, result, FALSE))
										{
											CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
											DialogEnableWindow(IDOK, TRUE);
											break;
										}
										else
										{
											data->color = ::GetSysColor(COLOR_WINDOWTEXT);
											data->action = svn_wc_notify_resolved;
											data->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
											data->bConflictedActionItem = false;
											m_nConflicts--;

											if (m_nConflicts==0)
											{
												// When the last conflict is resolved we remove
												// the warning which we assume is in the last line.
												int nIndex = GetItemCount()-1;
												VERIFY(DeleteItem(nIndex));

												delete m_arData[nIndex];
												m_arData.pop_back();
											}
											sResolvedPaths += data->path.GetWinPathString() + _T("\n");
										}
									}
								}
							}
							Invalidate();
							CString info = BuildInfoString();
							SetDlgItemText(IDC_INFOTEXT, info);

							if (!sResolvedPaths.IsEmpty())
							{
								CString msg;
								msg.Format(IDS_SVNPROGRESS_RESOLVED, (LPCTSTR)sResolvedPaths);
								CMessageBox::Show(m_hWnd, msg, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
							}
						}
						break;
#endif
					case ID_LOG:
						{
							if (!data)
								return;
							CString cmd = _T("/command:log");
							CString sPath = GetPathFromColumnText(data->sPathColumnText);
							if(data->action == git_wc_notify_update_ref)
							{
								cmd += _T(" /path:\"") + GetPathFromColumnText(CString()) + _T("\"");
								if (!data->m_OldHash.IsEmpty())
									cmd += _T(" /startrev:") + data->m_OldHash.ToString();
								if (!data->m_NewHash.IsEmpty())
									cmd += _T(" /endrev:") + data->m_NewHash.ToString();
							}
							else
								cmd += _T(" /path:\"") + sPath + _T("\"");
							CAppUtils::RunTortoiseGitProc(cmd);
						}
						break;
					case ID_OPENWITH:
						bOpenWith = true;
					case ID_OPEN:
						{
							if (!data)
								return;
							int ret = 0;
							CString sWinPath = GetPathFromColumnText(data->sPathColumnText);
							if (!bOpenWith)
								ret = (int)ShellExecute(this->m_hWnd, NULL, (LPCTSTR)sWinPath, NULL, NULL, SW_SHOWNORMAL);
							if ((ret <= HINSTANCE_ERROR)||bOpenWith)
							{
								CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
								cmd += sWinPath + _T(" ");
								CAppUtils::LaunchApplication(cmd, NULL, false);
							}
						}
					}
#if 0 //need
					DialogEnableWindow(IDOK, TRUE);
#endif
					theApp.DoWaitCursor(-1);
				} // if (bAdded)
			}
		}
	}
}

void CGitProgressList::OnLvnBegindragSvnprogress(NMHDR* , LRESULT *pResult)
{
	//LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
#if 0
	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return;

	CDropFiles dropFiles; // class for creating DROPFILES struct

	int index;
	POSITION pos = GetFirstSelectedItemPosition();
	while ( (index = GetNextSelectedItem(pos)) >= 0 )
	{
		NotificationData * data = m_arData[index];

		if ( data->kind==svn_node_file || data->kind==svn_node_dir )
		{
			CString sPath = GetPathFromColumnText(data->sPathColumnText);

			dropFiles.AddFile( sPath );
		}
	}

	if (!dropFiles.IsEmpty())
	{
		dropFiles.CreateStructure();
	}
#endif
	*pResult = 0;
}

void CGitProgressList::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);
	if ((nType == SIZE_RESTORED)&&(m_bLastVisible))
	{
		if(!m_hWnd)
			return;

		int count = GetItemCount();
		if (count > 0)
			EnsureVisible(count-1, false);
	}
}

//////////////////////////////////////////////////////////////////////////
/// commands
//////////////////////////////////////////////////////////////////////////
bool CGitProgressList::CmdAdd(CString& sWindowTitle, bool& localoperation)
{
	localoperation = true;
	SetWindowTitle(IDS_PROGRS_TITLE_ADD, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	SetBackgroundImage(IDI_ADD_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_ADD)));

	m_itemCountTotal = m_targetPathList.GetCount();

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_ADD))
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
		{
			ReportGitError();
			return false;
		}

		CAutoIndex index;
		if (git_repository_index(index.GetPointer(), repo))
		{
			ReportGitError();
			return false;
		}
		if (git_index_read(index, true))
		{
			ReportGitError();
			return false;
		}

		for (m_itemCount = 0; m_itemCount < m_itemCountTotal; ++m_itemCount)
		{
			if (git_index_add_bypath(index, CUnicodeUtils::GetMulti(m_targetPathList[m_itemCount].GetGitPathString(), CP_UTF8)))
			{
				ReportGitError();
				return false;
			}
			Notify(m_targetPathList[m_itemCount], git_wc_notify_add);

			if (IsCancelled() == TRUE)
			{
				ReportUserCanceled();
				return false;
			}
		}

		if (git_index_write(index))
		{
			ReportGitError();
			return false;
		}
	}
	else
	{
		CMassiveGitTask mgt(L"add -f");
		if (!mgt.ExecuteWithNotify(&m_targetPathList, m_bCancelled, git_wc_notify_add, this))
			return false;
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}

bool CGitProgressList::CmdResolve(CString& sWindowTitle, bool& localoperation)
{

	localoperation = true;
	ASSERT(m_targetPathList.GetCount() == 1);
	SetWindowTitle(IDS_PROGRS_TITLE_RESOLVE, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	SetBackgroundImage(IDI_RESOLVE_BKG);

	m_itemCountTotal = m_targetPathList.GetCount();
	for (m_itemCount = 0; m_itemCount < m_itemCountTotal; ++m_itemCount)
	{
		CString cmd,out,tempmergefile;
		cmd.Format(_T("git.exe add -f -- \"%s\""),m_targetPathList[m_itemCount].GetGitPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			m_bErrorsOccurred=true;
			return false;
		}

		CAppUtils::RemoveTempMergeFile((CTGitPath &)m_targetPathList[m_itemCount]);

		Notify(m_targetPathList[m_itemCount], git_wc_notify_resolved);
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}

bool CGitProgressList::CmdRevert(CString& sWindowTitle, bool& localoperation)
{

	localoperation = true;
	SetWindowTitle(IDS_PROGRS_TITLE_REVERT, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	SetBackgroundImage(IDI_REVERT_BKG);

	m_itemCountTotal = 2 * m_selectedPaths.GetCount();
	CTGitPathList delList;
	for (m_itemCount = 0; m_itemCount < m_selectedPaths.GetCount(); ++m_itemCount)
	{
		CTGitPath path;
		int action;
		path.SetFromWin(g_Git.CombinePath(m_selectedPaths[m_itemCount]));
		action = m_selectedPaths[m_itemCount].m_Action;
		/* rename file can't delete because it needs original file*/
		if((!(action & CTGitPath::LOGACTIONS_ADDED)) &&
			(!(action & CTGitPath::LOGACTIONS_REPLACED)))
			delList.AddPath(path);
	}
	if (DWORD(CRegDWORD(_T("Software\\TortoiseGit\\RevertWithRecycleBin"), TRUE)))
		delList.DeleteAllFiles(true);

	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));
	for (int i = 0; i < m_selectedPaths.GetCount(); ++i)
	{
		if(g_Git.Revert(_T("HEAD"), (CTGitPath&)m_selectedPaths[i]))
		{
			CMessageBox::Show(NULL,_T("Revert Fail"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			m_bErrorsOccurred=true;
			return false;
		}
		Notify(m_selectedPaths[i], git_wc_notify_revert);
		++m_itemCount;

		if (IsCancelled() == TRUE)
		{
			ReportUserCanceled();
			return false;
		}
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_selectedPaths);

	return true;
}

bool CGitProgressList::CmdClone(CString& sWindowTitle, bool& /*localoperation*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_CLONE))
	{
		// should never run to here
		ASSERT(FALSE);
		return false;
	}
	this->m_TotalBytesTransferred = 0;

	SetWindowTitle(IDS_PROGRS_TITLE_CLONE, m_url.GetGitPathString(), sWindowTitle);
	SetBackgroundImage(IDI_SWITCH_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROG_CLONE)));

	if (m_url.IsEmpty() || m_targetPathList.IsEmpty())
		return false;

	git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
	checkout_opts.checkout_strategy = m_bNoCheckout? GIT_CHECKOUT_NONE : GIT_CHECKOUT_SAFE_CREATE;
	checkout_opts.progress_cb = CheckoutCallback;
	checkout_opts.progress_payload = this;

	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	callbacks.update_tips = RemoteUpdatetipsCallback;
	callbacks.sideband_progress = RemoteProgressCallback;
	callbacks.completion = RemoteCompletionCallback;
	callbacks.transfer_progress = FetchCallback;
	callbacks.credentials = CAppUtils::Git2GetUserPassword;
	callbacks.payload = this;

	CSmartAnimation animate(m_pAnimate);
	CAutoRepository cloned_repo;

	git_clone_options cloneOpts = GIT_CLONE_OPTIONS_INIT;
	cloneOpts.bare = m_bBare;
	cloneOpts.repository_cb = [](git_repository** out, const char* path, int bare, void* /*payload*/) -> int {
		git_repository_init_options init_options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
		init_options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
		init_options.flags |= bare ? GIT_REPOSITORY_INIT_BARE : 0;

		return git_repository_init_ext(out, path, &init_options);
	};

	cloneOpts.remote_cb = [](git_remote** out, git_repository* repo, const char* /*name*/, const char* url, void* payload) -> int {
		int error;

		CAutoRemote origin;
		if ((error = git_remote_create(origin.GetPointer(), repo, (const char*)payload, url)) < 0)
			return error;

		*out = origin.Detach();
		return 0;
	};
	cloneOpts.remote_callbacks = callbacks;
	CStringA remoteName = m_remote.IsEmpty() ? "origin" : CUnicodeUtils::GetUTF8(m_remote);
	cloneOpts.remote_cb_payload = (void*)(const char*)remoteName;

	CStringA checkout_branch = CUnicodeUtils::GetUTF8(m_RefSpec);
	if (!checkout_branch.IsEmpty())
		cloneOpts.checkout_branch = checkout_branch;
	cloneOpts.checkout_opts = checkout_opts;

	if (git_clone(cloned_repo.GetPointer(), CUnicodeUtils::GetUTF8(m_url.GetGitPathString()), CUnicodeUtils::GetUTF8(m_targetPathList[0].GetWinPathString()), &cloneOpts) < 0)
		goto error;

	return true;

error:
	ReportGitError();
	return false;
}
bool CGitProgressList::CmdSendMail(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_SendMail);
	SetWindowTitle(IDS_PROGRS_TITLE_SENDMAIL, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	//SetBackgroundImage(IDI_ADD_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_SENDMAIL)));

	return m_SendMail->Send(m_targetPathList, this) == 0;
}

bool CGitProgressList::CmdFetch(CString& sWindowTitle, bool& /*localoperation*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
	{
		// should never run to here
		ASSERT(0);
		return false;
	}
	this->m_TotalBytesTransferred = 0;

	SetWindowTitle(IDS_PROGRS_TITLE_FETCH, g_Git.m_CurrentDir, sWindowTitle);
	SetBackgroundImage(IDI_UPDATE_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_TITLE_FETCH)) + _T(" ") + m_url.GetGitPathString() + _T(" ") + m_RefSpec);

	CStringA url = CUnicodeUtils::GetMulti(m_url.GetGitPathString(), CP_UTF8);

	CSmartAnimation animate(m_pAnimate);

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		ReportGitError();
		return false;
	}

	CAutoRemote remote;
	// first try with a named remote (e.g. "origin")
	if (git_remote_load(remote.GetPointer(), repo, url) < 0) 
	{
		// retry with repository located at a specific url
		if (git_remote_create_anonymous(remote.GetPointer(), repo, url, nullptr) < 0)
		{
			ReportGitError();
			return false;
		}
	}

	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	callbacks.update_tips = RemoteUpdatetipsCallback;
	callbacks.sideband_progress = RemoteProgressCallback;
	callbacks.transfer_progress = FetchCallback;
	callbacks.completion = RemoteCompletionCallback;
	callbacks.credentials = CAppUtils::Git2GetUserPassword;
	callbacks.payload = this;

	git_remote_set_callbacks(remote, &callbacks);
	git_remote_set_autotag(remote, (git_remote_autotag_option_t)m_AutoTag);

	if (!m_RefSpec.IsEmpty() && git_remote_add_fetch(remote, CUnicodeUtils::GetUTF8(m_RefSpec)))
		goto error;

	// Connect to the remote end specifying that we want to fetch
	// information from it.
	if (git_remote_connect(remote, GIT_DIRECTION_FETCH) < 0)
		goto error;

	// Download the packfile and index it. This function updates the
	// amount of received data and the indexer stats which lets you
	// inform the user about progress.
	if (git_remote_download(remote) < 0)
		goto error;

	// Update the references in the remote's namespace to point to the
	// right commits. This may be needed even if there was no packfile
	// to download, which can happen e.g. when the branches have been
	// changed but all the neede objects are available locally.
	if (git_remote_update_tips(remote, nullptr, nullptr) < 0)
		goto error;

	git_remote_disconnect(remote);

	return true;

error:
	ReportGitError();
	return false;
}

bool CGitProgressList::CmdReset(CString& sWindowTitle, bool& /*localoperation*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_RESET))
	{
		// should never run to here
		ASSERT(0);
		return false;
	}

	SetWindowTitle(IDS_PROGRS_TITLE_RESET, g_Git.m_CurrentDir, sWindowTitle);
	SetBackgroundImage(IDI_UPDATE_BKG);
	int resetTypesResource[] = { IDS_RESET_SOFT, IDS_RESET_MIXED, IDS_RESET_HARD };
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_TITLE_RESET)) + _T(" ") + CString(MAKEINTRESOURCE(resetTypesResource[m_resetType])) + _T(" ") + m_revision);

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		ReportGitError();
		return false;
	}

	CSmartAnimation animate(m_pAnimate);
	CAutoObject target;
	if (git_revparse_single(target.GetPointer(), repo, CUnicodeUtils::GetUTF8(m_revision)))
		goto error;
	if (git_reset(repo, target, (git_reset_t)(m_resetType + 1), nullptr, nullptr))
		goto error;

	return true;

error:
	ReportGitError();
	return false;
}


void CGitProgressList::Init()
{
	SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	DeleteAllItems();
	int c = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		DeleteColumn(c--);

	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	InsertColumn(1, temp);

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}

	// Call this early so that the column headings aren't hidden before any
	// text gets added.
	ResizeColumns();

	SetTimer(VISIBLETIMER, 300, NULL);
}


void CGitProgressList::OnClose()
{
	if (m_bCancelled)
	{
		TerminateThread(m_pThread->m_hThread, (DWORD)-1);
		InterlockedExchange(&m_bThreadRunning, FALSE);
	}
	else
	{
		m_bCancelled = TRUE;
		return;
	}
	CListCtrl::OnClose();
}


BOOL CGitProgressList::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == 'A')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				// Ctrl-A -> select all
				SetSelectionMark(0);
				for (int i=0; i<GetItemCount(); ++i)
				{
					SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
		if ((pMsg->wParam == 'C')||(pMsg->wParam == VK_INSERT))
		{
			int selIndex = GetSelectionMark();
			if (selIndex >= 0)
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					//Ctrl-C -> copy to clipboard
					CString sClipdata;
					POSITION pos = GetFirstSelectedItemPosition();
					if (pos != NULL)
					{
						while (pos)
						{
							int nItem = GetNextSelectedItem(pos);
							CString sAction = GetItemText(nItem, 0);
							CString sPath = GetItemText(nItem, 1);
							CString sMime = GetItemText(nItem, 2);
							CString sLogCopyText;
							sLogCopyText.Format(_T("%s: %s  %s\r\n"),
								(LPCTSTR)sAction, (LPCTSTR)sPath, (LPCTSTR)sMime);
							sClipdata +=  sLogCopyText;
						}
						CStringUtils::WriteAsciiStringToClipboard(sClipdata);
					}
				}
			}
		}
	} // if (pMsg->message == WM_KEYDOWN)
	return CListCtrl::PreTranslateMessage(pMsg);
}

CString CGitProgressList::GetPathFromColumnText(const CString& sColumnText)
{
	CString sPath = sColumnText;
	if (sPath.Find(':')<0)
	{
		// the path is not absolute: add the common root of all paths to it
		sPath = g_Git.CombinePath(sColumnText);
	}
	return sPath;
}

void CGitProgressList::SetWindowTitle(UINT id, const CString& urlorpath, CString& dialogname)
{
	if (!m_bSetTitle || !m_pPostWnd)
		return;

	dialogname.LoadString(id);
	CAppUtils::SetWindowTitle(m_pPostWnd->GetSafeHwnd(), urlorpath, dialogname);
}
