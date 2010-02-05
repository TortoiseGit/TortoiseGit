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
	void				SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), int size=-1, const char *face=0);
public:
	CSciEdit			m_ctrlPatchView;

	DECLARE_MESSAGE_MAP()
public:
	ProjectProperties	*m_pProjectProperties;
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
};
