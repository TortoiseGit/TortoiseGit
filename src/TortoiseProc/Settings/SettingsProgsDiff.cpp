// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013-2016 - TortoiseGit
// Copyright (C) 2003-2007,2011 - TortoiseSVN

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
#include "SettingsProgsDiff.h"


IMPLEMENT_DYNAMIC(CSettingsProgsDiff, ISettingsPropPage)
CSettingsProgsDiff::CSettingsProgsDiff()
	: ISettingsPropPage(CSettingsProgsDiff::IDD)
	, m_dlgAdvDiff(L"Diff")
	, m_iExtDiff(0)
	, m_iDiffViewer(0)
{
	m_regDiffPath = CRegString(L"Software\\TortoiseGit\\Diff");
	m_regDiffViewerPath = CRegString(L"Software\\TortoiseGit\\DiffViewer");
}

CSettingsProgsDiff::~CSettingsProgsDiff()
{
}

void CSettingsProgsDiff::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Radio(pDX, IDC_DIFFVIEWER_OFF, m_iDiffViewer);

	GetDlgItem(IDC_EXTDIFF)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(m_iDiffViewer == 1);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(m_iDiffViewer == 1);
	DDX_Control(pDX, IDC_EXTDIFF, m_cDiffEdit);
	DDX_Control(pDX, IDC_DIFFVIEWER, m_cUnifiedDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsDiff, ISettingsPropPage)
	ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
	ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
	ON_BN_CLICKED(IDC_DIFFVIEWER_OFF, OnBnClickedDiffviewerOff)
	ON_BN_CLICKED(IDC_DIFFVIEWER_ON, OnBnClickedDiffviewerOn)
	ON_BN_CLICKED(IDC_DIFFVIEWERBROWSE, OnBnClickedDiffviewerbrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
END_MESSAGE_MAP()


BOOL CSettingsProgsDiff::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();
	EnableToolTips();
	AdjustControlSize(IDC_EXTDIFF_OFF);
	AdjustControlSize(IDC_EXTDIFF_ON);
	AdjustControlSize(IDC_DIFFVIEWER_OFF);
	AdjustControlSize(IDC_DIFFVIEWER_ON);

	m_sDiffPath = m_regDiffPath;
	m_iExtDiff = IsExternal(m_sDiffPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_sDiffViewerPath = m_regDiffViewerPath;
	m_iDiffViewer = IsExternal(m_sDiffViewerPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_DIFFVIEWER), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsDiff::OnApply()
{
	UpdateData();
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != L"#")
		m_sDiffPath = L'#' + m_sDiffPath;
	m_regDiffPath = m_sDiffPath;

	m_dlgAdvDiff.SaveData();

	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != L"#")
		m_sDiffViewerPath = L'#' + m_sDiffViewerPath;

	m_regDiffViewerPath = m_sDiffViewerPath;

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
	UpdateData();
	CString filename = m_sDiffPath;
	if (!PathFileExists(filename))
		filename.Empty();
	if (CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		m_sDiffPath = filename;
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
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != L"#")
		m_sDiffPath = L'#' + m_sDiffPath;
	else if (m_iExtDiff == 1)
		m_sDiffPath.TrimLeft(L'#');
	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != L"#")
		m_sDiffViewerPath = L'#' + m_sDiffViewerPath;
	else if (m_iDiffViewer == 1)
		m_sDiffViewerPath.TrimLeft(L'#');
	UpdateData(FALSE);
}

void CSettingsProgsDiff::OnBnClickedDiffviewerOff()
{
	m_iDiffViewer = 0;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedDiffviewerOn()
{
	m_iDiffViewer = 1;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWER)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsDiff::OnEnChangeDiffviewer()
{
	SetModified();
}

void CSettingsProgsDiff::OnBnClickedDiffviewerbrowse()
{
	UpdateData();
	CString filename = m_sDiffViewerPath;
	if (!PathFileExists(filename))
		filename.Empty();
	if (CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTDIFFVIEWER, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		m_sDiffViewerPath = filename;
		UpdateData(FALSE);
		SetModified();
	}
}
