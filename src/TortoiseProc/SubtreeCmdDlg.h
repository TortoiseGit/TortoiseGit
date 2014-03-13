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


class CSubtreeCmdDlg : public CResizableStandAloneDialog, public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CSubtreeCmdDlg)

public:
	// TODO: would it be better to expose strings on the dialog instead of using this? See usage in cpp.
	enum SubCommand
	{
		SubCommand_Add,
		SubCommand_Push,
		SubCommand_Pull,
	};

	CSubtreeCmdDlg(const CString &subFolder, SubCommand subCommand, CWnd* pParent = NULL);
	virtual ~CSubtreeCmdDlg();

	BOOL OnInitDialog();

	// Dialog Data
	enum { IDD = IDD_SUBTREE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnBnClickedCheckSquash();
	void OnCbnSelchangeRemote();
	void OnPathBrowse();
 	void OnBnClickedPuttykeyAutoload();
	void OnBnClickedPuttykeyfileBrowse();
 	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	void OnBnClickedRd();
	void SetupComboBoxValues();

	ProjectProperties m_ProjectProperties;
	CSciEdit m_cLogMessage;

public:

	SubCommand m_eSubCommand;

	// values in the dialog, Data exachange assigns them to the controls
	CHistoryCombo m_PuttyKeyCombo;
	CHistoryCombo m_PathCtrl;
	CHistoryCombo m_Remote;
	CHistoryCombo m_RemoteBranch;

	CHistoryCombo m_RemoteURL;
	CHistoryCombo m_RemoteURLBranch;

	CString	m_URL;	// remote url or path
	CString	m_BranchName;	// remote branch

	BOOL m_bAutoloadPuttyKeyFile;
	BOOL m_bSquash;

	// This is the path where we'll create the new subtree
	CString m_strPath;
	CString	m_strPuttyKeyFile;
	CString m_strLogMesage;

	TCHAR *m_pDefaultText;
};
