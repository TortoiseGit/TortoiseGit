// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng

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
#include "afxcmn.h"
#include "sciedit.h"
#include "StandAloneDlg.h"
#include "SVNStatusListCtrl.h"
#include "ProjectProperties.h"
#include "RegHistory.h"
#include "SciEdit.h"
#include "Registry.h"
#include "Tooltip.h"

/**
 * \ingroup TortoiseProc
 * Dialog asking the user for a lock-message and a list control
 * where the user can select which files to lock.
 */
class CLockDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLockDlg)

public:
	CLockDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLockDlg();

	void	SetProjectProperties(ProjectProperties * pProps) {m_ProjectProperties = pProps;}
private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeLockmessage();
	afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHistory();
	afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void Refresh();

	DECLARE_MESSAGE_MAP()

	enum { IDD = IDD_LOCK };
public:
	CString				m_sLockMessage;
	BOOL				m_bStealLocks;
	CTSVNPathList		m_pathList;

private:
	CWinThread*			m_pThread;
	BOOL				m_bBlock;
	CSVNStatusListCtrl	m_cFileList;
	CSciEdit			m_cEdit;
	ProjectProperties *	m_ProjectProperties;
	bool				m_bCancelled;
	CToolTips			m_tooltips;
	CButton				m_SelectAll;
	CRegHistory			m_History;
};
