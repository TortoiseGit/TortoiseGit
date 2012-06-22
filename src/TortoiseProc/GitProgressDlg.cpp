// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "GITProgressDlg.h"
#include "LogDlg.h"
#include "TGitPath.h"
#include "Registry.h"
#include "GitStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "SoundUtils.h"
#include "GitDiff.h"
#include "Hooks.h"
#include "DropFiles.h"
//#include "GitLogHelper.h"
#include "RegHistory.h"
//#include "ConflictResolveDlg.h"
#include "LogFile.h"
#include "ShellUpdater.h"
#include "IconMenu.h"
#include "BugTraqAssociations.h"
#include "patch.h"
#include "MassiveGitTask.h"
#include "git2.h"
#include "SmartHandle.h"

static UINT WM_GITPROGRESS = RegisterWindowMessage(_T("TORTOISEGIT_GITPROGRESS_MSG"));

BOOL	CGitProgressDlg::m_bAscending = FALSE;
int		CGitProgressDlg::m_nSortedColumn = -1;

#define TRANSFERTIMER	100
#define VISIBLETIMER	101

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

IMPLEMENT_DYNAMIC(CGitProgressDlg, CResizableStandAloneDialog)
CGitProgressDlg::CGitProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CGitProgressDlg::IDD, pParent)
	, m_bCancelled(FALSE)
	, m_pThread(NULL)
	, m_bErrorsOccurred(false)
#if 0
	, m_Revision(_T("HEAD"))
	//, m_RevisionEnd(0)
	, m_bLockWarning(false)
	, m_bLockExists(false)
	, m_bThreadRunning(FALSE)
	, m_nConflicts(0)
	, m_bMergesAddsDeletesOccurred(FALSE)

	, m_options(ProgOptNone)
	, m_dwCloseOnEnd((DWORD)-1)
	, m_bFinishedItemAdded(false)
	, m_bLastVisible(false)
//	, m_depth(svn_depth_unknown)
	, m_itemCount(-1)
	, m_itemCountTotal(-1)
	, m_AlwaysConflicted(false)
	, m_BugTraqProvider(NULL)
	, sDryRun(MAKEINTRESOURCE(IDS_PROGRS_DRYRUN))
	, sRecordOnly(MAKEINTRESOURCE(IDS_MERGE_RECORDONLY))
#endif
{
}

CGitProgressDlg::~CGitProgressDlg()
{
	for (size_t i=0; i<m_arData.size(); i++)
	{
		delete m_arData[i];
	}
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CGitProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
}

BEGIN_MESSAGE_MAP(CGitProgressDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SVNPROGRESS, OnNMCustomdrawSvnprogress)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_SVNPROGRESS, OnNMDblclkSvnprogress)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickSvnprogress)
	ON_WM_SETCURSOR()
	ON_WM_CONTEXTMENU()
	ON_REGISTERED_MESSAGE(WM_GITPROGRESS, OnGitProgress)
	ON_WM_TIMER()
	ON_EN_SETFOCUS(IDC_INFOTEXT, &CGitProgressDlg::OnEnSetfocusInfotext)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_SVNPROGRESS, &CGitProgressDlg::OnLvnBegindragSvnprogress)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_SVNPROGRESS, &CGitProgressDlg::OnLvnGetdispinfoSvnprogress)
	ON_BN_CLICKED(IDC_NONINTERACTIVE, &CGitProgressDlg::OnBnClickedNoninteractive)
	ON_MESSAGE(WM_SHOWCONFLICTRESOLVER, OnShowConflictResolver)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()

BOOL CGitProgressDlg::Cancel()
{
	return m_bCancelled;
}

LRESULT CGitProgressDlg::OnShowConflictResolver(WPARAM /*wParam*/, LPARAM /*lParam*/)
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
svn_wc_conflict_choice_t CGitProgressDlg::ConflictResolveCallback(const svn_wc_conflict_description_t *description, CString& mergedfile)
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
void CGitProgressDlg::AddItemToList()
{
	int totalcount = m_ProgList.GetItemCount();

	m_ProgList.SetItemCountEx(totalcount+1, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);
	// make columns width fit
	if (iFirstResized < 30)
	{
		// only resize the columns for the first 30 or so entries.
		// after that, don't resize them anymore because that's an
		// expensive function call and the columns will be sized
		// close enough already.
		ResizeColumns();
		iFirstResized++;
	}

	// Make sure the item is *entirely* visible even if the horizontal
	// scroll bar is visible.
	int count = m_ProgList.GetCountPerPage();
	if (totalcount <= (m_ProgList.GetTopIndex() + count + nEnsureVisibleCount + 2))
	{
		nEnsureVisibleCount++;
		m_bLastVisible = true;
	}
	else
	{
		nEnsureVisibleCount = 0;
		if (IsIconic() == 0)
			m_bLastVisible = false;
	}
}


BOOL CGitProgressDlg::Notify(const CTGitPath& path, git_wc_notify_action_t action,
							 int /*status*/ ,
							 CString *strErr
							 /*
							 svn_node_kind_t kind, const CString& mime_type,
							 svn_wc_notify_state_t content_state,
							 svn_wc_notify_state_t prop_state, LONG rev,
							 const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
							 const CString& changelistname,
							 svn_merge_range_t * range,
							 svn_error_t * err, apr_pool_t * pool
							 */)
{
	bool bNoNotify = false;
	bool bDoAddData = true;
	NotificationData * data = new NotificationData();
	data->path = path;
	data->action = action;
	data->sPathColumnText=path.GetGitPathString();
	data->bAuxItem = false;
#if 0
	data->kind = kind;
	data->mime_type = mime_type;
	data->content_state = content_state;
	data->prop_state = prop_state;
	data->rev = rev;
	data->lock_state = lock_state;
	data->changelistname = changelistname;
	if ((lock)&&(lock->owner))
		data->owner = CUnicodeUtils::GetUnicode(lock->owner);
	data->sPathColumnText = path.GetUIPathString();
	if (!m_basePath.IsEmpty())
		data->basepath = m_basePath;
	if (range)
		data->merge_range = *range;
#endif
	switch (data->action)
	{
	case git_wc_notify_add:
	//case svn_wc_notify_update_add:
	//	if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
	//	{
	//		data->color = m_Colors.GetColor(CColors::Conflict);
	//		data->bConflictedActionItem = true;
	//		data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
	//		m_nConflicts++;
	//	}
	//	else
	//	{
	//		m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_ADD);
			data->color = m_Colors.GetColor(CColors::Added);
	//	}
		break;
	case git_wc_notify_sendmail_start:
		data->bAuxItem = true;
		data->sActionColumnText.LoadString(IDS_SVNACTION_SENDMAIL_START);
		data->color = m_Colors.GetColor(CColors::Modified);
		break;

	case git_wc_notify_sendmail_error:
		data->bAuxItem = true;
		data->sActionColumnText.LoadString(IDS_SVNACTION_SENDMAIL_ERROR);
		if(strErr)
			data->sPathColumnText = *strErr;
		else
			data->sPathColumnText.Empty();
		data->color = m_Colors.GetColor(CColors::Modified);
		break;

	case git_wc_notify_sendmail_done:

		data->sActionColumnText.LoadString(IDS_SVNACTION_SENDMAIL_DONE);
		data->sPathColumnText.Empty();
		data->color = m_Colors.GetColor(CColors::Modified);
		break;

	case git_wc_notify_sendmail_retry:
		data->sActionColumnText.LoadString(IDS_SVNACTION_SENDMAIL_RETRY);
		data->sPathColumnText.Empty();
		data->color = m_Colors.GetColor(CColors::Modified);
		break;


	case git_wc_notify_resolved:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
		break;

	case git_wc_notify_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REVERT);
		break;

#if 0
	case svn_wc_notify_commit_added:
		data->sActionColumnText.LoadString(IDS_SVNACTION_ADDING);
		data->color = m_Colors.GetColor(CColors::Added);
		break;
	case svn_wc_notify_copy:
		data->sActionColumnText.LoadString(IDS_SVNACTION_COPY);
		break;
	case svn_wc_notify_commit_modified:
		data->sActionColumnText.LoadString(IDS_SVNACTION_MODIFIED);
		data->color = m_Colors.GetColor(CColors::Modified);
		break;
	case svn_wc_notify_delete:
	case svn_wc_notify_update_delete:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETE);
		m_bMergesAddsDeletesOccurred = true;
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_commit_deleted:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETING);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_restore:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESTORE);
		break;

	case svn_wc_notify_update_replace:
	case svn_wc_notify_commit_replaced:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_exists:
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			m_nConflicts++;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
		}
		else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
		{
			data->color = m_Colors.GetColor(CColors::Merged);
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
		}
		else
			data->sActionColumnText.LoadString(IDS_SVNACTION_EXISTS);
		break;
	case svn_wc_notify_update_update:
		// if this is an inoperative dir change, don't show the notification.
		// an inoperative dir change is when a directory gets updated without
		// any real change in either text or properties.
		if ((kind == svn_node_dir)
			&& ((prop_state == svn_wc_notify_state_inapplicable)
			|| (prop_state == svn_wc_notify_state_unknown)
			|| (prop_state == svn_wc_notify_state_unchanged)))
		{
			bNoNotify = true;
			break;
		}
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			m_nConflicts++;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
		}
		else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
		{
			data->color = m_Colors.GetColor(CColors::Merged);
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
		}
		else if (((data->content_state != svn_wc_notify_state_unchanged)&&(data->content_state != svn_wc_notify_state_unknown)) ||
			((data->prop_state != svn_wc_notify_state_unchanged)&&(data->prop_state != svn_wc_notify_state_unknown)))
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_UPDATE);
		}
		else
		{
			bNoNotify = true;
			break;
		}
		if (lock_state == svn_wc_notify_lock_state_unlocked)
		{
			CString temp(MAKEINTRESOURCE(IDS_SVNACTION_UNLOCKED));
			data->sActionColumnText += _T(", ") + temp;
		}
		break;

	case svn_wc_notify_update_external:
		// For some reason we build a list of externals...
		m_ExtStack.AddHead(path.GetUIPathString());
		data->sActionColumnText.LoadString(IDS_SVNACTION_EXTERNAL);
		data->bAuxItem = true;
		break;

	case svn_wc_notify_update_completed:
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
			data->bAuxItem = true;
			bool bEmpty = !!m_ExtStack.IsEmpty();
			if (!bEmpty)
				data->sPathColumnText.Format(IDS_PROGRS_PATHATREV, (LPCTSTR)m_ExtStack.RemoveHead(), rev);
			else
				data->sPathColumnText.Format(IDS_PROGRS_ATREV, rev);

			if ((m_nConflicts>0)&&(bEmpty))
			{
				// We're going to add another aux item - let's shove this current onto the list first
				// I don't really like this, but it will do for the moment.
				m_arData.push_back(data);
				AddItemToList();

				data = new NotificationData();
				data->bAuxItem = true;
				data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
				data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
				data->color = m_Colors.GetColor(CColors::Conflict);
				CSoundUtils::PlayTSVNWarning();
				// This item will now be added after the switch statement
			}
			if (!m_basePath.IsEmpty())
				m_FinishedRevMap[m_basePath.GetSVNApiPath(pool)] = rev;
			m_RevisionEnd = rev;
			m_bFinishedItemAdded = true;
		}
		break;
	case svn_wc_notify_commit_postfix_txdelta:
		data->sActionColumnText.LoadString(IDS_SVNACTION_POSTFIX);
		break;
	case svn_wc_notify_failed_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDREVERT);
		break;
	case svn_wc_notify_status_completed:
	case svn_wc_notify_status_external:
		data->sActionColumnText.LoadString(IDS_SVNACTION_STATUS);
		break;
	case svn_wc_notify_skip:
		if ((content_state == svn_wc_notify_state_missing)||(content_state == svn_wc_notify_state_obstructed)||(content_state == svn_wc_notify_state_conflicted))
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIPMISSING);

			// The color settings dialog describes the red color with
			// "possible or real conflict / obstructed" which also applies to
			// skipped targets during a merge. So we just use the same color.
			data->color = m_Colors.GetColor(CColors::Conflict);
		}
		else
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIP);
		break;
	case svn_wc_notify_locked:
		if ((lock)&&(lock->owner))
			data->sActionColumnText.Format(IDS_SVNACTION_LOCKEDBY, (LPCTSTR)CUnicodeUtils::GetUnicode(lock->owner));
		break;
	case svn_wc_notify_unlocked:
		data->sActionColumnText.LoadString(IDS_SVNACTION_UNLOCKED);
		break;
	case svn_wc_notify_failed_lock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDLOCK);
		m_arData.push_back(data);
		AddItemToList();
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if (err->apr_err == SVN_ERR_FS_OUT_OF_DATE)
			m_bLockWarning = true;
		if (err->apr_err == SVN_ERR_FS_PATH_ALREADY_LOCKED)
			m_bLockExists = true;
		break;
	case svn_wc_notify_failed_unlock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDUNLOCK);
		m_arData.push_back(data);
		AddItemToList();
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if (err->apr_err == SVN_ERR_FS_OUT_OF_DATE)
			m_bLockWarning = true;
		break;
	case svn_wc_notify_changelist_set:
		data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTSET, (LPCTSTR)data->changelistname);
		break;
	case svn_wc_notify_changelist_clear:
		data->sActionColumnText.LoadString(IDS_SVNACTION_CHANGELISTCLEAR);
		break;
	case svn_wc_notify_changelist_moved:
		data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTMOVED, (LPCTSTR)data->changelistname);
		break;
	case svn_wc_notify_foreign_merge_begin:
	case svn_wc_notify_merge_begin:
		if (range == NULL)
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGEBEGINNONE);
		else if ((data->merge_range.start == data->merge_range.end) || (data->merge_range.start == data->merge_range.end - 1))
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLE, data->merge_range.end);
		else if (data->merge_range.start - 1 == data->merge_range.end)
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLEREVERSE, data->merge_range.start);
		else if (data->merge_range.start < data->merge_range.end)
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINMULTIPLE, data->merge_range.start + 1, data->merge_range.end);
		else
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINMULTIPLEREVERSE, data->merge_range.start, data->merge_range.end + 1);
		data->bAuxItem = true;
		break;
#endif
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
				m_itemCount--;

				CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
				progControl->ShowWindow(SW_SHOW);
				progControl->SetPos(m_itemCountTotal - m_itemCount);
				progControl->SetRange32(0, m_itemCountTotal);
				if (m_pTaskbarList)
				{
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
					m_pTaskbarList->SetProgressValue(m_hWnd, m_itemCountTotal-m_itemCount, m_itemCountTotal);
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


CString CGitProgressDlg::BuildInfoString()
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
				conflicted++;
			else
				added++;
			break;
		case svn_wc_notify_copy:
			copied++;
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			deleted++;
			break;
		case svn_wc_notify_restore:
			restored++;
			break;
		case svn_wc_notify_revert:
			reverted++;
			break;
		case svn_wc_notify_resolved:
			resolved++;
			break;
		case svn_wc_notify_update_update:
			if (dat->bConflictedActionItem)
				conflicted++;
			else if ((dat->content_state == svn_wc_notify_state_merged) || (dat->prop_state == svn_wc_notify_state_merged))
				merged++;
			else
				updated++;
			break;
		case svn_wc_notify_commit_modified:
			modified++;
			break;
		case svn_wc_notify_skip:
			skipped++;
			break;
		case svn_wc_notify_commit_replaced:
			replaced++;
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

void CGitProgressDlg::SetSelectedList(const CTGitPathList& selPaths)
{
	m_selectedPaths = selPaths;
}

void CGitProgressDlg::ResizeColumns()
{
	m_ProgList.SetRedraw(FALSE);

	TCHAR textbuf[MAX_PATH];

	int maxcol = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		// find the longest width of all items
		int count = m_ProgList.GetItemCount();
		HDITEM hdi = {0};
		hdi.mask = HDI_TEXT;
		hdi.pszText = textbuf;
		hdi.cchTextMax = sizeof(textbuf);
		((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItem(col, &hdi);
		int cx = m_ProgList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin

		for (int index = 0; index<count; ++index)
		{
			// get the width of the string and add 12 pixels for the column separator and margins
			int linewidth = cx;
			switch (col)
			{
			case 0:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->sActionColumnText) + 12;
				break;
			case 1:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->sPathColumnText) + 12;
				break;
			case 2:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->mime_type) + 12;
				break;
			}
			if (cx < linewidth)
				cx = linewidth;
		}
		m_ProgList.SetColumnWidth(col, cx);
	}

	m_ProgList.SetRedraw(TRUE);
}

BOOL CGitProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = ::LoadLibrary(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
	}
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

	m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_ProgList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ProgList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ProgList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ProgList.InsertColumn(1, temp);
	temp.LoadString(IDS_PROGRS_MIMETYPE);
	m_ProgList.InsertColumn(2, temp);

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

	UpdateData(FALSE);

	// Call this early so that the column headings aren't hidden before any
	// text gets added.
	ResizeColumns();

	SetTimer(VISIBLETIMER, 300, NULL);

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_NONINTERACTIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	//SetPromptParentWindow(this->m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("GITProgressDlg"));
	return TRUE;
}

bool CGitProgressDlg::SetBackgroundImage(UINT nID)
{
	return CAppUtils::SetListCtrlBackgroundImage(m_ProgList.GetSafeHwnd(), nID);
}

void CGitProgressDlg::ReportGitError()
{
	ReportError(CString(giterr_last()->message));
}

void CGitProgressDlg::ReportError(const CString& sError)
{
	CSoundUtils::PlayTGitError();
	ReportString(sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), m_Colors.GetColor(CColors::Conflict));
	m_bErrorsOccurred = true;
}

void CGitProgressDlg::ReportWarning(const CString& sWarning)
{
	CSoundUtils::PlayTGitWarning();
	ReportString(sWarning, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)), m_Colors.GetColor(CColors::Conflict));
}

void CGitProgressDlg::ReportNotification(const CString& sNotification)
{
	CSoundUtils::PlayTGitNotification();
	ReportString(sNotification, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
}

void CGitProgressDlg::ReportCmd(const CString& sCmd)
{
	ReportString(sCmd, CString(MAKEINTRESOURCE(IDS_PROGRS_CMDINFO)), m_Colors.GetColor(CColors::Cmd));
}

void CGitProgressDlg::ReportString(CString sMessage, const CString& sMsgKind, COLORREF color)
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

UINT CGitProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CGitProgressDlg*)pVoid)->ProgressThread();
}

UINT CGitProgressDlg::ProgressThread()
{
	// The SetParams function should have loaded something for us

	CString temp;
	CString sWindowTitle;
	bool localoperation = false;
	bool bSuccess = false;
	m_AlwaysConflicted = false;

	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, TRUE);
//	SetAndClearProgressInfo(m_hWnd);
	m_itemCount = m_itemCountTotal;

	InterlockedExchange(&m_bThreadRunning, TRUE);
	iFirstResized = 0;
	bSecondResized = FALSE;
	m_bFinishedItemAdded = false;
	CTime startTime = CTime::GetCurrentTime();

	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);

	switch (m_Command)
	{
	case GitProgress_Add:
		bSuccess = CmdAdd(sWindowTitle, localoperation);
		break;
	case GitProgress_Copy:
		bSuccess = CmdCopy(sWindowTitle, localoperation);
		break;
	case GitProgress_Export:
		bSuccess = CmdExport(sWindowTitle, localoperation);
		break;
	case GitProgress_Rename:
		bSuccess = CmdRename(sWindowTitle, localoperation);
		break;
	case GitProgress_Resolve:
		bSuccess = CmdResolve(sWindowTitle, localoperation);
		break;
	case GitProgress_Revert:
		bSuccess = CmdRevert(sWindowTitle, localoperation);
		break;
	case GitProgress_Switch:
		bSuccess = CmdSwitch(sWindowTitle, localoperation);
		break;
	case GitProgress_SendMail:
		bSuccess = CmdSendMail(sWindowTitle, localoperation);
		break;
	}
	if (!bSuccess)
		temp.LoadString(IDS_PROGRS_TITLEFAILED);
	else
		temp.LoadString(IDS_PROGRS_TITLEFIN);
	sWindowTitle = sWindowTitle + _T(" ") + temp;
	SetWindowText(sWindowTitle);

	KillTimer(TRANSFERTIMER);
	KillTimer(VISIBLETIMER);

	if (m_pTaskbarList)
	{
		if (DidErrorsOccur())
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
			m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
		}
		else
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}

	DialogEnableWindow(IDCANCEL, FALSE);
	DialogEnableWindow(IDOK, TRUE);

	CString info = BuildInfoString();
	if (!bSuccess)
		info.LoadString(IDS_PROGRS_INFOFAILED);
	SetDlgItemText(IDC_INFOTEXT, info);
	ResizeColumns();
	CWnd * pWndOk = GetDlgItem(IDOK);
	if (pWndOk && ::IsWindow(pWndOk->GetSafeHwnd()))
	{
		SendMessage(DM_SETDEFID, IDOK);
		GetDlgItem(IDOK)->SetFocus();
	}

	CString sFinalInfo;
	if (!m_sTotalBytesTransferred.IsEmpty())
	{
		CTimeSpan time = CTime::GetCurrentTime() - startTime;
		temp.Format(IDS_PROGRS_TIME, (LONG)time.GetTotalMinutes(), (LONG)time.GetSeconds());
		sFinalInfo.Format(IDS_PROGRS_FINALINFO, m_sTotalBytesTransferred, (LPCTSTR)temp);
		SetDlgItemText(IDC_PROGRESSLABEL, sFinalInfo);
	}
	else
		GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_PROGRESSBAR)->ShowWindow(SW_HIDE);

	if (!m_bFinishedItemAdded)
	{
		// there's no "finished: xxx" line at the end. We add one here to make
		// sure the user sees that the command is actually finished.
		NotificationData * data = new NotificationData();
		data->bAuxItem = true;
		data->sActionColumnText.LoadString(IDS_PROGRS_FINISHED);
		m_arData.push_back(data);
		AddItemToList();
	}

	int count = m_ProgList.GetItemCount();
	if ((count > 0)&&(m_bLastVisible))
		m_ProgList.EnsureVisible(count-1, FALSE);

	CLogFile logfile;
	if (logfile.Open())
	{
		logfile.AddTimeLine();
		for (size_t i=0; i<m_arData.size(); i++)
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
	RefreshCursor();

#if 0
	DWORD dwAutoClose = CRegStdDWORD(_T("Software\\TortoiseGit\\AutoClose"), CLOSE_MANUAL);
	if (m_options & ProgOptDryRun)
		dwAutoClose = 0;		// dry run means progress dialog doesn't auto close at all
	if (!m_bLastVisible)
		dwAutoClose = 0;
	if (m_dwCloseOnEnd != (DWORD)-1)
		dwAutoClose = m_dwCloseOnEnd;		// command line value has priority over setting value
	if ((dwAutoClose == CLOSE_NOERRORS)&&(!m_bErrorsOccurred))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_NOCONFLICTS)&&(!m_bErrorsOccurred)&&(m_nConflicts==0))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_NOMERGES)&&(!m_bErrorsOccurred)&&(m_nConflicts==0)&&(!m_bMergesAddsDeletesOccurred))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_LOCAL)&&(!m_bErrorsOccurred)&&(m_nConflicts==0)&&(localoperation))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
#endif

	//Don't do anything here which might cause messages to be sent to the window
	//The window thread is probably now blocked in OnOK if we've done an auto close
	return 0;
}

void CGitProgressDlg::OnBnClickedLogbutton()
{
	switch(this->m_Command)
	{
	case GitProgress_Add:
	case GitProgress_Resolve:
		{
			CString cmd = _T(" /command:commit");
			cmd += _T(" /path:\"")+g_Git.m_CurrentDir+_T("\"");

			CAppUtils::RunTortoiseProc(cmd);
			this->EndDialog(IDOK);
			break;
		}
	}
#if 0
	if (m_targetPathList.GetCount() != 1)
		return;
	StringRevMap::iterator it = m_UpdateStartRevMap.begin();
	svn_revnum_t rev = -1;
	if (it != m_UpdateStartRevMap.end())
	{
		rev = it->second;
	}
	CLogDlg dlg;
	dlg.SetParams(m_targetPathList[0], m_RevisionEnd, m_RevisionEnd, rev, 0, TRUE);
	dlg.DoModal();
#endif
}


void CGitProgressDlg::OnClose()
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
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CGitProgressDlg::OnOK()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
	{
		// I have made this wait a sensible amount of time (10 seconds) for the thread to finish
		// You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you
		// don't do any more operations on the window which might require message passing
		// If you try to send windows messages once we're waiting here, then the thread can't finished
		// because the Window's message loop is blocked at this wait
		WaitForSingleObject(m_pThread->m_hThread, 10000);
		__super::OnOK();
	}
	m_bCancelled = TRUE;
}

void CGitProgressDlg::OnCancel()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
		__super::OnCancel();
	m_bCancelled = TRUE;
}

void CGitProgressDlg::OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
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
						int cWidth = m_ProgList.GetColumnWidth(1);
						cWidth = max(12, cWidth-12);
						CDC * pDC = m_ProgList.GetDC();
                        if (pDC != NULL)
                        {
						    CFont * pFont = pDC->SelectObject(m_ProgList.GetFont());
						    PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
						    pDC->SelectObject(pFont);
							ReleaseDC(pDC);
                        }
					}
					break;
				case 2:
					lstrcpyn(m_columnbuf, data->mime_type, MAX_PATH);
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

void CGitProgressDlg::OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
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

void CGitProgressDlg::OnNMDblclkSvnprogress(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
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

void CGitProgressDlg::OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
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
	m_ProgList.SetRedraw(FALSE);
	m_ProgList.DeleteAllItems();
	m_ProgList.SetItemCountEx (static_cast<int>(m_arData.size()));

	m_ProgList.SetRedraw(TRUE);

	*pResult = 0;
}

bool CGitProgressDlg::NotificationDataIsAux(const NotificationData* pData)
{
	return pData->bAuxItem;
}

LRESULT CGitProgressDlg::OnGitProgress(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
#if 0
	SVNProgress * pProgressData = (SVNProgress *)lParam;
	CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
	if ((pProgressData->total > 1000)&&(!progControl->IsWindowVisible()))
	{
		progControl->ShowWindow(SW_SHOW);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
	}
	if (((pProgressData->total < 0)&&(pProgressData->progress > 1000)&&(progControl->IsWindowVisible()))&&(m_itemCountTotal<0))
	{
		progControl->ShowWindow(SW_HIDE);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
	}
	if (!GetDlgItem(IDC_PROGRESSLABEL)->IsWindowVisible())
		GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_SHOW);
	SetTimer(TRANSFERTIMER, 2000, NULL);
	if ((pProgressData->total > 0)&&(pProgressData->progress > 1000))
	{
		progControl->SetPos((int)pProgressData->progress);
		progControl->SetRange32(0, (int)pProgressData->total);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, pProgressData->progress, pProgressData->total);
		}
	}
	CString progText;
	if (pProgressData->overall_total < 1024)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pProgressData->overall_total);
	else if (pProgressData->overall_total < 1200000)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pProgressData->overall_total / 1024);
	else
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pProgressData->overall_total / 1024000.0));
	progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)pProgressData->SpeedString);
	SetDlgItemText(IDC_PROGRESSLABEL, progText);
#endif
	return 0;
}

void CGitProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TRANSFERTIMER)
	{
		CString progText;
		CString progSpeed;
		progSpeed.Format(IDS_SVN_PROGRESS_BYTES_SEC, 0);
		progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)progSpeed);
		SetDlgItemText(IDC_PROGRESSLABEL, progText);
		KillTimer(TRANSFERTIMER);
	}
	if (nIDEvent == VISIBLETIMER)
	{
		if (nEnsureVisibleCount)
			m_ProgList.EnsureVisible(m_ProgList.GetItemCount()-1, false);
		nEnsureVisibleCount = 0;
	}
}

void CGitProgressDlg::Sort()
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
		actionBlockBegin = std::find_if(actionBlockEnd, m_arData.end(), std::not1(std::ptr_fun(&CGitProgressDlg::NotificationDataIsAux)));
		if(actionBlockBegin == m_arData.end())
		{
			// There are no more actions
			break;
		}
		// Now search to find the end of the block
		actionBlockEnd = std::find_if(actionBlockBegin+1, m_arData.end(), std::ptr_fun(&CGitProgressDlg::NotificationDataIsAux));
		// Now sort the block
		std::sort(actionBlockBegin, actionBlockEnd, &CGitProgressDlg::SortCompare);
	}
}

bool CGitProgressDlg::SortCompare(const NotificationData * pData1, const NotificationData * pData2)
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
	case 2:		//mime-type column
		result = pData1->mime_type.Compare(pData2->mime_type);
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

BOOL CGitProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_SVNPROGRESS)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CGitProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!m_bThreadRunning)&&(!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
		if (pMsg->wParam == 'A')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				// Ctrl-A -> select all
				m_ProgList.SetSelectionMark(0);
				for (int i=0; i<m_ProgList.GetItemCount(); ++i)
				{
					m_ProgList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
		if ((pMsg->wParam == 'C')||(pMsg->wParam == VK_INSERT))
		{
			int selIndex = m_ProgList.GetSelectionMark();
			if (selIndex >= 0)
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					//Ctrl-C -> copy to clipboard
					CString sClipdata;
					POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
					if (pos != NULL)
					{
						while (pos)
						{
							int nItem = m_ProgList.GetNextSelectedItem(pos);
							CString sAction = m_ProgList.GetItemText(nItem, 0);
							CString sPath = m_ProgList.GetItemText(nItem, 1);
							CString sMime = m_ProgList.GetItemText(nItem, 2);
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
	return __super::PreTranslateMessage(pMsg);
}

void CGitProgressDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/)
{
#if 0
	if (m_options & ProgOptDryRun)
		return;	// don't do anything in a dry-run.

	if (pWnd == &m_ProgList)
	{
		int selIndex = m_ProgList.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			// Menu was invoked from the keyboard rather than by right-clicking
			CRect rect;
			m_ProgList.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_ProgList.ClientToScreen(&rect);
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
					if (data->action == svn_wc_notify_update_update || data->action == svn_wc_notify_resolved)
					{
						if (m_ProgList.GetSelectedCount() == 1)
						{
							popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
							bAdded = true;
						}
					}
						if (data->bConflictedActionItem)
						{
							if (m_ProgList.GetSelectedCount() == 1)
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

					if (m_ProgList.GetSelectedCount() == 1)
					{
						if ((data->action == svn_wc_notify_add)||
							(data->action == svn_wc_notify_update_add)||
							(data->action == svn_wc_notify_commit_added)||
							(data->action == svn_wc_notify_commit_modified)||
							(data->action == svn_wc_notify_restore)||
							(data->action == svn_wc_notify_revert)||
							(data->action == svn_wc_notify_resolved)||
							(data->action == svn_wc_notify_commit_replaced)||
							(data->action == svn_wc_notify_commit_modified)||
							(data->action == svn_wc_notify_commit_postfix_txdelta)||
							(data->action == svn_wc_notify_update_update))
						{
							popup.AppendMenuIcon(ID_LOG, IDS_MENULOG,IDI_LOG);
							if (data->action == svn_wc_notify_update_update)
								popup.AppendMenu(MF_SEPARATOR, NULL);
							popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
							popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
							bAdded = true;
						}
					}
				} // if ((data)&&(!data->path.IsDirectory()))
				if (m_ProgList.GetSelectedCount() == 1)
				{
					if (data)
					{
						CString sPath = GetPathFromColumnText(data->sPathColumnText);
						if ((!sPath.IsEmpty())&&(!SVN::PathIsURL(CTGitPath(sPath))))
						{
							CTGitPath path = CTGitPath(sPath);
							if (path.GetDirectory().Exists())
							{
								popup.AppendMenuIcon(ID_EXPLORE, IDS_SVNPROGRESS_MENUOPENPARENT, IDI_EXPLORER);
								bAdded = true;
							}
						}
					}
				}
				if (m_ProgList.GetSelectedCount() > 0)
				{
					if (bAdded)
						popup.AppendMenu(MF_SEPARATOR, NULL);
					popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPYTOCLIPBOARD,IDI_COPYCLIP);
					bAdded = true;
				}
				if (bAdded)
				{
					int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
					DialogEnableWindow(IDOK, FALSE);
					this->SetPromptApp(&theApp);
					theApp.DoWaitCursor(1);
					bool bOpenWith = false;
					switch (cmd)
					{
					case ID_COPY:
						{
							CString sLines;
							POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
							while (pos)
							{
								int nItem = m_ProgList.GetNextSelectedItem(pos);
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
							CString sPath = GetPathFromColumnText(data->sPathColumnText);

							CTGitPath path = CTGitPath(sPath);
							ShellExecute(m_hWnd, _T("explore"), path.GetDirectory().GetWinPath(), NULL, path.GetDirectory().GetWinPath(), SW_SHOW);
						}
						break;
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
							POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
							CString sResolvedPaths;
							while (pos)
							{
								int nItem = m_ProgList.GetNextSelectedItem(pos);
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
												int nIndex = m_ProgList.GetItemCount()-1;
												VERIFY(m_ProgList.DeleteItem(nIndex));

												delete m_arData[nIndex];
												m_arData.pop_back();
											}
											sResolvedPaths += data->path.GetWinPathString() + _T("\n");
										}
									}
								}
							}
							m_ProgList.Invalidate();
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
					case ID_LOG:
						{
							svn_revnum_t rev = m_RevisionEnd;
							if (!data->basepath.IsEmpty())
							{
								StringRevMap::iterator it = m_FinishedRevMap.find(data->basepath.GetSVNApiPath(pool));
								if (it != m_FinishedRevMap.end())
									rev = it->second;
							}
							CLogDlg dlg;
							// fetch the log from HEAD, not the revision we updated to:
							// the path might be inside an external folder which has its own
							// revisions.
							CString sPath = GetPathFromColumnText(data->sPathColumnText);
							dlg.SetParams(CTGitPath(sPath), SVNRev(), SVNRev::REV_HEAD, 1, -1, TRUE);
							dlg.DoModal();
						}
						break;
					case ID_OPENWITH:
						bOpenWith = true;
					case ID_OPEN:
						{
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
					DialogEnableWindow(IDOK, TRUE);
					theApp.DoWaitCursor(-1);
				} // if (bAdded)
			}
		}
	}
#endif
}

void CGitProgressDlg::OnEnSetfocusInfotext()
{
	CString sTemp;
	GetDlgItemText(IDC_INFOTEXT, sTemp);
	if (sTemp.IsEmpty())
		GetDlgItem(IDC_INFOTEXT)->HideCaret();
}

void CGitProgressDlg::OnLvnBegindragSvnprogress(NMHDR* , LRESULT *pResult)
{
	//LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
#if 0
	int selIndex = m_ProgList.GetSelectionMark();
	if (selIndex < 0)
		return;

	CDropFiles dropFiles; // class for creating DROPFILES struct

	int index;
	POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
	while ( (index = m_ProgList.GetNextSelectedItem(pos)) >= 0 )
	{
		NotificationData * data = m_arData[index];

		if ( data->kind==svn_node_file || data->kind==svn_node_dir )
		{
			CString sPath = GetPathFromColumnText(data->sPathColumnText);

			dropFiles.AddFile( sPath );
		}
	}

	if ( dropFiles.GetCount()>0 )
	{
		dropFiles.CreateStructure();
	}
#endif
	*pResult = 0;
}

void CGitProgressDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableStandAloneDialog::OnSize(nType, cx, cy);
	if ((nType == SIZE_RESTORED)&&(m_bLastVisible))
	{
		if(!m_ProgList.m_hWnd)
			return;

		int count = m_ProgList.GetItemCount();
		if (count > 0)
			m_ProgList.EnsureVisible(count-1, false);
	}
}

//////////////////////////////////////////////////////////////////////////
/// commands
//////////////////////////////////////////////////////////////////////////
bool CGitProgressDlg::CmdAdd(CString& sWindowTitle, bool& localoperation)
{
	localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_ADD);
	CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
	SetBackgroundImage(IDI_ADD_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_ADD)));

	if (CRegDWORD(_T("Software\\TortoiseGit\\UseLibgit2"), FALSE) == TRUE)
	{
		git_repository *repo = NULL;
		git_index *index;

		CStringA gitdir = CUnicodeUtils::GetMulti(CTGitPath(g_Git.m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdir.GetBuffer()))
		{
			gitdir.ReleaseBuffer();
			ReportGitError();
			return false;
		}
		gitdir.ReleaseBuffer();

		git_config * config;
		git_config_new(&config);

		CString adminDir;
		g_GitAdminDir.GetAdminDirPath(g_Git.m_CurrentDir, adminDir);
		CStringA projectConfigA = CUnicodeUtils::GetMulti(adminDir + _T("config"), CP_UTF8);
		if (git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 3))
		{
			projectConfigA.ReleaseBuffer();
			ReportGitError();
			git_config_free(config);
			git_repository_free(repo);
			return false;
		}
		projectConfigA.ReleaseBuffer();
		CString globalConfig = g_Git.GetHomeDirectory() + _T("\\.gitconfig");
		if (PathFileExists(globalConfig))
		{
			CStringA globalConfigA = CUnicodeUtils::GetMulti(globalConfig, CP_UTF8);
			if (git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 2))
			{
				globalConfigA.ReleaseBuffer();
				ReportGitError();
				git_config_free(config);
				git_repository_free(repo);
				return false;
			}
			globalConfigA.ReleaseBuffer();
		}
		CString msysGitBinPath(CRegString(REG_MSYSGIT_PATH, _T(""), FALSE));
		if (!msysGitBinPath.IsEmpty())
		{
			CStringA systemConfigA = CUnicodeUtils::GetMulti(msysGitBinPath + _T("\\..\\etc\\gitconfig"), CP_UTF8);
			if (git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1))
			{
				systemConfigA.ReleaseBuffer();
				ReportGitError();
				git_config_free(config);
				git_repository_free(repo);
				return false;
			}
			systemConfigA.ReleaseBuffer();
		}

		git_repository_set_config(repo, config);

		if (git_repository_index(&index, repo))
		{
			ReportGitError();
			git_repository_free(repo);
			return false;
		}
		if (git_index_read(index))
		{
			ReportGitError();
			git_index_free(index);
			git_repository_free(repo);
			return false;
		}

		for(int i=0;i<m_targetPathList.GetCount();i++)
		{
			if (git_index_add(index, CStringA(CUnicodeUtils::GetMulti(m_targetPathList[i].GetGitPathString(), CP_UTF8)).GetBuffer(), 0))
			{
				ReportGitError();
				git_index_free(index);
				git_repository_free(repo);
				return false;
			}
			Notify(m_targetPathList[i],git_wc_notify_add);
		}

		if (git_index_write(index))
		{
			ReportGitError();
			git_index_free(index);
			git_repository_free(repo);
			return false;
		}

		git_index_free(index);
		git_repository_free(repo);
	}
	else
	{
		CMassiveGitTask mgt(L"add -f");
		mgt.ExecuteWithNotify(&m_targetPathList, m_bCancelled, git_wc_notify_add, this, &CGitProgressDlg::Notify);
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	this->GetDlgItem(IDC_LOGBUTTON)->SetWindowText(_T("Commit ..."));
	this->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
	return true;
}

bool CGitProgressDlg::CmdCopy(CString& /*sWindowTitle*/, bool& /*localoperation*/)
{
#if 0
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_COPY);
	SetWindowText(sWindowTitle); // needs to be updated, see TSVN rev. 21375
	SetBackgroundImage(IDI_COPY_BKG);

	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_COPY,
		m_targetPathList[0].IsUrl() ? (LPCTSTR)m_targetPathList[0].GetSVNPathString() : m_targetPathList[0].GetWinPath(),
		(LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
	ReportCmd(sCmdInfo);

	if (!Copy(m_targetPathList, m_url, m_Revision, m_pegRev, m_sMessage))
	{
		ReportSVNError();
		return false;
	}
	if (m_options & ProgOptSwitchAfterCopy)
	{
		sCmdInfo.Format(IDS_PROGRS_CMD_SWITCH,
			m_targetPathList[0].GetWinPath(),
			(LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
		ReportCmd(sCmdInfo);
		if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, SVNRev::REV_HEAD, m_depth, TRUE, m_options & ProgOptIgnoreExternals))
		{
			if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, m_Revision, m_depth, TRUE, m_options & ProgOptIgnoreExternals))
			{
				ReportSVNError();
				return false;
			}
		}
	}
	else
	{
		if (SVN::PathIsURL(m_url))
		{
			CString sMsg(MAKEINTRESOURCE(IDS_PROGRS_COPY_WARNING));
			ReportNotification(sMsg);
		}
	}
#endif
	return true;
}

bool CGitProgressDlg::CmdExport(CString& /*sWindowTitle*/, bool& /*localoperation*/)
{
#if 0
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_EXPORT);
	sWindowTitle = m_url.GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
	SetWindowText(sWindowTitle); // needs to be updated, see TSVN rev. 21375
	SetBackgroundImage(IDI_EXPORT_BKG);
	CString eol;
	if (m_options & ProgOptEolCRLF)
		eol = _T("CRLF");
	if (m_options & ProgOptEolLF)
		eol = _T("LF");
	if (m_options & ProgOptEolCR)
		eol = _T("CR");
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_EXPORT)));
	if (!Export(m_url, m_targetPathList[0], m_Revision, m_Revision, TRUE, m_options & ProgOptIgnoreExternals, m_depth, NULL, FALSE, eol))
	{
		ReportSVNError();
		return false;
	}
#endif
	return true;
}

bool CGitProgressDlg::CmdRename(CString& /*sWindowTitle*/, bool& /*localoperation*/)
{
#if 0
	ASSERT(m_targetPathList.GetCount() == 1);
	if ((!m_targetPathList[0].IsUrl())&&(!m_url.IsUrl()))
		localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_RENAME);
	SetWindowText(sWindowTitle); // needs to be updated, see TSVN rev. 21375
	SetBackgroundImage(IDI_RENAME_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RENAME)));
	if (!Move(m_targetPathList, m_url, m_Revision, m_sMessage))
	{
		ReportSVNError();
		return false;
	}
#endif
	return true;
}

bool CGitProgressDlg::CmdResolve(CString& sWindowTitle, bool& localoperation)
{

	localoperation = true;
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_RESOLVE);
	CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
	SetBackgroundImage(IDI_RESOLVE_BKG);
	// check if the file may still have conflict markers in it.
	//BOOL bMarkers = FALSE;

	for(int i=0;i<m_targetPathList.GetCount();i++)
	{
		CString cmd,out,tempmergefile;
		cmd.Format(_T("git.exe add -f -- \"%s\""),m_targetPathList[i].GetGitPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			m_bErrorsOccurred=true;
			return false;
		}

		CAppUtils::RemoveTempMergeFile((CTGitPath &)m_targetPathList[i]);

		Notify(m_targetPathList[i],git_wc_notify_resolved);
	}
#if 0
	if ((m_options & ProgOptSkipConflictCheck) == 0)
	{
		try
		{
			for (INT_PTR fileindex=0; (fileindex<m_targetPathList.GetCount()) && (bMarkers==FALSE); ++fileindex)
			{
				if (!m_targetPathList[fileindex].IsDirectory())
				{
					CStdioFile file(m_targetPathList[fileindex].GetWinPath(), CFile::typeBinary | CFile::modeRead);
					CString strLine = _T("");
					while (file.ReadString(strLine))
					{
						if (strLine.Find(_T("<<<<<<<"))==0)
						{
							bMarkers = TRUE;
							break;
						}
					}
					file.Close();
				}
			}
		}
		catch (CFileException* pE)
		{
			TRACE(_T("CFileException in Resolve!\n"));
			TCHAR error[10000] = {0};
			pE->GetErrorMessage(error, 10000);
			ReportError(error);
			pE->Delete();
			return false;
		}
	}
	if (bMarkers)
	{
		if (CMessageBox::Show(m_hWnd, IDS_PROGRS_REVERTMARKERS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
		{
			ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RESOLVE)));
			for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
				Resolve(m_targetPathList[fileindex], svn_wc_conflict_choose_merged, true);
		}
	}
	else
	{
		ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RESOLVE)));
		for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
			Resolve(m_targetPathList[fileindex], svn_wc_conflict_choose_merged, true);
	}
#endif
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	this->GetDlgItem(IDC_LOGBUTTON)->SetWindowText(_T("Commit ..."));
	this->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);

	return true;
}

bool CGitProgressDlg::CmdRevert(CString& sWindowTitle, bool& localoperation)
{

	localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_REVERT);
	CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
	SetBackgroundImage(IDI_REVERT_BKG);

	CTGitPathList delList;
	for(int i=0;i<m_selectedPaths.GetCount();i++)
	{
		CTGitPath path;
		int action;
		path.SetFromWin(g_Git.m_CurrentDir+_T("\\")+m_selectedPaths[i].GetWinPath());
		action = m_selectedPaths[i].m_Action;
		/* rename file can't delete because it needs original file*/
		if((!(action & CTGitPath::LOGACTIONS_ADDED)) &&
			(!(action & CTGitPath::LOGACTIONS_REPLACED)))
			delList.AddPath(path);
	}
	if (DWORD(CRegDWORD(_T("Software\\TortoiseGit\\RevertWithRecycleBin"), TRUE)))
		delList.DeleteAllFiles(true);

	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));
	for(int i=0;i<m_selectedPaths.GetCount();i++)
	{
		if(g_Git.Revert(_T("HEAD"), (CTGitPath&)m_selectedPaths[i]))
		{
			CMessageBox::Show(NULL,_T("Revert Fail"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			m_bErrorsOccurred=true;
			return false;
		}
		Notify(m_selectedPaths[i],git_wc_notify_revert);
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_selectedPaths);

	return true;
}

bool CGitProgressDlg::CmdSwitch(CString& /*sWindowTitle*/, bool& /*localoperation*/)
{
#if 0
	ASSERT(m_targetPathList.GetCount() == 1);
	SVNStatus st;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
	SetWindowText(sWindowTitle); // needs to be updated, see TSVN rev. 21375
	SetBackgroundImage(IDI_SWITCH_BKG);
	LONG rev = 0;
	if (st.GetStatus(m_targetPathList[0]) != (-2))
	{
		if (st.status->entry != NULL)
		{
			rev = st.status->entry->revision;
		}
	}

	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_SWITCH,
		m_targetPathList[0].GetWinPath(), (LPCTSTR)m_url.GetSVNPathString(),
		(LPCTSTR)m_Revision.ToString());
	ReportCmd(sCmdInfo);

	bool depthIsSticky = true;
	if (m_depth == svn_depth_unknown)
		depthIsSticky = false;
	if (!Switch(m_targetPathList[0], m_url, m_Revision, m_Revision, m_depth, depthIsSticky, m_options & ProgOptIgnoreExternals))
	{
		ReportSVNError();
		return false;
	}
	m_UpdateStartRevMap[m_targetPathList[0].GetSVNApiPath(pool)] = rev;
	if ((m_RevisionEnd >= 0)&&(rev >= 0)
		&&((LONG)m_RevisionEnd > (LONG)rev))
	{
		GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
	}
#endif
	return true;
}

void CGitProgressDlg::OnBnClickedNoninteractive()
{
	LRESULT res = ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_GETCHECK, 0, 0);
	m_AlwaysConflicted = (res == BST_CHECKED);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseGit\\MergeNonInteractive"), FALSE);
	nonint = m_AlwaysConflicted;
}

CString CGitProgressDlg::GetPathFromColumnText(const CString& sColumnText)
{
	CString sPath = CPathUtils::ParsePathInString(sColumnText);
	if (sPath.Find(':')<0)
	{
		// the path is not absolute: add the common root of all paths to it
		sPath = m_targetPathList.GetCommonRoot().GetDirectory().GetWinPathString() + _T("\\") + CPathUtils::ParsePathInString(sColumnText);
	}
	return sPath;
}

bool CGitProgressDlg::CmdSendMail(CString& sWindowTitle, bool& /*localoperation*/)
{
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_SENDMAIL);
	CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
	//SetBackgroundImage(IDI_ADD_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_SENDMAIL)));
	bool ret=true;
	if(this->m_SendMailFlags&SENDMAIL_COMBINED)
	{
		CString error;
		CTGitPath path;
		Notify(path,git_wc_notify_sendmail_start);
		CString err;
		int retry=0;
		while(retry <3)
		{
			if(!!CPatch::SendPatchesCombined(m_targetPathList,m_SendMailTO,m_SendMailCC,m_SendMailSubject,!!(this->m_SendMailFlags&SENDMAIL_ATTACHMENT),!!(this->m_SendMailFlags&SENDMAIL_MAPI),&err))
			{
				Notify(path,git_wc_notify_sendmail_error,ret,&err);
				ret = false;
			}
			else
			{
				break;
			}

			retry++;
			if (retry < 3)
				Notify(path,git_wc_notify_sendmail_retry,ret,&err);
			Sleep(2000);
			if(m_bCancelled)
			{
				CString str(_T("User Canceled"));
				Notify(path,git_wc_notify_sendmail_error,ret,&str);
				return false;
			}
		}
		if (ret)
			Notify(path,git_wc_notify_sendmail_done,ret);
	}
	else
	{
		for(int i=0;ret && i<m_targetPathList.GetCount();i++)
		{
			CPatch patch;
			Notify(m_targetPathList[i],git_wc_notify_sendmail_start);

			int retry=0;
			while(retry<3)
			{
				if(!!patch.Send((CString&)m_targetPathList[i].GetWinPathString(),this->m_SendMailTO,
								this->m_SendMailCC,!!(this->m_SendMailFlags&SENDMAIL_ATTACHMENT),!!(this->m_SendMailFlags&SENDMAIL_MAPI)))
				{
					Notify(m_targetPathList[i],git_wc_notify_sendmail_error,ret,&patch.m_LastError);
					ret = false;

				}
				else
				{
					ret = true;
					break;
				}
				retry++;
				if (retry < 3)
					Notify(m_targetPathList[i],git_wc_notify_sendmail_retry,ret,&patch.m_LastError);
				Sleep(2000);
				if(m_bCancelled)
				{
					CString str(_T("User Canceled"));
					Notify(m_targetPathList[i],git_wc_notify_sendmail_error,ret,&str);
					return false;
				}
			}
			if (ret)
				Notify(m_targetPathList[i],git_wc_notify_sendmail_done,ret);
		}
	}
	return ret;
}

LRESULT CGitProgressDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return 0;
}
