// TortoiseGit - a Windows shell extension for easy version control

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
#include "ToolAssocDlg.h"
#include "SetProgsAdvDlg.h"
#include "PathUtils.h"
#include "DirFileEnum.h"

IMPLEMENT_DYNAMIC(CSetProgsAdvDlg, CDialog)
CSetProgsAdvDlg::CSetProgsAdvDlg(const CString& type, CWnd* pParent /*=NULL*/)
	: CDialog(CSetProgsAdvDlg::IDD, pParent)
	, m_sType(type)
	, m_regToolKey(_T("Software\\TortoiseGit\\") + type + _T("Tools"))
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
			for (POSITION pos = values.GetHeadPosition(); pos != NULL; )
			{
				CString ext = values.GetNext(pos);
				m_Tools[ext] = CRegString(m_regToolKey.m_path + _T("\\") + ext);
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
			for (POSITION pos = values.GetHeadPosition(); pos != NULL; )
			{
				CString ext = values.GetNext(pos);
				if (m_Tools.find(ext) == m_Tools.end())
				{
					CRegString to_remove(m_regToolKey.m_path + _T("\\") + ext);
					to_remove.removeValue();
				}
			}
		}

		// Add or update new or changed values
		for (TOOL_MAP::iterator it = m_Tools.begin(); it != m_Tools.end() ; it++)
		{
			CString ext = it->first;
			CString new_value = it->second;
			CRegString reg_value(m_regToolKey.m_path + _T("\\") + ext);
			if (reg_value != new_value)
				reg_value = new_value;
		}
	}
	return 0;
}

void CSetProgsAdvDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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
		for (TOOL_MAP::iterator it = m_Tools.begin(); it != m_Tools.end() ; it++)
		{
			CString ext = it->first;
			CString value = it->second;
			AddExtension(ext, value);
		}
	}
}


BEGIN_MESSAGE_MAP(CSetProgsAdvDlg, CDialog)
	ON_BN_CLICKED(IDC_ADDTOOL, OnBnClickedAddtool)
	ON_BN_CLICKED(IDC_REMOVETOOL, OnBnClickedRemovetool)
	ON_BN_CLICKED(IDC_EDITTOOL, OnBnClickedEdittool)
	ON_NOTIFY(NM_DBLCLK, IDC_TOOLLISTCTRL, OnNMDblclkToollistctrl)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TOOLLISTCTRL, &CSetProgsAdvDlg::OnLvnItemchangedToollistctrl)
	ON_BN_CLICKED(IDC_RESTOREDEFAULTS, &CSetProgsAdvDlg::OnBnClickedRestoredefaults)
END_MESSAGE_MAP()


BOOL CSetProgsAdvDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ToolListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_ToolListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ToolListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ToolListCtrl.DeleteColumn(c--);

	SetWindowTheme(m_hWnd, L"Explorer", NULL);

	CString temp;
	temp.LoadString(IDS_PROGS_EXTCOL);
	m_ToolListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGS_TOOLCOL);
	m_ToolListCtrl.InsertColumn(1, temp);

	m_ToolListCtrl.SetRedraw(FALSE);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_ToolListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_ToolListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_ToolListCtrl.SetRedraw(TRUE);

	temp.LoadString(m_sType == _T("Diff") ? IDS_DLGTITLE_ADV_DIFF : IDS_DLGTITLE_ADV_MERGE);
	SetWindowText(temp);

	LoadData();
	UpdateData(FALSE);
	EnableBtns();
	return TRUE;
}

int CSetProgsAdvDlg::AddExtension(const CString& ext, const CString& tool)
{
	// Note: list control automatically sorts entries
	int index = m_ToolListCtrl.InsertItem(0, ext);
	if (index >= 0)
	{
		m_ToolListCtrl.SetItemText(index, 1, tool);
	}
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
	CToolAssocDlg dlg(m_sType, true);;
	dlg.m_sExtension = _T("");
	dlg.m_sTool = _T("");
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
		CToolAssocDlg dlg(m_sType, false);;
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
	// set the custom diff/merge scripts
	CString scriptsdir = CPathUtils::GetAppParentDirectory();
	scriptsdir += _T("Diff-Scripts");
	CSimpleFileFind files(scriptsdir);
	while (files.FindNextFileNoDirectories())
	{
		CString file = files.GetFilePath();
		CString filename = files.GetFileName();
		CString ext = file.Mid(file.ReverseFind('-')+1);
		ext = _T(".")+ext.Left(ext.ReverseFind('.'));
		CString kind;
		if (file.Right(3).CompareNoCase(_T("vbs"))==0)
		{
			kind = _T(" //E:vbscript");
		}
		if (file.Right(2).CompareNoCase(_T("js"))==0)
		{
			kind = _T(" //E:javascript");
		}

		if (m_sType.Compare(_T("Diff"))==0)
		{
			if (filename.Left(5).CompareNoCase(_T("diff-"))==0)
			{
				CRegString diffreg = CRegString(_T("Software\\TortoiseGit\\DiffTools\\")+ext);
				diffreg = _T("wscript.exe \"") + file + _T("\" %base %mine") + kind;
			}
		}
		else if (m_sType.Compare(_T("Merge"))==0)
		{
			if (filename.Left(6).CompareNoCase(_T("merge-"))==0)
			{
				CRegString diffreg = CRegString(_T("Software\\TortoiseGit\\MergeTools\\")+ext);
				diffreg = _T("wscript.exe \"") + file + _T("\" %merged %theirs %mine %base") + kind;
			}
		}
	}
	m_ToolsValid = FALSE;
	LoadData();
	UpdateData(FALSE);
	EnableBtns();
}
