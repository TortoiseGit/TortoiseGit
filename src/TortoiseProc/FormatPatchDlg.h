// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2016 - TortoiseGit

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
#include "HistoryCombo.h"
#include "registry.h"

// CFormatPatchDlg dialog

class CFormatPatchDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFormatPatchDlg)

public:
	CFormatPatchDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFormatPatchDlg();

// Dialog Data
	enum { IDD = IDD_FORMAT_PATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;

	afx_msg void OnBnClickedButtonDir();
	afx_msg void OnBnClickedButtonFrom();
	afx_msg void OnBnClickedButtonTo();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedButtonRef();
	afx_msg void OnBnClickedButtonUnifieddiff();

	CHistoryCombo m_cDir;
	CHistoryCombo m_cSince;
	CHistoryCombo m_cFrom;
	CHistoryCombo m_cTo;
	CSpinButtonCtrl		m_spinNum;
	CEdit		  m_cNum;
	CRegDWORD	m_regSendMail;
	CRegDWORD	m_regNoPrefix;
	CRegString	m_regSince;

	DECLARE_MESSAGE_MAP()
public:
	int m_Num;
	CString m_Dir;
	CString m_From;
	CString m_To;
	CString m_Since;
	int m_Radio;
	BOOL m_bSendMail;
	BOOL m_bNoPrefix;
};
