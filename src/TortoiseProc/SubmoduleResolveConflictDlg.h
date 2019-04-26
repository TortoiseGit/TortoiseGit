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
#include "GitDiff.h"

class CSubmoduleResolveConflictDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleResolveConflictDlg)

public:
	CSubmoduleResolveConflictDlg(CWnd* pParent = nullptr);
	virtual ~CSubmoduleResolveConflictDlg();

	enum { IDD = IDD_RESOLVESUBMODULECONFLICT };

	void SetDiff(const CString& path, bool revertTheirMy, const CString& baseTitle, const CString& mineTitle, const CString& theirsTitle, const CGitHash& baseHash, const CString& baseSubject, bool baseOK, const CGitHash& mineHash, const CString& mineSubject, bool mineOK, CGitDiff::ChangeType mineChangeType, const CGitHash& theirsHash, const CString& theirsSubject, bool theirsOK, CGitDiff::ChangeType theirsChangeType);

	bool m_bResolved;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void Resolve(const CString& path, bool useMine);

	afx_msg void OnBnClickedLog();
	afx_msg void OnBnClickedLog2();
	afx_msg void OnBnClickedLog3();
	void ShowLog(CString hash);
	afx_msg void OnBnClickedButtonUpdate2();
	afx_msg void OnBnClickedButtonUpdate3();

	DECLARE_MESSAGE_MAP()

	CString	m_sPath;

	CString m_sBaseTitle;
	CString m_sMineTitle;
	CString m_sTheirsTitle;
	CGitHash m_sBaseHash;
	CString	m_sBaseSubject;
	bool	m_bBaseOK;
	CGitHash m_sMineHash;
	CString	m_sMineSubject;
	bool	m_bMineOK;
	CGitHash m_sTheirsHash;
	CString	m_sTheirsSubject;
	bool	m_bTheirsOK;
	CGitDiff::ChangeType m_nChangeTypeMine;
	CGitDiff::ChangeType m_nChangeTypeTheirs;
	bool	m_bRevertTheirMy;
};
