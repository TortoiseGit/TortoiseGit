// TortoiseGit - a Windows shell extension for easy version control

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


IMPLEMENT_DYNAMIC(CSetHooksAdv, CResizableStandAloneDialog)

CSetHooksAdv::CSetHooksAdv(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSetHooksAdv::IDD, pParent)
	, m_sPath(_T(""))
	, m_sCommandLine(_T(""))
	, m_bWait(FALSE)
	, m_bHide(FALSE)
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
	DDX_Check(pDX, IDC_WAITCHECK, m_bWait);
	DDX_Check(pDX, IDC_HIDECHECK, m_bHide);
	DDX_Control(pDX, IDC_HOOKTYPECOMBO, m_cHookTypeCombo);
}


BEGIN_MESSAGE_MAP(CSetHooksAdv, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_HOOKBROWSE, &CSetHooksAdv::OnBnClickedHookbrowse)
	ON_BN_CLICKED(IDC_HOOKCOMMANDBROWSE, &CSetHooksAdv::OnBnClickedHookcommandbrowse)
	ON_BN_CLICKED(IDHELP, &CSetHooksAdv::OnBnClickedHelp)
END_MESSAGE_MAP()

BOOL CSetHooksAdv::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// initialize the combo box with all the hook types we have
	int index;
	/*
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, start_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRECOMMIT)));
	m_cHookTypeCombo.SetItemData(index, pre_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, post_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTUPDATE)));
	m_cHookTypeCombo.SetItemData(index, start_update_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREUPDATE)));
	m_cHookTypeCombo.SetItemData(index, pre_update_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTUPDATE)));
	m_cHookTypeCombo.SetItemData(index, post_update_hook);
	*/
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREPUSH)));
	m_cHookTypeCombo.SetItemData(index, pre_push_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTPUSH)));
	m_cHookTypeCombo.SetItemData(index, post_push_hook);

	// preselect the right hook type in the combobox
	for (int i=0; i<m_cHookTypeCombo.GetCount(); ++i)
	{
		hooktype ht = (hooktype)m_cHookTypeCombo.GetItemData(i);
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
	m_tooltips.Create(this);
	UpdateData(FALSE);

	AddAnchor(IDC_HOOKTYPELABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKTYPECOMBO, TOP_RIGHT);
	AddAnchor(IDC_HOOKWCPATHLABEL, TOP_LEFT, TOP_RIGHT);
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
	EnableSaveRestore(_T("SetHooksAdvDlg"));
	return TRUE;
}

void CSetHooksAdv::OnOK()
{

	UpdateData();
	int cursel = m_cHookTypeCombo.GetCurSel();
	key.htype = unknown_hook;
	if (cursel != CB_ERR)
	{
		key.htype = (hooktype)m_cHookTypeCombo.GetItemData(cursel);
		key.path = CTGitPath(m_sPath);
		cmd.commandline = m_sCommandLine;
		cmd.bWait = !!m_bWait;
		cmd.bShow = !m_bHide;
	}
	if (key.htype == unknown_hook)
	{
		m_tooltips.ShowBalloon(IDC_HOOKTYPECOMBO, IDS_ERR_NOHOOKTYPESPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}
	if (key.path.IsEmpty())
	{
		ShowEditBalloon(IDC_HOOKPATH, IDS_ERR_NOHOOKPATHSPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
		return;
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
	if (CAppUtils::FileOpenSave(sCmdLine, NULL, IDS_SETTINGS_HOOKS_SELECTSCRIPTFILE, IDS_COMMONFILEFILTER, true, m_hWnd))
	{
		m_sCommandLine = sCmdLine;
		UpdateData(FALSE);
	}
}

void CSetHooksAdv::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CSetHooksAdv::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
