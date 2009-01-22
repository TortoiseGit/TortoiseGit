// PushDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PushDlg.h"

#include "Git.h"
// CPushDlg dialog

IMPLEMENT_DYNAMIC(CPushDlg, CResizableStandAloneDialog)

CPushDlg::CPushDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPushDlg::IDD, pParent)
{

}

CPushDlg::~CPushDlg()
{
}

void CPushDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BRANCH_REMOTE, this->m_BranchRemote);
	DDX_Control(pDX, IDC_BRANCH_SOURCE, this->m_BranchSource);
	DDX_Control(pDX, IDC_REMOTE, this->m_Remote);
	DDX_Control(pDX, IDC_URL, this->m_RemoteURL);
	DDX_Check(pDX,IDC_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_PACK,this->m_bPack);
	DDX_Check(pDX,IDC_TAGS,this->m_bTags);

}


BEGIN_MESSAGE_MAP(CPushDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RD_REMOTE, &CPushDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_RD_URL, &CPushDlg::OnBnClickedRd)
	ON_CBN_SELCHANGE(IDC_BRANCH_SOURCE, &CPushDlg::OnCbnSelchangeBranchSource)
	ON_BN_CLICKED(IDOK, &CPushDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CPushDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_BRANCH_REMOTE, TOP_RIGHT);
	AddAnchor(IDC_BRANCH_SOURCE, TOP_LEFT);

	AddAnchor(IDC_REMOTE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_URL, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_URL_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_OPTION_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_BRANCH_GROUP, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_STATIC_REMOTE, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	m_RemoteURL.SetURLHistory(TRUE);
	m_RemoteURL.LoadHistory(_T("Software\\TortoiseGit\\History\\PushURLS"), _T("url"));
	m_RemoteURL.SetCurSel(0);


	m_RemoteURL.EnableWindow(FALSE);
	CheckRadioButton(IDC_RD_REMOTE,IDC_RD_URL,IDC_RD_REMOTE);

	STRING_VECTOR list;

	if(!g_Git.GetRemoteList(list))
	{	
		for(unsigned int i=0;i<list.size();i++)
			m_Remote.AddString(list[i]);
	}

	int current=0;
	if(!g_Git.GetBranchList(list,&current))
	{
		for(unsigned int i=0;i<list.size();i++)
			m_BranchSource.AddString(list[i]);
	}
	m_BranchSource.SetCurSel(current);
	
	m_BranchRemote.LoadHistory(_T("Software\\TortoiseGit\\History\\RemoteBranch"), _T("branch"));
	m_BranchRemote.SetCurSel(0);

	//m_BranchRemote.SetWindowTextW(m_BranchSource.GetString());
	return TRUE;
}

// CPushDlg message handlers

void CPushDlg::OnBnClickedRd()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_REMOTE)
	{
		m_Remote.EnableWindow(TRUE);
		m_RemoteURL.EnableWindow(FALSE);;
	}
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL)
	{
		m_Remote.EnableWindow(FALSE);
		m_RemoteURL.EnableWindow(TRUE);
	}
}


void CPushDlg::OnCbnSelchangeBranchSource()
{
	// TODO: Add your control notification handler code here
	m_BranchRemote.SetWindowTextW(m_BranchSource.GetString());
}

void CPushDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CResizableStandAloneDialog::UpdateData(TRUE);

	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_REMOTE)
	{
		m_URL=m_Remote.GetString();
	}
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL)
	{
		m_URL=m_RemoteURL.GetString();
	}

	this->m_BranchRemoteName=m_BranchRemote.GetString();
	this->m_BranchSourceName=m_BranchSource.GetString();

	this->m_RemoteURL.SaveHistory();
	this->m_BranchRemote.SaveHistory();
	CResizableStandAloneDialog::OnOK();
}
