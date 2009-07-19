// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseGit

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

// SyncDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SyncDlg.h"


// CSyncDlg dialog

IMPLEMENT_DYNAMIC(CSyncDlg, CDialog)

CSyncDlg::CSyncDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSyncDlg::IDD, pParent)
	, m_bAutoLoadPuttyKey(FALSE)
{

}

CSyncDlg::~CSyncDlg()
{
}

void CSyncDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_PUTTY_KEY, m_bAutoLoadPuttyKey);
	DDX_Control(pDX, IDC_COMBOBOXEX_LOCAL_BRANCH, m_ctrlLocalBranch);
	DDX_Control(pDX, IDC_COMBOBOXEX_REMOTE_BRANCH, m_ctrlRemoteBranch);
	DDX_Control(pDX, IDC_COMBOBOXEX_URL, m_ctrlURL);
	DDX_Control(pDX, IDC_BUTTON_TABCTRL, m_ctrlDumyButton);
	DDX_Control(pDX, IDC_BUTTON_PULL, m_ctrlPull);
	DDX_Control(pDX, IDC_BUTTON_PUSH, m_ctrlPush);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_ctrlStatus);
	DDX_Control(pDX, IDC_PROGRESS_SYNC, m_ctrlProgress);
	DDX_Control(pDX, IDC_ANIMATE_SYNC, m_ctrlAnimate);
}


BEGIN_MESSAGE_MAP(CSyncDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_PULL, &CSyncDlg::OnBnClickedButtonPull)
	ON_BN_CLICKED(IDC_BUTTON_PUSH, &CSyncDlg::OnBnClickedButtonPush)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CSyncDlg::OnBnClickedButtonApply)
	ON_BN_CLICKED(IDC_BUTTON_EMAIL, &CSyncDlg::OnBnClickedButtonEmail)
END_MESSAGE_MAP()


// CSyncDlg message handlers

void CSyncDlg::OnBnClickedButtonPull()
{
	// TODO: Add your control notification handler code here
}

void CSyncDlg::OnBnClickedButtonPush()
{
	// TODO: Add your control notification handler code here
}

void CSyncDlg::OnBnClickedButtonApply()
{
	// TODO: Add your control notification handler code here
}

void CSyncDlg::OnBnClickedButtonEmail()
{
	// TODO: Add your control notification handler code here
}
