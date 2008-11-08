// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "SVN.h"
#include "TSVNPath.h"
#include "Blame.h"
#include "SVN.h"
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
class CFileDiffDlg : public CResizableStandAloneDialog, public SVN
{
	DECLARE_DYNAMIC(CFileDiffDlg)
public:
	class FileDiff
	{
	public:
		CTSVNPath path;
		svn_client_diff_summarize_kind_t kind; 
		bool propchanged;
		svn_node_kind_t node;
	};
public:
	CFileDiffDlg(CWnd* pParent = NULL);
	virtual ~CFileDiffDlg();

	void SetDiff(const CTSVNPath& path1, SVNRev rev1, const CTSVNPath& path2, SVNRev rev2, svn_depth_t depth, bool ignoreancestry);
	void SetDiff(const CTSVNPath& path, SVNRev peg, SVNRev rev1, SVNRev rev2, svn_depth_t depth, bool ignoreancestry);

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

	virtual svn_error_t* DiffSummarizeCallback(const CTSVNPath& path, 
											svn_client_diff_summarize_kind_t kind, 
											bool propchanged, 
											svn_node_kind_t node);

	int					AddEntry(const FileDiff * fd);
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
	CBlame				m_blamer;
	std::vector<FileDiff> m_arFileList;
	std::vector<FileDiff> m_arFilteredList;
	CArray<FileDiff, FileDiff> m_arSelectedFileList;

	CString				m_strExportDir;
	CProgressDlg *		m_pProgDlg;

	int					m_nIconFolder;

	CTSVNPath			m_path1;
	SVNRev				m_peg;
	SVNRev				m_rev1;
	CTSVNPath			m_path2;
	SVNRev				m_rev2;
	svn_depth_t			m_depth;
	bool				m_bIgnoreancestry;
	bool				m_bDoPegDiff;
	volatile LONG		m_bThreadRunning;

	bool				m_bCancelled;

	void				Sort();
	static bool			SortCompare(const FileDiff& Data1, const FileDiff& Data2);

	static BOOL			m_bAscending;
	static int			m_nSortedColumn;
};
