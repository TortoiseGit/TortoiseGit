// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2019 - TortoiseGit
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
#include "registry.h"


IMPLEMENT_DYNAMIC(CSettingsAdvanced, ISettingsPropPage)

CSettingsAdvanced::CSettingsAdvanced()
	: ISettingsPropPage(CSettingsAdvanced::IDD)
{
	int i = 0;
	settings[i].sName	= L"AutoCompleteMinChars";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 3;

	settings[i].sName	= L"AutocompleteParseMaxSize";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 300000L;

	settings[i].sName	= L"AutocompleteParseUnversioned";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"AutocompleteRemovesExtensions";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"BlockStatus";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"CacheTrayIcon";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"CacheSave";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"CygwinHack";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"Debug";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"DebugOutputString";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"DiffBlamesWithTortoiseMerge";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"DiffSimilarityIndexThreshold";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 50;

	settings[i].sName	= L"FullRowSelect";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"GroupTaskbarIconsPerRepo";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 3;

	settings[i].sName	= L"GroupTaskbarIconsPerRepoOverlay";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"LogIncludeBoundaryCommits";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"LogIncludeWorkingTreeChanges";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"LogShowSuperProjectSubmodulePointer";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"MaxRefHistoryItems";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 5;

	settings[i].sName   = L"Msys2Hack";
	settings[i].type    = CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b = false;

	settings[i].sName	= L"NamedRemoteFetchAll";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"NoSortLocalBranchesFirst";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"NumDiffWarning";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 10;

	settings[i].sName	= L"OverlaysCaseSensitive";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ProgressDlgLinesLimit";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 50000;

	settings[i].sName	= L"ReaddUnselectedAddedFilesAfterCommit";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"RefreshFileListAfterResolvingConflict";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"RememberFileListPosition";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"SanitizeCommitMsg";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ScintillaDirect2D";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"ShellMenuAccelerators";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ShowContextMenuIcons";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ShowAppContextMenuIcons";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ShowListBackgroundImage";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ShowListFullPathTooltip";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"SquashDate";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 0;

	settings[i].sName	= L"StyleCommitMessages";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"TGitCacheCheckContentMaxSize";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l	= 10 * 1024;

    settings[i].sName	= L"UseCustomWordBreak";
	settings[i].type	= CSettingsAdvanced::SettingTypeNumber;
	settings[i++].def.l = 2;

	settings[i].sName	= L"UseLibgit2";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"VersionCheck";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"VersionCheckPreview";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"";
	settings[i].type	= CSettingsAdvanced::SettingTypeNone;
	settings[i++].def.b	= false;
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

	SetWindowTheme(m_ListCtrl.GetSafeHwnd(), L"Explorer", nullptr);

	CString temp;
	temp.LoadString(IDS_SETTINGS_CONF_VALUECOL);
	m_ListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_SETTINGS_CONF_NAMECOL);
	m_ListCtrl.InsertColumn(1, temp);

	m_ListCtrl.SetRedraw(FALSE);

	int i = 0;
	while (settings[i].type != SettingTypeNone)
	{
		m_ListCtrl.InsertItem(i, settings[i].sName);
		m_ListCtrl.SetItemText(i, 1, settings[i].sName);
		switch (settings[i].type)
		{
		case SettingTypeBoolean:
			{
				CRegDWORD s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.b);
				m_ListCtrl.SetItemText(i, 0, DWORD(s) ?	L"true" : L"false");
			}
			break;
		case SettingTypeNumber:
			{
				CRegDWORD s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.l);
				temp.Format(L"%ld", static_cast<DWORD>(s));
				m_ListCtrl.SetItemText(i, 0, temp);
			}
			break;
		case SettingTypeString:
			{
				CRegString s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.s);
				m_ListCtrl.SetItemText(i, 0, CString(s));
			}
		}

		++i;
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
	int i = 0;
	while (settings[i].type != SettingTypeNone)
	{
		CString	sValue = m_ListCtrl.GetItemText(i, 0);
		switch (settings[i].type)
		{
		case SettingTypeBoolean:
			{
				CRegDWORD s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.b);
				if (sValue.IsEmpty())
					s.removeValue();
				else
				{
					DWORD newValue = sValue.Compare(L"true") == 0;
					if (DWORD(s) != newValue)
						s = newValue;
				}
			}
			break;
		case SettingTypeNumber:
			{
				CRegDWORD s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.l);
				if (DWORD(_wtol(sValue)) != DWORD(s))
					s = _wtol(sValue);
			}
			break;
		case SettingTypeString:
			{
				CRegString s(L"Software\\TortoiseGit\\" + settings[i].sName, settings[i].def.s);
				if (sValue.Compare(CString(s)))
					s = sValue;
			}
		}

		++i;
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

	bool allowEdit = false;
	switch (settings[pDispInfo->item.iItem].type)
	{
	case SettingTypeBoolean:
		{
			if ((pDispInfo->item.pszText[0] == 0) ||
				(wcscmp(pDispInfo->item.pszText, L"true") == 0) ||
				(wcscmp(pDispInfo->item.pszText, L"false") == 0))
			{
				allowEdit = true;
			}
		}
		break;
	case SettingTypeNumber:
		{
			TCHAR * pChar =	pDispInfo->item.pszText;
			allowEdit = true;
			while (*pChar)
			{
				if (!_istdigit(*pChar))
				{
					allowEdit = false;
					break;
				}
				++pChar;
			}
		}
		break;
	case SettingTypeString:
		allowEdit = true;
		break;
	}

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
