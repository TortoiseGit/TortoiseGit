// RefLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "RefLogDlg.h"


// CRefLogDlg dialog

IMPLEMENT_DYNAMIC(CRefLogDlg, CDialog)

CRefLogDlg::CRefLogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRefLogDlg::IDD, pParent)
{

}

CRefLogDlg::~CRefLogDlg()
{
}

void CRefLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_REF, m_ChooseRef);
	DDX_Control(pDX, IDC_REFLOG_LIST, m_RefList);
}


BEGIN_MESSAGE_MAP(CRefLogDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CRefLogDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CRefLogDlg message handlers

void CRefLogDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}
