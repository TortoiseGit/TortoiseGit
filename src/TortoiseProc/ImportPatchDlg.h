#pragma once

#include "StandAloneDlg.h"
// CImportPatchDlg dialog
#include "TGitPath.h"

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

	CListCtrl m_cList;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeListPatch();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedOk();

	CTGitPathList m_PathList;
};
