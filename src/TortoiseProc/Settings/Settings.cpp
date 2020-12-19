// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018, 2020 - TortoiseGit
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

void CSettings::AddPropPage(ISettingsPropPage* page)
{
	AddPage(page);
	SetPageIcon(page, page->GetIconID());
}

void CSettings::AddPropPage(ISettingsPropPage* page, CPropertyPage* parentPage)
{
	AddPropPage(page);
	SetParentPage(parentPage, page);
}

void CSettings::AddPropPages()
{
	m_pMainPage = new CSetMainPage();
	m_pOverlayPage = new CSetOverlayPage();
	m_pOverlaysPage = new CSetOverlayIcons();
	m_pOverlayHandlersPage = new CSetOverlayHandlers();
	m_pProxyPage = new CSetProxyPage();
	m_pSMTPPage = new CSettingSMTP();
	m_pProgsDiffPage = new CSettingsProgsDiff();
	m_pProgsMergePage = new CSettingsProgsMerge();
	m_pProgsAlternativeEditor = new CSettingsProgsAlternativeEditor();
	m_pLookAndFeelPage = new CSetLookAndFeelPage();

	m_pExtMenu	= new CSetExtMenu();

	m_pDialogsPage = new CSetDialogs();
	m_pDialogsPage2 = new CSetDialogs2();
	m_pDialogsPage3 = new CSetDialogs3();
	m_pColorsPage = new CSettingsColors();
	m_pColorsPage2 = new CSettingsColors2();
	m_pColorsPage3 = new CSettingsColors3();
	m_pSavedPage = new CSetSavedDataPage();
	m_pHooksPage = new CSetHooks();
	m_pBugTraqPage = new CSetBugTraq();
	m_pTBlamePage = new CSettingsTBlame();
	m_pGitConfig = new CSettingGitConfig();
	m_pGitRemote = new CSettingGitRemote();
	m_pGitCredential = new CSettingGitCredential();
	m_pBugtraqConfig = new CSettingsBugtraqConfig();
	m_pUDiffPage = new CSettingsUDiff();
	m_pAdvanced = new CSettingsAdvanced();

	AddPropPage(m_pMainPage);
	AddPropPage(m_pGitConfig);
	AddPropPage(m_pHooksPage);
	AddPropPage(m_pOverlayPage);
	AddPropPage(m_pOverlaysPage, m_pOverlayPage);
	AddPropPage(m_pOverlayHandlersPage, m_pOverlayPage);
	AddPropPage(m_pProxyPage);
	AddPropPage(m_pSMTPPage, m_pProxyPage);
	AddPropPage(m_pProgsDiffPage);
	AddPropPage(m_pProgsMergePage, m_pProgsDiffPage);
	AddPropPage(m_pLookAndFeelPage, m_pMainPage);
	AddPropPage(m_pExtMenu, m_pMainPage);
	AddPropPage(m_pDialogsPage, m_pMainPage);
	AddPropPage(m_pDialogsPage2, m_pMainPage);
	AddPropPage(m_pDialogsPage3, m_pMainPage);
	AddPropPage(m_pColorsPage, m_pMainPage);
	AddPropPage(m_pColorsPage2, m_pMainPage);
	AddPropPage(m_pColorsPage3, m_pMainPage);
	AddPropPage(m_pProgsAlternativeEditor, m_pMainPage);
	AddPropPage(m_pSavedPage);

	CString repo = g_Git.m_CurrentDir;
	bool hasLocalRepo = GitAdminDir::IsWorkingTreeOrBareRepo(repo);
	if (hasLocalRepo)
	{
		AddPropPage(m_pGitRemote, m_pGitConfig);
	}
	AddPropPage(m_pGitCredential, m_pGitConfig);
	AddPropPage(m_pBugTraqPage, m_pHooksPage);
	if (hasLocalRepo)
	{
		AddPropPage(m_pBugtraqConfig, m_pHooksPage);
	}
	AddPropPage(m_pTBlamePage);
	AddPropPage(m_pUDiffPage);
	AddPropPage(m_pAdvanced);
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
	delete m_pOverlayPage;
	delete m_pOverlaysPage;
	delete m_pOverlayHandlersPage;
	delete m_pProxyPage;
	delete m_pSMTPPage;
	delete m_pProgsDiffPage;
	delete m_pProgsMergePage;
	delete m_pProgsAlternativeEditor;
	delete m_pLookAndFeelPage;
	delete m_pDialogsPage;
	delete m_pDialogsPage2;
	delete m_pDialogsPage3;
	delete m_pColorsPage;
	delete m_pColorsPage2;
	delete m_pColorsPage3;
	delete m_pSavedPage;
	delete m_pHooksPage;
	delete m_pBugTraqPage;
	delete m_pTBlamePage;
	delete m_pUDiffPage;
	delete m_pGitConfig;
	delete m_pGitRemote;
	delete m_pGitCredential;
	delete m_pBugtraqConfig;
	delete m_pExtMenu;
	delete m_pAdvanced;
}

void CSettings::HandleRestart()
{
	int restart = ISettingsPropPage::Restart_None;
	restart |= m_pMainPage->GetRestart();
	restart |= m_pOverlayPage->GetRestart();
	restart |= m_pOverlaysPage->GetRestart();
	restart |= m_pOverlayHandlersPage->GetRestart();
	restart |= m_pProxyPage->GetRestart();
	restart |= m_pSMTPPage->GetRestart();
	restart |= m_pProgsDiffPage->GetRestart();
	restart |= m_pProgsMergePage->GetRestart();
	restart |= m_pProgsAlternativeEditor->GetRestart();
	restart |= m_pLookAndFeelPage->GetRestart();
	restart |= m_pDialogsPage->GetRestart();
	restart |= m_pDialogsPage2->GetRestart();
	restart |= m_pDialogsPage3->GetRestart();
	restart |= m_pColorsPage->GetRestart();
	restart |= m_pColorsPage2->GetRestart();
	restart |= m_pColorsPage3->GetRestart();
	restart |= m_pSavedPage->GetRestart();
	restart |= m_pHooksPage->GetRestart();
	restart |= m_pBugTraqPage->GetRestart();
	restart |= m_pTBlamePage->GetRestart();
	restart |= m_pUDiffPage->GetRestart();
	restart |= m_pGitConfig->GetRestart();
	restart |= m_pGitRemote->GetRestart();
	restart |= m_pGitCredential->GetRestart();
	restart |= m_pBugTraqPage->GetRestart();
	restart |= m_pExtMenu->GetRestart();
	restart |= m_pAdvanced->GetRestart();

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

	if (GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
	{
		CString title;
		GetWindowText(title);
		SetWindowText(g_Git.m_CurrentDir + L" - " + title);
	}

	DarkModeHelper::Instance().AllowDarkModeForApp(CTheme::Instance().IsDarkTheme());
	SetTheme(CTheme::Instance().IsDarkTheme());
	CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

	CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

	if (this->m_DefaultPage == L"gitremote")
	{
		this->SetActivePage(this->m_pGitRemote);
		this->m_pGitRemote->m_bNoFetch = true;
	}
	else if (this->m_DefaultPage == L"gitconfig")
		this->SetActivePage(this->m_pGitConfig);
	else if (this->m_DefaultPage == L"gitcredential")
		this->SetActivePage(this->m_pGitCredential);
	else if (this->m_DefaultPage == L"main")
		this->SetActivePage(this->m_pMainPage);
	else if (this->m_DefaultPage == L"overlay")
		this->SetActivePage(this->m_pOverlayPage);
	else if (this->m_DefaultPage == L"overlays")
		this->SetActivePage(this->m_pOverlaysPage);
	else if (this->m_DefaultPage == L"overlayshandlers")
		this->SetActivePage(this->m_pOverlayHandlersPage);
	else if (this->m_DefaultPage == L"proxy")
		this->SetActivePage(this->m_pProxyPage);
	else if (this->m_DefaultPage == L"smtp")
		this->SetActivePage(this->m_pSMTPPage);
	else if (this->m_DefaultPage == L"diff")
		this->SetActivePage(this->m_pProgsDiffPage);
	else if (this->m_DefaultPage == L"merge")
		this->SetActivePage(this->m_pProgsMergePage);
	else if (this->m_DefaultPage == L"alternativeeditor")
		this->SetActivePage(this->m_pProgsAlternativeEditor);
	else if (this->m_DefaultPage == L"look")
		this->SetActivePage(this->m_pLookAndFeelPage);
	else if (this->m_DefaultPage == L"dialog")
		this->SetActivePage(this->m_pDialogsPage);
	else if (this->m_DefaultPage == L"dialog2")
		this->SetActivePage(this->m_pDialogsPage2);
	else if (this->m_DefaultPage == L"dialog3")
		this->SetActivePage(this->m_pDialogsPage3);
	else if (this->m_DefaultPage == L"color1")
		this->SetActivePage(this->m_pColorsPage);
	else if (this->m_DefaultPage == L"color2")
		this->SetActivePage(this->m_pColorsPage2);
	else if (this->m_DefaultPage == L"color3")
		this->SetActivePage(this->m_pColorsPage3);
	else if (this->m_DefaultPage == L"save")
		this->SetActivePage(this->m_pSavedPage);
	else if (this->m_DefaultPage == L"advanced")
		this->SetActivePage(this->m_pAdvanced);
	else if (this->m_DefaultPage == L"blame")
		this->SetActivePage(this->m_pTBlamePage);
	else if (this->m_DefaultPage == L"udiff")
		this->SetActivePage(this->m_pUDiffPage);
	else if (GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
		this->SetActivePage(this->m_pGitConfig);
	return bResult;
}
