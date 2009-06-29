// PullFetchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PullFetchDlg.h"
#include "Git.h"
#include "AppUtils.h"
#include "BrowseRefsDlg.h"
// CPullFetchDlg dialog

IMPLEMENT_DYNAMIC(CPullFetchDlg, CResizableStandAloneDialog)

CPullFetchDlg::CPullFetchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPullFetchDlg::IDD, pParent),
	  m_bRebase(false)
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
    DDX_Check(pDX,IDC_CHECK_REBASE,m_bRebase);

}


BEGIN_MESSAGE_MAP(CPullFetchDlg,CResizableStandAloneDialog )
	ON_BN_CLICKED(IDC_REMOTE_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_OTHER_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDOK, &CPullFetchDlg::OnBnClickedOk)
    ON_STN_CLICKED(IDC_REMOTE_MANAGE, &CPullFetchDlg::OnStnClickedRemoteManage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF, &CPullFetchDlg::OnBnClickedButtonBrowseRef)
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
	AddAnchor(IDC_CHECK_REBASE,BOTTOM_LEFT);
    AddAnchor(IDC_REMOTE_MANAGE,BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

    this->AddOthersToAnchor();

    this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(m_bAutoLoadEnable);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
	m_Remote.EnableWindow(TRUE);
	m_Other.EnableWindow(FALSE);
	if(!m_IsPull)
		m_RemoteBranch.EnableWindow(FALSE);

//	if(!m_IsPull)
		//Todo: implement rebase option sometime with rebase dialog
		GetDlgItem(IDC_CHECK_REBASE)->ShowWindow(SW_HIDE);

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

	//Select pull-remote from current branch
	CString currentBranch = g_Git.GetSymbolicRef();
	CString configName;
	configName.Format(L"branch.%s.remote", currentBranch);
	CString pullRemote = m_configPullRemote = g_Git.GetConfigValue(configName);

	//Select pull-branch from current branch
	configName.Format(L"branch.%s.merge", currentBranch);
	CString pullBranch = m_configPullBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));
	m_RemoteBranch.AddString(pullBranch);

	if(pullRemote.IsEmpty())
		pullRemote = remote;

	if(!g_Git.GetRemoteList(list))
	{	
		for(unsigned int i=0;i<list.size();i++)
		{
			m_Remote.AddString(list[i]);
			if(list[i] == pullRemote)
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
		if(!m_IsPull)
			m_RemoteBranch.EnableWindow(FALSE);
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Remote.EnableWindow(FALSE);
		m_Other.EnableWindow(TRUE);;
		if(!m_IsPull)
			m_RemoteBranch.EnableWindow(TRUE);
	}
	

}

void CPullFetchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_RemoteURL=m_Remote.GetString();
		if( !m_IsPull ||
			(m_configPullRemote == m_RemoteURL && m_configPullBranch == m_RemoteBranch.GetString() ))
			//When fetching or when pulling from the configured tracking branch, dont explicitly set the remote branch name,
			//because otherwise git will not update the remote tracking branches.
			m_RemoteBranchName.Empty();
		else
			m_RemoteBranchName=m_RemoteBranch.GetString();
		
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

void CPullFetchDlg::OnBnClickedButtonBrowseRef()
{
	CString initialRef;
	initialRef.Format(L"refs/remotes/%s/%s", m_Remote.GetString(), m_RemoteBranch.GetString());
	CString selectedRef = CBrowseRefsDlg::PickRef(false, initialRef, gPickRef_Remote);
	if(selectedRef.Left(13) != "refs/remotes/")
		return;

	selectedRef = selectedRef.Mid(13);
	int ixSlash = selectedRef.Find('/');

	CString remoteName   = selectedRef.Left(ixSlash);
	CString remoteBranch = selectedRef.Mid(ixSlash + 1);
	
	int ixFound = m_Remote.FindStringExact(0, remoteName);
	if(ixFound >= 0)
		m_Remote.SetCurSel(ixFound);
	m_RemoteBranch.AddString(remoteBranch);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
}
