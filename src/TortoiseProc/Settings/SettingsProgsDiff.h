// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2017 - TortoiseGit
// Copyright (C) 2003-2008,2011 - TortoiseSVN

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
 * Settings page to configure the external diff tools.
 */
class CSettingsProgsDiff : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsProgsDiff)

public:
	CSettingsProgsDiff();
	virtual ~CSettingsProgsDiff();

	UINT GetIconID() override { return IDI_DIFF; }

	enum { IDD = IDD_SETTINGSPROGSDIFF };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnBnClickedExtdiffOff();
	afx_msg void OnBnClickedExtdiffOn();
	afx_msg void OnBnClickedExtdiffbrowse();
	afx_msg void OnBnClickedExtdiffadvanced();
	afx_msg void OnEnChangeExtdiff();
	afx_msg void OnBnClickedDiffviewerOff();
	afx_msg void OnBnClickedDiffviewerOn();
	afx_msg void OnBnClickedDiffviewerbrowse();
	afx_msg void OnEnChangeDiffviewer();

	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != L"#"; }
	void CheckProgComment();

private:
	CString			m_sDiffPath;
	CRegString		m_regDiffPath;
	int				m_iExtDiff;
	CSetProgsAdvDlg m_dlgAdvDiff;
	CString			m_sDiffViewerPath;
	CRegString		m_regDiffViewerPath;
	int				m_iDiffViewer;

	CFileDropEdit	m_cDiffEdit;
	CFileDropEdit	m_cUnifiedDiffEdit;
};
