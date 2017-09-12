// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit

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

class CFindBar : public CDialog
{
	DECLARE_DYNAMIC(CFindBar)

public:
	CFindBar(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFindBar(void);

	// Dialog Data
	enum { IDD = IDD_FINDBAR };

	bool IsMatchCase() const { return m_bMatchCase == BST_CHECKED; }
	const CString GetFindText() const { return m_sFindStr; }
	void SetFocusTextBox() const { GetDlgItem(IDC_FINDTEXT)->SetFocus(); }

	static UINT				WM_FINDEXIT;
	static UINT				WM_FINDNEXT;
	static UINT				WM_FINDPREV;
	static UINT				WM_FINDRESET;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	virtual void OnCancel() override;
	afx_msg void OnFindTextChange();
	afx_msg void OnFindNext();
	afx_msg void OnFindPrev();
	afx_msg void OnFindExit();

	HICON					m_hIcon;

	CString					m_sFindStr;
	BOOL					m_bMatchCase;
};
