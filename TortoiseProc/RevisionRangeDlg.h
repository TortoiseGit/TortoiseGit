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

#include "SVNRev.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * A dialog to select a revision range.
 */
class CRevisionRangeDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CRevisionRangeDlg)

public:
	CRevisionRangeDlg(CWnd* pParent = NULL);
	virtual ~CRevisionRangeDlg();

	enum { IDD = IDD_REVISIONRANGE };

	/**
	 * Returns the string entered in the start revision edit box.
	 */
	CString GetEnteredStartRevisionString() const {return m_sStartRevision;}

	/**
	 * Returns the string entered in the end revision edit box.
	 */
	CString GetEnteredEndRevisionString() const {return m_sEndRevision;}

	/**
	 * Returns the entered start revision.
	 */
	SVNRev GetStartRevision() const {return m_StartRev;}

	/**
	 * Returns the entered end revision.
	 */
	SVNRev GetEndRevision() const {return m_EndRev;}

	/**
	 * Sets the start revision to fill in when the dialog shows up.
	 */
	void SetStartRevision(const SVNRev& rev) {m_StartRev = rev;}

	/**
	 * Sets the end revision to fill in when the dialog shows up.
	 */
	void SetEndRevision(const SVNRev& rev) {m_EndRev = rev;}

	/**
	 * If set to \a true, then working copy revisions like BASE, WC, PREV are allowed.
	 * Otherwise, an error balloon is shown when the user tries to enter such revisions.
	 */
	void AllowWCRevs(bool bAllowWCRevs = true) {m_bAllowWCRevs = bAllowWCRevs;}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEnChangeRevnum();
	afx_msg void OnEnChangeRevnum2();

	DECLARE_MESSAGE_MAP()

protected:
	CString m_sStartRevision;
	CString m_sEndRevision;
	SVNRev	m_StartRev;
	SVNRev	m_EndRev;
	bool	m_bAllowWCRevs;
};
