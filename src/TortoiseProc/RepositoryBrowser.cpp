// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
// Copyright (C) 2012 Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2003-2012 - TortoiseSVN

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
#include "SysInfo.h"
#include "registry.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "GitDiff.h"

void SetSortArrowA(CListCtrl * control, int nColumn, bool bAscending)
{
	if (control == NULL)
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
		return ((CRepoListCompareFunc *) lParamSort)->Compare(lParam1, lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		CShadowFilesTree * pLeft	= (CShadowFilesTree *)m_pList->GetItemData((int)lParam1);
		CShadowFilesTree * pRight	= (CShadowFilesTree *)m_pList->GetItemData((int)lParam2);

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
	int SortStrCmp(CString &left, CString &right)
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

CRepositoryBrowser::CRepositoryBrowser(CString rev, CWnd* pParent /*=NULL*/)
: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
, m_currSortCol(0)
, m_currSortDesc(false)
, m_sRevision(rev)
, m_bHasWC(true)
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
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_REPOLIST, &CRepositoryBrowser::OnLvnColumnclickRepoList)
	ON_NOTIFY(NM_DBLCLK, IDC_REPOLIST, &CRepositoryBrowser::OnNMDblclk_RepoList)
	ON_BN_CLICKED(IDC_BUTTON_REVISION, &CRepositoryBrowser::OnBnClickedButtonRevision)
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
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
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	CRepositoryBrowser::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (CRepositoryBrowser::s_bSortLogical)
		CRepositoryBrowser::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);

	CString temp;
	temp.LoadString(IDS_STATUSLIST_COLFILENAME);
	m_RepoList.InsertColumn(eCol_Name, temp, 0, 150);
	temp.LoadString(IDS_STATUSLIST_COLEXT);
	m_RepoList.InsertColumn(eCol_Extension, temp, 0, 100);
	temp.LoadString(IDS_LOG_SIZE);
	m_RepoList.InsertColumn(eCol_FileSize, temp, 0, 100);

	// set up the list control
	// set the extended style of the list control
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(_T("Software\\TortoiseGit\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	m_RepoList.SetExtendedStyle(exStyle);
	m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	CAppUtils::SetListCtrlBackgroundImage(m_RepoList.GetSafeHwnd(), IDI_REPOBROWSER_BKG);

	m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
	if (SysInfo::Instance().IsVistaOrLater())
	{
		DWORD exStyle = TVS_EX_FADEINOUTEXPANDOS | TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
		m_RepoTree.SetExtendedStyle(exStyle, exStyle);
	}

	SetWindowTheme(m_RepoTree.GetSafeHwnd(), L"Explorer", NULL);
	SetWindowTheme(m_RepoList.GetSafeHwnd(), L"Explorer", NULL);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();

	EnableSaveRestore(L"Reposbrowser");

	DWORD xPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RepobrowserDivider"), 0);
	if (xPos == 0)
	{
		RECT rc;
		GetDlgItem(IDC_REPOTREE)->GetClientRect(&rc);
		xPos = rc.right - rc.left;
	}
	bDragMode = false;
	HandleDividerMove(CPoint(xPos + 20, 10), false);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_bHasWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);

	Refresh();

	m_RepoList.SetFocus();

	return FALSE;
}

void CRepositoryBrowser::OnOK()
{
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
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = 0;

	LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNmItemActivate->iItem < 0)
		return;

	CShadowFilesTree * pItem = (CShadowFilesTree *)m_RepoList.GetItemData(pNmItemActivate->iItem);
	if (pItem == NULL)
		return;

	if (!pItem->m_bFolder)
	{
		OpenFile(pItem->GetFullName(), OPEN);
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
	m_RepoTree.DeleteAllItems();
	m_RepoList.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_sName = "";
	m_TreeRoot.m_bFolder = true;

	TVINSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = TVI_ROOT;
	tvinsert.hInsertAfter = TVI_ROOT;
	tvinsert.itemex.mask = TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvinsert.itemex.pszText = L"/";
	tvinsert.itemex.lParam = (LPARAM)&m_TreeRoot;
	tvinsert.itemex.iImage = m_nIconFolder;
	tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
	m_TreeRoot.m_hTree= m_RepoTree.InsertItem(&tvinsert);

	ReadTree(&m_TreeRoot);
	m_RepoTree.Expand(m_TreeRoot.m_hTree, TVE_EXPAND);
	FillListCtrlForShadowTree(&m_TreeRoot);
	m_RepoTree.SelectItem(m_TreeRoot.m_hTree);
}

int CRepositoryBrowser::ReadTreeRecursive(git_repository &repo, git_tree * tree, CShadowFilesTree * treeroot)
{
	size_t count = git_tree_entrycount(tree);

	for (int i = 0; i < count; i++)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (entry == NULL)
			continue;
		int mode = git_tree_entry_filemode(entry);
		
		CString base = CUnicodeUtils::GetUnicode(git_tree_entry_name(entry), CP_UTF8);

		git_object *object = NULL;
		git_tree_entry_to_object(&object, &repo, entry);
		if (object == NULL)
			continue;

		CShadowFilesTree * pNextTree = &treeroot->m_ShadowTree[base];
		pNextTree->m_sName = base;
		pNextTree->m_pParent = treeroot;

		if (mode & S_IFDIR)
		{
			pNextTree->m_bFolder = true;

			TVINSERTSTRUCT tvinsert = {0};
			tvinsert.hParent = treeroot->m_hTree;
			tvinsert.hInsertAfter = TVI_SORT;
			tvinsert.itemex.mask = TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
			tvinsert.itemex.pszText = base.GetBuffer(base.GetLength());
			tvinsert.itemex.lParam = (LPARAM)pNextTree;
			tvinsert.itemex.iImage = m_nIconFolder;
			tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
			pNextTree->m_hTree = m_RepoTree.InsertItem(&tvinsert);
			base.ReleaseBuffer();
				
			ReadTreeRecursive(repo, (git_tree*)object, pNextTree);
		}
		else
		{
			const git_oid *	oid = git_object_id(object);

			git_blob * blob;
			git_blob_lookup(&blob, &repo, oid);
			if (blob == NULL)
				continue;
	
			pNextTree->m_iSize = git_blob_rawsize(blob);
			git_blob_free(blob);
		}

		git_object_free(object);
	}

	return 0;
}

int CRepositoryBrowser::ReadTree(CShadowFilesTree * treeroot)
{
	CStringA gitdir = CUnicodeUtils::GetMulti(g_Git.m_CurrentDir, CP_UTF8);
	git_repository *repository = NULL;
	git_commit *commit = NULL;
	git_tree * tree = NULL;
	int ret = 0;
	do
	{
		ret = git_repository_open(&repository, gitdir.GetBuffer());
		if (ret)
		{
			const git_error * err = giterr_last();
			MessageBox(_T("Could not open repository.\nlibgit2 reports: ") + CString(err->message), _T("TortoiseGit"), MB_ICONERROR);
			break;
		}

		CGitHash hash = g_Git.GetHash(m_sRevision);
		ret = git_commit_lookup(&commit, repository, (git_oid *) hash.m_hash);
		if (ret)
		{
			const git_error * err = giterr_last();
			MessageBox(_T("Could not lookup commit.\nlibgit2 reports: ") + CString(err->message), _T("TortoiseGit"), MB_ICONERROR);
			break;
		}

		ret = git_commit_tree(&tree, commit);
		if (ret)
		{
			const git_error * err = giterr_last();
			MessageBox(_T("Could get tree of commit.\nlibgit2 reports: ") + CString(err->message), _T("TortoiseGit"), MB_ICONERROR);
			break;
		}

		ReadTreeRecursive(*repository, tree, treeroot);

		// try to resolve hash to a branch name
		if (m_sRevision == hash.ToString())
		{
			MAP_HASH_NAME map;
			g_Git.GetMapHashToFriendName(map);
			if (!map[hash].empty())
				m_sRevision = map[hash].at(0);
		}
		this->GetDlgItem(IDC_BUTTON_REVISION)->SetWindowText(m_sRevision);
	} while(0);

	if (tree)
		git_tree_free(tree);

	if (commit)
		git_commit_free(commit);

	if (repository)
		git_repository_free(repository);

	return ret;
}

void CRepositoryBrowser::OnTvnSelchangedRepoTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	FillListCtrlForTreeNode(pNMTreeView->itemNew.hItem);
}

void CRepositoryBrowser::FillListCtrlForTreeNode(HTREEITEM treeNode)
{
	m_RepoList.DeleteAllItems();

	CShadowFilesTree* pTree = (CShadowFilesTree*)(m_RepoTree.GetItemData(treeNode));
	if (pTree == NULL)
	{
		ASSERT(FALSE);
		return;
	}

	CString url = _T("/") + pTree->GetFullName();
	GetDlgItem(IDC_REPOBROWSER_URL)->SetWindowText(url);

	FillListCtrlForShadowTree(pTree);
}

void CRepositoryBrowser::FillListCtrlForShadowTree(CShadowFilesTree* pTree)
{
	for (TShadowFilesTreeMap::iterator itShadowTree = pTree->m_ShadowTree.begin(); itShadowTree != pTree->m_ShadowTree.end(); ++itShadowTree)
	{
		int icon = m_nIconFolder;
		if (!(*itShadowTree).second.m_bFolder)
			icon = SYS_IMAGE_LIST().GetFileIconIndex((*itShadowTree).second.m_sName);

		int indexItem = m_RepoList.InsertItem(m_RepoList.GetItemCount(), (*itShadowTree).second.m_sName, icon);

		m_RepoList.SetItemData(indexItem, (DWORD_PTR)&(*itShadowTree).second);
		if (!(*itShadowTree).second.m_bFolder)
		{
			CString temp;

			temp = CPathUtils::GetFileExtFromPath((*itShadowTree).second.m_sName);
			m_RepoList.SetItemText(indexItem, eCol_Extension, temp);

			StrFormatByteSize((*itShadowTree).second.m_iSize, temp.GetBuffer(20), 20);
			temp.ReleaseBuffer();
			m_RepoList.SetItemText(indexItem, eCol_FileSize, temp);
		}
	}

	CRepoListCompareFunc compareFunc(&m_RepoList, m_currSortCol, m_currSortDesc);
	m_RepoList.SortItemsEx(&CRepoListCompareFunc::StaticCompare, (DWORD_PTR)&compareFunc);

	SetSortArrowA(&m_RepoList, m_currSortCol, !m_currSortDesc);
}

void CRepositoryBrowser::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if (pWndFrom == &m_RepoList)
		OnContextMenu_RepoList(point);
	else if (pWndFrom == &m_RepoTree)
		OnContextMenu_RepoTree(point);
}

void CRepositoryBrowser::OnContextMenu_RepoTree(CPoint point)
{
	CPoint clientPoint = point;
	m_RepoTree.ScreenToClient(&clientPoint);

	HTREEITEM hTreeItem = m_RepoTree.HitTest(clientPoint);
	if (hTreeItem == NULL)
		return;

	TShadowFilesTreeList selectedLeafs;
	selectedLeafs.push_back((CShadowFilesTree *)m_RepoTree.GetItemData(hTreeItem));

	ShowContextMenu(point, selectedLeafs, ONLY_FOLDERS);
}

void CRepositoryBrowser::OnContextMenu_RepoList(CPoint point)
{
	TShadowFilesTreeList selectedLeafs;
	selectedLeafs.reserve(m_RepoList.GetSelectedCount());

	bool folderSelected = false;
	bool filesSelected = false;

	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	while (pos)
	{
		CShadowFilesTree * item = (CShadowFilesTree *)m_RepoList.GetItemData(m_RepoList.GetNextSelectedItem(pos));
		if (item->m_bFolder)
			folderSelected = true;
		else
			filesSelected = true;
		selectedLeafs.push_back(item);
	}

	eSelectionType selType = ONLY_FILES;
	if (folderSelected && filesSelected)
		selType = MIXED;
	else if (folderSelected)
		selType = ONLY_FOLDERS;

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
		if (selType == ONLY_FILES)
		{
			popupMenu.AppendMenuIcon(eCmd_OpenWith, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
			popupMenu.AppendMenuIcon(eCmd_OpenWithAlternativeEditor, IDS_LOG_POPUP_VIEWREV);
		}

		popupMenu.AppendMenu(MF_SEPARATOR);

		if (m_bHasWC && selType == ONLY_FILES)
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

	if (selType == ONLY_FILES && m_bHasWC)
	{
		popupMenu.AppendMenuIcon(eCmd_Revert, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
		bAddSeparator = true;
	}

	if (bAddSeparator)
		popupMenu.AppendMenu(MF_SEPARATOR);
	bAddSeparator = false;

	popupMenu.AppendMenuIcon(eCmd_CopyPath, IDS_STATUSLIST_CONTEXT_COPY, IDI_COPYCLIP);

	eCmd cmd = (eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_ViewLog:
		{
			CString sCmd;
			sCmd.Format(_T("/command:log /path:\"%s\\%s\""), g_Git.m_CurrentDir, selectedLeafs.at(0)->GetFullName());
			CAppUtils::RunTortoiseProc(sCmd);
		}
		break;
	case eCmd_Blame:
		{
			CAppUtils::LaunchTortoiseBlame(g_Git.m_CurrentDir + _T("\\") + selectedLeafs.at(0)->GetFullName(), m_sRevision);
		}
		break;
	case eCmd_Open:
		if (selectedLeafs.at(0)->m_bFolder)
		{
			FillListCtrlForTreeNode(selectedLeafs.at(0)->m_hTree);
			m_RepoTree.SelectItem(selectedLeafs.at(0)->m_hTree);
			return;
		}
		OpenFile(selectedLeafs.at(0)->GetFullName(), OPEN);
		break;
	case eCmd_OpenWith:
		OpenFile(selectedLeafs.at(0)->GetFullName(), OPEN_WITH);
		break;
	case eCmd_OpenWithAlternativeEditor:
		OpenFile(selectedLeafs.at(0)->GetFullName(), ALTERNATIVEEDITOR);
		break;
	case eCmd_CompareWC:
		{
			CTGitPath file(selectedLeafs.at(0)->GetFullName());
			CGitDiff::Diff(&file, &file, GIT_REV_ZERO, m_sRevision);
		}
		break;
	case eCmd_Revert:
		{
			int count = 0;
			for (TShadowFilesTreeList::iterator itShadowTree = selectedLeafs.begin(); itShadowTree != selectedLeafs.end(); ++itShadowTree)
			{
				if (RevertItemToVersion((*itShadowTree)->GetFullName()))
					count++;
				else
					break;
			}
			CString msg;
			msg.Format(IDS_STATUSLIST_FILESREVERTED, count, m_sRevision);
			MessageBox(msg, _T("TortoiseGit"), MB_OK);
		}
		break;
	case eCmd_SaveAs:
		FileSaveAs(selectedLeafs.at(0)->GetFullName());
		break;
	case eCmd_CopyPath:
		{
			CString sClipboard;
			for (TShadowFilesTreeList::iterator itShadowTree = selectedLeafs.begin(); itShadowTree != selectedLeafs.end(); ++itShadowTree)
			{
				sClipboard += (*itShadowTree)->m_sName + _T("\r\n");
			}
			CStringUtils::WriteAsciiStringToClipboard(sClipboard);
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
	m_RepoList.SortItemsEx(&CRepoListCompareFunc::StaticCompare, (DWORD_PTR)&compareFunc);

	SetSortArrowA(&m_RepoList, m_currSortCol, !m_currSortDesc);
}

void CRepositoryBrowser::OnBnClickedButtonRevision()
{
		// use the git log to allow selection of a version
		CLogDlg dlg;
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		if (dlg.DoModal() == IDOK)
		{
			// get selected hash if any
			m_sRevision = dlg.GetSelectedHash();
			Refresh();
		}
}

void CRepositoryBrowser::SaveDividerPosition()
{
	RECT rc;
	GetDlgItem(IDC_REPOTREE)->GetClientRect(&rc);
	CRegDWORD xPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RepobrowserDivider"));
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

	CPoint point2 = point;
	if (point2.x < treelist.left + REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.left + REPOBROWSER_CTRL_MIN_WIDTH;
	if (point2.x > treelist.right - REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.right - REPOBROWSER_CTRL_MIN_WIDTH;

	point.x -= rect.left;
	point.y -= treelist.top;

	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if (bDraw)
	{
		CDC * pDC = GetDC();
		DrawXorBar(pDC, oldx + 2, treelistclient.top, 4, treelistclient.bottom - treelistclient.top - 2);
		ReleaseDC(pDC);
	}

	oldx = point.x;
	oldy = point.y;

	//position the child controls
	GetDlgItem(IDC_REPOTREE)->GetWindowRect(&treelist);
	treelist.right = point2.x - 2;
	ScreenToClient(&treelist);
	RemoveAnchor(IDC_REPOTREE);
	GetDlgItem(IDC_REPOTREE)->MoveWindow(&treelist);
	GetDlgItem(IDC_REPOLIST)->GetWindowRect(&treelist);
	treelist.left = point2.x + 2;
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

	if (point.x < treelist.left + REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left + REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right - REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.right - REPOBROWSER_CTRL_MIN_WIDTH;

	if ((nFlags & MK_LBUTTON) && (point.x != oldx))
	{
		CDC * pDC = GetDC();

		if (pDC)
		{
			DrawXorBar(pDC, oldx + 2, treelistclient.top, 4, treelistclient.bottom - treelistclient.top - 2);
			DrawXorBar(pDC, point.x + 2, treelistclient.top, 4, treelistclient.bottom - treelistclient.top - 2);

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

	if (point.x < treelist.left + REPOBROWSER_CTRL_MIN_WIDTH)
		return CStandAloneDialogTmpl < CResizableDialog>::OnLButtonDown(nFlags, point);
	if (point.x > treelist.right - 3)
		return CStandAloneDialogTmpl < CResizableDialog>::OnLButtonDown(nFlags, point);
	if (point.x > treelist.right - REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.right - REPOBROWSER_CTRL_MIN_WIDTH;

	if ((point.y < treelist.top + 3) || (point.y > treelist.bottom - 3))
		return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

	bDragMode = true;

	SetCapture();

	CDC * pDC = GetDC();
	DrawXorBar(pDC, point.x + 2, treelistclient.top, 4, treelistclient.bottom - treelistclient.top - 2);
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
	hbrushOld = (HBRUSH)pDC->SelectObject(hbr);

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
			if ((pt.x > rect.right) && (pt.y >= rect.top + 3) && (pt.y <= rect.bottom - 3))
			{
				// but left of the list control?
				GetDlgItem(IDC_REPOLIST)->GetWindowRect(&rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
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

	CString filename;
	filename.Format(_T("%s-%s%s"), gitPath.GetBaseFilename(), CGitHash(m_sRevision).ToString().Left(g_Git.GetShortHASHLength()), gitPath.GetFileExtension());
	CFileDialog dlg(FALSE, NULL, filename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL);

	CString cmd, out;
	INT_PTR ret = dlg.DoModal();
	SetCurrentDirectory(g_Git.m_CurrentDir);
	if (ret == IDOK)
	{
		filename = dlg.GetPathName();
		if (g_Git.GetOneFile(m_sRevision, gitPath, filename))
		{
			out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, gitPath.GetGitPathString(), m_sRevision, filename);
			MessageBox(out, _T("TortoiseGit"), MB_OK);
			return;
		}
	}
}

void CRepositoryBrowser::OpenFile(const CString path, eOpenType mode)
{
	CTGitPath gitPath(path);

	CString temppath;
	CString file;
	GetTempPath(temppath);
	file.Format(_T("%s%s_%s%s"), temppath, gitPath.GetBaseFilename(), CGitHash(m_sRevision).ToString().Left(g_Git.GetShortHASHLength()), gitPath.GetFileExtension());

	CString out;
	if(g_Git.GetOneFile(m_sRevision, gitPath, file))
	{
		out.Format(IDS_STATUSLIST_CHECKOUTFILEFAILED, gitPath.GetGitPathString(), m_sRevision, file);
		MessageBox(out, _T("TortoiseGit"), MB_OK);
		return;
	}

	if (mode == ALTERNATIVEEDITOR)
	{
		CAppUtils::LaunchAlternativeEditor(file);
		return;
	}
	else if (mode == OPEN)
	{
		int ret = HINSTANCE_ERROR;
		ret = (int)ShellExecute(this->m_hWnd, NULL, file, NULL, NULL, SW_SHOW);

		if (ret > HINSTANCE_ERROR)
			return;
	}

	CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ") + file;
	CAppUtils::LaunchApplication(cmd, NULL, false);
}
bool CRepositoryBrowser::RevertItemToVersion(const CString &path)
{
	CString cmd, out;	
	cmd.Format(_T("git.exe checkout %s -- \"%s\""), m_sRevision, path);
	if (g_Git.Run(cmd, &out, CP_UTF8))
	{
		if (MessageBox(out, _T("TortoiseGit"), MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
			return false;
	}

	return true;
}