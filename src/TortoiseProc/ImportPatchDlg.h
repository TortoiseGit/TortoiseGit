// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
// CImportPatchDlg dialog
#include "TGitPath.h"
#include "PatchListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"
#include "GitLogList.h"
#include "MenuButton.h"
#include "Win7.h"

#define MSG_REBASE_UPDATE_UI	(WM_USER+151)

#define IDC_AM_TAB 0x1000100

class CImportPatchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CImportPatchDlg)

public:
	CImportPatchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CImportPatchDlg();

	int m_CurrentItem;

	volatile LONG		m_bExitThread;
	volatile LONG 		m_bThreadRunning;
	
	CWinThread*			m_LoadingThread;

	static UINT ThreadEntry(LPVOID pVoid)
	{
		return ((CImportPatchDlg*)pVoid)->PatchThread();
	}

	UINT PatchThread();

	void SafeTerminateThread()
	{
		if(m_LoadingThread!=NULL)
		{
			InterlockedExchange(&m_bExitThread,TRUE);
			DWORD ret =::WaitForSingleObject(m_LoadingThread->m_hThread,20000);
			if(ret == WAIT_TIMEOUT)
				::TerminateThread(m_LoadingThread,0);
			m_LoadingThread = NULL;
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	};

	void AddLogString(CString str);

// Dialog Data
	enum { IDD = IDD_APPLY_PATCH_LIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	CMFCTabCtrl m_ctrlTabCtrl;

	CSciEdit	m_PatchCtrl;
	CSciEdit	m_wndOutput;
	CToolTips	m_tooltips;

	void AddAmAnchor();
	void SetSplitterRange();

	void DoSize(int delta);
	void SaveSplitterPos();

	CPatchListCtrl m_cList;
	CRect				m_DlgOrigRect;
	CRect				m_PatchListOrigRect;

	CSplitterControl	m_wndSplitter;

	BOOL m_b3Way;
	BOOL m_bIgnoreSpace;
	BOOL m_bAddSignedOffBy;
	BOOL m_bKeepCR;

	BOOL IsFinish()
	{
		return !(m_CurrentItem < this->m_cList.GetItemCount());
	}

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeListPatch();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedOk();

	void EnableInputCtrl(BOOL b);
	void UpdateOkCancelText();

	CTGitPathList m_PathList;
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;
public:
	afx_msg void OnBnClickedCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnHdnItemclickListPatch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemchangingListPatch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemchangedListPatch(NMHDR *pNMHDR, LRESULT *pResult);
};
