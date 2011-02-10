// ResetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ResetDlg.h"
#include "Git.h"


// CResetDlg dialog

IMPLEMENT_DYNAMIC(CResetDlg, CResizableStandAloneDialog)

CResetDlg::CResetDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CResetDlg::IDD, pParent)
	, m_ResetType(1)
{

}

CResetDlg::~CResetDlg()
{
}

void CResetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CResetDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CResetDlg message handlers
BOOL CResetDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CString resetTo;
	CString currentBranch = g_Git.GetCurrentBranch();
	resetTo.Format(IDS_PROC_RESETBRANCH, currentBranch, m_ResetToVersion);
	GetDlgItem(IDC_RESET_BRANCH_NAME)->SetWindowTextW(resetTo);

	AddAnchor(IDC_RESET_BRANCH_NAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_RESET_TYPE, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	this->CheckRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD,IDC_RADIO_RESET_SOFT+m_ResetType);

	return TRUE;
}

void CResetDlg::OnOK()
{
	m_ResetType=this->GetCheckedRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD)-IDC_RADIO_RESET_SOFT;
	return CResizableStandAloneDialog::OnOK();
}
