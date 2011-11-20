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
#include "Tooltip.h"
#include "Registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff. 
 */
class CSetMisc : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetMisc)

public:
	CSetMisc();
	virtual ~CSetMisc();

	UINT GetIconID() {return IDI_DIALOGS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSMISC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnChanged();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

private:
	CToolTips		m_tooltips;

	CRegDWORD		m_regUnversionedRecurse;
	BOOL			m_bUnversionedRecurse;
	CRegDWORD		m_regAutocompletion;
	BOOL			m_bAutocompletion;
	CRegDWORD		m_regAutocompletionTimeout;
	DWORD			m_dwAutocompletionTimeout;
	CRegDWORD		m_regSpell;
	BOOL			m_bSpell;
	CRegDWORD		m_regCheckRepo;
	BOOL			m_bCheckRepo;
	CRegDWORD		m_regMaxHistory;
	DWORD			m_dwMaxHistory;
	CRegDWORD		m_regCommitReopen;
	BOOL			m_bCommitReopen;
	CRegDWORD		m_regAutoSelect;
	BOOL			m_bAutoSelect;
};
