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
}

CCloneDlg::~CCloneDlg()
{
}

void CCloneDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Control(pDX, IDC_PUTTYKEYFILE, m_PuttyKeyCombo);
	DDX_Text(pDX, IDC_CLONE_DIR, m_Directory);
    DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD, m_bAutoloadPuttyKeyFile);

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

    AddAnchor(IDC_GROUP_CLONE,TOP_LEFT,BOTTOM_RIGHT);
    AddAnchor(IDC_PUTTYKEYFILE_BROWSE,BOTTOM_RIGHT);
    AddAnchor(IDC_PUTTYKEY_AUTOLOAD,BOTTOM_LEFT);
    AddAnchor(IDC_PUTTYKEYFILE,BOTTOM_LEFT,BOTTOM_RIGHT);

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

    m_PuttyKeyCombo.SetPathHistory(TRUE);
    m_PuttyKeyCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\puttykey"), _T("key"));
    m_PuttyKeyCombo.SetCurSel(0);

    this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow( CAppUtils::IsSSHPutty() );
    this->GetDlgItem(IDC_PUTTYKEYFILE)->EnableWindow(m_bAutoloadPuttyKeyFile);
    this->GetDlgItem(IDC_PUTTYKEYFILE_BROWSE)->EnableWindow(m_bAutoloadPuttyKeyFile);
       
    EnableSaveRestore(_T("CloneDlg"));
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
