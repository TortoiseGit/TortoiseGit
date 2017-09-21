// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2017 - TortoiseGit

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

class CDeleteRemoteTagDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CDeleteRemoteTagDlg)

public:
	CDeleteRemoteTagDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDeleteRemoteTagDlg();

// Dialog Data
	enum { IDD = IDD_DELETEREMOTETAG };

	CString m_sRemote;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;

	DECLARE_MESSAGE_MAP()

	CListCtrl m_ctrlTags;
	CButton	m_SelectAll;

	void Refresh();

	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedOk();
	afx_msg void OnSelchangeTags(NMHDR* pNMHDR, LRESULT* pResult);
};
