// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2025 - TortoiseGit
// Copyright (C) 2009-2011, 2013 - TortoiseSVN

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
#include "SettingsAdvanced.h"
#include "Git.h"

IMPLEMENT_DYNAMIC(CSettingsAdvanced, ISettingsPropPage)

CSettingsAdvanced::CSettingsAdvanced()
	: ISettingsPropPage(CSettingsAdvanced::IDD)
{
	AddSetting<DWORDSetting>  (L"AutoCompleteMinChars", 3);
	AddSetting<DWORDSetting>  (L"AutocompleteParseMaxSize", 300000L);
	AddSetting<BooleanSetting>(L"AutocompleteParseUnversioned", false);
	AddSetting<BooleanSetting>(L"AutocompleteRemovesExtensions", false);
	AddSetting<BooleanSetting>(L"BlockStatus", false);
	AddSetting<BooleanSetting>(L"CacheTrayIcon", false);
	AddSetting<BooleanSetting>(L"CacheSave", true);
	AddSetting<BooleanSetting>(L"ConflictDontGuessBranchNames", false);
	AddSetting<BooleanSetting>(L"CygwinHack", false);
	AddSetting<BooleanSetting>(L"Debug", false);
	AddSetting<BooleanSetting>(L"DebugOutputString", false);
	AddSetting<DWORDSetting>  (L"DialogTitles", 0);
	AddSetting<DWORDSetting>  (L"DiffSimilarityIndexThreshold", 50);
	AddSetting<BooleanSetting>(L"DownloadAnimation", true);
	AddSetting<BooleanSetting>(L"FullRowSelect", true);
	AddSetting<DWORDSetting>  (L"GroupTaskbarIconsPerRepo", 3);
	AddSetting<BooleanSetting>(L"GroupTaskbarIconsPerRepoOverlay", true);
	AddSetting<BooleanSetting>(L"LogFontForFileListCtrl", false);
	AddSetting<BooleanSetting>(L"LogFontForLogCtrl", false);
	AddSetting<DWORDSetting>  (L"LogTooManyItemsThreshold", 1000);
	AddSetting<BooleanSetting>(L"LogIncludeBoundaryCommits", false);
	AddSetting<BooleanSetting>(L"LogIncludeWorkingTreeChanges", true);
	AddSetting<BooleanSetting>(L"LogShowSuperProjectSubmodulePointer", true);
	AddSetting<DWORDSetting>  (L"MaxRefHistoryItems", 5);
	AddSetting<BooleanSetting>(L"ModifyExplorerTitle", true);
	AddSetting<BooleanSetting>(L"Msys2Hack", false);
	AddSetting<BooleanSetting>(L"NamedRemoteFetchAll", true);
	AddSetting<BooleanSetting>(L"NoSortLocalBranchesFirst", false);
	AddSetting<DWORDSetting>  (L"NumDiffWarning", 10);
	AddSetting<BooleanSetting>(L"OverlaysCaseSensitive", true);
	AddSetting<DWORDSetting>  (L"ProgressDlgLinesLimit", 50000);
	AddSetting<BooleanSetting>(L"ReaddUnselectedAddedFilesAfterCommit", true);
	AddSetting<BooleanSetting>(L"RefreshFileListAfterResolvingConflict", true);
	AddSetting<BooleanSetting>(L"RememberFileListPosition", true);
	AddSetting<BooleanSetting>(L"SanitizeCommitMsg", true);
	AddSetting<BooleanSetting>(L"ScintillaDirect2D", false);
	AddSetting<BooleanSetting>(L"ShellMenuAccelerators", true);
	AddSetting<DWORDSetting>  (L"ShortHashLengthForHyperLinkInLogMessage", g_Git.GetShortHASHLength());
	AddSetting<BooleanSetting>(L"ShowContextMenuIcons", true);
	AddSetting<BooleanSetting>(L"ShowAppContextMenuIcons", true);
	AddSetting<BooleanSetting>(L"ShowListBackgroundImage", true);
	AddSetting<BooleanSetting>(L"ShowListFullPathTooltip", true);
	AddSetting<DWORDSetting>  (L"SquashDate", 0);
	AddSetting<BooleanSetting>(L"StyleCommitMessages", true);
	AddSetting<BooleanSetting>(L"StyleGitOutput", true);
	AddSetting<DWORDSetting>  (L"TGitCacheCheckContentMaxSize", 10 * 1024);
	AddSetting<DWORDSetting>  (L"UseCustomWordBreak", 2);
	AddSetting<BooleanSetting>(L"UseLibgit2", true);
	AddSetting<BooleanSetting>(L"VersionCheck", true);
	AddSetting<BooleanSetting>(L"VersionCheckPreview", false);
	AddSetting<BooleanSetting>(L"Win8SpellChecker", false);
}

CSettingsAdvanced::~CSettingsAdvanced()
{
}

void CSettingsAdvanced::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONFIG, m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CSettingsAdvanced, ISettingsPropPage)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnBeginlabeledit)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnEndlabeledit)
	ON_NOTIFY(NM_DBLCLK, IDC_CONFIG, &CSettingsAdvanced::OnNMDblclkConfig)
END_MESSAGE_MAP()


BOOL CSettingsAdvanced::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_ListCtrl.DeleteAllItems();
	int c = m_ListCtrl.GetHeaderCtrl()->GetItemCount() - 1;
	while (c >= 0)
		m_ListCtrl.DeleteColumn(c--);

	CString temp;
	temp.LoadString(IDS_SETTINGS_CONF_VALUECOL);
	m_ListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_SETTINGS_CONF_NAMECOL);
	m_ListCtrl.InsertColumn(1, temp);

	m_ListCtrl.SetRedraw(FALSE);

	for (int i = 0; i < static_cast<int>(settings.size()); ++i)
	{
		m_ListCtrl.InsertItem(i, settings.at(i)->GetName());
		m_ListCtrl.SetItemText(i, 1, settings.at(i)->GetName());
		m_ListCtrl.SetItemText(i, 0, settings.at(i)->GetValue());
	}

	for (int col = 0, maxcol = m_ListCtrl.GetHeaderCtrl()->GetItemCount(); col < maxcol; ++col)
		m_ListCtrl.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	int arr[2] = {1,0};
	m_ListCtrl.SetColumnOrderArray(2, arr);
	m_ListCtrl.SetRedraw(TRUE);

	return TRUE;
}

BOOL CSettingsAdvanced::OnApply()
{
	for (int i = 0; i < static_cast<int>(settings.size()); ++i)
	{
		settings.at(i)->StoreValue(m_ListCtrl.GetItemText(i, 0));
	}

	return ISettingsPropPage::OnApply();
}

void CSettingsAdvanced::OnLvnBeginlabeledit(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = FALSE;
}

void CSettingsAdvanced::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;
	if (!pDispInfo->item.pszText)
		return;

	bool allowEdit = settings.at(pDispInfo->item.iItem)->IsValid(pDispInfo->item.pszText);

	if (allowEdit)
		SetModified();

	*pResult = allowEdit ? TRUE : FALSE;
}

BOOL CSettingsAdvanced::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F2:
			{
				m_ListCtrl.EditLabel(m_ListCtrl.GetSelectionMark());
			}
			break;
		}
	}
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsAdvanced::OnNMDblclkConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	m_ListCtrl.EditLabel(pNMItemActivate->iItem);
	*pResult = 0;
}
