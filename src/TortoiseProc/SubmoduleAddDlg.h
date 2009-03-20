#pragma once
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
// CSubmoduleAddDlg dialog

class CSubmoduleAddDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleAddDlg)

public:
	CSubmoduleAddDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSubmoduleAddDlg();
	BOOL OnInitDialog();
// Dialog Data
	enum { IDD = IDD_SUBMODULE_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnRepBrowse();
	void OnPathBrowse();
	void OnBranchCheck();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
public:
	CHistoryCombo m_Repository;
public:
	CHistoryCombo m_PathCtrl;
public:
	BOOL m_bBranch;
public:
	CString m_strBranch;
};
