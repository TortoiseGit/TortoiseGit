#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"

// CGitSwitchDlg dialog

class CGitSwitchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CGitSwitchDlg)

public:
	CGitSwitchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGitSwitchDlg();

// Dialog Data
	enum { IDD = IDD_GITSWITCH };

	BOOL m_bForce;
	BOOL m_bTrack;
	BOOL m_bBranch;
	CString m_NewBranch;
	CString m_Base;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	void OnBnClickedRadio();
	void OnBnClickedOk();

	CHistoryCombo m_Branch;
	CHistoryCombo m_Tags;
	CHistoryCombo m_Version;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheckBranch();
	afx_msg void OnCbnSelchangeComboboxexBranch();
};
