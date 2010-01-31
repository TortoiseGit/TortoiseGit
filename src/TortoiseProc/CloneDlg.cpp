// CloneDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CloneDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AppUtils.h"
// CCloneDlg dialog

IMPLEMENT_DYNCREATE(CCloneDlg, CResizableStandAloneDialog)

CCloneDlg::CCloneDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCloneDlg::IDD, pParent)
	, m_Directory(_T(""))
{
    m_bAutoloadPuttyKeyFile = CAppUtils::IsSSHPutty();

	m_bSVN = FALSE;
	m_bSVNTrunk = FALSE;
	m_bSVNTags = FALSE;
	m_bSVNBranch = FALSE;;
	m_strSVNTrunk = _T("trunk");
	m_strSVNTags = _T("tags");
	m_strSVNBranchs = _T("branches");

	m_regBrowseUrl = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\CloneBrowse"),0);
	m_nSVNFrom = 0;

	m_nDepth = 0;
	m_bDepth = false;
}

CCloneDlg::~CCloneDlg()
{
}

void CCloneDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Control(pDX, IDC_PUTTYKEYFILE, m_PuttyKeyCombo);
	DDX_Control(pDX, IDC_CLONE_BROWSE_URL, m_BrowseUrl);
	DDX_Text(pDX, IDC_CLONE_DIR, m_Directory);
    DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD, m_bAutoloadPuttyKeyFile);

	DDX_Check(pDX,IDC_CHECK_SVN, m_bSVN);
	DDX_Check(pDX,IDC_CHECK_SVN_TRUNK, m_bSVNTrunk);
	DDX_Check(pDX,IDC_CHECK_SVN_TAG, m_bSVNTags);
	DDX_Check(pDX,IDC_CHECK_SVN_BRANCH, m_bSVNBranch);
	DDX_Check(pDX,IDC_CHECK_SVN_FROM, m_bSVNFrom);

	DDX_Text(pDX, IDC_EDIT_SVN_TRUNK, m_strSVNTrunk);
	DDX_Text(pDX, IDC_EDIT_SVN_TAG, m_strSVNTags);
	DDX_Text(pDX, IDC_EDIT_SVN_BRANCH, m_strSVNBranchs);
	DDX_Text(pDX, IDC_EDIT_SVN_FROM, this->m_nSVNFrom);
	
	DDX_Check(pDX, IDC_CHECK_DEPTH, m_bDepth);
	DDX_Text(pDX, IDC_EDIT_DEPTH,m_nDepth);
}

BOOL CCloneDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CLONE_BROWSE_URL, TOP_RIGHT);
	AddAnchor(IDC_CLONE_DIR, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_CLONE_DIR_BROWSE, TOP_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

    AddAnchor(IDC_GROUP_CLONE,TOP_LEFT,TOP_RIGHT);
    AddAnchor(IDC_PUTTYKEYFILE_BROWSE,TOP_RIGHT);
    AddAnchor(IDC_PUTTYKEY_AUTOLOAD,TOP_LEFT);
    AddAnchor(IDC_PUTTYKEYFILE,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_CLONE_GROUP_SVN,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	m_tooltips.Create(this);
	CString tt;
	tt.LoadString(IDS_CLONE_DEPTH_TT);
	m_tooltips.AddTool(IDC_EDIT_DEPTH,tt);
	m_tooltips.AddTool(IDC_CHECK_DEPTH,tt);

	this->AddOthersToAnchor();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS"), _T("url"));
	if(m_URL.IsEmpty())
	{
		CString str=CAppUtils::GetClipboardLink();
		if(str.IsEmpty())
			m_URLCombo.SetCurSel(0);
		else
			m_URLCombo.SetWindowText(str);
	}
	else
		m_URLCombo.SetWindowText(m_URL);

	CWnd *window=this->GetDlgItem(IDC_CLONE_DIR);
	if(window)
		SHAutoComplete(window->m_hWnd, SHACF_FILESYSTEM);

	this->m_BrowseUrl.AddEntry(CString(_T("Dir...")));
	this->m_BrowseUrl.AddEntry(CString(_T("Web")));
	m_BrowseUrl.SetCurrentEntry(m_regBrowseUrl);

    m_PuttyKeyCombo.SetPathHistory(TRUE);
    m_PuttyKeyCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\puttykey"), _T("key"));
    m_PuttyKeyCombo.SetCurSel(0);

    this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow( CAppUtils::IsSSHPutty() );
    this->GetDlgItem(IDC_PUTTYKEYFILE)->EnableWindow(m_bAutoloadPuttyKeyFile);
    this->GetDlgItem(IDC_PUTTYKEYFILE_BROWSE)->EnableWindow(m_bAutoloadPuttyKeyFile);
       
    EnableSaveRestore(_T("CloneDlg"));
	
	OnBnClickedCheckSvn();
	OnBnClickedCheckDepth();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCloneDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_CLONE_BROWSE_URL, &CCloneDlg::OnBnClickedCloneBrowseUrl)
	ON_BN_CLICKED(IDC_CLONE_DIR_BROWSE, &CCloneDlg::OnBnClickedCloneDirBrowse)
    ON_BN_CLICKED(IDC_PUTTYKEYFILE_BROWSE, &CCloneDlg::OnBnClickedPuttykeyfileBrowse)
    ON_BN_CLICKED(IDC_PUTTYKEY_AUTOLOAD, &CCloneDlg::OnBnClickedPuttykeyAutoload)
	ON_CBN_SELCHANGE(IDC_URLCOMBO, &CCloneDlg::OnCbnSelchangeUrlcombo)
	ON_NOTIFY(CBEN_BEGINEDIT, IDC_URLCOMBO, &CCloneDlg::OnCbenBegineditUrlcombo)
	ON_NOTIFY(CBEN_ENDEDIT, IDC_URLCOMBO, &CCloneDlg::OnCbenEndeditUrlcombo)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCloneDlg::OnCbnEditchangeUrlcombo)
	ON_BN_CLICKED(IDC_CHECK_SVN, &CCloneDlg::OnBnClickedCheckSvn)
	ON_BN_CLICKED(IDC_CHECK_SVN_TRUNK, &CCloneDlg::OnBnClickedCheckSvnTrunk)
	ON_BN_CLICKED(IDC_CHECK_SVN_TAG, &CCloneDlg::OnBnClickedCheckSvnTag)
	ON_BN_CLICKED(IDC_CHECK_SVN_BRANCH, &CCloneDlg::OnBnClickedCheckSvnBranch)
	ON_BN_CLICKED(IDC_CHECK_SVN_FROM, &CCloneDlg::OnBnClickedCheckSvnFrom)
	ON_BN_CLICKED(IDC_CHECK_DEPTH, &CCloneDlg::OnBnClickedCheckDepth)
END_MESSAGE_MAP()



// CCloneDlg message handlers

void CCloneDlg::OnOK()
{
	this->m_URLCombo.GetWindowTextW(m_URL);
	m_URL.Trim();
	UpdateData(TRUE);
	if(m_URL.IsEmpty() || m_Directory.IsEmpty())
	{
		CMessageBox::Show(NULL, _T("URL or Directory can't be empty"), _T("TortoiseGit"), MB_OK);
		return;
	}

	m_URLCombo.SaveHistory();
    m_PuttyKeyCombo.SaveHistory();

    this->m_PuttyKeyCombo.GetWindowText(m_strPuttyKeyFile);
	CResizableDialog::OnOK();

}

void CCloneDlg::OnCancel()
{
	CResizableDialog::OnCancel();
}

void CCloneDlg::OnBnClickedCloneBrowseUrl()
{
	// TODO: Add your control notification handler code here
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;

	int sel = this->m_BrowseUrl.GetCurrentEntry();
	this->m_regBrowseUrl = sel;

	if( sel == 1 )
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		ShellExecute(NULL, _T("open"), str, NULL,NULL, SW_SHOW);
		return ;
	}

	this->m_URLCombo.GetWindowTextW(strCloneDirectory);
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK) 
	{
		this->m_URLCombo.SetWindowTextW(strCloneDirectory);
	}
}

void CCloneDlg::OnBnClickedCloneDirBrowse()
{
	// TODO: Add your control notification handler code here
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory = this->m_Directory;
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK) 
	{
		UpdateData(TRUE);
		m_Directory = strCloneDirectory;
		UpdateData(FALSE);
	}
}

void CCloneDlg::OnEnChangeCloneDir()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDHtmlDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CCloneDlg::OnBnClickedPuttykeyfileBrowse()
{
    // TODO: Add your control notification handler code here
    CFileDialog dlg(TRUE,NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					_T("Putty Private Key(*.ppk)|*.ppk|All Files(*.*)|*.*||"));
	
	this->UpdateData();
	if(dlg.DoModal()==IDOK)
	{
        this->m_PuttyKeyCombo.SetWindowText( dlg.GetPathName() );
	}

}

void CCloneDlg::OnBnClickedPuttykeyAutoload()
{
    // TODO: Add your control notification handler code here
    this->UpdateData();
    this->GetDlgItem(IDC_PUTTYKEYFILE)->EnableWindow(m_bAutoloadPuttyKeyFile);
    this->GetDlgItem(IDC_PUTTYKEYFILE_BROWSE)->EnableWindow(m_bAutoloadPuttyKeyFile);

}

void CCloneDlg::OnCbnSelchangeUrlcombo()
{
	// TODO: Add your control notification handler code here
}

void CCloneDlg::OnCbenBegineditUrlcombo(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CCloneDlg::OnCbenEndeditUrlcombo(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CCloneDlg::OnCbnEditchangeUrlcombo()
{
	// TODO: Add your control notification handler code here
	this->UpdateData();
	CString url;
	m_URLCombo.GetWindowText(url);

	if(m_OldURL == url )
		return;

	m_OldURL=url;

	//if(url.IsEmpty())
	//	return;

	CString old;
	old=m_ModuleName;

	url.Replace(_T('\\'),_T('/'));
	int start=url.ReverseFind(_T('/'));
	if(start<0)
	{
		start = url.ReverseFind(_T(':'));
		if(start <0)
			start = url.ReverseFind(_T('@'));

		if(start<0)
			start = 0;
	}
	CString temp;
	temp=url.Mid(start+1);
	
	temp=temp.MakeLower();

	int end;
	end=temp.Find(_T(".git"));
	if(end<0)
		end=temp.GetLength();

	//CString modulename;
	m_ModuleName=url.Mid(start+1,end);
	
	start = m_Directory.ReverseFind(_T('\\'));
	if(start <0 )
		start = m_Directory.ReverseFind(_T('/'));
	if(start <0 )
		start =0;

	int dirstart=m_Directory.Find(old,start);
	if(dirstart>=0 && (dirstart+old.GetLength() == m_Directory.GetLength()) )
	{
		m_Directory=m_Directory.Left(dirstart);
		m_Directory+=m_ModuleName;

	}else
	{
		if(m_Directory.GetLength()>0 && 
			(m_Directory[m_Directory.GetLength()-1] != _T('\\') ||
			m_Directory[m_Directory.GetLength()-1] != _T('/') ) )
		{
			m_Directory+=_T('\\');
		}
		m_Directory += m_ModuleName;
	}

	if(m_Directory.GetLength()>0)
	{
		if( m_Directory[m_Directory.GetLength()-1] == _T('\\') ||
			m_Directory[m_Directory.GetLength()-1] == _T('/')
		   )
		{
			m_Directory=m_Directory.Left(m_Directory.GetLength()-1);
		}

	}
	this->UpdateData(FALSE);
}

void CCloneDlg::OnBnClickedCheckSvn()
{
	this->UpdateData();

	if(this->m_bSVN)
	{
		CString str;
		m_URLCombo.GetWindowText(str);

		while(str.GetLength()>=1 && 
			str[str.GetLength()-1] == _T('\\') &&
			str[str.GetLength()-1] == _T('/'))
		{
			str=str.Left(str.GetLength()-1);
		}
		if(str.GetLength()>=5 && (str.Right(5).MakeLower() == _T("trunk") ))
		{
			this->m_bSVNBranch=this->m_bSVNTags=this->m_bSVNTrunk = FALSE;
			this->UpdateData(FALSE);
		}else
		{
			this->m_bSVNBranch=this->m_bSVNTags=this->m_bSVNTrunk = TRUE;
			this->UpdateData(FALSE);
		}

	}

	// TODO: Add your control notification handler code here
	OnBnClickedCheckSvnTrunk();
	OnBnClickedCheckSvnTag();
	OnBnClickedCheckSvnBranch();
	OnBnClickedCheckSvnFrom();
}

void CCloneDlg::OnBnClickedCheckSvnTrunk()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_TRUNK)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_TRUNK)->EnableWindow(this->m_bSVNTrunk&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnTag()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_TAG)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_TAG)->EnableWindow(this->m_bSVNTags&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnBranch()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_BRANCH)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_BRANCH)->EnableWindow(this->m_bSVNBranch&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnFrom()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_FROM)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_FROM)->EnableWindow(this->m_bSVNFrom&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckDepth()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_EDIT_DEPTH)->EnableWindow(this->m_bDepth);
}
BOOL CCloneDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	m_tooltips.RelayEvent(pMsg);

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
