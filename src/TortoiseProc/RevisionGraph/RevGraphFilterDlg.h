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
#include "afxcmn.h"

/**
 * \ingroup TortoiseProc
 * helper dialog to enter filter data for the revision graph.
 */
class CRevGraphFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CRevGraphFilterDlg)

public:
	CRevGraphFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevGraphFilterDlg();

	void	SetMaxRevision (svn_revnum_t rev) {m_HeadRev = rev;}
	void	GetRevisionRange (svn_revnum_t& minrev, svn_revnum_t& maxrev);
	void	SetRevisionRange (svn_revnum_t minrev, svn_revnum_t maxrev);
	CString	GetFilterString() {return m_sFilterPaths;}
	void	SetFilterString(const CString& str) {m_sFilterPaths = str;}

// Dialog Data
	enum { IDD = IDD_REVGRAPHFILTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnDeltaposFromspin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposTospin(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()
protected:
	CString				m_sFilterPaths;
	CSpinButtonCtrl		m_cFromSpin;
	CSpinButtonCtrl		m_cToSpin;
	CString				m_sFromRev;
	CString				m_sToRev;
	svn_revnum_t		m_HeadRev;
	svn_revnum_t		m_minrev;
	svn_revnum_t		m_maxrev;
};
