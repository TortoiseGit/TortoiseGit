// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
//#include "svn_wc.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog used in merge callbacks to resolve conflicts.
 */
class CConflictResolveDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CConflictResolveDlg)

public:
	CConflictResolveDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConflictResolveDlg();

	void SetConflictDescription(const svn_wc_conflict_description_t * description) {m_pConflictDescription = description;}
	svn_wc_conflict_choice_t GetResult() {return m_choice;}
	const CString& GetMergedFile() {return m_mergedfile;}
	bool IsCancelled() const {return m_bCancelled;}
	enum { IDD = IDD_CONFLICTRESOLVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedUselocal();
	afx_msg void OnBnClickedUserepo();
	afx_msg void OnBnClickedEditconflict();
	afx_msg void OnBnClickedResolved();
	afx_msg void OnBnClickedResolvealllater();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedAbort();

	DECLARE_MESSAGE_MAP()

private:
	const svn_wc_conflict_description_t *	m_pConflictDescription;
	svn_wc_conflict_choice_t				m_choice;
	CString									m_mergedfile;
	bool									m_bCancelled;
};
