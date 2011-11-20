// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "ProjectProperties.h"
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to let the user enter a log/commit message.
 */
class CInputLogDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CInputLogDlg)

public:
	CInputLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInputLogDlg();

	void SetProjectProperties(ProjectProperties * pProps) {m_pProjectProperties = pProps;}
	void SetUUID(const CString& sUUID) {m_sUUID = sUUID;}
	void SetActionText(const CString& sAction) {m_sActionText = sAction;}
	CString GetLogMessage() {return m_sLogMsg;}

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnEnChangeLogmessage();
	afx_msg void OnBnClickedHistory();
	DECLARE_MESSAGE_MAP()

// Dialog Data
	enum { IDD = IDD_INPUTLOGDLG };

private:
	void		UpdateOKButton();


private:
	CSciEdit			m_cInput;
	ProjectProperties * m_pProjectProperties;
	CFont				m_logFont;
	CString				m_sLogMsg;
	CString				m_sUUID;
	CString				m_sActionText;
};
