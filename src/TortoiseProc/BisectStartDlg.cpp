// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 Sven Strickroth, <email@cs-ware.de>

// with code of PullFetchDlg.cpp

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
#include "BisectStartDlg.h"
#include "git.h"
#include "LogDlg.h"
#include "MessageBox.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CBisectStartDlg, CHorizontalResizableStandAloneDialog)

CBisectStartDlg::CBisectStartDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CBisectStartDlg::IDD, pParent)
{
}

CBisectStartDlg::~CBisectStartDlg()
{
}

void CBisectStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_GOOD, m_cLastGoodRevision);
	DDX_Control(pDX, IDC_COMBOBOXEX_BAD, m_cFirstBadRevision);
}

BEGIN_MESSAGE_MAP(CBisectStartDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CBisectStartDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_GOOD, &CBisectStartDlg::OnBnClickedButtonGood)
	ON_BN_CLICKED(IDC_BUTTON_BAD, &CBisectStartDlg::OnBnClickedButtonBad)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_GOOD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_BAD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_GOOD, &CBisectStartDlg::OnChangedRevision)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_BAD, &CBisectStartDlg::OnChangedRevision)
END_MESSAGE_MAP()

BOOL CBisectStartDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddAnchor(IDC_BUTTON_GOOD, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BAD, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_GOOD, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_BAD, TOP_LEFT, TOP_RIGHT);

	EnableSaveRestore(_T("BisectStartDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	STRING_VECTOR list;
	int current;
	g_Git.GetBranchList(list, &current, CGit::BRANCH_ALL);
	m_cLastGoodRevision.SetMaxHistoryItems(0x7FFFFFFF);
	m_cFirstBadRevision.SetMaxHistoryItems(0x7FFFFFFF);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		m_cLastGoodRevision.AddString(list[i]);
		m_cFirstBadRevision.AddString(list[i]);
	}
	list.clear();
	g_Git.GetTagList(list);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		m_cLastGoodRevision.AddString(list[i]);
		m_cFirstBadRevision.AddString(list[i]);
	}

	if (m_sLastGood.IsEmpty())
		m_cLastGoodRevision.SetCurSel(-1);
	else
		m_cLastGoodRevision.SetWindowTextW(m_sLastGood);
	if (m_sFirstBad.IsEmpty())
		m_cFirstBadRevision.SetCurSel(current);
	else
		m_cFirstBadRevision.SetWindowTextW(m_sFirstBad);

	this->UpdateData(FALSE);

	// EnDisable OK Button
	OnChangedRevision();

	return TRUE;
}

void CBisectStartDlg::OnChangedRevision()
{
	this->GetDlgItem(IDOK)->EnableWindow(!(m_cLastGoodRevision.GetString().Trim().IsEmpty() || m_cFirstBadRevision.GetString().Trim().IsEmpty()));
}

void CBisectStartDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	m_LastGoodRevision = m_cLastGoodRevision.GetString().Trim();
	m_FirstBadRevision = m_cFirstBadRevision.GetString().Trim();

	if(m_FirstBadRevision.Find(_T("remotes/")) == 0)
		m_FirstBadRevision = m_FirstBadRevision.Mid(8);

	if(m_FirstBadRevision.Find(_T("remotes/")) == 0)
		m_FirstBadRevision = m_FirstBadRevision.Mid(8);

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CBisectStartDlg::OnBnClickedButtonGood()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK)
	{
		// get selected hash if any
		CString selectedHash = dlg.GetSelectedHash();
		// load into window, do this even if empty so that it is clear that nothing has been selected
		m_cLastGoodRevision.SetWindowText(selectedHash);
		OnChangedRevision();
	}
}

void CBisectStartDlg::OnBnClickedButtonBad()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK)
	{
		// get selected hash if any
		CString selectedHash = dlg.GetSelectedHash();
		// load into window, do this even if empty so that it is clear that nothing has been selected
		m_cFirstBadRevision.SetWindowText(selectedHash);
		OnChangedRevision();
	}
}
