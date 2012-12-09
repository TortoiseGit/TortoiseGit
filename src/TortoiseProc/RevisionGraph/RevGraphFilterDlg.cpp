// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "RevGraphFilterDlg.h"


IMPLEMENT_DYNAMIC(CRevGraphFilterDlg, CDialog)

CRevGraphFilterDlg::CRevGraphFilterDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CRevGraphFilterDlg::IDD, pParent)
    , m_sFromRev(_T(""))
    , m_sToRev(_T(""))
	, m_bCurrentBranch(FALSE)
{

}

CRevGraphFilterDlg::~CRevGraphFilterDlg()
{
}

void CRevGraphFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FROMREV, m_sFromRev);
	DDX_Text(pDX, IDC_TOREV, m_sToRev);
	DDX_Check(pDX, IDC_CURRENT_BRANCH, m_bCurrentBranch);
}


BEGIN_MESSAGE_MAP(CRevGraphFilterDlg, CDialog)
	ON_BN_CLICKED(IDC_REV1BTN1, &CRevGraphFilterDlg::OnBnClickedRev1btn1)
	ON_BN_CLICKED(IDC_REV1BTN2, &CRevGraphFilterDlg::OnBnClickedRev1btn2)
END_MESSAGE_MAP()

BOOL CRevGraphFilterDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    return TRUE;
}


void CRevGraphFilterDlg::GetRevisionRange(CString& minrev, CString& maxrev)
{
    minrev = m_sFromRev;
    maxrev = m_sToRev;
}

void CRevGraphFilterDlg::SetRevisionRange (CString minrev, CString maxrev)
{
    m_sToRev = minrev;
    m_sToRev = maxrev;
}

void CRevGraphFilterDlg::OnOK()
{
    CDialog::OnOK();
}

void CRevGraphFilterDlg::OnBnClickedRev1btn1()
{
	// TODO: Add your control notification handler code here
}

void CRevGraphFilterDlg::OnBnClickedRev1btn2()
{
	// TODO: Add your control notification handler code here
}
