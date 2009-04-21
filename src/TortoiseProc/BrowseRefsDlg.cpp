// BrowseRefsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "BrowseRefsDlg.h"


// CBrowseRefsDlg dialog

IMPLEMENT_DYNAMIC(CBrowseRefsDlg, CDialog)

CBrowseRefsDlg::CBrowseRefsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseRefsDlg::IDD, pParent)
{

}

CBrowseRefsDlg::~CBrowseRefsDlg()
{
}

void CBrowseRefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_REF, m_RefTreeCtrl);
}


BEGIN_MESSAGE_MAP(CBrowseRefsDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CBrowseRefsDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CBrowseRefsDlg message handlers

void CBrowseRefsDlg::OnBnClickedOk()
{
	OnOK();
}

BOOL CBrowseRefsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Refresh();


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
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
	nextNode.m_csName=nameSub;
	nextNode.m_pParent=this;
	return &nextNode;
}

typedef std::map<CString,CString> MAP_STRING_STRING;

void CBrowseRefsDlg::Refresh()
{
	m_RefMap.clear();
	g_Git.GetMapHashToFriendName(m_RefMap);

	m_RefTreeCtrl.DeleteAllItems();
	m_TreeRoot.m_ShadowTree.clear();
	m_TreeRoot.m_csName="Refs";
	m_TreeRoot.m_hTree=m_RefTreeCtrl.InsertItem(L"Refs",NULL,NULL);
	m_RefTreeCtrl.SetItemData(m_TreeRoot.m_hTree,(DWORD_PTR)&m_TreeRoot);


	MAP_STRING_STRING refName;

	//First sort on ref name
	for(MAP_HASH_NAME::iterator iterRef=m_RefMap.begin();iterRef!=m_RefMap.end();++iterRef)
		for(STRING_VECTOR::iterator iterRefName=iterRef->second.begin();iterRefName!=iterRef->second.end();++iterRefName)
			refName[*iterRefName]=iterRef->first;

	//Populate ref tree
	for(MAP_STRING_STRING::iterator iterRefName=refName.begin();iterRefName!=refName.end();++iterRefName)
	{
		CShadowTree& treeLeaf=GetTreeNode(iterRefName->first);
		treeLeaf.m_csRef=iterRefName->second;
	}

	m_RefTreeCtrl.Expand(m_TreeRoot.m_hTree,TVE_EXPAND);

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
	if(pNextTree->m_hTree==NULL)
	{
		//New tree. Create node in control.
		pNextTree->m_hTree=m_RefTreeCtrl.InsertItem(pNextTree->m_csName,pTreePos->m_hTree,NULL);
		m_RefTreeCtrl.SetItemData(pNextTree->m_hTree,(DWORD_PTR)pNextTree);
	}

	return GetTreeNode(refName,pNextTree);
}

