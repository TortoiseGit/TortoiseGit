// DeleteConflictDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DeleteConflictDlg.h"
#include "AppUtils.h"

// CDeleteConflictDlg dialog

IMPLEMENT_DYNAMIC(CDeleteConflictDlg, CStandAloneDialog)

CDeleteConflictDlg::CDeleteConflictDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CDeleteConflictDlg::IDD, pParent)

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


BEGIN_MESSAGE_MAP(CDeleteConflictDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_DELETE, &CDeleteConflictDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_MODIFY, &CDeleteConflictDlg::OnBnClickedModify)
END_MESSAGE_MAP()


BOOL CDeleteConflictDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	if(this->m_bShowModifiedButton )
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(_T("Modified"));
	else
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(_T("Created"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, this->m_File, sWindowTitle);

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
