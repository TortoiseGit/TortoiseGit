#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"

// CCreateBranchTagDlg dialog

class CCreateBranchTagDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCreateBranchTagDlg)

public:
	CCreateBranchTagDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreateBranchTagDlg();

// Dialog Data
	enum { IDD = IDD_NEW_BRANCH_TAG };

	BOOL m_bForce;
	BOOL m_bTrack;
	BOOL m_bIsTag;

	CString m_Base;
	CString m_BranchTagName;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	CHistoryCombo m_Branch;
	CHistoryCombo m_Tags;
	CHistoryCombo m_Version;
	
	DECLARE_MESSAGE_MAP()
};
