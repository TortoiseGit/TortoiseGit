// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
	CFirstStartWizard* wiz = (CFirstStartWizard*)GetParent();

	wiz->SetWizardButtons(PSWIZB_NEXT);

	return CFirstStartWizardBasePage::OnSetActive();
}
