// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010, 2013-2019 - TortoiseGit
// Copyright (C) 2003-2008,2010 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetHooksAdv.h"
#include "BrowseFolder.h"
#include "AppUtils.h"
#include "Git.h"
#include "GitAdminDir.h"

IMPLEMENT_DYNAMIC(CSetHooksAdv, CResizableStandAloneDialog)

CSetHooksAdv::CSetHooksAdv(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CSetHooksAdv::IDD, pParent)
	, m_bEnabled(FALSE)
	, m_bWait(FALSE)
	, m_bHide(FALSE)
	, m_bLocal(FALSE)
{
}

CSetHooksAdv::~CSetHooksAdv()
{
}

void CSetHooksAdv::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_HOOKPATH, m_sPath);
	DDX_Text(pDX, IDC_HOOKCOMMANDLINE, m_sCommandLine);
	DDX_Check(pDX, IDC_ENABLE, m_bEnabled);
	DDX_Check(pDX, IDC_LOCALCHECK, m_bLocal);
	DDX_Check(pDX, IDC_WAITCHECK, m_bWait);
	DDX_Check(pDX, IDC_HIDECHECK, m_bHide);
	DDX_Control(pDX, IDC_HOOKTYPECOMBO, m_cHookTypeCombo);
}


BEGIN_MESSAGE_MAP(CSetHooksAdv, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_HOOKBROWSE, &CSetHooksAdv::OnBnClickedHookbrowse)
	ON_BN_CLICKED(IDC_HOOKCOMMANDBROWSE, &CSetHooksAdv::OnBnClickedHookcommandbrowse)
	ON_BN_CLICKED(IDC_LOCALCHECK, &CSetHooksAdv::OnBnClickedLocalcheck)
END_MESSAGE_MAP()

BOOL CSetHooksAdv::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AdjustControlSize(IDC_ENABLE);
	AdjustControlSize(IDC_WAITCHECK);
	AdjustControlSize(IDC_HIDECHECK);
	AdjustControlSize(IDC_LOCALCHECK);

	// initialize the combo box with all the hook types we have
	int index;
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, start_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRECOMMIT)));
	m_cHookTypeCombo.SetItemData(index, pre_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, post_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREPUSH)));
	m_cHookTypeCombo.SetItemData(index, pre_push_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTPUSH)));
	m_cHookTypeCombo.SetItemData(index, post_push_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREREBASE)));
	m_cHookTypeCombo.SetItemData(index, pre_rebase_hook);

	// preselect the right hook type in the combobox
	for (int i=0; i<m_cHookTypeCombo.GetCount(); ++i)
	{
		hooktype ht = static_cast<hooktype>(m_cHookTypeCombo.GetItemData(i));
		if (ht == key.htype)
		{
			CString str;
			m_cHookTypeCombo.GetLBText(i, str);
			m_cHookTypeCombo.SelectString(i, str);
			break;
		}
	}

	m_sPath = key.path.GetWinPathString();
	m_sCommandLine = cmd.commandline;
	m_bWait = cmd.bWait;
	m_bHide = !cmd.bShow;
	m_bLocal = cmd.bLocal;
	m_bEnabled = cmd.bEnabled ? BST_CHECKED : BST_UNCHECKED;

	UpdateData(FALSE);
	OnBnClickedLocalcheck();

	DialogEnableWindow(IDC_LOCALCHECK, GitAdminDir::HasAdminDir(g_Git.m_CurrentDir));

	AddAnchor(IDC_ENABLE, TOP_LEFT);
	AddAnchor(IDC_HOOKTYPELABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKTYPECOMBO, TOP_RIGHT);
	AddAnchor(IDC_HOOKWCPATHLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOCALCHECK, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKPATH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKBROWSE, TOP_RIGHT);
	AddAnchor(IDC_HOOKCMLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKCOMMANDLINE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKCOMMANDBROWSE, TOP_RIGHT);
	AddAnchor(IDC_WAITCHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HIDECHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(L"SetHooksAdvDlg");
	return TRUE;
}

void CSetHooksAdv::OnOK()
{
	UpdateData();
	int cursel = m_cHookTypeCombo.GetCurSel();
	key.htype = unknown_hook;
	if (cursel != CB_ERR)
	{
		key.htype = static_cast<hooktype>(m_cHookTypeCombo.GetItemData(cursel));
		key.path = CTGitPath(m_sPath);
		cmd.commandline = m_sCommandLine;
		cmd.bEnabled = m_bEnabled == BST_CHECKED;
		cmd.bWait = !!m_bWait;
		cmd.bShow = !m_bHide;
		cmd.bLocal = !!m_bLocal;
	}
	if (key.htype == unknown_hook)
	{
		m_tooltips.ShowBalloon(IDC_HOOKTYPECOMBO, IDS_ERR_NOHOOKTYPESPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}
	if (!m_bLocal)
	{
		if (key.path.IsEmpty())
		{
			ShowEditBalloon(IDC_HOOKPATH, IDS_ERR_NOHOOKPATHSPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
			return;
		}
		if (key.path.GetWinPathString() != L"*" && (!PathIsDirectory(key.path.GetWinPathString()) || PathIsRelative(key.path.GetWinPathString())))
		{
			ShowEditBalloon(IDC_HOOKPATH, static_cast<LPCTSTR>(CFormatMessageWrapper(ERROR_PATH_NOT_FOUND)), CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), TTI_ERROR);
			return;
		}
	}
	if (cmd.commandline.IsEmpty())
	{
		ShowEditBalloon(IDC_HOOKCOMMANDLINE, IDS_ERR_NOHOOKCOMMANDPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	CResizableStandAloneDialog::OnOK();
}

void CSetHooksAdv::OnBnClickedHookbrowse()
{
	UpdateData();
	CBrowseFolder browser;
	CString sPath;
	browser.SetInfo(CString(MAKEINTRESOURCE(IDS_SETTINGS_HOOKS_SELECTFOLDERPATH)));
	browser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	if (browser.Show(m_hWnd, sPath) == CBrowseFolder::OK)
	{
		m_sPath = sPath;
		UpdateData(FALSE);
	}
}

void CSetHooksAdv::OnBnClickedHookcommandbrowse()
{
	UpdateData();
	CString sCmdLine = m_sCommandLine;
	if (!PathFileExists(sCmdLine))
		sCmdLine.Empty();
	// Display the Open dialog box.
	if (CAppUtils::FileOpenSave(sCmdLine, nullptr, IDS_SETTINGS_HOOKS_SELECTSCRIPTFILE, IDS_COMMONFILEFILTER, true, m_hWnd))
	{
		m_sCommandLine = sCmdLine;
		UpdateData(FALSE);
	}
}

void CSetHooksAdv::OnBnClickedLocalcheck()
{
	UpdateData();
	DialogEnableWindow(IDC_HOOKPATH, !m_bLocal);
	DialogEnableWindow(IDC_HOOKBROWSE, !m_bLocal);
}
