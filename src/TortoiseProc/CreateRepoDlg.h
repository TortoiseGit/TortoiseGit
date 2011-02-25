#pragma once

#include "StandAloneDlg.h"
//#include "HistoryCombo.h"
//#include "MenuButton.h"
#include "registry.h"
#include "tooltip.h"
// CCreateRepoDlg dialog

class CCreateRepoDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNCREATE(CCreateRepoDlg)

public:
	CCreateRepoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreateRepoDlg();

// Dialog Data
	enum { IDD = IDD_CREATEREPO};

protected:
	// Overrides
	virtual void OnOK();
	virtual void OnCancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	CString m_ModuleName;

	DECLARE_MESSAGE_MAP()

public:
	BOOL	m_bBare;

protected:
	afx_msg void OnBnClickedCheckBare();

	CToolTips	m_tooltips;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
