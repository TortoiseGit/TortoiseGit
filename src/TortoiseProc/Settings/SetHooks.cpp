// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "Git.h"
#include "SetHooks.h"
#include "SetHooksAdv.h"
#include "Hooks.h"

IMPLEMENT_DYNAMIC(CSetHooks, ISettingsPropPage)

CSetHooks::CSetHooks()
	: ISettingsPropPage(CSetHooks::IDD)
{
}

CSetHooks::~CSetHooks()
{
}

void CSetHooks::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOOKLIST, m_cHookList);
}


BEGIN_MESSAGE_MAP(CSetHooks, ISettingsPropPage)
	ON_BN_CLICKED(IDC_HOOKREMOVEBUTTON, &CSetHooks::OnBnClickedRemovebutton)
	ON_BN_CLICKED(IDC_HOOKEDITBUTTON, &CSetHooks::OnBnClickedEditbutton)
	ON_BN_CLICKED(IDC_HOOKADDBUTTON, &CSetHooks::OnBnClickedAddbutton)
	ON_BN_CLICKED(IDC_HOOKCOPYBUTTON, &CSetHooks::OnBnClickedHookcopybutton)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HOOKLIST, &CSetHooks::OnLvnItemchangedHooklist)
	ON_NOTIFY(NM_DBLCLK, IDC_HOOKLIST, &CSetHooks::OnNMDblclkHooklist)
END_MESSAGE_MAP()

BOOL CSetHooks::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cHookList.SetExtendedStyle((CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_CHECKBOXES);

	// clear all previously set header columns
	m_cHookList.DeleteAllItems();
	int c = m_cHookList.GetHeaderCtrl()->GetItemCount() - 1;
	while (c>=0)
		m_cHookList.DeleteColumn(c--);

	// now set up the requested columns
	CString temp;
	// the relative path is always visible
	temp.LoadString(IDS_SETTINGS_HOOKS_TYPECOL);
	m_cHookList.InsertColumn(0, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_PATHCOL);
	m_cHookList.InsertColumn(1, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_COMMANDLINECOL);
	m_cHookList.InsertColumn(2, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_WAITCOL);
	m_cHookList.InsertColumn(3, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_SHOWCOL);
	m_cHookList.InsertColumn(4, temp);

	SetWindowTheme(m_hWnd, L"Explorer", nullptr);

	ProjectProperties pp;
	pp.ReadProps();
	CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, pp);

	RebuildHookList();

	return TRUE;
}

void CSetHooks::RebuildHookList()
{
	m_cHookList.SetRedraw(false);
	m_cHookList.DeleteAllItems();
	// fill the list control with all the hooks
	if (!CHooks::Instance().empty())
	{
		for (hookiterator it = CHooks::Instance().begin(); it != CHooks::Instance().end(); ++it)
		{
			int pos = m_cHookList.InsertItem(m_cHookList.GetItemCount(), CHooks::Instance().GetHookTypeString(it->first.htype));
			m_cHookList.SetCheck(pos, it->second.bEnabled);
			m_cHookList.SetItemText(pos, 1, it->second.bLocal ? L"local" : it->first.path.GetWinPathString());
			m_cHookList.SetItemText(pos, 2, it->second.commandline);
			m_cHookList.SetItemText(pos, 3, (it->second.bWait ? L"true" : L"false"));
			m_cHookList.SetItemText(pos, 4, (it->second.bShow ? L"show" : L"hide"));
		}
	}

	int maxcol = m_cHookList.GetHeaderCtrl()->GetItemCount() - 1;
	for (int col = 0; col <= maxcol; col++)
		m_cHookList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	m_cHookList.SetRedraw(true);
}

void CSetHooks::OnBnClickedRemovebutton()
{
	bool bNeedsRefresh = false;
	// traversing from the end to the beginning so that the indices are not skipped
	int index = m_cHookList.GetItemCount()-1;
	while (index >= 0)
	{
		if (m_cHookList.GetItemState(index, LVIS_SELECTED) & LVIS_SELECTED)
		{
			hookkey key;
			key.htype = CHooks::GetHookType(static_cast<LPCTSTR>(m_cHookList.GetItemText(index, 0)));
			key.path = CTGitPath(m_cHookList.GetItemText(index, 1));
			key.local = m_cHookList.GetItemText(index, 1).Compare(L"local") == 0;;
			if (key.local)
				key.path = g_Git.m_CurrentDir;
			CHooks::Instance().Remove(key);
			bNeedsRefresh = true;
		}
		index--;
	}
	if (bNeedsRefresh)
	{
		RebuildHookList();
		SetModified();
	}
}

void CSetHooks::OnBnClickedEditbutton()
{
	if (m_cHookList.GetSelectedCount() > 1)
		return;
	POSITION pos = m_cHookList.GetFirstSelectedItemPosition();
	if (pos)
	{
		CSetHooksAdv dlg;
		int index = m_cHookList.GetNextSelectedItem(pos);
		dlg.key.htype = CHooks::GetHookType(static_cast<LPCTSTR>(m_cHookList.GetItemText(index, 0)));
		dlg.key.path = CTGitPath(m_cHookList.GetItemText(index, 1));
		dlg.cmd.bEnabled = m_cHookList.GetCheck(index) == BST_CHECKED;
		dlg.cmd.commandline = m_cHookList.GetItemText(index, 2);
		dlg.cmd.bWait = (m_cHookList.GetItemText(index, 3).Compare(L"true") == 0);
		dlg.cmd.bShow = (m_cHookList.GetItemText(index, 4).Compare(L"show") == 0);
		dlg.cmd.bLocal = m_cHookList.GetItemText(index, 1).Compare(L"local") == 0;
		hookkey key = dlg.key;
		key.local = dlg.cmd.bLocal;
		if (key.local)
			key.path = g_Git.m_CurrentDir;
		if (dlg.DoModal() == IDOK)
		{
			CHooks::Instance().Remove(key);
			if (dlg.cmd.bLocal)
				dlg.key.path = g_Git.m_CurrentDir;
			CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow, dlg.cmd.bEnabled, dlg.cmd.bLocal);
			RebuildHookList();
			SetModified();
		}
	}
}

void CSetHooks::OnBnClickedAddbutton()
{
	CSetHooksAdv dlg;
	if (dlg.DoModal() == IDOK)
	{
		if (dlg.cmd.bLocal)
			dlg.key.path = g_Git.m_CurrentDir;
		CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow, dlg.cmd.bEnabled, dlg.cmd.bLocal);
		RebuildHookList();
		SetModified();
	}
}

void CSetHooks::OnLvnItemchangedHooklist(NMHDR* pNMHDR, LRESULT* pResult)
{
	UINT count = m_cHookList.GetSelectedCount();
	GetDlgItem(IDC_HOOKREMOVEBUTTON)->EnableWindow(count > 0);
	GetDlgItem(IDC_HOOKEDITBUTTON)->EnableWindow(count == 1);
	GetDlgItem(IDC_HOOKCOPYBUTTON)->EnableWindow(count == 1);
	*pResult = 0;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((pNMLV->uOldState == 0) || (pNMLV->uNewState == 0) || (pNMLV->uNewState & LVIS_SELECTED) || (pNMLV->uNewState & LVIS_FOCUSED) || pNMLV->iItem < 0)
		return;

	hookkey key;
	key.htype = CHooks::GetHookType(static_cast<LPCTSTR>(m_cHookList.GetItemText(pNMLV->iItem, 0)));
	key.path = CTGitPath(m_cHookList.GetItemText(pNMLV->iItem, 1));
	key.local = m_cHookList.GetItemText(pNMLV->iItem, 1).Compare(L"local") == 0;
	if (key.local)
		key.path = g_Git.m_CurrentDir;
	if (CHooks::Instance().SetEnabled(key, m_cHookList.GetCheck(pNMLV->iItem) == BST_CHECKED))
		SetModified();
}

void CSetHooks::OnNMDblclkHooklist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	OnBnClickedEditbutton();
	*pResult = 0;
}

BOOL CSetHooks::OnApply()
{
	UpdateData();
	CHooks::Instance().Save();

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetHooks::OnBnClickedHookcopybutton()
{
	if (m_cHookList.GetSelectedCount() > 1)
		return;
	POSITION pos = m_cHookList.GetFirstSelectedItemPosition();
	if (pos)
	{
		CSetHooksAdv dlg;
		int index = m_cHookList.GetNextSelectedItem(pos);
		dlg.key.htype = CHooks::GetHookType(static_cast<LPCTSTR>(m_cHookList.GetItemText(index, 0)));
		dlg.cmd.commandline = m_cHookList.GetItemText(index, 2);
		dlg.cmd.bWait = (m_cHookList.GetItemText(index, 3).Compare(L"true") == 0);
		dlg.cmd.bShow = (m_cHookList.GetItemText(index, 4).Compare(L"show") == 0);
		dlg.cmd.bEnabled = m_cHookList.GetCheck(index) == BST_CHECKED;
		dlg.cmd.bLocal = m_cHookList.GetItemText(index, 1).Compare(L"local") == 0;
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.cmd.bLocal)
				dlg.key.path = g_Git.m_CurrentDir;
			CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow, dlg.cmd.bEnabled, dlg.cmd.bLocal);
			RebuildHookList();
			SetModified();
		}
	}
}
