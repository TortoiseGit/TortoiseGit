// TortoiseSVN - a Windows shell extension for easy version control

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
	, m_iExtDiffProps(0)
	, m_sDiffPropsPath(_T(""))
	, m_regConvertBase(_T("Software\\TortoiseGit\\ConvertBase"), TRUE)
	, m_bConvertBase(false)
{
	m_regDiffPath = CRegString(_T("Software\\TortoiseGit\\Diff"));
	m_regDiffPropsPath = CRegString(_T("Software\\TortoiseGit\\DiffProps"));
}

CSettingsProgsDiff::~CSettingsProgsDiff()
{
}

void CSettingsProgsDiff::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);
	DDX_Text(pDX, IDC_EXTDIFFPROPS, m_sDiffPropsPath);
	DDX_Radio(pDX, IDC_EXTDIFFPROPS_OFF, m_iExtDiffProps);
	DDX_Check(pDX, IDC_DONTCONVERT, m_bConvertBase);

	GetDlgItem(IDC_EXTDIFF)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFPROPS)->EnableWindow(m_iExtDiffProps == 1);
	GetDlgItem(IDC_EXTDIFFPROPSBROWSE)->EnableWindow(m_iExtDiffProps == 1);
	DDX_Control(pDX, IDC_EXTDIFF, m_cDiffEdit);
	DDX_Control(pDX, IDC_EXTDIFFPROPS, m_cDiffPropsEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsDiff, ISettingsPropPage)
	ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
	ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
	ON_BN_CLICKED(IDC_EXTDIFFPROPS_OFF, OnBnClickedExtdiffpropsOff)
	ON_BN_CLICKED(IDC_EXTDIFFPROPS_ON, OnBnClickedExtdiffpropsOn)
	ON_BN_CLICKED(IDC_EXTDIFFPROPSBROWSE, OnBnClickedExtdiffpropsbrowse)
	ON_BN_CLICKED(IDC_DONTCONVERT, OnBnClickedDontconvert)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
	ON_EN_CHANGE(IDC_EXTDIFFPROPS, OnEnChangeExtdiffprops)
END_MESSAGE_MAP()


BOOL CSettingsProgsDiff::OnInitDialog()
{
	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_iExtDiff = IsExternal(m_sDiffPath);

	m_sDiffPropsPath = m_regDiffPropsPath;
	m_iExtDiffProps = IsExternal(m_sDiffPropsPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_bConvertBase = m_regConvertBase;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DONTCONVERT, IDS_SETTINGS_CONVERTBASE_TT);

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

	if (m_iExtDiffProps == 0 && !m_sDiffPropsPath.IsEmpty() && m_sDiffPropsPath.Left(1) != _T("#"))
	{
		m_sDiffPropsPath = _T("#") + m_sDiffPropsPath;
	}
	m_regDiffPropsPath = m_sDiffPropsPath;

	m_regConvertBase = m_bConvertBase;
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

void CSettingsProgsDiff::OnBnClickedExtdiffpropsOff()
{
	m_iExtDiffProps = 0;
	SetModified();
	GetDlgItem(IDC_EXTDIFFPROPS)->EnableWindow(false);
	GetDlgItem(IDC_EXTDIFFPROPSBROWSE)->EnableWindow(false);
	CheckProgCommentProps();
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

void CSettingsProgsDiff::OnBnClickedExtdiffpropsOn()
{
	m_iExtDiffProps = 1;
	SetModified();
	GetDlgItem(IDC_EXTDIFFPROPS)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFFPROPSBROWSE)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFFPROPS)->SetFocus();
	CheckProgCommentProps();
}

void CSettingsProgsDiff::OnBnClickedExtdiffbrowse()
{
	if (CAppUtils::FileOpenSave(m_sDiffPath, NULL, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		UpdateData(FALSE);
		SetModified();
	}
}

void CSettingsProgsDiff::OnBnClickedExtdiffpropsbrowse()
{
	if (CAppUtils::FileOpenSave(m_sDiffPropsPath, NULL, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
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

void CSettingsProgsDiff::OnBnClickedDontconvert()
{
	SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiff()
{
	SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiffprops()
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

void CSettingsProgsDiff::CheckProgCommentProps()
{
	UpdateData();
	if (m_iExtDiffProps == 0 && !m_sDiffPropsPath.IsEmpty() && m_sDiffPropsPath.Left(1) != _T("#"))
		m_sDiffPropsPath = _T("#") + m_sDiffPropsPath;
	else if (m_iExtDiffProps == 1)
		m_sDiffPropsPath.TrimLeft('#');
	UpdateData(FALSE);
}
