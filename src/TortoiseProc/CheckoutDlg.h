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
#if 0
#include "GitRev.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "FileDropEdit.h"
#include "LogDlg.h"
#include "afxwin.h"
#include "Tooltip.h"

/**
 * \ingroup TortoiseProc
 * Prompts the user for required information for a checkout command. The information
 * is the module name and the repository url.
 */
class CCheckoutDlg : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckoutDlg)

public:
	CCheckoutDlg(CWnd* pParent = NULL);   ///< standard constructor
	virtual ~CCheckoutDlg();

// Dialog Data
	enum { IDD = IDD_CHECKOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedCheckoutdirectoryBrowse();
	afx_msg void OnEnChangeCheckoutdirectory();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedShowlog();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeRevisionNum();
	afx_msg void OnCbnEditchangeUrlcombo();

	DECLARE_MESSAGE_MAP()

	void		SetRevision(const GitRev& rev);
protected:
	CToolTips		m_tooltips;
	CString			m_sRevision;
	CString			m_sCheckoutDirOrig;
	bool			m_bAutoCreateTargetName;
	CComboBox		m_depthCombo;
public:
	CHistoryCombo	m_URLCombo;
	CString			m_URL;
	GitRev			Revision;
	BOOL			m_bNoExternals;
	CButton			m_butBrowse;
	CEdit			m_editRevision;
	CString			m_strCheckoutDirectory;
	CFileDropEdit	m_cCheckoutEdit;
	CLogDlg	*		m_pLogDlg;
	git_depth_t		m_depth;
};
#endif