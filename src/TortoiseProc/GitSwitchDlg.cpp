// GitSwitch.cpp : implementation file
//

#include "stdafx.h"
#include "Git.h"
#include "TortoiseProc.h"
#include "GitSwitchDlg.h"


#include "Messagebox.h"

// CGitSwitchDlg dialog

IMPLEMENT_DYNAMIC(CGitSwitchDlg, CResizableStandAloneDialog)

CGitSwitchDlg::CGitSwitchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CGitSwitchDlg::IDD, pParent)
	,CChooseVersion(this)
{
	m_bBranch=FALSE;
}

CGitSwitchDlg::~CGitSwitchDlg()
{
}

void CGitSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CHOOSE_VERSION_DDX;
				
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);
	DDX_Check(pDX,IDC_CHECK_BRANCH,this->m_bBranch);

	DDX_Text(pDX,IDC_EDIT_BRANCH,this->m_NewBranch);
}


BEGIN_MESSAGE_MAP(CGitSwitchDlg, CResizableStandAloneDialog)

	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDC_CHECK_BRANCH, &CGitSwitchDlg::OnBnClickedCheckBranch)
	ON_BN_CLICKED(IDOK, &CGitSwitchDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CGitSwitchDlg::OnCbnSelchangeComboboxexBranch)
END_MESSAGE_MAP()

BOOL CGitSwitchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_EDIT_BRANCH, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	CHOOSE_VERSION_ADDANCHOR;
	Init();

	if(m_Base.IsEmpty())
		SetDefaultChoose(IDC_RADIO_BRANCH);
	else
	{
		this->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetWindowTextW(m_Base);
		SetDefaultChoose(IDC_RADIO_VERSION);
	}

	OnBnClickedCheckBranch();
	this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	return TRUE;


}
// CCreateBranchTagDlg message handlers

void CGitSwitchDlg::OnBnClickedChooseRadioHost()
{
	// TODO: Add your control notification handler code here
	OnBnClickedChooseRadio();
	OnCbnSelchangeComboboxexBranch();
	OnBnClickedCheckBranch();
	
}

void CGitSwitchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	
	UpdateRevsionName();
	//this->m_Version.SaveHistory();

	OnOK();
}
void CGitSwitchDlg::OnBnClickedCheckBranch()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	this->UpdateRevsionName();
	GetDlgItem(IDC_EDIT_BRANCH)->SetWindowTextW(CString(_T("Branch_"))+this->m_VersionName);

#if 0	
	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	if(radio==IDC_RADIO_TAGS || radio==IDC_RADIO_VERSION)
	{
		this->m_bBranch=TRUE;
		this->UpdateData(FALSE);
		this->UpdateRevsionName();
		
	}else
	{
		this->m_bBranch=FALSE;
		this->UpdateData(FALSE);
	}
#endif
	this->GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(this->m_bBranch);
}

void CGitSwitchDlg::OnCbnSelchangeComboboxexBranch()
{
	// TODO: Add your control notification handler code here
	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	if(this->m_ChooseVersioinBranch.GetString().Left(6)==_T("origin") && radio==IDC_RADIO_BRANCH )
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}
