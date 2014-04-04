// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - TortoiseGit

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

#include "HistoryCombo.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "LoglistCommonResource.h"
#include "registry.h"

// CFindDlg dialog

#define IDT_FILTER		101

class CFindDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();
	void Create(CWnd * pParent = NULL) {m_pParent = pParent; CDialog::Create(IDD, pParent);ShowWindow(SW_SHOW);UpdateWindow();}

	bool IsTerminating() {return m_bTerminating;}
	bool FindNext() {return m_bFindNext;}
	bool MatchCase() {return !!m_bMatchCase;}
	bool WholeWord() {return !!m_bWholeWord;}
	bool Regex() {return !!m_bRegex;}
	bool IsRef()	{return !!m_bIsRef;}
	CString GetFindString() {return m_FindString;}
	void SetFindString(const CString& str) { if (!str.IsEmpty()) { m_FindCombo.SetWindowText(str); } }

// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnEditchangeFindcombo();

	DECLARE_MESSAGE_MAP()

	UINT			m_FindMsg;
	bool			m_bTerminating;
	bool			m_bFindNext;
	BOOL			m_bMatchCase;
	BOOL			m_bLimitToDiffs;
	BOOL			m_bWholeWord;
	BOOL			m_bRegex;
	bool			m_bIsRef;
	CHistoryCombo	m_FindCombo;
	CString			m_FindString;
	CWnd			*m_pParent;
	STRING_VECTOR	m_RefList;
	CRegDWORD		m_regMatchCase;
	CRegDWORD		m_regWholeWord;
	CRegDWORD		m_regRegex;

	void AddToList();

public:
	CListCtrl m_ctrlRefList;
	CEdit m_ctrlFilter;
	afx_msg void OnNMClickListRef(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEditFilter();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
