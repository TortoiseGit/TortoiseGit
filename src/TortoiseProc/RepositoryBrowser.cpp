// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit
// Copyright (C) 2003-2013 - TortoiseSVN

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
// RepositoryBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RepositoryBrowser.h"
#include "LogDlg.h"
#include "AppUtils.h"
#include "IconMenu.h"
#include "UnicodeUtils.h"
#include "SysImageList.h"
#include <sys/stat.h>
#include "registry.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "GitDiff.h"
#include "DragDropImpl.h"
#include "GitDataObject.h"
#include "TempFile.h"
#include "DPIAware.h"

#define OVERLAY_EXTERNAL	1
#define OVERLAY_EXECUTABLE	2
#define OVERLAY_SYMLINK		3

void SetSortArrowA(CListCtrl * control, int nColumn, bool bAscending)
{
	if (!control)
		return;

	// set the sort arrow
	CHeaderCtrl * pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i = 0; i < pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (nColumn >= 0)
	{
		pHeader->GetItem(nColumn, &HeaderItem);
		HeaderItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(nColumn, &HeaderItem);
	}
}

class CRepoListCompareFunc
{
public:
	CRepoListCompareFunc(CListCtrl* pList, int col, bool desc)
	: m_col(col)
	, m_desc(desc)
	, m_pList(pList)
	{}

	static int CALLBACK StaticCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		return reinterpret_cast<CRepoListCompareFunc*>(lParamSort)->Compare(lParam1, lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		auto pLeft	= reinterpret_cast<CShadowFilesTree*>(m_pList->GetItemData(static_cast<int>(lParam1)));
		auto pRight	= reinterpret_cast<CShadowFilesTree*>(m_pList->GetItemData(static_cast<int>(lParam2)));

		int result = 0;
		switch(m_col)
		{
		case CRepositoryBrowser::eCol_Name:
			result = SortStrCmp(pLeft->m_sName, pRight->m_sName);
			if (result != 0)
				break;
		case CRepositoryBrowser::eCol_Extension:
			result = m_pList->GetItemText(static_cast<int>(lParam1), 1).CompareNoCase(m_pList->GetItemText(static_cast<int>(lParam2), 1));
			if (result == 0)  // if extensions are the same, use the filename to sort
				result = SortStrCmp(pRight->m_sName, pRight->m_sName);
			if (result != 0)
				break;
		case CRepositoryBrowser::eCol_FileSize:
			if (pLeft->m_iSize > pRight->m_iSize)
				result = 1;
			else if (pLeft->m_iSize < pRight->m_iSize)
				result = -1;
			else // fallback
				result = SortStrCmp(pLeft->m_sName, pRight->m_sName);
		}

		if (m_desc)
			result = -result;

		if (pLeft->m_bFolder != pRight->m_bFolder)
		{
			if (pRight->m_bFolder)
				result = 1;
			else
				result = -1;
		}

		return result;
	}
	int SortStrCmp(const CString &left, const CString &right)
	{
		if (CRepositoryBrowser::s_bSortLogical)
			return StrCmpLogicalW(left, right);
		return StrCmpI(left, right);
	}

	int m_col;
	bool m_desc;
	CListCtrl* m_pList;
};

// CRepositoryBrowser dialog

bool CRepositoryBrowser::s_bSortLogical = true;

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(CString rev, CWnd* pParent /*=nullptr*/)
: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
, m_currSortCol(0)
, m_currSortDesc(false)
, m_sRevision(rev)
, m_bHasWC(true)
, m_ColumnManager(&m_RepoList)
, m_nIconFolder(0)
, m_nOpenIconFolder(0)
, m_nExternalOvl(0)
, m_nExecutableOvl(0)
, m_nSymlinkOvl(0)
, bDragMode(false)
, oldy(0)
, oldx(0)
{
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOTREE, m_RepoTree);
	DDX_Control(pDX, IDC_REPOLIST, m_RepoList);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableStandAloneDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_REPOTREE, &CRepositoryBrowser::OnTvnSelchangedRepoTree)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_REPOTREE, &CRepositoryBrowser::OnTvnItemExpandingRepoTree)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_REPOLIST, &CRepositoryBrowser::OnLvnColumnclickRepoList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REPOLIST, &CRepositoryBrowser::OnLvnItemchangedRepolist)
	ON_NOTIFY(NM_DBLCLK, IDC_REPOLIST, &CRepositoryBrowser::OnNMDblclk_RepoList)
	ON_BN_CLICKED(IDC_BUTTON_REVISION, &CRepositoryBrowser::OnBnClickedButtonRevision)
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_REPOLIST, &CRepositoryBrowser::OnLvnBegindragRepolist)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_REPOTREE, &CRepositoryBrowser::OnTvnBegindragRepotree)
END_MESSAGE_MAP()


// CRepositoryBrowser message handlers

BOOL CRepositoryBrowser::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_STATIC_REPOURL, TOP_LEFT);
	AddAnchor(IDC_REPOBROWSER_URL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_REF, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_REVISION, TOP_RIGHT);
	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	CRepositoryBrowser::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (CRepositoryBrowser::s_bSortLogical)
		CRepositoryBrowser::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);

	static UINT columnNames[] = { IDS_STATUSLIST_COLFILENAME, IDS_STATUSLIST_COLEXT, IDS_LOG_SIZE };
	static int columnWidths[] = { 150, 100, 100 };
	DWORD dwDefaultColumns = (1 << eCol_Name) | (1 << eCol_Extension) | (1 << eCol_FileSize);
	m_ColumnManager.SetNames(columnNames, _countof(columnNames));
	m_ColumnManager.ReadSettings(dwDefaultColumns, 0, L"RepoBrowser", _countof(columnNames), columnWidths);
	m_ColumnManager.SetRightAlign(2);

	// set up the list control
	// set the extended style of the list control
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(L"Software\\TortoiseGit\\FullRowSelect", TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	m_RepoList.SetExtendedStyle(exStyle);
	m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	CAppUtils::SetListCtrlBackgroundImage(m_RepoList.GetSafeHwnd(), IDI_REPOBROWSER_BKG);

	m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
	exStyle = TVS_EX_FADEINOUTEXPANDOS | TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
	m_RepoTree.SetExtendedStyle(exStyle, exStyle);

	m_nExternalOvl = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_EXTERNALOVL, 0, 0));
	m_nExecutableOvl = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_EXECUTABLEOVL, 0, 0));
	m_nSymlinkOvl = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_SYMLINKOVL, 0, 0));
	// set externaloverlay in SYS_IMAGE_LIST() in Refresh method, so that it is updated after every launch of the logdialog

	SetWindowTheme(m_RepoTree.GetSafeHwnd(), L"Explorer", nullptr);
	SetWindowTheme(m_RepoList.GetSafeHwnd(), L"Explorer", nullptr);

	int borderWidth = 0;
	if (IsAppThemed())
	{
		HTHEME hTheme = OpenThemeData(m_RepoTree, L"TREEVIEW");
		GetThemeMetric(hTheme, NULL, TVP_TREEITEM, TREIS_NORMAL, TMT_BORDERSIZE, &borderWidth);
		CloseThemeData(hTheme);
	}
	else
		borderWidth = GetSystemMetrics(SM_CYBORDER);
	m_RepoTree.SetItemHeight(static_cast<SHORT>(m_RepoTree.GetItemHeight() + 2 * borderWidth));

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();

	EnableSaveRestore(L"Reposbrowser");

	DWORD xPos = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RepobrowserDivider", 0);
	if (xPos == 0)
	{
		RECT rc;
		GetDlgItem(IDC_REPOTREE)->GetClientRect(&rc);
		xPos = rc.right - rc.left;
	}
	HandleDividerMove(CPoint(xPos + CDPIAware::Instance().ScaleX(20), CDPIAware::Instance().ScaleY(10)), false);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_bHasWC = !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir);

	Refresh();

	m_RepoList.SetFocus();

	return FALSE;
}

void CRepositoryBrowser::OnDestroy()
{
	int maxcol = m_ColumnManager.GetColumnCount();
	for (int col = 0; col < maxcol; ++col)
		if (m_ColumnManager.IsVisible(col))
			m_ColumnManager.ColumnResized(col);
	m_ColumnManager.WriteSettings();

	CResizableStandAloneDialog::OnDestroy();
}

void CRepositoryBrowser::OnOK()
{
	if (GetFocus() == &m_RepoList && (GetKeyState(VK_MENU) & 0x8000) == 0)
	{
		// list control has focus: 'enter' the folder
		if (m_RepoList.GetSelectedCount() != 1)
			return;

		POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
		if (pos)
		{
			auto item = GetListEntry(m_RepoList.GetNextSelectedItem(pos));
			if (item->m_bFolder)
			{
				FillListCtrlForShadowTree(item);
				m_RepoTree.SelectItem(item->m_hTree);
			}
			else
				OpenFile(item->GetFullName(), OPEN, item->m_bSubmodule, item->m_hash);
		}
		return;
	}

	SaveDividerPosition();
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
	SaveDividerPosition();
	CResizableStandAloneDialog::OnCancel();
}

void CRepositoryBrowser::OnNMDblclk_RepoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNmItemActivate->iItem < 0)
		return;

	auto pItem = GetListEntry(pNmItemActivate->iItem);
	if (!pItem )
		return;

	if (!pItem->m_bFolder)
	{
		OpenFile(pItem->GetFullName(), OPEN, pItem->m_bSubmodule, pItem->m_hash);
		return;
	}
	else
	{
		FillListCtrlForShadowTree(pItem);
		m_RepoTree.SelectItem(pItem->m_hTree);
	}
}

void CRepositoryBrowser::Refresh()
{
	BeginWaitCursor();
	if (m_nExternalOvl >= 0)
		SYS_IMAGE_LIST().SetOverlayImage(m_nExternalOvl, OVERLAY_EXTERNAL);
	if (m_nExecutableOvl >= 0)
		SYS_IMAGE_LIST().SetOverlayImage(m_nExecutableOvl, OVERLAY_EXECUTABLE);
	if (m_nSymlinkOvl >= 0)
		SYS_IMAGE_LIST().SetOverlayImage(m_nSymlinkOvl, OVERLAY_SYMLINK);

	m_RepoTree.DeleteAllItems();
	m_RepoList.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_sName.Empty();
	m_TreeRoot.m_bFolder = true;

	TVINSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = TVI_ROOT;
	tvinsert.hInsertAfter = TVI_ROOT;
	tvinsert.itemex.mask = TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvinsert.itemex.pszText = L"/";
	tvinsert.itemex.lParam = reinterpret_cast<LPARAM>(&m_TreeRoot);
	tvinsert.itemex.iImage = m_nIconFolder;
	tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
	m_TreeRoot.m_hTree= m_RepoTree.InsertItem(&tvinsert);

	ReadTree(&m_TreeRoot);
	m_RepoTree.Expand(m_TreeRoot.m_hTree, TVE_EXPAND);
	FillListCtrlForShadowTree(&m_TreeRoot);
	m_RepoTree.SelectItem(m_TreeRoot.m_hTree);
	EndWaitCursor();
}

int CRepositoryBrowser::ReadTreeRecursive(git_repository& repo, const git_tree* tree, CShadowFilesTree* treeroot, bool recursive)
{
	size_t count = git_tree_entrycount(tree);
	bool hasSubfolders = false;
	treeroot->m_bLoaded = true;

	for (size_t i = 0; i < count; ++i)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (!entry)
			continue;

		const int mode = git_tree_entry_filemode(entry);

		CString base = CUnicodeUtils::GetUnicode(git_tree_entry_name(entry), CP_UTF8);

		const git_oid *oid = git_tree_entry_id(entry);
		CShadowFilesTree * pNextTree = &treeroot->m_ShadowTree[base];
		pNextTree->m_sName = base;
		pNextTree->m_pParent = treeroot;
		pNextTree->m_hash = oid;

		if (mode == GIT_FILEMODE_COMMIT)
			pNextTree->m_bSubmodule = true;
		else if (mode & S_IFDIR)
		{
			hasSubfolders = true;
			pNextTree->m_bFolder = true;
			pNextTree->m_bLoaded = false;

			TVINSERTSTRUCT tvinsert = {0};
			tvinsert.hParent = treeroot->m_hTree;
			tvinsert.hInsertAfter = TVI_SORT;
			tvinsert.itemex.mask = TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_CHILDREN;
			tvinsert.itemex.pszText = base.GetBuffer(base.GetLength());
			tvinsert.itemex.cChildren = 1;
			tvinsert.itemex.lParam = reinterpret_cast<LPARAM>(pNextTree);
			tvinsert.itemex.iImage = m_nIconFolder;
			tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
			pNextTree->m_hTree = m_RepoTree.InsertItem(&tvinsert);
			base.ReleaseBuffer();
			if (recursive)
			{
				CAutoTree subtree;
				if (git_tree_lookup(subtree.GetPointer(), &repo, oid))
				{
					MessageBox(CGit::GetLibGit2LastErr(L"Could not lookup path."), L"TortoiseGit", MB_ICONERROR);
					return -1;
				}

				ReadTreeRecursive(repo, subtree, pNextTree, recursive);
			}
		}
		else
		{
			if (mode == GIT_FILEMODE_BLOB_EXECUTABLE)
				pNextTree->m_bExecutable = true;
			if (mode == GIT_FILEMODE_LINK)
				pNextTree->m_bSymlink = true;
			CAutoBlob blob;
			git_blob_lookup(blob.GetPointer(), &repo, oid);
			if (!blob)
				continue;

			pNextTree->m_iSize = git_blob_rawsize(blob);
		}
	}

	if (!hasSubfolders)
	{
		TVITEM tvitem = { 0 };
		tvitem.hItem = treeroot->m_hTree;
		tvitem.mask = TVIF_CHILDREN;
		tvitem.cChildren = 0;
		m_RepoTree.SetItem(&tvitem);
	}

	return 0;
}

int CRepositoryBrowser::ReadTree(CShadowFilesTree* treeroot, const CString& root, bool recursive)
{
	CWaitCursor wait;
	CAutoRepository repository(g_Git.GetGitRepository());
	if (!repository)
	{
		MessageBox(CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	if (m_sRevision == L"HEAD")
	{
		int ret = git_repository_head_unborn(repository);
		if (ret == 1)	// is orphan
			return ret;
		else if (ret != 0)
		{
			MessageBox(g_Git.GetLibGit2LastErr(L"Could not check HEAD."), L"TortoiseGit", MB_ICONERROR);
			return ret;
		}
	}

	CGitHash hash;
	if (CGit::GetHash(repository, hash, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
	{
		MessageBox(CGit::GetLibGit2LastErr(L"Could not get hash of \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	CAutoCommit commit;
	if (git_commit_lookup(commit.GetPointer(), repository, hash))
	{
		MessageBox(CGit::GetLibGit2LastErr(L"Could not lookup commit."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	CAutoTree tree;
	if (git_commit_tree(tree.GetPointer(), commit))
	{
		MessageBox(CGit::GetLibGit2LastErr(L"Could not get tree of commit."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	if (!root.IsEmpty())
	{
		CAutoTreeEntry treeEntry;
		if (git_tree_entry_bypath(treeEntry.GetPointer(), tree, CUnicodeUtils::GetUTF8(root)))
		{
			MessageBox(CGit::GetLibGit2LastErr(L"Could not lookup path."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
		if (git_tree_entry_type(treeEntry) != GIT_OBJECT_TREE)
		{
			MessageBox(CGit::GetLibGit2LastErr(L"Could not lookup path."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}

		CAutoObject object;
		if (git_tree_entry_to_object(object.GetPointer(), repository, treeEntry))
		{
			MessageBox(CGit::GetLibGit2LastErr(L"Could not lookup path."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}

		tree.ConvertFrom(std::move(object));
	}

	treeroot->m_hash = git_tree_id(tree);
	ReadTreeRecursive(*repository, tree, treeroot, recursive);

	// try to resolve hash to a branch name
	if (m_sRevision == hash.ToString())
	{
		MAP_HASH_NAME map;
		if (CGit::GetMapHashToFriendName(repository, map))
			MessageBox(g_Git.GetLibGit2LastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		if (!map[hash].empty())
			m_sRevision = map[hash].at(0);
	}
	this->GetDlgItem(IDC_BUTTON_REVISION)->SetWindowText(m_sRevision);

	return 0;
}

void CRepositoryBrowser::OnTvnSelchangedRepoTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	FillListCtrlForTreeNode(pNMTreeView->itemNew.hItem);
}

void CRepositoryBrowser::OnTvnItemExpandingRepoTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	auto pTree = GetTreeEntry(pNMTreeView->itemNew.hItem);
	if (!pTree)
		return;

	if (!pTree->m_bLoaded)
		ReadTree(pTree, pTree->GetFullName());
}

void CRepositoryBrowser::FillListCtrlForTreeNode(HTREEITEM treeNode)
{
	m_RepoList.DeleteAllItems();

	auto pTree = GetTreeEntry(treeNode);
	if (!pTree)
		return;

	CString url = L'/' + pTree->GetFullName();
	GetDlgItem(IDC_REPOBROWSER_URL)->SetWindowText(url);

	if (!pTree->m_bLoaded)
		ReadTree(pTree, pTree->GetFullName());

	FillListCtrlForShadowTree(pTree);
}

void CRepositoryBrowser::FillListCtrlForShadowTree(CShadowFilesTree* pTree)
{
	for (auto itShadowTree = pTree->m_ShadowTree.cbegin(); itShadowTree != pTree->m_ShadowTree.cend(); ++itShadowTree)
	{
		int icon = m_nIconFolder;
		if (!(*itShadowTree).second.m_bFolder && !(*itShadowTree).second.m_bSubmodule)
		{
			icon = SYS_IMAGE_LIST().GetPathIconIndex((*itShadowTree).second.m_sName);
		}

		int indexItem = m_RepoList.InsertItem(m_RepoList.GetItemCount(), (*itShadowTree).second.m_sName, icon);

		if ((*itShadowTree).second.m_bSubmodule)
		{
			m_RepoList.SetItemState(indexItem, INDEXTOOVERLAYMASK(OVERLAY_EXTERNAL), LVIS_OVERLAYMASK);
		}
		if ((*itShadowTree).second.m_bExecutable)
			m_RepoList.SetItemState(indexItem, INDEXTOOVERLAYMASK(OVERLAY_EXECUTABLE), LVIS_OVERLAYMASK);
		if ((*itShadowTree).second.m_bSymlink)
			m_RepoList.SetItemState(indexItem, INDEXTOOVERLAYMASK(OVERLAY_SYMLINK), LVIS_OVERLAYMASK);
		m_RepoList.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(&(*itShadowTree).second));
		if (!(*itShadowTree).second.m_bFolder && !(*itShadowTree).second.m_bSubmodule)
		{
			CString temp;

			temp = CPathUtils::GetFileExtFromPath((*itShadowTree).second.m_sName);
			m_RepoList.SetItemText(indexItem, eCol_Extension, temp);

			StrFormatByteSize64((*itShadowTree).second.m_iSize, CStrBuf(temp, 20), 20);
			m_RepoList.SetItemText(indexItem, eCol_FileSize, temp);
		}
	}

	CRepoListCompareFunc compareFunc(&m_RepoList, m_currSortCol, m_currSortDesc);
	m_RepoList.SortItemsEx(&CRepoListCompareFunc::StaticCompare, reinterpret_cast<DWORD_PTR>(&compareFunc));

	SetSortArrowA(&m_RepoList, m_currSortCol, !m_currSortDesc);

	UpdateInfoLabel();
}

void CRepositoryBrowser::UpdateInfoLabel()
{
	CString temp;
	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	if (pos)
	{
		if (m_RepoList.GetSelectedCount() > 1)
		{
			temp.FormatMessage(IDS_REPOBROWSE_INFOMULTI, m_RepoList.GetSelectedCount());
		}
		else
		{
			int index = m_RepoList.GetNextSelectedItem(pos);
			auto item = GetListEntry(index);
			if (item->m_bSubmodule)
				temp.FormatMessage(IDS_REPOBROWSE_INFOEXT, static_cast<LPCTSTR>(m_RepoList.GetItemText(index, eCol_Name)), static_cast<LPCTSTR>(item->m_hash.ToString()));
			else if (item->m_bFolder)
				temp = m_RepoList.GetItemText(index, eCol_Name);
			else
				temp.FormatMessage(IDS_REPOBROWSE_INFOFILE, static_cast<LPCTSTR>(m_RepoList.GetItemText(index, eCol_Name)), static_cast<LPCTSTR>(m_RepoList.GetItemText(index, eCol_FileSize)));
		}
	}
	else
	{
		HTREEITEM hTreeItem = m_RepoTree.GetSelectedItem();
		if (hTreeItem != nullptr)
		{
			auto pTree = GetTreeEntry(hTreeItem);
			if (pTree != nullptr)
			{
				size_t files = 0, submodules = 0;
				for (auto itShadowTree = pTree->m_ShadowTree.cbegin(); itShadowTree != pTree->m_ShadowTree.cend(); ++itShadowTree)
				{
					if (!(*itShadowTree).second.m_bFolder && !(*itShadowTree).second.m_bSubmodule)
						++files;
					if ((*itShadowTree).second.m_bSubmodule)
						++submodules;
				}
				temp.FormatMessage(IDS_REPOBROWSE_INFO, static_cast<LPCTSTR>(pTree->m_sName), files, submodules, pTree->m_ShadowTree.size() - files - submodules, pTree->m_ShadowTree.size());
			}
		}
	}
	SetDlgItemText(IDC_INFOLABEL, temp);
}

void CRepositoryBrowser::OnLvnItemchangedRepolist(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	*pResult = 0;
	UpdateInfoLabel();
}

void CRepositoryBrowser::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if (pWndFrom == &m_RepoList)
	{
		CRect headerPosition;
		m_RepoList.GetHeaderCtrl()->GetWindowRect(headerPosition);
		if (!headerPosition.PtInRect(point))
			OnContextMenu_RepoList(point);
	}
	else if (pWndFrom == &m_RepoTree)
		OnContextMenu_RepoTree(point);
}

void CRepositoryBrowser::OnContextMenu_RepoTree(CPoint point)
{
	CPoint clientPoint = point;
	m_RepoTree.ScreenToClient(&clientPoint);

	HTREEITEM hTreeItem = m_RepoTree.HitTest(clientPoint);
	if (!hTreeItem)
		return;

	TShadowFilesTreeList selectedLeafs;
	selectedLeafs.push_back(GetTreeEntry(hTreeItem));

	ShowContextMenu(point, selectedLeafs, ONLY_FOLDERS);
}

void CRepositoryBrowser::OnContextMenu_RepoList(CPoint point)
{
	TShadowFilesTreeList selectedLeafs;
	selectedLeafs.reserve(m_RepoList.GetSelectedCount());

	bool folderSelected = false;
	bool filesSelected = false;
	bool submodulesSelected = false;

	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	while (pos)
	{
		auto item = GetListEntry(m_RepoList.GetNextSelectedItem(pos));
		if (item->m_bSubmodule)
			submodulesSelected = true;
		if (item->m_bFolder)
			folderSelected = true;
		else
			filesSelected = true;
		selectedLeafs.push_back(item);
	}

	eSelectionType selType = ONLY_FILES;
	if (folderSelected && filesSelected)
		selType = MIXED_FOLDERS_FILES;
	else if (folderSelected)
		selType = ONLY_FOLDERS;
	else if (submodulesSelected)
		selType = ONLY_FILESSUBMODULES;
	ShowContextMenu(point, selectedLeafs, selType);
}

void CRepositoryBrowser::ShowContextMenu(CPoint point, TShadowFilesTreeList &selectedLeafs, eSelectionType selType)
{
	CIconMenu popupMenu;
	popupMenu.CreatePopupMenu();

	bool bAddSeparator = false;

	if (selectedLeafs.size() == 1)
	{
		popupMenu.AppendMenuIcon(eCmd_Open, IDS_REPOBROWSE_OPEN, IDI_OPEN);
		popupMenu.SetDefaultItem(eCmd_Open, FALSE);
		if (selType == ONLY_FILES || selType == ONLY_FILESSUBMODULES)
		{
			popupMenu.AppendMenuIcon(eCmd_OpenWith, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
			popupMenu.AppendMenuIcon(eCmd_OpenWithAlternativeEditor, IDS_LOG_POPUP_VIEWREV, IDI_NOTEPAD);
		}

		popupMenu.AppendMenu(MF_SEPARATOR);

		if (m_bHasWC && (selType == ONLY_FILES || selType == ONLY_FILESSUBMODULES))
		{
			popupMenu.AppendMenuIcon(eCmd_CompareWC, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
			bAddSeparator = true;
		}

		if (bAddSeparator)
			popupMenu.AppendMenu(MF_SEPARATOR);
		bAddSeparator = false;

		CString temp;
		temp.LoadString(IDS_MENULOG);
		popupMenu.AppendMenuIcon(eCmd_ViewLog, temp, IDI_LOG);
		if (selectedLeafs[0]->m_bSubmodule)
		{
			temp.LoadString(IDS_MENULOGSUBMODULE);
			popupMenu.AppendMenuIcon(eCmd_ViewLogSubmodule, temp, IDI_LOG);
		}

		if (selType == ONLY_FILES)
		{
			if (m_bHasWC)
				popupMenu.AppendMenuIcon(eCmd_Blame, IDS_LOG_POPUP_BLAME, IDI_BLAME);

			popupMenu.AppendMenu(MF_SEPARATOR);
			temp.LoadString(IDS_LOG_POPUP_SAVE);
			popupMenu.AppendMenuIcon(eCmd_SaveAs, temp, IDI_SAVEAS);
		}

		bAddSeparator = true;
	}

	if (!selectedLeafs.empty() && selType == ONLY_FILES && m_bHasWC)
	{
		popupMenu.AppendMenuIcon(eCmd_Revert, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
		bAddSeparator = true;
	}

	if (bAddSeparator)
		popupMenu.AppendMenu(MF_SEPARATOR);

	if (selectedLeafs.size() == 1 && selType == ONLY_FILES)
	{
		popupMenu.AppendMenuIcon(eCmd_PrepareDiff, IDS_PREPAREDIFF, IDI_DIFF);
		if (!m_sMarkForDiffFilename.IsEmpty())
		{
			CString diffWith;
			if (selectedLeafs.at(0)->GetFullName() == m_sMarkForDiffFilename)
				diffWith = m_sMarkForDiffVersion.ToString();
			else
			{
				PathCompactPathEx(CStrBuf(diffWith, 2 * GIT_HASH_SIZE), m_sMarkForDiffFilename, 2 * GIT_HASH_SIZE, 0);
				diffWith += L':' + m_sMarkForDiffVersion.ToString(g_Git.GetShortHASHLength());
			}
			CString menuEntry;
			menuEntry.Format(IDS_MENUDIFFNOW, static_cast<LPCTSTR>(diffWith));
			popupMenu.AppendMenuIcon(eCmd_PrepareDiff_Compare, menuEntry, IDI_DIFF);
		}
		popupMenu.AppendMenu(MF_SEPARATOR);
	}

	if (!selectedLeafs.empty())
	{
		popupMenu.AppendMenuIcon(eCmd_CopyPath, IDS_STATUSLIST_CONTEXT_COPY, IDI_COPYCLIP);
		popupMenu.AppendMenuIcon(eCmd_CopyHash, IDS_COPY_COMMIT_HASH, IDI_COPYCLIP);
	}

	eCmd cmd = static_cast<eCmd>(popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, nullptr));
	switch(cmd)
	{
	case eCmd_ViewLog:
	case eCmd_ViewLogSubmodule:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\"", static_cast<LPCTSTR>(g_Git.CombinePath(selectedLeafs.at(0)->GetFullName())));
			if (cmd == eCmd_ViewLog && selectedLeafs.at(0)->m_bSubmodule)
				sCmd += L" /submodule";
			if (cmd == eCmd_ViewLog)
				sCmd += L" /endrev:" + m_sRevision;
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
	case eCmd_Blame:
		{
			CAppUtils::LaunchTortoiseBlame(g_Git.CombinePath(selectedLeafs.at(0)->GetFullName()), m_sRevision);
		}
		break;
	case eCmd_Open:
		if (!selectedLeafs.at(0)->m_bSubmodule && selectedLeafs.at(0)->m_bFolder)
		{
			FillListCtrlForTreeNode(selectedLeafs.at(0)->m_hTree);
			m_RepoTree.SelectItem(selectedLeafs.at(0)->m_hTree);
			return;
		}
		OpenFile(selectedLeafs.at(0)->GetFullName(), OPEN, selectedLeafs.at(0)->m_bSubmodule, selectedLeafs.at(0)->m_hash);
		break;
	case eCmd_OpenWith:
		OpenFile(selectedLeafs.at(0)->GetFullName(), OPEN_WITH, selectedLeafs.at(0)->m_bSubmodule, selectedLeafs.at(0)->m_hash);
		break;
	case eCmd_OpenWithAlternativeEditor:
		OpenFile(selectedLeafs.at(0)->GetFullName(), ALTERNATIVEEDITOR, selectedLeafs.at(0)->m_bSubmodule, selectedLeafs.at(0)->m_hash);
		break;
	case eCmd_CompareWC:
		{
			CTGitPath file(selectedLeafs.at(0)->GetFullName());
			CGitDiff::Diff(GetSafeHwnd(), &file, &file, GIT_REV_ZERO, m_sRevision);
		}
		break;
	case eCmd_Revert:
		{
			int count = 0;
			for (auto itShadowTree = selectedLeafs.cbegin(); itShadowTree != selectedLeafs.cend(); ++itShadowTree)
			{
				if (RevertItemToVersion((*itShadowTree)->GetFullName()))
					++count;
				else
					break;
			}
			CString msg;
			msg.FormatMessage(IDS_STATUSLIST_FILESREVERTED, count, static_cast<LPCTSTR>(m_sRevision));
			MessageBox(msg, L"TortoiseGit", MB_OK);
		}
		break;
	case eCmd_SaveAs:
		FileSaveAs(selectedLeafs.at(0)->GetFullName());
		break;
	case eCmd_CopyPath:
		{
			CString sClipboard;
			for (auto itShadowTree = selectedLeafs.cbegin(); itShadowTree != selectedLeafs.cend(); ++itShadowTree)
			{
				sClipboard += (*itShadowTree)->m_sName + L"\r\n";
			}
			CStringUtils::WriteAsciiStringToClipboard(sClipboard);
		}
		break;
	case eCmd_CopyHash:
		{
			CopyHashToClipboard(selectedLeafs);
		}
		break;
	case eCmd_PrepareDiff:
		m_sMarkForDiffFilename = selectedLeafs.at(0)->GetFullName();
		if (g_Git.GetHash(m_sMarkForDiffVersion, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
		{
			m_sMarkForDiffFilename.Empty();
			MessageBox(g_Git.GetGitLastErr(L"Could not get SHA-1 for \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
		}
		break;
	case eCmd_PrepareDiff_Compare:
		{
			CTGitPath savedFile(m_sMarkForDiffFilename);
			CTGitPath selectedFile(selectedLeafs.at(0)->GetFullName());
			CGitHash currentHash;
			if (g_Git.GetHash(currentHash, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not get SHA-1 for \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
				return;
			}
			CGitDiff::Diff(GetSafeHwnd(), &selectedFile, &savedFile, currentHash.ToString(), m_sMarkForDiffVersion.ToString());
		}
		break;
	}
}

BOOL CRepositoryBrowser::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CRepositoryBrowser::OnLvnColumnclickRepoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pResult)
		*pResult = 0;

	if (m_currSortCol == pNMLV->iSubItem)
		m_currSortDesc = !m_currSortDesc;
	else
	{
		m_currSortCol  = pNMLV->iSubItem;
		m_currSortDesc = false;
	}

	CRepoListCompareFunc compareFunc(&m_RepoList, m_currSortCol, m_currSortDesc);
	m_RepoList.SortItemsEx(&CRepoListCompareFunc::StaticCompare, reinterpret_cast<DWORD_PTR>(&compareFunc));

	SetSortArrowA(&m_RepoList, m_currSortCol, !m_currSortDesc);
}

void CRepositoryBrowser::OnBnClickedButtonRevision()
{
		// use the git log to allow selection of a version
		CLogDlg dlg;
		dlg.SetParams(CTGitPath(), CTGitPath(), m_sRevision, m_sRevision, 0);
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		dlg.ShowWorkingTreeChanges(false);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
		{
			m_sRevision = dlg.GetSelectedHash().at(0).ToString();
			Refresh();
		}
}

void CRepositoryBrowser::SaveDividerPosition()
{
	RECT rc;
	GetDlgItem(IDC_REPOTREE)->GetClientRect(&rc);
	CRegDWORD xPos(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RepobrowserDivider");
	xPos = rc.right - rc.left;
}

void CRepositoryBrowser::HandleDividerMove(CPoint point, bool bDraw)
{
	RECT rect, tree, list, treelist, treelistclient;

	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);

	auto minWidth = CDPIAware::Instance().ScaleX(REPOBROWSER_CTRL_MIN_WIDTH);
	CPoint point2 = point;
	if (point2.x < treelist.left + minWidth)
		point2.x = treelist.left + minWidth;
	if (point2.x > treelist.right - minWidth)
		point2.x = treelist.right - minWidth;

	point.x -= rect.left;
	point.y -= treelist.top;

	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left + minWidth)
		point.x = treelist.left + minWidth;
	if (point.x > treelist.right - minWidth)
		point.x = treelist.right - minWidth;

	auto divWidth = CDPIAware::Instance().ScaleX(2);

	if (bDraw)
	{
		CDC * pDC = GetDC();
		DrawXorBar(pDC, oldx - divWidth, treelistclient.top, 2 * divWidth, treelistclient.bottom - treelistclient.top - CDPIAware::Instance().ScaleY(2));
		ReleaseDC(pDC);
	}

	oldx = point.x;
	oldy = point.y;

	//position the child controls
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&treelist);
	treelist.right = point2.x - divWidth;
	ScreenToClient(&treelist);
	RemoveAnchor(IDC_REPOTREE);
	GetDlgItem(IDC_REPOTREE)->MoveWindow(&treelist);
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&treelist);
	treelist.left = point2.x + divWidth;
	ScreenToClient(&treelist);
	RemoveAnchor(IDC_REPOLIST);
	GetDlgItem(IDC_REPOLIST)->MoveWindow(&treelist);

	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
}

void CRepositoryBrowser::OnMouseMove(UINT nFlags, CPoint point)
{
	if (bDragMode == FALSE)
		return;

	RECT rect, tree, list, treelist, treelistclient;
	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	auto minWidth = CDPIAware::Instance().ScaleX(REPOBROWSER_CTRL_MIN_WIDTH);
	if (point.x < treelist.left + minWidth)
		point.x = treelist.left + minWidth;
	if (point.x > treelist.right - minWidth)
		point.x = treelist.right - minWidth;

	if ((nFlags & MK_LBUTTON) && (point.x != oldx))
	{
		CDC * pDC = GetDC();

		if (pDC)
		{
			auto divWidth = CDPIAware::Instance().ScaleX(2);
			auto divHeight = CDPIAware::Instance().ScaleY(2);
			DrawXorBar(pDC, oldx - divWidth, treelistclient.top, 2 * divWidth, treelistclient.bottom - treelistclient.top - divHeight);
			DrawXorBar(pDC, point.x - divWidth, treelistclient.top, 2 * divWidth, treelistclient.bottom - treelistclient.top - divHeight);

			ReleaseDC(pDC);
		}

		oldx = point.x;
		oldy = point.y;
	}

	CStandAloneDialogTmpl<CResizableDialog>::OnMouseMove(nFlags, point);
}

void CRepositoryBrowser::OnLButtonDown(UINT nFlags, CPoint point)
{
	RECT rect, tree, list, treelist, treelistclient;

	// create an union of the tree and list control rectangle
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	ScreenToClient(&treelistclient);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(&point);
	GetClientRect(&rect);
	ClientToScreen(&rect);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	auto minWidth = CDPIAware::Instance().ScaleX(REPOBROWSER_CTRL_MIN_WIDTH);

	if (point.x < treelist.left + minWidth)
		return CStandAloneDialogTmpl < CResizableDialog>::OnLButtonDown(nFlags, point);
	if (point.x > treelist.right - CDPIAware::Instance().ScaleX(3))
		return CStandAloneDialogTmpl < CResizableDialog>::OnLButtonDown(nFlags, point);
	if (point.x > treelist.right - minWidth)
		point.x = treelist.right - minWidth;

	auto divHeight = CDPIAware::Instance().ScaleY(3);
	if ((point.y < treelist.top + divHeight) || (point.y > treelist.bottom - divHeight))
		return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

	bDragMode = true;

	SetCapture();

	auto divWidth = CDPIAware::Instance().ScaleX(2);
	CDC * pDC = GetDC();
	DrawXorBar(pDC, point.x - divWidth, treelistclient.top, 2 * divWidth, treelistclient.bottom - treelistclient.top - CDPIAware::Instance().ScaleY(2));
	ReleaseDC(pDC);

	oldx = point.x;
	oldy = point.y;

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
}

void CRepositoryBrowser::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (bDragMode == FALSE)
		return;

	HandleDividerMove(point, true);

	bDragMode = false;
	ReleaseCapture();

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonUp(nFlags, point);
}

void CRepositoryBrowser::OnCaptureChanged(CWnd *pWnd)
{
	bDragMode = false;

	__super::OnCaptureChanged(pWnd);
}

void CRepositoryBrowser::DrawXorBar(CDC * pDC, int x1, int y1, int width, int height)
{
	static WORD _dotPatternBmp[8] =
	{
		0x0055, 0x00aa, 0x0055, 0x00aa,
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	pDC->SetBrushOrg(x1, y1);
	hbrushOld = static_cast<HBRUSH>(pDC->SelectObject(hbr));

	PatBlt(pDC->GetSafeHdc(), x1, y1, width, height, PATINVERT);

	pDC->SelectObject(hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}

BOOL CRepositoryBrowser::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd == this)
	{
		RECT rect;
		POINT pt;
		GetClientRect(&rect);
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		if (PtInRect(&rect, pt))
		{
			ClientToScreen(&pt);
			// are we right of the tree control?
			GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rect);
			auto divHeight = CDPIAware::Instance().ScaleY(3);
			if ((pt.x > rect.right) && (pt.y >= rect.top + divHeight) && (pt.y <= rect.bottom - divHeight))
			{
				// but left of the list control?
				GetDlgItem(IDC_REPOLIST)->GetWindowRect(&rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(nullptr, IDC_SIZEWE);
					SetCursor(hCur);
					return TRUE;
				}
			}
		}
	}
	return CStandAloneDialogTmpl<CResizableDialog>::OnSetCursor(pWnd, nHitTest, message);
}

void CRepositoryBrowser::FileSaveAs(const CString path)
{
	CTGitPath gitPath(path);

	CGitHash hash;
	if (g_Git.GetHash(hash, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	CString filename;
	filename.Format(L"%s\\%s-%s%s", static_cast<LPCTSTR>(g_Git.CombinePath(gitPath.GetContainingDirectory())), static_cast<LPCTSTR>(gitPath.GetBaseFilename()), static_cast<LPCTSTR>(hash.ToString(g_Git.GetShortHASHLength())), static_cast<LPCTSTR>(gitPath.GetFileExtension()));
	if (!CAppUtils::FileOpenSave(filename, nullptr, 0, 0, false, GetSafeHwnd()))
		return;

	if (g_Git.GetOneFile(m_sRevision, gitPath, filename))
	{
		CString out;
		out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCTSTR>(gitPath.GetGitPathString()), static_cast<LPCTSTR>(m_sRevision), static_cast<LPCTSTR>(filename));
		MessageBox(g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_ICONERROR);
	}
}

void CRepositoryBrowser::OpenFile(const CString path, eOpenType mode, bool isSubmodule, const CGitHash& itemHash)
{
	CTGitPath gitPath(path);

	CGitHash hash;
	if (g_Git.GetHash(hash, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	CString file = CTempFiles::Instance().GetTempFilePath(false, gitPath, hash).GetWinPathString();
	if (isSubmodule)
	{
		if (mode == OPEN && !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
		{
			CTGitPath subPath = CTGitPath(g_Git.m_CurrentDir);
			subPath.AppendPathString(gitPath.GetWinPathString());
			CAutoRepository repo(subPath.GetGitPathString());
			CAutoCommit commit;
			if (!repo || git_commit_lookup(commit.GetPointer(), repo, itemHash))
			{
				CString out;
				out.FormatMessage(IDS_REPOBROWSEASKSUBMODULEUPDATE, static_cast<LPCTSTR>(itemHash.ToString()), static_cast<LPCTSTR>(gitPath.GetGitPathString()));
				if (MessageBox(out, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) != IDYES)
					return;

				CString sCmd;
				sCmd.Format(L"/command:subupdate /bkpath:\"%s\" /selectedpath:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(gitPath.GetGitPathString()));
				CAppUtils::RunTortoiseGitProc(sCmd);
				return;
			}

			CString cmd;
			cmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%s", static_cast<LPCTSTR>(g_Git.CombinePath(path)), static_cast<LPCTSTR>(itemHash.ToString()));
			CAppUtils::RunTortoiseGitProc(cmd);
			return;
		}

		file += L".txt";
		CFile submoduleCommit(file, CFile::modeCreate | CFile::modeWrite);
		CStringA commitInfo = "Subproject commit " + CStringA(itemHash.ToString());
		submoduleCommit.Write(commitInfo, commitInfo.GetLength());
	}
	else if (g_Git.GetOneFile(m_sRevision, gitPath, file))
	{
		CString out;
		out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCTSTR>(gitPath.GetGitPathString()), static_cast<LPCTSTR>(m_sRevision), static_cast<LPCTSTR>(file));
		MessageBox(g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	if (mode == ALTERNATIVEEDITOR)
	{
		CAppUtils::LaunchAlternativeEditor(file);
		return;
	}
	else if (mode == OPEN)
	{
		CAppUtils::ShellOpen(file);
		return;
	}

	CAppUtils::ShowOpenWithDialog(file);
}
bool CRepositoryBrowser::RevertItemToVersion(const CString &path)
{
	CString cmd, out;
	cmd.Format(L"git.exe checkout %s -- \"%s\"", static_cast<LPCTSTR>(m_sRevision), static_cast<LPCTSTR>(path));
	if (g_Git.Run(cmd, &out, CP_UTF8))
	{
		if (MessageBox(out, L"TortoiseGit", MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
			return false;
	}

	return true;
}

void CRepositoryBrowser::CopyHashToClipboard(TShadowFilesTreeList &selectedLeafs)
{
	if (!selectedLeafs.empty())
	{
		CString sClipdata;
		bool first = true;
		for (size_t i = 0; i < selectedLeafs.size(); ++i)
		{
			if (!first)
				sClipdata += L"\r\n";
			sClipdata += selectedLeafs[i]->m_hash.ToString();
			first = false;
		}
		CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
	}
}

void CRepositoryBrowser::OnLvnBegindragRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	// get selected paths
	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	if (!pos)
		return;

	HTREEITEM hTreeItem = m_RepoTree.GetSelectedItem();
	if (!hTreeItem)
	{
		ASSERT(FALSE);
		return;
	}
	auto pTree = GetTreeEntry(hTreeItem);
	if (!pTree)
		return;

	CTGitPathList toExport;
	int index = -1;
	while ((index = m_RepoList.GetNextSelectedItem(pos)) >= 0)
	{
		auto item = GetListEntry(index);
		if (item->m_bFolder)
		{
			RecursivelyAdd(toExport, item);
			continue;
		}

		CTGitPath path;
		path.SetFromGit(item->GetFullName(), item->m_bSubmodule);
		toExport.AddPath(path);
	}

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	BeginDrag(m_RepoList, toExport, pTree->GetFullName(), pNMLV->ptAction);
}

void CRepositoryBrowser::RecursivelyAdd(CTGitPathList& toExport, CShadowFilesTree* pTree)
{
	if (!pTree->m_bLoaded)
		ReadTree(pTree, pTree->GetFullName(), true);

	for (auto itShadowTree = pTree->m_ShadowTree.begin(); itShadowTree != pTree->m_ShadowTree.end(); ++itShadowTree)
	{
		if ((*itShadowTree).second.m_bFolder)
		{
			RecursivelyAdd(toExport, &(*itShadowTree).second);
			continue;
		}
		CTGitPath path;
		path.SetFromGit((*itShadowTree).second.GetFullName(), (*itShadowTree).second.m_bSubmodule);
		toExport.AddPath(path);
	}
}

void CRepositoryBrowser::OnTvnBegindragRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	auto pTree = GetTreeEntry(pNMTreeView->itemNew.hItem);
	if (!pTree)
		return;

	CTGitPathList toExport;
	RecursivelyAdd(toExport, pTree);

	BeginDrag(m_RepoTree, toExport, pTree->m_pParent ? pTree->m_pParent->GetFullName() : L"", pNMTreeView->ptDrag);
}

void CRepositoryBrowser::BeginDrag(const CWnd& window, CTGitPathList& files, const CString& root, POINT& point)
{
	CGitHash hash;
	if (g_Git.GetHash(hash, m_sRevision + L"^{}")) // add ^{} in order to dereference signed tags
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_sRevision + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	// build copy source / content
	auto pdsrc = std::make_unique<CIDropSource>();
	if (!pdsrc)
		return;

	pdsrc->AddRef();

	GitDataObject* pdobj = new GitDataObject(files, hash, root.GetLength());
	if (!pdobj)
		return;
	pdobj->AddRef();
	pdobj->SetAsyncMode(TRUE);
	CDragSourceHelper dragsrchelper;
	dragsrchelper.InitializeFromWindow(window.GetSafeHwnd(), point, pdobj);
	pdsrc->m_pIDataObj = pdobj;
	pdsrc->m_pIDataObj->AddRef();

	// Initiate the Drag & Drop
	DWORD dwEffect;
	::DoDragDrop(pdobj, pdsrc.get(), DROPEFFECT_MOVE | DROPEFFECT_COPY, &dwEffect);
	pdsrc->Release();
	pdsrc.release();
	pdobj->Release();
}


CShadowFilesTree* CRepositoryBrowser::GetListEntry(int index)
{
	auto entry = reinterpret_cast<CShadowFilesTree*>(m_RepoList.GetItemData(index));
	ASSERT(entry);
	return entry;
}

CShadowFilesTree* CRepositoryBrowser::GetTreeEntry(HTREEITEM treeItem)
{
	auto entry = reinterpret_cast<CShadowFilesTree*>(m_RepoTree.GetItemData(treeItem));
	ASSERT(entry);
	return entry;
}

void CRepositoryBrowser::OnSysColorChange()
{
	__super::OnSysColorChange();
	CAppUtils::SetListCtrlBackgroundImage(m_RepoList.GetSafeHwnd(), IDI_REPOBROWSER_BKG);
}
