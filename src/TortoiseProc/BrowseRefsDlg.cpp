// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit

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
#include "Settings\SettingGitRemote.h"
#include "SinglePropSheetDlg.h"
#include "MessageBox.h"
#include "RefLogDlg.h"
#include "IconMenu.h"
#include "FileDiffDlg.h"
#include "DeleteRemoteTagDlg.h"
#include "git2.h"
#include "UnicodeUtils.h"
#include "InputDlg.h"

void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
	if (control == NULL)
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
		return ((CRefLeafListCompareFunc*)lParamSort)->Compare(lParam1,lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		return Compare(
			(CShadowTree*)m_pList->GetItemData((int)lParam1),
			(CShadowTree*)m_pList->GetItemData((int)lParam2));
	}

	int Compare(CShadowTree* pLeft, CShadowTree* pRight)
	{
		int result=CompareNoDesc(pLeft,pRight);
		if(m_desc)
			return -result;
		return result;
	}

	int CompareNoDesc(CShadowTree* pLeft, CShadowTree* pRight)
	{
		switch(m_col)
		{
		case CBrowseRefsDlg::eCol_Name:	return SortStrCmp(pLeft->GetRefName(), pRight->GetRefName());
		case CBrowseRefsDlg::eCol_Date:	return pLeft->m_csDate_Iso8601.CompareNoCase(pRight->m_csDate_Iso8601);
		case CBrowseRefsDlg::eCol_Msg:	return SortStrCmp(pLeft->m_csSubject, pRight->m_csSubject);
		case CBrowseRefsDlg::eCol_LastAuthor: return SortStrCmp(pLeft->m_csAuthor, pRight->m_csAuthor);
		case CBrowseRefsDlg::eCol_Hash:	return pLeft->m_csRefHash.CompareNoCase(pRight->m_csRefHash);
		case CBrowseRefsDlg::eCol_Description: return SortStrCmp(pLeft->m_csDescription, pRight->m_csDescription);
		}
		return 0;
	}
	int SortStrCmp(CString left, CString right)
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

CBrowseRefsDlg::CBrowseRefsDlg(CString cmdPath, CWnd* pParent /*=NULL*/)
:	CResizableStandAloneDialog(CBrowseRefsDlg::IDD, pParent),
	m_cmdPath(cmdPath),
	m_currSortCol(0),
	m_currSortDesc(false),
	m_initialRef(L"HEAD"),
	m_pickRef_Kind(gPickRef_All),
	m_pListCtrlRoot(NULL),
	m_bHasWC(true),
	m_SelectedFilters(LOGFILTER_ALL),
	m_bPickOne(false)
{

}

CBrowseRefsDlg::~CBrowseRefsDlg()
{
}

void CBrowseRefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_REF,			m_RefTreeCtrl);
	DDX_Control(pDX, IDC_LIST_REF_LEAFS,	m_ListRefLeafs);
	DDX_Control(pDX, IDC_BROWSEREFS_EDIT_FILTER, m_ctrlFilter);
}


BEGIN_MESSAGE_MAP(CBrowseRefsDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CBrowseRefsDlg::OnBnClickedOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_REF, &CBrowseRefsDlg::OnTvnSelchangedTreeRef)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnColumnclickListRefLeafs)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnNMDblclkListRefLeafs)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnEndlabeleditListRefLeafs)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnBeginlabeleditListRefLeafs)
	ON_EN_CHANGE(IDC_BROWSEREFS_EDIT_FILTER, &CBrowseRefsDlg::OnEnChangeEditFilter)
	ON_MESSAGE(WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CBrowseRefsDlg message handlers

void CBrowseRefsDlg::OnBnClickedOk()
{
	OnOK();
}

BOOL CBrowseRefsDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_ctrlFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_ctrlFilter.SetInfoIcon(IDI_FILTEREDIT);
	SetFilterCueText();

	AddAnchor(IDC_TREE_REF, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_LIST_REF_LEAFS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BROWSEREFS_STATIC_FILTER, TOP_LEFT);
	AddAnchor(IDC_BROWSEREFS_EDIT_FILTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	m_ListRefLeafs.SetExtendedStyle(m_ListRefLeafs.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
	CString temp;
	temp.LoadString(IDS_BRANCHNAME);
	m_ListRefLeafs.InsertColumn(eCol_Name, temp, 0, 150);
	temp.LoadString(IDS_DATELASTCOMMIT);
	m_ListRefLeafs.InsertColumn(eCol_Date, temp, 0, 100);
	temp.LoadString(IDS_LASTCOMMIT);
	m_ListRefLeafs.InsertColumn(eCol_Msg, temp, 0, 300);
	temp.LoadString(IDS_LASTAUTHOR);
	m_ListRefLeafs.InsertColumn(eCol_LastAuthor, temp, 0, 100);
	temp.LoadString(IDS_HASH);
	m_ListRefLeafs.InsertColumn(eCol_Hash, temp, 0, 80);
	temp.LoadString(IDS_DESCRIPTION);
	m_ListRefLeafs.InsertColumn(eCol_Description, temp, 0, 80);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	Refresh(m_initialRef);

	EnableSaveRestore(L"BrowseRefs");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_bHasWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);

	if (m_bPickOne)
		m_ListRefLeafs.ModifyStyle(0, LVS_SINGLESEL);

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
		return NULL;

	if(!bCreateIfNotExist && m_ShadowTree.find(nameSub)==m_ShadowTree.end())
		return NULL;

	CShadowTree& nextNode=m_ShadowTree[nameSub];
	nextNode.m_csRefName=nameSub;
	nextNode.m_pParent=this;
	return &nextNode;
}

CShadowTree* CShadowTree::FindLeaf(CString partialRefName)
{
	if(IsLeaf())
	{
		if(m_csRefName.GetLength() > partialRefName.GetLength())
			return NULL;
		if(partialRefName.Right(m_csRefName.GetLength()) == m_csRefName)
		{
			//Match of leaf name. Try match on total name.
			CString totalRefName = GetRefName();
			if(totalRefName.Right(partialRefName.GetLength()) == partialRefName)
				return this; //Also match. Found.
		}
	}
	else
	{
		//Not a leaf. Search all nodes.
		for(TShadowTreeMap::iterator itShadowTree = m_ShadowTree.begin(); itShadowTree != m_ShadowTree.end(); ++itShadowTree)
		{
			CShadowTree* pSubtree = itShadowTree->second.FindLeaf(partialRefName);
			if(pSubtree != NULL)
				return pSubtree; //Found
		}
	}
	return NULL;//Not found
}


typedef std::map<CString,CString> MAP_STRING_STRING;

CString CBrowseRefsDlg::GetSelectedRef(bool onlyIfLeaf, bool pickFirstSelIfMultiSel)
{
	POSITION pos=m_ListRefLeafs.GetFirstSelectedItemPosition();
	//List ctrl selection?
	if(pos && (pickFirstSelIfMultiSel || m_ListRefLeafs.GetSelectedCount() == 1))
	{
		//A leaf is selected
		CShadowTree* pTree=(CShadowTree*)m_ListRefLeafs.GetItemData(
				m_ListRefLeafs.GetNextSelectedItem(pos));
		return pTree->GetRefName();
	}
	else if(!onlyIfLeaf)
	{
		//Tree ctrl selection?
		HTREEITEM hTree=m_RefTreeCtrl.GetSelectedItem();
		if(hTree!=NULL)
		{
			CShadowTree* pTree=(CShadowTree*)m_RefTreeCtrl.GetItemData(hTree);
			return pTree->GetRefName();
		}
	}
	return CString();//None
}

static int GetBranchDescriptionsCallback(const char *var_name, const char *value, void *data)
{
	MAP_STRING_STRING *descriptions = (MAP_STRING_STRING *) data;
	CString key = CUnicodeUtils::GetUnicode(var_name, CP_UTF8);
	CString val = CUnicodeUtils::GetUnicode(value, CP_UTF8);
	descriptions->insert(make_pair(key.Mid(7, key.GetLength() - 7 - 12), val)); // 7: branch., 12: .description
	return 0;
}

MAP_STRING_STRING GetBranchDescriptions()
{
	MAP_STRING_STRING descriptions;
	git_config * config;
	git_config_new(&config);
	CStringA projectConfigA = CUnicodeUtils::GetMulti(g_Git.m_CurrentDir + _T("\\.git\\config"), CP_UTF8);
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 3);
	projectConfigA.ReleaseBuffer();
	git_config_foreach_match(config, "branch\\..*\\.description", GetBranchDescriptionsCallback, &descriptions);
	git_config_free(config);
	return descriptions;
}

void CBrowseRefsDlg::Refresh(CString selectRef)
{
//	m_RefMap.clear();
//	g_Git.GetMapHashToFriendName(m_RefMap);

	if(!selectRef.IsEmpty())
	{
		if(selectRef == "HEAD")
		{
			selectRef = g_Git.GetSymbolicRef(selectRef, false);
		}
	}
	else
	{
		selectRef = GetSelectedRef(false, true);
	}

	m_RefTreeCtrl.DeleteAllItems();
	m_ListRefLeafs.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_csRefName = "refs";
	m_TreeRoot.m_hTree=m_RefTreeCtrl.InsertItem(L"Refs",NULL,NULL);
	m_RefTreeCtrl.SetItemData(m_TreeRoot.m_hTree,(DWORD_PTR)&m_TreeRoot);

	CString allRefs;
	g_Git.Run(L"git for-each-ref --format="
			  L"%(refname)%04"
			  L"%(objectname)%04"
			  L"%(authordate:relative)%04"
			  L"%(subject)%04"
			  L"%(authorname)%04"
			  L"%(authordate:iso8601)%03",
			  &allRefs, NULL, CP_UTF8);

	int linePos=0;
	CString singleRef;

	MAP_STRING_STRING refMap;

	//First sort on ref name
	while(!(singleRef=allRefs.Tokenize(L"\03",linePos)).IsEmpty())
	{
		singleRef.TrimLeft(L"\r\n");
		int valuePos=0;
		CString refName=singleRef.Tokenize(L"\04",valuePos);
		if(refName.IsEmpty())
			continue;
		CString refRest=singleRef.Mid(valuePos);


		//Use ref based on m_pickRef_Kind
		if(wcsncmp(refName,L"refs/heads",10)==0 && !(m_pickRef_Kind & gPickRef_Head) )
			continue; //Skip
		if(wcsncmp(refName,L"refs/tags",9)==0 && !(m_pickRef_Kind & gPickRef_Tag) )
			continue; //Skip
		if(wcsncmp(refName,L"refs/remotes",12)==0 && !(m_pickRef_Kind & gPickRef_Remote) )
			continue; //Skip

		refMap[refName] = refRest; //Use
	}

	MAP_STRING_STRING descriptions = GetBranchDescriptions();

	//Populate ref tree
	for(MAP_STRING_STRING::iterator iterRefMap=refMap.begin();iterRefMap!=refMap.end();++iterRefMap)
	{
		CShadowTree& treeLeaf=GetTreeNode(iterRefMap->first,NULL,true);
		CString values=iterRefMap->second;
		values.Replace(L"\04" L"\04",L"\04 \04");//Workaround Tokenize problem (treating 2 tokens as one)

		int valuePos=0;
		treeLeaf.m_csRefHash=		values.Tokenize(L"\04",valuePos); if(valuePos < 0) continue;
		treeLeaf.m_csDate=			values.Tokenize(L"\04",valuePos); if(valuePos < 0) continue;
		treeLeaf.m_csSubject=		values.Tokenize(L"\04",valuePos); if(valuePos < 0) continue;
		treeLeaf.m_csAuthor=		values.Tokenize(L"\04",valuePos); if(valuePos < 0) continue;
		treeLeaf.m_csDate_Iso8601=	values.Tokenize(L"\04",valuePos);

		if (wcsncmp(iterRefMap->first, L"refs/heads", 10) == 0)
			treeLeaf.m_csDescription = descriptions[treeLeaf.m_csRefName];
	}


	if(selectRef.IsEmpty() || !SelectRef(selectRef, false))
		//Probably not on a branch. Select root node.
		m_RefTreeCtrl.Expand(m_TreeRoot.m_hTree,TVE_EXPAND);

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
	if(_wcsnicmp(refName, L"refs/", 5) != 0)
		return false; // Not a ref name

	CShadowTree& treeLeafHead=GetTreeNode(refName,NULL,false);
	if(treeLeafHead.m_hTree != NULL)
	{
		//Not a leaf. Select tree node and return
		m_RefTreeCtrl.Select(treeLeafHead.m_hTree,TVGN_CARET);
		return true;
	}

	if(treeLeafHead.m_pParent==NULL)
		return false; //Weird... should not occur.

	//This is the current head.
	m_RefTreeCtrl.Select(treeLeafHead.m_pParent->m_hTree,TVGN_CARET);

	for(int indexPos = 0; indexPos < m_ListRefLeafs.GetItemCount(); ++indexPos)
	{
		CShadowTree* pCurrShadowTree = (CShadowTree*)m_ListRefLeafs.GetItemData(indexPos);
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
	if(pTreePos==NULL)
	{
		if(_wcsnicmp(refName, L"refs/", 5) == 0)
			refName=refName.Mid(5);
		pTreePos=&m_TreeRoot;
	}
	if(refName.IsEmpty())
		return *pTreePos;//Found leaf

	CShadowTree* pNextTree=pTreePos->GetNextSub(refName,bCreateIfNotExist);
	if(pNextTree==NULL)
	{
		//Should not occur when all ref-names are valid and bCreateIfNotExist is true.
		ASSERT(!bCreateIfNotExist);
		return *pTreePos;
	}

	if(!refName.IsEmpty())
	{
		//When the refName is not empty, this node is not a leaf, so lets add it to the tree control.
		//Leafs are for the list control.
		if(pNextTree->m_hTree==NULL)
		{
			//New tree. Create node in control.
			pNextTree->m_hTree=m_RefTreeCtrl.InsertItem(pNextTree->m_csRefName,pTreePos->m_hTree,NULL);
			m_RefTreeCtrl.SetItemData(pNextTree->m_hTree,(DWORD_PTR)pNextTree);
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

	CShadowTree* pTree=(CShadowTree*)(m_RefTreeCtrl.GetItemData(treeNode));
	if(pTree==NULL)
	{
		ASSERT(FALSE);
		return;
	}
	FillListCtrlForShadowTree(pTree,L"",true);
}

void CBrowseRefsDlg::FillListCtrlForShadowTree(CShadowTree* pTree, CString refNamePrefix, bool isFirstLevel)
{
	if(pTree->IsLeaf())
	{
		CString filter;
		m_ctrlFilter.GetWindowText(filter);
		filter.MakeLower();
		CString ref = refNamePrefix + pTree->m_csRefName;
		if (!(pTree->m_csRefName.IsEmpty() || pTree->m_csRefName == "refs" && pTree->m_pParent == NULL) && IsMatchFilter(pTree, ref, filter))
		{
			int indexItem = m_ListRefLeafs.InsertItem(m_ListRefLeafs.GetItemCount(), L"");

			m_ListRefLeafs.SetItemData(indexItem,(DWORD_PTR)pTree);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Name, ref);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Date, pTree->m_csDate);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Msg, pTree->m_csSubject);
			m_ListRefLeafs.SetItemText(indexItem,eCol_LastAuthor, pTree->m_csAuthor);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Hash, pTree->m_csRefHash);
			int pos = 0;
			m_ListRefLeafs.SetItemText(indexItem,eCol_Description, pTree->m_csDescription.Tokenize(_T("\n"), pos));
		}
	}
	else
	{

		CString csThisName;
		if(!isFirstLevel)
			csThisName=refNamePrefix+pTree->m_csRefName+L"/";
		else
			m_pListCtrlRoot = pTree;
		for(CShadowTree::TShadowTreeMap::iterator itSubTree=pTree->m_ShadowTree.begin(); itSubTree!=pTree->m_ShadowTree.end(); ++itSubTree)
		{
			FillListCtrlForShadowTree(&itSubTree->second,csThisName,false);
		}
	}
	if (isFirstLevel)
	{
		CRefLeafListCompareFunc compareFunc(&m_ListRefLeafs, m_currSortCol, m_currSortDesc);
		m_ListRefLeafs.SortItemsEx(&CRefLeafListCompareFunc::StaticCompare, (DWORD_PTR)&compareFunc);

		SetSortArrow(&m_ListRefLeafs,m_currSortCol,!m_currSortDesc);
	}
}

bool CBrowseRefsDlg::IsMatchFilter(const CShadowTree* pTree, const CString &ref, const CString &filter)
{
	if (m_SelectedFilters & LOGFILTER_REFNAME)
	{
		CString msg = ref;
		msg = msg.MakeLower();

		if (msg.Find(filter) >= 0)
			return true;
	}

	if (m_SelectedFilters & LOGFILTER_SUBJECT)
	{
		CString msg = pTree->m_csSubject;
		msg = msg.MakeLower();

		if (msg.Find(filter) >= 0)
			return true;
	}

	if (m_SelectedFilters & LOGFILTER_AUTHORS)
	{
		CString msg = pTree->m_csAuthor;
		msg = msg.MakeLower();

		if (msg.Find(filter) >= 0)
			return true;
	}

	if (m_SelectedFilters & LOGFILTER_REVS)
	{
		CString msg = pTree->m_csRefHash;
		msg = msg.MakeLower();

		if (msg.Find(filter) >= 0)
			return true;
	}
	return false;
}

bool CBrowseRefsDlg::ConfirmDeleteRef(VectorPShadowTree& leafs)
{
	ASSERT(!leafs.empty());

	CString csMessage;
	UINT mbIcon=MB_ICONQUESTION;

	bool bIsRemoteBranch = false;
	bool bIsBranch = false;
	if		(leafs[0]->IsFrom(L"refs/remotes"))	{bIsBranch = true; bIsRemoteBranch = true;}
	else if	(leafs[0]->IsFrom(L"refs/heads"))	{bIsBranch = true;}

	if(bIsBranch)
	{
		if(leafs.size() == 1)
		{
			CString branchToDelete = leafs[0]->GetRefName().Mid(bIsRemoteBranch ? 13 : 11);
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, branchToDelete);

			//Check if branch is fully merged in HEAD
			if (!g_Git.IsFastForward(leafs[0]->GetRefName(), _T("HEAD")))
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
	else if(leafs[0]->IsFrom(L"refs/tags"))
	{
		if(leafs.size() == 1)
		{
			CString tagToDelete = leafs[0]->GetRefName().Mid(10);
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, tagToDelete);
		}
		else
		{
			csMessage.Format(IDS_PROC_DELETENREFS, leafs.size());
		}
	}

	return CMessageBox::Show(m_hWnd, csMessage, _T("TortoiseGit"), MB_YESNO | mbIcon) == IDYES;

}

bool CBrowseRefsDlg::DoDeleteRefs(VectorPShadowTree& leafs, bool bForce)
{
	for(VectorPShadowTree::iterator i = leafs.begin(); i != leafs.end(); ++i)
		if(!DoDeleteRef((*i)->GetRefName(), bForce))
			return false;
	return true;
}

bool CBrowseRefsDlg::DoDeleteRef(CString completeRefName, bool bForce)
{
	bool bIsRemoteBranch = false;
	bool bIsBranch = false;
	if		(wcsncmp(completeRefName, L"refs/remotes",12)==0)	{bIsBranch = true; bIsRemoteBranch = true;}
	else if	(wcsncmp(completeRefName, L"refs/heads",10)==0)		{bIsBranch = true;}

	if(bIsBranch)
	{
		CString branchToDelete = completeRefName.Mid(bIsRemoteBranch ? 13 : 11);
		CString cmd;
		if(bIsRemoteBranch)
		{
			int slash = branchToDelete.Find(L'/');
			if(slash < 0)
				return false;
			CString remoteName = branchToDelete.Left(slash);
			CString remoteBranchToDelete = branchToDelete.Mid(slash + 1);

			if(CAppUtils::IsSSHPutty())
			{
				CAppUtils::LaunchPAgent(NULL, &remoteName);
			}

			cmd.Format(L"git.exe push \"%s\" :refs/heads/%s", remoteName, remoteBranchToDelete);
		}
		else
			cmd.Format(L"git.exe branch -%c -- %s",bForce?L'D':L'd',branchToDelete);
		CString errorMsg;
		if(g_Git.Run(cmd,&errorMsg,CP_UTF8)!=0)
		{
			CMessageBox::Show(m_hWnd, errorMsg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return false;
		}
	}
	else if(wcsncmp(completeRefName,L"refs/tags",9)==0)
	{
		CString tagToDelete = completeRefName.Mid(10);
		CString cmd;
		cmd.Format(L"git.exe tag -d -- %s",tagToDelete);
		CString errorMsg;
		if(g_Git.Run(cmd,&errorMsg,CP_UTF8)!=0)
		{
			CMessageBox::Show(m_hWnd, errorMsg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return false;
		}
	}
	return true;
}

CString CBrowseRefsDlg::GetFullRefName(CString partialRefName)
{
	CShadowTree* pLeaf = m_TreeRoot.FindLeaf(partialRefName);
	if(pLeaf == NULL)
		return CString();
	return pLeaf->GetRefName();
}


void CBrowseRefsDlg::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if(pWndFrom==&m_RefTreeCtrl)       OnContextMenu_RefTreeCtrl(point);
	else if(pWndFrom==&m_ListRefLeafs) OnContextMenu_ListRefLeafs(point);
}

void CBrowseRefsDlg::OnContextMenu_RefTreeCtrl(CPoint point)
{
	CPoint clientPoint=point;
	m_RefTreeCtrl.ScreenToClient(&clientPoint);

	HTREEITEM hTreeItem=m_RefTreeCtrl.HitTest(clientPoint);
	if(hTreeItem!=NULL)
		m_RefTreeCtrl.Select(hTreeItem,TVGN_CARET);

	VectorPShadowTree tree;
	ShowContextMenu(point,hTreeItem,tree);
}


void CBrowseRefsDlg::OnContextMenu_ListRefLeafs(CPoint point)
{
	std::vector<CShadowTree*> selectedLeafs;
	selectedLeafs.reserve(m_ListRefLeafs.GetSelectedCount());
	POSITION pos=m_ListRefLeafs.GetFirstSelectedItemPosition();
	while(pos)
	{
		selectedLeafs.push_back(
			(CShadowTree*)m_ListRefLeafs.GetItemData(
				m_ListRefLeafs.GetNextSelectedItem(pos)));
	}

	ShowContextMenu(point,m_RefTreeCtrl.GetSelectedItem(),selectedLeafs);
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
		bool bShowSwitchOption				= false;
		bool bShowRenameOption				= false;
		bool bShowCreateBranchOption		= false;
		bool bShowEditBranchDescriptionOption = false;

		CString fetchFromCmd;

		if(selectedLeafs[0]->IsFrom(L"refs/heads"))
		{
			bShowReflogOption = true;
			bShowSwitchOption = true;
			bShowRenameOption = true;
			bShowEditBranchDescriptionOption = true;
		}
		else if(selectedLeafs[0]->IsFrom(L"refs/remotes"))
		{
			bShowReflogOption = true;
			bShowFetchOption  = true;
			bShowCreateBranchOption = true;

			int dummy = 0;//Needed for tokenize
			remoteName = selectedLeafs[0]->GetRefName();
			remoteName = remoteName.Mid(13);
			remoteName = remoteName.Tokenize(L"/", dummy);
			fetchFromCmd.Format(IDS_PROC_BROWSEREFS_FETCHFROM, remoteName);
		}
		else if(selectedLeafs[0]->IsFrom(L"refs/tags"))
		{
		}

		CString temp;
		temp.LoadString(IDS_MENULOG);
		popupMenu.AppendMenuIcon(eCmd_ViewLog, temp, IDI_LOG);
		popupMenu.AppendMenuIcon(eCmd_RepoBrowser, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
		if(bShowReflogOption)
		{
			temp.LoadString(IDS_MENUREFLOG);
			popupMenu.AppendMenuIcon(eCmd_ShowReflog, temp, IDI_LOG);
		}

		popupMenu.AppendMenu(MF_SEPARATOR);
		bAddSeparator = false;

		if(bShowFetchOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_Fetch, fetchFromCmd, IDI_PULL);
		}

		if(bAddSeparator)
			popupMenu.AppendMenu(MF_SEPARATOR);

		bAddSeparator = false;
		if (m_bHasWC)
		{
			popupMenu.AppendMenuIcon(eCmd_Switch, CString(MAKEINTRESOURCE(IDS_SWITCH_TO_THIS)), IDI_SWITCH);
			popupMenu.AppendMenu(MF_SEPARATOR);
		}

		if(bShowCreateBranchOption)
		{
			bAddSeparator = true;
			temp.LoadString(IDS_MENUBRANCH);
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, temp, IDI_COPY);
		}

		if (bShowEditBranchDescriptionOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_EditBranchDescription, CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_EDITDESCRIPTION)), IDI_RENAME);
		}
		if(bShowRenameOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_Rename, CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_RENAME)), IDI_RENAME);
		}
	}
	else if(selectedLeafs.size() == 2)
	{
		bAddSeparator = true;
		popupMenu.AppendMenuIcon(eCmd_Diff, CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_COMPAREREFS)), IDI_DIFF);
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


	if(hTreePos!=NULL && selectedLeafs.empty())
	{
		CShadowTree* pTree=(CShadowTree*)m_RefTreeCtrl.GetItemData(hTreePos);
		if(pTree->IsFrom(L"refs/remotes"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_ManageRemotes, CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_MANAGEREMOTES)), IDI_SETTINGS);
			bAddSeparator = true;
			if(selectedLeafs.empty())
			{
				int dummy = 0;//Needed for tokenize
				remoteName = pTree->GetRefName();
				remoteName = remoteName.Mid(13);
				remoteName = remoteName.Tokenize(L"/", dummy);
				if(!remoteName.IsEmpty())
				{
					CString temp;
					temp.Format(IDS_PROC_BROWSEREFS_FETCHFROM, remoteName);
					popupMenu.AppendMenuIcon(eCmd_Fetch, temp, IDI_PULL);

					temp.LoadString(IDS_DELETEREMOTETAG);
					popupMenu.AppendMenuIcon(eCmd_DeleteRemoteTag, temp, IDI_DELETE);
				}
			}
		}
		if(pTree->IsFrom(L"refs/heads"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString temp;
			temp.LoadString(IDS_MENUBRANCH);
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, temp, IDI_COPY);
		}
		if(pTree->IsFrom(L"refs/tags"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString temp;
			temp.LoadString(IDS_MENUTAG);
			popupMenu.AppendMenuIcon(eCmd_CreateTag, temp, IDI_TAG);
		}
	}


	eCmd cmd=(eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_ViewLog:
		{
			CLogDlg dlg;
			dlg.SetStartRef(selectedLeafs[0]->GetRefName());
			dlg.DoModal();
		}
		break;
	case eCmd_RepoBrowser:
		CAppUtils::RunTortoiseProc(_T("/command:repobrowser /path:\"") + g_Git.m_CurrentDir + _T("\" /rev:") + selectedLeafs[0]->GetRefName());
		break;
	case eCmd_DeleteBranch:
	case eCmd_DeleteRemoteBranch:
		{
			if(ConfirmDeleteRef(selectedLeafs))
				DoDeleteRefs(selectedLeafs, true);
			Refresh();
		}
		break;
	case eCmd_DeleteTag:
		{
			if(ConfirmDeleteRef(selectedLeafs))
				DoDeleteRefs(selectedLeafs, true);
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
			CAppUtils::Fetch(remoteName);
			Refresh();
		}
		break;
	case eCmd_DeleteRemoteTag:
		{
			CDeleteRemoteTagDlg deleteRemoteTagDlg;
			deleteRemoteTagDlg.m_sRemote = remoteName;
			deleteRemoteTagDlg.DoModal();
		}
		break;
	case eCmd_Switch:
		{
			CAppUtils::Switch(selectedLeafs[0]->GetRefName());
		}
		break;
	case eCmd_Rename:
		{
			POSITION pos = m_ListRefLeafs.GetFirstSelectedItemPosition();
			if(pos != NULL)
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
			CSinglePropSheetDlg(CString(MAKEINTRESOURCE(IDS_PROCS_TITLE_GITREMOTESETTINGS)), new CSettingGitRemote(g_Git.m_CurrentDir), this).DoModal();
//			CSettingGitRemote W_Remotes(m_cmdPath);
//			W_Remotes.DoModal();
			Refresh();
		}
		break;
	case eCmd_CreateBranch:
		{
			CString *commitHash = NULL;
			if (selectedLeafs.size() == 1)
				commitHash = &(selectedLeafs[0]->m_csRefHash);
			CAppUtils::CreateBranchTag(false, commitHash);
			Refresh();
		}
		break;
	case eCmd_CreateTag:
		{
			CAppUtils::CreateBranchTag(true);
			Refresh();
		}
		break;
	case eCmd_Diff:
		{
			CFileDiffDlg dlg;
			dlg.SetDiff(
				NULL,
				selectedLeafs[0]->m_csRefHash,
				selectedLeafs[1]->m_csRefHash);
			dlg.DoModal();
		}
		break;
	case eCmd_EditBranchDescription:
		{
			CInputDlg dlg;
			dlg.m_sHintText = CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_EDITDESCRIPTION));
			dlg.m_sInputText = selectedLeafs[0]->m_csDescription;
			dlg.m_sTitle = CString(MAKEINTRESOURCE(IDS_PROC_BROWSEREFS_EDITDESCRIPTION));
			dlg.m_bUseLogWidth = true;
			if(dlg.DoModal() == IDOK)
			{
				CString key;
				key.Format(_T("branch.%s.description"), selectedLeafs[0]->m_csRefName);
				dlg.m_sInputText.Replace(_T("\r"), _T(""));
				dlg.m_sInputText.Trim();
				if (dlg.m_sInputText.IsEmpty())
					g_Git.UnsetConfigValue(key);
				else
					g_Git.SetConfigValue(key, dlg.m_sInputText);
				Refresh();
			}
		}
		break;
	}
}

bool CBrowseRefsDlg::AreAllFrom(VectorPShadowTree& leafs, const wchar_t* from)
{
	for(VectorPShadowTree::iterator i = leafs.begin(); i != leafs.end(); ++i)
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
					if(pos != NULL)
						m_ListRefLeafs.EditLabel(m_ListRefLeafs.GetNextSelectedItem(pos));
				}
			}
			break;

		case VK_F5:
			{
				Refresh();
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
	m_ListRefLeafs.SortItemsEx(&CRefLeafListCompareFunc::StaticCompare, (DWORD_PTR)&compareFunc);

	SetSortArrow(&m_ListRefLeafs,m_currSortCol,!m_currSortDesc);
}

void CBrowseRefsDlg::OnDestroy()
{
	m_pickedRef = GetSelectedRef(true, false);

	CResizableStandAloneDialog::OnDestroy();
}

void CBrowseRefsDlg::OnNMDblclkListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = 0;

	EndDialog(IDOK);
}

CString CBrowseRefsDlg::PickRef(bool /*returnAsHash*/, CString initialRef, int pickRef_Kind)
{
	CBrowseRefsDlg dlg(CString(),NULL);

	if(initialRef.IsEmpty())
		initialRef = L"HEAD";
	dlg.m_initialRef = initialRef;
	dlg.m_pickRef_Kind = pickRef_Kind;
	dlg.m_bPickOne = true;

	if(dlg.DoModal() != IDOK)
		return CString();

	return dlg.m_pickedRef;
}

bool CBrowseRefsDlg::PickRefForCombo(CComboBoxEx* pComboBox, int pickRef_Kind)
{
	CString origRef;
	pComboBox->GetLBText(pComboBox->GetCurSel(), origRef);
	CString resultRef = PickRef(false,origRef,pickRef_Kind);
	if(resultRef.IsEmpty())
		return false;
	if(wcsncmp(resultRef,L"refs/",5)==0)
		resultRef = resultRef.Mid(5);
//	if(wcsncmp(resultRef,L"heads/",6)==0)
//		resultRef = resultRef.Mid(6);

	//Find closest match of choice in combobox
	int ixFound = -1;
	int matchLength = 0;
	CString comboRefName;
	for(int i = 0; i < pComboBox->GetCount(); ++i)
	{
		pComboBox->GetLBText(i, comboRefName);
		if(comboRefName.Find(L'/') < 0 && !comboRefName.IsEmpty())
			comboRefName.Insert(0,L"heads/"); // If combo contains single level ref name, it is usualy from 'heads/'
		if(matchLength < comboRefName.GetLength() && resultRef.Right(comboRefName.GetLength()) == comboRefName)
		{
			matchLength = comboRefName.GetLength();
			ixFound = i;
		}
	}
	if(ixFound >= 0)
		pComboBox->SetCurSel(ixFound);
	else
		ASSERT(FALSE);//No match found. So either pickRef_Kind is wrong or the combobox does not contain the ref specified in the picker (which it should unless the repo has changed before creating the CBrowseRef dialog)

	return true;
}

void CBrowseRefsDlg::OnLvnEndlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = FALSE;

	if(pDispInfo->item.pszText == NULL)
		return; //User canceled changing

	CShadowTree* pTree=(CShadowTree*)m_ListRefLeafs.GetItemData(pDispInfo->item.iItem);

	if(!pTree->IsFrom(L"refs/heads"))
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_BROWSEREFS_RENAMEONLYBRANCHES, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	CString origName = pTree->GetRefName().Mid(11);

	CString newName;
	if(m_pListCtrlRoot != NULL)
		newName = m_pListCtrlRoot->GetRefName() + L'/';
	newName += pDispInfo->item.pszText;

	if(wcsncmp(newName,L"refs/heads/",11)!=0)
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_BROWSEREFS_NOCHANGEOFTYPE, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	CString newNameTrunced = newName.Mid(11);

	CString errorMsg;
	if(g_Git.Run(L"git branch -m \"" + origName + L"\" \"" + newNameTrunced + L"\"", &errorMsg, CP_UTF8) != 0)
	{
		CMessageBox::Show(m_hWnd, errorMsg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return;
	}
	//Do as if it failed to rename. Let Refresh() do the job.
	//*pResult = TRUE;

	Refresh(newName);

//	CString W_csPopup;W_csPopup.Format8(L"Ref: %s. New name: %s. With path: %s", pTree->GetRefName(), pDispInfo->item.pszText, newName);

//	AfxMessageBox(W_csPopup);

}

void CBrowseRefsDlg::OnLvnBeginlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = FALSE;

	CShadowTree* pTree=(CShadowTree*)m_ListRefLeafs.GetItemData(pDispInfo->item.iItem);

	if(!pTree->IsFrom(L"refs/heads"))
	{
		*pResult = TRUE; //Dont allow renaming any other things then branches at the moment.
		return;
	}
}

void CBrowseRefsDlg::OnEnChangeEditFilter()
{
	SetTimer(IDT_FILTER, 1000, NULL);
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

	RECT * rect = (LPRECT)lParam;
	CPoint point;
	CString temp;
	point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_SelectedFilters & x ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_LOG_FILTER_REFNAME);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REFNAME), LOGFILTER_REFNAME, temp);

		temp.LoadString(IDS_LOG_FILTER_SUBJECT);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_SUBJECT), LOGFILTER_SUBJECT, temp);

		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);

		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);

		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (selection != 0)
		{
			m_SelectedFilters ^= selection;
			SetFilterCueText();
			SetTimer(IDT_FILTER, 1000, NULL);
		}
	}
	return 0L;
}

void CBrowseRefsDlg::SetFilterCueText()
{
	CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));
	temp += _T(" ");

	if (m_SelectedFilters & LOGFILTER_REFNAME)
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REFNAME));

	if (m_SelectedFilters & LOGFILTER_SUBJECT)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_SUBJECT));
	}

	if (m_SelectedFilters & LOGFILTER_AUTHORS)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
	}

	if (m_SelectedFilters & LOGFILTER_REVS)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
	}

	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ") + temp;
	m_ctrlFilter.SetCueBanner(temp.TrimRight());
}
