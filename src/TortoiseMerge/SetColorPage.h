// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2008, 2013-2014, 2017 - TortoiseSVN
// Copyright (C) 2016 - TortoiseGit

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

#include "registry.h"


/**
 * \ingroup TortoiseMerge
 * Color settings page
 */
class CSetColorPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetColorPage)

public:
	CSetColorPage();
	virtual ~CSetColorPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	BOOL	m_bReloadNeeded;
// Dialog Data
	enum { IDD = IDD_SETCOLORPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	afx_msg void OnBnClickedColor();
	afx_msg void OnBnClickedRestore();

	DECLARE_MESSAGE_MAP()

protected:
	BOOL m_bInit;
	CRegDWORD		m_regInlineAdded;
	CRegDWORD		m_regInlineRemoved;
	CRegDWORD		m_regModifiedBackground;
	CMFCColorButton m_cBkNormal;
	CMFCColorButton m_cBkRemoved;
	CMFCColorButton m_cBkAdded;
	CMFCColorButton m_cBkInlineAdded;
	CMFCColorButton m_cBkInlineRemoved;
	CMFCColorButton m_cBkEmpty;
	CMFCColorButton m_cBkConflict;
	CMFCColorButton m_cBkConflictResolved;
	CMFCColorButton m_cBkModified;
	CMFCColorButton m_cFgNormal;
	CMFCColorButton m_cFgRemoved;
	CMFCColorButton m_cFgAdded;
	CMFCColorButton m_cFgEmpty;
	CMFCColorButton m_cFgConflict;
	CMFCColorButton m_cFgConflictResolved;
	CMFCColorButton m_cFgWhitespaces;
	CMFCColorButton m_cBkFiltered;
};
