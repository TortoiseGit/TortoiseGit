// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2018 - TortoiseGit

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
#include "CommonDialogFunctions.h"

// CUserPassword dialog

#define MAX_LENGTH_PASSWORD 256

class CUserPassword : public CDialog, protected CommonDialogFunctions<CDialog>
{
	DECLARE_DYNAMIC(CUserPassword)

public:
	CUserPassword(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CUserPassword();

// Dialog Data
	enum { IDD = IDD_USER_PASSWD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_UserName;
	CString m_URL;
	TCHAR m_password[MAX_LENGTH_PASSWORD];
	char m_passwordA[4 * MAX_LENGTH_PASSWORD];
	virtual BOOL OnInitDialog() override;
	afx_msg void OnBnClickedOk();
	afx_msg void OnDestroy();
};
