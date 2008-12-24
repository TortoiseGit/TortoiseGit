#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"

// CCherryPick dialog

class CCherryPick : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCherryPick)

public:
	CCherryPick(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCherryPick();

// Dialog Data
	enum { IDD = IDD_CHERRY_PICK };

	CString m_PickVersion;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	CHistoryCombo m_Version;

	DECLARE_MESSAGE_MAP()
};
