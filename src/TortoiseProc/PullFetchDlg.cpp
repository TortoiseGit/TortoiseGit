// PullFetchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PullFetchDlg.h"
#include "Git.h"

// CPullFetchDlg dialog

IMPLEMENT_DYNAMIC(CPullFetchDlg, CResizableStandAloneDialog)

CPullFetchDlg::CPullFetchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPullFetchDlg::IDD, pParent)
{

}

CPullFetchDlg::~CPullFetchDlg()
{
}

void CPullFetchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_COMBO, this->m_Remote);
	DDX_Control(pDX, IDC_OTHER, this->m_Other);

}


BEGIN_MESSAGE_MAP(CPullFetchDlg,CResizableStandAloneDialog )
	ON_BN_CLICKED(IDC_REMOTE_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_OTHER_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDOK, &CPullFetchDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CPullFetchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_REMOTE_COMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_OTHER, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
	m_Remote.EnableWindow(TRUE);
	m_Other.EnableWindow(FALSE);

	m_Other.SetURLHistory(TRUE);
	m_Other.LoadHistory(_T("Software\\TortoiseGit\\History\\PullURLS"), _T("url"));
	m_Other.SetCurSel(0);

	

	return TRUE;
}
// CPullFetchDlg message handlers

void CPullFetchDlg::OnBnClickedRd()
{

	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_Remote.EnableWindow(TRUE);
		m_Other.EnableWindow(FALSE);;
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Remote.EnableWindow(FALSE);
		m_Other.EnableWindow(TRUE);;
	}
	

}

void CPullFetchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_RemoteURL=m_Remote.GetString();
		
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Other.GetWindowTextW(m_RemoteURL);
	}
	this->OnOK();
}
