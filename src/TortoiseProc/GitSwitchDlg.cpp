// GitSwitch.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitSwitchDlg.h"

#include "Git.h"
#include "Messagebox.h"

// CGitSwitchDlg dialog

IMPLEMENT_DYNAMIC(CGitSwitchDlg, CResizableStandAloneDialog)

CGitSwitchDlg::CGitSwitchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CGitSwitchDlg::IDD, pParent)
{
	m_bBranch=FALSE;
}

CGitSwitchDlg::~CGitSwitchDlg()
{
}

void CGitSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH, this->m_Branch);
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS, this->m_Tags);
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION, this->m_Version);
				
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);
	DDX_Check(pDX,IDC_CHECK_BRANCH,this->m_bBranch);

	DDX_Text(pDX,IDC_EDIT_BRANCH,this->m_NewBranch);
}


BEGIN_MESSAGE_MAP(CGitSwitchDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RADIO_BRANCH, &CGitSwitchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_TAGS, &CGitSwitchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_VERSION, &CGitSwitchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_CHECK_BRANCH, &CGitSwitchDlg::OnBnClickedCheckBranch)
	ON_BN_CLICKED(IDOK, &CGitSwitchDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CGitSwitchDlg::OnCbnSelchangeComboboxexBranch)
END_MESSAGE_MAP()

BOOL CGitSwitchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_EDIT_BRANCH, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	AddAnchor(IDC_BUTTON_SHOW,TOP_RIGHT);
	


	CheckRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION,IDC_RADIO_BRANCH);

	CStringList list;
	g_Git.GetTagList(list);
	m_Tags.AddString(list);

	list.RemoveAll();
	int current;
	g_Git.GetBranchList(list,&current,CGit::BRANCH_ALL);
	m_Branch.AddString(list);
	m_Branch.SetCurSel(current);

	m_Version.LoadHistory(_T("Software\\TortoiseGit\\History\\VersionHash"), _T("hash"));
	m_Version.SetCurSel(0);

	OnBnClickedRadio();
	OnBnClickedCheckBranch();
	this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	return TRUE;


}
// CCreateBranchTagDlg message handlers

void CGitSwitchDlg::OnBnClickedRadio()
{
	// TODO: Add your control notification handler code here
	this->m_Branch.EnableWindow(FALSE);
	this->m_Tags.EnableWindow(FALSE);
	this->m_Version.EnableWindow(FALSE);
	int radio=GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
	
	switch (radio)
	{
	case IDC_RADIO_BRANCH:
		this->m_Branch.EnableWindow(TRUE);
		break;
	case IDC_RADIO_TAGS:
		this->m_Tags.EnableWindow(TRUE);		
		break;
	case IDC_RADIO_VERSION:
		this->m_Version.EnableWindow(TRUE);
		break;
	}
	OnCbnSelchangeComboboxexBranch();
	OnBnClickedCheckBranch();
	

}

void CGitSwitchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	
	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	switch (radio)
	{
	case IDC_RADIO_BRANCH:
		this->m_Base=m_Branch.GetString();
		break;
	case IDC_RADIO_TAGS:
		this->m_Base=m_Tags.GetString();
		this->m_bTrack=FALSE;
		break;
	case IDC_RADIO_VERSION:
		this->m_Base=m_Version.GetString();
		this->m_bTrack=FALSE;
		break;
	}
	if(m_bBranch)
	{
		if(this->m_NewBranch.Trim().IsEmpty())
		{
			CMessageBox::Show(NULL,_T("Branch can't empty"),_T("TortoiseGit"),MB_OK);
			return;
		}
	}
	this->m_Version.SaveHistory();

	OnOK();
}
void CGitSwitchDlg::OnBnClickedCheckBranch()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	
	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	if(radio==IDC_RADIO_TAGS || radio==IDC_RADIO_VERSION)
	{
		this->m_bBranch=TRUE;
		this->UpdateData(FALSE);
		if(radio==IDC_RADIO_TAGS)
			GetDlgItem(IDC_EDIT_BRANCH)->SetWindowTextW(CString(_T("Branch_"))+m_Tags.GetString());
		if(radio==IDC_RADIO_VERSION)
			GetDlgItem(IDC_EDIT_BRANCH)->SetWindowTextW(CString(_T("Branch_"))+m_Version.GetString());

	}else
	{
		this->m_bBranch=FALSE;
		this->UpdateData(FALSE);
	}

	this->GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(this->m_bBranch);
}

void CGitSwitchDlg::OnCbnSelchangeComboboxexBranch()
{
	// TODO: Add your control notification handler code here
	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	if(this->m_Branch.GetString().Left(6)==_T("origin") && radio==IDC_RADIO_BRANCH )
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}
