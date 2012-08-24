// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit
// Copyright (C) 2009-2011 - TortoiseSVN

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
	settings[i].sName	= L"AutocompleteRemovesExtensions";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"BlockStatus";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"CacheTrayIcon";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"ColumnsEveryWhere";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"Debug";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"DiffBlamesWithTortoiseMerge";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"FullRowSelect";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ReaddUnselectedAddedFilesAfterCommit";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ScintillaDirect2D";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"ShowContextMenuIcons";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"ShowListBackgroundImage";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"TGitCacheCheckContent";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= false;

	settings[i].sName	= L"UseLibgit2";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

	settings[i].sName	= L"VersionCheck";
	settings[i].type	= CSettingsAdvanced::SettingTypeBoolean;
	settings[i++].def.b	= true;

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
	int c = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount() - 1;
	while (c >= 0)
		m_ListCtrl.DeleteColumn(c--);

	SetWindowTheme(m_ListCtrl.GetSafeHwnd(), L"Explorer", NULL);

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
				CRegDWORD s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.b);
				m_ListCtrl.SetItemText(i, 0, DWORD(s) ?	_T("true") : _T("false"));
			}
			break;
		case SettingTypeNumber:
			{
				CRegDWORD s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.l);
				temp.Format(_T("%ld"), (DWORD)s);
				m_ListCtrl.SetItemText(i, 0, temp);
			}
			break;
		case SettingTypeString:
			{
				CRegString s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.s);
				m_ListCtrl.SetItemText(i, 0, CString(s));
			}
		}

		i++;
	}

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount() - 1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_ListCtrl.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}
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
				CRegDWORD s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.b);
				if (sValue.IsEmpty())
					s.removeValue();
				else
				{
					DWORD newValue = sValue.Compare(_T("true")) == 0;
					if (DWORD(s) != newValue)
					{
						s = newValue;
					}
				}
			}
			break;
		case SettingTypeNumber:
			{
				CRegDWORD s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.l);
				if (DWORD(_tstol(sValue)) != DWORD(s))
				{
					s = _tstol(sValue);
				}
			}
			break;
		case SettingTypeString:
			{
				CRegString s(_T("Software\\TortoiseGit\\") + settings[i].sName, settings[i].def.s);
				if (sValue.Compare(CString(s)))
				{
					s = sValue;
				}
			}
		}

		i++;
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
	if (pDispInfo->item.pszText == NULL)
		return;

	bool allowEdit = false;
	switch (settings[pDispInfo->item.iItem].type)
	{
	case SettingTypeBoolean:
		{
			if ((pDispInfo->item.pszText[0] == 0) ||
				(_tcscmp(pDispInfo->item.pszText, _T("true")) == 0) ||
				(_tcscmp(pDispInfo->item.pszText, _T("false")) == 0))
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
				pChar++;
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
