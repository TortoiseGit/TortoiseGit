// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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
#include "StringUtils.h"
#include ".\settingsprogsdiff.h"


IMPLEMENT_DYNAMIC(CSettingsProgsDiff, ISettingsPropPage)
CSettingsProgsDiff::CSettingsProgsDiff()
	: ISettingsPropPage(CSettingsProgsDiff::IDD)
	, m_dlgAdvDiff(_T("Diff"))
	, m_iExtDiff(0)
	, m_sDiffPath(_T(""))
{
	m_regDiffPath = CRegString(_T("Software\\TortoiseGit\\Diff"));
}

CSettingsProgsDiff::~CSettingsProgsDiff()
{
}

void CSettingsProgsDiff::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);

	GetDlgItem(IDC_EXTDIFF)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(m_iExtDiff == 1);
	DDX_Control(pDX, IDC_EXTDIFF, m_cDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsDiff, ISettingsPropPage)
	ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
	ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
END_MESSAGE_MAP()


BOOL CSettingsProgsDiff::OnInitDialog()
{
	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_iExtDiff = IsExternal(m_sDiffPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsDiff::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsDiff::OnApply()
{
	UpdateData();
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
	{
		m_sDiffPath = _T("#") + m_sDiffPath;
	}
	m_regDiffPath = m_sDiffPath;

	m_dlgAdvDiff.SaveData();
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOff()
{
	m_iExtDiff = 0;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(false);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(false);
	CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOn()
{
	m_iExtDiff = 1;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFF)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffbrowse()
{
	if (CAppUtils::FileOpenSave(m_sDiffPath, NULL, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		UpdateData(FALSE);
		SetModified();
	}
}

void CSettingsProgsDiff::OnBnClickedExtdiffadvanced()
{
	if (m_dlgAdvDiff.DoModal() == IDOK)
		SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiff()
{
	SetModified();
}

void CSettingsProgsDiff::CheckProgComment()
{
	UpdateData();
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
		m_sDiffPath = _T("#") + m_sDiffPath;
	else if (m_iExtDiff == 1)
		m_sDiffPath.TrimLeft('#');
	UpdateData(FALSE);
}
