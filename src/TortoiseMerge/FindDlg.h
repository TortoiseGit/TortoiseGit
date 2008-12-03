// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#include "HistoryCombo.h"

/**
 * \ingroup TortoiseMerge
 * Find dialog used in TortoiseMerge.
 */
class CFindDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();
	void Create(CWnd * pParent = NULL) {CDialog::Create(IDD, pParent);ShowWindow(SW_SHOW);UpdateWindow();}
	bool IsTerminating() {return m_bTerminating;}
	bool FindNext() {return m_bFindNext;}
	bool MatchCase() {return !!m_bMatchCase;}
	bool LimitToDiffs() {return !!m_bLimitToDiffs;}
	bool WholeWord() {return !!m_bWholeWord;}
	CString GetFindString() {return m_FindCombo.GetString();}
// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnEditchangeFindcombo();
private:
	UINT			m_FindMsg;
	bool			m_bTerminating;
	bool			m_bFindNext;
	BOOL			m_bMatchCase;
	BOOL			m_bLimitToDiffs;
	BOOL			m_bWholeWord;
	CHistoryCombo	m_FindCombo;
};
