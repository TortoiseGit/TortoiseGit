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

// CRepositoryBrowser dialog

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(CString rev, CWnd* pParent /*=NULL*/)
: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
, m_sRevision(rev)
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
	ON_BN_CLICKED(IDC_BUTTON_REVISION, &CRepositoryBrowser::OnBnClickedButtonRevision)
END_MESSAGE_MAP()


// CRepositoryBrowser message handlers

BOOL CRepositoryBrowser::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_STATIC, TOP_LEFT);
	AddAnchor(IDC_BUTTON_REVISION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	m_RepoList.SetExtendedStyle(m_RepoList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	CString temp;
	temp.LoadString(IDS_STATUSLIST_COLFILENAME);
	m_RepoList.InsertColumn(eCol_Name, temp, 0, 150);
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

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	Refresh();

	m_RepoList.SetFocus();

	return FALSE;
}

void CRepositoryBrowser::OnOK()
{
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
	CResizableStandAloneDialog::OnCancel();
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
		int mode = git_tree_entry_attributes(entry);
		
		CString base = CUnicodeUtils::GetUnicode(git_tree_entry_name(entry), CP_UTF8);

		git_object *object = NULL;
		git_tree_entry_2object(&object, &repo, entry);
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
		if(ret)
			break;

		CGitHash hash = g_Git.GetHash(m_sRevision);
		ret = git_commit_lookup(&commit, repository, (git_oid *) hash.m_hash);
		if(ret)
			break;

		ret = git_commit_tree(&tree, commit);
		if(ret)
			break;

		ret = ReadTreeRecursive(*repository, tree, treeroot);
		if(ret)
			break;

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
			StrFormatByteSize((*itShadowTree).second.m_iSize, temp.GetBuffer(20), 20);
			temp.ReleaseBuffer();
			m_RepoList.SetItemText(indexItem, eCol_FileSize, temp);
		}
	}
}

void CRepositoryBrowser::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if (pWndFrom == &m_RepoList)
		OnContextMenu_RepoList(point);
}

void CRepositoryBrowser::OnContextMenu_RepoList(CPoint point)
{
	TShadowFilesTreeList selectedLeafs;
	selectedLeafs.reserve(m_RepoList.GetSelectedCount());
	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	while (pos)
	{
		selectedLeafs.push_back((CShadowFilesTree *)m_RepoList.GetItemData(m_RepoList.GetNextSelectedItem(pos)));
	}

	CIconMenu popupMenu;
	popupMenu.CreatePopupMenu();

	bool bAddSeparator = false;

	if (selectedLeafs.size() == 1)
	{
		CString temp;
		temp.LoadString(IDS_MENULOG);
		popupMenu.AppendMenuIcon(eCmd_ViewLog, temp, IDI_LOG);
		bAddSeparator = true;
	}

	eCmd cmd = (eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_ViewLog:
		{
			CString sCmd;
			sCmd.Format(_T("/command:log /path:\"%s%s\""), g_Git.m_CurrentDir, selectedLeafs.at(0)->GetFullName());
			CAppUtils::RunTortoiseProc(sCmd);
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
