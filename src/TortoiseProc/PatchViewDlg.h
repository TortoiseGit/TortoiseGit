#pragma once
#include "SciEdit.h"
#include "ProjectProperties.h"
// CPatchViewDlg dialog
class CCommitDlg;
class CPatchViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CPatchViewDlg)

public:
	CPatchViewDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPatchViewDlg();
	CCommitDlg	*m_ParentCommitDlg;

// Dialog Data
	enum { IDD = IDD_PATCH_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	CSciEdit			m_ctrlPatchView;
	ProjectProperties	*m_pProjectProperties;

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
};
