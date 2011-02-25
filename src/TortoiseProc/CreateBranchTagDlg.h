#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "ChooseVersion.h"
// CCreateBranchTagDlg dialog

class CCreateBranchTagDlg : public CResizableStandAloneDialog,public CChooseVersion
{
	DECLARE_DYNAMIC(CCreateBranchTagDlg)

public:
	CCreateBranchTagDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreateBranchTagDlg();

// Dialog Data
	enum { IDD = IDD_NEW_BRANCH_TAG };

	BOOL	m_bForce;
	BOOL	m_bTrack;
	BOOL	m_bIsTag;
	BOOL	m_bSwitch;

	CString	m_Base;
	CString	m_BranchTagName;
	CString	m_Message;
	CString	m_OldSelectBranch;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CToolTipCtrl m_ToolTip;

	CHOOSE_EVENT_RADIO();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeComboboxexBranch();

	virtual void OnVersionChanged();
	afx_msg void OnDestroy();
};
