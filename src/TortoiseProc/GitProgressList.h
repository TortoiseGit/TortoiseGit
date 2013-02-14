// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit
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

#pragma once
#include "TGitPath.h"
#include "ProjectProperties.h"
#include "Git.h"
#include "GitStatus.h"
#include "Colors.h"
//#include "..\IBugTraqProvider\IBugTraqProvider_h.h"
#include "afxwin.h"
#include "Win7.h"
#include "UnicodeUtils.h"
#include "resource.h"
/**
 * \ingroup TortoiseProc
 * Options which can be used to configure the way the dialog box works
 */
typedef enum
{
	ProgOptNone = 0,
	ProgOptRecursive = 0x01,
	ProgOptNonRecursive = 0x00,
	/// Don't actually do the merge - just practice it
	ProgOptDryRun = 0x04,
	ProgOptIgnoreExternals = 0x08,
	ProgOptKeeplocks = 0x10,
	/// for locking this means steal the lock, for unlocking it means breaking the lock
	ProgOptLockForce = 0x20,
	ProgOptSwitchAfterCopy = 0x40,
	ProgOptIncludeIgnored = 0x80,
	ProgOptIgnoreAncestry = 0x100,
	ProgOptEolDefault = 0x200,
	ProgOptEolCRLF = 0x400,
	ProgOptEolLF = 0x800,
	ProgOptEolCR = 0x1000,
	ProgOptSkipConflictCheck = 0x2000,
	ProgOptRecordOnly = 0x4000
} ProgressOptions;

typedef enum
{
	CLOSE_MANUAL = 0,
	CLOSE_NOERRORS,
	CLOSE_NOCONFLICTS,
	CLOSE_NOMERGES,
	CLOSE_LOCAL
} ProgressCloseOptions;

typedef enum
{
	git_wc_notify_add,
	git_wc_notify_sendmail_start,
	git_wc_notify_sendmail_error,
	git_wc_notify_sendmail_retry,
	git_wc_notify_sendmail_done,
	git_wc_notify_resolved,
	git_wc_notify_revert,
	git_wc_notify_fetch,
	git_wc_notify_checkout,
	git_wc_notify_update_ref,

}git_wc_notify_action_t;

typedef enum
{
	SENDMAIL_ATTACHMENT	=0x1,
	SENDMAIL_COMBINED	=0x2,
	SENDMAIL_MAPI		=0x4
};
// CGitProgressList
struct git_transfer_progress;
#define WM_SHOWCONFLICTRESOLVER (WM_APP + 100)
#define WM_PROG_CMD_FINISH		(WM_APP + 200)
#define WM_PROG_CMD_START		(WM_APP + 201)

class CGitProgressList : public CListCtrl
{
	DECLARE_DYNAMIC(CGitProgressList)

public:
	typedef enum
	{
		GitProgress_Add,
		GitProgress_Checkout,
		GitProgress_Copy,
		GitProgress_Export,
		GitProgress_Rename,
		GitProgress_Resolve,
		GitProgress_Revert,
		GitProgress_Switch,
		GitProgress_SendMail,
		GitProgress_Clone,
		GitProgress_Fetch,
	} Command;

	CGitProgressList();
	virtual ~CGitProgressList();

	void SetCommand(CGitProgressList::Command cmd) {m_Command = cmd;}
	void SetOptions(DWORD opts) {m_options = opts;}
	void SetPathList(const CTGitPathList& pathList) {m_targetPathList = pathList;}
	void SetUrl(const CString& url) {m_url.SetFromUnknown(url);}
	void SetSecondUrl(const CString& url) {m_url2.SetFromUnknown(url);}
	void SetCommitMessage(const CString& msg) {m_sMessage = msg;}
	void SetIsBare(bool b) { m_bBare = b; }
	void SetNoCheckout(bool b){ m_bNoCheckout = b; }
	void SetRefSpec(CString spec){ m_RefSpec = spec; }
	void SetAutoTag(int tag){ m_AutoTag = tag; }

//	void SetRevision(const GitRev& rev) {m_Revision = rev;}
//	void SetRevisionEnd(const GitRev& rev) {m_RevisionEnd = rev;}

	void SetDiffOptions(const CString& opts) {m_diffoptions = opts;}
	void SetSendMailOption(CString &TO, CString &CC,CString &Subject,DWORD flags){m_SendMailTO=TO;m_SendMailSubject=Subject; m_SendMailCC=CC;this->m_SendMailFlags = flags;}
	void SetDepth(git_depth_t depth = git_depth_unknown) {m_depth = depth;}
	void SetPegRevision(GitRev pegrev = GitRev()) {m_pegRev = pegrev;}
	void SetProjectProperties(ProjectProperties props) {m_ProjectProperties = props;}
	void SetChangeList(const CString& changelist, bool keepchangelist) {m_changelist = changelist; m_keepchangelist = keepchangelist;}
	void SetSelectedList(const CTGitPathList& selPaths);
//	void SetRevisionRanges(const GitRevRangeArray& revArray) {m_revisionArray = revArray;}
//	void SetBugTraqProvider(const CComPtr<IBugTraqProvider> pBugtraqProvider) { m_BugTraqProvider = pBugtraqProvider;}
	/**
	 * If the number of items for which the operation is done on is known
	 * beforehand, that number can be set here. It is then used to show a more
	 * accurate progress bar during the operation.
	 */
	void SetItemCount(long count) {if(count) m_itemCountTotal = count;}
	bool SetBackgroundImage(UINT nID);
	bool DidErrorsOccur() {return m_bErrorsOccurred;}
	bool			m_bErrorsOccurred;
	CWnd			*m_pProgressLabelCtrl;
	CWnd			*m_pInfoCtrl;
	CAnimateCtrl	*m_pAnimate;
	CProgressCtrl	*m_pProgControl;
	Command			m_Command;
	virtual BOOL	Cancel();
	volatile BOOL IsCancelled()	{return m_bCancelled;}
	volatile LONG IsRunning()	{return m_bThreadRunning;}
	CWinThread*				m_pThread;
	bool					m_AlwaysConflicted;
private:
	class NotificationData
	{
	public:
		NotificationData()
		: color(::GetSysColor(COLOR_WINDOWTEXT))
		, action((git_wc_notify_action_t)-1)
		, bConflictedActionItem(false)
		, bAuxItem(false)
		{};
	    git_wc_notify_action_t action;
#if 0
		  action((git_wc_notify_action_t)-1),
			  kind(git_node_none),
			  content_state(git_wc_notify_state_inapplicable),
			  prop_state(git_wc_notify_state_inapplicable),
			  rev(0),

			  bConflictedActionItem(false),
			  bAuxItem(false)
			  //,
//			  lock_state(git_wc_notify_lock_state_unchanged)
		  {
//			  merge_range.end = 0;
//			  merge_range.start = 0;
		  }
#endif
	public:
		// The text we put into the first column (the Git action for normal items, just text for aux items)
		CString					sActionColumnText;
		CTGitPath				path;
		CTGitPath				basepath;
//		CString					changelistname;

//		git_node_kind_t			kind;
//		CString					mime_type;
//		git_wc_notify_state_t	content_state;
//		git_wc_notify_state_t	prop_state;
//		git_wc_notify_lock_state_t lock_state;
//		git_merge_range_t		merge_range;
		git_revnum_t			rev;
		COLORREF				color;
//		CString					owner;						///< lock owner
		bool					bConflictedActionItem;		// Is this item a conflict?
		bool					bAuxItem;					// Set if this item is not a true 'Git action'
		CString					sPathColumnText;
		CGitHash				m_OldHash;
		CGitHash				m_NewHash;

	};
protected:
	DECLARE_MESSAGE_MAP()

	//Need update in the future implement the virtual methods from Git base class
	virtual BOOL Notify(const CTGitPath& path,
								git_wc_notify_action_t action,
								int status = 0,
								CString *strErr =NULL
		/*
		git_node_kind_t kind, const CString& mime_type,
		git_wc_notify_state_t content_state,
		git_wc_notify_state_t prop_state, LONG rev,
		const git_lock_t * lock, git_wc_notify_lock_state_t lock_state,
		const CString& changelistname,
		git_merge_range_t * range,
		git_error_t * err, apr_pool_t * pool*/
		);
	virtual BOOL Notify(const git_wc_notify_action_t action, const git_transfer_progress *stat);
	virtual BOOL Notify(const git_wc_notify_action_t action, CString str, const git_oid *a, const git_oid *b);

//	virtual git_wc_conflict_choice_t	ConflictResolveCallback(const git_wc_conflict_description_t *description, CString& mergedfile);

	static int FetchCallback(const git_transfer_progress *stats, void *payload)
	{
		return !((CGitProgressList*)payload) -> Notify(git_wc_notify_fetch, stats);
	}

	static void CheckoutCallback(const char *path, size_t cur, size_t tot, void *payload)
	{
		CTGitPath tpath = CUnicodeUtils::GetUnicode(CStringA(path), CP_UTF8);
		((CGitProgressList*)payload) -> m_itemCountTotal = tot;
		((CGitProgressList*)payload) -> m_itemCount = cur;
		((CGitProgressList*)payload) -> Notify(tpath, git_wc_notify_checkout);
	}

	static void RemoteProgressCallback(const char *str, int len, void *data)
	{
		CString progText;
		progText = CUnicodeUtils::GetUnicode(CStringA(str, len));
		((CGitProgressList*)data) -> SetDlgItemText(IDC_PROGRESSLABEL, progText);
	}
	static int RemoteCompletionCallback(git_remote_completion_type /*type*/, void * /*data*/)
	{
		return 0;
	}
	static int RemoteUpdatetipsCallback(const char *refname, const git_oid *a, const git_oid *b, void *data)
	{
		CString str;
		str = CUnicodeUtils::GetUnicode(refname);
		((CGitProgressList*)data) -> Notify(git_wc_notify_update_ref, str, a, b);
		return 0;
	}

	afx_msg void	OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnGitProgress(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	LRESULT			OnShowConflictResolver(WPARAM, LPARAM);
	afx_msg void	OnLvnBegindragSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);

	void			Sort();
	static bool		SortCompare(const NotificationData* pElem1, const NotificationData* pElem2);

	static BOOL		m_bAscending;
	static int		m_nSortedColumn;
	CStringList		m_ExtStack;

private:
	static UINT ProgressThreadEntry(LPVOID pVoid);
	UINT		ProgressThread();

	void		ReportGitError();
	void		ReportError(const CString& sError);
	void		ReportWarning(const CString& sWarning);
	void		ReportNotification(const CString& sNotification);
	void		ReportCmd(const CString& sCmd);
	void		ReportString(CString sMessage, const CString& sMsgKind, COLORREF color = ::GetSysColor(COLOR_WINDOWTEXT));
	void		AddItemToList();
	CString		BuildInfoString();
	CString		GetPathFromColumnText(const CString& sColumnText);

		/**
	 * Resizes the columns of the progress list so that the headings are visible.
	 */
	void		ResizeColumns();

	/// Predicate function to tell us if a notification data item is auxiliary or not
	static bool NotificationDataIsAux(const NotificationData* pData);

	// the commands to execute
	bool		CmdAdd(CString& sWindowTitle, bool& localoperation);
	bool		CmdCheckout(CString& sWindowTitle, bool& localoperation);
	bool		CmdCopy(CString& sWindowTitle, bool& localoperation);
	bool		CmdExport(CString& sWindowTitle, bool& localoperation);
	bool		CmdRename(CString& sWindowTitle, bool& localoperation);
	bool		CmdResolve(CString& sWindowTitle, bool& localoperation);
	bool		CmdRevert(CString& sWindowTitle, bool& localoperation);
	bool		CmdSwitch(CString& sWindowTitle, bool& localoperation);
	bool		CmdSendMail(CString& sWindowTitle, bool& localoperation);
	bool		CmdClone(CString& sWindowTitle, bool& localoperation);
	bool		CmdFetch(CString& sWindowTitle, bool& localoperation);

private:
	typedef std::map<CStringA, git_revnum_t> StringRevMap;
	typedef std::vector<NotificationData *> NotificationDataVect;

	CString					m_mergedfile;
	NotificationDataVect	m_arData;

	volatile LONG			m_bThreadRunning;

	ProjectProperties		m_ProjectProperties;

	int						m_options;	// Use values from the ProgressOptions enum
	git_depth_t				m_depth;
	CTGitPathList			m_targetPathList;
	CTGitPathList			m_selectedPaths;
	CTGitPath				m_url;
	CTGitPath				m_url2;
	CString					m_sMessage;
	CString					m_diffoptions;
	GitRev					m_Revision;
	GitRev					m_RevisionEnd;
	GitRev					m_pegRev;
//	GitRevRangeArray		m_revisionArray;
	CString					m_changelist;
	bool					m_keepchangelist;

		CTGitPath				m_basePath;
	StringRevMap			m_UpdateStartRevMap;
	StringRevMap			m_FinishedRevMap;

	TCHAR					m_columnbuf[MAX_PATH];

	volatile BOOL			m_bCancelled;
	int						m_nConflicts;
	bool					m_bMergesAddsDeletesOccurred;

	int						iFirstResized;
	BOOL					bSecondResized;
	int						nEnsureVisibleCount;

	CString					m_sTotalBytesTransferred;
	size_t					m_TotalBytesTransferred;

	CColors					m_Colors;

	bool					m_bFinishedItemAdded;
	bool					m_bLastVisible;

	int						m_itemCount;
	int						m_itemCountTotal;

	DWORD					m_SendMailFlags;
	CString					m_SendMailTO;
	CString					m_SendMailCC;
	CString					m_SendMailSubject;

///	CComPtr<IBugTraqProvider> m_BugTraqProvider;
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	// some strings different methods can use
	CString					sDryRun;
	CString					sRecordOnly;

	bool					m_bBare;
	bool					m_bNoCheckout;
	CString					m_RefSpec;
	int						m_AutoTag;
public:
	void Init();
	afx_msg void OnClose();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


