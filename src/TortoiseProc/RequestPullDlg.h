// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - TortoiseGit
// Copyright (C) 2011,2013 Sven Strickroth, <email@cs-ware.de>

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

class CRequestPullDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRequestPullDlg)

public:
	CRequestPullDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRequestPullDlg();

	// Dialog Data
	enum { IDD = IDD_REQUESTPULL };

protected:
	virtual BOOL OnInitDialog() override;

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonLocalBranch();

	CHistoryCombo	m_cStartRevision;
	CHistoryCombo	m_cRepositoryURL;
	CEdit			m_cEndRevision;
	CRegString		m_RegStartRevision;
	CRegString		m_RegRepositoryURL;
	CRegString		m_RegEndRevision;
	CRegDWORD		m_regSendMail;

public:
	BOOL			m_bSendMail;
	CString			m_StartRevision;
	CString			m_RepositoryURL;
	CString			m_EndRevision;
};
