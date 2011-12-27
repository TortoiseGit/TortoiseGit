// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011 - TortoiseGit

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
	m_initialRef(L"HEAD"),
	m_pickRef_Kind(gPickRef_All),
	m_pListCtrlRoot(NULL),
	m_bHasWC(true)
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
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnEndlabeleditListRefLeafs)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_REF_LEAFS, &CBrowseRefsDlg::OnLvnBeginlabeleditListRefLeafs)
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

	AddAnchor(IDC_TREE_REF, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_LIST_REF_LEAFS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	m_ListRefLeafs.SetExtendedStyle(m_ListRefLeafs.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
	m_ListRefLeafs.InsertColumn(eCol_Name,	L"Name",0,150);
	m_ListRefLeafs.InsertColumn(eCol_Date,	L"Date Last Commit",0,100);
	m_ListRefLeafs.InsertColumn(eCol_Msg,	L"Last Commit",0,300);
	m_ListRefLeafs.InsertColumn(eCol_Hash,	L"Hash",0,80);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	Refresh(m_initialRef);

	EnableSaveRestore(L"BrowseRefs");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_bHasWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);

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
		if (!(pTree->m_csRefName.IsEmpty() || pTree->m_csRefName == "refs" && pTree->m_pParent == NULL))
		{
			int indexItem = m_ListRefLeafs.InsertItem(m_ListRefLeafs.GetItemCount(), L"");

			m_ListRefLeafs.SetItemData(indexItem,(DWORD_PTR)pTree);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Name, refNamePrefix+pTree->m_csRefName);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Date, pTree->m_csDate);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Msg, pTree->m_csSubject);
			m_ListRefLeafs.SetItemText(indexItem,eCol_Hash, pTree->m_csRefHash);
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
}

bool CBrowseRefsDlg::ConfirmDeleteRef(VectorPShadowTree& leafs)
{
	ASSERT(!leafs.empty());

	CString csMessage;
	CString csTitle;

	UINT mbIcon=MB_ICONQUESTION;
	csMessage = L"Are you sure you want to delete ";

	bool bIsRemoteBranch = false;
	bool bIsBranch = false;
	if		(leafs[0]->IsFrom(L"refs/remotes"))	{bIsBranch = true; bIsRemoteBranch = true;}
	else if	(leafs[0]->IsFrom(L"refs/heads"))	{bIsBranch = true;}

	if(bIsBranch)
	{
		if(leafs.size() == 1)
		{
			CString branchToDelete = leafs[0]->GetRefName().Mid(bIsRemoteBranch ? 13 : 11);
			csTitle.Format(L"Confirm deletion of %sbranch %s",
				bIsRemoteBranch? L"remote ": L"",
				branchToDelete);

			csMessage += "the ";
			if(bIsRemoteBranch)
				csMessage += L"<ct=0x0000FF><i>remote</i></ct> ";
			csMessage += L"branch:\r\n\r\n<b>";
			csMessage += branchToDelete;
			csMessage += L"</b>";

			//Check if branch is fully merged in HEAD
			CGitHash branchHash = g_Git.GetHash(leafs[0]->GetRefName());
			CGitHash commonAncestor;
			CString commonAncestorstr;
			CString cmd;
			cmd.Format(L"git.exe merge-base HEAD %s", leafs[0]->GetRefName());
			g_Git.Run(cmd, &commonAncestorstr, NULL, CP_UTF8);

			commonAncestor=commonAncestorstr;

			if(commonAncestor != branchHash)
			{
				csMessage += L"\r\n\r\n<b>Warning:\r\nThis branch is not fully merged into HEAD.</b>";
				mbIcon = MB_ICONWARNING;
			}
			if(bIsRemoteBranch)
			{
				csMessage += L"\r\n\r\n<b>Warning:\r\nThis action will remove the branch on the remote.</b>";
				mbIcon = MB_ICONWARNING;
			}
		}
		else
		{
			csTitle.Format(L"Confirm deletion of %d %sbranches",
				leafs.size(),
				bIsRemoteBranch? L"remote ": L"");

			CString csMoreMsgText;
			csMoreMsgText.Format(L"<b>%d</b> ", leafs.size());
			csMessage += csMoreMsgText;
			if(bIsRemoteBranch)
				csMessage += L"<ct=0x0000FF><i>remote</i></ct> ";
			csMessage += L"branches";

			csMessage += L"\r\n\r\n<b>Warning:\r\nIt has not been checked if these branches have been fully merged into HEAD.</b>";
			mbIcon = MB_ICONWARNING;

			if(bIsRemoteBranch)
			{
				csMessage += L"\r\n\r\n<b>Warning:\r\nThis action will remove the branches on the remote.</b>";
				mbIcon = MB_ICONWARNING;
			}
		}

	}
	else if(leafs[0]->IsFrom(L"refs/tags"))
	{
		if(leafs.size() == 1)
		{
			CString tagToDelete = leafs[0]->GetRefName().Mid(10);
			csTitle.Format(L"Confirm deletion of tag %s", tagToDelete);
			csMessage += "the tag:\r\n\r\n<b>";
			csMessage += tagToDelete;
			csMessage += "</b>";
		}
		else
		{
			CString tagToDelete = leafs[0]->GetRefName().Mid(10);
			csTitle.Format(L"Confirm deletion of %d tags", leafs.size());
			CString csMoreMsgText;
			csMoreMsgText.Format(L"<b>%d</b> ", leafs.size());
			csMessage += csMoreMsgText;
			csMessage += L"tags";
		}
	}

	return CMessageBox::Show(m_hWnd,csMessage,csTitle,MB_YESNO|mbIcon)==IDYES;

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

			cmd.Format(L"git.exe push \"%s\" :%s", remoteName, remoteBranchToDelete);
		}
		else
			cmd.Format(L"git.exe branch -%c -- %s",bForce?L'D':L'd',branchToDelete);
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
		cmd.Format(L"git.exe tag -d -- %s",tagToDelete);
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

		CString fetchFromCmd;

		if(selectedLeafs[0]->IsFrom(L"refs/heads"))
		{
			bShowReflogOption = true;
			bShowSwitchOption = true;
			bShowRenameOption = true;
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
			fetchFromCmd.Format(L"Fetch from \"%s\"", remoteName);
		}
		else if(selectedLeafs[0]->IsFrom(L"refs/tags"))
		{
		}

											popupMenu.AppendMenuIcon(eCmd_ViewLog, L"Show Log", IDI_LOG);
		if(bShowReflogOption)				popupMenu.AppendMenuIcon(eCmd_ShowReflog, L"Show Reflog", IDI_LOG);

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
			popupMenu.AppendMenuIcon(eCmd_Switch, L"Switch to this Ref", IDI_SWITCH);
			popupMenu.AppendMenu(MF_SEPARATOR);
		}

		if(bShowCreateBranchOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, L"Create branch", IDI_COPY);
		}

		if(bShowRenameOption)
		{
			bAddSeparator = true;
			popupMenu.AppendMenuIcon(eCmd_Rename, L"Rename", IDI_RENAME);
		}
	}
	else if(selectedLeafs.size() == 2)
	{
		bAddSeparator = true;
		popupMenu.AppendMenuIcon(eCmd_Diff, L"Compare these Refs", IDI_DIFF);
	}

	if(!selectedLeafs.empty())
	{
		if(AreAllFrom(selectedLeafs, L"refs/remotes/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName = L"Delete remote branch";
			else
				menuItemName.Format(L"Delete %d remote branches", selectedLeafs.size());

			popupMenu.AppendMenuIcon(eCmd_DeleteRemoteBranch, menuItemName, IDI_DELETE);
			bAddSeparator = true;
		}
		else if(AreAllFrom(selectedLeafs, L"refs/heads/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName = L"Delete branch";
			else
				menuItemName.Format(L"Delete %d branches", selectedLeafs.size());

			popupMenu.AppendMenuIcon(eCmd_DeleteBranch, menuItemName, IDI_DELETE);
			bAddSeparator = true;
		}
		else if(AreAllFrom(selectedLeafs, L"refs/tags/"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			CString menuItemName;
			if(selectedLeafs.size() == 1)
				menuItemName = L"Delete tag";
			else
				menuItemName.Format(L"Delete %d tags", selectedLeafs.size());

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
			popupMenu.AppendMenuIcon(eCmd_ManageRemotes, L"Manage Remotes", IDI_SETTINGS);
			bAddSeparator = true;
			if(selectedLeafs.empty())
			{
				int dummy = 0;//Needed for tokenize
				remoteName = pTree->GetRefName();
				remoteName = remoteName.Mid(13);
				remoteName = remoteName.Tokenize(L"/", dummy);
				if(!remoteName.IsEmpty())
				{
					CString fetchFromCmd;
					fetchFromCmd.Format(L"Fetch from %s", remoteName);
					popupMenu.AppendMenuIcon(eCmd_Fetch, fetchFromCmd, IDI_PULL);
				}
			}
		}
		if(pTree->IsFrom(L"refs/heads"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_CreateBranch, L"Create branch", IDI_COPY);
		}
		if(pTree->IsFrom(L"refs/tags"))
		{
			if(bAddSeparator)
				popupMenu.AppendMenu(MF_SEPARATOR);
			popupMenu.AppendMenuIcon(eCmd_CreateTag, L"Create tag", IDI_TAG);
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
	case eCmd_Switch:
		{
			CAppUtils::Switch(NULL, selectedLeafs[0]->GetRefName());
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
			CSinglePropSheetDlg(L"Git Remote Settings",new CSettingGitRemote(g_Git.m_CurrentDir),this).DoModal();
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
		CMessageBox::Show(m_hWnd, L"At the moment, you can only rename branches.", L"Cannot Rename This Ref",MB_OK|MB_ICONERROR);
		return;
	}

	CString origName = pTree->GetRefName().Mid(11);

	CString newName;
	if(m_pListCtrlRoot != NULL)
		newName = m_pListCtrlRoot->GetRefName() + L'/';
	newName += pDispInfo->item.pszText;

	if(wcsncmp(newName,L"refs/heads/",11)!=0)
	{
		CMessageBox::Show(m_hWnd, L"You cannot change the type of this ref with a rename.", L"Cannot Change Ref Type",MB_OK|MB_ICONERROR);
		return;
	}

	CString newNameTrunced = newName.Mid(11);

	CString result;
	if(g_Git.Run(L"git branch -m \"" + origName + L"\" \"" + newNameTrunced + L"\"", &result, CP_UTF8) != 0)
	{
		CString errorMsg;
		errorMsg.Format(L"Could not rename branch %s. Message from git:\r\n\r\n%s",origName,result);
		CMessageBox::Show(m_hWnd,errorMsg,L"Error Renaming Branch",MB_OK|MB_ICONERROR);
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
