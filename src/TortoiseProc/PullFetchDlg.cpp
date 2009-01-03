// PullFetchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PullFetchDlg.h"
#include "Git.h"

// CPullFetchDlg dialog

IMPLEMENT_DYNAMIC(CPullFetchDlg, CResizableStandAloneDialog)

CPullFetchDlg::CPullFetchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPullFetchDlg::IDD, pParent)
{
	m_IsPull=TRUE;
}

CPullFetchDlg::~CPullFetchDlg()
{
}

void CPullFetchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_COMBO, this->m_Remote);
	DDX_Control(pDX, IDC_OTHER, this->m_Other);
	DDX_Control(pDX, IDC_REMOTE_BRANCH, this->m_RemoteBranch);

}


BEGIN_MESSAGE_MAP(CPullFetchDlg,CResizableStandAloneDialog )
	ON_BN_CLICKED(IDC_REMOTE_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_OTHER_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDOK, &CPullFetchDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CPullFetchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_REMOTE_COMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_OTHER, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT,TOP_RIGHT);
	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
	m_Remote.EnableWindow(TRUE);
	m_Other.EnableWindow(FALSE);
	m_RemoteBranch.EnableWindow(FALSE);

	m_Other.SetURLHistory(TRUE);
	m_Other.LoadHistory(_T("Software\\TortoiseGit\\History\\PullURLS"), _T("url"));
	m_Other.SetCurSel(0);

	m_RemoteBranch.LoadHistory(_T("Software\\TortoiseGit\\History\\PullRemoteBranch"), _T("br"));
	m_RemoteBranch.SetCurSel(0);

	if(m_IsPull)
		this->SetWindowTextW(_T("Pull"));
	else
		this->SetWindowTextW(_T("Fetch"));

	STRING_VECTOR list;

	if(!g_Git.GetRemoteList(list))
	{	
		for(int i=0;i<list.size();i++)
			m_Remote.AddString(list[i]);
	}

	return TRUE;
}
// CPullFetchDlg message handlers

void CPullFetchDlg::OnBnClickedRd()
{

	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_Remote.EnableWindow(TRUE);
		m_Other.EnableWindow(FALSE);
		m_RemoteBranch.EnableWindow(FALSE);

	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Remote.EnableWindow(FALSE);
		m_Other.EnableWindow(TRUE);;
		m_RemoteBranch.EnableWindow(TRUE);
	}
	

}

void CPullFetchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_RemoteURL=m_Remote.GetString();
		m_RemoteBranchName.Empty();
		
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Other.GetWindowTextW(m_RemoteURL);
		m_RemoteBranchName=m_RemoteBranch.GetString();
		
	}

	m_Other.SaveHistory();
	m_RemoteBranch.SaveHistory();
	this->OnOK();
}
