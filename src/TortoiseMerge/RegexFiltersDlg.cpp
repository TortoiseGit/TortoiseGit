// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "RegexFiltersDlg.h"
#include "RegexFilterDlg.h"
#include <afxdialogex.h>


// CRegexFiltersDlg dialog

IMPLEMENT_DYNAMIC(CRegexFiltersDlg, CDialogEx)

CRegexFiltersDlg::CRegexFiltersDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(CRegexFiltersDlg::IDD, pParent)
	, m_pIni(nullptr)
{
}

CRegexFiltersDlg::~CRegexFiltersDlg()
{
}

void CRegexFiltersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REGEXLIST, m_RegexList);
}


BEGIN_MESSAGE_MAP(CRegexFiltersDlg, CDialogEx)
	ON_BN_CLICKED(IDC_ADD, &CRegexFiltersDlg::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_EDIT, &CRegexFiltersDlg::OnBnClickedEdit)
	ON_BN_CLICKED(IDC_REMOVE, &CRegexFiltersDlg::OnBnClickedRemove)
	ON_NOTIFY(NM_DBLCLK, IDC_REGEXLIST, &CRegexFiltersDlg::OnNMDblclkRegexlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REGEXLIST, &CRegexFiltersDlg::OnLvnItemchangedRegexlist)
END_MESSAGE_MAP()


// CRegexFiltersDlg message handlers


void CRegexFiltersDlg::OnBnClickedAdd()
{
	CRegexFilterDlg dlg(this);
	if (dlg.DoModal() == IDOK)
	{
		m_pIni->SetValue(dlg.m_sName, L"regex", dlg.m_sRegex);
		m_pIni->SetValue(dlg.m_sName, L"replace", dlg.m_sReplace);
	}
	SetupListControl();
}


void CRegexFiltersDlg::OnBnClickedEdit()
{
	CRegexFilterDlg dlg(this);
	auto pos = m_RegexList.GetFirstSelectedItemPosition();
	int index = m_RegexList.GetNextSelectedItem(pos);
	if (index >= 0)
	{
		CString sName = m_RegexList.GetItemText(index, 0);
		dlg.m_sName = sName;
		CString sRegex = m_pIni->GetValue(sName, L"regex", L"");
		dlg.m_sRegex = sRegex;
		CString sReplace = m_pIni->GetValue(sName, L"replace", L"");
		dlg.m_sReplace = sReplace;
		if (dlg.DoModal() == IDOK)
		{
			if (sName != dlg.m_sName)
			{
				m_pIni->Delete(sName, L"regex", true);
				m_pIni->Delete(sName, L"replace", true);
			}
			if (!dlg.m_sName.IsEmpty())
			{
				m_pIni->SetValue(dlg.m_sName, L"regex", dlg.m_sRegex);
				m_pIni->SetValue(dlg.m_sName, L"replace", dlg.m_sReplace);
			}
		}
	}
	SetupListControl();
}


void CRegexFiltersDlg::OnBnClickedRemove()
{
	auto pos = m_RegexList.GetFirstSelectedItemPosition();
	int index = m_RegexList.GetNextSelectedItem(pos);
	if (index >= 0)
	{
		CString sName = m_RegexList.GetItemText(index, 0);
		m_pIni->Delete(sName, L"regex", true);
		m_pIni->Delete(sName, L"replace", true);
	}
	SetupListControl();
}


BOOL CRegexFiltersDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTheme(m_RegexList.GetSafeHwnd(), L"Explorer", nullptr);

	SetupListControl();


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRegexFiltersDlg::OnNMDblclkRegexlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	//LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	OnBnClickedEdit();
}

void CRegexFiltersDlg::SetupListControl()
{
	m_RegexList.SetRedraw(false);
	m_RegexList.DeleteAllItems();

	int c = m_RegexList.GetHeaderCtrl()->GetItemCount() - 1;
	while (c>=0)
		m_RegexList.DeleteColumn(c--);
	m_RegexList.InsertColumn(0, L"Regex");

	CRect rect;
	m_RegexList.GetClientRect(&rect);
	m_RegexList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

	CSimpleIni::TNamesDepend sections;
	m_pIni->GetAllSections(sections);

	int index = 0;
	for (const auto& section : sections)
	{
		m_RegexList.InsertItem(index++, section.pItem);
	}

	m_RegexList.SetRedraw(true);
}


void CRegexFiltersDlg::OnLvnItemchangedRegexlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	//LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	bool bOneItemSelected = (m_RegexList.GetSelectedCount() == 1);
	GetDlgItem(IDC_EDIT)  ->EnableWindow(bOneItemSelected);
	GetDlgItem(IDC_REMOVE)->EnableWindow(bOneItemSelected);
}
