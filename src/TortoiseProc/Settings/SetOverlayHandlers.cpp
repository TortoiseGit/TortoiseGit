// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetOverlayHandlers.h"


IMPLEMENT_DYNAMIC(CSetOverlayHandlers, ISettingsPropPage)
CSetOverlayHandlers::CSetOverlayHandlers()
	: ISettingsPropPage(CSetOverlayHandlers::IDD)
	, m_bShowIgnoredOverlay(TRUE)
	, m_bShowUnversionedOverlay(TRUE)
	, m_bShowAddedOverlay(TRUE)
	, m_bShowLockedOverlay(TRUE)
	, m_bShowReadonlyOverlay(TRUE)
	, m_bShowDeletedOverlay(TRUE)
{
	m_regShowIgnoredOverlay     = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowIgnoredOverlay"), TRUE);
	m_regShowUnversionedOverlay = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowUnversionedOverlay"), TRUE);
	m_regShowAddedOverlay       = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowAddedOverlay"), TRUE);
	m_regShowLockedOverlay      = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowLockedOverlay"), TRUE);
	m_regShowReadonlyOverlay    = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowReadonlyOverlay"), TRUE);
	m_regShowDeletedOverlay     = CRegDWORD(_T("Software\\TortoiseOverlays\\ShowDeletedOverlay"), TRUE);

	m_bShowIgnoredOverlay       = m_regShowIgnoredOverlay;
	m_bShowUnversionedOverlay   = m_regShowUnversionedOverlay;
	m_bShowAddedOverlay         = m_regShowAddedOverlay;
	m_bShowLockedOverlay        = m_regShowLockedOverlay;
	m_bShowReadonlyOverlay      = m_regShowReadonlyOverlay;
	m_bShowDeletedOverlay       = m_regShowDeletedOverlay;
}

CSetOverlayHandlers::~CSetOverlayHandlers()
{
}

void CSetOverlayHandlers::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SHOWIGNOREDOVERLAY, m_bShowIgnoredOverlay);
	DDX_Check(pDX, IDC_SHOWUNVERSIONEDOVERLAY, m_bShowUnversionedOverlay);
	DDX_Check(pDX, IDC_SHOWADDEDOVERLAY, m_bShowAddedOverlay);
	DDX_Check(pDX, IDC_SHOWLOCKEDOVERLAY, m_bShowLockedOverlay);
	DDX_Check(pDX, IDC_SHOWREADONLYOVERLAY, m_bShowReadonlyOverlay);
	DDX_Check(pDX, IDC_SHOWDELETEDOVERLAY, m_bShowDeletedOverlay);
}

BEGIN_MESSAGE_MAP(CSetOverlayHandlers, ISettingsPropPage)
	ON_BN_CLICKED(IDC_SHOWIGNOREDOVERLAY, &CSetOverlayHandlers::OnChange)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONEDOVERLAY, &CSetOverlayHandlers::OnChange)
	ON_BN_CLICKED(IDC_SHOWADDEDOVERLAY, &CSetOverlayHandlers::OnChange)
	ON_BN_CLICKED(IDC_SHOWLOCKEDOVERLAY, &CSetOverlayHandlers::OnChange)
	ON_BN_CLICKED(IDC_SHOWREADONLYOVERLAY, &CSetOverlayHandlers::OnChange)
	ON_BN_CLICKED(IDC_SHOWDELETEDOVERLAY, &CSetOverlayHandlers::OnChange)
END_MESSAGE_MAP()

BOOL CSetOverlayHandlers::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	UpdateData(FALSE);

	return TRUE;
}

void CSetOverlayHandlers::OnChange()
{
	SetModified();
}

BOOL CSetOverlayHandlers::OnApply()
{
	UpdateData();

	if (DWORD(m_regShowIgnoredOverlay) != DWORD(m_bShowIgnoredOverlay))
		m_restart = Restart_System;
	Store (m_bShowIgnoredOverlay, m_regShowIgnoredOverlay);

	if (DWORD(m_regShowUnversionedOverlay) != DWORD(m_bShowUnversionedOverlay))
		m_restart = Restart_System;
	Store (m_bShowUnversionedOverlay, m_regShowUnversionedOverlay);

	if (DWORD(m_regShowAddedOverlay) != DWORD(m_bShowAddedOverlay))
		m_restart = Restart_System;
	Store (m_bShowAddedOverlay, m_regShowAddedOverlay);

	if (DWORD(m_regShowLockedOverlay) != DWORD(m_bShowLockedOverlay))
		m_restart = Restart_System;
	Store (m_bShowLockedOverlay, m_regShowLockedOverlay);

	if (DWORD(m_regShowReadonlyOverlay) != DWORD(m_bShowReadonlyOverlay))
		m_restart = Restart_System;
	Store (m_bShowReadonlyOverlay, m_regShowReadonlyOverlay);

	if (DWORD(m_regShowDeletedOverlay) != DWORD(m_bShowDeletedOverlay))
		m_restart = Restart_System;
	Store (m_bShowDeletedOverlay, m_regShowDeletedOverlay);


	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
