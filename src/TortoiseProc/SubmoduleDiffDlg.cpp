// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2017, 2019 - TortoiseGit

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
#include "Git.h"
#include "AppUtils.h"
#include "SubmoduleDiffDlg.h"
#include "LoglistCommonResource.h"

IMPLEMENT_DYNAMIC(CSubmoduleDiffDlg, CHorizontalResizableStandAloneDialog)
CSubmoduleDiffDlg::CSubmoduleDiffDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CSubmoduleDiffDlg::IDD, pParent)
	, m_bToIsWorkingCopy(false)
	, m_bFromOK(false)
	, m_bToOK(false)
	, m_bDirty(false)
	, m_nChangeType(CGitDiff::Unknown)
	, m_bRefresh(false)
{
}

CSubmoduleDiffDlg::~CSubmoduleDiffDlg()
{
}

void CSubmoduleDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FROMSUBJECT, m_sFromSubject);
	DDX_Text(pDX, IDC_TOSUBJECT, m_sToSubject);
	DDX_Control(pDX, IDC_SHOW_DIFF, m_ctrlShowDiffBtn);
	GetDlgItem(IDC_FROMHASH)->SetWindowText(m_sFromHash.ToString());
	if (m_bDirty)
		GetDlgItem(IDC_TOHASH)->SetWindowText(m_sToHash.ToString() + L"-dirty");
	else
		GetDlgItem(IDC_TOHASH)->SetWindowText(m_sToHash.ToString());
}

BEGIN_MESSAGE_MAP(CSubmoduleDiffDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, &CSubmoduleDiffDlg::OnBnClickedLog)
	ON_BN_CLICKED(IDC_LOG2, &CSubmoduleDiffDlg::OnBnClickedLog2)
	ON_BN_CLICKED(IDC_SHOW_DIFF, &CSubmoduleDiffDlg::OnBnClickedShowDiff)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CSubmoduleDiffDlg::OnBnClickedButtonUpdate)
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
	AddAnchor(IDC_BUTTON_UPDATE, TOP_RIGHT);
	AddAnchor(IDC_SHOW_DIFF, TOP_RIGHT);

	AddAnchor(IDC_FROMHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FROMSUBJECT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHANGETYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOSUBJECT, TOP_LEFT, TOP_RIGHT);

	EnableSaveRestore(L"SubmoduleDiffDlg");

	if (m_bToIsWorkingCopy)
	{
		CString toGroup;
		GetDlgItem(IDC_TOGROUP)->GetWindowText(toGroup);
		toGroup += L" (" + CString(MAKEINTRESOURCE(IDS_WORKING_TREE)) +  L')';
		GetDlgItem(IDC_TOGROUP)->SetWindowText(toGroup);
	}

	CString fsPath = m_sPath;
	fsPath.Replace(L'\\', L'/');
	CString title = L"Submodule \"" + fsPath + L'"';
	GetDlgItem(IDC_SUBMODULEDIFFTITLE)->SetWindowText(title);

	UpdateData(FALSE);

	CString changeTypeTable[] =
	{
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_UNKNOWN)),
		CString(MAKEINTRESOURCE(IDS_SUBMODULEDIFF_IDENTICAL)),
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
	if (m_bDirty && m_nChangeType != CGitDiff::Unknown)
	{
		m_ctrlShowDiffBtn.AddEntry(MAKEINTRESOURCE(IDS_PROC_SHOWDIFF));
		m_ctrlShowDiffBtn.AddEntry(MAKEINTRESOURCE(IDS_LOG_POPUP_COMPARE));
	}

	GetDlgItem(IDC_BUTTON_UPDATE)->ShowWindow(!m_bFromOK || !m_bToOK || m_bToIsWorkingCopy ? SW_SHOW : SW_HIDE);

	return FALSE;
}


HBRUSH CSubmoduleDiffDlg::GetInvalidBrush(CDC* pDC)
{
	pDC->SetBkColor(RGB(255, 0, 0));
	pDC->SetTextColor(RGB(255, 255, 255));
	return CreateSolidBrush(RGB(255, 0, 0));
}

HBRUSH CSubmoduleDiffDlg::GetChangeTypeBrush(CDC* pDC, const CGitDiff::ChangeType& changeType)
{
	if (changeType == CGitDiff::FastForward)
	{
		// light green
		pDC->SetBkColor(RGB(211, 249, 154));
		pDC->SetTextColor(RGB(0, 0, 0));
		return CreateSolidBrush(RGB(211, 249, 154));
	}
	if (changeType == CGitDiff::Rewind)
	{
		// pink
		pDC->SetBkColor(RGB(249, 199, 229));
		pDC->SetTextColor(RGB(0, 0, 0));
		return CreateSolidBrush(RGB(249, 199, 229));
	}
	if (changeType == CGitDiff::NewerTime)
	{
		// light blue
		pDC->SetBkColor(RGB(176, 223, 244));
		pDC->SetTextColor(RGB(0, 0, 0));
		return CreateSolidBrush(RGB(176, 223, 244));
	}
	if (changeType == CGitDiff::OlderTime)
	{
		// light orange
		pDC->SetBkColor(RGB(244, 207, 159));
		pDC->SetTextColor(RGB(0, 0, 0));
		return CreateSolidBrush(RGB(244, 207, 159));
	}

	// light gray
	pDC->SetBkColor(RGB(222, 222, 222));
	pDC->SetTextColor(RGB(0, 0, 0));
	return CreateSolidBrush(RGB(222, 222, 222));
}

HBRUSH CSubmoduleDiffDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		if (pWnd->GetDlgCtrlID() == IDC_FROMSUBJECT && !m_bFromOK)
			return GetInvalidBrush(pDC);

		if (pWnd->GetDlgCtrlID() == IDC_TOSUBJECT && !m_bToOK)
			return GetInvalidBrush(pDC);

		if (pWnd->GetDlgCtrlID() == IDC_TOHASH && m_bDirty)
		{
			pDC->SetBkColor(RGB(255, 255, 0));
			pDC->SetTextColor(RGB(255, 0, 0));
			return CreateSolidBrush(RGB(255, 255, 0));
		}

		if (pWnd->GetDlgCtrlID() == IDC_CHANGETYPE && m_nChangeType != CGitDiff::Identical)
			return GetChangeTypeBrush(pDC, m_nChangeType);
	}

	return CHorizontalResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

BOOL CSubmoduleDiffDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F5)
	{
		m_bRefresh = true;
		EndDialog(0);
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CSubmoduleDiffDlg::SetDiff(CString path, bool toIsWorkingCopy, const CGitHash& fromHash, CString fromSubject, bool fromOK, const CGitHash& toHash, CString toSubject, bool toOK, bool dirty, CGitDiff::ChangeType changeType)
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
	sCmd.Format(L"/command:log /path:\"%s\" /endrev:%s", static_cast<LPCTSTR>(g_Git.CombinePath(m_sPath)), static_cast<LPCTSTR>(hash));
	CAppUtils::RunTortoiseGitProc(sCmd, false, false);
}

void CSubmoduleDiffDlg::OnBnClickedLog()
{
	ShowLog(m_sFromHash.ToString());
}

void CSubmoduleDiffDlg::OnBnClickedLog2()
{
	ShowLog(m_sToHash.ToString());
}

void CSubmoduleDiffDlg::OnBnClickedShowDiff()
{
	CString sCmd;
	sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:%s /revision2:%s", static_cast<LPCTSTR>(g_Git.CombinePath(m_sPath)), static_cast<LPCTSTR>(m_sFromHash.ToString()), ((m_bDirty && m_nChangeType == CGitDiff::Unknown) || m_ctrlShowDiffBtn.GetCurrentEntry() == 1) ? GIT_REV_ZERO : static_cast<LPCTSTR>(m_sToHash.ToString()));

	if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		sCmd += L" /alternative";

	CAppUtils::RunTortoiseGitProc(sCmd, false, false);
}

void CSubmoduleDiffDlg::OnBnClickedButtonUpdate()
{
	CString sCmd;
	sCmd.Format(L"/command:subupdate /bkpath:\"%s\" /selectedpath:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(m_sPath));
	CAppUtils::RunTortoiseGitProc(sCmd);
}
