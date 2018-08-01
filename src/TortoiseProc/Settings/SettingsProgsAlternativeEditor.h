// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2011, 2018 - Sven Strickroth <email@cs-ware.de>

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
 * Settings page to configure external unified diff viewers.
 */
class CSettingsProgsAlternativeEditor : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsProgsAlternativeEditor)

public:
	CSettingsProgsAlternativeEditor();
	virtual ~CSettingsProgsAlternativeEditor();

	UINT GetIconID() override { return IDI_NOTEPAD; }

	enum { IDD = IDD_SETTINGSPROGSALTERNATIVEEDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnBnClickedAlternativeEditorOff();
	afx_msg void OnBnClickedAlternativeEditorOn();
	afx_msg void OnBnClickedAlternativeEditorBrowse();
	afx_msg void OnEnChangeAlternativeEditor();

	DECLARE_MESSAGE_MAP()

private:
	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != L"#"; }
	void CheckProgComment();

	CString			m_sAlternativeEditorPath;
	CRegString		m_regAlternativeEditorPath;
	int				m_iAlternativeEditor;

	CFileDropEdit	m_cAlternativeEditorEdit;
};
