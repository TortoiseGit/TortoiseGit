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
#include "InputDlg.h"
#include "Registry.h"


IMPLEMENT_DYNAMIC(CInputDlg, CResizableStandAloneDialog)
CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CInputDlg::IDD, pParent)
	, m_sInputText(_T(""))
	, m_pProjectProperties(NULL)
	, m_iCheck(0)
	, m_bUseLogWidth(true)
{
}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INPUTTEXT, m_cInput);
	DDX_Check(pDX, IDC_CHECKBOX, m_iCheck);
}


BEGIN_MESSAGE_MAP(CInputDlg, CResizableStandAloneDialog)
	ON_EN_CHANGE(IDC_INPUTTEXT, OnEnChangeLogmessage)
END_MESSAGE_MAP()

BOOL CInputDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	if (m_pProjectProperties)
		m_cInput.Init(*m_pProjectProperties);
	else
		m_cInput.Init();

	m_cInput.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));

	if (m_pProjectProperties)
	{
		if (m_pProjectProperties->nLogWidthMarker)
		{
			m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
			m_cInput.Call(SCI_SETEDGEMODE, EDGE_LINE);
			m_cInput.Call(SCI_SETEDGECOLUMN, m_pProjectProperties->nLogWidthMarker);
		}
		else
		{
			m_cInput.Call(SCI_SETEDGEMODE, EDGE_NONE);
			m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
		}
		m_cInput.SetText(m_pProjectProperties->sLogTemplate);
	}
	if (!m_sInputText.IsEmpty())
	{
		m_cInput.SetText(m_sInputText);
	}
	if (!m_sHintText.IsEmpty())
	{
		SetDlgItemText(IDC_HINTTEXT, m_sHintText);
	}
	if (!m_sTitle.IsEmpty())
	{
		this->SetWindowText(m_sTitle);
	}
	if (!m_sCheckText.IsEmpty())
	{
		SetDlgItemText(IDC_CHECKBOX, m_sCheckText);
		GetDlgItem(IDC_CHECKBOX)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_CHECKBOX)->ShowWindow(SW_HIDE);
	}

	AddAnchor(IDC_HINTTEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECKBOX, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	AdjustControlSize(IDC_CHECKBOX);

	EnableSaveRestore(_T("InputDlg"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_INPUTTEXT)->SetFocus();
	// clear the selection
	m_cInput.Call(SCI_SETSEL, (WPARAM)-1, (LPARAM)-1);
	return FALSE;
}

void CInputDlg::OnOK()
{
	UpdateData();
	m_sInputText = m_cInput.GetText();
	CResizableDialog::OnOK();
}

BOOL CInputDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CInputDlg::OnEnChangeLogmessage()
{
	CString sTemp = m_cInput.GetText();
	if ((!m_bUseLogWidth)||((m_pProjectProperties==NULL)||(sTemp.GetLength() >= m_pProjectProperties->nMinLogSize)))
	{
		DialogEnableWindow(IDOK, TRUE);
	}
	else
	{
		DialogEnableWindow(IDOK, FALSE);
	}
}
