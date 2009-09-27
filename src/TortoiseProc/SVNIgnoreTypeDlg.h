#pragma once
#include "StandAloneDlg.h"

// CSVNIgnoreTypeDlg dialog

class CSVNIgnoreTypeDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSVNIgnoreTypeDlg)

public:
	CSVNIgnoreTypeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSVNIgnoreTypeDlg();

// Dialog Data
	enum { IDD = IDD_SVNIGNORE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_SVNIgnoreType;
	virtual BOOL OnInitDialog();
};
