// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014, 2016-2017, 2019 - TortoiseGit

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
// FormatPatch.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "FormatPatchDlg.h"
#include "Git.h"
#include "BrowseFolder.h"
#include "LogDlg.h"
#include "BrowseRefsDlg.h"
#include "AppUtils.h"

// CFormatPatchDlg dialog

IMPLEMENT_DYNAMIC(CFormatPatchDlg, CHorizontalResizableStandAloneDialog)

CFormatPatchDlg::CFormatPatchDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CFormatPatchDlg::IDD, pParent)
	, m_regSendMail(L"Software\\TortoiseGit\\TortoiseProc\\FormatPatch\\SendMail", 0)
	, m_regNoPrefix(L"Software\\TortoiseGit\\TortoiseProc\\FormatPatch\\NoPrefix", FALSE)
	, m_Num(1)
{
	this->m_bSendMail = m_regSendMail;
	this->m_Radio = IDC_RADIO_SINCE;
	m_bNoPrefix = m_regNoPrefix;

	CString workingDir = g_Git.m_CurrentDir;
	workingDir.Replace(L':', L'_');
	m_regSince = CRegString(L"Software\\TortoiseGit\\History\\FormatPatch\\" + workingDir + L"\\Since");
}

CFormatPatchDlg::~CFormatPatchDlg()
{
}

void CFormatPatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_DIR,	m_cDir);
	DDX_Control(pDX, IDC_COMBOBOXEX_SINCE,	m_cSince);
	DDX_Control(pDX, IDC_COMBOBOXEX_FROM,	m_cFrom);
	DDX_Control(pDX, IDC_COMBOBOXEX_TO,		m_cTo);
	DDX_Control(pDX, IDC_EDIT_NUM,			m_cNum);
	DDX_Control(pDX, IDC_SPIN_NUM,			m_spinNum);

	DDX_Text(pDX,IDC_EDIT_NUM,m_Num);

	DDX_Text(pDX, IDC_COMBOBOXEX_DIR,	m_Dir);
	DDX_Text(pDX, IDC_COMBOBOXEX_SINCE,	m_Since);
	DDX_Text(pDX, IDC_COMBOBOXEX_FROM,	m_From);
	DDX_Text(pDX, IDC_COMBOBOXEX_TO,	m_To);

	DDX_Check(pDX, IDC_CHECK_SENDMAIL, m_bSendMail);
	DDX_Check(pDX, IDC_CHECK_NOPREFIX, m_bNoPrefix);
}


BEGIN_MESSAGE_MAP(CFormatPatchDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BUTTON_DIR, &CFormatPatchDlg::OnBnClickedButtonDir)
	ON_BN_CLICKED(IDC_BUTTON_FROM, &CFormatPatchDlg::OnBnClickedButtonFrom)
	ON_BN_CLICKED(IDC_BUTTON_TO, &CFormatPatchDlg::OnBnClickedButtonTo)
	ON_BN_CLICKED(IDOK, &CFormatPatchDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_SINCE, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_NUM, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_RANGE, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_BUTTON_REF, &CFormatPatchDlg::OnBnClickedButtonRef)
	ON_BN_CLICKED(IDC_BUTTON_UNIFIEDDIFF, &CFormatPatchDlg::OnBnClickedButtonUnifieddiff)
END_MESSAGE_MAP()

BOOL CFormatPatchDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_RADIO_SINCE);
	AdjustControlSize(IDC_RADIO_NUM);
	AdjustControlSize(IDC_RADIO_RANGE);
	AdjustControlSize(IDC_CHECK_SENDMAIL);
	AdjustControlSize(IDC_CHECK_NOPREFIX);

	AddAnchor(IDC_GROUP_DIR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_DIR,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_DIR, TOP_RIGHT);

	AddAnchor(IDC_GROUP_VERSION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_SINCE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDIT_NUM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPIN_NUM, TOP_RIGHT);

	AddAnchor(IDC_COMBOBOXEX_FROM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TO, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_BUTTON_FROM,  TOP_RIGHT);
	AddAnchor(IDC_BUTTON_TO,	TOP_RIGHT);
	AddAnchor(IDC_CHECK_SENDMAIL,BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_NOPREFIX, BOTTOM_LEFT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_REF,TOP_RIGHT);

	this->AddOthersToAnchor();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_cDir.SetPathHistory(TRUE);
	m_cDir.LoadHistory(L"Software\\TortoiseGit\\History\\FormatPatchURLS", L"path");
	m_cDir.AddString(g_Git.m_CurrentDir);

	STRING_VECTOR list;
	g_Git.GetBranchList(list, nullptr, CGit::BRANCH_ALL_F);
	m_cSince.SetMaxHistoryItems(static_cast<int>(list.size()));
	m_cSince.SetList(list);

	if (!m_Since.IsEmpty())
		m_cSince.SetWindowText(m_Since);
	else
		m_cSince.SetWindowText(static_cast<CString>(m_regSince));

	m_cFrom.LoadHistory(L"Software\\TortoiseGit\\History\\FormatPatchFromURLS", L"ver");
	m_cFrom.SetCurSel(0);

	if(!m_From.IsEmpty())
		m_cFrom.SetWindowText(m_From);

	m_cTo.LoadHistory(L"Software\\TortoiseGit\\History\\FormatPatchToURLS", L"ver");
	m_cTo.SetCurSel(0);

	if(!m_To.IsEmpty())
		m_cTo.SetWindowText(m_To);

	m_spinNum.SetRange32(1, INT_MAX);
	this->CheckRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE,this->m_Radio);

	OnBnClickedRadio();

	if (g_Git.IsInitRepos())
		DialogEnableWindow(IDOK, FALSE);

	EnableSaveRestore(L"FormatPatchDlg");
	return TRUE;
}
// CFormatPatchDlg message handlers

void CFormatPatchDlg::OnBnClickedButtonDir()
{
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;
	this->UpdateData(TRUE);
	strCloneDirectory=m_Dir;
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK)
	{
		m_Dir=strCloneDirectory;
		this->UpdateData(FALSE);
	}
}

void CFormatPatchDlg::OnBnClickedButtonFrom()
{
	CLogDlg dlg;
	CString revision;
	m_cFrom.GetWindowText(revision);
	dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	dlg.ShowWorkingTreeChanges(false);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
	{
		m_cFrom.AddString(dlg.GetSelectedHash().at(0).ToString());
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_RANGE);
		OnBnClickedRadio();
	}
}

void CFormatPatchDlg::OnBnClickedButtonTo()
{
	CLogDlg dlg;
	CString revision;
	m_cTo.GetWindowText(revision);
	dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
	{
		m_cTo.AddString(dlg.GetSelectedHash().at(0).ToString());
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_RANGE);
		OnBnClickedRadio();
	}
}

void CFormatPatchDlg::OnBnClickedOk()
{
	m_cDir.SaveHistory();
	m_cFrom.SaveHistory();
	m_cTo.SaveHistory();
	this->UpdateData(TRUE);
	this->m_Radio=GetCheckedRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE);

	if (m_Radio == IDC_RADIO_SINCE && !m_Since.IsEmpty())
		m_regSince = m_Since;

	m_regSendMail=this->m_bSendMail;
	m_regNoPrefix = m_bNoPrefix;
	OnOK();
}

void CFormatPatchDlg::OnBnClickedRadio()
{
	int radio=this->GetCheckedRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE);
	m_cSince.EnableWindow(FALSE);
	m_cNum.EnableWindow(FALSE);
	m_cFrom.EnableWindow(FALSE);
	m_cTo.EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_REF)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_FROM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TO)->EnableWindow(FALSE);
	switch(radio)
	{
	case IDC_RADIO_SINCE:
		m_cSince.EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_REF)->EnableWindow(TRUE);
		break;
	case IDC_RADIO_NUM:
		m_cNum.EnableWindow(TRUE);
		break;
	case IDC_RADIO_RANGE:
		m_cFrom.EnableWindow(TRUE);
		m_cTo.EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_FROM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_TO)->EnableWindow(TRUE);
		break;
	}
}

void CFormatPatchDlg::OnBnClickedButtonRef()
{
	if (CBrowseRefsDlg::PickRefForCombo(m_cSince, gPickRef_NoTag))
	{
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_SINCE);
		OnBnClickedRadio();
	}
}
void CFormatPatchDlg::OnBnClickedButtonUnifieddiff()
{
	UpdateData(TRUE);
	m_regNoPrefix = m_bNoPrefix;
	CAppUtils::StartShowUnifiedDiff(m_hWnd, CTGitPath(), GitRev::GetHead(), CTGitPath(), GitRev::GetWorkingCopy(), !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, false, false, !!m_bNoPrefix);
}
