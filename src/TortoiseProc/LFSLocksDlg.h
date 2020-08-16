// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2020 - TortoiseGit

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
#include "GitStatusListCtrl.h"

/**
* \ingroup TortoiseProc
* Dialog showing a list of versioned files which don't have the status 'normal'.
* The dialog effectively shows a list of files which can be reverted.
*/
class CLFSLocksDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLFSLocksDlg)

public:
	CLFSLocksDlg(CWnd* pParent = nullptr); // standard constructor
	virtual ~CLFSLocksDlg();

	enum { IDD = IDD_LFS_LOCKS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedUnLock();
	afx_msg LRESULT OnStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
	static UINT	LocksThreadEntry(LPVOID pVoid);
	UINT		LocksThread();
	void		Refresh();

public:
	CTGitPathList		m_pathList;

private:
	volatile LONG		m_bThreadRunning;
	CGitStatusListCtrl	m_LocksList;
	CButton				m_SelectAll;
	CButton				m_UnLock;
	CButton				m_Refresh;
	bool				m_bCancelled;
};
