// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2016, 2020 - TortoiseGit
// Copyright (C) 2003-2011 - TortoiseSVN

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
#pragma once

#include "StandAloneDlg.h"
#include "TreePropSheet/TreePropSheet.h"
#include "TGitPath.h"
#include "SettingsPropPage.h"
#include "SettingGitRemote.h"

using namespace TreePropSheet;

class CSettingsPage
{
public:
	CSettingsPage(ISettingsPropPage* page, CString pageName)
		: pageName(pageName)
		, page(page)
	{
	}
	~CSettingsPage()
	{
		delete page;
	}
	CSettingsPage(const CSettingsPage&) = delete;
	CSettingsPage(CSettingsPage&& t)
	{
		this->pageName = t.pageName;
		this->page = t.page;
		t.page = nullptr;
	};

public:
	ISettingsPropPage* page;
	CString pageName;
};

/**
 * \ingroup TortoiseProc
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 */
class CSettings : public CTreePropSheet
{
	DECLARE_DYNAMIC(CSettings)

private:
	/**
	 * Adds all pages to this Settings-Dialog.
	 */
	void AddPropPages();
	/**
	 * Removes the pages and frees up memory.
	 */
	void RemovePropPages();

	ISettingsPropPage* AddPropPage(ISettingsPropPage* page, CString pageName);
	void AddPropPage(ISettingsPropPage* page, CString pageName, CPropertyPage* parentPage);

private:
	std::vector<CSettingsPage> m_pPages;
	ISettingsPropPage* m_pGitConfig = nullptr;
	CSettingGitRemote* m_pGitRemote = nullptr;

public:
	CSettings(UINT nIDCaption, CTGitPath* CmdPath = nullptr, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	virtual ~CSettings();
	CString		m_DefaultPage;
	/**
	 * Calls the SaveData()-methods of each of the settings pages.
	 */
	void HandleRestart();

	void SetTheme(bool bDark);

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog() override;
};
