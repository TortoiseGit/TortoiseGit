#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "ChooseVersion.h"
// CMergeDlg dialog

class CMergeDlg : public CResizableStandAloneDialog,public CChooseVersion
{
	DECLARE_DYNAMIC(CMergeDlg)

public:
	CMergeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

	BOOL m_bSquash;
	BOOL m_bNoFF;

	//CString m_Base;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	CHOOSE_EVENT_RADIO() ;
public:

	afx_msg void OnBnClickedOk();
};
