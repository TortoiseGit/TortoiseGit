// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018, 2020-2021, 2024 - TortoiseGit
// Copyright (C) 2003-2008, 2020 - TortoiseSVN

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
#include "Settings.h"
#include "Git.h"
#include "MessageBox.h"
#include "../../TGitCache\CacheInterface.h"
#include "GitAdminDir.h"
#include "AppUtils.h"
#include "Theme.h"
#include "DarkModeHelper.h"
#include "SetMainPage.h"
#include "SetProxyPage.h"
#include "SetOverlayPage.h"
#include "SetOverlayIcons.h"
#include "SetOverlayHandlers.h"
#include "SettingsProgsDiff.h"
#include "SettingsProgsAlternativeEditor.h"
#include "SettingsProgsMerge.h"
#include "SetLookAndFeelPage.h"
#include "SetDialogs.h"
#include "SetDialogs2.h"
#include "SetDialogs3.h"
#include "SettingsColors.h"
#include "SettingsColors2.h"
#include "SettingsColors3.h"
#include "SetSavedDataPage.h"
#include "SetHooks.h"
#include "SetBugTraq.h"
#include "SettingsTBlame.h"
#include "SettingGitConfig.h"
#include "SettingGitCredential.h"
#include "SettingsBugtraqConfig.h"
#include "SetExtMenu.h"
#include "SetWin11ContextMenu.h"
#include "SettingsAdvanced.h"
#include "SettingSMTP.h"
#include "SettingsTUDiff.h"
#include "SysInfo.h"

IMPLEMENT_DYNAMIC(CSettings, CTreePropSheet)
CSettings::CSettings(UINT nIDCaption, CTGitPath * /*cmdPath*/, CWnd* pParentWnd, UINT iSelectPage)
: CTreePropSheet(nIDCaption, pParentWnd)
{
	AddPropPages();
	SetTheme(CTheme::Instance().IsDarkTheme());
	SetActivePage(iSelectPage);
}

CSettings::~CSettings()
{
	RemovePropPages();
}

void CSettings::SetTheme(bool bDark)
{
	__super::SetTheme(bDark);
	for (int i = 0; i < GetPageCount(); ++i)
	{
		auto pPage = GetPage(i);
		if (IsWindow(pPage->GetSafeHwnd()))
			CTheme::Instance().SetThemeForDialog(pPage->GetSafeHwnd(), bDark);
	}
	::RedrawWindow(GetSafeHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

ISettingsPropPage* CSettings::AddPropPage(ISettingsPropPage* page, CString pageName)
{
	ASSERT(std::none_of(m_pPages.cbegin(), m_pPages.cend(), [&pageName](auto& settingsPage) { return settingsPage.pageName == pageName; }) && "pageName must be unique");
	AddPage(page);
	SetPageIcon(page, page->GetIconID());
	m_pPages.emplace_back(page, pageName);
	return page;
}

void CSettings::AddPropPage(ISettingsPropPage* page, CString pageName, CPropertyPage* parentPage)
{
	AddPropPage(page, pageName);
	SetParentPage(parentPage, page);
}

void CSettings::AddPropPages()
{
	auto pMainPage = AddPropPage(new CSetMainPage(), L"main");
	AddPropPage(new CSetLookAndFeelPage(), L"look", pMainPage);
	AddPropPage(new CSetExtMenu(), L"extmenu", pMainPage);
	if (SysInfo::Instance().IsWin11OrLater())
		AddPropPage(new CSetWin11ContextMenu(), L"win11menu", pMainPage);
	AddPropPage(new CSetDialogs(), L"dialog", pMainPage);
	AddPropPage(new CSetDialogs2(), L"dialog2", pMainPage);
	AddPropPage(new CSetDialogs3(), L"dialog3", pMainPage);
	AddPropPage(new CSettingsColors(), L"color1", pMainPage);
	AddPropPage(new CSettingsColors2(), L"color2", pMainPage);
	AddPropPage(new CSettingsColors3(), L"color3", pMainPage);
	AddPropPage(new CSettingsProgsAlternativeEditor(), L"alternativeeditor", pMainPage);

	m_pGitConfig = AddPropPage(new CSettingGitConfig(), L"gitconfig");
	CString repo = g_Git.m_CurrentDir;
	bool hasLocalRepo = GitAdminDir::IsWorkingTreeOrBareRepo(repo);
	if (hasLocalRepo)
	{
		m_pGitRemote = new CSettingGitRemote();
		AddPropPage(m_pGitRemote, L"gitremote", m_pGitConfig);
	}
	AddPropPage(new CSettingGitCredential(), L"gitcredential", m_pGitConfig);

	auto pHooksPage = AddPropPage(new CSetHooks(), L"hooks");
	AddPropPage(new CSetBugTraq(), L"bugtraq", pHooksPage);
	if (hasLocalRepo)
		AddPropPage(new CSettingsBugtraqConfig(), L"bugtraqconfig", pHooksPage);

	auto pOverlayPage = AddPropPage(new CSetOverlayPage(), L"overlay");
	AddPropPage(new CSetOverlayIcons(), L"overlays", pOverlayPage);
	AddPropPage(new CSetOverlayHandlers(), L"overlayshandlers", pOverlayPage);

	auto pProxyPage = AddPropPage(new CSetProxyPage(), L"proxy");
	AddPropPage(new CSettingSMTP(), L"smtp", pProxyPage);

	auto pProgsDiffPage = AddPropPage(new CSettingsProgsDiff(), L"diff");
	AddPropPage(new CSettingsProgsMerge(), L"merge", pProgsDiffPage);

	AddPropPage(new CSetSavedDataPage(), L"save");
	AddPropPage(new CSettingsTBlame(), L"blame");
	AddPropPage(new CSettingsUDiff(), L"udiff");
	AddPropPage(new CSettingsAdvanced(), L"advanced");
}

void CSettings::RemovePropPages()
{
	m_pPages.clear();
}

void CSettings::HandleRestart()
{
	int restart = ISettingsPropPage::Restart_None;
	for (const auto& page : m_pPages)
	{
		restart |= page.page->GetRestart();
	}

	if (restart & ISettingsPropPage::Restart_System)
	{
		DWORD_PTR res = 0;
		::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
		CMessageBox::Show(GetSafeHwnd(), IDS_SETTINGS_RESTARTSYSTEM, IDS_APPNAME, MB_ICONINFORMATION);
	}
	if (restart & ISettingsPropPage::Restart_Cache)
	{
		DWORD_PTR res = 0;
		::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
		// tell the cache to refresh everything
		SendCacheCommand(TGITCACHECOMMAND_REFRESHALL);
		SendCacheCommand(TGITCACHECOMMAND_END);
	}
}

BEGIN_MESSAGE_MAP(CSettings, CTreePropSheet)
END_MESSAGE_MAP()

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = __super::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString title;
	GetWindowText(title);
	if (GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
		CAppUtils::SetWindowTitle(GetSafeHwnd(), g_Git.m_CurrentDir, title);
	else
		SetWindowText(title + " - " + CString(MAKEINTRESOURCE(IDS_APPNAME)));

	DarkModeHelper::Instance().AllowDarkModeForApp(CTheme::Instance().IsDarkTheme());
	SetTheme(CTheme::Instance().IsDarkTheme());
	CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

	CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

	bool foundDefaultPage = false;
	if (!m_DefaultPage.IsEmpty())
	{
		for (const auto& page : m_pPages)
		{
			if (this->m_DefaultPage == page.pageName)
			{
				if (page.page == m_pGitRemote)
					m_pGitRemote->m_bNoFetch = true;
				SetActivePage(page.page);
				foundDefaultPage = true;
				break;
			}
		}
	}
	if (!foundDefaultPage && GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
		SetActivePage(m_pGitConfig);

	return bResult;
}
