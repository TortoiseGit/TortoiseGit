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

#pragma once
#include "TGitPath.h"
#include "Git.h"
#include "Colors.h"
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
	/// Don't actually do the merge - just practice it
	ProgOptDryRun = 0x04,
} ProgressOptions;

typedef enum
{
	git_wc_notify_add,
	git_wc_notify_sendmail,
	git_wc_notify_resolved,
	git_wc_notify_revert,
	git_wc_notify_fetch,
	git_wc_notify_checkout,
	git_wc_notify_update_ref,

}git_wc_notify_action_t;

// CGitProgressList
struct git_transfer_progress;
#define WM_SHOWCONFLICTRESOLVER (WM_APP + 100)
#define WM_PROG_CMD_FINISH		(WM_APP + 200)
#define WM_PROG_CMD_START		(WM_APP + 201)

class CSendMail;

class CGitProgressList : public CListCtrl
{
	DECLARE_DYNAMIC(CGitProgressList)

public:
	typedef enum
	{
		GitProgress_none,
		GitProgress_Add,
		GitProgress_Checkout,
		GitProgress_Resolve,
		GitProgress_Revert,
		GitProgress_SendMail,
		GitProgress_Clone,
		GitProgress_Fetch,
		GitProgress_Reset,
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
	void SetRemote(const CString& remote) { m_remote = remote; }
	void SetAutoTag(int tag){ m_AutoTag = tag; }
	void SetRevision(CString revision){ m_revision = revision; }
	void SetResetType(int resetType){ m_resetType = resetType; }

	void SetSendMailOption(CSendMail *sendmail) { m_SendMail = sendmail; }
	void SetSelectedList(const CTGitPathList& selPaths);
	/**
	 * If the number of items for which the operation is done on is known
	 * beforehand, that number can be set here. It is then used to show a more
	 * accurate progress bar during the operation.
	 */
	void SetItemCountTotal(long count) { if(count) m_itemCountTotal = count; }
	void SetItemProgress(long count) { m_itemCount = count;} // do not use SetItemCount here as this overrides the ListBox method
	bool SetBackgroundImage(UINT nID);
	bool DidErrorsOccur() {return m_bErrorsOccurred;}
	bool			m_bErrorsOccurred;
	CWnd			*m_pProgressLabelCtrl;
	CWnd			*m_pInfoCtrl;
	CAnimateCtrl	*m_pAnimate;
	CProgressCtrl	*m_pProgControl;
	Command			m_Command;
	void			Cancel();
	volatile BOOL IsCancelled()	{return m_bCancelled;}
	volatile LONG IsRunning()	{return m_bThreadRunning;}
	CWinThread*				m_pThread;
	CWnd			*m_pPostWnd;
	bool					m_bSetTitle;
private:
	class NotificationData
	{
	public:
		NotificationData()
		: color(::GetSysColor(COLOR_WINDOWTEXT))
		, action((git_wc_notify_action_t)-1)
		, bAuxItem(false)
		{};
		git_wc_notify_action_t action;
	public:
		// The text we put into the first column (the Git action for normal items, just text for aux items)
		CString					sActionColumnText;
		CTGitPath				path;
		CTGitPath				basepath;
		git_revnum_t			rev;
		COLORREF				color;
		bool					bAuxItem;					// Set if this item is not a true 'Git action'
		CString					sPathColumnText;
		CGitHash				m_OldHash;
		CGitHash				m_NewHash;
	};
protected:
	DECLARE_MESSAGE_MAP()

public:
	//Need update in the future implement the virtual methods from Git base class
	virtual BOOL Notify(const CTGitPath& path, git_wc_notify_action_t action);
protected:
	virtual BOOL Notify(const git_wc_notify_action_t action, const git_transfer_progress *stat);
	virtual BOOL Notify(const git_wc_notify_action_t action, CString str, const git_oid *a, const git_oid *b);

	void SetWindowTitle(UINT id, const CString& urlorpath, CString& dialogname);

	static int FetchCallback(const git_transfer_progress *stats, void *payload)
	{
		return !((CGitProgressList*)payload) -> Notify(git_wc_notify_fetch, stats);
	}

	static void CheckoutCallback(const char *path, size_t cur, size_t tot, void *payload)
	{
		CTGitPath tpath = CUnicodeUtils::GetUnicode(CStringA(path), CP_UTF8);
		((CGitProgressList*)payload) -> m_itemCountTotal = (int)tot;
		((CGitProgressList*)payload) -> m_itemCount = (int)cur;
		((CGitProgressList*)payload) -> Notify(tpath, git_wc_notify_checkout);
	}

	static int RemoteProgressCallback(const char *str, int len, void *data)
	{
		CString progText;
		progText = CUnicodeUtils::GetUnicode(CStringA(str, len));
		((CGitProgressList*)data) -> SetDlgItemText(IDC_PROGRESSLABEL, progText);
		return 0;
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
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	LRESULT			OnShowConflictResolver(WPARAM, LPARAM);
	afx_msg void	OnLvnBegindragSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);

	void			Sort();
	static bool		SortCompare(const NotificationData* pElem1, const NotificationData* pElem2);

	static BOOL		m_bAscending;
	static int		m_nSortedColumn;

private:
	static UINT ProgressThreadEntry(LPVOID pVoid);
	UINT		ProgressThread();

public:
	void		ReportGitError();
	void		ReportUserCanceled();
	void		ReportError(const CString& sError);
	void		ReportWarning(const CString& sWarning);
	void		ReportNotification(const CString& sNotification);
	void		ReportCmd(const CString& sCmd);
	void		ReportString(CString sMessage, const CString& sMsgKind, COLORREF color = ::GetSysColor(COLOR_WINDOWTEXT));

private:
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
	bool		CmdResolve(CString& sWindowTitle, bool& localoperation);
	bool		CmdRevert(CString& sWindowTitle, bool& localoperation);
	bool		CmdSendMail(CString& sWindowTitle, bool& localoperation);
	bool		CmdClone(CString& sWindowTitle, bool& localoperation);
	bool		CmdFetch(CString& sWindowTitle, bool& localoperation);
	bool		CmdReset(CString& sWindowTitle, bool& localoperation);

private:
	typedef std::map<CStringA, git_revnum_t> StringRevMap;
	typedef std::vector<NotificationData *> NotificationDataVect;

	NotificationDataVect	m_arData;

	volatile LONG			m_bThreadRunning;

	int						m_options;	// Use values from the ProgressOptions enum
	CTGitPathList			m_targetPathList;
	CTGitPathList			m_selectedPaths;
	CTGitPath				m_url;
	CTGitPath				m_url2;
	CString					m_sMessage;
	GitRev					m_Revision;
	GitRev					m_RevisionEnd;
	GitRev					m_pegRev;
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

	CSendMail *				m_SendMail;

	// some strings different methods can use
	CString					sDryRun;
	CString					sRecordOnly;

	bool					m_bBare;
	bool					m_bNoCheckout;
	CString					m_RefSpec;
	CString					m_remote;
	int						m_AutoTag;
	CString					m_revision;
	int						m_resetType;

public:
	CComPtr<ITaskbarList3>	m_pTaskbarList;
	void Init();
	afx_msg void OnClose();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
