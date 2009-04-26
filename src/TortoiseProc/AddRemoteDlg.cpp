// AddRemoteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "AddRemoteDlg.h"


// CAddRemoteDlg dialog

IMPLEMENT_DYNAMIC(CAddRemoteDlg, CDialog)

CAddRemoteDlg::CAddRemoteDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddRemoteDlg::IDD, pParent)
	, m_Name(_T(""))
	, m_Url(_T(""))
{

}

CAddRemoteDlg::~CAddRemoteDlg()
{
}

void CAddRemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAME, m_Name);
	DDX_Text(pDX, IDC_EDIT_URL, m_Url);
}


BEGIN_MESSAGE_MAP(CAddRemoteDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CAddRemoteDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CAddRemoteDlg message handlers

void CAddRemoteDlg::OnBnClickedOk()
{
	UpdateData();



	OnOK();
}
