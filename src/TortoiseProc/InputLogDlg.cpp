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
#include "InputLogDlg.h"
#include "Registry.h"
#include "HistoryDlg.h"
#include "RegHistory.h"


// CInputLogDlg dialog

IMPLEMENT_DYNAMIC(CInputLogDlg, CResizableStandAloneDialog)

CInputLogDlg::CInputLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CInputLogDlg::IDD, pParent)
	, m_pProjectProperties(NULL)
{

}

CInputLogDlg::~CInputLogDlg()
{
}

void CInputLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INPUTTEXT, m_cInput);
}


BEGIN_MESSAGE_MAP(CInputLogDlg, CResizableStandAloneDialog)
	ON_EN_CHANGE(IDC_INPUTTEXT, OnEnChangeLogmessage)
	ON_BN_CLICKED(IDC_HISTORY, &CInputLogDlg::OnBnClickedHistory)
END_MESSAGE_MAP()


BOOL CInputLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

#ifdef DEBUG
	if (m_pProjectProperties == NULL)
		TRACE("InputLogDlg: project properties not set\n");
	if (m_sActionText.IsEmpty())
		TRACE("InputLogDlg: action text not set\n");
	if (m_sUUID.IsEmpty())
		TRACE("InputLogDlg: repository UUID not set\n");
#endif

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

	SetDlgItemText(IDC_ACTIONLABEL, m_sActionText);

	AddAnchor(IDC_ACTIONLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUPBOX, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(_T("InputDlg"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_INPUTTEXT)->SetFocus();
	return FALSE;
}

void CInputLogDlg::OnOK()
{
	UpdateData();
	m_sLogMsg = m_cInput.GetText();
	
	CString reg;
	reg.Format(_T("Software\\TortoiseGit\\History\\commit%s"), (LPCTSTR)m_sUUID);

	CRegHistory history;
	history.Load(reg, _T("logmsgs"));
	history.AddEntry(m_sLogMsg);
	history.Save();

	CResizableStandAloneDialog::OnOK();
}

BOOL CInputLogDlg::PreTranslateMessage(MSG* pMsg)
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

void CInputLogDlg::OnEnChangeLogmessage()
{
	UpdateOKButton();
}

void CInputLogDlg::UpdateOKButton()
{
	CString sTemp = m_cInput.GetText();
	if (((m_pProjectProperties==NULL)||(sTemp.GetLength() >= m_pProjectProperties->nMinLogSize)))
	{
		DialogEnableWindow(IDOK, TRUE);
	}
	else
	{
		DialogEnableWindow(IDOK, FALSE);
	}
}

void CInputLogDlg::OnBnClickedHistory()
{
	CString reg;
	reg.Format(_T("Software\\TortoiseGit\\History\\commit%s"), (LPCTSTR)m_sUUID);
	CRegHistory history;
	history.Load(reg, _T("logmsgs"));
	CHistoryDlg HistoryDlg;
	HistoryDlg.SetHistory(history);
	if (HistoryDlg.DoModal()==IDOK)
	{
		if (HistoryDlg.GetSelectedText().Compare(m_cInput.GetText().Left(HistoryDlg.GetSelectedText().GetLength()))!=0)
		{
			if ((m_pProjectProperties)&&(m_pProjectProperties->sLogTemplate.Compare(m_cInput.GetText())!=0))
				m_cInput.InsertText(HistoryDlg.GetSelectedText(), !m_cInput.GetText().IsEmpty());
			else
				m_cInput.SetText(HistoryDlg.GetSelectedText());
		}

		UpdateOKButton();
		GetDlgItem(IDC_INPUTTEXT)->SetFocus();
	}
}
