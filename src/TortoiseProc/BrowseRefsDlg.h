#pragma once

#include "Git.h"
#include <map>
#include "afxcmn.h"
#include "StandAloneDlg.h"

class CShadowTree
{
public:
	typedef std::map<CString,CShadowTree> TShadowTreeMap;

	CShadowTree():m_hTree(NULL){}
	
	CShadowTree*	GetNextSub(CString& nameLeft);

	bool			IsLeaf()const {return m_ShadowTree.empty();}


	CString			m_csName;
	CString			m_csRef;
	CString			m_csDate;
	CString			m_csAuthor;
	CString			m_csSubject;

	HTREEITEM		m_hTree;

	TShadowTreeMap	m_ShadowTree;
	CShadowTree*	m_pParent;
};

class CBrowseRefsDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CBrowseRefsDlg)

public:
	CBrowseRefsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowseRefsDlg();

	enum eCmd
	{
		eCmd_ViewLog = WM_APP
	};

// Dialog Data
	enum { IDD = IDD_DIALOG_BROWSE_REFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();

	void			Refresh();

	CShadowTree&	GetTreeNode(CString refName, CShadowTree* pTreePos=NULL);

	void			FillListCtrlForTreeNode(HTREEITEM treeNode);

	void			FillListCtrlForShadowTree(CShadowTree* pTree, CString refNamePrefix, bool isFirstLevel);

	bool			SelectRef(CString refName);

private:
//	MAP_HASH_NAME	m_RefMap;

	CShadowTree		m_TreeRoot;
	CTreeCtrl		m_RefTreeCtrl;
	CListCtrl		m_ListRefLeafs;
	afx_msg void OnTvnSelchangedTreeRef(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnNMRClickListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult);
};
