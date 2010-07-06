// PushDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PushDlg.h"

#include "Git.h"
#include "registry.h"
#include "AppUtils.h"
#include "BrowseRefsDlg.h"

// CPushDlg dialog

IMPLEMENT_DYNAMIC(CPushDlg, CResizableStandAloneDialog)

CPushDlg::CPushDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPushDlg::IDD, pParent)
{
    m_bAutoLoad = CAppUtils::IsSSHPutty();
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
    DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD,this->m_bAutoLoad);

}


BEGIN_MESSAGE_MAP(CPushDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RD_REMOTE, &CPushDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_RD_URL, &CPushDlg::OnBnClickedRd)
	ON_CBN_SELCHANGE(IDC_BRANCH_SOURCE, &CPushDlg::OnCbnSelchangeBranchSource)
	ON_BN_CLICKED(IDOK, &CPushDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_REMOTE_MANAGE, &CPushDlg::OnBnClickedRemoteManage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SOURCE_BRANCH, &CPushDlg::OnBnClickedButtonBrowseSourceBranch)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DEST_BRANCH, &CPushDlg::OnBnClickedButtonBrowseDestBranch)
END_MESSAGE_MAP()

BOOL CPushDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_BRANCH_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_STATIC_REMOTE, TOP_RIGHT);
	AddAnchor(IDC_STATIC_SOURCE, TOP_LEFT);

	AddAnchor(IDC_BRANCH_REMOTE, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BROWSE_DEST_BRANCH, TOP_RIGHT);
	AddAnchor(IDC_BRANCH_SOURCE, TOP_LEFT);
	AddAnchor(IDC_BUTTON_BROWSE_SOURCE_BRANCH, TOP_LEFT);

	AddAnchor(IDC_URL_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_RD_REMOTE, TOP_LEFT);
	AddAnchor(IDC_RD_URL, TOP_LEFT);

	AddAnchor(IDC_REMOTE, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_URL, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_OPTION_GROUP, TOP_LEFT,TOP_RIGHT);
	
	AddAnchor(IDC_FORCE, TOP_LEFT);
	AddAnchor(IDC_PACK, TOP_LEFT);
	AddAnchor(IDC_TAGS, TOP_LEFT);
    AddAnchor(IDC_PUTTYKEY_AUTOLOAD,TOP_LEFT);

    AddAnchor(IDC_REMOTE_MANAGE,TOP_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();

    this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(m_bAutoLoad);

	EnableSaveRestore(_T("PushDlg"));

	m_RemoteURL.SetURLHistory(TRUE);
	
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));
	m_RemoteURL.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\PushURLS\\"))+WorkingDir, _T("url"));
	CString clippath=CAppUtils::GetClipboardLink();
	if(clippath.IsEmpty())
		m_RemoteURL.SetCurSel(0);
	else
		m_RemoteURL.SetWindowText(clippath);

	m_RemoteURL.EnableWindow(FALSE);
	CheckRadioButton(IDC_RD_REMOTE,IDC_RD_URL,IDC_RD_REMOTE);


	Refresh();


	//m_BranchRemote.SetWindowTextW(m_BranchSource.GetString());

	
	return TRUE;
}

void CPushDlg::Refresh()
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));

	CRegString remote(CString(_T("Software\\TortoiseGit\\History\\PushRemote\\")+WorkingDir));
	m_RemoteReg = remote;
	int sel=0;

	CString currentBranch = g_Git.GetSymbolicRef();
	CString configName;

	configName.Format(L"branch.%s.pushremote", currentBranch);
	CString pushRemote = g_Git.GetConfigValue(configName);
	if( pushRemote.IsEmpty() )
	{
		configName.Format(L"branch.%s.remote", currentBranch);
		pushRemote = g_Git.GetConfigValue(configName);
	}

	if( !pushRemote.IsEmpty() )
		remote=pushRemote;

	//Select pull-branch from current branch
	configName.Format(L"branch.%s.pushbranch", currentBranch);
	CString pushBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));
	if( pushBranch.IsEmpty() )
	{
		configName.Format(L"branch.%s.merge", currentBranch);
		pushBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));		
	}

	STRING_VECTOR list;
	m_Remote.Reset();

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

	int current=0;
	list.clear();
	m_BranchSource.Reset();
	m_BranchSource.SetMaxHistoryItems(0x7FFFFFFF);
	if(!g_Git.GetBranchList(list,&current))
	{
		for(unsigned int i=0;i<list.size();i++)
			m_BranchSource.AddString(list[i]);
	}
	m_BranchSource.SetCurSel(current);

	m_BranchRemote.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\RemoteBranch\\"))+WorkingDir, _T("branch"));
	if( !pushBranch.IsEmpty() )
	{
		m_BranchRemote.AddString(pushBranch);

	}
	else
		m_BranchRemote.SetCurSel(-1);

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
	m_BranchRemote.AddString(m_BranchSource.GetString());
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
	
	m_RemoteReg = m_Remote.GetString();

	CResizableStandAloneDialog::OnOK();
}

void CPushDlg::OnBnClickedRemoteManage()
{
    // TODO: Add your control notification handler code here
    CAppUtils::LaunchRemoteSetting();
}

void CPushDlg::OnBnClickedButtonBrowseSourceBranch()
{
	if(CBrowseRefsDlg::PickRefForCombo(&m_BranchSource, gPickRef_Head))
		OnCbnSelchangeBranchSource();
}

void CPushDlg::OnBnClickedButtonBrowseDestBranch()
{
	CString remoteBranchName;
	CString remoteName;
	m_BranchRemote.GetWindowText(remoteBranchName);
	remoteName = m_Remote.GetString();
	remoteBranchName = remoteName + '/' + remoteBranchName;
	remoteBranchName = CBrowseRefsDlg::PickRef(false, remoteBranchName, gPickRef_Remote);
	if(remoteBranchName.IsEmpty())
		return; //Canceled
	remoteBranchName = remoteBranchName.Mid(13);//Strip 'refs/remotes/'
	int slashPlace = remoteBranchName.Find('/');
	remoteName = remoteBranchName.Left(slashPlace);
	remoteBranchName = remoteBranchName.Mid(slashPlace + 1); //Strip remote name (for example 'origin/')

	//Select remote
	int remoteSel = m_Remote.FindStringExact(0,remoteName);
	if(remoteSel >= 0)
		m_Remote.SetCurSel(remoteSel);

	//Select branch
	m_BranchRemote.AddString(remoteBranchName);
}

BOOL CPushDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
