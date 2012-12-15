// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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
#include "SubmoduleDiffDlg.h"
#include "LoglistCommonResource.h"

IMPLEMENT_DYNAMIC(CSubmoduleDiffDlg, CHorizontalResizableStandAloneDialog)
CSubmoduleDiffDlg::CSubmoduleDiffDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CSubmoduleDiffDlg::IDD, pParent)
	, m_bFromOK(false)
	, m_bToOK(false)
	, m_bDirty(false)
{
}

CSubmoduleDiffDlg::~CSubmoduleDiffDlg()
{
}

void CSubmoduleDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FROMHASH, m_sFromHash);
	DDX_Text(pDX, IDC_FROMSUBJECT, m_sFromSubject);
	DDX_Text(pDX, IDC_TOHASH, m_sToHash);
	DDX_Text(pDX, IDC_TOSUBJECT, m_sToSubject);
	DDX_Control(pDX, IDC_SHOW_DIFF, m_ctrlShowDiffBtn);
}

BEGIN_MESSAGE_MAP(CSubmoduleDiffDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, &CSubmoduleDiffDlg::OnBnClickedLog)
	ON_BN_CLICKED(IDC_LOG2, &CSubmoduleDiffDlg::OnBnClickedLog2)
	ON_BN_CLICKED(IDC_SHOW_DIFF, &CSubmoduleDiffDlg::OnBnClickedShowDiff)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CSubmoduleDiffDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AddAnchor(IDC_SUBMODULEDIFFTITLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_REVISION, TOP_LEFT);
	AddAnchor(IDC_STATIC_REVISION2, TOP_LEFT);
	AddAnchor(IDC_STATIC_SUBJECT, TOP_LEFT);
	AddAnchor(IDC_STATIC_SUBJECT2, TOP_LEFT);
	AddAnchor(IDC_STATIC_CHANGETYPE, TOP_LEFT);

	AddAnchor(IDC_FROMGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOGROUP, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_LOG, TOP_RIGHT);
	AddAnchor(IDC_LOG2, TOP_RIGHT);
	AddAnchor(IDC_SHOW_DIFF, TOP_RIGHT);

	AddAnchor(IDC_FROMHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FROMSUBJECT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHANGETYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOSUBJECT, TOP_LEFT, TOP_RIGHT);

	EnableSaveRestore(_T("SubmoduleDiffDlg"));

	if (m_bToIsWorkingCopy)
	{
		CString toGroup;
		GetDlgItem(IDC_TOGROUP)->GetWindowText(toGroup);
		toGroup += _T(" (") + CString(MAKEINTRESOURCE(IDS_git_DEPTH_WORKING)) +  _T(")");
		GetDlgItem(IDC_TOGROUP)->SetWindowText(toGroup);
	}

	CString title = _T("Submodule \"") + m_sPath + _T("\"");
	GetDlgItem(IDC_SUBMODULEDIFFTITLE)->SetWindowText(title);

	UpdateData(FALSE);
	if (m_bDirty)
		GetDlgItem(IDC_TOHASH)->SetWindowText(m_sToHash + _T("-dirty"));

	CString changeTypeTable[] =
	{
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_UNKNOWN)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_NEWSUBMODULE)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_DELETESUBMODULE)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_FASTFORWARD)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_REWIND)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_NEWERTIME)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_OLDERTIME)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_SAMETIME))
	};
	GetDlgItem(IDC_CHANGETYPE)->SetWindowText(changeTypeTable[m_nChangeType]);

	DialogEnableWindow(IDC_SHOW_DIFF, m_bFromOK && m_bToOK);
	if (m_bDirty && m_nChangeType != Unknown)
	{
		m_ctrlShowDiffBtn.AddEntry(MAKEINTRESOURCE(IDS_PROC_SHOWDIFF));
		m_ctrlShowDiffBtn.AddEntry(MAKEINTRESOURCE(IDS_LOG_POPUP_COMPARE));
	}

	return FALSE;
}

HBRUSH CSubmoduleDiffDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_FROMSUBJECT && nCtlColor == CTLCOLOR_STATIC && !m_bFromOK)
	{
		pDC->SetBkColor(RGB(255, 0, 0));
		pDC->SetTextColor(RGB(255, 255, 255));
		return CreateSolidBrush(RGB(255, 0, 0));
	}

	if (pWnd->GetDlgCtrlID() == IDC_TOSUBJECT && nCtlColor == CTLCOLOR_STATIC && !m_bToOK)
	{
		pDC->SetBkColor(RGB(255, 0, 0));
		pDC->SetTextColor(RGB(255, 255, 255));
		return CreateSolidBrush(RGB(255, 0, 0));
	}

	if (pWnd->GetDlgCtrlID() == IDC_TOHASH && nCtlColor == CTLCOLOR_STATIC && m_bDirty)
	{
		pDC->SetBkColor(RGB(255, 255, 0));
		pDC->SetTextColor(RGB(255, 0, 0));
		return CreateSolidBrush(RGB(255, 255, 0));
	}

	if (pWnd->GetDlgCtrlID() == IDC_CHANGETYPE && nCtlColor == CTLCOLOR_STATIC)
	{
		if (m_nChangeType == FastForward)
		{
			// light green
			pDC->SetBkColor(RGB(211, 249, 154));
			pDC->SetTextColor(RGB(0, 0, 0));
			return CreateSolidBrush(RGB(211, 249, 154));
		}
		else if (m_nChangeType == Rewind)
		{
			// pink
			pDC->SetBkColor(RGB(249, 199, 229));
			pDC->SetTextColor(RGB(0, 0, 0));
			return CreateSolidBrush(RGB(249, 199, 229));
		}
		else if (m_nChangeType == NewerTime)
		{
			// light blue
			pDC->SetBkColor(RGB(176, 223, 244));
			pDC->SetTextColor(RGB(0, 0, 0));
			return CreateSolidBrush(RGB(176, 223, 244));
		}
		else if (m_nChangeType == OlderTime)
		{
			// light orange
			pDC->SetBkColor(RGB(244, 207, 159));
			pDC->SetTextColor(RGB(0, 0, 0));
			return CreateSolidBrush(RGB(244, 207, 159));
		}
		else
		{
			// light gray
			pDC->SetBkColor(RGB(222, 222, 222));
			pDC->SetTextColor(RGB(0, 0, 0));
			return CreateSolidBrush(RGB(222, 222, 222));
		}
	}

	return CHorizontalResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSubmoduleDiffDlg::SetDiff(CString path, bool toIsWorkingCopy, CString fromHash, CString fromSubject, bool fromOK, CString toHash, CString toSubject, bool toOK, bool dirty, ChangeType changeType)
{
	m_bToIsWorkingCopy = toIsWorkingCopy;

	m_sPath = path;

	m_sFromHash = fromHash;
	m_sFromSubject = fromSubject;
	m_bFromOK = fromOK;
	m_sToHash = toHash;
	m_sToSubject = toSubject;
	m_bToOK = toOK;
	m_bDirty = dirty;
	m_nChangeType = changeType;
}

void CSubmoduleDiffDlg::ShowLog(CString hash)
{
	CString sCmd;
	sCmd.Format(_T("/command:log /path:\"%s\" /endrev:%s"), g_Git.m_CurrentDir + _T("\\") + m_sPath, hash);
	CAppUtils::RunTortoiseProc(sCmd);
}

void CSubmoduleDiffDlg::OnBnClickedLog()
{
	ShowLog(m_sFromHash);
}

void CSubmoduleDiffDlg::OnBnClickedLog2()
{
	ShowLog(m_sToHash);
}

void CSubmoduleDiffDlg::OnBnClickedShowDiff()
{
	CString sCmd;
	sCmd.Format(_T("/command:showcompare /path:\"%s\" /revision1:%s /revision2:%s"), g_Git.m_CurrentDir + _T("\\") + m_sPath, m_sFromHash, ((m_bDirty && m_nChangeType == Unknown) || m_ctrlShowDiffBtn.GetCurrentEntry() == 1) ? GIT_REV_ZERO : m_sToHash);
	CAppUtils::RunTortoiseProc(sCmd);
}
