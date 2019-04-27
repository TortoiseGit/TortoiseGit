// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2014, 2017, 2019 - TortoiseGit

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
#include "HorizontalResizableStandAloneDialog.h"
#include "resource.h"
#include "MenuButton.h"
#include "GitDiff.h"

class CSubmoduleDiffDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleDiffDlg)

public:
	CSubmoduleDiffDlg(CWnd* pParent = nullptr);
	virtual ~CSubmoduleDiffDlg();

	enum { IDD = IDD_DIFFSUBMODULE };

	void SetDiff(CString path, bool toIsWorkingCopy, const CGitHash& fromHash, CString fromSubject, bool fromOK, const CGitHash& toHash, CString toSubject, bool toOK, bool dirty, CGitDiff::ChangeType changeType);
	bool IsRefresh() { return m_bRefresh; }

	static HBRUSH GetInvalidBrush(CDC* pDC);
	static HBRUSH GetChangeTypeBrush(CDC* pDC, const CGitDiff::ChangeType& changeType);

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	afx_msg void OnBnClickedLog();
	afx_msg void OnBnClickedLog2();
	afx_msg void OnBnClickedShowDiff();
	afx_msg void OnBnClickedButtonUpdate();
	void ShowLog(CString hash);
	CMenuButton	m_ctrlShowDiffBtn;

	DECLARE_MESSAGE_MAP()

	bool	m_bToIsWorkingCopy;
	CString	m_sPath;

	CGitHash m_sFromHash;
	CString	m_sFromSubject;
	bool	m_bFromOK;
	CGitHash m_sToHash;
	CString	m_sToSubject;
	bool	m_bToOK;
	bool	m_bDirty;
	CGitDiff::ChangeType m_nChangeType;
	bool	m_bRefresh;
};
