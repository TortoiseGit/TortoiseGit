// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
// Copyright (C) 2012 - Sven Strickroth <email@cs-ware.de>

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

#include "git2.h"
#include <map>
#include "StandAloneDlg.h"

class CShadowFilesTree;

typedef std::map<CString, CShadowFilesTree> TShadowFilesTreeMap;

class CShadowFilesTree
{
public:
	CShadowFilesTree()
	: m_hTree(NULL)
	, m_pParent(NULL)
	, m_bFolder(false)
	, m_iSize(0)
	{}

	CString				m_sName;
	size_t				m_iSize;
	bool				m_bFolder;

	HTREEITEM			m_hTree;

	TShadowFilesTreeMap	m_ShadowTree;
	CShadowFilesTree*	m_pParent;

	CString	GetFullName() const
	{
		if (m_pParent == NULL)
			return m_sName;

		return m_pParent->GetFullName() + _T("/") + m_sName;
	}
};
typedef std::vector<CShadowFilesTree *> TShadowFilesTreeList;

class CRepositoryBrowser : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(CWnd* pParent = NULL);	// standard constructor
	virtual ~CRepositoryBrowser();

	// Dialog Data
	enum { IDD = IDD_REPOSITORY_BROWSER };

	enum eCmd
	{
		eCmd_ViewLog = WM_APP,
	};

	enum eCol
	{
		eCol_Name,
		eCol_FileSize,
	};

private:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void			OnOK();
	afx_msg void			OnCancel();
	virtual BOOL			OnInitDialog();

	CTreeCtrl				m_RepoTree;
	CListCtrl				m_RepoList;

	CShadowFilesTree		m_TreeRoot;
	int						ReadTreeRecursive(git_repository &repo, git_tree * tree, CShadowFilesTree * treeroot);
	int						ReadTree(CShadowFilesTree * treeroot);
	int						m_nIconFolder;
	int						m_nOpenIconFolder;

	void					Refresh();
	CString					m_sRevision;
	void					FillListCtrlForTreeNode(HTREEITEM treeNode);
	void					FillListCtrlForShadowTree(CShadowFilesTree* pTree);
	afx_msg void			OnTvnSelchangedRepoTree(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void			OnContextMenu(CWnd* pWndFrom, CPoint point);
	void					OnContextMenu_RepoList(CPoint point);

	afx_msg void			OnBnClickedButtonRevision();

	virtual BOOL			PreTranslateMessage(MSG* pMsg);
};
