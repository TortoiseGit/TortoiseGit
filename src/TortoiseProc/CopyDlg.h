// TortoiseGit - a Windows shell extension for easy version control

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
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "RegHistory.h"
#include "SciEdit.h"
#include "TSVNPath.h"
#include "SVNRev.h"
#include "LogDlg.h"
#include "Tooltip.h"

#define WM_TSVN_MAXREVFOUND			(WM_APP + 1)

/**
 * \ingroup TortoiseProc
 * Prompts the user for the required information needed for a copy command.
 * The required information is a single URL to copy the current URL of the 
 * working copy to.
 */
class CCopyDlg : public CResizableStandAloneDialog, public SVN
{
	DECLARE_DYNAMIC(CCopyDlg)

public:
	CCopyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCopyDlg();

// Dialog Data
	enum { IDD = IDD_COPY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT OnRevFound(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBrowsefrom();
	afx_msg void OnBnClickedCopyhead();
	afx_msg void OnBnClickedCopyrev();
	afx_msg void OnBnClickedCopywc();
	afx_msg void OnBnClickedHistory();
	afx_msg void OnEnChangeLogmessage();
	afx_msg void OnCbnEditchangeUrlcombo();
	DECLARE_MESSAGE_MAP()

	virtual BOOL Cancel() {return m_bCancelled;}
	void		SetRevision(const SVNRev& rev);
public:
	CString			m_URL;
	CTSVNPath		m_path;
	CString			m_sLogMessage;
	SVNRev			m_CopyRev;
	BOOL			m_bDoSwitch;

private:
	CLogDlg *		m_pLogDlg;
	CSciEdit		m_cLogMessage;
	CFont			m_logFont;
	BOOL			m_bFile;
	ProjectProperties	m_ProjectProperties;
	CString			m_sBugID;
	CHistoryCombo	m_URLCombo;
	CString			m_wcURL;
	CButton			m_butBrowse;
	CRegHistory		m_History;
	CToolTips		m_tooltips;

	svn_revnum_t	m_minrev;
	svn_revnum_t	m_maxrev;
	bool			m_bswitched;
	bool			m_bmodified;
	bool			m_bSparse;
	bool			m_bSettingChanged;
	static UINT		FindRevThreadEntry(LPVOID pVoid);
	UINT			FindRevThread();
	CWinThread *	m_pThread;
	bool			m_bCancelled;
	volatile LONG	m_bThreadRunning;
};
