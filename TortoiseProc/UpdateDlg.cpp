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
#include "UpdateDlg.h"
#include "registry.h"


IMPLEMENT_DYNAMIC(CUpdateDlg, CStandAloneDialog)
CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CUpdateDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CUpdateDlg::~CUpdateDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sRevision);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, OnBnClickedShowLog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_EN_CHANGE(IDC_REVNUM, &CUpdateDlg::OnEnChangeRevnum)
END_MESSAGE_MAP()

BOOL CUpdateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	AdjustControlSize(IDC_NEWEST);
	AdjustControlSize(IDC_REVISION_N);
	AdjustControlSize(IDC_NOEXTERNALS);

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	m_depthCombo.SetCurSel(0);

	CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	GetDlgItem(IDC_REVNUM)->SetFocus();
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return FALSE;  
}

void CUpdateDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	Revision = SVNRev(m_sRevision);
	if (GetCheckedRadioButton(IDC_NEWEST, IDC_REVISION_N) == IDC_NEWEST)
	{
		Revision = SVNRev(_T("HEAD"));
	}
	if (!Revision.IsValid())
	{
		ShowBalloon(IDC_REVNUM, IDS_ERR_INVALIDREV);
		return;
	}
	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		m_depth = svn_depth_unknown;
		break;
	case 1:
		m_depth = svn_depth_infinity;
		break;
	case 2:
		m_depth = svn_depth_immediates;
		break;
	case 3:
		m_depth = svn_depth_files;
		break;
	case 4:
		m_depth = svn_depth_empty;
		break;
	default:
		m_depth = svn_depth_empty;
		break;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CUpdateDlg::OnBnClickedShowLog()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	if (!m_wcPath.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->m_wParam = 0;
		m_pLogDlg->SetParams(m_wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CUpdateDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_REVNUM, temp);
	CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CUpdateDlg::OnEnChangeRevnum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	else
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
}
