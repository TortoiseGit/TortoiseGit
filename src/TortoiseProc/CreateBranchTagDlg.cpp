// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
// CreateBranchTagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Git.h"
#include "TortoiseProc.h"
#include "CreateBranchTagDlg.h"
#include "AppUtils.h"
#include "Messagebox.h"

// CCreateBranchTagDlg dialog

IMPLEMENT_DYNAMIC(CCreateBranchTagDlg, CResizableStandAloneDialog)

CCreateBranchTagDlg::CCreateBranchTagDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreateBranchTagDlg::IDD, pParent)
	, m_bForce(FALSE)
	, CChooseVersion(this)
{
	m_bIsTag=0;
	m_bSwitch = 0;	// default switch to checkbox not selected
	m_bTrack=2;
	m_bSign=0;
}

CCreateBranchTagDlg::~CCreateBranchTagDlg()
{
}

void CCreateBranchTagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CHOOSE_VERSION_DDX;

	DDX_Text(pDX, IDC_BRANCH_TAG, this->m_BranchTagName);
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);
	DDX_Check(pDX,IDC_CHECK_SWITCH,this->m_bSwitch);
	DDX_Check(pDX,IDC_CHECK_SIGN,this->m_bSign);
	DDX_Text(pDX, IDC_EDIT_MESSAGE,this->m_Message);
}


BEGIN_MESSAGE_MAP(CCreateBranchTagDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CCreateBranchTagDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BRANCH, &CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch)
//	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF, &CCreateBranchTagDlg::OnBnClickedButtonBrowseRef)
ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CCreateBranchTagDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CCreateBranchTagDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP_MESSAGE,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT_MESSAGE,TOP_LEFT,BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	AdjustControlSize(IDC_RADIO_HEAD);
	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);
	AdjustControlSize(IDC_CHECK_TRACK);
	AdjustControlSize(IDC_CHECK_FORCE);
	AdjustControlSize(IDC_CHECK_SWITCH);
	AdjustControlSize(IDC_CHECK_SIGN);

	this->SetDefaultChoose(IDC_RADIO_HEAD);

	Init();

	this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	CString sWindowTitle;
	if(this->m_bIsTag)
	{
		sWindowTitle = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CREATETAG));
		this->GetDlgItem(IDC_LABEL_BRANCH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_TAG)));
		this->GetDlgItem(IDC_CHECK_SIGN)->EnableWindow(!g_Git.GetConfigValue(_T("user.signingkey")).IsEmpty());
	}
	else
	{
		sWindowTitle = CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CREATEBRANCH));
		this->GetDlgItem(IDC_LABEL_BRANCH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_BRANCH)));
		this->GetDlgItem(IDC_EDIT_MESSAGE)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_CHECK_SIGN)->ShowWindow(SW_HIDE);
	}

	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	// show the switch checkbox if we are a create branch dialog
	this->GetDlgItem(IDC_CHECK_SWITCH)->ShowWindow( !m_bIsTag && !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir));
	CWnd* pHead = GetDlgItem(IDC_RADIO_HEAD);
	CString HeadText;
	pHead->GetWindowText( HeadText );
	pHead->SetWindowText( HeadText + " (" + g_Git.GetCurrentBranch() + ")");
	EnableSaveRestore(_T("BranchTagDlg"));

	//Create the ToolTip control
	if( !m_ToolTip.Create(this))
	{
		TRACE0("Unable to create the ToolTip!");
	}
	else
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_FORCE), CString(MAKEINTRESOURCE(IDS_PROC_NEWBRANCHTAG_FORCE_TT)));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_SIGN), CString(MAKEINTRESOURCE(IDS_PROC_NEWBRANCHTAG_SIGN_TT)));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_TRACK), CString(MAKEINTRESOURCE(IDS_PROC_NEWBRANCHTAG_TRACK_TT)));
		m_ToolTip.Activate(TRUE);
	}

	OnCbnSelchangeComboboxexBranch();
	return TRUE;
}
// CCreateBranchTagDlg message handlers

void CCreateBranchTagDlg::OnBnClickedOk()
{
	this->UpdateData(TRUE);

	if(this->m_bSign && this->m_Message.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_COMMITDLG_NOMESSAGE, IDS_APPNAME, MB_OK);
		return;
	}

	this->m_BranchTagName.Trim();
	if(!g_Git.IsBranchNameValid(this->m_BranchTagName))
	{
		ShowEditBalloon(IDC_BRANCH_TAG, IDS_B_T_NOTEMPTY, TTI_ERROR);
		return;
	}
	if (!m_bForce && g_Git.BranchTagExists(m_BranchTagName, !m_bIsTag))
	{
		CString msg;
		if (m_bIsTag)
			msg.LoadString(IDS_T_EXISTS);
		else
			msg.LoadString(IDS_B_EXISTS);
		ShowEditBalloon(IDC_BRANCH_TAG, msg + _T(" ") + CString(MAKEINTRESOURCE(IDS_B_T_DIFFERENTNAMEORFORCE)), CString(MAKEINTRESOURCE(IDS_WARN_WARNING)));
		return;
	}
	if (g_Git.BranchTagExists(m_BranchTagName, m_bIsTag == TRUE))
	{
		CString msg;
		if (m_bIsTag)
			msg.LoadString(IDS_T_SAMEBRANCHNAMEEXISTS);
		else
			msg.LoadString(IDS_B_SAMETAGNAMEEXISTS);
		if (CMessageBox::Show(m_hWnd, msg, _T("TortoiseGit"), 2, IDI_EXCLAMATION, CString(MAKEINTRESOURCE(IDS_CONTINUEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
			return;
	}

	this->UpdateRevsionName();
	OnOK();
}

void CCreateBranchTagDlg::OnCbnSelchangeComboboxexBranch()
{
	if (this->m_ChooseVersioinBranch.GetString().Left(8)==_T("remotes/") && !m_bIsTag)
	{
		bool isDefault = false;
		this->UpdateData();

		CString str = this->m_OldSelectBranch;
		int start = str.ReverseFind(_T('/'));
		if(start>=0)
			str=str.Mid(start+1);
		if(str == m_BranchTagName)
			isDefault =true;

		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(TRUE);

		if( m_BranchTagName.IsEmpty() ||  isDefault)
		{
			m_BranchTagName= m_ChooseVersioinBranch.GetString();
			int start = m_BranchTagName.Find(_T('/'), 9);;
			if(start>=0)
				m_BranchTagName = m_BranchTagName.Mid(start+1);

			UpdateData(FALSE);
		}
	}
	else
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	if(this->m_bIsTag)
		this->GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);

	m_OldSelectBranch = m_ChooseVersioinBranch.GetString();
}

void CCreateBranchTagDlg::OnVersionChanged()
{
	int radio=GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
	if (radio == IDC_RADIO_BRANCH)
		OnCbnSelchangeComboboxexBranch();
	else
		GetDlgItem(IDC_CHECK_TRACK)->EnableWindow(FALSE);
}

void CCreateBranchTagDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}
