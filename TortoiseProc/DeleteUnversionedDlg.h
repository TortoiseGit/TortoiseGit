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
* Dialog showing a list of unversioned and ignored files.
*/
class CDeleteUnversionedDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CDeleteUnversionedDlg)

public:
	CDeleteUnversionedDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDeleteUnversionedDlg();

	enum { IDD = IDD_DELUNVERSIONED };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedSelectall();
	afx_msg LRESULT	OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	void StartDiff(int fileindex);

private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT		StatusThread();

public:
	CTSVNPathList 		m_pathList;

private:
	BOOL				m_bSelectAll;
	CString				m_sWindowTitle;
	volatile LONG		m_bThreadRunning;
	CSVNStatusListCtrl	m_StatusList;
	CButton				m_SelectAll;
	bool				m_bCancelled;
};

