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
#include "SettingsPropPage.h"
#include "SetProgsAdvDlg.h"
#include "FileDropEdit.h"

/**
 * \ingroup TortoiseProc
 * Setting page to configure the external merge tools.
 */
class CSettingsProgsMerge : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsProgsMerge)

public:
	CSettingsProgsMerge();
	virtual ~CSettingsProgsMerge();

	UINT GetIconID() override { return IDI_MERGE; }
// Dialog Data
	enum { IDD = IDD_SETTINGSPROGSMERGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnBnClickedExtmergeOff();
	afx_msg void OnBnClickedExtmergeOn();
	afx_msg void OnBnClickedExtmergebrowse();
	afx_msg void OnBnClickedExtmergeadvanced();
	afx_msg void OnEnChangeExtmerge();
private:
	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != L"#"; }
	void CheckProgComment();
private:
	CString			m_sMergePath;
	CRegString		m_regMergePath;
	int             m_iExtMerge;
	CSetProgsAdvDlg m_dlgAdvMerge;

	CFileDropEdit	m_cMergeEdit;
};
