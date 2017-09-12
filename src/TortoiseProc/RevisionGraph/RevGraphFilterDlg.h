// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng
// Copyright (C) 2012, 2014-2016 - TortoiseGit

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
#include "acedit.h"

/**
 * \ingroup TortoiseProc
 * helper dialog to enter filter data for the revision graph.
 */
class CRevGraphFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CRevGraphFilterDlg)

public:
	CRevGraphFilterDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRevGraphFilterDlg();

	void	GetRevisionRange (CString& minrev, CString& maxrev);
	void	SetRevisionRange (CString minrev, CString maxrev);

// Dialog Data
	enum { IDD = IDD_REVGRAPHFILTER };

	BOOL m_bCurrentBranch;
	BOOL m_bLocalBranches;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;	// DDX/DDV support
	afx_msg void OnBnClickedResetfilter();
	afx_msg void OnBnClickedCurrentBranch();
	afx_msg void OnBnClickedLocalBranches();
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;

	DECLARE_MESSAGE_MAP()
	CString	m_sFromRev;
	CString	m_sToRev;
	afx_msg void OnBnClickedRev1btn1();
	afx_msg void OnBnClickedRev1btn2();
	CACEdit m_ctrlFromRev;
	CACEdit m_ctrlToRev;
};
