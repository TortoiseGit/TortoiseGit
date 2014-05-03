// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014 - TortoiseGit

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

#include <map>
#include "StandAloneDlg.h"
#include "GitHash.h"
#include "GitStatusListCtrl.h"

#define REPOBROWSER_CTRL_MIN_WIDTH	20

class CShadowFilesTree;

typedef std::map<CString, CShadowFilesTree> TShadowFilesTreeMap;

class CShadowFilesTree
{
public:
	CShadowFilesTree()
	: m_hTree(NULL)
	, m_pParent(NULL)
	, m_bFolder(false)
	, m_bLoaded(true)
	, m_bSubmodule(false)
	, m_bExecutable(false)
	, m_bSymlink(false)
	, m_iSize(0)
	{}

	CString				m_sName;
	CGitHash			m_hash;
	git_off_t			m_iSize;
	bool				m_bFolder;
	bool				m_bLoaded;
	bool				m_bSubmodule;
	bool				m_bExecutable;
	bool				m_bSymlink;

	HTREEITEM			m_hTree;

	TShadowFilesTreeMap	m_ShadowTree;
	CShadowFilesTree*	m_pParent;

	CString	GetFullName() const
	{
		if (m_pParent == NULL)
			return m_sName;

		CString parentPath = m_pParent->GetFullName();
		if (parentPath.IsEmpty())
			return m_sName;
		else
			return m_pParent->GetFullName() + _T("/") + m_sName;
	}
};
typedef std::vector<CShadowFilesTree *> TShadowFilesTreeList;

class CRepositoryBrowser : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(CString rev, CWnd* pParent = NULL);	// standard constructor
	virtual ~CRepositoryBrowser();

	// Dialog Data
	enum { IDD = IDD_REPOSITORY_BROWSER };

	static bool s_bSortLogical;

	enum eCmd
	{
		eCmd_Open = WM_APP,
		eCmd_OpenWith,
		eCmd_OpenWithAlternativeEditor,
		eCmd_ViewLog,
		eCmd_Blame,
		eCmd_CompareWC,
		eCmd_Revert,
		eCmd_SaveAs,
		eCmd_CopyPath,
		eCmd_CopyHash,
	};

	enum eCol
	{
		eCol_Name,
		eCol_Extension,
		eCol_FileSize,
	};

	enum eOpenType
	{
		ALTERNATIVEEDITOR,
		OPEN,
		OPEN_WITH,
	};

	enum eSelectionType
	{
		ONLY_FILES,
		ONLY_FILESSUBMODULES,
		ONLY_FOLDERS,
		MIXED_FOLDERS_FILES,
	};

private:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void			OnOK();
	afx_msg void			OnCancel();
	afx_msg void			OnDestroy();
	virtual BOOL			OnInitDialog();

	CTreeCtrl				m_RepoTree;
	CListCtrl				m_RepoList;
	ColumnManager			m_ColumnManager;

	afx_msg void			OnLvnColumnclickRepoList(NMHDR *pNMHDR, LRESULT *pResult);
	int						m_currSortCol;
	bool					m_currSortDesc;

	CShadowFilesTree		m_TreeRoot;
	int						ReadTreeRecursive(git_repository &repo, const git_tree * tree, CShadowFilesTree * treeroot);
	int						ReadTree(CShadowFilesTree * treeroot, const CString& root = L"");
	int						m_nIconFolder;
	int						m_nOpenIconFolder;
	int						m_nExternalOvl;
	int						m_nExecutableOvl;
	int						m_nSymlinkOvl;

	bool					m_bHasWC;

	void					Refresh();
	CString					m_sRevision;
	void					FillListCtrlForTreeNode(HTREEITEM treeNode);
	void					FillListCtrlForShadowTree(CShadowFilesTree* pTree);
	void					UpdateInfoLabel();
	afx_msg void			OnTvnSelchangedRepoTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void			OnTvnItemExpandingRepoTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void			OnLvnItemchangedRepolist(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void			OnNMDblclk_RepoList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void			OnContextMenu(CWnd* pWndFrom, CPoint point);
	void					OnContextMenu_RepoList(CPoint point);
	void					OnContextMenu_RepoTree(CPoint point);
	void					ShowContextMenu(CPoint point, TShadowFilesTreeList &selectedLeafs, eSelectionType selType);

	void					FileSaveAs(const CString path);
	void					OpenFile(const CString path, eOpenType mode, bool isSubmodule, CGitHash itemHash);
	bool					RevertItemToVersion(const CString &path);

	afx_msg void			OnBnClickedButtonRevision();

	virtual BOOL			PreTranslateMessage(MSG* pMsg);

	/// resizes the control so that the divider is at position 'point'
	void					HandleDividerMove(CPoint point, bool bDraw);
	bool					bDragMode;
	void					SaveDividerPosition();
	int						oldy, oldx;
	/// draws the bar when the tree and list control are resized
	void					DrawXorBar(CDC * pDC, int x1, int y1, int width, int height);
	afx_msg BOOL			OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void			OnCaptureChanged(CWnd *pWnd);
	afx_msg void			OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void			OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void			CopyHashToClipboard(TShadowFilesTreeList &selectedLeafs);
};
