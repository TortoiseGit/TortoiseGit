// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2018-2019 - TortoiseGit

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
#include "TaskbarUUID.h"

IMPLEMENT_DYNAMIC(CFirstStartWizard, CStandAloneDialogTmpl<CPropertySheetEx>)

CFirstStartWizard::CFirstStartWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
:CStandAloneDialogTmpl<CPropertySheetEx>(nIDCaption, pParentWnd)
{
	SetWizardMode();
	SetActivePage(iSelectPage);
	AddPage(&language);
	AddPage(&start);
	AddPage(&git);
	AddPage(&user);
	AddPage(&authentication);

	m_psh.dwFlags |= PSH_WIZARD97 | PSH_HEADER;
	m_psh.pszbmHeader = L"TortoiseGit";

	m_psh.hInstance = AfxGetResourceHandle();
}

CFirstStartWizard::~CFirstStartWizard()
{
}

BEGIN_MESSAGE_MAP(CFirstStartWizard, CStandAloneDialogTmpl<CPropertySheetEx>)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(IDCANCEL, &CFirstStartWizard::OnCancel)
END_MESSAGE_MAP()

// CFirstStartWizard message handlers
BOOL CFirstStartWizard::OnInitDialog()
{
	BOOL bResult = __super::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	if (!m_pParentWnd && GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

	return bResult;
}

void CFirstStartWizard::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID & 0xFFF0)
	{
	case SC_CLOSE:
		{
			auto page = static_cast<CFirstStartWizardBasePage*>(GetActivePage());
			if (page && !page->OkToCancel())
				break;
		}
		// fall through
	default:
		__super::OnSysCommand(nID, lParam);
		break;
	}
}

void CFirstStartWizard::OnCancel()
{
	auto page = static_cast<CFirstStartWizardBasePage*>(GetActivePage());
	if (!page || page->OkToCancel())
		Default();
}
