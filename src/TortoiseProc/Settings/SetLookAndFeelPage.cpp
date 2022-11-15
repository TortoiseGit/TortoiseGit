// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2019, 2021-2023 - TortoiseGit
// Copyright (C) 2003-2008, 2011, 2014 - TortoiseSVN

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
#include "Globals.h"
#include "ShellUpdater.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "SetLookAndFeelPage.h"
#include "MenuInfo.h"
#include "ShellCache.h"
#include "PathUtils.h"

#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;

extern MenuInfo menuInfo[];

void InsertMenuItemToList(CListCtrl *list,CImageList *imagelist)
{
	int i=0;
	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	while(menuInfo[i].command != ShellMenuLastEntry)
	{
		if ((menuInfo[i].command != ShellSeparator &&
		   menuInfo[i].command != ShellSubMenu &&
		   menuInfo[i].command != ShellSubMenuFile &&
		   menuInfo[i].command != ShellSubMenuFolder &&
		   menuInfo[i].command != ShellSubMenuLink &&
		   menuInfo[i].command != ShellMenuMergeAbort &&
		   menuInfo[i].command != ShellSubMenuMultiple) &&
		   (i == 0 || menuInfo[i - 1].menuID != menuInfo[i].menuID))
		{
			auto hIcon = CAppUtils::LoadIconEx(menuInfo[i].iconID, iconWidth, iconHeight);

			int nImage = imagelist -> Add(hIcon);

			CString temp;
			temp.LoadString(menuInfo[i].menuTextID);
			CStringUtils::RemoveAccelerators(temp);

			int nIndex = list->GetItemCount();
			list->InsertItem(nIndex,temp,nImage);
			list->SetItemData(nIndex,i);
		}
		i++;
	}
}

static void SetMenuItemCheck(CListCtrl* list, unsigned __int64 mask, CButton* selectAll)
{
	bool allChecked = true;
	for(int i=0;i<list->GetItemCount();i++)
	{
		int data = static_cast<int>(list->GetItemData(i));

		list->SetCheck(i,(menuInfo[data].menuID & mask) == menuInfo[data].menuID);
		if (!((menuInfo[data].menuID & mask) == menuInfo[data].menuID))
			allChecked = false;
	}

	if (!mask)
		selectAll->SetCheck(BST_UNCHECKED);
	else if (allChecked)
		selectAll->SetCheck(BST_CHECKED);
	else
		selectAll->SetCheck(BST_INDETERMINATE);
}

inline static void SetMenuItemCheck(CListCtrl* list, ULARGE_INTEGER mask, CButton* selectAll)
{
	SetMenuItemCheck(list, mask.QuadPart, selectAll);
}

static unsigned __int64 GetMenuListMask(CListCtrl* list, CButton* selectAll = nullptr)
{
	unsigned __int64 mask = 0;
	bool allChecked = true;

	for(int i=0;i<list->GetItemCount();i++)
	{
		if(list->GetCheck(i))
		{
			int data = static_cast<int>(list->GetItemData(i));
			mask |= menuInfo[data].menuID ;
		}
		else
			allChecked = false;
	}
	if (!selectAll)
		return mask;

	if (!mask)
		selectAll->SetCheck(BST_UNCHECKED);
	else if (allChecked)
		selectAll->SetCheck(BST_CHECKED);
	else
		selectAll->SetCheck(BST_INDETERMINATE);
	return mask;
}

// Handles click on "select all" and returns the calculated mask
static unsigned __int64 ClickedSelectAll(CListCtrl *list, CButton *selectAll)
{
	UINT state = (selectAll->GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		selectAll->SetCheck(state);
	}

	theApp.DoWaitCursor(1);

	for (int i = 0; i < list->GetItemCount(); i++)
		list->SetCheck(i, state == BST_CHECKED);

	unsigned __int64 mask = GetMenuListMask(list);

	theApp.DoWaitCursor(-1);

	return mask;
}

IMPLEMENT_DYNAMIC(CSetLookAndFeelPage, ISettingsPropPage)
CSetLookAndFeelPage::CSetLookAndFeelPage()
	: ISettingsPropPage(CSetLookAndFeelPage::IDD)
	, m_bBlock(false)
	, m_bHideMenus(false)
{
	ShellCache cache;
	m_regTopmenu = cache.menulayoutlow;
	m_regTopmenuhigh = cache.menulayouthigh;

	m_topmenu.HighPart = m_regTopmenuhigh;
	m_topmenu.LowPart = m_regTopmenu;

	m_regHideMenus = CRegDWORD(L"Software\\TortoiseGit\\HideMenusForUnversionedItems", FALSE);
	m_bHideMenus = m_regHideMenus;

	m_regNoContextPaths = CRegString(L"Software\\TortoiseGit\\NoContextPaths", L"");
	m_sNoContextPaths = m_regNoContextPaths;
	m_sNoContextPaths.Replace(L"\n", L"\r\n");

	m_regEnableDragContextMenu = CRegDWORD(L"Software\\TortoiseGit\\EnableDragContextMenu", TRUE);
	m_bEnableDragContextMenu = m_regEnableDragContextMenu;
}

CSetLookAndFeelPage::~CSetLookAndFeelPage()
{
}

void CSetLookAndFeelPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
	DDX_Check(pDX, IDC_HIDEMENUS, m_bHideMenus);
	DDX_Text(pDX, IDC_NOCONTEXTPATHS, m_sNoContextPaths);
	DDX_Check(pDX, IDC_ENABLEDRAGCONTEXTMENU, m_bEnableDragContextMenu);
}


BEGIN_MESSAGE_MAP(CSetLookAndFeelPage, ISettingsPropPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestoreDefaults)
	ON_BN_CLICKED(IDC_HIDEMENUS, OnChange)
	ON_EN_CHANGE(IDC_NOCONTEXTPATHS, &CSetLookAndFeelPage::OnEnChangeNocontextpaths)
	ON_BN_CLICKED(IDC_ENABLEDRAGCONTEXTMENU, OnChange)
END_MESSAGE_MAP()


BOOL CSetLookAndFeelPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SELECTALL);
	AdjustControlSize(IDC_HIDEMENUS);
	AdjustControlSize(IDC_ENABLEDRAGCONTEXTMENU);

	m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);
	m_tooltips.AddTool(IDC_HIDEMENUS, IDS_SETTINGS_HIDEMENUS_TT);
	m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);
	m_tooltips.AddTool(IDC_ENABLEDRAGCONTEXTMENU, IDS_SETTINGS_ENABLEDRAGCONTEXTMENU_TT);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | (CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = m_cMenuList.GetHeaderCtrl()->GetItemCount() - 1;
	while (c>=0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, L"");

	SetWindowTheme(m_hWnd, L"Explorer", nullptr);

	m_cMenuList.SetRedraw(false);

	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	m_imgList.Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 4, 1);

	m_bBlock = true;

	InsertMenuItemToList(&m_cMenuList,&m_imgList);
	SetMenuItemCheck(&m_cMenuList, m_topmenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));

	m_bBlock = false;

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	for (int col = 0, maxcol = m_cMenuList.GetHeaderCtrl()->GetItemCount(); col < maxcol; ++col)
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	m_cMenuList.SetRedraw(true);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetLookAndFeelPage::OnApply()
{
	UpdateData();

	m_regTopmenu = m_topmenu.LowPart;
	m_regTopmenuhigh = m_topmenu.HighPart;

	m_regTopmenu.getErrorString();
	m_sNoContextPaths.Remove('\r');
	if (!CStringUtils::EndsWith(m_sNoContextPaths, L'\n'))
		m_sNoContextPaths += L'\n';

	Store(m_bHideMenus, m_regHideMenus);
	Store(m_sNoContextPaths, m_regNoContextPaths);
	Store(m_bEnableDragContextMenu, m_regEnableDragContextMenu);

	m_sNoContextPaths.Replace(L"\n", L"\r\n");

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetLookAndFeelPage::OnBnClickedRestoreDefaults()
{
	SetModified(TRUE);
	m_topmenu.QuadPart = DEFAULTMENUTOPENTRIES;
	m_bBlock = true;
	SetMenuItemCheck(&m_cMenuList, m_topmenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetLookAndFeelPage::OnBnClickedSelectall()
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	m_bBlock = true;
	m_topmenu.QuadPart = ClickedSelectAll(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetLookAndFeelPage::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if (m_bBlock)
		return;
	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
		m_topmenu.QuadPart = GetMenuListMask(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	*pResult = 0;
}

void CSetLookAndFeelPage::OnChange()
{
	SetModified();
}

void CSetLookAndFeelPage::OnEnChangeNocontextpaths()
{
	SetModified();
}

// Set Extmenu class
#include "SetExtMenu.h"

IMPLEMENT_DYNAMIC(CSetExtMenu, ISettingsPropPage)
CSetExtMenu::CSetExtMenu()
	: ISettingsPropPage(CSetExtMenu::IDD)
{
	ShellCache shell;

	m_bBlock = false;

	m_regExtmenu = shell.menuextlow;
	m_regExtmenuhigh = shell.menuexthigh;

	m_extmenu.HighPart = m_regExtmenuhigh;
	m_extmenu.LowPart = m_regExtmenu;
}

CSetExtMenu::~CSetExtMenu()
{
}

void CSetExtMenu::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
}

BEGIN_MESSAGE_MAP(CSetExtMenu, ISettingsPropPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestoreDefaults)
END_MESSAGE_MAP()

BOOL CSetExtMenu::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SELECTALL);

	m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_EXTMENULAYOUT_TT);
	//m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | (CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = m_cMenuList.GetHeaderCtrl()->GetItemCount() - 1;
	while (c>=0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, L"");

	SetWindowTheme(m_cMenuList.GetSafeHwnd(), L"Explorer", nullptr);

	m_cMenuList.SetRedraw(false);

	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	m_imgList.Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 4, 1);

	m_bBlock = true;

	InsertMenuItemToList(&m_cMenuList,&m_imgList);
	SetMenuItemCheck(&m_cMenuList, m_extmenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));

	m_bBlock = false;

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	for (int col = 0, maxcol = m_cMenuList.GetHeaderCtrl()->GetItemCount(); col < maxcol; ++col)
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	m_cMenuList.SetRedraw(true);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetExtMenu::OnApply()
{
	UpdateData();

	m_regExtmenu = m_extmenu.LowPart;
	m_regExtmenuhigh = m_extmenu.HighPart;

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetExtMenu::OnBnClickedRestoreDefaults()
{
	SetModified(TRUE);
	m_extmenu.QuadPart = DEFAULTMENUEXTENTRIES;
	m_bBlock = true;
	SetMenuItemCheck(&m_cMenuList, m_extmenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetExtMenu::OnBnClickedSelectall()
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	m_bBlock = true;
	m_extmenu.QuadPart = ClickedSelectAll(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetExtMenu::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if( m_bBlock )
		return;

	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
		m_extmenu.QuadPart = GetMenuListMask(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	*pResult = 0;
}

void CSetExtMenu::OnChange()
{
	SetModified();
}

// Set Win11 top menu class
#include "SetWin11ContextMenu.h"

IMPLEMENT_DYNAMIC(CSetWin11ContextMenu, ISettingsPropPage)
CSetWin11ContextMenu::CSetWin11ContextMenu()
	: ISettingsPropPage(CSetWin11ContextMenu::IDD)
{
	ShellCache shell;

	m_regtopMenu = shell.menuLayout11;
	m_topMenu = m_regtopMenu;
}

CSetWin11ContextMenu::~CSetWin11ContextMenu()
{
}

void CSetWin11ContextMenu::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
}

BEGIN_MESSAGE_MAP(CSetWin11ContextMenu, ISettingsPropPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestoreDefaults)
	ON_BN_CLICKED(IDC_REGISTER, &CSetWin11ContextMenu::OnBnClickedRegister)
END_MESSAGE_MAP()

BOOL CSetWin11ContextMenu::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SELECTALL);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | (CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = m_cMenuList.GetHeaderCtrl()->GetItemCount() - 1;
	while (c >= 0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, L"");

	SetWindowTheme(m_cMenuList.GetSafeHwnd(), L"Explorer", nullptr);

	m_cMenuList.SetRedraw(false);

	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	m_imgList.Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 4, 1);

	m_bBlock = true;

	InsertMenuItemToList(&m_cMenuList, &m_imgList);
	SetMenuItemCheck(&m_cMenuList, m_topMenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));

	m_bBlock = false;

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	for (int col = 0, maxcol = m_cMenuList.GetHeaderCtrl()->GetItemCount(); col < maxcol; ++col)
		m_cMenuList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	m_cMenuList.SetRedraw(true);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetWin11ContextMenu::OnApply()
{
	UpdateData();

	m_regtopMenu = m_topMenu;

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetWin11ContextMenu::OnBnClickedRestoreDefaults()
{
	SetModified(TRUE);
	m_topMenu = DEFAULTWIN11MENUTOPENTRIES;
	m_bBlock = true;
	SetMenuItemCheck(&m_cMenuList, m_topMenu, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetWin11ContextMenu::OnBnClickedSelectall()
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	m_bBlock = true;
	m_topMenu = ClickedSelectAll(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	m_bBlock = false;
}

void CSetWin11ContextMenu::OnLvnItemchangedMenulist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
		m_topMenu = GetMenuListMask(&m_cMenuList, static_cast<CButton*>(GetDlgItem(IDC_SELECTALL)));
	*pResult = 0;
}

void CSetWin11ContextMenu::OnChange()
{
	SetModified();
}

void CSetWin11ContextMenu::OnBnClickedRegister()
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	SCOPE_EXIT
	{
		CoUninitialize();
	};
	PackageManager manager;

	// first unregister if already registered
	Collections::IIterable<winrt::Windows::ApplicationModel::Package> packages;
	try
	{
		packages = manager.FindPackagesForUser(L"");
	}
	catch (winrt::hresult_error const& ex)
	{
		std::wstring error = L"FindPackagesForUser failed (Errorcode: ";
		error += std::to_wstring(ex.code().value);
		error += L"):\n";
		error += ex.message();
		MessageBox(error.c_str(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	for (const auto& package : packages)
	{
		if (package.Id().Name() != L"0BF99681-825C-4B2A-A14F-2AC01DB9B70E")
			continue;

		winrt::hstring fullName = package.Id().FullName();
		auto deploymentOperation = manager.RemovePackageAsync(fullName, RemovalOptions::None);
		auto deployResult = deploymentOperation.get();
		if (SUCCEEDED(deployResult.ExtendedErrorCode()))
			break;

		// Undeployment failed
		std::wstring error = L"RemovePackageAsync failed (Errorcode: ";
		error += std::to_wstring(deployResult.ExtendedErrorCode());
		error += L"):\n";
		error += deployResult.ErrorText();
		MessageBox(error.c_str(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	// now register the package
	auto appDir = CPathUtils::GetAppParentDirectory();
	Uri externalUri(static_cast<LPCWSTR>(appDir));
	auto packagePath = appDir + L"bin\\package.msix";
	Uri packageUri(static_cast<LPCWSTR>(packagePath));
	AddPackageOptions options;
	options.ExternalLocationUri(externalUri);
	auto deploymentOperation = manager.AddPackageByUriAsync(packageUri, options);

	auto deployResult = deploymentOperation.get();

	if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
	{
		std::wstring error = L"AddPackageByUriAsync failed (Errorcode: ";
		error += std::to_wstring(deployResult.ExtendedErrorCode());
		error += L"):\n";
		error += deployResult.ErrorText();
		MessageBox(error.c_str(), nullptr, MB_ICONERROR);
		return;
	}
	MessageBox(CString(MAKEINTRESOURCE(IDS_PACKAGE_REGISTERED)), L"TortoiseGit", MB_OK);
}
