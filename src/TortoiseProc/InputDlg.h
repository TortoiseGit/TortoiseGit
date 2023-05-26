// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2016, 2023 - TortoiseGit
// Copyright (C) 2003-2006 - Stefan Kueng

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

#include "StandAloneDlg.h"
#include "SciEdit.h"
#include "resource.h"
/**
 * \ingroup TortoiseProc
 * Helper dialog to ask for various text inputs.
 */
class CInputDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CInputDlg)

public:
	CInputDlg(CWnd* pParent = nullptr);
	virtual ~CInputDlg();

	enum { IDD = IDD_INPUTDLG };

protected:
	CFont			m_logFont;

	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	void OnOK() override;

	afx_msg void OnEnChangeLogmessage();
	DECLARE_MESSAGE_MAP()
public:
	CString				m_sInputText;
	CString				m_sHintText;
	CString				m_sTitle;
	CString				m_sCheckText;
	int					m_iCheck = 0;
	CSciEdit			m_cInput;
	ProjectProperties*	m_pProjectProperties = nullptr;
	bool				m_bUseLogWidth = true;
};
