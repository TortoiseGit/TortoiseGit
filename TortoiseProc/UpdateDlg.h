// TortoiseSVN - a Windows shell extension for easy version control

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
#include "SVNRev.h"
#include "LogDlg.h"
#include "afxwin.h"


/**
 * \ingroup TortoiseProc
 * A simple dialog box asking the user for a revision number
 * to update to.
 */
class CUpdateDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CUpdateDlg)

public:
	CUpdateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUpdateDlg();

// Dialog Data
	enum { IDD = IDD_UPDATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedShowLog();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeRevnum();

	DECLARE_MESSAGE_MAP()

	CLogDlg *	m_pLogDlg;
	CString		m_sRevision;
	CComboBox	m_depthCombo;
public:
	SVNRev		Revision;
	CTSVNPath	m_wcPath;
	BOOL		m_bNoExternals;
	svn_depth_t	m_depth;
};
