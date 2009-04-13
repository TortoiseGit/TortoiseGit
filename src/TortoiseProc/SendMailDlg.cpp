// SendMailDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SendMailDlg.h"


// CSendMailDlg dialog

IMPLEMENT_DYNAMIC(CSendMailDlg, CResizableStandAloneDialog)

CSendMailDlg::CSendMailDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSendMailDlg::IDD, pParent)
	, m_To(_T(""))
	, m_CC(_T(""))
	, m_Subject(_T(""))
	, m_bAttachment(FALSE)
	, m_bBranch(FALSE)
{

}

CSendMailDlg::~CSendMailDlg()
{
}

void CSendMailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SENDMAIL_TO, m_To);
	DDX_Text(pDX, IDC_SENDMAIL_CC, m_CC);
	DDX_Text(pDX, IDC_SENDMAIL_SUBJECT, m_Subject);
	DDX_Check(pDX, IDC_SENDMAIL_ATTACHMENT, m_bAttachment);
	DDX_Check(pDX, IDC_SENDMAIL_COMBINE, m_bBranch);
	DDX_Control(pDX, IDC_SENDMAIL_PATCHS, m_ctrlList);
	DDX_Control(pDX,IDC_SENDMAIL_SETUP, this->m_SmtpSetup);
}


BEGIN_MESSAGE_MAP(CSendMailDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SENDMAIL_COMBINE, &CSendMailDlg::OnBnClickedSendmailCombine)
END_MESSAGE_MAP()


// CSendMailDlg message handlers

BOOL CSendMailDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_SENDMAIL_GROUP,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_TO,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_CC,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SUBJECT,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SETUP,TOP_RIGHT);

	AddAnchor(IDC_SENDMAIL_PATCHS,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	this->AddOthersToAnchor();
	EnableSaveRestore(_T("SendMailDlg"));
	return TRUE;
}
void CSendMailDlg::OnBnClickedSendmailCombine()
{
	// TODO: Add your control notification handler code here
}
