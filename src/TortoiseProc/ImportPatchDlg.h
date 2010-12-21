#pragma once

#include "StandAloneDlg.h"
// CImportPatchDlg dialog
#include "TGitPath.h"
#include "PatchListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"
#include "Balloon.h"
#include "GitLogList.h"
#include "MenuButton.h"

#define IDC_AM_TAB 0x1000100

class CImportPatchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CImportPatchDlg)

public:
	CImportPatchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CImportPatchDlg();

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

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeListPatch();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedOk();

	CTGitPathList m_PathList;
	afx_msg void OnStnClickedAmSplit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};
