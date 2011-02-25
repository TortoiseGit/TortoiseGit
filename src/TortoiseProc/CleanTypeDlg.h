#pragma once


#include "StandAloneDlg.h"
#include "registry.h"

// CCleanTypeDlg dialog

class CCleanTypeDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCleanTypeDlg)

public:
	CCleanTypeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCleanTypeDlg();

// Dialog Data
	enum { IDD = IDD_CLEAN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	CRegDWORD m_regDir;
	CRegDWORD m_regType;

	DECLARE_MESSAGE_MAP()

	virtual	BOOL OnInitDialog();
	virtual	void OnOK();

public:
	BOOL	m_bDir;
	int		m_CleanType;
};
