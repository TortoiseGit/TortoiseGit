// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseGit

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
#include "HistoryCombo.h"
#include "ChooseVersion.h"
#include "SciEdit.h"
// CMergeDlg dialog

class CMergeDlg : public CResizableStandAloneDialog,public CChooseVersion , public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CMergeDlg)

public:
	CMergeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

	BOOL m_bSquash;
	BOOL m_bNoFF;
	BOOL m_bNoCommit;
	BOOL m_bLog;
	int m_nLog;
	//CString m_Base;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CSciEdit			m_cLogMessage;
	ProjectProperties	m_ProjectProperties;

	TCHAR				* m_pDefaultText;
	DECLARE_MESSAGE_MAP()
	CHOOSE_EVENT_RADIO() ;

public:
	CString m_strLogMesage;

	afx_msg void OnBnClickedOk();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCheckMergeLog();
};
