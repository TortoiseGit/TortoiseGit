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
#pragma once

#include "Git.h"
#include <map>
#include "StandAloneDlg.h"
#include "afxwin.h"
#include "FilterEdit.h"

const int gPickRef_Head		= 1;
const int gPickRef_Tag		= 2;
const int gPickRef_Remote	= 4;
const int gPickRef_All		= gPickRef_Head | gPickRef_Tag | gPickRef_Remote;
const int gPickRef_NoTag	= gPickRef_All & ~gPickRef_Tag;

class CShadowTree
{
public:
	typedef std::map<CString,CShadowTree> TShadowTreeMap;

	CShadowTree():m_hTree(NULL),m_pParent(NULL){}

	CShadowTree*	GetNextSub(CString& nameLeft, bool bCreateIfNotExist);

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

	CShadowTree*	FindLeaf(CString partialRefName);

	CString			m_csRefName;
	CString			m_csRefHash;
	CString			m_csDate;
	CString			m_csDate_Iso8601;
	CString			m_csAuthor;
	CString			m_csSubject;

	HTREEITEM		m_hTree;

	TShadowTreeMap	m_ShadowTree;
	CShadowTree*	m_pParent;
};
typedef std::vector<CShadowTree*> VectorPShadowTree;

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
		eCmd_CreateTag,
		eCmd_DeleteBranch,
		eCmd_DeleteRemoteBranch,
		eCmd_DeleteTag,
		eCmd_ShowReflog,
		eCmd_Diff,
		eCmd_Fetch,
		eCmd_Switch,
		eCmd_Rename,
		eCmd_RepoBrowser,
	};

	enum eCol
	{
		eCol_Name,
		eCol_Date,
		eCol_Msg,
		eCol_LastAuthor,
		eCol_Hash
	};

// Dialog Data
	enum { IDD = IDD_DIALOG_BROWSE_REFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();

	CString			GetSelectedRef(bool onlyIfLeaf, bool pickFirstSelIfMultiSel = false);

	void			Refresh(CString selectRef = CString());

	CShadowTree&	GetTreeNode(CString refName, CShadowTree* pTreePos=NULL, bool bCreateIfNotExist=false);

	void			FillListCtrlForTreeNode(HTREEITEM treeNode);

	void			FillListCtrlForShadowTree(CShadowTree* pTree, CString refNamePrefix, bool isFirstLevel);

	bool			SelectRef(CString refName, bool bExactMatch);

	bool			ConfirmDeleteRef(VectorPShadowTree& leafs);
	bool			DoDeleteRefs(VectorPShadowTree& leafs, bool bForce);
	bool			DoDeleteRef(CString completeRefName, bool bForce);

	CString			GetFullRefName(CString partialRefName);

private:
	bool			m_bHasWC;

	CString			m_cmdPath;

	CShadowTree		m_TreeRoot;
	CShadowTree*	m_pListCtrlRoot;
	CTreeCtrl		m_RefTreeCtrl;
	CListCtrl		m_ListRefLeafs;

	CFilterEdit		m_ctrlFilter;
	afx_msg void OnEnChangeEditFilter();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DWORD			m_SelectedFilters;
	void			SetFilterCueText();
	afx_msg LRESULT OnClickedInfoIcon(WPARAM wParam, LPARAM lParam);
	bool			IsMatchFilter(const CShadowTree* pTree, const CString &ref, const CString &filter);

	int				m_currSortCol;
	bool			m_currSortDesc;
	afx_msg void OnTvnSelchangedTreeRef(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnContextMenu(CWnd* pWndFrom, CPoint point);

	void		OnContextMenu_ListRefLeafs(CPoint point);
	void		OnContextMenu_RefTreeCtrl(CPoint point);

	bool		AreAllFrom(VectorPShadowTree& leafs, const wchar_t* from);
	void		ShowContextMenu(CPoint point, HTREEITEM hTreePos, VectorPShadowTree& selectedLeafs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLvnColumnclickListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();
	afx_msg void OnNMDblclkListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBeginlabeleditListRefLeafs(NMHDR *pNMHDR, LRESULT *pResult);

	CString m_initialRef;
	int		m_pickRef_Kind;
	CString m_pickedRef;

public:
	static CString	PickRef(bool returnAsHash = false, CString initialRef = CString(), int pickRef_Kind = gPickRef_All);
	static bool		PickRefForCombo(CComboBoxEx* pComboBox, int pickRef_Kind = gPickRef_All);
};
