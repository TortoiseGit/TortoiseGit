// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit

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
#include "FirstStartWizard.h"
#include "FirstStartWizardStart.h"

#define WM_SETPAGEFOCUS WM_APP+2

IMPLEMENT_DYNAMIC(CFirstStartWizardStart, CFirstStartWizardBasePage)

CFirstStartWizardStart::CFirstStartWizardStart() : CFirstStartWizardBasePage(CFirstStartWizardStart::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_STARTTITLE);
}

CFirstStartWizardStart::~CFirstStartWizardStart()
{
}

void CFirstStartWizardStart::DoDataExchange(CDataExchange* pDX)
{
	CFirstStartWizardBasePage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFirstStartWizardStart, CFirstStartWizardBasePage)
	ON_MESSAGE(WM_SETPAGEFOCUS, OnDialogDisplayed)
	ON_NOTIFY(NM_CLICK, IDC_FIRSTSTART_HINT, OnClickedLink)
END_MESSAGE_MAP()

static void AppendStringResource(CString& text, UINT resouceID)
{
	CString temp;
	temp.LoadString(resouceID);
	text.AppendChar(L'\n');
	text.AppendChar(L'\n');
	text.Append(temp);
}

BOOL CFirstStartWizardStart::OnInitDialog()
{
	CFirstStartWizardBasePage::OnInitDialog();

	CString hinttext;
	hinttext.LoadString(IDS_FIRSTSTART_HINT1);
	AppendStringResource(hinttext, IDS_FIRSTSTART_HINT2);
	AppendStringResource(hinttext, IDS_FIRSTSTART_HINT3);
	AppendStringResource(hinttext, IDS_FIRSTSTART_HINT4);
	GetDlgItem(IDC_FIRSTSTART_HINT)->SetWindowText(hinttext);

	return TRUE;
}

BOOL CFirstStartWizardStart::OnSetActive()
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

	wiz->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	PostMessage(WM_SETPAGEFOCUS, 0, 0);

	return CFirstStartWizardBasePage::OnSetActive();
}

LRESULT CFirstStartWizardStart::OnDialogDisplayed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

	wiz->GetDlgItem(ID_WIZNEXT)->SetFocus();

	return 0;
}

void CFirstStartWizardStart::OnClickedLink(NMHDR* pNMHDR, LRESULT* pResult)
{
	ATLASSERT(pNMHDR && pResult);
	auto pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	if (wcscmp(pNMLink->item.szID, L"manual") == 0)
	{
		CString helppath(theApp.m_pszHelpFilePath);
		helppath += L"::/tgit-dug.html#tgit-dug-general";
		::HtmlHelp(GetSafeHwnd(), helppath, HH_DISPLAY_TOPIC, 0);
	}
	else if (wcscmp(pNMLink->item.szID, L"support") == 0)
		ShellExecute(GetSafeHwnd(), L"open", L"https://tortoisegit.org/support/", nullptr, nullptr, SW_SHOWNORMAL);
	*pResult = 0;
}
