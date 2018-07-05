// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013, 2018 - TortoiseGit

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

// SinglePropSheetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SinglePropSheetDlg.h"


// CSinglePropSheetDlg dialog
using namespace TreePropSheet;

IMPLEMENT_DYNAMIC(CSinglePropSheetDlg, CTreePropSheet)

CSinglePropSheetDlg::CSinglePropSheetDlg(const TCHAR* szCaption, ISettingsPropPage* pThePropPage, CWnd* pParent /*=nullptr*/)
:	CTreePropSheet(szCaption,pParent),// CSinglePropSheetDlg::IDD, pParent),
	m_pThePropPage(pThePropPage)
{
	AddPropPages();
}

CSinglePropSheetDlg::~CSinglePropSheetDlg()
{
	RemovePropPages();
}

void CSinglePropSheetDlg::AddPropPages()
{
	SetPageIcon(m_pThePropPage, m_pThePropPage->GetIconID());
	AddPage(m_pThePropPage);
}

void CSinglePropSheetDlg::RemovePropPages()
{
	delete m_pThePropPage;
}

void CSinglePropSheetDlg::DoDataExchange(CDataExchange* pDX)
{
	CTreePropSheet::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSinglePropSheetDlg, CTreePropSheet)
END_MESSAGE_MAP()


// CSinglePropSheetDlg message handlers

BOOL CSinglePropSheetDlg::OnInitDialog()
{
	BOOL bReturn = CTreePropSheet::OnInitDialog();

//	CRect clientRect;
//	GetClientRect(&clientRect);
//	clientRect.DeflateRect(10,10,10,10);
//	m_pThePropPage->Create(m_pThePropPage->m_lpszTemplateName,this);
//	m_pThePropPage->MoveWindow(clientRect);


	CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

	return bReturn;
}
