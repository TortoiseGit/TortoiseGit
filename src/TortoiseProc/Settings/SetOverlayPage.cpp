// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009,2011,2013-2017 - TortoiseGit
// Copyright (C) 2003-2008, 2011, 2017 - TortoiseSVN

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
#include "SetOverlayPage.h"
#include "SetOverlayIcons.h"
#include "Globals.h"
#include "../TGitCache/CacheInterface.h"
#include "StringUtils.h"

IMPLEMENT_DYNAMIC(CSetOverlayPage, ISettingsPropPage)
CSetOverlayPage::CSetOverlayPage()
	: ISettingsPropPage(CSetOverlayPage::IDD)
	, m_bRemovable(FALSE)
	, m_bNetwork(FALSE)
	, m_bFixed(FALSE)
	, m_bCDROM(FALSE)
	, m_bRAM(FALSE)
	, m_bUnknown(FALSE)
	, m_bOnlyExplorer(FALSE)
	, m_bOnlyNonElevated(FALSE)
	, m_bUnversionedAsModified(FALSE)
	, m_bRecurseSubmodules(FALSE)
	, m_bFloppy(FALSE)
	, m_bShowExcludedAsNormal(TRUE)
{
	m_regOnlyExplorer = CRegDWORD(L"Software\\TortoiseGit\\LoadDllOnlyInExplorer", FALSE);
	m_regOnlyNonElevated = CRegDWORD(L"Software\\TortoiseGit\\ShowOverlaysOnlyNonElevated", FALSE);
	m_regDriveMaskRemovable = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskRemovable");
	m_regDriveMaskFloppy = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskFloppy");
	m_regDriveMaskRemote = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskRemote");
	m_regDriveMaskFixed = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskFixed", TRUE);
	m_regDriveMaskCDROM = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskCDROM");
	m_regDriveMaskRAM = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskRAM");
	m_regDriveMaskUnknown = CRegDWORD(L"Software\\TortoiseGit\\DriveMaskUnknown");
	m_regExcludePaths = CRegString(L"Software\\TortoiseGit\\OverlayExcludeList");
	m_regIncludePaths = CRegString(L"Software\\TortoiseGit\\OverlayIncludeList");
	m_regCacheType = CRegDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? 2 : 1);
	m_regUnversionedAsModified = CRegDWORD(L"Software\\TortoiseGit\\UnversionedAsModified", FALSE);
	m_regRecurseSubmodules = CRegDWORD(L"Software\\TortoiseGit\\TGitCacheRecurseSubmodules", FALSE);
	m_regShowExcludedAsNormal = CRegDWORD(L"Software\\TortoiseGit\\ShowExcludedAsNormal", TRUE);

	m_bOnlyExplorer = m_regOnlyExplorer;
	m_bOnlyNonElevated = m_regOnlyNonElevated;
	m_bRemovable = m_regDriveMaskRemovable;
	m_bFloppy = m_regDriveMaskFloppy;
	m_bNetwork = m_regDriveMaskRemote;
	m_bFixed = m_regDriveMaskFixed;
	m_bCDROM = m_regDriveMaskCDROM;
	m_bRAM = m_regDriveMaskRAM;
	m_bUnknown = m_regDriveMaskUnknown;
	m_bUnversionedAsModified = m_regUnversionedAsModified;
	m_bRecurseSubmodules = m_regRecurseSubmodules;
	m_bShowExcludedAsNormal = m_regShowExcludedAsNormal;
	m_sExcludePaths = m_regExcludePaths;
	m_sExcludePaths.Replace(L"\n", L"\r\n");
	m_sIncludePaths = m_regIncludePaths;
	m_sIncludePaths.Replace(L"\n", L"\r\n");
	m_dwCacheType = m_regCacheType;
}

CSetOverlayPage::~CSetOverlayPage()
{
}

void CSetOverlayPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_REMOVABLE, m_bRemovable);
	DDX_Check(pDX, IDC_NETWORK, m_bNetwork);
	DDX_Check(pDX, IDC_FIXED, m_bFixed);
	DDX_Check(pDX, IDC_CDROM, m_bCDROM);
	DDX_Check(pDX, IDC_RAM, m_bRAM);
	DDX_Check(pDX, IDC_UNKNOWN, m_bUnknown);
	DDX_Check(pDX, IDC_ONLYEXPLORER, m_bOnlyExplorer);
	DDX_Check(pDX, IDC_ONLYNONELEVATED, m_bOnlyNonElevated);
	DDX_Text(pDX, IDC_EXCLUDEPATHS, m_sExcludePaths);
	DDX_Text(pDX, IDC_INCLUDEPATHS, m_sIncludePaths);
	DDX_Check(pDX, IDC_UNVERSIONEDASMODIFIED, m_bUnversionedAsModified);
	DDX_Check(pDX, IDC_RECURSIVESUBMODULES, m_bRecurseSubmodules);
	DDX_Check(pDX, IDC_FLOPPY, m_bFloppy);
	DDX_Check(pDX, IDC_SHOWEXCLUDEDASNORMAL, m_bShowExcludedAsNormal);
}

BEGIN_MESSAGE_MAP(CSetOverlayPage, ISettingsPropPage)
	ON_BN_CLICKED(IDC_REMOVABLE, OnChange)
	ON_BN_CLICKED(IDC_FLOPPY, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_NETWORK, OnChange)
	ON_BN_CLICKED(IDC_FIXED, OnChange)
	ON_BN_CLICKED(IDC_CDROM, OnChange)
	ON_BN_CLICKED(IDC_UNKNOWN, OnChange)
	ON_BN_CLICKED(IDC_RAM, OnChange)
	ON_BN_CLICKED(IDC_ONLYEXPLORER, OnChange)
	ON_BN_CLICKED(IDC_ONLYNONELEVATED, &CSetOverlayPage::OnChange)
	ON_EN_CHANGE(IDC_EXCLUDEPATHS, OnChange)
	ON_EN_CHANGE(IDC_INCLUDEPATHS, OnChange)
	ON_BN_CLICKED(IDC_CACHEDEFAULT, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHESHELL, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHESHELL2, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHENONE, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_UNVERSIONEDASMODIFIED, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_RECURSIVESUBMODULES, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_SHOWEXCLUDEDASNORMAL, &CSetOverlayPage::OnChange)
END_MESSAGE_MAP()

BOOL CSetOverlayPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_REMOVABLE);
	AdjustControlSize(IDC_NETWORK);
	AdjustControlSize(IDC_FIXED);
	AdjustControlSize(IDC_CDROM);
	AdjustControlSize(IDC_RAM);
	AdjustControlSize(IDC_UNKNOWN);
	AdjustControlSize(IDC_ONLYEXPLORER);
	AdjustControlSize(IDC_UNVERSIONEDASMODIFIED);
	AdjustControlSize(IDC_RECURSIVESUBMODULES);
	AdjustControlSize(IDC_FLOPPY);
	AdjustControlSize(IDC_CACHEDEFAULT);
	AdjustControlSize(IDC_CACHENONE);
	AdjustControlSize(IDC_CACHESHELL);
	AdjustControlSize(IDC_CACHESHELL2);

	switch (m_dwCacheType)
	{
	case 0:
		CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHENONE);
		break;
	default:
	case 1:
		CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHEDEFAULT);
		break;
	case 2:
		CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHESHELL);
		break;
	case 3:
		CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHESHELL2);
		break;
	}
	GetDlgItem(IDC_UNVERSIONEDASMODIFIED)->EnableWindow(m_dwCacheType == 1);
	GetDlgItem(IDC_RECURSIVESUBMODULES)->EnableWindow(m_dwCacheType == 1);

	m_tooltips.AddTool(IDC_ONLYEXPLORER, IDS_SETTINGS_ONLYEXPLORER_TT);
	m_tooltips.AddTool(IDC_ONLYNONELEVATED, IDS_SETTINGS_ONLYNONELEVATED_TT);
	m_tooltips.AddTool(IDC_EXCLUDEPATHS, IDS_SETTINGS_EXCLUDELIST_TT);
	m_tooltips.AddTool(IDC_INCLUDEPATHS, IDS_SETTINGS_INCLUDELIST_TT);
	m_tooltips.AddTool(IDC_CACHEDEFAULT, IDS_SETTINGS_CACHEDEFAULT_TT);
	m_tooltips.AddTool(IDC_CACHESHELL, IDS_SETTINGS_CACHESHELL_TT);
	m_tooltips.AddTool(IDC_CACHESHELL2, IDS_SETTINGS_CACHESHELLEXT_TT);
	m_tooltips.AddTool(IDC_CACHENONE, IDS_SETTINGS_CACHENONE_TT);
	m_tooltips.AddTool(IDC_UNVERSIONEDASMODIFIED, IDS_SETTINGS_UNVERSIONEDASMODIFIED_TT);
	m_tooltips.AddTool(IDC_SHOWEXCLUDEDASNORMAL, IDS_SETTINGS_SHOWEXCLUDEDASNORMAL_TT);

	UpdateData(FALSE);

	return TRUE;
}

void CSetOverlayPage::OnChange()
{
	UpdateData();
	int id = GetCheckedRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE);
	switch (id)
	{
	default:
	case IDC_CACHEDEFAULT:
		m_dwCacheType = 1;
		break;
	case IDC_CACHESHELL:
		m_dwCacheType = 2;
		break;
	case IDC_CACHESHELL2:
		m_dwCacheType = 3;
		break;
	case IDC_CACHENONE:
		m_dwCacheType = 0;
		break;
	}
	GetDlgItem(IDC_UNVERSIONEDASMODIFIED)->EnableWindow(m_dwCacheType == 1);
	GetDlgItem(IDC_RECURSIVESUBMODULES)->EnableWindow(m_dwCacheType == 1);
	SetModified();
}

BOOL CSetOverlayPage::OnApply()
{
	UpdateData();
	Store(m_bOnlyExplorer, m_regOnlyExplorer);
	Store(m_bOnlyNonElevated, m_regOnlyNonElevated);
	if (DWORD(m_regDriveMaskRemovable) != DWORD(m_bRemovable))
		m_restart = Restart_Cache;
	Store(m_bRemovable, m_regDriveMaskRemovable);

	if (DWORD(m_regDriveMaskFloppy) != DWORD(m_bFloppy))
		m_restart = Restart_Cache;
	Store(m_bFloppy, m_regDriveMaskFloppy);

	if (DWORD(m_regDriveMaskRemote) != DWORD(m_bNetwork))
		m_restart = Restart_Cache;
	Store(m_bNetwork, m_regDriveMaskRemote);

	if (DWORD(m_regDriveMaskFixed) != DWORD(m_bFixed))
		m_restart = Restart_Cache;
	Store(m_bFixed, m_regDriveMaskFixed);

	if (DWORD(m_regDriveMaskCDROM) != DWORD(m_bCDROM))
		m_restart = Restart_Cache;
	Store(m_bCDROM, m_regDriveMaskCDROM);

	if (DWORD(m_regDriveMaskRAM) != DWORD(m_bRAM))
		m_restart = Restart_Cache;
	Store(m_bRAM, m_regDriveMaskRAM);

	if (DWORD(m_regDriveMaskUnknown) != DWORD(m_bUnknown))
		m_restart = Restart_Cache;
	Store(m_bUnknown, m_regDriveMaskUnknown);

	if (m_sExcludePaths.Compare(CString(m_regExcludePaths)))
		m_restart = Restart_Cache;
	m_sExcludePaths.Remove(L'\r');
	if (!CStringUtils::EndsWith(m_sExcludePaths, L'\n'))
		m_sExcludePaths += L'\n';
	Store(m_sExcludePaths, m_regExcludePaths);
	m_sExcludePaths.Replace(L"\n", L"\r\n");
	m_sIncludePaths.Remove(L'\r');
	if (!CStringUtils::EndsWith(m_sIncludePaths, L'\n'))
		m_sIncludePaths += L'\n';
	if (m_sIncludePaths.Compare(CString(m_regIncludePaths)))
		m_restart = Restart_Cache;
	Store(m_sIncludePaths, m_regIncludePaths);
	m_sIncludePaths.Replace(L"\n", L"\r\n");

	if (DWORD(m_regUnversionedAsModified) != DWORD(m_bUnversionedAsModified))
		m_restart = Restart_Cache;
	Store(m_bUnversionedAsModified, m_regUnversionedAsModified);
	if (DWORD(m_regRecurseSubmodules) != DWORD(m_bRecurseSubmodules))
		m_restart = Restart_Cache;
	Store(m_bRecurseSubmodules, m_regRecurseSubmodules);
	if (DWORD(m_regShowExcludedAsNormal) != DWORD(m_bShowExcludedAsNormal))
		m_restart = Restart_Cache;
	Store(m_bShowExcludedAsNormal, m_regShowExcludedAsNormal);

	Store(m_dwCacheType, m_regCacheType);
	if (m_dwCacheType != 1)
	{
		// close the possible running cache process
		HWND hWnd = ::FindWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME);
		if (hWnd)
			::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
		m_restart = Restart_None;
	}
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
