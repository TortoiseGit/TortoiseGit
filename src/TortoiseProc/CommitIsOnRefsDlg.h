// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseGit

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
#include "StandAloneDlg.h"
#include "registry.h"
#include "GitRev.h"
#include "ACEdit.h"
#include "MenuButton.h"
#include "FilterEdit.h"
#include "HintCtrl.h"
#include "GestureEnabledControl.h"

// CCommitIsOnRefsDlg dialog

#define IDT_FILTER		101
#define IDT_INPUT		102

class CCommitIsOnRefsDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCommitIsOnRefsDlg)

public:
	CCommitIsOnRefsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCommitIsOnRefsDlg();
	void Create(CWnd* pParent = nullptr) { m_bNonModalParentHWND = pParent->GetSafeHwnd(); CDialog::Create(IDD, pParent); ShowWindow(SW_SHOW); UpdateWindow(); }

// Dialog Data
	enum { IDD = IDD_COMMITISONREFS };

	enum eCmd
	{
		eCmd_ViewLog = WM_APP,
		eCmd_Diff,
		eCmd_RepoBrowser,
		eCmd_ViewLogRange,
		eCmd_ViewLogRangeReachableFromOnlyOne,
		eCmd_UnifiedDiff,
		eCmd_Copy,
		eCmd_DiffWC,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual void OnCancel() override;
	virtual BOOL OnInitDialog() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual void PostNcDestroy() override;
	afx_msg void OnEnChangeEditFilter();
	afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedSelRevBtn();
	afx_msg void OnBnClickedShowLog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnEnChangeCommit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnItemChangedListRefs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblClickListRefs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnGettingRefsFinished(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	STRING_VECTOR	m_RefList;

	static CString GetTwoSelectedRefs(const STRING_VECTOR& selectedRefs, const CString& lastSelected, const CString& separator);
	void AddToList();
	void CopySelectionToClipboard();
	int FillRevFromString(const CString& str)
	{
		GitRev gitrev;
		if (gitrev.GetCommit(str))
		{
			MessageBox(gitrev.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
		m_gitrev = gitrev;
		return 0;
	}
	GitRev m_gitrev;

public:
	CString				m_Rev;

private:
	HWND				m_bNonModalParentHWND;
	static UINT			WM_GETTINGREFSFINISHED;
	void				StartGetRefsThread();
	static UINT			GetRefsThreadEntry(LPVOID pVoid);
	UINT				GetRefsThread();

	volatile LONG		m_bThreadRunning;
	bool				m_bRefsLoaded;
	CString				m_sLastSelected;
	CGestureEnabledControlTmpl<CHintCtrl<CListCtrl>>	m_cRefList;
	CACEdit				m_cRevEdit;
	CMenuButton			m_cSelRevBtn;
	CFilterEdit			m_cFilter;
	bool				m_bHasWC;
};
