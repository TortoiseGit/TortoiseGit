// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "GitRev.h"
#include "Registry.h"
#include "StandAloneDlg.h"
#include "TGitPath.h"

/**
 * \ingroup TortoiseProc
 * Show the blame dialog where the user can select the revision to blame
 * and whether to use TortoiseBlame or the default text editor to view the blame.
 */
class CBlameDlg : public CStandAloneDialog
{
//	DECLARE_DYNAMIC(CBlameDlg)

public:
	CBlameDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBlameDlg();

// Dialog Data
	enum { IDD = IDD_BLAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeRevisionEnd();
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

protected:
	CString m_sStartRev;
	CString m_sEndRev;
	CRegDWORD m_regTextView;

public:
	CTGitPath	m_path;
	CString	StartRev;
	CString	EndRev;
	BOOL	m_bTextView;
	BOOL	m_bIgnoreEOL;
	BOOL	m_bIncludeMerge;
	int		m_IgnoreSpaces;
};
