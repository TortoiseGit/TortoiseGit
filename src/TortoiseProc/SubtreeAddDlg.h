// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseGit

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
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "SciEdit.h"


class CSubtreeAddDlg : public CResizableStandAloneDialog, public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CSubtreeAddDlg)

public:
	CSubtreeAddDlg(const CString &subFolder, CWnd* pParent = NULL);
	virtual ~CSubtreeAddDlg();

	BOOL OnInitDialog();

	// Dialog Data
	enum { IDD = IDD_SUBTREE_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// 	void OnRepBrowse();
// 	void OnBranchCheck();
	void OnCbnSelchangeRemote();
	void OnPathBrowse();
 	void OnBnClickedPuttykeyAutoload();
	void OnBnClickedPuttykeyfileBrowse();
 	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:

	// values in the dialog, Data exachange assigns them to the controls
	CHistoryCombo m_PuttyKeyCombo;
	CHistoryCombo m_PathCtrl;
	CHistoryCombo m_Remote;
	CHistoryCombo m_RemoteBranch;
	CString	m_URL;

	BOOL m_bAutoloadPuttyKeyFile;
	BOOL m_bSquash;

	CString m_strPath;

	ProjectProperties m_ProjectProperties;
	CSciEdit m_cLogMessage;
	TCHAR *m_pDefaultText;
};
