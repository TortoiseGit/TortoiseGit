// DeleteConflictDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DeleteConflictDlg.h"


// CDeleteConflictDlg dialog

IMPLEMENT_DYNAMIC(CDeleteConflictDlg, CResizableStandAloneDialog)

CDeleteConflictDlg::CDeleteConflictDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CDeleteConflictDlg::IDD, pParent)

	, m_LocalStatus(_T(""))
	, m_RemoteStatus(_T(""))
{
	m_bIsDelete =FALSE;
}

CDeleteConflictDlg::~CDeleteConflictDlg()
{
}

void CDeleteConflictDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_LOCAL_STATUS, m_LocalStatus);
	DDX_Text(pDX, IDC_REMOTE_STATUS, m_RemoteStatus);
}


BEGIN_MESSAGE_MAP(CDeleteConflictDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_DELETE, &CDeleteConflictDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_MODIFY, &CDeleteConflictDlg::OnBnClickedModify)
END_MESSAGE_MAP()


BOOL CDeleteConflictDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_DEL_GROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_DELETE, BOTTOM_RIGHT);
	AddAnchor(IDC_MODIFY, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	if(this->m_bShowModifiedButton )
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(_T("Modified"));
	else
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(_T("Created"));

	CString title;
	this->GetWindowText(title);
	title +=_T(" - ") +this->m_File;
	this->SetWindowText(title);
	return TRUE;
}
// CDeleteConflictDlg message handlers

void CDeleteConflictDlg::OnBnClickedDelete()
{
	m_bIsDelete = TRUE;
	OnOK();
}

void CDeleteConflictDlg::OnBnClickedModify()
{
	m_bIsDelete = FALSE;
	OnOK();
}
