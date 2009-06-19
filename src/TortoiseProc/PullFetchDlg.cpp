// PullFetchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PullFetchDlg.h"
#include "Git.h"
#include "AppUtils.h"

// CPullFetchDlg dialog

IMPLEMENT_DYNAMIC(CPullFetchDlg, CResizableStandAloneDialog)

CPullFetchDlg::CPullFetchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPullFetchDlg::IDD, pParent)
{
	m_IsPull=TRUE;
    m_bAutoLoad = CAppUtils::IsSSHPutty();
    m_bAutoLoadEnable=true;
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
    DDX_Control(pDX,IDC_REMOTE_MANAGE, this->m_RemoteManage);
    DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD,m_bAutoLoad);

}


BEGIN_MESSAGE_MAP(CPullFetchDlg,CResizableStandAloneDialog )
	ON_BN_CLICKED(IDC_REMOTE_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_OTHER_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDOK, &CPullFetchDlg::OnBnClickedOk)
    ON_STN_CLICKED(IDC_REMOTE_MANAGE, &CPullFetchDlg::OnStnClickedRemoteManage)
END_MESSAGE_MAP()

BOOL CPullFetchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_REMOTE_COMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_OTHER, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT,TOP_RIGHT);
	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
    AddAnchor(IDC_GROUPT_REMOTE,TOP_LEFT,BOTTOM_RIGHT);
    AddAnchor(IDC_PUTTYKEY_AUTOLOAD,BOTTOM_LEFT);
    AddAnchor(IDC_REMOTE_MANAGE,BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

    this->AddOthersToAnchor();

    this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(m_bAutoLoadEnable);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
	m_Remote.EnableWindow(TRUE);
	m_Other.EnableWindow(FALSE);
	m_RemoteBranch.EnableWindow(FALSE);

	m_Other.SetURLHistory(TRUE);
	m_Other.LoadHistory(_T("Software\\TortoiseGit\\History\\PullURLS"), _T("url"));
	CString clippath=CAppUtils::GetClipboardLink();
	if(clippath.IsEmpty())
		m_Other.SetCurSel(0);
	else
		m_Other.SetWindowText(clippath);

	m_RemoteBranch.LoadHistory(_T("Software\\TortoiseGit\\History\\PullRemoteBranch"), _T("br"));
	m_RemoteBranch.SetCurSel(0);

	CString WorkingDir=g_Git.m_CurrentDir;

	if(m_IsPull)
		this->SetWindowTextW(CString(_T("Pull - "))+WorkingDir);
	else
		this->SetWindowTextW(CString(_T("Fetch - "))+WorkingDir);

	STRING_VECTOR list;
	
	CRegString remote(CString(_T("Software\\TortoiseGit\\History\\PullRemote\\")+WorkingDir));
	m_RemoteReg = remote;
	int sel=0;

	if(!g_Git.GetRemoteList(list))
	{	
		for(unsigned int i=0;i<list.size();i++)
		{
			m_Remote.AddString(list[i]);
			if(list[i] == remote)
				sel = i;
		}
	}
	m_Remote.SetCurSel(sel);

	EnableSaveRestore(_T("PullFetchDlg"));
    this->m_RemoteManage.SetURL(CString());
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

	m_RemoteReg = m_Remote.GetString();

	m_Other.SaveHistory();
	m_RemoteBranch.SaveHistory();
	this->OnOK();
}

void CPullFetchDlg::OnStnClickedRemoteManage()
{
    // TODO: Add your control notification handler code here
    CAppUtils::LaunchRemoteSetting();
}
