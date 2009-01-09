// TortoiseSVN - a Windows shell extension for easy version control

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "DirFileEnum.h"
#include "MessageBox.h"
#include ".\setoverlayicons.h"
#include "GitStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "ShellUpdater.h"
#include "XPTheme.h"

IMPLEMENT_DYNAMIC(CSetOverlayIcons, ISettingsPropPage)
CSetOverlayIcons::CSetOverlayIcons()
	: ISettingsPropPage(CSetOverlayIcons::IDD)
{ 
	m_regNormal = CRegString(_T("Software\\TortoiseOverlays\\NormalIcon"));
	m_regModified = CRegString(_T("Software\\TortoiseOverlays\\ModifiedIcon"));
	m_regConflicted = CRegString(_T("Software\\TortoiseOverlays\\ConflictIcon"));
	m_regReadOnly = CRegString(_T("Software\\TortoiseOverlays\\ReadOnlyIcon"));
	m_regDeleted = CRegString(_T("Software\\TortoiseOverlays\\DeletedIcon"));
	m_regLocked = CRegString(_T("Software\\TortoiseOverlays\\LockedIcon"));
	m_regAdded = CRegString(_T("Software\\TortoiseOverlays\\AddedIcon"));
	m_regIgnored = CRegString(_T("Software\\TortoiseOverlays\\IgnoredIcon"));
	m_regUnversioned = CRegString(_T("Software\\TortoiseOverlays\\UnversionedIcon"));
}

CSetOverlayIcons::~CSetOverlayIcons()
{
}

void CSetOverlayIcons::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICONSETCOMBO, m_cIconSet);
	DDX_Control(pDX, IDC_ICONLIST, m_cIconList);
}


BEGIN_MESSAGE_MAP(CSetOverlayIcons, ISettingsPropPage)
	ON_BN_CLICKED(IDC_LISTRADIO, OnBnClickedListradio)
	ON_BN_CLICKED(IDC_SYMBOLRADIO, OnBnClickedSymbolradio)
	ON_CBN_SELCHANGE(IDC_ICONSETCOMBO, OnCbnSelchangeIconsetcombo)
END_MESSAGE_MAP()

BOOL CSetOverlayIcons::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cIconList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES);
	// get the path to our icon sets
	TCHAR buf[MAX_PATH] = {0};
	SHGetSpecialFolderPath(m_hWnd, buf, CSIDL_PROGRAM_FILES_COMMON, true);
	m_sIconPath = buf;
	m_sIconPath += _T("\\TortoiseOverlays\\Icons");
	// list all the icon sets
	CDirFileEnum filefinder(m_sIconPath);
	bool isDir = false;
	CString item;
	while (filefinder.NextFile(item, &isDir))
	{
		if (!isDir)
			continue;
		m_cIconSet.AddString(CPathUtils::GetFileNameFromPath(item));
	}
	CheckRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO, IDC_LISTRADIO);
	CString sModifiedIcon = m_regModified;
	if (sModifiedIcon.IsEmpty())
	{
		// no custom icon set, use the default
		sModifiedIcon = m_sIconPath + _T("\\XPStyle\\ModifiedIcon.ico");
	}
	if (sModifiedIcon.Left(m_sIconPath.GetLength()).CompareNoCase(m_sIconPath)!=0)
	{
		// an icon set outside our own installation? We don't support that,
		// so fall back to the default!
		sModifiedIcon = m_sIconPath + _T("\\XPStyle\\ModifiedIcon.ico");
	}
	// the name of the icon set is the folder of the icon location
	m_sOriginalIconSet = sModifiedIcon.Mid(m_sIconPath.GetLength()+1);
	m_sOriginalIconSet = m_sOriginalIconSet.Left(m_sOriginalIconSet.ReverseFind('\\'));
	// now we have the name of the icon set. Set the combobox to show
	// that as selected
	CString ComboItem;
	for (int i=0; i<m_cIconSet.GetCount(); ++i)
	{
		m_cIconSet.GetLBText(i, ComboItem);
		if (ComboItem.CompareNoCase(m_sOriginalIconSet)==0)
			m_cIconSet.SetCurSel(i);
	}
#if 0
	WORD langID = (WORD)(DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\LanguageID"), GetUserDefaultLangID());
	TCHAR statustext[MAX_STATUS_STRING_LENGTH];
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_normal, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sNormal = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_modified, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sModified = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_conflicted, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sConflicted = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_deleted, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sDeleted = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_added, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sAdded = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_ignored, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sIgnored = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_unversioned, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sUnversioned = statustext;
#endif
	m_sReadOnly.LoadString(IDS_SETTINGS_READONLYNAME);
	m_sLocked.LoadString(IDS_SETTINGS_LOCKEDNAME);

	CXPTheme theme;
	theme.SetWindowTheme(m_cIconList.GetSafeHwnd(), L"Explorer", NULL);

	ShowIconSet(true);

	return TRUE;
}

void CSetOverlayIcons::ShowIconSet(bool bSmallIcons)
{
	m_cIconList.SetRedraw(FALSE);
	m_cIconList.DeleteAllItems();
	m_ImageList.DeleteImageList();
	m_ImageListBig.DeleteImageList();
	m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_ImageListBig.Create(32, 32, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_cIconList.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_cIconList.SetImageList(&m_ImageListBig, LVSIL_NORMAL);

	// find all the icons of the selected icon set
	CString sIconSet;
	int index = m_cIconSet.GetCurSel();
	if (index == CB_ERR)
	{
		// nothing selected. Shouldn't happen!
		return;
	}
	m_cIconSet.GetLBText(index, sIconSet);
	CString sIconSetPath = m_sIconPath + _T("\\") + sIconSet;

	CImageList * pImageList = bSmallIcons ? &m_ImageList : &m_ImageListBig;
	int pixelsize = (bSmallIcons ? 16 : 32);
	HICON hNormalOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\NormalIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hNormalOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 1));
	HICON hModifiedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\ModifiedIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hModifiedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 2));
	HICON hConflictedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\ConflictIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hConflictedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 3));
	HICON hReadOnlyOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\ReadOnlyIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hReadOnlyOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 4));
	HICON hDeletedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\DeletedIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hDeletedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 5));
	HICON hLockedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\LockedIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hLockedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 6));
	HICON hAddedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\AddedIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hAddedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 7));
	HICON hIgnoredOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\IgnoredIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hIgnoredOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 8));
	HICON hUnversionedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\UnversionedIcon.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hUnversionedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 9));

	DestroyIcon(hNormalOverlay);
	DestroyIcon(hModifiedOverlay);
	DestroyIcon(hConflictedOverlay);
	DestroyIcon(hReadOnlyOverlay);
	DestroyIcon(hDeletedOverlay);
	DestroyIcon(hLockedOverlay);
	DestroyIcon(hAddedOverlay);
	DestroyIcon(hIgnoredOverlay);
	DestroyIcon(hUnversionedOverlay);

	// create an image list with different file icons
	SHFILEINFO sfi;
	SecureZeroMemory(&sfi, sizeof sfi);

	UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;
	if (bSmallIcons)
		flags |= SHGFI_SMALLICON;
	else
		flags |= SHGFI_LARGEICON;
	SHGetFileInfo(
		_T("Doesn't matter"),
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, sizeof sfi,
		flags);

	int folderindex = pImageList->Add(sfi.hIcon);	//folder
	DestroyIcon(sfi.hIcon);
	//folders
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal, folderindex);
	VERIFY(m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK));
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sIgnored, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(8), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sUnversioned, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(9), TVIS_OVERLAYMASK);

	AddFileTypeGroup(_T(".cpp"), bSmallIcons);
	AddFileTypeGroup(_T(".h"), bSmallIcons);
	AddFileTypeGroup(_T(".txt"), bSmallIcons);
	AddFileTypeGroup(_T(".java"), bSmallIcons);
	AddFileTypeGroup(_T(".doc"), bSmallIcons);
	AddFileTypeGroup(_T(".pl"), bSmallIcons);
	AddFileTypeGroup(_T(".php"), bSmallIcons);
	AddFileTypeGroup(_T(".asp"), bSmallIcons);
	AddFileTypeGroup(_T(".cs"), bSmallIcons);
	AddFileTypeGroup(_T(".vb"), bSmallIcons);
	AddFileTypeGroup(_T(".xml"), bSmallIcons);
	AddFileTypeGroup(_T(".pas"), bSmallIcons);
	AddFileTypeGroup(_T(".dpr"), bSmallIcons);
	AddFileTypeGroup(_T(".dfm"), bSmallIcons);
	AddFileTypeGroup(_T(".res"), bSmallIcons);
	AddFileTypeGroup(_T(".asmx"), bSmallIcons);
	AddFileTypeGroup(_T(".aspx"), bSmallIcons);
	AddFileTypeGroup(_T(".resx"), bSmallIcons);
	AddFileTypeGroup(_T(".vbp"), bSmallIcons);
	AddFileTypeGroup(_T(".frm"), bSmallIcons);
	AddFileTypeGroup(_T(".frx"), bSmallIcons);
	AddFileTypeGroup(_T(".bas"), bSmallIcons);
	AddFileTypeGroup(_T(".config"), bSmallIcons);
	AddFileTypeGroup(_T(".css"), bSmallIcons);
	AddFileTypeGroup(_T(".acsx"), bSmallIcons);
	AddFileTypeGroup(_T(".jpg"), bSmallIcons);
	AddFileTypeGroup(_T(".png"), bSmallIcons);
	m_cIconList.SetRedraw(TRUE);
}
void CSetOverlayIcons::AddFileTypeGroup(CString sFileType, bool bSmallIcons)
{
	UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;
	if (bSmallIcons)
		flags |= SHGFI_SMALLICON;
	else
		flags |= SHGFI_LARGEICON;
	SHFILEINFO sfi;
	SecureZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		sFileType,
		FILE_ATTRIBUTE_NORMAL,
		&sfi, sizeof sfi,
		flags);

	int imageindex = 0;
	if (bSmallIcons)
		imageindex = m_ImageList.Add(sfi.hIcon);
	else
		imageindex = m_ImageListBig.Add(sfi.hIcon);

	DestroyIcon(sfi.hIcon);
	int index = 0;
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sIgnored+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(8), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sUnversioned+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(9), TVIS_OVERLAYMASK);
}

void CSetOverlayIcons::OnBnClickedListradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
	ShowIconSet(true);
}

void CSetOverlayIcons::OnBnClickedSymbolradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
	ShowIconSet(false);
}

void CSetOverlayIcons::OnCbnSelchangeIconsetcombo()
{
	bool bSmallIcons = (GetCheckedRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO) == IDC_LISTRADIO);
	ShowIconSet(bSmallIcons);
	m_selIndex = m_cIconSet.GetCurSel();
	if (m_selIndex != CB_ERR)
	{
		m_cIconSet.GetLBText(m_selIndex, m_sIconSet);
	}
	if (m_sIconSet.CompareNoCase(m_sOriginalIconSet)!=0)
		SetModified();
}

BOOL CSetOverlayIcons::OnApply()
{
	UpdateData();

	if ((!m_sIconSet.IsEmpty())&&(m_sIconSet.CompareNoCase(m_sOriginalIconSet)!=0))
	{
		// the selected icon set has changed.
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\NormalIcon.ico"), m_regNormal);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\ModifiedIcon.ico"), m_regModified);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\ConflictIcon.ico"), m_regConflicted);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\ReadOnlyIcon.ico"), m_regReadOnly);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\DeletedIcon.ico"), m_regDeleted);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\LockedIcon.ico"), m_regLocked);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\AddedIcon.ico"), m_regAdded);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\IgnoredIcon.ico"), m_regIgnored);
		Store (m_sIconPath + _T("\\") + m_sIconSet + _T("\\UnversionedIcon.ico"), m_regUnversioned);

		m_restart = Restart_System;
		m_sOriginalIconSet = m_sIconSet;
	}
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
