#pragma once

#include "Git.h"
#include <map>
#include "afxcmn.h"
#include "StandAloneDlg.h"

class CShadowTree
{
public:
	typedef std::map<CString,CShadowTree> TShadowTreeMap;

	CShadowTree():m_hTree(NULL),m_pParent(NULL){}
	
	CShadowTree*	GetNextSub(CString& nameLeft);

	bool			IsLeaf()const {return m_ShadowTree.empty();}
	CString			GetRefName()const
	{
		if(m_pParent==NULL)
			return m_csRefName;
		return m_pParent->GetRefName()+"/"+m_csRefName;
	}
	bool			IsFrom(const wchar_t* from)const
	{
		return wcsncmp(GetRefName(),from,wcslen(from))==0;
	}

	CString			m_csRefName;
	CString			m_csRefHash;
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
	CBrowseRefsDlg(CString cmdPath, CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowseRefsDlg();

	enum eCmd
	{
		eCmd_ViewLog = WM_APP,
		eCmd_AddRemote,
		eCmd_ManageRemotes,
		eCmd_CreateBranch,
		eCmd_CreateTag
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
	CString			m_cmdPath;

	CShadowTree		m_TreeRoot;
	CTreeCtrl		m_RefTreeCtrl;
	CListCtrl		m_ListRefLeafs;
	afx_msg void OnTvnSelchangedTreeRef(NMHDR *pNMHDR, LRESULT *pResult);
public:

	afx_msg void OnContextMenu(CWnd* pWndFrom, CPoint point);

	void		OnContextMenu_ListRefLeafs(CPoint point);
	void		OnContextMenu_RefTreeCtrl(CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
