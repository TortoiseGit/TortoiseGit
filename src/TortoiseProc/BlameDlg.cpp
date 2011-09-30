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
#include "BlameDlg.h"
#include "Registry.h"
#include "AppUtils.h"

//IMPLEMENT_DYNAMIC(CBlameDlg, CStandAloneDialog)
CBlameDlg::CBlameDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CBlameDlg::IDD, pParent)
//	, StartRev(1)
//	, EndRev(0)
//	, m_sStartRev(_T("1"))
	, m_bTextView(FALSE)
	, m_bIgnoreEOL(TRUE)
	, m_bIncludeMerge(TRUE)
{
	m_regTextView = CRegDWORD(_T("Software\\TortoiseGit\\TextBlame"), FALSE);
	m_bTextView = m_regTextView;
}

CBlameDlg::~CBlameDlg()
{
}

void CBlameDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVISON_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Check(pDX, IDC_USETEXTVIEWER, m_bTextView);
	DDX_Check(pDX, IDC_IGNOREEOL2, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_INCLUDEMERGEINFO, m_bIncludeMerge);
}


BEGIN_MESSAGE_MAP(CBlameDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_END, &CBlameDlg::OnEnChangeRevisionEnd)
END_MESSAGE_MAP()



BOOL CBlameDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sWindowTitle);

	AdjustControlSize(IDC_USETEXTVIEWER);
	AdjustControlSize(IDC_IGNOREEOL);
	AdjustControlSize(IDC_COMPAREWHITESPACES);
	AdjustControlSize(IDC_IGNOREWHITESPACECHANGES);
	AdjustControlSize(IDC_IGNOREALLWHITESPACES);
	AdjustControlSize(IDC_INCLUDEMERGEINFO);

	m_bTextView = m_regTextView;
	// set head revision as default revision
//	if (EndRev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
//	else
//	{
//		m_sEndRev = EndRev.ToString();
//		UpdateData(FALSE);
//		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
//	}

	CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_IGNOREALLWHITESPACES);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

void CBlameDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	m_regTextView = m_bTextView;
//	StartRev = SVNRev(m_sStartRev);
//	EndRev = SVNRev(m_sEndRev);
//	if (!StartRev.IsValid())
//	{
///		ShowBalloon(IDC_REVISON_START, IDS_ERR_INVALIDREV);
//		return;
//	}
//	EndRev = SVNRev(m_sEndRev);
//	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = _T("HEAD");
	}
//	if (!EndRev.IsValid())
//	{
//		ShowBalloon(IDC_REVISION_END, IDS_ERR_INVALIDREV);
//		return;
//	}
	BOOL extBlame = CRegDWORD(_T("Software\\TortoiseGit\\TextBlame"), FALSE);
	if (extBlame)
		m_bTextView = true;

	int rb = GetCheckedRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES);
	switch (rb)
	{
	case IDC_IGNOREWHITESPACECHANGES:
//		m_IgnoreSpaces = svn_diff_file_ignore_space_change;
		break;
	case IDC_IGNOREALLWHITESPACES:
//		m_IgnoreSpaces = svn_diff_file_ignore_space_all;
		break;
	case IDC_COMPAREWHITESPACES:
	default:
//		m_IgnoreSpaces = svn_diff_file_ignore_space_none;
		break;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CBlameDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CBlameDlg::OnEnChangeRevisionEnd()
{
	UpdateData();
	if (m_sEndRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}
