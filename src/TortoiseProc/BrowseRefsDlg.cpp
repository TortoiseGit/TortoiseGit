// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit

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
// BrowseRefsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "BrowseRefsDlg.h"
#include "LogDlg.h"
#include "AddRemoteDlg.h"
#include "AppUtils.h"
#include "Settings/SettingGitRemote.h"
#include "SinglePropSheetDlg.h"
#include "MessageBox.h"
#include "RefLogDlg.h"
#include "IconMenu.h"
#include "FileDiffDlg.h"
#include "DeleteRemoteTagDlg.h"
#include "UnicodeUtils.h"
#include "InputDlg.h"
#include "SysProgressDlg.h"
#include "LoglistUtils.h"
#include "GitRevRefBrowser.h"
#include "StringUtils.h"
#include "BrowseRefsDlgFilter.h"

static int SplitRemoteBranchName(CString ref, CString &remote, CString &branch)
{
	if (CStringUtils::StartsWith(ref, L"refs/remotes/"))
		ref = ref.Mid(static_cast<int>(wcslen(L"refs/remotes/")));
	else if (CStringUtils::StartsWith(ref, L"remotes/"))
		ref = ref.Mid(static_cast<int>(wcslen(L"remotes/")));

	STRING_VECTOR list;
	int result = g_Git.GetRemoteList(list);
	if (result != 0)
		return result;

	for (size_t i = 0; i < list.size(); ++i)
	{
		if (CStringUtils::StartsWith(ref, list[i] + L"/"))
		{
			remote = list[i];
			branch = ref.Mid(list[i].GetLength() + 1);
			return 0;
		}
		if (ref == list[i])
		{
			remote = list[i];
			branch.Empty();
			return 0;
		}
	}

	return -1;
}

void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
	if (!control)
		return;
	// set the sort arrow
	CHeaderCtrl * pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
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

class CRefLeafListCompareFunc
{
public:
	CRefLeafListCompareFunc(CListCtrl* pList, int col, bool desc):m_col(col),m_desc(desc),m_pList(pList){
		m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
		if (m_bSortLogical)
			m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
	}

	static int CALLBACK StaticCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		return reinterpret_cast<CRefLeafListCompareFunc*>(lParamSort)->Compare(lParam1, lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		return Compare(
			reinterpret_cast<CShadowTree*>(m_pList->GetItemData(static_cast<int>(lParam1))),
			reinterpret_cast<CShadowTree*>(m_pList->GetItemData(static_cast<int>(lParam2))));
	}

	int Compare(const CShadowTree* pLeft, const CShadowTree* pRight)
	{
		int result=CompareNoDesc(pLeft,pRight);
		if(m_desc)
			return -result;
		return result;
	}

	int CompareNoDesc(const CShadowTree* pLeft, const CShadowTree* pRight)
	{
		switch(m_col)
		{
		case CBrowseRefsDlg::eCol_Name:	return SortStrCmp(pLeft->GetRefName(), pRight->GetRefName());
		case CBrowseRefsDlg::eCol_Upstream:	return SortStrCmp(pLeft->m_csUpstream, pRight->m_csUpstream);
		case CBrowseRefsDlg::eCol_Date:	return ((pLeft->m_csDate == pRight->m_csDate) ? 0 : ((pLeft->m_csDate > pRight->m_csDate) ? 1 : -1));
		case CBrowseRefsDlg::eCol_Msg:	return SortStrCmp(pLeft->m_csSubject, pRight->m_csSubject);
		case CBrowseRefsDlg::eCol_LastAuthor: return SortStrCmp(pLeft->m_csAuthor, pRight->m_csAuthor);
		case CBrowseRefsDlg::eCol_Hash:	return pLeft->m_csRefHash.CompareNoCase(pRight->m_csRefHash);
		case CBrowseRefsDlg::eCol_Description: return SortStrCmp(pLeft->m_csDescription, pRight->m_csDescription);
		}
		return 0;
	}
	int SortStrCmp(const CString& left, const CString& right)
	{
		if (m_bSortLogical)
			return StrCmpLogicalW(left, right);
		return StrCmpI(left, right);
	}

	int m_col;
	bool m_desc;
	CListCtrl* m_pList;
	bool m_bSortLogical;
};

// CBrowseRefsDlg dialog

IMPLEMENT_DYNAMIC(CBrowseRefsDlg, CResizableStandAloneDialog)

CBrowseRefsDlg::CBrowseRefsDlg(CString cmdPath, CWnd* pParent /*=nullptr*/)
:	CResizableStandAloneDialog(CBrowseRefsDlg::IDD, pParent),
	m_cmdPath(cmdPath),
	m_currSortCol(0),
	m_currSortDesc(false),
	m_regCurrSortCol(L"Software\\TortoiseGit\\RefBrowserSortCol", 0),
	m_regCurrSortDesc(L"Software\\TortoiseGit\\RefBrowserSortDesc", FALSE),
	m_initialRef(L"HEAD"),
	m_pickRef_Kind(gPickRef_All),
	m_pListCtrlRoot(nullptr),
	m_bHasWC(true),
	m_SelectedFilters(LOGFILTER_ALL),
	m_bPickOne(false),
	m_bIncludeNestedRefs(TRUE),
	m_bPickedRefSet(false)
	, m_bWantPick(false)
{
	// get short/long datetime setting from registry
	DWORD RegUseShortDateFormat = CRegDWORD(L"Software\\TortoiseGit\\LogDateFormat", TRUE);
	if (RegUseShortDateFormat)
		m_DateFormat = DATE_SHORTDATE;
	else
		m_DateFormat = DATE_LONGDATE;
	// get relative time display setting from registry
	DWORD regRelativeTimes = CRegDWORD(L"Software\\TortoiseGit\\RelativeTimes", FALSE);
	m_bRelativeTimes = (regRelativeTimes != 0);

	m_regIncludeNestedRefs = CRegDWORD(L"Software\\TortoiseGit\\RefBrowserIncludeNestedRefs", TRUE);

	m_currSortCol = m_regCurrSortCol;
	m_currSortDesc = m_regCurrSortDesc == TRUE;
}

CBrowseRefsDlg::~CBrowseRefsDlg()
{
	m_regCurrSortCol = m_currSortCol;
	m_regCurrSortDesc = m_currSortDesc;
}

void CBrowseRefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_REF,			m_RefTreeCtrl);
	DDX_Control(pDX, IDC_LIST_REF_LEAFS,	m_ListRefLeafs);
	DDX_Control(pDX, IDC_BROWSEREFS_EDIT_FILTER, m_ctrlFilter);
	DDX_Check(pDX, IDC_INCLUDENESTEDREFS, m_bIncludeNestedRefs);
	DDX_Control(pDX, IDC_BROWSE_REFS_BRANCHFILTER, m_cBranchFilter);
}


BEGIN_MESSAGE_MAP(CBrowseRefsDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CBrowseRefsDlg::OnBnClickedOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_REF, &CBrowseRefsDlg::OnTvnSelchangedTreeRef)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnColumnclickListRefLeafs)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnNMDblclkListRefLeafs)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnItemChangedListRefLeafs)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnEndlabeleditListRefLeafs)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnBeginlabeleditListRefLeafs)
	ON_EN_CHANGE(IDC_BROWSEREFS_EDIT_FILTER, &CBrowseRefsDlg::OnEnChangeEditFilter)
	ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
	ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CURRENTBRANCH, OnBnClickedCurrentbranch)
	ON_BN_CLICKED(IDC_INCLUDENESTEDREFS, &CBrowseRefsDlg::OnBnClickedIncludeNestedRefs)
	ON_CBN_SELCHANGE(IDC_BROWSE_REFS_BRANCHFILTER, &CBrowseRefsDlg::OnCbnSelchangeBrowseRefsBranchfilter)
END_MESSAGE_MAP()


// CBrowseRefsDlg message handlers

void CBrowseRefsDlg::OnBnClickedOk()
{
	if (m_bPickOne || m_ListRefLeafs.GetSelectedCount() != 2)
	{
		OnOK();
		return;
	}

	CIconMenu popupMenu;
	popupMenu.CreatePopupMenu();

	std::vector<CShadowTree*> selectedLeafs;
	GetSelectedLeaves(selectedLeafs);

	popupMenu.AppendMenuIcon(1, GetSelectedRef(true, false), IDI_LOG);
	popupMenu.SetDefaultItem(1);
	popupMenu.AppendMenuIcon(2, GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L".."), IDI_LOG);
	popupMenu.AppendMenuIcon(3, GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"..."), IDI_LOG);

	RECT rect;
	GetDlgItem(IDOK)->GetWindowRect(&rect);
	TPMPARAMS params;
	params.cbSize = sizeof(TPMPARAMS);
	params.rcExclude = rect;
	int selection = popupMenu.TrackPopupMenuEx(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_VERTICAL, rect.left, rect.top, this, &params);
	switch (selection)
	{
	case 1:
		OnOK();
		break;
	case 2:
		{
			m_bPickedRefSet = true;
			m_pickedRef = GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"..");
			OnOK();
		}
		break;
	case 3:
		{
			m_bPickedRefSet = true;
			m_pickedRef = GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"...");
			OnOK();
		}
		break;
	default:
		break;
	}
}

BOOL CBrowseRefsDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_ctrlFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED, 14, 14);
	m_ctrlFilter.SetInfoIcon(IDI_LOGFILTER, 19, 19);
	SetFilterCueText();

	AddAnchor(IDC_TREE_REF, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_LIST_REF_LEAFS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BROWSEREFS_STATIC_FILTER, TOP_LEFT);
	AddAnchor(IDC_BROWSEREFS_EDIT_FILTER, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_BROWSE_REFS_BRANCHFILTER, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_INFOLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_INCLUDENESTEDREFS, BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	m_ListRefLeafs.SetExtendedStyle(m_ListRefLeafs.GetExtendedStyle() | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	static UINT columnNames[] = { IDS_BRANCHNAME, IDS_TRACKEDBRANCH, IDS_DATELASTCOMMIT, IDS_LASTCOMMIT, IDS_LASTAUTHOR, IDS_HASH, IDS_DESCRIPTION };
	static int columnWidths[] = { 0, 0, 0, 300, 0, 0, 80 };
	DWORD dwDefaultColumns = (1 << eCol_Name) | (1 << eCol_Upstream ) | (1 << eCol_Date) | (1 << eCol_Msg) |
		(1 << eCol_LastAuthor) | (1 << eCol_Hash) | (1 << eCol_Description);
	m_ListRefLeafs.m_bAllowHiding = false;
	m_ListRefLeafs.Init();
	m_ListRefLeafs.SetListContextMenuHandler([&](CPoint point) {OnContextMenu_ListRefLeafs(point); });
	m_ListRefLeafs.m_ColumnManager.SetNames(columnNames, _countof(columnNames));
	m_ListRefLeafs.m_ColumnManager.ReadSettings(dwDefaultColumns, 0, L"BrowseRefs", _countof(columnNames), columnWidths);
	m_bPickedRefSet = false;

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_CURRENTBRANCH, BOTTOM_RIGHT);

	m_bIncludeNestedRefs = !!m_regIncludeNestedRefs;
	UpdateData(FALSE);

	Refresh(m_initialRef);

	EnableSaveRestore(L"BrowseRefs");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_bHasWC = !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir);

	if (m_bPickOne)
		m_ListRefLeafs.ModifyStyle(0, LVS_SINGLESEL);

	m_cBranchFilter.AddString(CString(MAKEINTRESOURCE(IDS_ALL)));
	m_cBranchFilter.AddString(CString(MAKEINTRESOURCE(IDS_BROWSE_REFS_ONLYMERGED)));
	m_cBranchFilter.AddString(CString(MAKEINTRESOURCE(IDS_BROWSE_REFS_ONLYUNMERGED)));
	m_cBranchFilter.SetCurSel(0);

	m_ListRefLeafs.SetFocus();
	return FALSE;
}

CShadowTree* CShadowTree::GetNextSub(CString& nameLeft, bool bCreateIfNotExist)
{
	int posSlash=nameLeft.Find('/');
	CString nameSub;
	if(posSlash<0)
	{
		nameSub=nameLeft;
		nameLeft.Empty();//Nothing left
	}
	else
	{
		nameSub=nameLeft.Left(posSlash);
		nameLeft=nameLeft.Mid(posSlash+1);
	}
	if(nameSub.IsEmpty())
		return nullptr;

	if(!bCreateIfNotExist && m_ShadowTree.find(nameSub)==m_ShadowTree.end())
		return nullptr;

	CShadowTree& nextNode=m_ShadowTree[nameSub];
	nextNode.m_csRefName=nameSub;
	nextNode.m_pParent=this;
	return &nextNode;
}

CShadowTree* CShadowTree::FindLeaf(CString partialRefName)
{
	if(IsLeaf())
	{
		if (CStringUtils::EndsWith(partialRefName, m_csRefName))
		{
			//Match of leaf name. Try match on total name.
			CString totalRefName = GetRefName();
			if (CStringUtils::EndsWith(totalRefName, partialRefName))
				return this; //Also match. Found.
		}
	}
	else
	{
		//Not a leaf. Search all nodes.
		for (auto itShadowTree = m_ShadowTree.begin(); itShadowTree != m_ShadowTree.end(); ++itShadowTree)
		{
			CShadowTree* pSubtree = itShadowTree->second.FindLeaf(partialRefName);
			if (pSubtree)
				return pSubtree; //Found
		}
	}
	return nullptr; //Not found
}

CString CBrowseRefsDlg::GetSelectedRef(bool onlyIfLeaf, bool pickFirstSelIfMultiSel)
{
	POSITION pos=m_ListRefLeafs.GetFirstSelectedItemPosition();
	//List ctrl selection?
	if(pos && (pickFirstSelIfMultiSel || m_ListRefLeafs.GetSelectedCount() == 1))
	{
		//A leaf is selected
		return GetListEntry(m_ListRefLeafs.GetNextSelectedItem(pos))->GetRefName();
	}
	else if (pos && !pickFirstSelIfMultiSel)
	{
		// at least one leaf is selected
		CString refs;
		int index;
		while ((index = m_ListRefLeafs.GetNextSelectedItem(pos)) >= 0)
		{
			CString ref = GetListEntry(index)->GetRefName();
			if (CStringUtils::StartsWith(ref, L"refs/"))
				ref = ref.Mid(static_cast<int>(wcslen(L"refs/")));
			if (CStringUtils::StartsWith(ref, L"heads/"))
				ref = ref.Mid(static_cast<int>(wcslen(L"heads/")));
			refs += ref + L' ';
		}
		return refs.Trim();
	}
	else if(!onlyIfLeaf)
	{
		//Tree ctrl selection?
		HTREEITEM hTree=m_RefTreeCtrl.GetSelectedItem();
		if (hTree)
			return GetTreeEntry(hTree)->GetRefName();
	}
	return CString();//None
}

void CBrowseRefsDlg::Refresh(CString selectRef)
{
	remotes.clear();
	if (g_Git.GetRemoteList(remotes))
		MessageBox(CGit::GetLibGit2LastErr(L"Could not get a list of remotes."), L"TortoiseGit", MB_ICONERROR);

	if(!selectRef.IsEmpty())
	{
		if (selectRef == L"HEAD")
		{
			if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, selectRef))
				selectRef.Empty();
			else
				selectRef = L"refs/heads/" + selectRef;
		}
	}
	else
		selectRef = GetSelectedRef(false, true);

	m_RefTreeCtrl.DeleteAllItems();
	m_ListRefLeafs.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_csRefName = L"refs";
	m_TreeRoot.m_hTree = m_RefTreeCtrl.InsertItem(L"refs");
	m_RefTreeCtrl.SetItemData(m_TreeRoot.m_hTree, reinterpret_cast<DWORD_PTR>(&m_TreeRoot));

	MAP_REF_GITREVREFBROWSER refMap;
	if (CString err; GitRevRefBrowser::GetGitRevRefMap(refMap, m_cBranchFilter.GetCurSel(), err, [&](const CString& refName)
	{
		//Use ref based on m_pickRef_Kind
		if (CStringUtils::StartsWith(refName, L"refs/heads/") && !(m_pickRef_Kind & gPickRef_Head))
			return false; //Skip
		if (CStringUtils::StartsWith(refName, L"refs/tags/") && !(m_pickRef_Kind & gPickRef_Tag))
			return false; //Skip
		if (CStringUtils::StartsWith(refName, L"refs/remotes/") && !(m_pickRef_Kind & gPickRef_Remote))
			return false; //Skip
		if (m_pickRef_Kind == gPickRef_Remote && !CStringUtils::StartsWith(refName, L"refs/remotes/")) // do not show refs/stash if only remote branches are requested
			return false;
		return true;
	}))
	{
		MessageBox(L"Get refs failed:" + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
	}

	//Populate ref tree
	for (auto iterRefMap = refMap.cbegin(); iterRefMap != refMap.cend(); ++iterRefMap)
	{
		CShadowTree& treeLeaf = GetTreeNode(iterRefMap->first, nullptr, true);
		GitRevRefBrowser ref = iterRefMap->second;

		treeLeaf.m_csRefHash = ref.m_CommitHash.ToString();
		treeLeaf.m_csUpstream = ref.m_UpstreamRef;
		CGit::GetShortName(treeLeaf.m_csUpstream, treeLeaf.m_csUpstream, L"refs/remotes/");
		treeLeaf.m_csSubject = ref.GetSubject();
		treeLeaf.m_csAuthor = ref.GetAuthorName();
		treeLeaf.m_csDate = ref.GetAuthorDate();
		treeLeaf.m_csDescription = ref.m_Description;
	}

	// always expand the tree first
	m_RefTreeCtrl.Expand(m_TreeRoot.m_hTree, TVE_EXPAND);

	// try exact match first
	if (!selectRef.IsEmpty() && !SelectRef(selectRef, true))
		SelectRef(selectRef, false);
}

bool CBrowseRefsDlg::SelectRef(CString refName, bool bExactMatch)
{
	if(!bExactMatch)
	{
		CString newRefName = GetFullRefName(refName);
		if(!newRefName.IsEmpty())
			refName = newRefName;
		//else refName is not a valid ref. Try to select as good as possible.
	}
	if (!CStringUtils::StartsWith(refName, L"refs"))
		return false; // Not a ref name

	CShadowTree& treeLeafHead = GetTreeNode(refName, nullptr, false);
	if (treeLeafHead.m_hTree)
	{
		//Not a leaf. Select tree node and return
		m_RefTreeCtrl.Select(treeLeafHead.m_hTree,TVGN_CARET);
		return true;
	}

	if (!treeLeafHead.m_pParent)
		return false; //Weird... should not occur.

	//This is the current head.
	m_RefTreeCtrl.Select(treeLeafHead.m_pParent->m_hTree,TVGN_CARET);

	for(int indexPos = 0; indexPos < m_ListRefLeafs.GetItemCount(); ++indexPos)
	{
		auto pCurrShadowTree = GetListEntry(indexPos);
		if(pCurrShadowTree == &treeLeafHead)
		{
			m_ListRefLeafs.SetItemState(indexPos,LVIS_SELECTED,LVIS_SELECTED);
			m_ListRefLeafs.EnsureVisible(indexPos,FALSE);
		}
	}

	return true;
}

CShadowTree& CBrowseRefsDlg::GetTreeNode(CString refName, CShadowTree* pTreePos, bool bCreateIfNotExist)
{
	if (!pTreePos)
	{
		if (CStringUtils::StartsWith(refName, L"refs/"))
			refName = refName.Mid(static_cast<int>(wcslen(L"refs/")));
		pTreePos=&m_TreeRoot;
	}
	if(refName.IsEmpty())
		return *pTreePos;//Found leaf

	CShadowTree* pNextTree=pTreePos->GetNextSub(refName,bCreateIfNotExist);
	if (!pNextTree)
	{
		//Should not occur when all ref-names are valid and bCreateIfNotExist is true.
		ASSERT(!bCreateIfNotExist);
		return *pTreePos;
	}

	if(!refName.IsEmpty())
	{
		//When the refName is not empty, this node is not a leaf, so lets add it to the tree control.
		//Leafs are for the list control.
		if (!pNextTree->m_hTree)
		{
			//New tree. Create node in control.
			pNextTree->m_hTree = m_RefTreeCtrl.InsertItem(pNextTree->m_csRefName, pTreePos->m_hTree);
			m_RefTreeCtrl.SetItemData(pNextTree->m_hTree, reinterpret_cast<DWORD_PTR>(pNextTree));
		}
	}

	return GetTreeNode(refName, pNextTree, bCreateIfNotExist);
}


void CBrowseRefsDlg::OnTvnSelchangedTreeRef(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	FillListCtrlForTreeNode(pNMTreeView->itemNew.hItem);
}

void CBrowseRefsDlg::FillListCtrlForTreeNode(HTREEITEM treeNode)
{
	m_ListRefLeafs.DeleteAllItems();

	auto pTree = GetTreeEntry(treeNode);
	if (!pTree)
		return;

	CString filterText;
	m_ctrlFilter.GetWindowText(filterText);

	CBrowseRefsDlgFilter filter(filterText, false, m_SelectedFilters, false);

	FillListCtrlForShadowTree(pTree, L"", true, filter);
	m_ListRefLeafs.m_ColumnManager.SetVisible(eCol_Upstream, pTree->IsFrom(L"refs/heads"));
	m_ListRefLeafs.m_ColumnManager.SetVisible(eCol_Description, pTree->IsFrom(L"refs/heads"));
	m_ListRefLeafs.AdjustColumnWidths();
	UpdateInfoLabel();
}

void CBrowseRefsDlg::FillListCtrlForShadowTree(CShadowTree* pTree, CString refNamePrefix, bool isFirstLevel, const CBrowseRefsDlgFilter& filter)
{
	if(pTree->IsLeaf())
	{
		CString ref = refNamePrefix + pTree->m_csRefName;
		if (!(pTree->m_csRefName.IsEmpty() || pTree->m_csRefName == L"refs" && !pTree->m_pParent) && filter(pTree, ref))
		{
			int indexItem = m_ListRefLeafs.InsertItem(m_ListRefLeafs.GetItemCount(), L"");

			m_ListRefLeafs.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(pTree));
			m_ListRefLeafs.SetItemText(indexItem,eCol_Name, ref);
			m_ListRefLeafs.SetItemText(indexItem, eCol_Upstream, pTree->m_csUpstream);
			m_ListRefLeafs.SetItemText(indexItem, eCol_Date, pTree->m_csDate != 0 ? CLoglistUtils::FormatDateAndTime(pTree->m_csDate, m_DateFormat, true, m_bRelativeTimes) : L"");
			m_ListRefLeafs.SetItemText(indexItem,eCol_Msg, pTree->m_csSubject);
			m_ListRefLeafs.SetItemText(indexItem,eCol_LastAuthor, pTree->m_csAuthor);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Hash, pTree->m_csRefHash);
			CString descrition = pTree->m_csDescription;
			descrition.Replace(L'\n', L' ');
			m_ListRefLeafs.SetItemText(indexItem, eCol_Description, descrition);
		}
	}
	else
	{
		CString csThisName;
		if (!isFirstLevel && !m_bIncludeNestedRefs)
			return;
		else if (!isFirstLevel)
			csThisName=refNamePrefix+pTree->m_csRefName+L"/";
		else
			m_pListCtrlRoot = pTree;
		for(CShadowTree::TShadowTreeMap::iterator itSubTree=pTree->m_ShadowTree.begin(); itSubTree!=pTree->m_ShadowTree.end(); ++itSubTree)
		{
			FillListCtrlForShadowTree(&itSubTree->second, csThisName, false, filter);
		}
	}
	if (isFirstLevel)
	{
		CRefLeafListCompareFunc compareFunc(&m_ListRefLeafs, m_currSortCol, m_currSortDesc);
		m_ListRefLeafs.SortItemsEx(&CRefLeafListCompareFunc::StaticCompare, reinterpret_cast<DWORD_PTR>(&compareFunc));

		SetSortArrow(&m_ListRefLeafs,m_currSortCol,!m_currSortDesc);
	}
}

bool CBrowseRefsDlg::ConfirmDeleteRef(VectorPShadowTree& leafs)
{
	ASSERT(!leafs.empty());

	CString csMessage;
	UINT mbIcon=MB_ICONQUESTION;

	bool bIsRemoteBranch = false;
	bool bIsBranch = false;
	if		(leafs[0]->IsFrom(L"refs/remotes/"))	{bIsBranch = true; bIsRemoteBranch = true;}
	else if	(leafs[0]->IsFrom(L"refs/heads/"))	{bIsBranch = true;}

	if(bIsBranch)
	{
		if(leafs.size() == 1)
		{
			CString branchToDelete = leafs[0]->GetRefName().Mid(bIsRemoteBranch ? 13 : 11);
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, static_cast<LPCTSTR>(branchToDelete));

			//Check if branch is fully merged in HEAD
			if (!g_Git.IsFastForward(leafs[0]->GetRefName(), L"HEAD"))
			{
				csMessage += L"\r\n\r\n";
				csMessage += CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_WARNINGUNMERGED));
				mbIcon = MB_ICONWARNING;
			}

			if(bIsRemoteBranch)
			{
				csMessage += L"\r\n\r\n";
				csMessage += CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_WARNINGDELETEREMOTEBRANCHES));
				mbIcon = MB_ICONWARNING;
			}
		}
		else
		{
			csMessage.Format(IDS_PROC_DELETENREFS, leafs.size());

			csMessage += L"\r\n\r\n";
			csMessage += CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_WARNINGNOMERGECHECK));
			mbIcon = MB_ICONWARNING;

			if(bIsRemoteBranch)
			{
				csMessage += L"\r\n\r\n";
				csMessage += CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_WARNINGDELETEREMOTEBRANCHES));
				mbIcon = MB_ICONWARNING;
			}
		}

	}
	else if(leafs[0]->IsFrom(L"refs/tags/"))
	{
		if(leafs.size() == 1)
		{
			CString tagToDelete = leafs[0]->GetRefName().Mid(static_cast<int>(wcslen(L"refs/tags/")));
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, static_cast<LPCTSTR>(tagToDelete));
		}
		else
			csMessage.Format(IDS_PROC_DELETENREFS, leafs.size());
	}

	return MessageBox(csMessage, L"TortoiseGit", MB_YESNO | mbIcon) == IDYES;
}

bool CBrowseRefsDlg::DoDeleteRefs(VectorPShadowTree& leafs)
{
	bool allRemoteBranch = true;
	std::map<CString, STRING_VECTOR> remoteBranches;
	for (auto i = leafs.cbegin(); i != leafs.cend(); ++i)
	{
		CString completeRefName = (*i)->GetRefName();
		if (CStringUtils::StartsWith(completeRefName, L"refs/remotes/"))
		{
			CString branchToDelete = completeRefName.Mid(static_cast<int>(wcslen(L"refs/remotes/")));
			CString remoteName, remoteBranchToDelete;
			if (!SplitRemoteBranchName(branchToDelete, remoteName, remoteBranchToDelete))
				remoteBranches[remoteName].push_back(remoteBranchToDelete);
		}
		else
		{
			allRemoteBranch = false;
			break;
		}
	}
	if (allRemoteBranch)
	{
		// delete multiple remote branches in batch, so it is faster, fewer password prompt
		for (const auto& remotebranchlist : remoteBranches)
		{
			auto& remoteName = remotebranchlist.first;
			if (CAppUtils::IsSSHPutty())
				CAppUtils::LaunchPAgent(this->GetSafeHwnd(), nullptr, &remoteName);

			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.ShowModal(this, true);

			STRING_VECTOR list;
			list.reserve(remotebranchlist.second.size());
			std::transform(remotebranchlist.second.cbegin(), remotebranchlist.second.cend(), std::back_inserter(list), [](auto& branch) { return L"refs/heads/" + branch; });
			if (g_Git.DeleteRemoteRefs(remoteName, list))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not delete remote refs.", CGit::GIT_CMD_PUSH), L"TortoiseGit", MB_OK | MB_ICONERROR);
				sysProgressDlg.Stop();
				BringWindowToTop();
				return false;
			}
			sysProgressDlg.Stop();
		}
		BringWindowToTop();
		return true;
	}

	for (auto i = leafs.cbegin(); i != leafs.cend(); ++i)
		if(!DoDeleteRef((*i)->GetRefName()))
			return false;
	return true;
}

bool CBrowseRefsDlg::DoDeleteRef(CString completeRefName)
{
	bool bIsRemoteBranch = false;
	bool bIsBranch = false;
	if (CStringUtils::StartsWith(completeRefName, L"refs/remotes/"))
	{
		bIsBranch = true;
		bIsRemoteBranch = true;
	}
	else if (CStringUtils::StartsWith(completeRefName, L"refs/heads/"))
		bIsBranch = true;

	if (bIsRemoteBranch)
	{
		CString branchToDelete = completeRefName.Mid(static_cast<int>(wcslen(L"refs/remotes/")));
		CString remoteName, remoteBranchToDelete;
		if (SplitRemoteBranchName(branchToDelete, remoteName, remoteBranchToDelete))
			return false;

		if (CAppUtils::IsSSHPutty())
			CAppUtils::LaunchPAgent(this->GetSafeHwnd(), nullptr, &remoteName);

		CSysProgressDlg sysProgressDlg;
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		sysProgressDlg.SetShowProgressBar(false);
		sysProgressDlg.ShowModal(this, true);

		STRING_VECTOR list;
		list.push_back(L"refs/heads/" + remoteBranchToDelete);
		if (g_Git.DeleteRemoteRefs(remoteName, list))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not delete remote ref.", CGit::GIT_CMD_PUSH), L"TortoiseGit", MB_OK | MB_ICONERROR);
			sysProgressDlg.Stop();
			BringWindowToTop();
			return false;
		}
		sysProgressDlg.Stop();
		BringWindowToTop();
	}
	else if (bIsBranch)
	{
		if (g_Git.DeleteRef(completeRefName))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	else if (CStringUtils::StartsWith(completeRefName, L"refs/tags/"))
	{
		if (g_Git.DeleteRef(completeRefName))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not delete reference.", CGit::GIT_CMD_DELETETAGBRANCH), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	return true;
}

CString CBrowseRefsDlg::GetFullRefName(CString partialRefName)
{
	CShadowTree* pLeaf = m_TreeRoot.FindLeaf(partialRefName);
	if (!pLeaf)
		return CString();
	return pLeaf->GetRefName();
}


void CBrowseRefsDlg::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if (pWndFrom == &m_RefTreeCtrl)
		OnContextMenu_RefTreeCtrl(point);
}

void CBrowseRefsDlg::OnContextMenu_RefTreeCtrl(CPoint point)
{
	CPoint clientPoint=point;
	m_RefTreeCtrl.ScreenToClient(&clientPoint);

	HTREEITEM hTreeItem=m_RefTreeCtrl.HitTest(clientPoint);
	if (hTreeItem)
		m_RefTreeCtrl.Select(hTreeItem,TVGN_CARET);

	VectorPShadowTree tree;
	ShowContextMenu(point,hTreeItem,tree);
}

void CBrowseRefsDlg::GetSelectedLeaves(VectorPShadowTree& selectedLeafs)
{
	selectedLeafs.reserve(m_ListRefLeafs.GetSelectedCount());
	POSITION pos = m_ListRefLeafs.GetFirstSelectedItemPosition();
	while (pos)
	{
		selectedLeafs.push_back(GetListEntry(m_ListRefLeafs.GetNextSelectedItem(pos)));
	}
}

void CBrowseRefsDlg::OnContextMenu_ListRefLeafs(CPoint point)
{
	std::vector<CShadowTree*> selectedLeafs;
	GetSelectedLeaves(selectedLeafs);
	ShowContextMenu(point,m_RefTreeCtrl.GetSelectedItem(),selectedLeafs);
}

CString CBrowseRefsDlg::GetTwoSelectedRefs(VectorPShadowTree& selectedLeafs, const CString &lastSelected, const CString &separator)
{
	ASSERT(selectedLeafs.size() == 2);

	if (selectedLeafs.at(0)->GetRefName() == lastSelected)
		return g_Git.StripRefName(selectedLeafs.at(1)->GetRefName()) + separator + g_Git.StripRefName(lastSelected);
	else
		return g_Git.StripRefName(selectedLeafs.at(0)->GetRefName()) + separator + g_Git.StripRefName(lastSelected);
}

int findVectorPosition(const STRING_VECTOR& vector, const CString& entry)
{
	int i = 0;
	for (auto it = vector.cbegin(); it != vector.cend(); ++it, ++i)
	{
		if (*it == entry)
			return i;
	}
	return -1;
}

void CBrowseRefsDlg::ShowContextMenu(CPoint point, HTREEITEM hTreePos, VectorPShadowTree& selectedLeafs)
{
	CIconMenu popupMenu;
	popupMenu.CreatePopupMenu();

	bool bAddSeparator = false;
	CString remoteName;

	if(selectedLeafs.size()==1)
	{
		bAddSeparator = true;

		bool bShowReflogOption				= false;
		bool bShowFetchOption				= false;
		bool bShowRenameOption				= false;
		bool bShowCreateBranchOption		= false;
		bool bShowEditBranchDescriptionOption = false;

		CString fetchFromCmd;

		if(selectedLeafs[0]->IsFrom(L"refs/heads/"))
		{
			bShowReflogOption = true;
			bShowRenameOption = true;
			bShowEditBranchDescriptionOption = true;
		}
		else if(selectedLeafs[0]->IsFrom(L"refs/remotes/"))
		{
			bShowReflogOption = true;
			bShowFetchOption  = true;
			bShowCreateBranchOption = true;

			CString remoteBranch;
			if (SplitRemoteBranchName(selectedLeafs[0]->GetRefName(), remoteName, remoteBranch))
				bShowFetchOption = false;
			else
				fetchFromCmd.Format(IDS_PROC_BROWSEREFS_FETCHFROM, static_cast<LPCTSTR>(remoteName));
		}

		if (m_bWantPick)
		{
			popupMenu.AppendMenuIcon(eCmd_Select, IDS_SELECT);
			popupMenu.AppendMenu(MF_SEPARATOR);
		}
		popupMenu.AppendMenuIcon(eCmd_ViewLog, IDS_MENULOG, IDI_LOG);
		popupMenu.SetDefaultItem(0, TRUE);
		popupMenu.AppendMenuIcon(eCmd_RepoBrowser, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
		if(bShowReflogOption)
			popupMenu.AppendMenuIcon(eCmd_ShowReflog, IDS_MENUREFLOG, IDI_LOG);

		if (m_bHasWC)
		{
			popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_DiffWC, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
		}

		popupMenu.AppendMenu(MF_SEPARATOR);
		bAddSeparator = false;

		if(bShowFetchOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_Fetch, fetchFromCmd, IDI_UPDATE);
		}

		if(bAddSeparator)
			popupMenu.AppendMenu(MF_SEPARATOR);

		bAddSeparator = false;
		if (m_bHasWC)
		{
			CString str;
			if (selectedLeafs[0]->GetRefName() != L"refs/heads/" + g_Git.GetCurrentBranch())
			{
				str.Format(IDS_LOG_POPUP_MERGEREV, static_cast<LPCTSTR>(g_Git.GetCurrentBranch()));
				popupMenu.AppendMenuIcon(eCmd_Merge, str, IDI_MERGE);
			}
			popupMenu.AppendMenuIcon(eCmd_Switch, IDS_SWITCH_TO_THIS, IDI_SWITCH);
			popupMenu.AppendMenu(MF_SEPARATOR);
		}

		if(bShowCreateBranchOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, IDS_MENUBRANCH, IDI_COPY);
		}

		if (bShowEditBranchDescriptionOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_EditBranchDescription, IDS_PROC_BROWSEREFS_EDITDESCRIPTION, IDI_RENAME);
		}
		if(bShowRenameOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_Rename, IDS_PROC_BROWSEREFS_RENAME, IDI_RENAME);
		}

		if (m_bHasWC && selectedLeafs[0]->IsFrom(L"refs/heads/"))
		{
			if (bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			bAddSeparator = true;
			if (!selectedLeafs[0]->m_csUpstream.IsEmpty())
				popupMenu.AppendMenuIcon(eCmd_UpstreamDrop, IDS_PROC_BROWSEREFS_DROPTRACKEDBRANCH);
			popupMenu.AppendMenuIcon(eCmd_UpstreamSet, IDS_PROC_BROWSEREFS_SETTRACKEDBRANCH);
		}
	}
	else if(selectedLeafs.size() == 2)
	{
		bAddSeparator = true;
		popupMenu.AppendMenuIcon(eCmd_Diff, IDS_PROC_BROWSEREFS_COMPAREREFS, IDI_DIFF);
		popupMenu.AppendMenuIcon(eCmd_UnifiedDiff, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
		CString menu;
		menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"..")));
		popupMenu.AppendMenuIcon(eCmd_ViewLogRange, menu, IDI_LOG);
		menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"...")));
		popupMenu.AppendMenuIcon(eCmd_ViewLogRangeReachableFromOnlyOne, menu, IDI_LOG);
	}

	if(!selectedLeafs.empty())
	{
		if(AreAllFrom(selectedLeafs, L"refs/remotes/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName.LoadString(IDS_PROC_BROWSEREFS_DELETEREMOTEBRANCH);
			else
				menuItemName.Format(IDS_PROC_BROWSEREFS_DELETEREMOTEBRANCHES, selectedLeafs.size());

			popupMenu.AppendMenuIcon(eCmd_DeleteRemoteBranch, menuItemName, IDI_DELETE);
			bAddSeparator = true;
		}
		else if(AreAllFrom(selectedLeafs, L"refs/heads/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName.LoadString(IDS_PROC_BROWSEREFS_DELETEBRANCH);
			else
				menuItemName.Format(IDS_PROC_BROWSEREFS_DELETEBRANCHES, selectedLeafs.size());

			popupMenu.AppendMenuIcon(eCmd_DeleteBranch, menuItemName, IDI_DELETE);
			bAddSeparator = true;
		}
		else if(AreAllFrom(selectedLeafs, L"refs/tags/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName.LoadString(IDS_PROC_BROWSEREFS_DELETETAG);
			else
				menuItemName.Format(IDS_PROC_BROWSEREFS_DELETETAGS, selectedLeafs.size());

			popupMenu.AppendMenuIcon(eCmd_DeleteTag, menuItemName, IDI_DELETE);
			bAddSeparator = true;
		}
	}


	if (hTreePos && selectedLeafs.empty())
	{
		auto pTree = GetTreeEntry(hTreePos);
		if(pTree->IsFrom(L"refs/remotes"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_ManageRemotes, IDS_PROC_BROWSEREFS_MANAGEREMOTES, IDI_SETTINGS);
			bAddSeparator = true;
			if(selectedLeafs.empty())
			{
				CString remoteBranch;
				if (SplitRemoteBranchName(pTree->GetRefName(), remoteName, remoteBranch))
					remoteName.Empty();
				int pos = findVectorPosition(remotes, remoteName);
				if (pos >= 0)
				{
					CString temp;
					temp.Format(IDS_PROC_BROWSEREFS_FETCHFROM, static_cast<LPCTSTR>(remoteName));
					popupMenu.AppendMenuIcon(eCmd_Fetch, temp, IDI_UPDATE);

					temp.LoadString(IDS_DELETEREMOTETAG);
					popupMenu.AppendMenuIcon(eCmd_DeleteRemoteTag | (pos << 16), temp, IDI_DELETE);
				}
			}
			bAddSeparator = false;
		}
		if(pTree->IsFrom(L"refs/heads"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString temp;
			temp.LoadString(IDS_MENUBRANCH);
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, temp, IDI_COPY);
			bAddSeparator = false;
		}
		if(pTree->IsFrom(L"refs/tags"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_CreateTag, IDS_MENUTAG, IDI_TAG);
			popupMenu.AppendMenuIcon(eCmd_DeleteAllTags, IDS_PROC_BROWSEREFS_DELETEALLTAGS, IDI_DELETE);
			if (!remotes.empty())
			{
				popupMenu.AppendMenu(MF_SEPARATOR);
				int i = 0;
				for (auto it = remotes.cbegin(); it != remotes.cend(); ++it, ++i)
				{
					CString temp;
					temp.Format(IDS_DELETEREMOTETAGON, static_cast<LPCTSTR>(*it));
					popupMenu.AppendMenuIcon(eCmd_DeleteRemoteTag | (i << 16), temp, IDI_DELETE);
				}
			}
			bAddSeparator = false;
		}
	}
	if (bAddSeparator)
		popupMenu.AppendMenu(MF_SEPARATOR);
	popupMenu.AppendMenuIcon(eCmd_Copy, IDS_COPY_REF_NAMES, IDI_COPYCLIP);

	int selection = popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, nullptr);
	switch (static_cast<eCmd>(selection & 0xFFFF))
	{
	case eCmd_Select:
			EndDialog(IDOK);
		break;
	case eCmd_ViewLog:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(g_Git.FixBranchName(selectedLeafs[0]->GetRefName())));
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
	case eCmd_ViewLogRange:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"..")));
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
	case eCmd_ViewLogRangeReachableFromOnlyOne:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedLeafs, m_sLastSelected, L"...")));
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
	case eCmd_RepoBrowser:
		CAppUtils::RunTortoiseGitProc(L"/command:repobrowser /path:\"" + g_Git.m_CurrentDir + L"\" /rev:" + selectedLeafs[0]->GetRefName());
		break;
	case eCmd_DeleteBranch:
	case eCmd_DeleteRemoteBranch:
		{
			if(ConfirmDeleteRef(selectedLeafs))
				DoDeleteRefs(selectedLeafs);
			Refresh();
		}
		break;
	case eCmd_DeleteTag:
		{
			if(ConfirmDeleteRef(selectedLeafs))
				DoDeleteRefs(selectedLeafs);
			Refresh();
		}
		break;
	case eCmd_ShowReflog:
		{
			CRefLogDlg refLogDlg(this);
			refLogDlg.m_CurrentBranch = selectedLeafs[0]->GetRefName();
			refLogDlg.DoModal();
		}
		break;
	case eCmd_Fetch:
		{
			CAppUtils::Fetch(GetSafeHwnd(), remoteName);
			Refresh();
		}
		break;
	case eCmd_DeleteRemoteTag:
		{
			CDeleteRemoteTagDlg deleteRemoteTagDlg;
			int remoteInx = selection >> 16;
			if (remoteInx < 0 || static_cast<size_t>(remoteInx) >= remotes.size())
				return;
			deleteRemoteTagDlg.m_sRemote = remotes[remoteInx];
			deleteRemoteTagDlg.DoModal();
		}
		break;
	case eCmd_Merge:
		{
			CString ref = selectedLeafs[0]->GetRefName();
			CAppUtils::Merge(GetSafeHwnd(), &ref);
		}
		break;
	case eCmd_Switch:
		{
			CAppUtils::Switch(GetSafeHwnd(), selectedLeafs[0]->GetRefName());
		}
		break;
	case eCmd_Rename:
		{
			POSITION pos = m_ListRefLeafs.GetFirstSelectedItemPosition();
			if (pos)
				m_ListRefLeafs.EditLabel(m_ListRefLeafs.GetNextSelectedItem(pos));
		}
		break;
	case eCmd_AddRemote:
		{
			CAddRemoteDlg(this).DoModal();
			Refresh();
		}
		break;
	case eCmd_ManageRemotes:
		{
			CSinglePropSheetDlg(CString(MAKEINTRESOURCE(IDS_PROCS_TITLE_GITREMOTESETTINGS)), new CSettingGitRemote(), this).DoModal();
//			CSettingGitRemote W_Remotes(m_cmdPath);
//			W_Remotes.DoModal();
			Refresh();
		}
		break;
	case eCmd_CreateBranch:
		{
			CString* commitHash = nullptr;
			if (selectedLeafs.size() == 1)
				commitHash = &(selectedLeafs[0]->m_csRefHash);
			CAppUtils::CreateBranchTag(GetSafeHwnd(), false, commitHash);
			Refresh();
		}
		break;
	case eCmd_CreateTag:
		{
			CAppUtils::CreateBranchTag(GetSafeHwnd(), true);
			Refresh();
		}
		break;
	case eCmd_DeleteAllTags:
		{
			for (int i = 0; i < m_ListRefLeafs.GetItemCount(); ++i)
			{
				m_ListRefLeafs.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				selectedLeafs.push_back(GetListEntry(i));
			}
			if (ConfirmDeleteRef(selectedLeafs))
				DoDeleteRefs(selectedLeafs);
			Refresh();
		}
		break;
	case eCmd_Diff:
		{
			CFileDiffDlg dlg;
			dlg.SetDiff(
				nullptr,
				selectedLeafs[0]->GetRefName(),
				selectedLeafs[1]->GetRefName());
			dlg.DoModal();
		}
		break;
	case eCmd_UnifiedDiff:
		{
			CAppUtils::StartShowUnifiedDiff(nullptr, CTGitPath(), selectedLeafs[0]->m_csRefHash, CTGitPath(), selectedLeafs[1]->m_csRefHash, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		}
		break;
	case eCmd_DiffWC:
		{
			CString sCmd;
			sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:%s /revision2:%s", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(selectedLeafs[0]->GetRefName()), static_cast<LPCTSTR>(GitRev::GetWorkingCopy()));
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				sCmd += L" /alternative";

			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
	case eCmd_EditBranchDescription:
		{
			CInputDlg dlg;
			dlg.m_sHintText.LoadString(IDS_PROC_BROWSEREFS_EDITDESCRIPTION);
			dlg.m_sInputText = selectedLeafs[0]->m_csDescription;
			dlg.m_sTitle.LoadString(IDS_PROC_BROWSEREFS_EDITDESCRIPTION);
			dlg.m_bUseLogWidth = true;
			if(dlg.DoModal() == IDOK)
			{
				CAppUtils::UpdateBranchDescription(selectedLeafs[0]->GetRefsHeadsName(), dlg.m_sInputText);
				Refresh();
			}
		}
		break;
	case eCmd_UpstreamDrop:
		{
			CString key;
			key.Format(L"branch.%s.remote", static_cast<LPCTSTR>(selectedLeafs[0]->GetRefsHeadsName()));
			g_Git.UnsetConfigValue(key);
			key.Format(L"branch.%s.merge", static_cast<LPCTSTR>(selectedLeafs[0]->GetRefsHeadsName()));
			g_Git.UnsetConfigValue(key);
		}
		Refresh();
		break;
	case eCmd_UpstreamSet:
		{
			CString newRef = CBrowseRefsDlg::PickRef(false, L"", gPickRef_Remote, false);
			if (newRef.IsEmpty() || newRef.Find(L"refs/remotes/") != 0)
				return;
			CString remote, branch;
			if (SplitRemoteBranchName(newRef, remote, branch))
				return;
			CString key;
			key.Format(L"branch.%s.remote", static_cast<LPCTSTR>(selectedLeafs[0]->GetRefsHeadsName()));
			g_Git.SetConfigValue(key, remote);
			key.Format(L"branch.%s.merge", static_cast<LPCTSTR>(selectedLeafs[0]->GetRefsHeadsName()));
			g_Git.SetConfigValue(key, L"refs/heads/" + branch);
			Refresh();
		}
		break;
	case eCmd_Copy:
		{
			CString sClipdata;
			bool first = true;
			POSITION pos = m_ListRefLeafs.GetFirstSelectedItemPosition();
			while (pos)
			{
				auto index = m_ListRefLeafs.GetNextSelectedItem(pos);
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += m_ListRefLeafs.GetItemText(index, eCol_Name);
				first = false;
			}
			CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
		}
		break;
	}
}

bool CBrowseRefsDlg::AreAllFrom(VectorPShadowTree& leafs, const wchar_t* from)
{
	for (auto i = leafs.cbegin(); i != leafs.cend(); ++i)
		if(!(*i)->IsFrom(from))
			return false;
	return true;
}

BOOL CBrowseRefsDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
/*		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
			}
			break;
*/		case VK_F2:
			{
				if(pMsg->hwnd == m_ListRefLeafs.m_hWnd)
				{
					POSITION pos = m_ListRefLeafs.GetFirstSelectedItemPosition();
					if (pos)
						m_ListRefLeafs.EditLabel(m_ListRefLeafs.GetNextSelectedItem(pos));
				}
			}
			break;

		case VK_F5:
			{
				Refresh();
			}
			break;
		case L'E':
			{
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				{
					m_ctrlFilter.SetSel(0, -1, FALSE);
					m_ctrlFilter.SetFocus();
					return TRUE;
				}
			}
			break;
		case VK_ESCAPE:
			if (GetFocus() == GetDlgItem(IDC_BROWSEREFS_EDIT_FILTER) && m_ctrlFilter.GetWindowTextLength())
			{
				OnClickedCancelFilter(NULL, NULL);
				return TRUE;
			}
			break;
		}
	}


	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CBrowseRefsDlg::OnLvnColumnclickListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if(m_currSortCol == pNMLV->iSubItem)
		m_currSortDesc = !m_currSortDesc;
	else
	{
		m_currSortCol  = pNMLV->iSubItem;
		m_currSortDesc = false;
	}

	CRefLeafListCompareFunc compareFunc(&m_ListRefLeafs, m_currSortCol, m_currSortDesc);
	m_ListRefLeafs.SortItemsEx(&CRefLeafListCompareFunc::StaticCompare, reinterpret_cast<DWORD_PTR>(&compareFunc));

	SetSortArrow(&m_ListRefLeafs,m_currSortCol,!m_currSortDesc);
}

void CBrowseRefsDlg::OnDestroy()
{
	if (!m_bPickedRefSet)
		m_pickedRef = GetSelectedRef(true, false);

	CResizableStandAloneDialog::OnDestroy();
}

void CBrowseRefsDlg::OnItemChangedListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if (!(pNMListView->uChanged & LVIF_STATE))
		return;

	auto item = GetListEntry(pNMListView->iItem);
	if (item && pNMListView->uNewState == LVIS_SELECTED)
		m_sLastSelected = item->GetRefName();

	UpdateInfoLabel();
}

void CBrowseRefsDlg::OnNMDblclkListRefLeafs(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;

	if (!m_ListRefLeafs.GetFirstSelectedItemPosition())
		return;

	if (m_bWantPick)
	{
		EndDialog(IDOK);
		return;
	}

	CString sCmd;
	sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(g_Git.FixBranchName(GetSelectedRef(true, false))));
	CAppUtils::RunTortoiseGitProc(sCmd);
}

CString CBrowseRefsDlg::PickRef(bool /*returnAsHash*/, CString initialRef, int pickRef_Kind, bool pickMultipleRefsOrRange)
{
	CBrowseRefsDlg dlg(CString(), nullptr);

	if(initialRef.IsEmpty())
		initialRef = L"HEAD";
	dlg.m_bWantPick = true;
	dlg.m_initialRef = initialRef;
	dlg.m_pickRef_Kind = pickRef_Kind;
	dlg.m_bPickOne = !pickMultipleRefsOrRange;

	if(dlg.DoModal() != IDOK)
		return CString();

	return dlg.m_pickedRef;
}

bool CBrowseRefsDlg::PickRefForCombo(CHistoryCombo& refComboBox, int pickRef_Kind /* = gPickRef_All*/, int useShortName /* = gPickRef_Head*/)
{
	CString origRef;
	refComboBox.GetLBText(refComboBox.GetCurSel(), origRef);
	CString resultRef = PickRef(false,origRef,pickRef_Kind);
	if(resultRef.IsEmpty())
		return false;

	if (useShortName)
	{
		CGit::REF_TYPE refType;
		CString shortName = CGit::GetShortName(resultRef, &refType);
		switch (refType)
		{
		case CGit::REF_TYPE::LOCAL_BRANCH:
			if (useShortName & gPickRef_Head)
				resultRef = shortName;
			break;
		case CGit::REF_TYPE::ANNOTATED_TAG:
		case CGit::REF_TYPE::TAG:
			if (useShortName & gPickRef_Tag)
				resultRef = shortName;
			break;
		case CGit::REMOTE_BRANCH:
			if (useShortName & gPickRef_Remote)
				resultRef = shortName;
			break;
		}
	}

	CGit::StripRefName(resultRef);

	refComboBox.AddString(resultRef);

	return true;
}

void CBrowseRefsDlg::OnLvnEndlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = FALSE;

	if (!pDispInfo->item.pszText)
		return; //User canceled changing

	auto pTree = GetListEntry(pDispInfo->item.iItem);
	if(!pTree->IsFrom(L"refs/heads/"))
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_BROWSEREFS_RENAMEONLYBRANCHES, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	CString selectedTreeRef;
	HTREEITEM hTree = m_RefTreeCtrl.GetSelectedItem();
	if (!hTree)
	{
		auto pTree2 = GetTreeEntry(hTree);
		selectedTreeRef = pTree2->GetRefName();
	}

	CString origName = pTree->GetRefName().Mid(static_cast<int>(wcslen(L"refs/heads/")));

	CString newName;
	if (m_pListCtrlRoot)
		newName = m_pListCtrlRoot->GetRefName() + L'/';
	newName += pDispInfo->item.pszText;

	if (!CStringUtils::StartsWith(newName, L"refs/heads/"))
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_BROWSEREFS_NOCHANGEOFTYPE, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	CString newNameTrunced = newName.Mid(static_cast<int>(wcslen(L"refs/heads/")));

	if (CString errorMsg; g_Git.Run(L"git.exe branch -m \"" + origName + L"\" \"" + newNameTrunced + L'"', &errorMsg, CP_UTF8) != 0)
	{
		MessageBox(errorMsg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return;
	}
	//Do as if it failed to rename. Let Refresh() do the job.
	//*pResult = TRUE;

	Refresh(selectedTreeRef);

//	CString W_csPopup;W_csPopup.Format8(L"Ref: %s. New name: %s. With path: %s", pTree->GetRefName(), pDispInfo->item.pszText, newName);

//	AfxMessageBox(W_csPopup);
}

void CBrowseRefsDlg::OnLvnBeginlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = FALSE;

	auto pTree = GetListEntry(pDispInfo->item.iItem);
	if(!pTree->IsFrom(L"refs/heads/"))
	{
		*pResult = TRUE; //Dont allow renaming any other things then branches at the moment.
		return;
	}
}

void CBrowseRefsDlg::OnEnChangeEditFilter()
{
	SetTimer(IDT_FILTER, 1000, nullptr);
}

void CBrowseRefsDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_FILTER)
	{
		KillTimer(IDT_FILTER);
		FillListCtrlForTreeNode(m_RefTreeCtrl.GetSelectedItem());
	}

	CResizableStandAloneDialog::OnTimer(nIDEvent);
}

LRESULT CBrowseRefsDlg::OnClickedInfoIcon(WPARAM /*wParam*/, LPARAM lParam)
{
	// FIXME: x64 version would get this function called with unexpected parameters.
	if (!lParam)
		return 0;

	auto rect = reinterpret_cast<LPRECT>(lParam);
	CPoint point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | ((m_SelectedFilters & x) ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		temp.LoadString(IDS_LOG_FILTER_REFNAME);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REFNAME), LOGFILTER_REFNAME, temp);

		temp.LoadString(IDS_LOG_FILTER_SUBJECT);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_SUBJECT), LOGFILTER_SUBJECT, temp);

		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);

		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);

		popup.AppendMenu(MF_SEPARATOR);

		temp.LoadString(IDS_LOG_FILTER_TOGGLE);
		popup.AppendMenu(MF_STRING | MF_ENABLED, LOGFILTER_TOGGLE, temp);

		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		if (selection != 0)
		{
			if (selection == LOGFILTER_TOGGLE)
				m_SelectedFilters = (~m_SelectedFilters) & LOGFILTER_ALL;
			else
				m_SelectedFilters ^= selection;
			SetFilterCueText();
			SetTimer(IDT_FILTER, 1000, nullptr);
		}
	}
	return 0L;
}

LRESULT CBrowseRefsDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	KillTimer(LOGFILTER_TIMER);
	m_ctrlFilter.SetWindowText(L"");
	FillListCtrlForTreeNode(m_RefTreeCtrl.GetSelectedItem());
	return 0L;
}

void CBrowseRefsDlg::SetFilterCueText()
{
	CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));
	temp += L' ';

	if (m_SelectedFilters & LOGFILTER_REFNAME)
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REFNAME));

	if (m_SelectedFilters & LOGFILTER_SUBJECT)
	{
		if (!CStringUtils::EndsWith(temp, L' '))
			temp += L", ";
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_SUBJECT));
	}

	if (m_SelectedFilters & LOGFILTER_AUTHORS)
	{
		if (!CStringUtils::EndsWith(temp, L' '))
			temp += L", ";
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
	}

	if (m_SelectedFilters & LOGFILTER_REVS)
	{
		if (!CStringUtils::EndsWith(temp, L' '))
			temp += L", ";
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
	}

	// to make the cue banner text appear more to the right of the edit control
	temp = L"   " + temp;
	m_ctrlFilter.SetCueBanner(temp.TrimRight());
}

void CBrowseRefsDlg::OnBnClickedCurrentbranch()
{
	m_pickedRef = g_Git.GetCurrentBranch(true);
	m_bPickedRefSet = true;
	OnOK();
}

void CBrowseRefsDlg::UpdateInfoLabel()
{
	CString temp;
	temp.FormatMessage(IDS_REFBROWSE_INFO, m_ListRefLeafs.GetItemCount(), m_ListRefLeafs.GetSelectedCount());
	SetDlgItemText(IDC_INFOLABEL, temp);
}

void CBrowseRefsDlg::OnBnClickedIncludeNestedRefs()
{
	UpdateData(TRUE);
	m_regIncludeNestedRefs = m_bIncludeNestedRefs;
	Refresh();
}

CShadowTree* CBrowseRefsDlg::GetListEntry(int index)
{
	auto entry = reinterpret_cast<CShadowTree*>(m_ListRefLeafs.GetItemData(index));
	ASSERT(entry);
	return entry;
}

CShadowTree* CBrowseRefsDlg::GetTreeEntry(HTREEITEM treeItem)
{
	auto entry = reinterpret_cast<CShadowTree*>(m_RefTreeCtrl.GetItemData(treeItem));
	ASSERT(entry);
	return entry;
}


void CBrowseRefsDlg::OnCbnSelchangeBrowseRefsBranchfilter()
{
	Refresh();
}
