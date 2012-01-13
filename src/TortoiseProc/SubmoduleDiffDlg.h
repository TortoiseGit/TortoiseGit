// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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
#include "resource.h"

class CSubmoduleDiffDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleDiffDlg)

public:
	CSubmoduleDiffDlg(CWnd* pParent = NULL);
	virtual ~CSubmoduleDiffDlg();

	enum { IDD = IDD_DIFFSUBMODULE };

	void SetDiff(CString path, bool toIsWorkingCopy, CString fromHash, CString fromSubject, CString toHash, CString toSubject);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	bool	m_bToIsWorkingCopy;
	CString	m_sPath;

	CString	m_sFromHash;
	CString	m_sFromSubject;
	CString	m_sToHash;
	CString	m_sToSubject;
};
