#pragma once

#include "RepositoryInfo.h"

// CGoOffline dialog

class CGoOffline : public CDialog
{
	DECLARE_DYNAMIC(CGoOffline)

public:
	CGoOffline(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGoOffline();

// Dialog Data
	enum { IDD = IDD_GOOFFLINE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

    LogCache::CRepositoryInfo::ConnectionState selection;
    BOOL asDefault;

    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedPermanentlyOffline();
    afx_msg void OnBnClickedCancel();
};
