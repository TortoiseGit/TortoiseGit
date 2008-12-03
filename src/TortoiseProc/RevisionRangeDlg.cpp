// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "RevisionRangeDlg.h"


IMPLEMENT_DYNAMIC(CRevisionRangeDlg, CStandAloneDialog)

CRevisionRangeDlg::CRevisionRangeDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CRevisionRangeDlg::IDD, pParent)
	, m_bAllowWCRevs(true)
	, m_StartRev(_T("HEAD"))
	, m_EndRev(_T("HEAD"))
{
}

CRevisionRangeDlg::~CRevisionRangeDlg()
{
}

void CRevisionRangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sStartRevision);
	DDX_Text(pDX, IDC_REVNUM2, m_sEndRevision);
}


BEGIN_MESSAGE_MAP(CRevisionRangeDlg, CStandAloneDialog)
	ON_EN_CHANGE(IDC_REVNUM, OnEnChangeRevnum)
	ON_EN_CHANGE(IDC_REVNUM2, OnEnChangeRevnum2)
END_MESSAGE_MAP()

BOOL CRevisionRangeDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	if (m_StartRev.IsHead())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		if (m_StartRev.IsDate())
			sRev = m_StartRev.GetDateString();
		else
			sRev.Format(_T("%ld"), (LONG)(m_StartRev));
		SetDlgItemText(IDC_REVNUM, sRev);
	}
	if (m_EndRev.IsHead())
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_NEWEST2);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_REVISION_N2);
		CString sRev;
		if (m_EndRev.IsDate())
			sRev = m_EndRev.GetDateString();
		else
			sRev.Format(_T("%ld"), (LONG)(m_EndRev));
		SetDlgItemText(IDC_REVNUM2, sRev);
	}

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_REVNUM)->SetFocus();
	return FALSE;
}

void CRevisionRangeDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	m_StartRev = SVNRev(m_sStartRevision);
	if (GetCheckedRadioButton(IDC_NEWEST, IDC_REVISION_N) == IDC_NEWEST)
	{
		m_StartRev = SVNRev(_T("HEAD"));
		m_sStartRevision = _T("HEAD");
	}
	if ((!m_StartRev.IsValid())||((!m_bAllowWCRevs)&&(m_StartRev.IsPrev() || m_StartRev.IsCommitted() || m_StartRev.IsBase())))
	{
		ShowBalloon(IDC_REVNUM, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC);
		return;
	}

	m_EndRev = SVNRev(m_sEndRevision);
	if (GetCheckedRadioButton(IDC_NEWEST2, IDC_REVISION_N2) == IDC_NEWEST2)
	{
		m_EndRev = SVNRev(_T("HEAD"));
		m_sEndRevision = _T("HEAD");
	}
	if ((!m_EndRev.IsValid())||((!m_bAllowWCRevs)&&(m_EndRev.IsPrev() || m_EndRev.IsCommitted() || m_EndRev.IsBase())))
	{
		ShowBalloon(IDC_REVNUM2, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC);
		return;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CRevisionRangeDlg::OnEnChangeRevnum()
{
	CString sText;
	GetDlgItemText(IDC_REVNUM, sText);
	if (sText.IsEmpty())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	}
}

void CRevisionRangeDlg::OnEnChangeRevnum2()
{
	CString sText;
	GetDlgItemText(IDC_REVNUM2, sText);
	if (sText.IsEmpty())
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_NEWEST2);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_REVISION_N2);
	}
}
