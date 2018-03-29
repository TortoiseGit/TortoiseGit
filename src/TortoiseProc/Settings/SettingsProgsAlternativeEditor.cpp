// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN
// Copyright (C) 2011, 2013-2016, 2018 - TortoiseGit

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
#include "SettingsProgsAlternativeEditor.h"


IMPLEMENT_DYNAMIC(CSettingsProgsAlternativeEditor, ISettingsPropPage)
CSettingsProgsAlternativeEditor::CSettingsProgsAlternativeEditor()
	: ISettingsPropPage(CSettingsProgsAlternativeEditor::IDD)
	, m_iAlternativeEditor(0)
{
	m_regAlternativeEditorPath = CRegString(L"Software\\TortoiseGit\\AlternativeEditor");
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
	UpdateData();
	CString filename = m_sAlternativeEditorPath;
	if (!PathFileExists(filename))
		filename.Empty();
	if (CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTDIFFVIEWER, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		m_sAlternativeEditorPath = filename;
		UpdateData(FALSE);
		SetModified();
	}
}

BOOL CSettingsProgsAlternativeEditor::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_ALTERNATIVEEDITOR_OFF);
	AdjustControlSize(IDC_ALTERNATIVEEDITOR_ON);

	EnableToolTips();

	m_sAlternativeEditorPath = m_regAlternativeEditorPath;
	m_iAlternativeEditor = IsExternal(m_sAlternativeEditorPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_ALTERNATIVEEDITOR), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.AddTool(IDC_ALTERNATIVEEDITOR, IDS_SETTINGS_ALTERNATIVEEDITOR_TT);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsAlternativeEditor::OnApply()
{
	UpdateData();
	if (m_iAlternativeEditor == 0 && !m_sAlternativeEditorPath.IsEmpty() && m_sAlternativeEditorPath.Left(1) != L"#")
		m_sAlternativeEditorPath = L'#' + m_sAlternativeEditorPath;

	m_regAlternativeEditorPath = m_sAlternativeEditorPath;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsProgsAlternativeEditor::CheckProgComment()
{
	UpdateData();
	if (m_iAlternativeEditor == 0 && !m_sAlternativeEditorPath.IsEmpty() && m_sAlternativeEditorPath.Left(1) != L"#")
		m_sAlternativeEditorPath = L'#' + m_sAlternativeEditorPath;
	else if (m_iAlternativeEditor == 1)
		m_sAlternativeEditorPath.TrimLeft(L'#');
	UpdateData(FALSE);
}
