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
	m_bSwitch = 0;	// default switch to checkbox not selected
	m_bTrack=0;
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
	DDX_Check(pDX,IDC_CHECK_SWITCH,this->m_bSwitch);

}


BEGIN_MESSAGE_MAP(CCreateBranchTagDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CCreateBranchTagDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch)
//	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF, &CCreateBranchTagDlg::OnBnClickedButtonBrowseRef)
END_MESSAGE_MAP()

BOOL CCreateBranchTagDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	if(m_Base.IsEmpty())
	{
		this->SetDefaultChoose(IDC_RADIO_HEAD);
	
	}else
	{
		this->SetDefaultChoose(IDC_RADIO_VERSION);
		this->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetWindowTextW(m_Base);
	}

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
	// show the switch checkbox if we are a create branch dialog
	this->GetDlgItem(IDC_CHECK_SWITCH)->ShowWindow( !m_bIsTag );
	CWnd* pHead = GetDlgItem(IDC_RADIO_HEAD);
	CString HeadText;
	pHead->GetWindowText( HeadText ); 
	pHead->SetWindowText( HeadText + " (" + g_Git.GetCurrentBranch() + ")");
	EnableSaveRestore(_T("BranchTagDlg"));

	OnCbnSelchangeComboboxexBranch();
	return TRUE;


}
// CCreateBranchTagDlg message handlers

void CCreateBranchTagDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);

	this->m_BranchTagName.Trim();
	if(this->m_BranchTagName.IsEmpty()  ||  this->m_BranchTagName.Find(' ') >= 0 )
	{
		CMessageBox::Show(NULL, IDS_B_T_NOTEMPTY, IDS_TORTOISEGIT, MB_OK);
		return;
	}
	this->UpdateRevsionName();
	OnOK();
}

void CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch()
{
	// TODO: Add your control notification handler code here
	
	if(this->m_ChooseVersioinBranch.GetString().Left(8)==_T("remotes/"))
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	if(this->m_bIsTag)
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}
