// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "GitRev.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog which shows revision properties.
 */
class CPropDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CPropDlg)

public:
	CPropDlg(CWnd* pParent = NULL);
	virtual ~CPropDlg();

	enum { IDD = IDD_PROPERTIES };
private:
	static UINT PropThreadEntry(LPVOID pVoid);
	UINT		PropThread();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	DECLARE_MESSAGE_MAP()

public:
	CTGitPath	m_Path;
	GitRev		m_rev;
private:
	HANDLE		m_hThread;
	CListCtrl	m_proplist;
};

