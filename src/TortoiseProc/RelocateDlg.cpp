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
#include "RelocateDlg.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CRelocateDlg, CResizableStandAloneDialog)
CRelocateDlg::CRelocateDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRelocateDlg::IDD, pParent)
	, m_sToUrl(_T(""))
	, m_sFromUrl(_T(""))
{
}

CRelocateDlg::~CRelocateDlg()
{
}

void CRelocateDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOURL, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CRelocateDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL CRelocateDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS"), _T("url"));
	m_URLCombo.SetCurSel(0);

	RECT rect;
	GetWindowRect(&rect);
	m_height = rect.bottom - rect.top;

	AddAnchor(IDC_TOURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	SetDlgItemText(IDC_FROMURL, m_sFromUrl);
	m_URLCombo.SetWindowText(m_sFromUrl);
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RelocateDlg"));
	return TRUE;
}

void CRelocateDlg::OnBnClickedBrowse()
{
	SVNRev rev(SVNRev::REV_HEAD);
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

void CRelocateDlg::OnOK()
{
	UpdateData(TRUE);
	m_URLCombo.SaveHistory();
	m_sToUrl = m_URLCombo.GetString();
	UpdateData(FALSE);

	CResizableStandAloneDialog::OnOK();
}

void CRelocateDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CRelocateDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + m_height;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - m_height;
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}
