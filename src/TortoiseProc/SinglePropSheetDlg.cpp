// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013, 2018, 2024 - TortoiseGit

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

// SinglePropSheetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SinglePropSheetDlg.h"
#include "Git.h"
#include "AppUtils.h"
#include "DarkModeHelper.h"
#include "DPIAware.h"
#include "AutoCloakWindow.h"

// CSinglePropSheetDlg dialog
using namespace TreePropSheet;

IMPLEMENT_DYNAMIC(CSinglePropSheetDlg, CTreePropSheet)

CSinglePropSheetDlg::CSinglePropSheetDlg(const wchar_t* szCaption, ISettingsPropPage* pThePropPage, CWnd* pParent /*=nullptr*/)
:	CTreePropSheet(szCaption,pParent),// CSinglePropSheetDlg::IDD, pParent),
	m_pThePropPage(pThePropPage)
{
	SetTreeViewMode(TRUE, TRUE, TRUE);
	SetTreeWidth(220 * CDPIAware::Instance().GetDPI(nullptr) / 96);
	AddPropPages();
}

CSinglePropSheetDlg::~CSinglePropSheetDlg()
{
	RemovePropPages();
}

void CSinglePropSheetDlg::AddPropPages()
{
	SetPageIcon(m_pThePropPage, m_pThePropPage->GetIconID());
	AddPage(m_pThePropPage);
}

void CSinglePropSheetDlg::RemovePropPages()
{
	delete m_pThePropPage;
}

void CSinglePropSheetDlg::DoDataExchange(CDataExchange* pDX)
{
	CTreePropSheet::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSinglePropSheetDlg, CTreePropSheet)
END_MESSAGE_MAP()


// CSinglePropSheetDlg message handlers

BOOL CSinglePropSheetDlg::OnInitDialog()
{
	CAutoCloakWindow window_cloaker{ GetSafeHwnd() };
	BOOL bReturn = CTreePropSheet::OnInitDialog();

	if (GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
		CAppUtils::SetWindowTitle(*this, g_Git.m_CurrentDir);
	else
	{
		CString title;
		GetWindowText(title);
		SetWindowText(title + L" - " + CString(MAKEINTRESOURCE(IDS_APPNAME)));
	}

	CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

	DarkModeHelper::Instance().AllowDarkModeForApp(CTheme::Instance().IsDarkTheme());
	SetTheme(CTheme::Instance().IsDarkTheme());
	CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

	return bReturn;
}
