// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014-2016 - TortoiseGit
// Copyright (C) 2011 - TortoiseSVN

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
#include "BugtraqRegexTestDlg.h"
#include "AppUtils.h"
#include "ProjectProperties.h"

// CBugtraqRegexTestDlg dialog

IMPLEMENT_DYNAMIC(CBugtraqRegexTestDlg, CResizableStandAloneDialog)

CBugtraqRegexTestDlg::CBugtraqRegexTestDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CBugtraqRegexTestDlg::IDD, pParent)
{
}

CBugtraqRegexTestDlg::~CBugtraqRegexTestDlg()
{
}

void CBugtraqRegexTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BUGTRAQLOGREGEX1, m_sBugtraqRegex1);
	DDX_Text(pDX, IDC_BUGTRAQLOGREGEX2, m_sBugtraqRegex2);
	DDX_Control(pDX, IDC_BUGTRAQLOGREGEX1, m_BugtraqRegex1);
	DDX_Control(pDX, IDC_BUGTRAQLOGREGEX2, m_BugtraqRegex2);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
}

BEGIN_MESSAGE_MAP(CBugtraqRegexTestDlg, CResizableStandAloneDialog)
	ON_EN_CHANGE(IDC_BUGTRAQLOGREGEX1, &CBugtraqRegexTestDlg::OnEnChangeBugtraqlogregex1)
	ON_EN_CHANGE(IDC_BUGTRAQLOGREGEX2, &CBugtraqRegexTestDlg::OnEnChangeBugtraqlogregex2)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

void CBugtraqRegexTestDlg::OnEnChangeBugtraqlogregex1()
{
	UpdateLogControl();
}

void CBugtraqRegexTestDlg::OnEnChangeBugtraqlogregex2()
{
	UpdateLogControl();
}

BOOL CBugtraqRegexTestDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	ProjectProperties projectprops;
	projectprops.lProjectLanguage = -1;
	projectprops.SetBugIDRe(m_sBugtraqRegex1);
	projectprops.SetCheckRe(m_sBugtraqRegex2);

	m_cLogMessage.Init(projectprops);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());

	m_cLogMessage.SetText(CString(MAKEINTRESOURCE(IDS_SAMPLEBUGTRAQTESTMSG)));
	m_cLogMessage.Call(SCI_SETCURRENTPOS, 0);
	m_cLogMessage.Call(SCI_SETSEL, 0, 0);

	AddAnchor(IDC_SAMPLELABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REGEXIDLABEL, BOTTOM_LEFT);
	AddAnchor(IDC_BUGTRAQLOGREGEX1, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REGEXMSGLABEL, BOTTOM_LEFT);
	AddAnchor(IDC_BUGTRAQLOGREGEX2, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	return TRUE;
}

void CBugtraqRegexTestDlg::UpdateLogControl()
{
	if (m_BugtraqRegex1.IsValidRegex() && m_BugtraqRegex2.IsValidRegex())
	{
		UpdateData();
		ProjectProperties projectprops;
		projectprops.lProjectLanguage = -1;
		projectprops.SetBugIDRe(m_sBugtraqRegex1.Trim());
		projectprops.SetCheckRe(m_sBugtraqRegex2.Trim());
		m_cLogMessage.Init(projectprops);
		m_cLogMessage.RestyleBugIDs();
	}
}

void CBugtraqRegexTestDlg::OnSysColorChange()
{
	__super::OnSysColorChange();

	m_cLogMessage.SetColors(true);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
}
