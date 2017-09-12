// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013-2014, 2016 - TortoiseGit
// Copyright (C) 2007-2008 - TortoiseSVN

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
 * Settings page to configure TortoiseBlame
 */
class CSettingsTBlame : public ISettingsPropPage
{
//	DECLARE_DYNAMIC(CSettingsTBlame)

public:
	CSettingsTBlame();
	virtual ~CSettingsTBlame();

	UINT GetIconID() override { return IDI_TORTOISEBLAME; }

// Dialog Data
	enum { IDD = IDD_SETTINGSTBLAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnBnClickedColor();
	afx_msg void OnChange();
	afx_msg void OnBnClickedRestore();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	void UpdateDependencies();

	DECLARE_MESSAGE_MAP()

private:
	CMFCColorButton		m_cNewLinesColor;
	CMFCColorButton		m_cOldLinesColor;
	CRegDWORD			m_regNewLinesColor;
	CRegDWORD			m_regOldLinesColor;

	CMFCFontComboBox	m_cFontNames;
	CComboBox			m_cFontSizes;
	CRegDWORD			m_regFontSize;
	DWORD				m_dwFontSize;
	CRegString			m_regFontName;
	CString				m_sFontName;
	DWORD				m_dwTabSize;
	CRegDWORD			m_regTabSize;
	CComboBox			m_cDetectMovedOrCopiedLines;
	DWORD				m_dwDetectMovedOrCopiedLines;
	CRegDWORD			m_regDetectMovedOrCopiedLines;
	DWORD				m_dwDetectMovedOrCopiedLinesNumCharactersWithinFile;
	CRegDWORD			m_regDetectMovedOrCopiedLinesNumCharactersWithinFile;
	DWORD				m_dwDetectMovedOrCopiedLinesNumCharactersFromFiles;
	CRegDWORD			m_regDetectMovedOrCopiedLinesNumCharactersFromFiles;
	BOOL				m_bIgnoreWhitespace;
	CRegDWORD			m_regIgnoreWhitespace;
	BOOL				m_bShowCompleteLog;
	CRegDWORD			m_regShowCompleteLog;
	BOOL				m_bFollowRenames;
	CRegDWORD			m_regFollowRenames;
	BOOL				m_bFirstParent;
	CRegDWORD			m_regFirstParent;
};
