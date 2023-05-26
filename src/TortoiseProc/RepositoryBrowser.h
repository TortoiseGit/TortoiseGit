// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014, 2016-2017, 2020-2023 - TortoiseGit

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
#include "GestureEnabledControl.h"

#define REPOBROWSER_CTRL_MIN_WIDTH	20

class CShadowFilesTree;

using TShadowFilesTreeMap = std::map<CString, CShadowFilesTree>;

class CShadowFilesTree
{
public:
	CShadowFilesTree() = default;

	CString				m_sName;
	CGitHash			m_hash;
	git_off_t			m_iSize = 0;
	bool				m_bFolder = false;
	bool				m_bLoaded = true;
	bool				m_bSubmodule = false;
	bool				m_bExecutable = false;
	bool				m_bSymlink = false;

	HTREEITEM			m_hTree = nullptr;

	TShadowFilesTreeMap	m_ShadowTree;
	CShadowFilesTree*	m_pParent = nullptr;

	CString	GetFullName() const
	{
		if (!m_pParent)
			return m_sName;

		CString parentPath = m_pParent->GetFullName();
		if (parentPath.IsEmpty())
			return m_sName;
		else
			return m_pParent->GetFullName() + L'/' + m_sName;
	}
};
using TShadowFilesTreeList = std::vector<CShadowFilesTree*>;

class CRepositoryBrowser : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(CString rev, CWnd* pParent = nullptr);	// standard constructor
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
		eCmd_ViewLogSubmodule,
		eCmd_PrepareDiff,
		eCmd_PrepareDiff_Compare,
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
	void DoDataExchange(CDataExchange* pDX) override;	// DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void			OnOK() override;
	afx_msg void			OnCancel() override;
	afx_msg void			OnDestroy();
	BOOL					OnInitDialog() override;

	CGestureEnabledControlTmpl<CTreeCtrl>	m_RepoTree;
	CGestureEnabledControlTmpl<CListCtrl>	m_RepoList;
	ColumnManager			m_ColumnManager;

	afx_msg void			OnLvnColumnclickRepoList(NMHDR *pNMHDR, LRESULT *pResult);
	int						m_currSortCol = 0;
	bool					m_currSortDesc = false;

	CShadowFilesTree		m_TreeRoot;
	int						ReadTreeRecursive(git_repository& repo, const git_tree* tree, CShadowFilesTree* treeroot, bool recursive);
	int						ReadTree(CShadowFilesTree* treeroot, const CString& root = L"", bool recursive = false);
	int						m_nIconFolder = 0;
	int						m_nOpenIconFolder = 0;
	int						m_nExternalOvl = 0;
	int						m_nExecutableOvl = 0;
	int						m_nSymlinkOvl = 0;

	CShadowFilesTree*		GetListEntry(int index);
	CShadowFilesTree*		GetTreeEntry(HTREEITEM treeItem);

	bool					m_bHasWC = true;

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
	void					OpenFile(const CString path, eOpenType mode, bool isSubmodule, const CGitHash& itemHash);
	bool					RevertItemToVersion(const CString &path);

	afx_msg void			OnBnClickedButtonRevision();

	BOOL					PreTranslateMessage(MSG* pMsg) override;

	void					UpdateDiffWithFileFromReg();
	CString					m_sMarkForDiffFilename;
	CGitHash				m_sMarkForDiffVersion;

	/// resizes the control so that the divider is at position 'point'
	void					HandleDividerMove(CPoint point, bool bDraw);
	bool					bDragMode = false;
	void					SaveDividerPosition();
	int						m_oldSliderXPos = 0;
	/// draws the bar when the tree and list control are resized
	void					DrawXorBar(CDC * pDC, int x1, int y1, int width, int height);
	afx_msg BOOL			OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void			OnCaptureChanged(CWnd *pWnd);
	afx_msg void			OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void			OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void			CopyHashToClipboard(TShadowFilesTreeList &selectedLeafs);
	afx_msg void			OnLvnBegindragRepolist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void			OnTvnBegindragRepotree(NMHDR* pNMHDR, LRESULT* pResult);
	void					BeginDrag(const CWnd& window, CTGitPathList& files, const CString& root, POINT& point);
	void					RecursivelyAdd(CTGitPathList& toExport, CShadowFilesTree* pTree);
};
