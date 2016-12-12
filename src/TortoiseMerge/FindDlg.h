// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010, 2013-2014, 2016 - TortoiseSVN

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
#include "resource.h"
#include <afxcmn.h>
#include "HistoryCombo.h"
#include "registry.h"

#define REPLACEMSGSTRINGW  L"TortoiseGitMerge_FindReplace"

/**
 * \ingroup TortoiseMerge
 * Find dialog used in TortoiseMerge.
 */
class CFindDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFindDlg();
	void Create(CWnd* pParent = nullptr, int id = 0);
	bool IsTerminating() {return m_bTerminating;}
	bool FindNext() {return m_bFindNext;}
	bool MatchCase() {return !!m_bMatchCase;}
	bool LimitToDiffs() {return !!m_bLimitToDiffs;}
	bool WholeWord() {return !!m_bWholeWord;}
	bool SearchUp() { return !!m_bSearchUp; }
	CString GetFindString() { return m_FindCombo.GetString(); }
	CString GetReplaceString() { return m_ReplaceCombo.GetString(); }
	void SetFindString(const CString& str) { if (!str.IsEmpty()) { m_FindCombo.SetWindowText(str); } }
	void SetStatusText(const CString& str, COLORREF color = RGB(0, 0, 255));
	void SetReadonly(bool bReadonly);
// Dialog Data
	enum { IDD = IDD_FIND };

	enum FindType
	{
		Find,
		Count,
		Replace,
		ReplaceAll
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnEditchangeFindcombo();
	afx_msg void OnBnClickedCount();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedReplace();
	afx_msg void OnBnClickedReplaceall();
	void SaveWindowPos(CWnd* pParent);
private:
	UINT			m_FindMsg;
	bool			m_bTerminating;
	bool			m_bFindNext;
	BOOL			m_bMatchCase;
	BOOL			m_bLimitToDiffs;
	BOOL			m_bWholeWord;
	BOOL			m_bSearchUp;
	CHistoryCombo	m_FindCombo;
	CHistoryCombo	m_ReplaceCombo;
	CStatic			m_FindStatus;
	CWnd *			m_pParent;
	CRegDWORD		m_regMatchCase;
	CRegDWORD		m_regLimitToDiffs;
	CRegDWORD		m_regWholeWord;
	COLORREF		m_clrFindStatus;
	bool			m_bReadonly;
	int				m_id;
};
