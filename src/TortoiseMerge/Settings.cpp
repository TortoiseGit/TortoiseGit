// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006, 2009-2010, 2014-2015, 2017-2018 - TortoiseSVN

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
#include "Settings.h"
#include "SetMainPage.h"
#include "SetColorPage.h"

int CALLBACK PropSheetProc(HWND /*hWndDlg*/, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
	case PSCB_PRECREATE:
	{
		auto pResource = reinterpret_cast<LPDLGTEMPLATE>(lParam);
		CDialogTemplate dlgTemplate(pResource);
		dlgTemplate.SetFont(L"MS Shell Dlg 2", 9);
		memmove(reinterpret_cast<void*>(lParam), dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
	}
	break;
	}
	return 0;
}

IMPLEMENT_DYNAMIC(CSettings, CPropertySheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
	, m_pMainPage(nullptr)
	, m_pColorPage(nullptr)
{
}

CSettings::CSettings(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage)
	, m_pMainPage(nullptr)
	, m_pColorPage(nullptr)
{
	AddPropPages();
}

CSettings::~CSettings()
{
	RemovePropPages();
}

void CSettings::AddPropPages()
{
	m_pMainPage = new CSetMainPage();
	m_pColorPage = new CSetColorPage();

	AddPage(m_pMainPage);
	AddPage(m_pColorPage);

	// remove the "apply" button: changes show only after the settings dialog
	// is closed, so the OK button is enough and the "apply" button only
	// confuses users.
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.pfnCallback = PropSheetProc;
	m_psh.dwFlags |= PSH_USECALLBACK;
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
	m_pMainPage = nullptr;
	delete m_pColorPage;
	m_pColorPage = nullptr;
}

void CSettings::SaveData()
{
	m_pMainPage->SaveData();
	m_pColorPage->SaveData();
}

BOOL CSettings::IsReloadNeeded() const
{
	BOOL bReload = FALSE;
	bReload = (m_pMainPage->m_bReloadNeeded || bReload);
	bReload = (m_pColorPage->m_bReloadNeeded || bReload);
	return bReload;
}

BEGIN_MESSAGE_MAP(CSettings, CPropertySheet)
END_MESSAGE_MAP()


// CSettings message handlers

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	return bResult;
}
