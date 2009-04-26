// BrowseRefsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "BrowseRefsDlg.h"
#include "LogDlg.h"
#include "AddRemoteDlg.h"
#include "CreateBranchTagDlg.h"


// CBrowseRefsDlg dialog

IMPLEMENT_DYNAMIC(CBrowseRefsDlg, CResizableStandAloneDialog)

CBrowseRefsDlg::CBrowseRefsDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CBrowseRefsDlg::IDD, pParent)
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
	m_ListRefLeafs.InsertColumn(0,L"Name",0,150);
	m_ListRefLeafs.InsertColumn(1,L"Date Last Commit",0,100);
	m_ListRefLeafs.InsertColumn(2,L"Last Commit",0,300);
	m_ListRefLeafs.InsertColumn(3,L"Hash",0,80);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	Refresh();


	return TRUE;
}

CShadowTree* CShadowTree::GetNextSub(CString& nameLeft)
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

	CShadowTree& nextNode=m_ShadowTree[nameSub];
	nextNode.m_csRefName=nameSub;
	nextNode.m_pParent=this;
	return &nextNode;
}

typedef std::map<CString,CString> MAP_STRING_STRING;

void CBrowseRefsDlg::Refresh()
{
//	m_RefMap.clear();
//	g_Git.GetMapHashToFriendName(m_RefMap);
		

	m_RefTreeCtrl.DeleteAllItems();
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
			  L"%(authorname)",
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
		CShadowTree& treeLeaf=GetTreeNode(iterRefMap->first);
		CString values=iterRefMap->second;

		int valuePos=0;
		treeLeaf.m_csRefHash= values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csDate=    values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csSubject= values.Tokenize(L"\04",valuePos);
		treeLeaf.m_csAuthor=  values.Tokenize(L"\04",valuePos);
	}

	CString currHead;
	g_Git.Run(L"git symbolic-ref HEAD",&currHead,CP_UTF8);

	currHead.Trim(L"\r\n\t ");

	if(!SelectRef(currHead))
		//Probably not on a branch. Select root node.
		m_RefTreeCtrl.Expand(m_TreeRoot.m_hTree,TVE_EXPAND);

}

bool CBrowseRefsDlg::SelectRef(CString refName)
{
	if(wcsnicmp(refName,L"refs/",5)!=0)
		return false; // Not a ref name

	CShadowTree& treeLeafHead=GetTreeNode(refName);
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
		}
	}

	return true;
}

CShadowTree& CBrowseRefsDlg::GetTreeNode(CString refName, CShadowTree* pTreePos)
{
	if(pTreePos==NULL)
	{
		if(wcsnicmp(refName,L"refs/",5)==0)
			refName=refName.Mid(5);
		pTreePos=&m_TreeRoot;
	}
	if(refName.IsEmpty())
		return *pTreePos;//Found leaf

	CShadowTree* pNextTree=pTreePos->GetNextSub(refName);
	if(pNextTree==NULL)
	{
		//Should not occur when all ref-names are valid.
		ASSERT(FALSE);
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

	return GetTreeNode(refName,pNextTree);
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
		int indexItem=m_ListRefLeafs.InsertItem(m_ListRefLeafs.GetItemCount(),L"");

		m_ListRefLeafs.SetItemData(indexItem,(DWORD_PTR)pTree);
		m_ListRefLeafs.SetItemText(indexItem,0,refNamePrefix+pTree->m_csRefName);
		m_ListRefLeafs.SetItemText(indexItem,1,pTree->m_csDate);
		m_ListRefLeafs.SetItemText(indexItem,2,pTree->m_csSubject);
		m_ListRefLeafs.SetItemText(indexItem,3,pTree->m_csRefHash);
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

void CBrowseRefsDlg::OnContextMenu_ListRefLeafs(CPoint point)
{

	CPoint clientPoint=point;
	m_RefTreeCtrl.ScreenToClient(&clientPoint);

	int selectedItemCount=m_ListRefLeafs.GetSelectedCount();


	std::vector<CShadowTree*> selectedTrees;
	selectedTrees.reserve(selectedItemCount);
	POSITION pos=m_ListRefLeafs.GetFirstSelectedItemPosition();
	while(pos)
	{
		selectedTrees.push_back(
			(CShadowTree*)m_ListRefLeafs.GetItemData(
				m_ListRefLeafs.GetNextSelectedItem(pos)));
	}

	CMenu popupMenu;
	popupMenu.CreatePopupMenu();

	if(selectedItemCount==1)
	{
		popupMenu.AppendMenu(MF_STRING,eCmd_ViewLog,L"View log");

//		CShadowTree* pTree = (CShadowTree*)m_ListRefLeafs.GetItemData(pNMHDR->idFrom);
//		if(pTree==NULL)
//			return;
	}


	eCmd cmd=(eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_ViewLog:
		{
			CLogDlg dlg;
			dlg.SetStartRef(selectedTrees[0]->m_csRefHash);
			dlg.DoModal();
		}
		break;
	}
}

void CBrowseRefsDlg::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if(pWndFrom==&m_RefTreeCtrl)       OnContextMenu_RefTreeCtrl(point);
	else if(pWndFrom==&m_ListRefLeafs) OnContextMenu_ListRefLeafs(point);
}

void CBrowseRefsDlg::OnContextMenu_RefTreeCtrl(CPoint point)
{
	CMenu popupMenu;
	popupMenu.CreatePopupMenu();

	CPoint clientPoint=point;
	m_RefTreeCtrl.ScreenToClient(&clientPoint);

	HTREEITEM hTreeItem=m_RefTreeCtrl.HitTest(clientPoint);
	if(hTreeItem!=NULL)
	{
		m_RefTreeCtrl.Select(hTreeItem,TVGN_CARET);
		CShadowTree* pTree=(CShadowTree*)m_RefTreeCtrl.GetItemData(hTreeItem);
		if(pTree->IsFrom(L"refs/remotes"))
			popupMenu.AppendMenu(MF_STRING,eCmd_AddRemote,L"Add Remote");
		else if(pTree->IsFrom(L"refs/heads"))
			popupMenu.AppendMenu(MF_STRING,eCmd_CreateBranch,L"Create Branch");
		else if(pTree->IsFrom(L"refs/tags"))
			popupMenu.AppendMenu(MF_STRING,eCmd_CreateTag,L"Create Tag");
	}

	eCmd cmd=(eCmd)popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN|TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(cmd)
	{
	case eCmd_AddRemote:
		{
			CAddRemoteDlg(this).DoModal();
			Refresh();
		}
		break;
	case eCmd_CreateBranch:
		{
			CCreateBranchTagDlg dlg(this);
			dlg.m_bIsTag=false;
			dlg.DoModal();
			Refresh();
		}
		break;
	case eCmd_CreateTag:
		{
			CCreateBranchTagDlg dlg(this);
			dlg.m_bIsTag=true;
			dlg.DoModal();
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
