// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// CloneDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "Git.h"
#include "CloneDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AppUtils.h"
// CCloneDlg dialog

IMPLEMENT_DYNCREATE(CCloneDlg, CHorizontalResizableStandAloneDialog)

CCloneDlg::CCloneDlg(CWnd* pParent /*=nullptr*/)
: CHorizontalResizableStandAloneDialog(CCloneDlg::IDD, pParent)
, m_bRecursive(BST_UNCHECKED)
, m_bBare(BST_UNCHECKED)
, m_bBranch(BST_UNCHECKED)
, m_bOrigin(BST_UNCHECKED)
, m_bNoCheckout(BST_UNCHECKED)
, m_bSVN(BST_UNCHECKED)
, m_bSVNTrunk(BST_UNCHECKED)
, m_bSVNTags(BST_UNCHECKED)
, m_bSVNBranch(BST_UNCHECKED)
, m_bSVNFrom(BST_UNCHECKED)
, m_bSVNUserName(BST_UNCHECKED)
, m_bExactPath(FALSE)
, m_strSVNTrunk(L"trunk")
, m_strSVNTags(L"tags")
, m_strSVNBranchs(L"branches")
, m_nDepth(1)
, m_bDepth(BST_UNCHECKED)
, m_bSaving(false)
, m_nSVNFrom(0)
, m_bUseLFS(FALSE)
, m_regBrowseUrl(L"Software\\TortoiseGit\\TortoiseProc\\CloneBrowse", 0)
, m_regCloneDir(L"Software\\TortoiseGit\\TortoiseProc\\CloneDir")
, m_regCloneRecursive(L"Software\\TortoiseGit\\TortoiseProc\\CloneRecursive", FALSE)
, m_regUseSSHKey(L"Software\\TortoiseGit\\TortoiseProc\\CloneUseSSHKey", TRUE)
{
	m_bAutoloadPuttyKeyFile = m_regUseSSHKey && CAppUtils::IsSSHPutty();
}

CCloneDlg::~CCloneDlg()
{
}

void CCloneDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
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
	DDX_Check(pDX,IDC_CHECK_USERNAME, m_bSVNUserName);

	DDX_Text(pDX, IDC_EDIT_SVN_TRUNK, m_strSVNTrunk);
	DDX_Text(pDX, IDC_EDIT_SVN_TAG, m_strSVNTags);
	DDX_Text(pDX, IDC_EDIT_SVN_BRANCH, m_strSVNBranchs);
	DDX_Text(pDX, IDC_EDIT_SVN_FROM, this->m_nSVNFrom);
	DDX_Text(pDX, IDC_EDIT_USERNAME,m_strUserName);

	DDX_Check(pDX, IDC_CHECK_DEPTH, m_bDepth);
	DDX_Check(pDX, IDC_CHECK_BARE, m_bBare);
	DDX_Check(pDX, IDC_CHECK_RECURSIVE, m_bRecursive);
	DDX_Text(pDX, IDC_EDIT_DEPTH,m_nDepth);
	DDX_Check(pDX,IDC_CHECK_BRANCH, m_bBranch);
	DDX_Text(pDX, IDC_EDIT_BRANCH, m_strBranch);
	DDX_Check(pDX, IDC_CHECK_ORIGIN, m_bOrigin);
	DDX_Text(pDX, IDC_EDIT_ORIGIN, m_strOrigin);
	DDX_Check(pDX,IDC_CHECK_NOCHECKOUT, m_bNoCheckout);
	DDX_Check(pDX, IDC_CHECK_LFS, m_bUseLFS);
}

BOOL CCloneDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_CHECK_DEPTH);
	AdjustControlSize(IDC_CHECK_RECURSIVE);
	AdjustControlSize(IDC_CHECK_BARE);
	AdjustControlSize(IDC_PUTTYKEY_AUTOLOAD);
	AdjustControlSize(IDC_CHECK_SVN);
	AdjustControlSize(IDC_CHECK_SVN_TRUNK);
	AdjustControlSize(IDC_CHECK_SVN_TAG);
	AdjustControlSize(IDC_CHECK_SVN_BRANCH);
	AdjustControlSize(IDC_CHECK_SVN_FROM);
	AdjustControlSize(IDC_CHECK_USERNAME);
	AdjustControlSize(IDC_CHECK_LFS);

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

	CString tt;
	tt.LoadString(IDS_CLONE_DEPTH_TT);
	m_tooltips.AddTool(IDC_EDIT_DEPTH,tt);
	m_tooltips.AddTool(IDC_CHECK_DEPTH,tt);
	m_tooltips.AddTool(IDC_CHECK_LFS, IDS_PROC_USELFS_TT);

	this->AddOthersToAnchor();

	if (m_Directory.IsEmpty())
	{
		CString dir = m_regCloneDir;
		int index = dir.ReverseFind('\\');
		if (index >= 0)
			dir = dir.Left(index);
		m_Directory = dir;
	}
	if (m_Directory.IsEmpty())
	{
		TCHAR szPath[MAX_PATH] = {0};
		if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, szPath)))
			m_Directory = szPath;
	}
	m_bRecursive = m_regCloneRecursive;
	UpdateData(FALSE);

	m_URLCombo.SetCaseSensitive(TRUE);
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(L"Software\\TortoiseGit\\History\\repoURLS", L"url");
	if(m_URL.IsEmpty())
	{
		CString str = CAppUtils::GetClipboardLink(L"git clone ");
		str.Trim();
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

	this->m_BrowseUrl.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_CLONE_DIR)));
	this->m_BrowseUrl.AddEntry(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_OPEN)));
	m_BrowseUrl.SetCurrentEntry(m_regBrowseUrl);

	m_PuttyKeyCombo.SetPathHistory(TRUE);
	m_PuttyKeyCombo.LoadHistory(L"Software\\TortoiseGit\\History\\puttykey", L"key");
	m_PuttyKeyCombo.SetCurSel(0);

	this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow( CAppUtils::IsSSHPutty() );
	this->GetDlgItem(IDC_PUTTYKEYFILE)->EnableWindow(m_bAutoloadPuttyKeyFile);
	this->GetDlgItem(IDC_PUTTYKEYFILE_BROWSE)->EnableWindow(m_bAutoloadPuttyKeyFile);

	EnableSaveRestore(L"CloneDlg");

	OnBnClickedCheckSvn();
	OnBnClickedCheckDepth();
	OnBnClickedCheckBranch();
	OnBnClickedCheckOrigin();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCloneDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_CLONE_BROWSE_URL, &CCloneDlg::OnBnClickedCloneBrowseUrl)
	ON_BN_CLICKED(IDC_CLONE_DIR_BROWSE, &CCloneDlg::OnBnClickedCloneDirBrowse)
	ON_BN_CLICKED(IDC_CHECK_BRANCH, &CCloneDlg::OnBnClickedCheckBranch)
	ON_BN_CLICKED(IDC_CHECK_ORIGIN, &CCloneDlg::OnBnClickedCheckOrigin)
	ON_BN_CLICKED(IDC_PUTTYKEYFILE_BROWSE, &CCloneDlg::OnBnClickedPuttykeyfileBrowse)
	ON_BN_CLICKED(IDC_PUTTYKEY_AUTOLOAD, &CCloneDlg::OnBnClickedPuttykeyAutoload)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCloneDlg::OnCbnEditchangeUrlcombo)
	ON_BN_CLICKED(IDC_CHECK_SVN, &CCloneDlg::OnBnClickedCheckSvn)
	ON_BN_CLICKED(IDC_CHECK_SVN_TRUNK, &CCloneDlg::OnBnClickedCheckSvnTrunk)
	ON_BN_CLICKED(IDC_CHECK_SVN_TAG, &CCloneDlg::OnBnClickedCheckSvnTag)
	ON_BN_CLICKED(IDC_CHECK_SVN_BRANCH, &CCloneDlg::OnBnClickedCheckSvnBranch)
	ON_BN_CLICKED(IDC_CHECK_SVN_FROM, &CCloneDlg::OnBnClickedCheckSvnFrom)
	ON_BN_CLICKED(IDC_CHECK_DEPTH, &CCloneDlg::OnBnClickedCheckDepth)
	ON_BN_CLICKED(IDC_CHECK_BARE, &CCloneDlg::OnBnClickedCheckBare)
	ON_BN_CLICKED(IDC_CHECK_RECURSIVE, &CCloneDlg::OnBnClickedCheckRecursive)
	ON_BN_CLICKED(IDC_CHECK_NOCHECKOUT, &CCloneDlg::OnBnClickedCheckRecursive)
	ON_BN_CLICKED(IDC_CHECK_USERNAME, &CCloneDlg::OnBnClickedCheckUsername)
END_MESSAGE_MAP()

// CCloneDlg message handlers

void CCloneDlg::OnOK()
{
	m_bSaving = true;
	this->m_URLCombo.GetWindowTextW(m_URL);
	m_URL.Trim();
	UpdateData(TRUE);
	if(m_URL.IsEmpty() || m_Directory.IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_CLONE_URLDIREMPTY, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		m_bSaving = false;
		return;
	}

	if (m_bBranch && !g_Git.IsBranchNameValid(m_strBranch))
	{
		ShowEditBalloon(IDC_EDIT_BRANCH, IDS_B_T_NOTEMPTY, IDS_ERR_ERROR, TTI_ERROR);
		m_bSaving = false;
		return;
	}

	if (m_bOrigin && m_strOrigin.IsEmpty() && !m_bSVN)
	{
		ShowEditBalloon(IDC_EDIT_ORIGIN, IDS_B_T_NOTEMPTY, IDS_ERR_ERROR, TTI_ERROR);
		m_bSaving = false;
		return;
	}

	m_URLCombo.SaveHistory();
	m_PuttyKeyCombo.SaveHistory();
	m_regCloneDir = m_Directory;
	m_regUseSSHKey = m_bAutoloadPuttyKeyFile;
	m_regCloneRecursive = m_bRecursive;

	this->m_PuttyKeyCombo.GetWindowText(m_strPuttyKeyFile);
	CResizableDialog::OnOK();
	m_bSaving = false;
}

void CCloneDlg::OnCancel()
{
	CResizableDialog::OnCancel();
}

void CCloneDlg::OnBnClickedCloneBrowseUrl()
{
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;

	int sel = static_cast<int>(this->m_BrowseUrl.GetCurrentEntry());
	this->m_regBrowseUrl = sel;

	if( sel == 1 )
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		str.Trim();
		if (str.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_CLONE_URLDIREMPTY, IDS_APPNAME, MB_ICONEXCLAMATION);
			return;
		}
		if (CAppUtils::ExploreTo(GetSafeHwnd(), str) && reinterpret_cast<INT_PTR>(ShellExecute(nullptr, L"open", str, nullptr, nullptr, SW_SHOW)) <= 32)
			MessageBox(CFormatMessageWrapper(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	this->m_URLCombo.GetWindowTextW(strCloneDirectory);
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK)
	{
		this->m_URLCombo.SetWindowTextW(strCloneDirectory);
	}
}

void CCloneDlg::OnBnClickedCloneDirBrowse()
{
	UpdateData(TRUE);
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory = this->m_Directory;
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK)
	{
		m_Directory = strCloneDirectory;
		UpdateData(FALSE);
	}
}

void CCloneDlg::OnBnClickedPuttykeyfileBrowse()
{
	UpdateData();
	CString filename;
	m_PuttyKeyCombo.GetWindowText(filename);
	if (!PathFileExists(filename))
		filename.Empty();
	if (!CAppUtils::FileOpenSave(filename, nullptr, 0, IDS_PUTTYKEYFILEFILTER, true, GetSafeHwnd()))
		return;

	m_PuttyKeyCombo.SetWindowText(filename);
}

void CCloneDlg::OnBnClickedPuttykeyAutoload()
{
	this->UpdateData();
	this->GetDlgItem(IDC_PUTTYKEYFILE)->EnableWindow(m_bAutoloadPuttyKeyFile);
	this->GetDlgItem(IDC_PUTTYKEYFILE_BROWSE)->EnableWindow(m_bAutoloadPuttyKeyFile);
}

void CCloneDlg::OnCbnEditchangeUrlcombo()
{
	// do not update member variables from UI while saving
	if (m_bSaving || m_bExactPath)
		return;

	this->UpdateData();
	CString url;
	m_URLCombo.GetWindowText(url);
	url.Trim();

	if(m_OldURL == url )
		return;

	m_OldURL=url;

	//if(url.IsEmpty())
	//	return;

	CString old;
	old=m_ModuleName;

	url.Replace(L'\\', L'/');

	// add compatibility for Google Code git urls
	url.TrimRight(L"/");

	int start = url.ReverseFind(L'/');
	if(start<0)
	{
		start = url.ReverseFind(L':');
		if(start <0)
			start = url.ReverseFind(L'@');

		if(start<0)
			start = 0;
	}
	CString temp;
	temp=url.Mid(start+1);

	temp=temp.MakeLower();

	// we've to check whether the URL ends with .git (instead of using the first .git)
	int end = CStringUtils::EndsWith(temp, L".git") ? (temp.GetLength() - 4) : temp.GetLength();

	//CString modulename;
	m_ModuleName=url.Mid(start+1,end);

	start = m_Directory.ReverseFind(L'\\');
	if(start <0 )
		start = m_Directory.ReverseFind(L'/');
	if(start <0 )
		start =0;

	int dirstart=m_Directory.Find(old,start);
	if(dirstart>=0 && (dirstart+old.GetLength() == m_Directory.GetLength()) )
		m_Directory=m_Directory.Left(dirstart);

	m_Directory.TrimRight(L"\\/");
	m_Directory += L'\\';
	m_Directory += m_ModuleName;

	// check if URL starts with http://, https:// or git:// in those cases loading putty keys is only
	// asking for passwords for keys that are never used
	if (url.Find(L"http://", 0) >= 0 || url.Find(L"https://", 0) >= 0 || url.Find(L"git://", 0) >= 0)
		m_bAutoloadPuttyKeyFile = false;
	else
		m_bAutoloadPuttyKeyFile = m_regUseSSHKey && CAppUtils::IsSSHPutty();

	this->UpdateData(FALSE);
}

void CCloneDlg::OnBnClickedCheckSvn()
{
	this->UpdateData();

	if(this->m_bSVN)
	{
		CString str;
		m_URLCombo.GetWindowText(str);

		str.TrimRight(L"\\/");
		if (CStringUtils::EndsWithI(str, L"trunk"))
			this->m_bSVNBranch=this->m_bSVNTags=this->m_bSVNTrunk = FALSE;
		else
			this->m_bSVNBranch=this->m_bSVNTags=this->m_bSVNTrunk = TRUE;
		m_bDepth = false;
		m_bBare = false;
		m_bRecursive = false;
		m_bBranch = FALSE;
		m_bNoCheckout = FALSE;
		m_bUseLFS = FALSE;
		this->UpdateData(FALSE);
		OnBnClickedCheckDepth();
	}
	this->GetDlgItem(IDC_CHECK_DEPTH)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_CHECK_BARE)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_CHECK_RECURSIVE)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_CHECK_BRANCH)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_CHECK_NOCHECKOUT)->EnableWindow(!m_bSVN);
	this->GetDlgItem(IDC_CHECK_LFS)->EnableWindow(!m_bSVN);
	OnBnClickedCheckSvnTrunk();
	OnBnClickedCheckSvnTag();
	OnBnClickedCheckSvnBranch();
	OnBnClickedCheckSvnFrom();
	OnBnClickedCheckUsername();
}

void CCloneDlg::OnBnClickedCheckSvnTrunk()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_TRUNK)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_TRUNK)->EnableWindow(this->m_bSVNTrunk&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnTag()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_TAG)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_TAG)->EnableWindow(this->m_bSVNTags&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnBranch()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_BRANCH)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_BRANCH)->EnableWindow(this->m_bSVNBranch&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckSvnFrom()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_SVN_FROM)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_SVN_FROM)->EnableWindow(this->m_bSVNFrom&&this->m_bSVN);
}

void CCloneDlg::OnBnClickedCheckRecursive()
{
	UpdateData(TRUE);
	if (m_bRecursive || m_bNoCheckout)
	{
		m_bBare = FALSE;
		GetDlgItem(IDC_CHECK_BARE)->EnableWindow(FALSE);
		UpdateData(FALSE);
	}
	else
		GetDlgItem(IDC_CHECK_BARE)->EnableWindow(TRUE);
}

void CCloneDlg::OnBnClickedCheckBare()
{
	UpdateData(TRUE);
	if (m_bBare)
	{
		m_bRecursive = FALSE;
		m_bNoCheckout = FALSE;
		UpdateData(FALSE);
	}
	GetDlgItem(IDC_CHECK_RECURSIVE)->EnableWindow(!m_bBare);
	GetDlgItem(IDC_CHECK_NOCHECKOUT)->EnableWindow(!m_bBare);
	GetDlgItem(IDC_CHECK_ORIGIN)->EnableWindow(!m_bBare);
	GetDlgItem(IDC_EDIT_ORIGIN)->EnableWindow(!m_bBare);
}
void CCloneDlg::OnBnClickedCheckDepth()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_EDIT_DEPTH)->EnableWindow(this->m_bDepth);
}

void CCloneDlg::OnBnClickedCheckBranch()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(this->m_bBranch);
}

void CCloneDlg::OnBnClickedCheckOrigin()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_EDIT_ORIGIN)->EnableWindow(this->m_bOrigin);
	GetDlgItem(IDC_CHECK_BARE)->EnableWindow(!m_bOrigin);
}

void CCloneDlg::OnBnClickedCheckUsername()
{
	UpdateData(TRUE);
	this->GetDlgItem(IDC_CHECK_USERNAME)->EnableWindow(this->m_bSVN);
	this->GetDlgItem(IDC_EDIT_USERNAME)->EnableWindow(this->m_bSVNUserName && this->m_bSVN);
}
