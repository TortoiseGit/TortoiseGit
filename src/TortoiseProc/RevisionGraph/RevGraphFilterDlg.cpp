// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng
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
#include "RevGraphFilterDlg.h"
#include "gittype.h"
#include "git.h"
#include "BrowseRefsDlg.h"

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
	DDX_Control(pDX, IDC_FROMREV, m_ctrlFromRev);
	DDX_Control(pDX, IDC_TOREV, m_ctrlToRev);
}


BEGIN_MESSAGE_MAP(CRevGraphFilterDlg, CDialog)
	ON_BN_CLICKED(IDC_REV1BTN1, &CRevGraphFilterDlg::OnBnClickedRev1btn1)
	ON_BN_CLICKED(IDC_REV1BTN2, &CRevGraphFilterDlg::OnBnClickedRev1btn2)
END_MESSAGE_MAP()

BOOL CRevGraphFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	this->m_ctrlFromRev.Init();
	this->m_ctrlToRev.Init();

	STRING_VECTOR list;
	g_Git.GetRefList(list);

	m_ctrlFromRev.AddSearchString(_T("HEAD"));
	m_ctrlToRev.AddSearchString(_T("HEAD"));

	for(int i=0;i<list.size();i++)
	{
		CString str=list[i];
		
		m_ctrlFromRev.AddSearchString(list[i]);
		m_ctrlToRev.AddSearchString(list[i]);

		if(str.Find(_T("refs/")) == 0)
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(5));
			m_ctrlToRev.AddSearchString(list[i].Mid(5));
		}

		if(str.Find(_T("refs/heads/")) == 0)
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(11));
			m_ctrlToRev.AddSearchString(list[i].Mid(11));
		}

		if(str.Find(_T("refs/remotes/")) == 0)
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(13));
			m_ctrlToRev.AddSearchString(list[i].Mid(13));
		}

		if(str.Find(_T("refs/tags/")) == 0)
		{
			m_ctrlFromRev.AddSearchString(list[i].Mid(10));
			m_ctrlToRev.AddSearchString(list[i].Mid(10));
		}
	}

	return TRUE;
}


void CRevGraphFilterDlg::GetRevisionRange(CString& minrev, CString& maxrev)
{
	minrev = m_sFromRev;
	maxrev = m_sToRev;
}

void CRevGraphFilterDlg::SetRevisionRange (CString minrev, CString maxrev)
{
	m_sFromRev = minrev;
	m_sToRev = maxrev;
}

void CRevGraphFilterDlg::OnOK()
{
	CDialog::OnOK();
}

void CRevGraphFilterDlg::OnBnClickedRev1btn1()
{
	// TODO: Add your control notification handler code here
	CString str = CBrowseRefsDlg::PickRef();
	if(str.IsEmpty())
		return;

	m_ctrlFromRev.SetWindowText(str);
}

void CRevGraphFilterDlg::OnBnClickedRev1btn2()
{
	// TODO: Add your control notification handler code here
	CString str = CBrowseRefsDlg::PickRef();
	if(str.IsEmpty())
		return;

	m_ctrlToRev.SetWindowText(str);
}
