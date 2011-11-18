// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2011 - TortoiseSVN

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
#include "ShellUpdater.h"
#include "..\TGitCache\CacheInterface.h"
#include ".\setoverlaypage.h"
#include "MessageBox.h"


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
	, m_sExcludePaths(_T(""))
	, m_sIncludePaths(_T(""))
	, m_bUnversionedAsModified(FALSE)
	, m_bFloppy(FALSE)
	, m_bShowExcludedAsNormal(TRUE)
{
	m_regOnlyExplorer = CRegDWORD(_T("Software\\TortoiseGit\\LoadDllOnlyInExplorer"), FALSE);
	m_regDriveMaskRemovable = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskRemovable"));
	m_regDriveMaskFloppy = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskFloppy"));
	m_regDriveMaskRemote = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskRemote"));
	m_regDriveMaskFixed = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskFixed"), TRUE);
	m_regDriveMaskCDROM = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskCDROM"));
	m_regDriveMaskRAM = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskRAM"));
	m_regDriveMaskUnknown = CRegDWORD(_T("Software\\TortoiseGit\\DriveMaskUnknown"));
	m_regExcludePaths = CRegString(_T("Software\\TortoiseGit\\OverlayExcludeList"));
	m_regIncludePaths = CRegString(_T("Software\\TortoiseGit\\OverlayIncludeList"));
	m_regCacheType = CRegDWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? 2 : 1);
	m_regUnversionedAsModified = CRegDWORD(_T("Software\\TortoiseGit\\UnversionedAsModified"), FALSE);
	m_regShowExcludedAsNormal = CRegDWORD(_T("Software\\TortoiseGit\\ShowExcludedAsNormal"), TRUE);

	m_bOnlyExplorer = m_regOnlyExplorer;
	m_bRemovable = m_regDriveMaskRemovable;
	m_bFloppy = m_regDriveMaskFloppy;
	m_bNetwork = m_regDriveMaskRemote;
	m_bFixed = m_regDriveMaskFixed;
	m_bCDROM = m_regDriveMaskCDROM;
	m_bRAM = m_regDriveMaskRAM;
	m_bUnknown = m_regDriveMaskUnknown;
	m_bUnversionedAsModified = m_regUnversionedAsModified;
	m_bShowExcludedAsNormal = m_regShowExcludedAsNormal;
	m_sExcludePaths = m_regExcludePaths;
	m_sExcludePaths.Replace(_T("\n"), _T("\r\n"));
	m_sIncludePaths = m_regIncludePaths;
	m_sIncludePaths.Replace(_T("\n"), _T("\r\n"));
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
	DDX_Text(pDX, IDC_EXCLUDEPATHS, m_sExcludePaths);
	DDX_Text(pDX, IDC_INCLUDEPATHS, m_sIncludePaths);
	DDX_Check(pDX, IDC_UNVERSIONEDASMODIFIED, m_bUnversionedAsModified);
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
	ON_EN_CHANGE(IDC_EXCLUDEPATHS, OnChange)
	ON_EN_CHANGE(IDC_INCLUDEPATHS, OnChange)
	ON_BN_CLICKED(IDC_CACHEDEFAULT, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHESHELL, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHESHELL2, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_CACHENONE, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_UNVERSIONEDASMODIFIED, &CSetOverlayPage::OnChange)
	ON_BN_CLICKED(IDC_SHOWEXCLUDEDASNORMAL, &CSetOverlayPage::OnChange)
END_MESSAGE_MAP()

BOOL CSetOverlayPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

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
	GetDlgItem(IDC_FLOPPY)->EnableWindow(m_bRemovable);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_ONLYEXPLORER, IDS_SETTINGS_ONLYEXPLORER_TT);
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

BOOL CSetOverlayPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
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
	GetDlgItem(IDC_FLOPPY)->EnableWindow(m_bRemovable);
	SetModified();
}

BOOL CSetOverlayPage::OnApply()
{
	UpdateData();
	Store (m_bOnlyExplorer, m_regOnlyExplorer);
	if (DWORD(m_regDriveMaskRemovable) != DWORD(m_bRemovable))
		m_restart = Restart_Cache;
	Store (m_bRemovable, m_regDriveMaskRemovable);

	if (DWORD(m_regDriveMaskFloppy) != DWORD(m_bFloppy))
		m_restart = Restart_Cache;
	Store (m_bFloppy, m_regDriveMaskFloppy);

	if (DWORD(m_regDriveMaskRemote) != DWORD(m_bNetwork))
		m_restart = Restart_Cache;
	Store (m_bNetwork, m_regDriveMaskRemote);

	if (DWORD(m_regDriveMaskFixed) != DWORD(m_bFixed))
		m_restart = Restart_Cache;
	Store (m_bFixed, m_regDriveMaskFixed);

	if (DWORD(m_regDriveMaskCDROM) != DWORD(m_bCDROM))
		m_restart = Restart_Cache;
	Store (m_bCDROM, m_regDriveMaskCDROM);

	if (DWORD(m_regDriveMaskRAM) != DWORD(m_bRAM))
		m_restart = Restart_Cache;
	Store (m_bRAM, m_regDriveMaskRAM);

	if (DWORD(m_regDriveMaskUnknown) != DWORD(m_bUnknown))
		m_restart = Restart_Cache;
	Store (m_bUnknown, m_regDriveMaskUnknown);

	if (m_sExcludePaths.Compare(CString(m_regExcludePaths)))
		m_restart = Restart_Cache;
	m_sExcludePaths.Remove('\r');
	if (m_sExcludePaths.Right(1).Compare(_T("\n"))!=0)
		m_sExcludePaths += _T("\n");
	Store (m_sExcludePaths, m_regExcludePaths);
	m_sExcludePaths.Replace(_T("\n"), _T("\r\n"));
	m_sIncludePaths.Remove('\r');
	if (m_sIncludePaths.Right(1).Compare(_T("\n"))!=0)
		m_sIncludePaths += _T("\n");
	if (m_sIncludePaths.Compare(CString(m_regIncludePaths)))
		m_restart = Restart_Cache;
	Store (m_sIncludePaths, m_regIncludePaths);
	m_sIncludePaths.Replace(_T("\n"), _T("\r\n"));

	if (DWORD(m_regUnversionedAsModified) != DWORD(m_bUnversionedAsModified))
		m_restart = Restart_Cache;
	Store (m_bUnversionedAsModified, m_regUnversionedAsModified);
	if (DWORD(m_regShowExcludedAsNormal) != DWORD(m_bShowExcludedAsNormal))
		m_restart = Restart_Cache;
	Store (m_bShowExcludedAsNormal, m_regShowExcludedAsNormal);

	Store (m_dwCacheType, m_regCacheType);
	if (m_dwCacheType != 1)
	{
		// close the possible running cache process
		HWND hWnd = ::FindWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME);
		if (hWnd)
		{
			::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
		}
		m_restart = Restart_None;
	}
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
