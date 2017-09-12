// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseGit
// Copyright (C) 2003-2008, 2010, 2017 - TortoiseSVN

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
 * Settings page to configure how the icon overlays and the cache should
 * behave.
 */
class CSetOverlayPage : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetOverlayPage)

public:
	CSetOverlayPage();
	virtual ~CSetOverlayPage();

	UINT GetIconID() override { return IDI_SET_OVERLAYS; }

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnChange();
	virtual BOOL OnApply() override;

	DECLARE_MESSAGE_MAP()

private:
	BOOL			m_bOnlyExplorer;
	BOOL			m_bOnlyNonElevated;
	BOOL			m_bRemovable;
	BOOL			m_bFloppy;
	BOOL			m_bNetwork;
	BOOL			m_bFixed;
	BOOL			m_bCDROM;
	BOOL			m_bRAM;
	BOOL			m_bUnknown;
	BOOL			m_bUnversionedAsModified;
	BOOL			m_bRecurseSubmodules;
	BOOL			m_bShowExcludedAsNormal;
	CRegDWORD		m_regOnlyExplorer;
	CRegDWORD		m_regOnlyNonElevated;
	CRegDWORD		m_regDriveMaskRemovable;
	CRegDWORD		m_regDriveMaskFloppy;
	CRegDWORD		m_regDriveMaskRemote;
	CRegDWORD		m_regDriveMaskFixed;
	CRegDWORD		m_regDriveMaskCDROM;
	CRegDWORD		m_regDriveMaskRAM;
	CRegDWORD		m_regDriveMaskUnknown;
	CRegDWORD		m_regUnversionedAsModified;
	CRegDWORD		m_regRecurseSubmodules;
	CRegDWORD		m_regShowExcludedAsNormal;
	CRegString		m_regExcludePaths;
	CString			m_sExcludePaths;
	CRegString		m_regIncludePaths;
	CString			m_sIncludePaths;
	CRegDWORD		m_regCacheType;
	DWORD			m_dwCacheType;
};
