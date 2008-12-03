// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "Git.h"
#include "TGitPath.h"
//#include "Blame.h"
#include "Git.h"
#include "HintListCtrl.h"
#include "Colors.h"
#include "XPImageButton.h"
#include "FilterEdit.h"
#include "Tooltip.h"


#define IDT_FILTER		101

/**
 * \ingroup TortoiseProc
 * Dialog which fetches and shows the difference between two urls in the
 * repository. It shows a list of files/folders which were changed in those
 * two revisions.
 */
class CFileDiffDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFileDiffDlg)
public:
//	class FileDiff
//	{
//	public:
//		CTGitPath path;
//		svn_client_diff_summarize_kind_t kind; 
		bool propchanged;
//		svn_node_kind_t node;
//	};

public:
	CFileDiffDlg(CWnd* pParent = NULL);
	virtual ~CFileDiffDlg();

//	void SetDiff(const CTGitPath& path1, GitRev rev1, const CTGitPath& path2, GitRev rev2, svn_depth_t depth, bool ignoreancestry);
//	void SetDiff(const CTGitPath& path, GitRev peg, GitRev rev1, GitRev rev2, svn_depth_t depth, bool ignoreancestry);

	void SetDiff(CTGitPath * path, GitRev rev1,GitRev rev2);

	void	DoBlame(bool blame = true) {m_bBlame = blame;}

	enum { IDD = IDD_DIFFFILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnEnSetfocusSecondurl();
	afx_msg void OnEnSetfocusFirsturl();
	afx_msg void OnBnClickedSwitchleftright();
	afx_msg void OnHdnItemclickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRev1btn();
	afx_msg void OnBnClickedRev2btn();
	afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeFilter();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

//	virtual svn_error_t* DiffSummarizeCallback(const CTGitPath& path, 
//											svn_client_diff_summarize_kind_t kind, 
//											bool propchanged, 
//											svn_node_kind_t node);

	int					AddEntry(const CTGitPath * fd);
	void				DoDiff(int selIndex, bool blame);
	void				DiffProps(int selIndex);
	void				SetURLLabels();
	void				Filter(CString sFilterText);
	void				CopySelectionToClipboard();
private:
	static UINT			DiffThreadEntry(LPVOID pVoid);
	UINT				DiffThread();
	static UINT			ExportThreadEntry(LPVOID pVoid);
	UINT				ExportThread();

	virtual BOOL		Cancel() {return m_bCancelled;}

	CToolTips			m_tooltips;

	CButton				m_cRev1Btn;
	CButton				m_cRev2Btn;
	CFilterEdit			m_cFilter;

	CXPImageButton		m_SwitchButton;
	HICON				m_hSwitchIcon;
	CColors				m_colors;
	CHintListCtrl		m_cFileList;
	bool				m_bBlame;
//	CBlame				m_blamer;
	CTGitPathList		m_arFileList;
	std::vector<CTGitPath*> m_arFilteredList;
	CArray<CTGitPath*, CTGitPath*> m_arSelectedFileList;

	CString				m_strExportDir;
	CProgressDlg *		m_pProgDlg;

	int					m_nIconFolder;

	CTGitPath			m_path1;
	GitRev				m_peg;
	GitRev				m_rev1;
	CTGitPath			m_path2;
	GitRev				m_rev2;

	bool				m_bIgnoreancestry;
	bool				m_bDoPegDiff;
	volatile LONG		m_bThreadRunning;

	bool				m_bCancelled;

	void				Sort();
//	static bool			SortCompare(const FileDiff& Data1, const FileDiff& Data2);

	static BOOL			m_bAscending;
	static int			m_nSortedColumn;
};
