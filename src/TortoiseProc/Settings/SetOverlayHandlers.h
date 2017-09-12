// TortoiseGit - a Windows shell extension for easy	version	control

// Copyright (C) 2010, 2012 - TortoiseSVN

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
class CSetOverlayHandlers : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetOverlayHandlers)

public:
	CSetOverlayHandlers();
	virtual ~CSetOverlayHandlers();

	UINT GetIconID() override { return IDI_SET_OVERLAYS; }

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAYHANDLERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnChange();
	afx_msg void OnBnClickedRegedt();

	DECLARE_MESSAGE_MAP()

	int				GetInstalledOverlays();
	void			UpdateInfoLabel();

private:
	BOOL			m_bShowIgnoredOverlay;
	BOOL			m_bShowUnversionedOverlay;
	BOOL			m_bShowAddedOverlay;
	BOOL			m_bShowLockedOverlay;
	BOOL			m_bShowReadonlyOverlay;
	BOOL			m_bShowDeletedOverlay;
	CRegDWORD		m_regShowIgnoredOverlay;
	CRegDWORD		m_regShowUnversionedOverlay;
	CRegDWORD		m_regShowAddedOverlay;
	CRegDWORD		m_regShowLockedOverlay;
	CRegDWORD		m_regShowReadonlyOverlay;
	CRegDWORD		m_regShowDeletedOverlay;
};
