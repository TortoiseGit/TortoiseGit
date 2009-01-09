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
#include ".\settingsprogsunidiff.h"


IMPLEMENT_DYNAMIC(CSettingsProgsUniDiff, ISettingsPropPage)
CSettingsProgsUniDiff::CSettingsProgsUniDiff()
	: ISettingsPropPage(CSettingsProgsUniDiff::IDD)
	, m_sDiffViewerPath(_T(""))
	, m_iDiffViewer(0)
{
	m_regDiffViewerPath = CRegString(_T("Software\\TortoiseGit\\DiffViewer"));
}

CSettingsProgsUniDiff::~CSettingsProgsUniDiff()
{
}

void CSettingsProgsUniDiff::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Radio(pDX, IDC_DIFFVIEWER_OFF, m_iDiffViewer);

	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(m_iDiffViewer == 1);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(m_iDiffViewer == 1);
	DDX_Control(pDX, IDC_DIFFVIEWER, m_cUnifiedDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsUniDiff, ISettingsPropPage)
	ON_BN_CLICKED(IDC_DIFFVIEWER_OFF, OnBnClickedDiffviewerOff)
	ON_BN_CLICKED(IDC_DIFFVIEWER_ON, OnBnClickedDiffviewerOn)
	ON_BN_CLICKED(IDC_DIFFVIEWERBROWSE, OnBnClickedDiffviewerbrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
END_MESSAGE_MAP()

void CSettingsProgsUniDiff::OnBnClickedDiffviewerOff()
{
	m_iDiffViewer = 0;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsUniDiff::OnBnClickedDiffviewerOn()
{
	m_iDiffViewer = 1;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWER)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsUniDiff::OnEnChangeDiffviewer()
{
	SetModified();
}

void CSettingsProgsUniDiff::OnBnClickedDiffviewerbrowse()
{
	if (CAppUtils::FileOpenSave(m_sDiffViewerPath, NULL, IDS_SETTINGS_SELECTDIFFVIEWER, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		UpdateData(FALSE);
		SetModified();
	}
}

BOOL CSettingsProgsUniDiff::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	EnableToolTips();

	m_sDiffViewerPath = m_regDiffViewerPath;
	m_iDiffViewer = IsExternal(m_sDiffViewerPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_DIFFVIEWER), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsUniDiff::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsUniDiff::OnApply()
{
	UpdateData();
	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
		m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;

	m_regDiffViewerPath = m_sDiffViewerPath;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsProgsUniDiff::CheckProgComment()
{
	UpdateData();
	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
		m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;
	else if (m_iDiffViewer == 1)
		m_sDiffViewerPath.TrimLeft('#');
	UpdateData(FALSE);
}