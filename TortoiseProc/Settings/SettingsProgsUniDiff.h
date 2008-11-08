// TortoiseSVN - a Windows shell extension for easy version control

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
#include "Tooltip.h"


/**
 * \ingroup TortoiseProc
 * Settings page to configure external unified diff viewers.
 */
class CSettingsProgsUniDiff : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsProgsUniDiff)

public:
	CSettingsProgsUniDiff();
	virtual ~CSettingsProgsUniDiff();

	UINT GetIconID() {return IDI_DIFF;}

	enum { IDD = IDD_SETTINGSPROGSUNIDIFF };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedDiffviewerOff();
	afx_msg void OnBnClickedDiffviewerOn();
	afx_msg void OnBnClickedDiffviewerbrowse();
	afx_msg void OnEnChangeDiffviewer();

	DECLARE_MESSAGE_MAP()

private:
	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != _T("#"); }
	void CheckProgComment();
private:
	CString			m_sDiffViewerPath;
	CRegString		m_regDiffViewerPath;
	int             m_iDiffViewer;
	CToolTips		m_tooltips;

	CFileDropEdit	m_cUnifiedDiffEdit;
};
