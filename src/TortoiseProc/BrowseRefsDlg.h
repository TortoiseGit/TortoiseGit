#pragma once

#include "Git.h"
#include <map>
#include "afxcmn.h"

class CShadowTree
{
public:
	typedef std::map<CString,CShadowTree> TShadowTreeMap;

	CShadowTree():m_hTree(NULL){}
	
	CShadowTree*	GetNextSub(CString& nameLeft);


	CString			m_csName;
	CString			m_csRef;

	HTREEITEM		m_hTree;

	TShadowTreeMap	m_ShadowTree;
	CShadowTree*	m_pParent;
};

class CBrowseRefsDlg : public CDialog
{
	DECLARE_DYNAMIC(CBrowseRefsDlg)

public:
	CBrowseRefsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowseRefsDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_BROWSE_REFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();

	void		Refresh();

	CShadowTree&	GetTreeNode(CString refName, CShadowTree* pTreePos=NULL);

	MAP_HASH_NAME	m_RefMap;

	CShadowTree		m_TreeRoot;
	CTreeCtrl		m_RefTreeCtrl;
};
