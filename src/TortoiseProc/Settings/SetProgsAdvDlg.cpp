// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit
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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "ToolAssocDlg.h"
#include "SetProgsAdvDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSetProgsAdvDlg, CResizableStandAloneDialog)
CSetProgsAdvDlg::CSetProgsAdvDlg(const CString& type, CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CSetProgsAdvDlg::IDD, pParent)
	, m_sType(type)
	, m_regToolKey(L"Software\\TortoiseGit\\" + type + L"Tools")
	, m_ToolsValid(false)
{
}

CSetProgsAdvDlg::~CSetProgsAdvDlg()
{
}

void CSetProgsAdvDlg::LoadData()
{
	if (!m_ToolsValid)
	{
		m_Tools.clear();

		CStringList values;
		if (m_regToolKey.getValues(values))
		{
			for (POSITION pos = values.GetHeadPosition(); pos; )
			{
				CString ext = values.GetNext(pos);
				m_Tools[ext] = CRegString(m_regToolKey.m_path + L'\\' + ext);
			}
		}

		m_ToolsValid = true;
	}
}

int CSetProgsAdvDlg::SaveData()
{
	if (m_ToolsValid)
	{
		// Remove all registry values which are no longer in the list
		CStringList values;
		if (m_regToolKey.getValues(values))
		{
			for (POSITION pos = values.GetHeadPosition(); pos; )
			{
				CString ext = values.GetNext(pos);
				if (m_Tools.find(ext) == m_Tools.end())
				{
					CRegString to_remove(m_regToolKey.m_path + L'\\' + ext);
					to_remove.removeValue();
				}
			}
		}

		// Add or update new or changed values
		for (auto it = m_Tools.cbegin(); it != m_Tools.cend() ; ++it)
		{
			CString ext = it->first;
			CString new_value = it->second;
			CRegString reg_value(m_regToolKey.m_path + L'\\' + ext);
			if (reg_value != new_value)
				reg_value = new_value;
		}
	}
	return 0;
}

void CSetProgsAdvDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOOLLISTCTRL, m_ToolListCtrl);

	if (pDX->m_bSaveAndValidate)
	{
		m_Tools.clear();
		int count = m_ToolListCtrl.GetItemCount();
		for (int i = 0; i < count; i++)
		{
			CString ext = m_ToolListCtrl.GetItemText(i, 0);
			CString value = m_ToolListCtrl.GetItemText(i, 1);
			m_Tools[ext] = value;
		}
	}
	else
	{
		m_ToolListCtrl.DeleteAllItems();
		for (auto it = m_Tools.cbegin(); it != m_Tools.cend() ; ++it)
		{
			CString ext = it->first;
			CString value = it->second;
			AddExtension(ext, value);
		}
	}
}


BEGIN_MESSAGE_MAP(CSetProgsAdvDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_ADDTOOL, OnBnClickedAddtool)
	ON_BN_CLICKED(IDC_REMOVETOOL, OnBnClickedRemovetool)
	ON_BN_CLICKED(IDC_EDITTOOL, OnBnClickedEdittool)
	ON_NOTIFY(NM_DBLCLK, IDC_TOOLLISTCTRL, OnNMDblclkToollistctrl)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TOOLLISTCTRL, &CSetProgsAdvDlg::OnLvnItemchangedToollistctrl)
	ON_BN_CLICKED(IDC_RESTOREDEFAULTS, &CSetProgsAdvDlg::OnBnClickedRestoredefaults)
END_MESSAGE_MAP()


BOOL CSetProgsAdvDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_ToolListCtrl.SetExtendedStyle((CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | LVS_EX_DOUBLEBUFFER);

	m_ToolListCtrl.DeleteAllItems();
	int c = m_ToolListCtrl.GetHeaderCtrl()->GetItemCount() - 1;
	while (c>=0)
		m_ToolListCtrl.DeleteColumn(c--);

	SetWindowTheme(m_ToolListCtrl.GetSafeHwnd(), L"Explorer", nullptr);

	CString temp;
	temp.LoadString(IDS_PROGS_EXTCOL);
	m_ToolListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGS_TOOLCOL);
	m_ToolListCtrl.InsertColumn(1, temp);

	m_ToolListCtrl.SetRedraw(FALSE);
	for (int col = 0, maxcol = maxcol = m_ToolListCtrl.GetHeaderCtrl()->GetItemCount(); col < maxcol; ++col)
		m_ToolListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	m_ToolListCtrl.SetRedraw(TRUE);

	temp.LoadString(m_sType == L"Diff" ? IDS_DLGTITLE_ADV_DIFF : IDS_DLGTITLE_ADV_MERGE);
	SetWindowText(temp);

	LoadData();
	UpdateData(FALSE);
	EnableBtns();

	AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TOOLLISTCTRL, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_ADDTOOL, BOTTOM_LEFT);
	AddAnchor(IDC_EDITTOOL, BOTTOM_LEFT);
	AddAnchor(IDC_REMOVETOOL, BOTTOM_LEFT);
	AddAnchor(IDC_RESTOREDEFAULTS, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	return TRUE;
}

int CSetProgsAdvDlg::AddExtension(const CString& ext, const CString& tool)
{
	// Note: list control automatically sorts entries
	int index = m_ToolListCtrl.InsertItem(0, ext);
	if (index >= 0)
		m_ToolListCtrl.SetItemText(index, 1, tool);
	return index;
}

int CSetProgsAdvDlg::FindExtension(const CString& ext)
{
	int count = m_ToolListCtrl.GetItemCount();

	for (int i = 0; i < count; i++)
	{
		if (m_ToolListCtrl.GetItemText(i, 0) == ext)
			return i;
	}

	return -1;
}

void CSetProgsAdvDlg::EnableBtns()
{
	bool enable_btns = m_ToolListCtrl.GetSelectedCount() > 0;
	GetDlgItem(IDC_EDITTOOL)->EnableWindow(enable_btns);
	GetDlgItem(IDC_REMOVETOOL)->EnableWindow(enable_btns);
}


// CSetProgsPage message handlers

void CSetProgsAdvDlg::OnBnClickedAddtool()
{
	CToolAssocDlg dlg(m_sType, true);
	if (dlg.DoModal() == IDOK)
	{
		int index = AddExtension(dlg.m_sExtension, dlg.m_sTool);
		m_ToolListCtrl.SetItemState(index, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
		m_ToolListCtrl.SetSelectionMark(index);
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedEdittool()
{
	int selected = m_ToolListCtrl.GetSelectionMark();
	if (selected >= 0)
	{
		CToolAssocDlg dlg(m_sType, false);
		dlg.m_sExtension = m_ToolListCtrl.GetItemText(selected, 0);
		dlg.m_sTool = m_ToolListCtrl.GetItemText(selected, 1);
		if (dlg.DoModal() == IDOK)
		{
			if (m_ToolListCtrl.DeleteItem(selected))
			{
				selected = AddExtension(dlg.m_sExtension, dlg.m_sTool);
				m_ToolListCtrl.SetItemState(selected, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
				m_ToolListCtrl.SetSelectionMark(selected);
			}
		}
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedRemovetool()
{
	int selected = m_ToolListCtrl.GetSelectionMark();
	if (selected >= 0)
	{
		m_ToolListCtrl.SetRedraw(FALSE);
		if (m_ToolListCtrl.DeleteItem(selected))
		{
			if (m_ToolListCtrl.GetItemCount() <= selected)
				--selected;
			if (selected >= 0)
			{
				m_ToolListCtrl.SetItemState(selected, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
				m_ToolListCtrl.SetSelectionMark(selected);
			}
		}
		m_ToolListCtrl.SetRedraw(TRUE);
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnNMDblclkToollistctrl(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	OnBnClickedEdittool();
	*pResult = 0;
}

void CSetProgsAdvDlg::OnLvnItemchangedToollistctrl(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	EnableBtns();

	*pResult = 0;
}

void CSetProgsAdvDlg::OnBnClickedRestoredefaults()
{
	CAppUtils::SetupDiffScripts(true, m_sType);
	m_ToolsValid = FALSE;
	LoadData();
	UpdateData(FALSE);
	EnableBtns();
}
