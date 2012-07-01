// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 - TortoiseGit
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
#include "Globals.h"
#include "ShellUpdater.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include ".\setlookandfeelpage.h"
#include "MessageBox.h"
#include "MenuInfo.h"
#include "ShellCache.h"

extern MenuInfo menuInfo[];

void InsertMenuItemToList(CListCtrl *list,CImageList *imagelist)
{
	int i=0;
	while(menuInfo[i].command != ShellMenuLastEntry)
	{
		if(menuInfo[i].command != ShellSeparator &&
		   menuInfo[i].command != ShellSubMenu &&
		   menuInfo[i].command != ShellSubMenuFile &&
		   menuInfo[i].command != ShellSubMenuFolder &&
		   menuInfo[i].command != ShellSubMenuLink &&
		   menuInfo[i].command != ShellSubMenuMultiple)
		{
			HICON hIcon = reinterpret_cast<HICON>(::LoadImage(AfxGetResourceHandle(),
					MAKEINTRESOURCE(menuInfo[i].iconID),IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT ));

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

void SetMenuItemCheck(CListCtrl *list, unsigned __int64 mask, CButton *selectAll)
{
	bool allChecked = true;
	for(int i=0;i<list->GetItemCount();i++)
	{
		int data = list->GetItemData(i);

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

unsigned __int64 GetMenuListMask(CListCtrl *list, CButton *selectAll = NULL)
{
	unsigned __int64 mask = 0;
	bool allChecked = true;

	for(int i=0;i<list->GetItemCount();i++)
	{
		if(list->GetCheck(i))
		{
			int data = list->GetItemData(i);
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
unsigned __int64 ClickedSelectAll(CListCtrl *list, CButton *selectAll)
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
	{
		list->SetCheck(i, state == BST_CHECKED);
	}

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

	m_topmenu = unsigned __int64(DWORD(m_regTopmenuhigh))<<32;
	m_topmenu |= unsigned __int64(DWORD(m_regTopmenu));

	m_regHideMenus = CRegDWORD(_T("Software\\TortoiseGit\\HideMenusForUnversionedItems"), FALSE);
	m_bHideMenus = m_regHideMenus;

	m_regNoContextPaths = CRegString(_T("Software\\TortoiseGit\\NoContextPaths"), _T(""));
	m_sNoContextPaths = m_regNoContextPaths;
	m_sNoContextPaths.Replace(_T("\n"), _T("\r\n"));
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
}


BEGIN_MESSAGE_MAP(CSetLookAndFeelPage, ISettingsPropPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestoreDefaults)
	ON_BN_CLICKED(IDC_HIDEMENUS, OnChange)
	ON_EN_CHANGE(IDC_NOCONTEXTPATHS, &CSetLookAndFeelPage::OnEnChangeNocontextpaths)
END_MESSAGE_MAP()


BOOL CSetLookAndFeelPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);
	m_tooltips.AddTool(IDC_HIDEMENUS, IDS_SETTINGS_HIDEMENUS_TT);
	m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, _T(""));

	SetWindowTheme(m_hWnd, L"Explorer", NULL);

	m_cMenuList.SetRedraw(false);

	m_imgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 4, 1);

	m_bBlock = true;

	InsertMenuItemToList(&m_cMenuList,&m_imgList);
	SetMenuItemCheck(&m_cMenuList, m_topmenu, (CButton*)GetDlgItem(IDC_SELECTALL));

	m_bBlock = false;

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cMenuList.SetRedraw(true);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetLookAndFeelPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSetLookAndFeelPage::OnApply()
{
	UpdateData();

	m_regTopmenu = m_topmenu & 0xFFFFFFFF;
	m_regTopmenuhigh = (m_topmenu >> 32);

	m_regTopmenu.getErrorString();
	m_sNoContextPaths.Remove('\r');
	if (m_sNoContextPaths.Right(1).Compare(_T("\n"))!=0)
		m_sNoContextPaths += _T("\n");

	Store (m_bHideMenus, m_regHideMenus);
	Store (m_sNoContextPaths, m_regNoContextPaths);

	m_sNoContextPaths.Replace(_T("\n"), _T("\r\n"));

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetLookAndFeelPage::OnBnClickedRestoreDefaults()
{
	SetModified(TRUE);
	m_topmenu = DEFAULTMENUTOPENTRIES;
	m_bBlock = true;
	SetMenuItemCheck(&m_cMenuList, m_topmenu, (CButton*)GetDlgItem(IDC_SELECTALL));
	m_bBlock = false;
}

void CSetLookAndFeelPage::OnBnClickedSelectall()
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	m_bBlock = true;
	m_topmenu = ClickedSelectAll(&m_cMenuList, (CButton*)GetDlgItem(IDC_SELECTALL));
	m_bBlock = false;
}

void CSetLookAndFeelPage::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if (m_bBlock)
		return;
	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
	{
		m_topmenu = GetMenuListMask(&m_cMenuList, (CButton*)GetDlgItem(IDC_SELECTALL));
	}
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

	m_regExtmenu = shell.menuextlow;
	m_regExtmenuhigh = shell.menuexthigh;

	m_extmenu = unsigned __int64(DWORD(m_regExtmenuhigh))<<32;
	m_extmenu |= unsigned __int64(DWORD(m_regExtmenu));

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

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_EXTMENULAYOUT_TT);
	//m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, _T(""));

	SetWindowTheme(m_cMenuList.GetSafeHwnd(), L"Explorer", NULL);

	m_cMenuList.SetRedraw(false);

	m_imgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 4, 1);

	m_bBlock = true;

	InsertMenuItemToList(&m_cMenuList,&m_imgList);
	SetMenuItemCheck(&m_cMenuList, m_extmenu, (CButton*)GetDlgItem(IDC_SELECTALL));

	m_bBlock = false;

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cMenuList.SetRedraw(true);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetExtMenu::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSetExtMenu::OnApply()
{
	UpdateData();

	m_regExtmenu = (DWORD)(m_extmenu & 0xFFFFFFFF);
	m_regExtmenuhigh = (DWORD)(m_extmenu >> 32);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetExtMenu::OnBnClickedRestoreDefaults()
{
	SetModified(TRUE);
	m_extmenu = DEFAULTMENUEXTENTRIES;
	m_bBlock = true;
	SetMenuItemCheck(&m_cMenuList, m_extmenu, (CButton*)GetDlgItem(IDC_SELECTALL));
	m_bBlock = false;
}

void CSetExtMenu::OnBnClickedSelectall()
{
	if (m_bBlock)
		return;

	SetModified(TRUE);
	m_bBlock = true;
	m_extmenu = ClickedSelectAll(&m_cMenuList, (CButton*)GetDlgItem(IDC_SELECTALL));
	m_bBlock = false;
}

void CSetExtMenu::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if( m_bBlock )
		return;

	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
	{
		m_extmenu = GetMenuListMask(&m_cMenuList, (CButton*)GetDlgItem(IDC_SELECTALL));
	}
	*pResult = 0;
}

void CSetExtMenu::OnChange()
{
	SetModified();
}
