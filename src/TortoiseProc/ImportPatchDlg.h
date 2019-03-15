// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2015-2019 - TortoiseGit

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
#include "TGitPath.h"
#include "PatchListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"

#define MSG_REBASE_UPDATE_UI	(WM_USER+151)

#define IDC_AM_TAB 0x1000100

class CImportPatchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CImportPatchDlg)

public:
	CImportPatchDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CImportPatchDlg();

	// Dialog Data
	enum { IDD = IDD_APPLY_PATCH_LIST };

	CTGitPathList m_PathList;

protected:
	int m_CurrentItem;

	volatile LONG		m_bExitThread;
	volatile LONG 		m_bThreadRunning;

	CWinThread*			m_LoadingThread;

	static UINT ThreadEntry(LPVOID pVoid)
	{
		return static_cast<CImportPatchDlg*>(pVoid)->PatchThread();
	}

	UINT PatchThread();

	void SafeTerminateThread()
	{
		if (m_LoadingThread)
		{
			InterlockedExchange(&m_bExitThread,TRUE);
			DWORD ret =::WaitForSingleObject(m_LoadingThread->m_hThread,20000);
			if(ret == WAIT_TIMEOUT)
				::TerminateThread(m_LoadingThread,0);
			m_LoadingThread = nullptr;
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	};

	void AddLogString(const CString& str);

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;

	CMFCTabCtrl m_ctrlTabCtrl;

	CSciEdit	m_PatchCtrl;
	CSciEdit	m_wndOutput;

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

	afx_msg void OnLbnSelchangeListPatch();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedOk();
	afx_msg void OnSysColorChange();
	afx_msg LRESULT OnThemeChanged();

	void EnableInputCtrl(BOOL b);
	void UpdateOkCancelText();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;
	afx_msg void OnBnClickedCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnHdnItemchangedListPatch(NMHDR *pNMHDR, LRESULT *pResult);
};
