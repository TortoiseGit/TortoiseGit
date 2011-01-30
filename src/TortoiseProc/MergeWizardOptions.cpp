// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardOptions.h"
#include "SVNProgressDlg.h"


IMPLEMENT_DYNAMIC(CMergeWizardOptions, CMergeWizardBasePage)

CMergeWizardOptions::CMergeWizardOptions()
	: CMergeWizardBasePage(CMergeWizardOptions::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSSUBTITLE);
}

CMergeWizardOptions::~CMergeWizardOptions()
{
}

void CMergeWizardOptions::DoDataExchange(CDataExchange* pDX)
{
	CMergeWizardBasePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREANCESTRY, ((CMergeWizard*)GetParent())->m_bIgnoreAncestry);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREEOL, ((CMergeWizard*)GetParent())->m_bIgnoreEOL);
	DDX_Check(pDX, IDC_RECORDONLY, ((CMergeWizard*)GetParent())->m_bRecordOnly);
}


BEGIN_MESSAGE_MAP(CMergeWizardOptions, CMergeWizardBasePage)
	ON_BN_CLICKED(IDC_DRYRUN, &CMergeWizardOptions::OnBnClickedDryrun)
END_MESSAGE_MAP()


BOOL CMergeWizardOptions::OnInitDialog()
{
	CMergeWizardBasePage::OnInitDialog();

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	switch (((CMergeWizard*)GetParent())->m_depth)
	{
	case svn_depth_unknown:
		m_depthCombo.SetCurSel(0);
		break;
	case svn_depth_infinity:
		m_depthCombo.SetCurSel(1);
		break;
	case svn_depth_immediates:
		m_depthCombo.SetCurSel(2);
		break;
	case svn_depth_files:
		m_depthCombo.SetCurSel(3);
		break;
	case svn_depth_empty:
		m_depthCombo.SetCurSel(4);
		break;
	default:
		m_depthCombo.SetCurSel(0);
		break;
	}

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_RECORDONLY, IDS_MERGEWIZARD_OPTIONS_RECORDONLY_TT);

	CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);

	AdjustControlSize(IDC_IGNOREANCESTRY);
	AdjustControlSize(IDC_IGNOREEOL);
	AdjustControlSize(IDC_COMPAREWHITESPACES);
	AdjustControlSize(IDC_IGNOREWHITESPACECHANGES);
	AdjustControlSize(IDC_IGNOREALLWHITESPACES);
	AdjustControlSize(IDC_RECORDONLY);

	return TRUE;
}

LRESULT CMergeWizardOptions::OnWizardBack()
{
	return ((CMergeWizard*)GetParent())->GetSecondPage();
}

BOOL CMergeWizardOptions::OnWizardFinish()
{
	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_unknown;
		break;
	case 1:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_infinity;
		break;
	case 2:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_immediates;
		break;
	case 3:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_files;
		break;
	case 4:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_empty;
		break;
	default:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_empty;
		break;
	}

	return CMergeWizardBasePage::OnWizardFinish();
}

BOOL CMergeWizardOptions::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);
	SetButtonTexts();
	GetDlgItem(IDC_RECORDONLY)->EnableWindow(((CMergeWizard*)GetParent())->nRevRangeMerge != MERGEWIZARD_REINTEGRATE);
	return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardOptions::OnBnClickedDryrun()
{
	UpdateData();
	CMergeWizard * pWizard = ((CMergeWizard*)GetParent());
	CSVNProgressDlg progDlg;
	progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
	int options = ProgOptDryRun;
	options |= pWizard->m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
	options |= pWizard->m_bRecordOnly ? ProgOptRecordOnly : 0;
	progDlg.SetOptions(options);
	progDlg.SetPathList(CTSVNPathList(pWizard->wcPath));
	progDlg.SetUrl(pWizard->URL1);
	progDlg.SetSecondUrl(pWizard->URL2);

	switch (pWizard->nRevRangeMerge)
	{
	case MERGEWIZARD_REVRANGE:
		{
			if (pWizard->revRangeArray.GetCount())
			{
				SVNRevRangeArray tempRevArray = pWizard->revRangeArray;
				tempRevArray.AdjustForMerge(!!pWizard->bReverseMerge);
				progDlg.SetRevisionRanges(tempRevArray);
			}
			else
			{
				SVNRevRangeArray tempRevArray;
				tempRevArray.AddRevRange(1, SVNRev::REV_HEAD);
				progDlg.SetRevisionRanges(tempRevArray);
			}
		}
		break;
	case MERGEWIZARD_TREE:
		{
			progDlg.SetRevision(pWizard->startRev);
			progDlg.SetRevisionEnd(pWizard->endRev);
			if (pWizard->URL1.Compare(pWizard->URL2) == 0)
			{
				SVNRevRangeArray tempRevArray;
				tempRevArray.AdjustForMerge(!!pWizard->bReverseMerge);
				tempRevArray.AddRevRange(pWizard->startRev, pWizard->endRev);
				progDlg.SetRevisionRanges(tempRevArray);
			}
		}
		break;
	case MERGEWIZARD_REINTEGRATE:
		{
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrate);
		}
		break;
	}

	progDlg.SetDepth(pWizard->m_depth);
	progDlg.SetDiffOptions(SVN::GetOptionsString(pWizard->m_bIgnoreEOL, pWizard->m_IgnoreSpaces));
	progDlg.DoModal();
}

BOOL CMergeWizardOptions::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CMergeWizardBasePage::PreTranslateMessage(pMsg);
}
