// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2013-2016 - TortoiseGit
// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "AppUtils.h"
#include "SettingsProgsMerge.h"


IMPLEMENT_DYNAMIC(CSettingsProgsMerge, ISettingsPropPage)
CSettingsProgsMerge::CSettingsProgsMerge()
	: ISettingsPropPage(CSettingsProgsMerge::IDD)
	, m_iExtMerge(0)
	, m_dlgAdvMerge(L"Merge")
{
	m_regMergePath = CRegString(L"Software\\TortoiseGit\\Merge");
}

CSettingsProgsMerge::~CSettingsProgsMerge()
{
}

void CSettingsProgsMerge::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTMERGE, m_sMergePath);
	DDX_Radio(pDX, IDC_EXTMERGE_OFF, m_iExtMerge);

	GetDlgItem(IDC_EXTMERGE)->EnableWindow(m_iExtMerge == 1);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(m_iExtMerge == 1);
	DDX_Control(pDX, IDC_EXTMERGE, m_cMergeEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsMerge, ISettingsPropPage)
	ON_BN_CLICKED(IDC_EXTMERGE_OFF, OnBnClickedExtmergeOff)
	ON_BN_CLICKED(IDC_EXTMERGE_ON, OnBnClickedExtmergeOn)
	ON_BN_CLICKED(IDC_EXTMERGEBROWSE, OnBnClickedExtmergebrowse)
	ON_BN_CLICKED(IDC_EXTMERGEADVANCED, OnBnClickedExtmergeadvanced)
	ON_EN_CHANGE(IDC_EXTMERGE, OnEnChangeExtmerge)
END_MESSAGE_MAP()


BOOL CSettingsProgsMerge::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_EXTMERGE_OFF);
	AdjustControlSize(IDC_EXTMERGE_ON);
	EnableToolTips();

	m_sMergePath = m_regMergePath;
	m_iExtMerge = IsExternal(m_sMergePath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTMERGE), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsMerge::OnApply()
{
	UpdateData();
	if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != L"#")
		m_sMergePath = L'#' + m_sMergePath;

	m_regMergePath = m_sMergePath;

	m_dlgAdvMerge.SaveData();
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsProgsMerge::OnBnClickedExtmergeOff()
{
	m_iExtMerge = 0;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsMerge::OnBnClickedExtmergeOn()
{
	m_iExtMerge = 1;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGE)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsMerge::OnEnChangeExtmerge()
{
	SetModified();
}

void CSettingsProgsMerge::OnBnClickedExtmergebrowse()
{
	UpdateData();
	CString filename = m_sMergePath;
	if (!PathFileExists(filename))
		filename.Empty();
	if (CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTMERGE, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		m_sMergePath = filename;
		UpdateData(FALSE);
		SetModified();
	}
}

void CSettingsProgsMerge::OnBnClickedExtmergeadvanced()
{
	if (m_dlgAdvMerge.DoModal() == IDOK)
		SetModified();
}

void CSettingsProgsMerge::CheckProgComment()
{
	UpdateData();
	if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != L"#")
		m_sMergePath = L'#' + m_sMergePath;
	else if (m_iExtMerge == 1)
		m_sMergePath.TrimLeft(L'#');
	UpdateData(FALSE);
}