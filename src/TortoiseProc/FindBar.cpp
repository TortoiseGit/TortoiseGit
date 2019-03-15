// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2018-2019 - TortoiseGit

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
#include "resource.h"
#include "FindBar.h"
#include "LoadIconEx.h"

UINT CFindBar::WM_FINDEXIT = RegisterWindowMessage(L"TORTOISEGIT_PATCHVIEW_FINDEXIT_MSG");
UINT CFindBar::WM_FINDNEXT = RegisterWindowMessage(L"TORTOISEGIT_PATCHVIEW_FINDNEXT_MSG");
UINT CFindBar::WM_FINDPREV = RegisterWindowMessage(L"TORTOISEGIT_PATCHVIEW_FINDPREV_MSG");
UINT CFindBar::WM_FINDRESET = RegisterWindowMessage(L"TORTOISEGIT_PATCHVIEW_FINDRESET_MSG");

IMPLEMENT_DYNAMIC(CFindBar, CDialog)

CFindBar::CFindBar(CWnd* pParent /*=nullptr*/)
: CDialog(CFindBar::IDD, pParent)
, m_hIcon(nullptr)
, m_bMatchCase(false)
{
}

CFindBar::~CFindBar(void)
{
	DestroyIcon(m_hIcon);
}

BOOL CFindBar::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_hIcon = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CANCELNORMAL));
	GetDlgItem(IDC_FINDEXIT)->SendMessage(BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(m_hIcon));

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CFindBar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FINDTEXT, m_sFindStr);
	DDX_Check(pDX, IDC_MATCHCASECHECK, m_bMatchCase);
}

BEGIN_MESSAGE_MAP(CFindBar, CDialog)
	ON_BN_CLICKED(IDC_FINDEXIT, &OnFindExit)
	ON_BN_CLICKED(IDC_FINDNEXT, &OnFindNext)
	ON_BN_CLICKED(IDC_FINDPREV, &OnFindPrev)
	ON_EN_CHANGE(IDC_FINDTEXT, &OnFindTextChange)
END_MESSAGE_MAP()

void CFindBar::OnOK()
{
	// Do nothing if enter is pressed
}

void CFindBar::OnCancel()
{
	// hide the find bar on escape
	OnFindExit();
}

void CFindBar::OnFindNext()
{
	UpdateData(TRUE);
	if (!m_sFindStr.IsEmpty())
		GetParent()->SendMessage(WM_FINDNEXT);
}

void CFindBar::OnFindPrev()
{
	UpdateData(TRUE);
	if (!m_sFindStr.IsEmpty())
		GetParent()->SendMessage(WM_FINDPREV);
}

void CFindBar::OnFindExit()
{
	GetParent()->SendMessage(WM_FINDEXIT);
}

void CFindBar::OnFindTextChange()
{
	GetParent()->SendMessage(WM_FINDRESET);
	OnFindNext();
}
