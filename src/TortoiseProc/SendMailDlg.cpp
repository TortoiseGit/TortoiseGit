// SendMailDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SendMailDlg.h"
#include "MessageBox.h"
#include "commonresource.h"
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
	DDX_Control(pDX,IDC_SENDMAIL_TO,m_ctrlTO);
	DDX_Control(pDX,IDC_SENDMAIL_CC,m_ctrlCC);
}


BEGIN_MESSAGE_MAP(CSendMailDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SENDMAIL_COMBINE, &CSendMailDlg::OnBnClickedSendmailCombine)
	ON_BN_CLICKED(IDOK, &CSendMailDlg::OnBnClickedOk)
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

	m_ctrlCC.Init();
	m_ctrlTO.Init();

	m_ctrlCC.SetSeparator(_T(";"));
	m_ctrlTO.SetSeparator(_T(";"));

	m_AddressReg.SetMaxHistoryItems(0xFFFF);

	m_AddressReg.Load(_T("Software\\TortoiseGit\\TortoiseProc\\EmailAddress\\"),_T("email"));
	for(int i=0;i<m_AddressReg.GetCount();i++)
	{
		m_ctrlCC.AddSearchString(m_AddressReg.GetEntry(i));
		m_ctrlTO.AddSearchString(m_AddressReg.GetEntry(i));
	}

	m_ctrlList.SetExtendedStyle( m_ctrlList.GetExtendedStyle()| LVS_EX_CHECKBOXES );

	for(int i=0;i<m_PathList.GetCount();i++)
	{
		m_ctrlList.InsertItem(i,m_PathList[i].GetWinPathString());
		m_ctrlList.SetCheck(i,true);
	}
	
//	m_ctrlCC.AddSearchString(_T("Tortoisegit-dev@google.com"));
//	m_ctrlTO.AddSearchString(_T("Tortoisegit-dev@google.com"));

	return TRUE;
}
void CSendMailDlg::OnBnClickedSendmailCombine()
{
	// TODO: Add your control notification handler code here
}

void CSendMailDlg::OnBnClickedOk()
{
	
	this->UpdateData();
	if(this->m_To.IsEmpty() && this->m_CC.IsEmpty())
	{
		CMessageBox::Show(NULL,IDS_ERR_ADDRESS_NO_EMPTY,IDS_APPNAME,MB_OK|MB_ICONERROR);
		return;
	}
	int start =0;
	CString Address;
	while(start>=0)
	{
		Address=this->m_CC.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}
	start =0;
	while(start>=0)
	{
		Address=this->m_To.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}	
	
	OnOK();
	// TODO: Add your control notification handler code here
}
