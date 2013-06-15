// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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
#include "WhitesFixDlg.h"
#include "WhitesFixSetupDlg.h"
#include "EncodingDlg.h"

// dialog

IMPLEMENT_DYNAMIC(CWhitesFixDlg, CDialog)

CWhitesFixDlg::CWhitesFixDlg(CWnd* pParent)
	: CDialog(CWhitesFixDlg::IDD, pParent)
	, lineendings(EOL_AUTOLINE)
	, convertSpaces(false)
	, convertTabs(false)
	, trimRight(false)
	, fixEols(false)
	, stopAsking(false)
{
}

CWhitesFixDlg::~CWhitesFixDlg()
{
}

void CWhitesFixDlg::Create(CWnd * pParent)
{
	CDialog::Create(IDD, pParent);
	ShowWindow(SW_SHOW);
	UpdateWindow();
}

void CWhitesFixDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRIM, m_trimRight);
	DDX_Control(pDX, IDC_USESPACES, m_useSpaces);
	DDX_Control(pDX, IDC_USETABS, m_useTabs);
	DDX_Control(pDX, IDC_FIXEOLS, m_fixEols);
	DDX_Control(pDX, IDC_COMBO_EOL, m_EOL);
	DDX_Control(pDX, IDC_STOPASKING, m_stopAsking);
	DDX_Control(pDX, IDC_SETUP, m_setup);
}


BEGIN_MESSAGE_MAP(CWhitesFixDlg, CDialog)
	ON_COMMAND(IDC_FIXEOLS, OnUseEolsClick)
	ON_COMMAND(IDC_USETABS, OnUseTabsClick)
	ON_COMMAND(IDC_USESPACES, OnUseSpacesClick)
	ON_COMMAND(IDC_STOPASKING, OnStopAskingClick)
	ON_COMMAND(IDC_SETUP, OnSetupClick)
END_MESSAGE_MAP()


// message handlers

void CWhitesFixDlg::OnCancel()
{
	__super::OnCancel();
}

void CWhitesFixDlg::OnOK()
{
	UpdateData();
	if (m_stopAsking.GetCheck())
	{
		Enable(false);
		convertSpaces = false;
		convertTabs = false;
		trimRight = false;
		fixEols = false;
	}
	else
	{
		convertSpaces = m_useTabs.GetCheck();
		convertTabs = m_useSpaces.GetCheck();
		trimRight = m_trimRight.GetCheck();
		fixEols = m_fixEols.GetCheck();
		lineendings = eolArray[m_EOL.GetCurSel()+1];
	}

	__super::OnOK();
}

/// Initialize Gui elements according mode
BOOL CWhitesFixDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_trimRight.SetCheck(trimRight);
	m_useSpaces.SetCheck(convertTabs);
	m_useTabs.SetCheck(convertSpaces);
	m_fixEols.SetCheck(fixEols);
	m_EOL.EnableWindow(fixEols);
	for (int i = 1; i < _countof(eolArray); i++)
	{
		m_EOL.AddString(GetEolName(eolArray[i]));
		if (lineendings == eolArray[i])
		{
			m_EOL.SetCurSel(i-1);
		}
	}

	return FALSE;
}

void CWhitesFixDlg::OnStopAskingClick()
{
	BOOL bStopAsking = m_stopAsking.GetCheck();
	m_trimRight.EnableWindow(!bStopAsking);
	m_useSpaces.EnableWindow(!bStopAsking);
	m_useTabs.EnableWindow(!bStopAsking);
	m_fixEols.EnableWindow(!bStopAsking);
	m_EOL.EnableWindow(!bStopAsking && m_fixEols.GetCheck());
	m_setup.EnableWindow(!bStopAsking);
}


// methods

void CWhitesFixDlg::Enable(bool bEnable)
{
	DWORD nMap = GetSettingsMap() & ~TMERGE_WSF_GLOBALCHECK;
	nMap |= bEnable ? TMERGE_WSF_GLOBALCHECK : 0;
	SetSettingsMap(nMap);
}

bool CWhitesFixDlg::IsEnabled()
{
	return GetSettingsMap() & TMERGE_WSF_GLOBALCHECK;
}

void CWhitesFixDlg::OnSetupClick()
{
	CWhitesFixSetupDlg dlg;
	dlg.DoModal();
}

bool CWhitesFixDlg::HasSomethingToFix()
{
	// check if any of the inconsistencies
	// are enabled for fixing

	DWORD nMap = GetSettingsMap();
	if ((nMap & TMERGE_WSF_GLOBALCHECK) == 0)
		return false;
	if ((nMap & TMERGE_WSF_ASKFIX_TABSPACE) && (convertTabs || convertSpaces))
		return true;
	if ((nMap & TMERGE_WSF_ASKFIX_TRAIL) && trimRight)
		return true;
	if ((nMap & TMERGE_WSF_ASKFIX_TRAIL) && fixEols)
		return true;

	return false;
}
