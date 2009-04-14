// SettingSMTP.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingSMTP.h"


// CSettingSMTP dialog

IMPLEMENT_DYNAMIC(CSettingSMTP, CPropertyPage)

CSettingSMTP::CSettingSMTP()
	: CPropertyPage(CSettingSMTP::IDD)
	, m_Server(_T(""))
	, m_Port(0)
	, m_From(_T(""))
	, m_bAuth(FALSE)
	, m_User(_T(""))
	, m_Password(_T(""))
{

}

CSettingSMTP::~CSettingSMTP()
{
}

void CSettingSMTP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STMP_SERVER, m_Server);
	DDX_Text(pDX, IDC_SMTP_PORT, m_Port);
	DDX_Text(pDX, IDC_SEND_ADDRESS, m_From);
	DDX_Check(pDX, IDC_SMTP_AUTH, m_bAuth);
	DDX_Text(pDX, IDC_SMTP_USER, m_User);
	DDX_Text(pDX, IDC_SMTP_PASSWORD, m_Password);
}


BEGIN_MESSAGE_MAP(CSettingSMTP, CPropertyPage)
END_MESSAGE_MAP()


// CSettingSMTP message handlers
