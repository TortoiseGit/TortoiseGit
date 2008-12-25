// CreateBranchTagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CreateBranchTagDlg.h"
#include "Git.h"
#include "Messagebox.h"

// CCreateBranchTagDlg dialog

IMPLEMENT_DYNAMIC(CCreateBranchTagDlg, CResizableStandAloneDialog)

CCreateBranchTagDlg::CCreateBranchTagDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreateBranchTagDlg::IDD, pParent)
{
	m_bIsTag=0;
}

CCreateBranchTagDlg::~CCreateBranchTagDlg()
{
}

void CCreateBranchTagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH, this->m_Branch);
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS, this->m_Tags);
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION, this->m_Version);
				
	DDX_Text(pDX, IDC_BRANCH_TAG, this->m_BranchTagName);
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);

}


BEGIN_MESSAGE_MAP(CCreateBranchTagDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RADIO_HEAD, &CCreateBranchTagDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_BRANCH, &CCreateBranchTagDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_TAGS, &CCreateBranchTagDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_VERSION, &CCreateBranchTagDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDOK, &CCreateBranchTagDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch)
END_MESSAGE_MAP()

BOOL CCreateBranchTagDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	CheckRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION,IDC_RADIO_HEAD);

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

void CCreateBranchTagDlg::OnBnClickedRadio()
{
	// TODO: Add your control notification handler code here
	this->m_Branch.EnableWindow(FALSE);
	this->m_Tags.EnableWindow(FALSE);
	this->m_Version.EnableWindow(FALSE);
	int radio=GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
	switch (radio)
	{
	case IDC_RADIO_HEAD:
		break;
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
}

void CCreateBranchTagDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	if(this->m_BranchTagName.Trim().IsEmpty())
	{
		CMessageBox::Show(NULL,_T("Branch\Tag name can't empty"),_T("TortiseGit"),MB_OK);
		return;
	}
	int radio=GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
	switch (radio)
	{
	case IDC_RADIO_HEAD:
		this->m_Base=_T("HEAD");
		break;
	case IDC_RADIO_BRANCH:
		this->m_Base=m_Branch.GetString();
		break;
	case IDC_RADIO_TAGS:
		this->m_Base=m_Tags.GetString();
		break;
	case IDC_RADIO_VERSION:
		this->m_Base=m_Version.GetString();
		break;
	}
	this->m_Version.SaveHistory();
	OnOK();
}

void CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch()
{
	// TODO: Add your control notification handler code here
	
	if(this->m_Branch.GetString().Left(6)==_T("origin"))
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	if(this->m_bIsTag)
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}
