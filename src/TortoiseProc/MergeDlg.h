#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"

// CMergeDlg dialog

class CMergeDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CMergeDlg)

public:
	CMergeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

	BOOL m_bSquash;
	BOOL m_bNoFF;

	CString m_Base;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CHistoryCombo m_Branch;
	CHistoryCombo m_Tags;
	CHistoryCombo m_Version;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedOk();
};
