// UserPassword.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "UserPassword.h"
#include "afxdialogex.h"


// CUserPassword dialog

IMPLEMENT_DYNAMIC(CUserPassword, CDialog)

CUserPassword::CUserPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CUserPassword::IDD, pParent)
	, m_UserName(_T(""))
	, m_Password(_T(""))
{

}

CUserPassword::~CUserPassword()
{
}

void CUserPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USER_NAME, m_UserName);
	DDX_Text(pDX, IDC_USER_PASSWORD, m_Password);
}


BEGIN_MESSAGE_MAP(CUserPassword, CDialog)
END_MESSAGE_MAP()


// CUserPassword message handlers


BOOL CUserPassword::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (!m_URL.IsEmpty())
	{
		CString title;
		this->GetWindowText(title);
		title += _T(" - ");
		title += m_URL;
		this->SetWindowText(title);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}
