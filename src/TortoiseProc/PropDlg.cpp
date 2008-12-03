// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng

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
#include "MessageBox.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "Propdlg.h"
#include "Registry.h"


IMPLEMENT_DYNAMIC(CPropDlg, CResizableStandAloneDialog)
CPropDlg::CPropDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CPropDlg::IDD, pParent)
	, m_rev(SVNRev::REV_WC)
{
}

CPropDlg::~CPropDlg()
{
}

void CPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROPERTYLIST, m_proplist);
}


BEGIN_MESSAGE_MAP(CPropDlg, CResizableStandAloneDialog)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CPropDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_proplist.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

	m_proplist.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_proplist.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROPPROPERTY);
	m_proplist.InsertColumn(0, temp);
	temp.LoadString(IDS_PROPVALUE);
	m_proplist.InsertColumn(1, temp);
	m_proplist.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_proplist.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_proplist.SetRedraw(false);

	DialogEnableWindow(IDOK, FALSE);
	if (AfxBeginThread(PropThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	AddAnchor(IDC_PROPERTYLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);
	EnableSaveRestore(_T("PropDlg"));
	return TRUE;
}

void CPropDlg::OnCancel()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		CResizableStandAloneDialog::OnCancel();
}

void CPropDlg::OnOK()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		CResizableStandAloneDialog::OnOK();
}

UINT CPropDlg::PropThreadEntry(LPVOID pVoid)
{
	return ((CPropDlg*)pVoid)->PropThread();
}

UINT CPropDlg::PropThread()
{
	SVNProperties props(m_Path, m_rev, false);

	m_proplist.SetRedraw(false);
	int row = 0;
	for (int i=0; i<props.GetCount(); ++i)
	{
		CString name = props.GetItemName(i).c_str();
		CString val;
		val = props.GetItemValue(i).c_str();

		int nFound = -1;
		do 
		{
			nFound = val.FindOneOf(_T("\r\n"));
			m_proplist.InsertItem(row, name);
			if (nFound >= 0)
				m_proplist.SetItemText(row++, 1, val.Left(nFound));
			else
				m_proplist.SetItemText(row++, 1, val);
			val = val.Mid(nFound);
			val.Trim();
			name.Empty();
		} while (!val.IsEmpty()&&(nFound>=0));
	}
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_proplist.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_proplist.SetRedraw(true);
	DialogEnableWindow(IDOK, TRUE);
	return 0;
}

BOOL CPropDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ((GetDlgItem(IDOK)->IsWindowEnabled())||(pWnd != GetDlgItem(IDC_PROPERTYLIST)))
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}
