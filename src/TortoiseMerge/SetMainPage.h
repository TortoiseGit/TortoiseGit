// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseGit
// Copyright (C) 2006-2010, 2013-2014 - TortoiseSVN

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

#include "resource.h"
#include "registry.h"

/**
 * \ingroup TortoiseMerge
 * Main settings page.
 */
class CSetMainPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();

	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	BOOL	m_bReloadNeeded;
	enum { IDD = IDD_SETMAINPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();

	afx_msg void OnModified();
	afx_msg void OnModifiedWithReload();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	DECLARE_MESSAGE_MAP()

	BOOL DialogEnableWindow(UINT nID, BOOL bEnable);

	BOOL			m_bBackup;
	CRegDWORD		m_regBackup;
	BOOL			m_bFirstDiffOnLoad;
	CRegDWORD		m_regFirstDiffOnLoad;
	BOOL			m_bFirstConflictOnLoad;
	CRegDWORD		m_regFirstConflictOnLoad;
	BOOL			m_bUseSpaces;
	BOOL			m_bSmartTabChar;
	CRegDWORD		m_regTabMode;
	int				m_nTabSize;
	CRegDWORD		m_regTabSize;
	BOOL			m_bEnableEditorConfig;
	CRegDWORD		m_regEnableEditorConfig;
	int				m_nContextLines;
	CRegDWORD		m_regContextLines;
	BOOL			m_bIgnoreEOL;
	CRegDWORD		m_regIgnoreEOL;
	BOOL			m_bOnePane;
	CRegDWORD		m_regOnePane;
	BOOL			m_bViewLinenumbers;
	CRegDWORD		m_regViewLinenumbers;
	BOOL			m_bCaseInsensitive;
	CRegDWORD		m_regCaseInsensitive;
	BOOL			m_bUTF8Default;
	CRegDWORD		m_regUTF8Default;
	BOOL			m_bAutoAdd;
	CRegDWORD		m_regAutoAdd;
	int				m_nMaxInline;
	CRegDWORD		m_regMaxInline;
	BOOL			m_bUseRibbons;
	CRegDWORD		m_regUseRibbons;

	CRegDWORD		m_regFontSize;
	DWORD			m_dwFontSize;
	CRegString		m_regFontName;
	CString			m_sFontName;

	CMFCFontComboBox m_cFontNames;
	CComboBox		m_cFontSizes;
};
