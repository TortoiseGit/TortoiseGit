// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
#include "SciEdit.h"
#include "ProjectProperties.h"
// CPatchViewDlg dialog
class CCommitDlg;
class CPatchViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CPatchViewDlg)

public:
	CPatchViewDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatchViewDlg();
	CCommitDlg	*m_ParentCommitDlg;

// Dialog Data
	enum { IDD = IDD_PATCH_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	CSciEdit			m_ctrlPatchView;
	ProjectProperties	*m_pProjectProperties;

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
};
