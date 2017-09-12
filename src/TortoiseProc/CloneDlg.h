// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2017 - TortoiseGit

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
#include "MenuButton.h"
#include "registry.h"
// CCloneDlg dialog

class CCloneDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNCREATE(CCloneDlg)

public:
	CCloneDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCloneDlg();

// Dialog Data
	enum { IDD = IDD_CLONE};

protected:
	// Overrides
	virtual void OnOK() override;
	virtual void OnCancel() override;

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;

	CString m_ModuleName;
	CString m_OldURL;

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedCloneBrowseUrl();
	afx_msg void OnBnClickedCloneDirBrowse();

	CHistoryCombo	m_URLCombo;
	CHistoryCombo	m_PuttyKeyCombo;

public:
	CString	m_Directory;
	CString	m_strPuttyKeyFile;
	CString	m_URL;
	CString m_strBranch;
	CString	m_strOrigin;
	BOOL	m_bAutoloadPuttyKeyFile;
	BOOL	m_bSVN;
	BOOL	m_bSVNTrunk;
	BOOL	m_bSVNTags;
	BOOL	m_bSVNBranch;
	BOOL	m_bSVNFrom;
	BOOL	m_bDepth;
	BOOL	m_bSVNUserName;
	BOOL	m_bBare;
	BOOL	m_bRecursive;
	BOOL	m_bBranch;
	BOOL	m_bOrigin;
	BOOL	m_bNoCheckout;
	BOOL	m_bExactPath;
	BOOL	m_bUseLFS;

	int		m_nDepth;

	CString	m_strSVNTrunk;
	CString	m_strSVNTags;
	CString	m_strSVNBranchs;
	CString	m_strUserName;

	int		m_nSVNFrom;

protected:
	CMenuButton	m_BrowseUrl;
	CRegDWORD	m_regBrowseUrl;
	CRegDWORD	m_regUseSSHKey;
	CRegDWORD	m_regCloneRecursive;
	CRegString	m_regCloneDir;
	bool		m_bSaving;

	afx_msg void OnBnClickedPuttykeyfileBrowse();
	afx_msg void OnBnClickedPuttykeyAutoload();
	afx_msg void OnCbnEditchangeUrlcombo();

	afx_msg void OnBnClickedCheckRecursive();
	afx_msg void OnBnClickedCheckBare();
	afx_msg void OnBnClickedCheckBranch();
	afx_msg void OnBnClickedCheckOrigin();
	afx_msg void OnBnClickedCheckSvn();
	afx_msg void OnBnClickedCheckSvnTrunk();
	afx_msg void OnBnClickedCheckSvnTag();
	afx_msg void OnBnClickedCheckSvnBranch();
	afx_msg void OnBnClickedCheckSvnFrom();
	afx_msg void OnBnClickedCheckDepth();
	afx_msg void OnBnClickedCheckUsername();
};
