// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2019 - TortoiseGit

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
#include "SubmoduleResolveConflictDlg.h"
#include "LoglistCommonResource.h"
#include "SubmoduleDiffDlg.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSubmoduleResolveConflictDlg, CHorizontalResizableStandAloneDialog)
CSubmoduleResolveConflictDlg::CSubmoduleResolveConflictDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CSubmoduleResolveConflictDlg::IDD, pParent)
	, m_bBaseOK(false)
	, m_bMineOK(false)
	, m_bTheirsOK(false)
	, m_nChangeTypeMine(CGitDiff::Unknown)
	, m_nChangeTypeTheirs(CGitDiff::Unknown)
	, m_bRevertTheirMy(false)
	, m_bResolved(false)
{
}

CSubmoduleResolveConflictDlg::~CSubmoduleResolveConflictDlg()
{
}

void CSubmoduleResolveConflictDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASESUBJECT, m_sBaseSubject);
	DDX_Text(pDX, IDC_MINESUBJECT, m_sMineSubject);
	DDX_Text(pDX, IDC_THEIRSSUBJECT, m_sTheirsSubject);
	GetDlgItem(IDC_BASEHASH)->SetWindowText(m_sBaseHash.ToString());
	GetDlgItem(IDC_MINEHASH)->SetWindowText(m_sMineHash.ToString());
	GetDlgItem(IDC_THEIRSHASH)->SetWindowText(m_sTheirsHash.ToString());
}

BEGIN_MESSAGE_MAP(CSubmoduleResolveConflictDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, &OnBnClickedLog)
	ON_BN_CLICKED(IDC_LOG2, &OnBnClickedLog2)
	ON_BN_CLICKED(IDC_LOG3, &OnBnClickedLog3)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE2, &CSubmoduleResolveConflictDlg::OnBnClickedButtonUpdate2)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE3, &CSubmoduleResolveConflictDlg::OnBnClickedButtonUpdate3)
END_MESSAGE_MAP()

BOOL CSubmoduleResolveConflictDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AddAnchor(IDC_SUBMODULEDIFFTITLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_REVISION, TOP_LEFT);
	AddAnchor(IDC_STATIC_REVISION2, TOP_LEFT);
	AddAnchor(IDC_STATIC_REVISION3, TOP_LEFT);
	AddAnchor(IDC_STATIC_SUBJECT, TOP_LEFT);
	AddAnchor(IDC_STATIC_SUBJECT2, TOP_LEFT);
	AddAnchor(IDC_STATIC_SUBJECT3, TOP_LEFT);
	AddAnchor(IDC_STATIC_CHANGETYPE, TOP_LEFT);
	AddAnchor(IDC_STATIC_CHANGETYPE2, TOP_LEFT);

	AddAnchor(IDC_FROMGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOGROUP2, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_LOG, TOP_RIGHT);
	AddAnchor(IDC_LOG2, TOP_RIGHT);
	AddAnchor(IDC_LOG3, TOP_RIGHT);

	AddAnchor(IDC_BASEHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BASESUBJECT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MINECHANGETYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MINEHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MINESUBJECT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_THEIRSCHANGETYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_THEIRSHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_THEIRSSUBJECT, TOP_LEFT, TOP_RIGHT);

	EnableSaveRestore(L"SubmoduleResolveConflictDlg");

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
	GetDlgItem(IDC_MINECHANGETYPE)->SetWindowText(changeTypeTable[m_nChangeTypeMine]);
	GetDlgItem(IDC_THEIRSCHANGETYPE)->SetWindowText(changeTypeTable[m_nChangeTypeTheirs]);

	GetDlgItem(IDC_FROMGROUP)->SetWindowText(m_sBaseTitle);
	GetDlgItem(IDC_TOGROUP)->SetWindowText(m_sMineTitle);
	GetDlgItem(IDC_TOGROUP2)->SetWindowText(m_sTheirsTitle);

	DialogEnableWindow(IDC_LOG, m_bBaseOK);
	DialogEnableWindow(IDC_LOG2, m_bMineOK && m_nChangeTypeMine != CGitDiff::DeleteSubmodule);
	DialogEnableWindow(IDC_LOG3, m_bTheirsOK && m_nChangeTypeTheirs != CGitDiff::DeleteSubmodule);

	return FALSE;
}

HBRUSH CSubmoduleResolveConflictDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		if (pWnd->GetDlgCtrlID() == IDC_BASESUBJECT && !m_bBaseOK)
			return CSubmoduleDiffDlg::GetInvalidBrush(pDC);

		if (pWnd->GetDlgCtrlID() == IDC_MINESUBJECT && !m_bMineOK)
			return CSubmoduleDiffDlg::GetInvalidBrush(pDC);

		if (pWnd->GetDlgCtrlID() == IDC_THEIRSSUBJECT && !m_bTheirsOK)
			return CSubmoduleDiffDlg::GetInvalidBrush(pDC);

		if (pWnd->GetDlgCtrlID() == IDC_MINECHANGETYPE && m_nChangeTypeMine != CGitDiff::Identical)
			return CSubmoduleDiffDlg::GetChangeTypeBrush(pDC, m_nChangeTypeMine);

		if (pWnd->GetDlgCtrlID() == IDC_THEIRSCHANGETYPE && m_nChangeTypeTheirs != CGitDiff::Identical)
			return CSubmoduleDiffDlg::GetChangeTypeBrush(pDC, m_nChangeTypeTheirs);
	}

	return CHorizontalResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSubmoduleResolveConflictDlg::SetDiff(const CString& path, bool revertTheirMy, const CString& baseTitle, const CString& mineTitle, const CString& theirsTitle, const CGitHash& baseHash, const CString& baseSubject, bool baseOK, const CGitHash& mineHash, const CString& mineSubject, bool mineOK, CGitDiff::ChangeType mineChangeType, const CGitHash& theirsHash, const CString& theirsSubject, bool theirsOK, CGitDiff::ChangeType theirsChangeType)
{
	m_sPath = path;

	m_sBaseTitle = baseTitle;
	m_sBaseHash = baseHash;
	m_sBaseSubject = baseSubject;
	m_bBaseOK = baseOK;

	m_bRevertTheirMy = revertTheirMy;

	if (!m_bRevertTheirMy)
	{
		m_sMineTitle = mineTitle;
		m_sMineHash = mineHash;
		m_sMineSubject = mineSubject;
		m_bMineOK = mineOK;
		m_nChangeTypeMine = mineChangeType;

		m_sTheirsTitle = theirsTitle;
		m_sTheirsHash = theirsHash;
		m_sTheirsSubject = theirsSubject;
		m_bTheirsOK = theirsOK;
		m_nChangeTypeTheirs = theirsChangeType;
	}
	else
	{
		m_sMineTitle = theirsTitle;
		m_sMineHash = theirsHash;
		m_sMineSubject = theirsSubject;
		m_bMineOK = theirsOK;
		m_nChangeTypeMine = theirsChangeType;

		m_sTheirsTitle = mineTitle;
		m_sTheirsHash = mineHash;
		m_sTheirsSubject = mineSubject;
		m_bTheirsOK = mineOK;
		m_nChangeTypeTheirs = mineChangeType;
	}
}

void CSubmoduleResolveConflictDlg::ShowLog(CString hash)
{
	CString sCmd;
	sCmd.Format(L"/command:log /path:\"%s\" /endrev:%s", static_cast<LPCTSTR>(g_Git.CombinePath(m_sPath)), static_cast<LPCTSTR>(hash));
	CAppUtils::RunTortoiseGitProc(sCmd, false, false);
}

void CSubmoduleResolveConflictDlg::OnBnClickedLog()
{
	ShowLog(m_sBaseHash.ToString());
}

void CSubmoduleResolveConflictDlg::OnBnClickedLog2()
{
	ShowLog(m_sMineHash.ToString());
}

void CSubmoduleResolveConflictDlg::OnBnClickedLog3()
{
	ShowLog(m_sTheirsHash.ToString());
}

void CSubmoduleResolveConflictDlg::Resolve(const CString& path, bool useMine)
{
	if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO) != IDYES)
		return;

	CTGitPath gitpath(path);
	gitpath.m_Action = CTGitPath::LOGACTIONS_UNMERGED;
	if (CAppUtils::ResolveConflict(GetSafeHwnd(), gitpath, useMine ? CAppUtils::RESOLVE_WITH_MINE : CAppUtils::RESOLVE_WITH_THEIRS) == 0)
	{
		m_bResolved = true;
		EndDialog(0);
	}
}

void CSubmoduleResolveConflictDlg::OnBnClickedButtonUpdate2()
{
	Resolve(m_sPath, !m_bRevertTheirMy);
}

void CSubmoduleResolveConflictDlg::OnBnClickedButtonUpdate3()
{
	Resolve(m_sPath, m_bRevertTheirMy);
}
