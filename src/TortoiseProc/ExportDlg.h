// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008 - Stefan Kueng
// Copyright (C) 2008-2011, 2013, 2015-2016 - TortoiseGit

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
#include "ChooseVersion.h"

/**
 * \ingroup TortoiseProc
 * Prompts the user for required information for an export command. The information
 * is the module name and the repository url.
 */
class CExportDlg : public CHorizontalResizableStandAloneDialog, public CChooseVersion
{
	DECLARE_DYNAMIC(CExportDlg)

public:
	CExportDlg(CWnd* pParent = nullptr);   ///< standard constructor
	virtual ~CExportDlg();

	// Dialog Data
	enum { IDD = IDD_EXPORT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCheckoutdirectoryBrowse();
	afx_msg void OnEnChangeCheckoutdirectory();
	afx_msg void OnBnClickedWholeProject();
	afx_msg void OnBnClickedShowlog();
	void SetDlgTitle();

	DECLARE_MESSAGE_MAP()
protected:
	CString			m_sTitle;

	CHOOSE_EVENT_RADIO()	;

public:
	CTGitPath		m_orgPath;
	BOOL			m_bWholeProject;
	CString			m_Revision;
	CButton			m_butBrowse;
	CString			m_strFile;
};
