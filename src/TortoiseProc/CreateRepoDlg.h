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
// Overrides
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_CREATEREPO};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	CString m_ModuleName;
	//CString m_OldURL;

	DECLARE_MESSAGE_MAP()

public:

	BOOL	m_bBare;

	afx_msg void OnBnClickedCheckBare();

	CToolTips			m_tooltips;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
