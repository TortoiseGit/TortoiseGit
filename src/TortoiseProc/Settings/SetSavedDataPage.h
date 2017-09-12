// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012, 2015 - TortoiseGit
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
#include "SettingsPropPage.h"
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Setting page to clear various saved/cached data.
 */
class CSetSavedDataPage : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetSavedDataPage)

public:
	CSetSavedDataPage();
	virtual ~CSetSavedDataPage();

	UINT GetIconID() override { return IDI_SAVEDDATA; }

	enum { IDD = IDD_SETTINGSSAVEDDATA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedUrlhistclear();
	afx_msg void OnBnClickedLoghistclear();
	afx_msg void OnBnClickedResizablehistclear();
	afx_msg void OnBnClickedAuthhistclear();
	afx_msg void OnBnClickedRepologclear();
	afx_msg void OnBnClickedActionlogshow();
	afx_msg void OnBnClickedActionlogclear();
	afx_msg void OnBnClickedTempfileclear();
	afx_msg void OnBnClickedStoreddecisionsclear();
	afx_msg void OnModified();

	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	void DeleteViaShell(LPCTSTR path, UINT progressText);

private:
	CButton			m_btnUrlHistClear;
	CButton			m_btnLogHistClear;
	CButton			m_btnResizableHistClear;
	CButton			m_btnAuthHistClear;
	CButton			m_btnRepoLogClear;
	CButton			m_btnActionLogShow;
	CButton			m_btnActionLogClear;
	DWORD			m_maxLines;
	CRegDWORD		m_regMaxLines;
};
