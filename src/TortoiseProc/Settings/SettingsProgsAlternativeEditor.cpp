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
#include ".\settingsprogsalternativeeditor.h"


IMPLEMENT_DYNAMIC(CSettingsProgsAlternativeEditor, ISettingsPropPage)
CSettingsProgsAlternativeEditor::CSettingsProgsAlternativeEditor()
	: ISettingsPropPage(CSettingsProgsAlternativeEditor::IDD)
	, m_sAlternativeEditorPath(_T(""))
	, m_iAlternativeEditor(0)
{
	m_regAlternativeEditorPath = CRegString(_T("Software\\TortoiseGit\\AlternativeEditor"));
}

CSettingsProgsAlternativeEditor::~CSettingsProgsAlternativeEditor()
{
}

void CSettingsProgsAlternativeEditor::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ALTERNATIVEEDITOR, m_sAlternativeEditorPath);
	DDX_Radio(pDX, IDC_ALTERNATIVEEDITOR_OFF, m_iAlternativeEditor);

	GetDlgItem(IDC_ALTERNATIVEEDITOR)->EnableWindow(m_iAlternativeEditor == 1);
	GetDlgItem(IDC_ALTERNATIVEEDITORBROWSE)->EnableWindow(m_iAlternativeEditor == 1);
	DDX_Control(pDX, IDC_ALTERNATIVEEDITOR, m_cAlternativeEditorEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsAlternativeEditor, ISettingsPropPage)
	ON_BN_CLICKED(IDC_ALTERNATIVEEDITOR_OFF, OnBnClickedAlternativeEditorOff)
	ON_BN_CLICKED(IDC_ALTERNATIVEEDITOR_ON, OnBnClickedAlternativeEditorOn)
	ON_BN_CLICKED(IDC_ALTERNATIVEEDITORBROWSE, OnBnClickedAlternativeEditorBrowse)
	ON_EN_CHANGE(IDC_ALTERNATIVEEDITOR, OnEnChangeAlternativeEditor)
END_MESSAGE_MAP()

void CSettingsProgsAlternativeEditor::OnBnClickedAlternativeEditorOff()
{
	m_iAlternativeEditor = 0;
	SetModified();
	GetDlgItem(IDC_ALTERNATIVEEDITOR)->EnableWindow(FALSE);
	GetDlgItem(IDC_ALTERNATIVEEDITORBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsAlternativeEditor::OnBnClickedAlternativeEditorOn()
{
	m_iAlternativeEditor = 1;
	SetModified();
	GetDlgItem(IDC_ALTERNATIVEEDITOR)->EnableWindow(TRUE);
	GetDlgItem(IDC_ALTERNATIVEEDITORBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_ALTERNATIVEEDITOR)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsAlternativeEditor::OnEnChangeAlternativeEditor()
{
	SetModified();
}

void CSettingsProgsAlternativeEditor::OnBnClickedAlternativeEditorBrowse()
{
	if (CAppUtils::FileOpenSave(m_sAlternativeEditorPath, NULL, IDS_SETTINGS_SELECTDIFFVIEWER, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		UpdateData(FALSE);
		SetModified();
	}
}

BOOL CSettingsProgsAlternativeEditor::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	EnableToolTips();

	m_sAlternativeEditorPath = m_regAlternativeEditorPath;
	m_iAlternativeEditor = IsExternal(m_sAlternativeEditorPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_ALTERNATIVEEDITOR), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_ALTERNATIVEEDITOR, IDS_SETTINGS_ALTERNATIVEEDITOR_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsAlternativeEditor::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsAlternativeEditor::OnApply()
{
	UpdateData();
	if (m_iAlternativeEditor == 0 && !m_sAlternativeEditorPath.IsEmpty() && m_sAlternativeEditorPath.Left(1) != _T("#"))
		m_sAlternativeEditorPath = _T("#") + m_sAlternativeEditorPath;

	m_regAlternativeEditorPath = m_sAlternativeEditorPath;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsProgsAlternativeEditor::CheckProgComment()
{
	UpdateData();
	if (m_iAlternativeEditor == 0 && !m_sAlternativeEditorPath.IsEmpty() && m_sAlternativeEditorPath.Left(1) != _T("#"))
		m_sAlternativeEditorPath = _T("#") + m_sAlternativeEditorPath;
	else if (m_iAlternativeEditor == 1)
		m_sAlternativeEditorPath.TrimLeft('#');
	UpdateData(FALSE);
}
