// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2007 - TortoiseSVN

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
#include "SettingsPropPage.h"

IMPLEMENT_DYNAMIC(ISettingsPropPage, CPropertyPage)

ISettingsPropPage::ISettingsPropPage() : CPropertyPage()
	, CommonDialogFunctions(this)
	, m_restart(Restart_None)
{
}

ISettingsPropPage::ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle /* = 0 */, DWORD dwSize /* = sizeof */)
: CPropertyPage(lpszTemplateName, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle, dwSize)
, CommonDialogFunctions(this)
, m_restart(Restart_None)
{
}

ISettingsPropPage::ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption, UINT nIDHeaderTitle, UINT nIDHeaderSubTitle /* = 0 */, DWORD dwSize /* = sizeof */)
: CPropertyPage(nIDTemplate, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle, dwSize)
, CommonDialogFunctions(this)
, m_restart(Restart_None)
{
}

ISettingsPropPage::ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption /* = 0 */, DWORD dwSize /* = sizeof */)
: CPropertyPage(lpszTemplateName, nIDCaption, dwSize)
, CommonDialogFunctions(this)
, m_restart(Restart_None)
{
}

ISettingsPropPage::ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption /* = 0 */, DWORD dwSize /* = sizeof */)
: CPropertyPage(nIDTemplate, nIDCaption, dwSize)
, CommonDialogFunctions(this)
, m_restart(Restart_None)
{
}

ISettingsPropPage::~ISettingsPropPage()
{
}

