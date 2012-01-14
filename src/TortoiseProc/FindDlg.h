#pragma once

#include "HistoryCombo.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "LoglistCommonResource.h"
// CFindDlg dialog

#define IDT_FILTER		101

class CFindDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();
	void Create(CWnd * pParent = NULL) {m_pParent = pParent; CDialog::Create(IDD, pParent);ShowWindow(SW_SHOW);UpdateWindow();}

	bool IsTerminating() {return m_bTerminating;}
	bool FindNext() {return m_bFindNext;}
	bool MatchCase() {return !!m_bMatchCase;}
	bool WholeWord() {return !!m_bWholeWord;}
	bool IsRef()	{return !!m_bIsRef;}
	CString GetFindString() {return m_FindString;}

// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnEditchangeFindcombo();

	DECLARE_MESSAGE_MAP()

	UINT			m_FindMsg;
	bool			m_bTerminating;
	bool			m_bFindNext;
	BOOL			m_bMatchCase;
	BOOL			m_bLimitToDiffs;
	BOOL			m_bWholeWord;
	bool			m_bIsRef;
	CHistoryCombo	m_FindCombo;
	CString			m_FindString;
	CWnd			*m_pParent;
	STRING_VECTOR	m_RefList;

	void AddToList();

public:
	CListCtrl m_ctrlRefList;
	CEdit m_ctrlFilter;
	afx_msg void OnNMClickListRef(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEditFilter();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
