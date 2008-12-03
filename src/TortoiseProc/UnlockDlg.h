// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "SVNStatusListCtrl.h"

/**
 * \ingroup TortoiseProc
 * a simple dialog to show the user all locked
 * files below a specified folder.
 */
class CUnlockDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CUnlockDlg)

public:
	CUnlockDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUnlockDlg();

// Dialog Data
	enum { IDD = IDD_UNLOCK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();
	afx_msg LRESULT	OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
private:
	static UINT UnlockThreadEntry(LPVOID pVoid);
	UINT UnlockThread();

public:
	/** holds all the selected files/folders the user wants to unlock 
	 *  on exit */
	CTSVNPathList	m_pathList;

private:
	CSVNStatusListCtrl	m_unlockListCtrl;
	volatile LONG	m_bThreadRunning;
	CButton			m_SelectAll;
	bool			m_bCancelled;
};
