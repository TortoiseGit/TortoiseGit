// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
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
#include "Git.h"
#include "TGitPath.h"
#include "Git.h"
#include "HintListCtrl.h"
#include "Colors.h"
#include "FilterEdit.h"
#include "MessageBox.h"
#include "Tooltip.h"
#include "ProgressDlg.h"
#include "MenuButton.h"
#include "ACEdit.h"
#define IDT_FILTER		101
#define IDT_INPUT		102

/**
 * \ingroup TortoiseProc
 * Dialog which fetches and shows the difference between two urls in the
 * repository. It shows a list of files/folders which were changed in those
 * two revisions.
 */
#define MSG_REF_LOADED	(WM_USER+120)

class CFileDiffDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFileDiffDlg)
public:
	CFileDiffDlg(CWnd* pParent = NULL);
	virtual ~CFileDiffDlg();

	void SetDiff(const CTGitPath * path, const GitRev &rev1, const GitRev &rev2);
	void SetDiff(const CTGitPath * path, const GitRev &rev1);
	void SetDiff(const CTGitPath * path, const CString &hash1, const CString &hash2);

	void	DoBlame(bool blame = true) {m_bBlame = blame;}

	enum { IDD = IDD_DIFFFILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT OnRefLoad(WPARAM wParam, LPARAM lParam);
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
	afx_msg LRESULT OnEnUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeFilter();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedDiffoption();
	afx_msg void OnBnClickedLog();
	afx_msg LRESULT OnDisableButtons(WPARAM, LPARAM);
	afx_msg LRESULT OnDiffFinished(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	int					AddEntry(const CTGitPath * fd);
	void				DoDiff(int selIndex, bool blame);
	void				DiffProps(int selIndex);
	void				SetURLLabels(int mask=0x3);
	void				ClearURLabels(int mask);
	void				Filter(CString sFilterText);
	void				CopySelectionToClipboard(BOOL isFull=FALSE);

	void				ClickRevButton(CMenuButton *button,GitRev *rev, CACEdit *edit);

	void				EnableInputControl(bool b=true);

	int					RevertSelectedItemToVersion(CString rev);

	int					FillRevFromString(GitRev *rev, CString str)
	{
		GitRev gitrev;
		bool revfail = false;
		try
		{
			revfail = !!gitrev.GetCommit(str);//TODO
		}
		catch (const char *msg)
		{
			CMessageBox::Show(m_hWnd, _T("Could not get commit ") + str + _T("\nlibgit reports:\n") + CString(msg), _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}

		if (revfail)
		{
			CString msg;
			msg.Format(_T("Reference %s is wrong"), str);
			CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return -1;
		}
		*rev=gitrev;
		return 0;
	}

private:
	static UINT			WM_DISABLEBUTTONS;
	static UINT			WM_DIFFFINISHED;

	static UINT			DiffThreadEntry(LPVOID pVoid);
	UINT				DiffThread();

	static UINT			LoadRefThreadEntry(LPVOID pVoid)
	{
		return ((CFileDiffDlg *)pVoid)->LoadRefThread();
	};

	UINT				LoadRefThread();

	STRING_VECTOR		m_Reflist;

	virtual BOOL		Cancel() {return m_bCancelled;}
	virtual BOOL DestroyWindow();
	void OnTextUpdate(CACEdit *pEdit);

	CToolTips			m_tooltips;

	CMenuButton			m_cRev1Btn;
	CMenuButton			m_cRev2Btn;
	CFilterEdit			m_cFilter;

	CMFCButton			m_SwitchButton;
	CColors				m_colors;
	CHintListCtrl		m_cFileList;
	bool				m_bBlame;
	CTGitPathList		m_arFileList;
	std::vector<CTGitPath*> m_arFilteredList;

	CString				m_strExportDir;

	int					m_nIconFolder;

	bool				m_bIsBare;
	CTGitPath			m_path1;
	GitRev				m_peg;
	GitRev				m_rev1;
	CTGitPath			m_path2;
	GitRev				m_rev2;

	volatile LONG		m_bThreadRunning;

	volatile LONG		m_bLoadingRef;

	bool				m_bCancelled;

	void				Sort();
	static bool			SortCompare(const CTGitPath& Data1, const CTGitPath& Data2);

	static BOOL			m_bAscending;
	static int			m_nSortedColumn;

	CACEdit				m_ctrRev1Edit;
	CACEdit				m_ctrRev2Edit;

	CString				m_FileListText;

	bool				m_bIgnoreSpaceAtEol;
	bool				m_bIgnoreSpaceChange;
	bool				m_bIgnoreAllSpace;
	bool				m_bIgnoreBlankLines;
public:
	CString				m_strRev1;
	CString				m_strRev2;
	CString				m_sFilter;
};
