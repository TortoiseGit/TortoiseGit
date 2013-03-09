// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
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
#pragma once

#include "StandAloneDlg.h"
#include "TGitPath.h"
#include "ProjectProperties.h"
#include "Git.h"
#include "GitStatus.h"
#include "Colors.h"
//#include "..\IBugTraqProvider\IBugTraqProvider_h.h"
#include "afxwin.h"
#include "Win7.h"
#include "UnicodeUtils.h"
#include "GitProgressList.h"

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * Handles different Subversion commands and shows the notify messages
 * in a listbox. Since several Subversion commands have similar notify
 * messages they are grouped together in this single class.
 */
class CGitProgressDlg : public CResizableStandAloneDialog
{
public:

	DECLARE_DYNAMIC(CGitProgressDlg)

public:

	CGitProgressDlg(CWnd* pParent = NULL);
	virtual ~CGitProgressDlg();


	void SetCommand(CGitProgressList::Command cmd) {m_ProgList.SetCommand(cmd);}
	void SetAutoClose(DWORD ac) {m_dwCloseOnEnd = ac;}
	void SetOptions(DWORD opts) {m_ProgList.SetOptions(opts);}
	void SetPathList(const CTGitPathList& pathList) {m_ProgList.SetPathList(pathList);}
	void SetUrl(const CString& url) {m_ProgList.SetUrl(url);}
	void SetSecondUrl(const CString& url) {m_ProgList.SetSecondUrl(url);}
	void SetCommitMessage(const CString& msg) {m_ProgList.SetCommitMessage(msg);}
	void SetIsBare(bool b) { m_ProgList.SetIsBare(b); }
	void SetNoCheckout(bool b){ m_ProgList.SetNoCheckout(b); }
	void SetRefSpec(CString spec){ m_ProgList.SetRefSpec(spec); }
	void SetAutoTag(int tag){ m_ProgList.SetAutoTag(tag); }

//	void SetRevision(const GitRev& rev) {m_Revision = rev;}
//	void SetRevisionEnd(const GitRev& rev) {m_RevisionEnd = rev;}

	void SetDiffOptions(const CString& opts) {m_ProgList.SetDiffOptions(opts);}
	void SetSendMailOption(CSendMail *sendmail) { m_ProgList.SetSendMailOption(sendmail); }
	void SetDepth(git_depth_t depth = git_depth_unknown) {m_ProgList.SetDepth(depth);}
	void SetPegRevision(GitRev pegrev = GitRev()) {m_ProgList.SetPegRevision(pegrev);}
	void SetProjectProperties(ProjectProperties props) {m_ProgList.SetProjectProperties(props);}
	void SetChangeList(const CString& changelist, bool keepchangelist) {m_ProgList.SetChangeList(changelist, keepchangelist);}
	void SetSelectedList(const CTGitPathList& selPaths) {m_ProgList.SetSelectedList(selPaths);};
//	void SetRevisionRanges(const GitRevRangeArray& revArray) {m_revisionArray = revArray;}
//	void SetBugTraqProvider(const CComPtr<IBugTraqProvider> pBugtraqProvider) { m_BugTraqProvider = pBugtraqProvider;}
	/**
	 * If the number of items for which the operation is done on is known
	 * beforehand, that number can be set here. It is then used to show a more
	 * accurate progress bar during the operation.
	 */
	void SetItemCount(long count) {if(count) m_ProgList.SetItemCount(count);}

	bool DidErrorsOccur() {return m_ProgList.m_bErrorsOccurred;}

	enum { IDD = IDD_SVNPROGRESS };


protected:

	virtual BOOL						OnInitDialog();
	virtual void						OnCancel();
	virtual BOOL						PreTranslateMessage(MSG* pMsg);
	virtual void						DoDataExchange(CDataExchange* pDX);

	afx_msg void	OnBnClickedLogbutton();
	afx_msg void	OnBnClickedOk();
	afx_msg void	OnBnClickedNoninteractive();
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnClose();
	afx_msg void	OnEnSetfocusInfotext();
	afx_msg LRESULT	OnCtlColorStatic(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH	OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	LRESULT			OnCmdEnd(WPARAM wParam, LPARAM lParam);
	LRESULT			OnCmdStart(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	virtual void OnOK();

	CAnimateCtrl			m_Animate;
	CProgressCtrl			m_ProgCtrl;
	CGitProgressList		m_ProgList;
	CEdit					m_InfoCtrl;
	CStatic					m_ProgLableCtrl;

	CBrush					m_background_brush;
	DWORD					m_dwCloseOnEnd;
};
