// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "ConflictResolveDlg.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "TempFile.h"

IMPLEMENT_DYNAMIC(CConflictResolveDlg, CResizableStandAloneDialog)

CConflictResolveDlg::CConflictResolveDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CConflictResolveDlg::IDD, pParent)
	, m_pConflictDescription(NULL)
	, m_choice(svn_wc_conflict_choose_postpone)
	, m_bCancelled(false)
{

}

CConflictResolveDlg::~CConflictResolveDlg()
{
}

void CConflictResolveDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConflictResolveDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_USELOCAL, &CConflictResolveDlg::OnBnClickedUselocal)
	ON_BN_CLICKED(IDC_USEREPO, &CConflictResolveDlg::OnBnClickedUserepo)
	ON_BN_CLICKED(IDC_EDITCONFLICT, &CConflictResolveDlg::OnBnClickedEditconflict)
	ON_BN_CLICKED(IDC_RESOLVED, &CConflictResolveDlg::OnBnClickedResolved)
	ON_BN_CLICKED(IDC_RESOLVEALLLATER, &CConflictResolveDlg::OnBnClickedResolvealllater)
	ON_BN_CLICKED(IDHELP, &CConflictResolveDlg::OnBnClickedHelp)
	ON_BN_CLICKED(IDC_ABORT, &CConflictResolveDlg::OnBnClickedAbort)
END_MESSAGE_MAP()


// CConflictResolveDlg message handlers

BOOL CConflictResolveDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// without a conflict description, this dialog is useless.
	ASSERT(m_pConflictDescription);

	CString filename = CUnicodeUtils::GetUnicode(m_pConflictDescription->path);
	filename = CPathUtils::GetFileNameFromPath(filename);

	CString sInfoText;
	CString sActionText;
	CString sReasonText;
	switch (m_pConflictDescription->action)
	{
	case svn_wc_conflict_action_edit:
		if (m_pConflictDescription->property_name)
			sActionText.Format(IDS_EDITCONFLICT_PROP_ACTIONINFO_MODIFY,
				(LPCTSTR)CUnicodeUtils::GetUnicode(m_pConflictDescription->property_name),
				(LPCTSTR)filename);
		else
			sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_MODIFY, (LPCTSTR)filename);
		break;
	case svn_wc_conflict_action_add:
		if (m_pConflictDescription->property_name)
			sActionText.Format(IDS_EDITCONFLICT_PROP_ACTIONINFO_ADD,
				(LPCTSTR)CUnicodeUtils::GetUnicode(m_pConflictDescription->property_name),
				(LPCTSTR)filename);
		else
			sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_ADD, (LPCTSTR)filename);
		break;
	case svn_wc_conflict_action_delete:
		if (m_pConflictDescription->property_name)
			sActionText.Format(IDS_EDITCONFLICT_PROP_ACTIONINFO_DELETE,
				(LPCTSTR)CUnicodeUtils::GetUnicode(m_pConflictDescription->property_name),
				(LPCTSTR)filename);
		else
			sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_DELETE, (LPCTSTR)filename);
		break;
	default:
		break;
	}

	switch (m_pConflictDescription->reason)
	{
	case svn_wc_conflict_reason_edited:
		sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_EDITED);
		break;
	case svn_wc_conflict_reason_obstructed:
		sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_OBSTRUCTED);
		break;
	case svn_wc_conflict_reason_deleted:
		sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_DELETED);
		break;
	case svn_wc_conflict_reason_missing:
		sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_MISSING);
		break;
	case svn_wc_conflict_reason_unversioned:
		sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_UNVERSIONED);
		break;
	default:
		break;
	}

	sInfoText = sActionText + _T(" ") + sReasonText;
	SetDlgItemText(IDC_INFOLABEL, sInfoText);

	// if we deal with a binary file, editing the conflict isn't possible
	// because the 'merged_file' is not used and Subversion therefore can't
	// use it as the result of the edit.
	if (m_pConflictDescription->is_binary)
	{
		GetDlgItem(IDC_RESOLVELABEL)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITCONFLICT)->EnableWindow(FALSE);
		GetDlgItem(IDC_RESOLVED)->EnableWindow(FALSE);
	}

	// the "resolved" button must not be enabled as long as the user hasn't used
	// the "edit" button.
	GetDlgItem(IDC_RESOLVED)->EnableWindow(FALSE);

	m_bCancelled = false;

	AddAnchor(IDC_INFOLABEL, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHOOSELABEL, BOTTOM_LEFT);
	AddAnchor(IDC_USELOCAL, BOTTOM_LEFT);
	AddAnchor(IDC_USEREPO, BOTTOM_RIGHT);
	AddAnchor(IDC_ORLABEL, BOTTOM_LEFT);
	AddAnchor(IDC_RESOLVELABEL, BOTTOM_LEFT);
	AddAnchor(IDC_EDITCONFLICT, BOTTOM_LEFT);
	AddAnchor(IDC_RESOLVED, BOTTOM_RIGHT);
	AddAnchor(IDC_ORLABEL2, BOTTOM_LEFT);
	AddAnchor(IDC_LEAVELABEL, BOTTOM_LEFT);
	AddAnchor(IDCANCEL, BOTTOM_LEFT);
	AddAnchor(IDC_RESOLVEALLLATER, BOTTOM_RIGHT);
	AddAnchor(IDC_ABORT, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	return TRUE;
}

void CConflictResolveDlg::OnBnClickedUselocal()
{
	m_choice = svn_wc_conflict_choose_mine_full;
	EndDialog(IDOK);
}

void CConflictResolveDlg::OnBnClickedUserepo()
{
	m_choice = svn_wc_conflict_choose_theirs_full;
	EndDialog(IDOK);
}

void CConflictResolveDlg::OnBnClickedEditconflict()
{
	CString filename, n1, n2, n3;
	if (m_pConflictDescription->property_name)
	{
		filename = CUnicodeUtils::GetUnicode(m_pConflictDescription->property_name);
		n1.Format(IDS_DIFF_PROP_WCNAME, (LPCTSTR)filename);
		n2.Format(IDS_DIFF_PROP_BASENAME, (LPCTSTR)filename);
		n3.Format(IDS_DIFF_PROP_REMOTENAME, (LPCTSTR)filename);
	}
	else
	{
		filename = CUnicodeUtils::GetUnicode(m_pConflictDescription->path);
		filename = CPathUtils::GetFileNameFromPath(filename);
		n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)filename);
		n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)filename);
		n3.Format(IDS_DIFF_REMOTENAME, (LPCTSTR)filename);
	}

	if (m_pConflictDescription->base_file == NULL)
	{
		CAppUtils::DiffFlags flags;
		// no base file, start TortoiseMerge in Two-way diff mode
		CAppUtils::StartExtDiff(CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->their_file)),
			CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->my_file)),
			n3, n1, flags);
	}
	else
	{
		// Subversion segfaults (1.5.1) if the path of the merged file is not a child of the
		// folder where the conflict occurs. That's why we try to use the provided file path first...
		if (m_pConflictDescription->merged_file)
			m_mergedfile = CUnicodeUtils::GetUnicode(m_pConflictDescription->merged_file);
		else
			m_mergedfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->path))).GetWinPath();
		CAppUtils::StartExtMerge(CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->base_file)),
								CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->their_file)),
								CTSVNPath(CUnicodeUtils::GetUnicode(m_pConflictDescription->my_file)),
								CTSVNPath(m_mergedfile),
								n2, n3, n1, CString(), false);
	}

	GetDlgItem(IDC_RESOLVED)->EnableWindow(TRUE);
}

void CConflictResolveDlg::OnBnClickedResolved()
{
	m_choice = svn_wc_conflict_choose_merged;
	if (m_mergedfile.IsEmpty())
		m_mergedfile = CUnicodeUtils::GetUnicode(m_pConflictDescription->my_file);
	EndDialog(IDOK);
}

void CConflictResolveDlg::OnBnClickedResolvealllater()
{
	m_choice = svn_wc_conflict_choose_postpone;
	EndDialog(IDOK);
}

void CConflictResolveDlg::OnCancel()
{
	m_choice = svn_wc_conflict_choose_postpone;

	CResizableStandAloneDialog::OnCancel();
}

void CConflictResolveDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CConflictResolveDlg::OnBnClickedAbort()
{
	m_bCancelled = true;
	m_choice = svn_wc_conflict_choose_postpone;
	CResizableStandAloneDialog::OnCancel();
}
