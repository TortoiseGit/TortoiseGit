// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2011-2013, 2015-2019 - TortoiseGit
// Copyright (C) 2003-2011, 2013 - TortoiseSVN

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
#include "TGitPath.h"
#include "RenameDlg.h"
#include "AppUtils.h"
#include "ControlsBridge.h"
#include "Git.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CRenameDlg, CHorizontalResizableStandAloneDialog)
CRenameDlg::CRenameDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CRenameDlg::IDD, pParent)
	, m_renameRequired(true)
	, m_pInputValidator(nullptr)
	, m_bBalloonVisible(false)
	, m_sBaseDir(g_Git.m_CurrentDir)
{
}

CRenameDlg::~CRenameDlg()
{
}

void CRenameDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_name);
}


BEGIN_MESSAGE_MAP(CRenameDlg, CHorizontalResizableStandAloneDialog)
	ON_EN_SETFOCUS(IDC_NAME, &CRenameDlg::OnEnSetfocusName)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF, &CRenameDlg::OnBnClickedButtonBrowseRef)
END_MESSAGE_MAP()

BOOL CRenameDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	SHAutoComplete(GetDlgItem(IDC_NAME)->m_hWnd, SHACF_DEFAULT);

	if (!m_windowtitle.IsEmpty())
		this->SetWindowText(m_windowtitle);
	if (!m_label.IsEmpty())
		SetDlgItemText(IDC_LABEL, m_label);

	if (!m_name.IsEmpty())
	{
		CString sTmp;
		sTmp.Format(IDS_RENAME_INFO, static_cast<LPCWSTR>(m_name));
		SetDlgItemText(IDC_RENINFOLABEL, sTmp);
	}

	AddAnchor(IDC_RENINFOLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LABEL, TOP_LEFT);
	AddAnchor(IDC_NAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BROWSE_REF, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	CControlsBridge::AlignHorizontally(this, IDC_LABEL, IDC_NAME);
	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"RenameDlg");
	m_originalName = m_name;
	return TRUE;
}

void CRenameDlg::OnOK()
{
	UpdateData();
	m_name.Trim();
	bool nameAllowed = ((m_originalName != m_name) || !m_renameRequired) && !m_name.IsEmpty();
	if (!nameAllowed)
	{
		m_bBalloonVisible = true;
		ShowEditBalloon(IDC_NAME, IDS_WARN_RENAMEREQUIRED, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	CTGitPath path(m_name);
	if (!path.IsValidOnWindows() || !PathIsRelative(m_name))
	{
		m_bBalloonVisible = true;
		ShowEditBalloon(IDC_NAME, IDS_WARN_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	if (m_pInputValidator)
	{
		CString sError = m_pInputValidator(IDC_NAME, m_name);
		if (!sError.IsEmpty())
		{
			m_bBalloonVisible = true;
			ShowEditBalloon(IDC_NAME, sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), TTI_ERROR);
			return;
		}
	}

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CRenameDlg::OnCancel()
{
	// find out if there's a balloon tip showing and if there is,
	// hide that tooltip but do NOT exit the dialog.
	if (m_bBalloonVisible)
	{
		Edit_HideBalloonTip(GetDlgItem(IDC_NAME)->GetSafeHwnd());
		m_bBalloonVisible = false;
		return;
	}

	CHorizontalResizableStandAloneDialog::OnCancel();
}

void CRenameDlg::OnEnSetfocusName()
{
	m_bBalloonVisible = false;
}

void CRenameDlg::OnBnClickedButtonBrowseRef()
{
	CString ext;
	CString path;
	if (!m_originalName.IsEmpty())
	{
		CTGitPath origname(m_sBaseDir);
		origname.AppendPathString(m_originalName);
		ext = origname.GetFileExtension();
		path = origname.GetWinPathString();
	}

	if (CAppUtils::FileOpenSave(path, nullptr, AFX_IDD_FILESAVE, 0, false, GetSafeHwnd(), ext.Mid(1), !path.IsEmpty()))
	{
		GetDlgItem(IDC_NAME)->SetFocus();
		CTGitPath target(path);
		CString targetRoot;
		if (!target.HasAdminDir(&targetRoot) || g_Git.m_CurrentDir.CompareNoCase(targetRoot) != 0)
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_ERR_MUSTBESAMEWT, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		CString relPath;
		m_sBaseDir.Replace(L"/", L"\\");
		if (!PathRelativePathTo(CStrBuf(relPath, MAX_PATH), m_sBaseDir, FILE_ATTRIBUTE_DIRECTORY, path, FILE_ATTRIBUTE_DIRECTORY))
			return;
		if (CStringUtils::StartsWith(relPath, L".\\"))
			relPath = relPath.Mid(static_cast<int>(wcslen(L".\\")));
		m_name = relPath;
		UpdateData(FALSE);
	}
}
