// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2007, 2009, 2011 - TortoiseSVN

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
#include "ToolAssocDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CToolAssocDlg, CDialog)
CToolAssocDlg::CToolAssocDlg(const CString& type, bool add, CWnd* pParent /*=nullptr*/)
	: CDialog(CToolAssocDlg::IDD, pParent)
	, m_sType(type)
	, m_bAdd(add)
{
}

CToolAssocDlg::~CToolAssocDlg()
{
}

void CToolAssocDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTEDIT, m_sExtension);
	DDX_Text(pDX, IDC_TOOLEDIT, m_sTool);

	if (pDX->m_bSaveAndValidate)
	{
		if (m_sExtension.Find(L'/') < 0)
			m_sExtension.TrimLeft(L'*');
	}
}


BEGIN_MESSAGE_MAP(CToolAssocDlg, CDialog)
	ON_BN_CLICKED(IDC_TOOLBROWSE, OnBnClickedToolbrowse)
END_MESSAGE_MAP()

BOOL CToolAssocDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	EnableToolTips();
	m_tooltips.Create(this);

	CString title;
	if (m_sType == L"Diff")
	{
		title.LoadString(m_bAdd ? IDS_DLGTITLE_ADD_DIFF_TOOL : IDS_DLGTITLE_EDIT_DIFF_TOOL);
		m_tooltips.AddTool(IDC_TOOLEDIT, IDS_SETTINGS_EXTDIFF_TT);
	}
	else
	{
		title.LoadString(m_bAdd ? IDS_DLGTITLE_ADD_MERGE_TOOL : IDS_DLGTITLE_EDIT_MERGE_TOOL);
		m_tooltips.AddTool(IDC_TOOLEDIT, IDS_SETTINGS_EXTMERGE_TT);
	}

	SetWindowText(title);
	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_TOOLEDIT), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CToolAssocDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CToolAssocDlg::OnBnClickedToolbrowse()
{
	UpdateData(TRUE);
	CString filename = m_sTool;
	if (!PathFileExists(filename))
		filename.Empty();
	if (!CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
		return;

	m_sTool = filename;
	UpdateData(FALSE);
}
