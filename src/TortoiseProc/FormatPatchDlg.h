#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "Registry.h"

// CFormatPatchDlg dialog

class CFormatPatchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFormatPatchDlg)

public:
	CFormatPatchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFormatPatchDlg();

// Dialog Data
	enum { IDD = IDD_FORMAT_PATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CHistoryCombo m_cDir;
	CHistoryCombo m_cSince;
	CHistoryCombo m_cFrom;
	CHistoryCombo m_cTo;
	CEdit		  m_cNum;
	CRegDWORD	m_regSendMail;

	DECLARE_MESSAGE_MAP()
public:
	int m_Num;
	CString m_Dir;
	CString m_From;
	CString m_To;
	CString m_Since;
	int m_Radio;
    BOOL m_bSendMail;

	afx_msg void OnBnClickedButtonDir();
	afx_msg void OnBnClickedButtonFrom();
	afx_msg void OnBnClickedButtonTo();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedButtonRef();
};
