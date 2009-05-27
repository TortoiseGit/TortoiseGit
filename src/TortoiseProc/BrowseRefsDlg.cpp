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

// CBrowseRefsDlg dialog

IMPLEMENT_DYNAMIC(CBrowseRefsDlg, CResizableStandAloneDialog)

CBrowseRefsDlg::CBrowseRefsDlg(CString cmdPath, CWnd* pParent /*=NULL*/)
:	CResizableStandAloneDialog(CBrowseRefsDlg::IDD, pParent),
	m_cmdPath(cmdPath),
	m_currSortCol(-1),
	m_currSortDesc(false),
	m_initialRef(L"HEAD")
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
}


BEGIN_MESSAGE_MAP(CBrowseRefsDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CBrowseRefsDlg::OnBnClickedOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_REF, &CBrowseRefsDlg::OnTvnSelchangedTreeRef)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnColumnclickListRefLeafs)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnNMDblclkListRefLeafs)
END_MESSAGE_MAP()


// CBrowseRefsDlg message handlers

void CBrowseRefsDlg::OnBnClickedOk()
{
	OnOK();
}

BOOL CBrowseRefsDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_TREE_REF, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_LIST_REF_LEAFS, TOP_LEFT, BOTTOM_RIGHT);

	m_ListRefLeafs.SetExtendedStyle(m_ListRefLeafs.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
	m_ListRefLeafs.InsertColumn(eCol_Name,	L"Name",0,150);
	m_ListRefLeafs.InsertColumn(eCol_Date,	L"Date Last Commit",0,100);
	m_ListRefLeafs.InsertColumn(eCol_Msg,	L"Last Commit",0,300);
	m_ListRefLeafs.InsertColumn(eCol_Hash,	L"Hash",0,80);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	Refresh(m_initialRef);


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

CString CBrowseRefsDlg::GetSelectedRef(bool onlyIfLeaf)
{
	POSITION pos=m_ListRefLeafs.GetFirstSelectedItemPosition();
	//List ctrl selection?
	if(pos)
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

void CBrowseRefsDlg::Refresh(CString selectRef)
{
//	m_RefMap.clear();
//	g_Git.GetMapHashToFriendName(m_RefMap);
		
	if(!selectRef.IsEmpty())
	{
		if(selectRef == "HEAD")
		{
			selectRef.Empty();
			g_Git.Run(L"git symbolic-ref HEAD",&selectRef,CP_UTF8);
			selectRef.Trim(L"\r\n\t ");
		}
	}
	else
	{
		selectRef = GetSelectedRef(false);
	}

	m_RefTreeCtrl.DeleteAllItems();
	m_ListRefLeafs.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_csRefName="refs";
//	m_TreeRoot.m_csShowName="Refs";
	m_TreeRoot.m_hTree=m_RefTreeCtrl.InsertItem(L"Refs",NULL,NULL);
	m_RefTreeCtrl.SetItemData(m_TreeRoot.m_hTree,(DWORD_PTR)&m_TreeRoot);

	CString allRefs;
	g_Git.Run(L"git for-each-ref --format="
			  L"%(refname)%04"
			  L"%(objectname)%04"
			  L"%(authordate:relative)%04"
			  L"%(subject)%04"
			  L"%(authorname)%04"
			  L"%(authordate:iso8601)",
			  &allRefs,CP_UTF8);

	int linePos=0;
	CString singleRef;

	MAP_STRING_STRING refMap;

	//First sort on ref name
	while(!(singleRef=allRefs.Tokenize(L"\r\n",linePos)).IsEmpty())
	{
		int valuePos=0;
		CString refName=singleRef.Tokenize(L"\04",valuePos);
		CString refRest=singleRef.Mid(valuePos);
		refMap[refName]=refRest;
	}



//	for(MAP_HASH_NAME::iterator iterRef=m_RefMap.begin();iterRef!=m_RefMap.end();++iterRef)
//		for(STRING_VECTOR::iterator iterRefName=iterRef->second.begin();iterRefName!=iterRef->second.end();++iterRefName)
//			refName[*iterRefName]=iterRef->first;

	//Populate ref tree
	for(MAP_STRING_STRING::iterator iterRefMap=refMap.begin();iterRefMap!=refMap.end();++iterRefMap)
	{
		CShadowTree& treeLeaf=GetTreeNode(iterRefMap->first,NULL,true);
		CString values=iterRefMap->second;

		int valuePos=0;
		treeLeaf.m_csRefHash=		values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csDate=			values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csSubject=		values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csAuthor=		values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csDate_Iso8601=	values.Tokenize(L"\04",valuePos);
	}


	if(selectRef.IsEmpty() || !SelectRef(selectRef, false))
		//Probably not on a branch. Select root node.
		m_RefTreeCtrl.Expand(m_TreeRoot.m_hTree,TVE_EXPAND);

}

bool CBrowseRefsDlg::SelectRef(CString refName, bool bExactMatch)
{
	if(!bExactMatch)
		refName = GetFullRefName(refName);
	if(wcsnicmp(refName,L"refs/",5)!=0)
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
		if(wcsnicmp(refName,L"refs/",5)==0)
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
	m_currSortCol = -1;
	m_currSortDesc = false;
	SetSortArrow(&m_ListRefLeafs,-1,false);

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
		int indexItem=m_ListRefLeafs.InsertItem(m_ListRefLeafs.GetItemCount(),L"");

		m_ListRefLeafs.SetItemData(indexItem,(DWORD_PTR)pTree);
		m_ListRefLeafs.SetItemText(indexItem,eCol_Name,	refNamePrefix+pTree->m_csRefName);
		m_ListRefLeafs.SetItemText(indexItem,eCol_Date,	pTree->m_csDate);
		m_ListRefLeafs.SetItemText(indexItem,eCol_Msg,	pTree->m_csSubject);
		m_ListRefLeafs.SetItemText(indexItem,eCol_Hash,	pTree->m_csRefHash);
	}
	else
	{

		CString csThisName;
		if(!isFirstLevel)
			csThisName=refNamePrefix+pTree->m_csRefName+L"/";
		for(CShadowTree::TShadowTreeMap::iterator itSubTree=pTree->m_ShadowTree.begin(); itSubTree!=pTree->m_ShadowTree.end(); ++itSubTree)
		{
			FillListCtrlForShadowTree(&itSubTree->second,csThisName,false);
		}
	}
}

bool CBrowseRefsDlg::ConfirmDeleteRef(CString completeRefName)
{
	CString csMessage;
	CString csTitle;

	UINT mbIcon=MB_ICONQUESTION;
	csMessage=L"Are you sure you want to delete the ";
	if(wcsncmp(completeRefName,L"refs/heads",10)==0)
	{
		CString branchToDelete = completeRefName.Mid(11);
		csTitle.Format(L"Confirm deletion of branch %s", branchToDelete);
		csMessage += "branch:\r\n\r\n<b>";
		csMessage += branchToDelete;
		csMessage += "</b>";

		//Check if branch is fully merged in HEAD
		CString branchHash = g_Git.GetHash(completeRefName);
		CString commonAncestor;
		CString cmd;
		cmd.Format(L"git.exe merge-base HEAD %s",completeRefName);
		g_Git.Run(cmd,&commonAncestor,CP_UTF8);

		branchHash=branchHash.Left(40);
		commonAncestor=commonAncestor.Left(40);
		
		if(commonAncestor != branchHash)
		{
			csMessage += L"\r\n\r\n<b>Warning:\r\nThis branch is not fully merged into HEAD.</b>";
			mbIcon=MB_ICONWARNING;
		}
	}
	else if(wcsncmp(completeRefName,L"refs/tags",9)==0)
	{
		CString tagToDelete = completeRefName.Mid(10);
		csTitle.Format(L"Confirm deletion of tag %s", tagToDelete);
		csMessage += "tag:\r\n\r\n<b>";
		csMessage += tagToDelete;
		csMessage += "</b>";
	}

	return CMessageBox::Show(m_hWnd,csMessage,csTitle,MB_YESNO|mbIcon)==IDYES;

}


bool CBrowseRefsDlg::DoDeleteRef(CString completeRefName, bool bForce)
{
	if(wcsncmp(completeRefName,L"refs/heads",10)==0)
	{
		CString branchToDelete = completeRefName.Mid(11);
		CString cmd;
		cmd.Format(L"git.exe branch -%c %s",bForce?L'D':L'd',branchToDelete);
		CString resultDummy;
		if(g_Git.Run(cmd,&resultDummy,CP_UTF8)!=0)
		{
			CString errorMsg;
			errorMsg.Format(L"Could not delete branch %s. Message from git:\r\n\r\n%s",branchToDelete,resultDummy);
			CMessageBox::Show(m_hWnd,errorMsg,L"Error deleting branch",MB_OK|MB_ICONERROR);
			return false;
		}
	}
	else if(wcsncmp(completeRefName,L"refs/tags",9)==0)
	{
		CString tagToDelete = completeRefName.Mid(10);
		CString cmd;
		cmd.Format(L"git.exe tag -d %s",tagToDelete);
		CString resultDummy;
		if(g_Git.Run(cmd,&resultDummy,CP_UTF8)!=0)
		{
			CString errorMsg;
			errorMsg.Format(L"Could not delete tag %s. Message from git:\r\n\r\n%s",tagToDelete,resultDummy);
			CMessageBox::Show(m_hWnd,errorMsg,L"Error deleting tag",MB_OK|MB_ICONERROR);
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

	ShowContextMenu(point,hTreeItem,VectorPShadowTree());
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
	CMenu popupMenu;
	popupMenu.CreatePopupMenu();

	if(selectedLeafs.size()==1)
	{
		popupMenu.AppendMenu(MF_STRING,eCmd_ViewLog,L"View log");
		if(selectedLeafs[0]->IsFrom(L"refs/heads"))
			popupMenu.AppendMenu(MF_STRING,eCmd_DeleteBranch,L"Delete Branch");
		else if(selectedLeafs[0]->IsFrom(L"refs/tags"))
			popupMenu.AppendMenu(MF_STRING,eCmd_DeleteTag,L"Delete Tag");

//		CShadowTree* pTree = (CShadowTree*)m_ListRefLeafs.GetItemData(pNMHDR->idFrom);
//		if(pTree==NULL)
//			return;
	}

	if(hTreePos!=NULL)
	{
		CShadowTree* pTree=(CShadowTree*)m_RefTreeCtrl.GetItemData(hTreePos);
		if(pTree->IsFrom(L"refs/remotes"))
		{
//			popupMenu.AppendMenu(MF_STRING,eCmd_AddRemote,L"Add Remote");
			if(!m_cmdPath.IsEmpty())
				popupMenu.AppendMenu(MF_STRING,eCmd_ManageRemotes,L"Manage Remotes");
		}
		else if(pTree->IsFrom(L"refs/heads"))
			popupMenu.AppendMenu(MF_STRING,eCmd_CreateBranch,L"Create Branch");
		else if(pTree->IsFrom(L"refs/tags"))
			popupMenu.AppendMenu(MF_STRING,eCmd_CreateTag,L"Create Tag");
	}


	eCmd cmd=(eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_ViewLog:
		{
			CLogDlg dlg;
			dlg.SetStartRef(selectedLeafs[0]->m_csRefHash);
			dlg.DoModal();
		}
		break;
	case eCmd_DeleteBranch:
		{
			if(ConfirmDeleteRef(selectedLeafs[0]->GetRefName()))
				DoDeleteRef(selectedLeafs[0]->GetRefName(), true);
			Refresh();
		}
		break;
	case eCmd_DeleteTag:
		{
			if(ConfirmDeleteRef(selectedLeafs[0]->GetRefName()))
				DoDeleteRef(selectedLeafs[0]->GetRefName(), true);
			Refresh();
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
			CSinglePropSheetDlg(L"Git Remote Settings",new CSettingGitRemote(m_cmdPath),this).DoModal();
//			CSettingGitRemote W_Remotes(m_cmdPath);
//			W_Remotes.DoModal();
			Refresh();
		}
		break;
	case eCmd_CreateBranch:
		{
			CAppUtils::CreateBranchTag(false);
			Refresh();
		}
		break;
	case eCmd_CreateTag:
		{
			CAppUtils::CreateBranchTag(true);
			Refresh();
		}
		break;
	}
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
*/		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}


	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

class CRefLeafListCompareFunc
{
public:
	CRefLeafListCompareFunc(CListCtrl* pList, int col, bool desc):m_col(col),m_desc(desc),m_pList(pList){}

	static int CALLBACK StaticCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		return ((CRefLeafListCompareFunc*)lParamSort)->Compare(lParam1,lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		return Compare(
			(CShadowTree*)m_pList->GetItemData(lParam1), 
			(CShadowTree*)m_pList->GetItemData(lParam2));
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
		case CBrowseRefsDlg::eCol_Name:	return pLeft->GetRefName().CompareNoCase(pRight->GetRefName());
		case CBrowseRefsDlg::eCol_Date:	return pLeft->m_csDate_Iso8601.CompareNoCase(pRight->m_csDate_Iso8601);
		case CBrowseRefsDlg::eCol_Msg:	return pLeft->m_csSubject.CompareNoCase(pRight->m_csSubject);
		case CBrowseRefsDlg::eCol_Hash:	return pLeft->m_csRefHash.CompareNoCase(pRight->m_csRefHash);
		}
		return 0;
	}

	int m_col;
	bool m_desc;
	CListCtrl* m_pList;


};


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
	m_pickedRef = GetSelectedRef(true);

	CResizableStandAloneDialog::OnDestroy();
}

void CBrowseRefsDlg::OnNMDblclkListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	EndDialog(IDOK);
}

CString CBrowseRefsDlg::PickRef(bool returnAsHash, CString initialRef)
{
	CBrowseRefsDlg dlg(CString(),NULL);
	
	dlg.m_initialRef = initialRef;

	if(dlg.DoModal() != IDOK)
		return CString();

	return dlg.m_pickedRef;
}

