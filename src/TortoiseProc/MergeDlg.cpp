// MergeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MergeDlg.h"

#include "Git.h"
#include "Messagebox.h"
// CMergeDlg dialog

IMPLEMENT_DYNAMIC(CMergeDlg, CResizableStandAloneDialog)

CMergeDlg::CMergeDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CMergeDlg::IDD, pParent)
{

}

CMergeDlg::~CMergeDlg()
{
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH, this->m_Branch);
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS, this->m_Tags);
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION, this->m_Version);

	DDX_Check(pDX,IDC_CHECK_NOFF,this->m_bNoFF);
	DDX_Check(pDX,IDC_CHECK_SQUASH,this->m_bSquash);
}


BEGIN_MESSAGE_MAP(CMergeDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RADIO_BRANCH, &CMergeDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_TAGS, &CMergeDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_VERSION, &CMergeDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDOK, &CMergeDlg::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CMergeDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_BUTTON_SHOW, TOP_RIGHT);
	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	CheckRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION,IDC_RADIO_BRANCH);

	CStringList list;
	g_Git.GetTagList(list);
	m_Tags.AddString(list);

	list.RemoveAll();
	int current;
	g_Git.GetBranchList(list,&current,CGit::BRANCH_ALL);
	m_Branch.AddString(list);
	//m_Branch.SetCurSel(current);

	m_Version.LoadHistory(_T("Software\\TortoiseGit\\History\\VersionHash"), _T("hash"));
	m_Version.SetCurSel(0);

	OnBnClickedRadio();
			
	return TRUE;
}

// CMergeDlg message handlers

void CMergeDlg::OnBnClickedRadio()
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
}

void CMergeDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	
	int radio=GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
	switch (radio)
	{
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

	if(m_Base.Trim().IsEmpty())
	{
		CMessageBox::Show(NULL,_T("You must choose source"),_T("TortiseGit"),MB_OK);
		return;
	}
	this->m_Version.SaveHistory();
	OnOK();
}
