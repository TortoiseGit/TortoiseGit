// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2014-2016, 2019 - TortoiseGit
// Copyright (C) 2010, 2012, 2014-2016 - TortoiseSVN

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
	m_regShowIgnoredOverlay     = CRegDWORD(L"Software\\TortoiseOverlays\\ShowIgnoredOverlay", TRUE);
	m_regShowUnversionedOverlay = CRegDWORD(L"Software\\TortoiseOverlays\\ShowUnversionedOverlay", TRUE);
	m_regShowAddedOverlay       = CRegDWORD(L"Software\\TortoiseOverlays\\ShowAddedOverlay", TRUE);
	m_regShowLockedOverlay      = CRegDWORD(L"Software\\TortoiseOverlays\\ShowLockedOverlay", TRUE);
	m_regShowReadonlyOverlay    = CRegDWORD(L"Software\\TortoiseOverlays\\ShowReadonlyOverlay", TRUE);
	m_regShowDeletedOverlay     = CRegDWORD(L"Software\\TortoiseOverlays\\ShowDeletedOverlay", TRUE);

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
	ON_BN_CLICKED(IDC_REGEDT, &CSetOverlayHandlers::OnBnClickedRegedt)
END_MESSAGE_MAP()

BOOL CSetOverlayHandlers::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SHOWIGNOREDOVERLAY);
	AdjustControlSize(IDC_SHOWUNVERSIONEDOVERLAY);
	AdjustControlSize(IDC_SHOWADDEDOVERLAY);
	AdjustControlSize(IDC_SHOWLOCKEDOVERLAY);
	AdjustControlSize(IDC_SHOWREADONLYOVERLAY);
	AdjustControlSize(IDC_SHOWDELETEDOVERLAY);

	UpdateInfoLabel();

	UpdateData(FALSE);

	return TRUE;
}

void CSetOverlayHandlers::OnChange()
{
	UpdateInfoLabel();
	SetModified();
}

BOOL CSetOverlayHandlers::OnApply()
{
	UpdateData();

	if (DWORD(m_regShowIgnoredOverlay) != DWORD(m_bShowIgnoredOverlay))
		m_restart = Restart_System;
	Store(m_bShowIgnoredOverlay, m_regShowIgnoredOverlay);

	if (DWORD(m_regShowUnversionedOverlay) != DWORD(m_bShowUnversionedOverlay))
		m_restart = Restart_System;
	Store(m_bShowUnversionedOverlay, m_regShowUnversionedOverlay);

	if (DWORD(m_regShowAddedOverlay) != DWORD(m_bShowAddedOverlay))
		m_restart = Restart_System;
	Store(m_bShowAddedOverlay, m_regShowAddedOverlay);

	if (DWORD(m_regShowLockedOverlay) != DWORD(m_bShowLockedOverlay))
		m_restart = Restart_System;
	Store(m_bShowLockedOverlay, m_regShowLockedOverlay);

	if (DWORD(m_regShowReadonlyOverlay) != DWORD(m_bShowReadonlyOverlay))
		m_restart = Restart_System;
	Store(m_bShowReadonlyOverlay, m_regShowReadonlyOverlay);

	if (DWORD(m_regShowDeletedOverlay) != DWORD(m_bShowDeletedOverlay))
		m_restart = Restart_System;
	Store(m_bShowDeletedOverlay, m_regShowDeletedOverlay);


	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

int CSetOverlayHandlers::GetInstalledOverlays()
{
	// if there are more than 12 overlay handlers installed, then that means not all
	// of the overlay handlers can be shown. Windows chooses the ones first
	// returned by RegEnumKeyEx() and just drops the ones that come last in
	// that enumeration.
	int nInstalledOverlayhandlers = 0;
	// scan the registry for installed overlay handlers
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers",
		0, KEY_ENUMERATE_SUB_KEYS, &hKey)==ERROR_SUCCESS)
	{
		TCHAR value[2048] = { 0 };
		TCHAR keystring[2048] = { 0 };
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
		{
			DWORD size = _countof(value);
			FILETIME last_write_time;
			rc = RegEnumKeyEx(hKey, i, value, &size, nullptr, nullptr, nullptr, &last_write_time);
			if (rc == ERROR_SUCCESS)
			{
				for (int j = 0; value[j]; ++j)
					value[j] = static_cast<wchar_t>(towlower(value[j]));
				if (!wcsstr(&value[0], L"tortoise"))
				{
					// check if there's a 'default' entry with a guid
					wcscpy_s(keystring, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\");
					wcscat_s(keystring, value);
					DWORD dwType = 0;
					DWORD dwSize = _countof(value); // the API docs only specify "The size of the destination data buffer",
					// but better be safe than sorry using _countof instead of sizeof
					if (SHGetValue(HKEY_LOCAL_MACHINE,
						keystring,
						nullptr,
						&dwType, value, &dwSize) == ERROR_SUCCESS)
					{
						if ((dwSize > 10)&&(value[0] == '{'))
							nInstalledOverlayhandlers++;
					}
				}
			}
		}
		RegCloseKey(hKey);
	}
	return nInstalledOverlayhandlers;
}

void CSetOverlayHandlers::UpdateInfoLabel()
{
	CString sUnversioned, sNeedslock, sIgnored, sLocked;
	GetDlgItemText(IDC_SHOWUNVERSIONEDOVERLAY, sUnversioned);
	GetDlgItemText(IDC_SHOWREADONLYOVERLAY, sNeedslock);
	GetDlgItemText(IDC_SHOWIGNOREDOVERLAY, sIgnored);
	GetDlgItemText(IDC_SHOWLOCKEDOVERLAY, sLocked);

	int nInstalledOverlays = GetInstalledOverlays();
	CString sInfo;
	sInfo.Format(IDS_SETTINGS_OVERLAYINFO, nInstalledOverlays);

	const int nOverlayLimit = 3;
	// max     registered: drop the locked overlay
	// max + 1 registered: drop the locked and the ignored overlay
	// max + 2 registered: drop the locked, ignored and readonly overlay
	// max + 3 or more registered: drop the locked, ignored, readonly and unversioned overlay
	CString sInfo2;
	if (nInstalledOverlays > nOverlayLimit+3)
		sInfo2 += L", " + sUnversioned;
	if (nInstalledOverlays > nOverlayLimit+2)
		sInfo2 += L", " + sNeedslock;
	if (nInstalledOverlays > nOverlayLimit+1)
		sInfo2 += L", " + sIgnored;
	if (nInstalledOverlays > nOverlayLimit)
		sInfo2 += L", " + sLocked;
	sInfo2.Trim(L" ,");

	if (!sInfo2.IsEmpty())
	{
		sInfo += L'\n';
		sInfo.AppendFormat(IDS_SETTINGS_OVERLAYINFO2, static_cast<LPCTSTR>(sInfo2));
	}
	SetDlgItemText(IDC_HANDLERHINT, sInfo);
}

void CSetOverlayHandlers::OnBnClickedRegedt()
{
	CComHeapPtr<WCHAR> pszPath;
	if (SHGetKnownFolderPath(FOLDERID_Windows, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
	{
		CString path = pszPath;
		path += L"\\regedit.exe";

		// regedit stores the key it showed last in
		// HKEY_Current_User\Software\Microsoft\Windows\CurrentVersion\Applets\Regedit\LastKey
		// we set that here to
		// HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers
		// so when we start regedit, it will show that key on start
		CRegString regLastKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\LastKey");
		regLastKey = L"HKEY_Local_Machine\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers";

		SHELLEXECUTEINFO si = { sizeof(SHELLEXECUTEINFO) };
		si.hwnd = GetSafeHwnd();
		si.lpVerb = L"open";
		si.lpFile = path;
		si.nShow = SW_SHOW;
		ShellExecuteEx(&si);
	}
}
