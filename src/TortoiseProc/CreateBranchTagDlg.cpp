// CreateBranchTagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Git.h"
#include "TortoiseProc.h"
#include "CreateBranchTagDlg.h"

#include "Messagebox.h"

// CCreateBranchTagDlg dialog

IMPLEMENT_DYNAMIC(CCreateBranchTagDlg, CResizableStandAloneDialog)

CCreateBranchTagDlg::CCreateBranchTagDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreateBranchTagDlg::IDD, pParent),
	CChooseVersion(this)
{
	m_bIsTag=0;
}

CCreateBranchTagDlg::~CCreateBranchTagDlg()
{
}

void CCreateBranchTagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CHOOSE_VERSION_DDX;

	DDX_Text(pDX, IDC_BRANCH_TAG, this->m_BranchTagName);
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);

}


BEGIN_MESSAGE_MAP(CCreateBranchTagDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CCreateBranchTagDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch)
END_MESSAGE_MAP()

BOOL CCreateBranchTagDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	this->SetDefaultChoose(IDC_RADIO_HEAD);
	Init();

	this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	if(this->m_bIsTag)
	{
		this->SetWindowTextW(_T("Create Tag"));
		this->GetDlgItem(IDC_LABEL_BRANCH)->SetWindowTextW(_T("Tag"));
	}
	else
	{
		this->SetWindowTextW(_T("Create Branch"));
		this->GetDlgItem(IDC_LABEL_BRANCH)->SetWindowTextW(_T("Branch"));
	}
	
	return TRUE;


}
// CCreateBranchTagDlg message handlers

void CCreateBranchTagDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	if(this->m_BranchTagName.Trim().IsEmpty())
	{
		CMessageBox::Show(NULL,_T("Branch\\Tag name can't empty"),_T("TortiseGit"),MB_OK);
		return;
	}
	this->UpdateRevsionName();
	OnOK();
}

void CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch()
{
	// TODO: Add your control notification handler code here
	
	if(this->m_ChooseVersioinBranch.GetString().Left(6)==_T("origin"))
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	if(this->m_bIsTag)
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}
