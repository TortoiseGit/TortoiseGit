// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
// GitSwitch.cpp : implementation file
//

#include "stdafx.h"
#include "Git.h"
#include "TortoiseProc.h"
#include "GitSwitchDlg.h"
#include "AppUtils.h"

#include "Messagebox.h"

// CGitSwitchDlg dialog

IMPLEMENT_DYNAMIC(CGitSwitchDlg, CHorizontalResizableStandAloneDialog)

CGitSwitchDlg::CGitSwitchDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CGitSwitchDlg::IDD, pParent)
	,CChooseVersion(this)
{
	m_bBranch=FALSE;
	m_bTrack=FALSE;
	m_bForce=FALSE;
}

CGitSwitchDlg::~CGitSwitchDlg()
{
}

void CGitSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CHOOSE_VERSION_DDX;
				
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);
	DDX_Check(pDX,IDC_CHECK_BRANCH,this->m_bBranch);
	DDX_Check(pDX,IDC_CHECK_BRANCHOVERRIDE,this->m_bBranchOverride);

	DDX_Text(pDX,IDC_EDIT_BRANCH,this->m_NewBranch);
}


BEGIN_MESSAGE_MAP(CGitSwitchDlg, CHorizontalResizableStandAloneDialog)

	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDC_CHECK_BRANCH, &CGitSwitchDlg::OnBnClickedCheckBranch)
	ON_BN_CLICKED(IDOK, &CGitSwitchDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CGitSwitchDlg::OnCbnEditchangeComboboxexVersion)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_TAGS, &CGitSwitchDlg::OnCbnEditchangeComboboxexVersion)
	ON_WM_DESTROY()
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_VERSION, &CGitSwitchDlg::OnCbnEditchangeComboboxexVersion)
END_MESSAGE_MAP()

BOOL CGitSwitchDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_EDIT_BRANCH, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP,BOTTOM_RIGHT);

	CHOOSE_VERSION_ADDANCHOR;
	this->AddOthersToAnchor();

	EnableSaveRestore(_T("SwitchDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	Init();

	if(m_Base.IsEmpty())
		SetDefaultChoose(IDC_RADIO_BRANCH);
	else
	{
		this->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetWindowTextW(m_Base);
		SetDefaultChoose(IDC_RADIO_VERSION);
	}

	SetDefaultName(TRUE);
	this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	return TRUE;
}
// CCreateBranchTagDlg message handlers

void CGitSwitchDlg::OnBnClickedChooseRadioHost()
{
	OnBnClickedChooseRadio();
	SetDefaultName(TRUE);
}

void CGitSwitchDlg::OnBnClickedShow()
{
	OnBnClickedChooseVersion();
}

void CGitSwitchDlg::OnBnClickedOk()
{
	this->UpdateData(TRUE);

	// make sure a valid branch has been entered if a new branch is required
	m_NewBranch.Trim();
	if ( m_bBranch && (!g_Git.IsBranchNameValid(m_NewBranch)))
	{
		// new branch requested but name is empty or contains spaces
		CMessageBox::Show(NULL, IDS_B_T_NOTEMPTY, IDS_APPNAME, MB_OK);
	}
	else
	{
		UpdateRevsionName();
		//this->m_Version.SaveHistory();
		OnOK();
	}
}
void CGitSwitchDlg::SetDefaultName(BOOL isUpdateCreateBranch)
{
	this->UpdateData(TRUE);
	this->UpdateRevsionName();

	CString version = m_VersionName;
	
	if((version.Left(7)==_T("origin/") || version.Left(8)==_T("remotes/")))
	{
		int start=0;
		start = version.ReverseFind(_T('/'));
		if(start>=0)
			version = version.Mid(start+1);
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);
		this->m_NewBranch=version;
		
		if(isUpdateCreateBranch)
			this->m_bBranch=TRUE;

		this->m_bTrack=TRUE;
	}
	else
	{
		m_NewBranch = CString(_T("Branch_"))+this->m_VersionName;
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

		if(isUpdateCreateBranch)
			this->m_bBranch=FALSE;

		this->m_bTrack=FALSE;
	}

	int radio=GetCheckedRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION);
	if(radio==IDC_RADIO_TAGS || radio==IDC_RADIO_VERSION)
	{
		if(isUpdateCreateBranch)
			this->m_bBranch=TRUE;
	}

	GetDlgItem(IDC_EDIT_BRANCH)->EnableWindow(m_bBranch);
	GetDlgItem(IDC_CHECK_BRANCHOVERRIDE)->EnableWindow(m_bBranch);
	if (!m_bBranch)
	{
		this->m_bBranchOverride=FALSE;
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
		this->m_bTrack=FALSE;
	}
	this->UpdateData(FALSE);
}

void CGitSwitchDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}

void CGitSwitchDlg::OnCbnEditchangeComboboxexVersion()
{
	SetDefaultName(TRUE);
}

void CGitSwitchDlg::OnBnClickedCheckBranch()
{
	SetDefaultName(FALSE);
}
