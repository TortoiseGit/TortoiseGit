// TortoiseSVN - a Windows shell extension for easy version control

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
#include "ImportDlg.h"
#include "RepositoryBrowser.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "MessageBox.h"
#include "BrowseFolder.h"
#include "Registry.h"
#include "HistoryDlg.h"

IMPLEMENT_DYNAMIC(CImportDlg, CResizableStandAloneDialog)
CImportDlg::CImportDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CImportDlg::IDD, pParent)
	, m_bIncludeIgnored(FALSE)
{
}

CImportDlg::~CImportDlg()
{
}

void CImportDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Control(pDX, IDC_MESSAGE, m_cMessage);
	DDX_Check(pDX, IDC_IMPORTIGNORED, m_bIncludeIgnored);
}

BEGIN_MESSAGE_MAP(CImportDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_MESSAGE, OnEnChangeLogmessage)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
END_MESSAGE_MAP()

BOOL CImportDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25));

	if (m_url.IsEmpty())
	{
		m_URLCombo.SetURLHistory(TRUE);
		m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
	}
	else
	{
		m_URLCombo.SetWindowText(m_url);
		if (GetFocus() == &m_URLCombo)
			SendMessage(WM_NEXTDLGCTL, 0, FALSE);
		m_URLCombo.EnableWindow(FALSE);
	}
	m_URLCombo.SetCurSel(0);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
	
	m_History.Load(_T("Software\\TortoiseSVN\\History\\commit"), _T("logmsgs"));
	m_ProjectProperties.ReadProps(m_path);
	m_cMessage.Init(m_ProjectProperties);
	m_cMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));

	AdjustControlSize(IDC_IMPORTIGNORED);

	AddAnchor(IDC_STATIC1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC4, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_STATIC2, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_MESSAGE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_IMPORTIGNORED, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ImportDlg"));
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CImportDlg::OnOK()
{
	if (m_URLCombo.IsWindowEnabled())
	{
		m_URLCombo.SaveHistory();
		m_url = m_URLCombo.GetString();
		UpdateData();
	}

	UpdateData();
	m_sMessage = m_cMessage.GetText();
	m_History.AddEntry(m_sMessage);
	m_History.Save();

	CResizableStandAloneDialog::OnOK();
}

void CImportDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	SVNRev rev(SVNRev::REV_HEAD);
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

BOOL CImportDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
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

void CImportDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CImportDlg::OnEnChangeLogmessage()
{
    CString sTemp = m_cMessage.GetText();
	DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

void CImportDlg::OnCancel()
{
	UpdateData();
	if (m_ProjectProperties.sLogTemplate.Compare(m_cMessage.GetText()) != 0)
		m_History.AddEntry(m_cMessage.GetText());
	m_History.Save();
	CResizableStandAloneDialog::OnCancel();
}

void CImportDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	SVN svn;
	CHistoryDlg historyDlg;
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal()==IDOK)
	{
		if (historyDlg.GetSelectedText().Compare(m_cMessage.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
		{
			if (m_ProjectProperties.sLogTemplate.Compare(m_cMessage.GetText())!=0)
				m_cMessage.InsertText(historyDlg.GetSelectedText(), !m_cMessage.GetText().IsEmpty());
			else
				m_cMessage.SetText(historyDlg.GetSelectedText());
		}
		DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cMessage.GetText().GetLength());
	}

}
