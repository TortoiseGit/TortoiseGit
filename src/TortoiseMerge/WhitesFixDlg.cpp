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
	DDX_Control(pDX, IDC_TITLE_FIX, m_titleFix);
	DDX_Control(pDX, IDC_TITLE_SETUP, m_titleSetup);
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
	switch (m_eMode)
	{
	case FIX:
		// in fix mode copy data to public attributes
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
		break;

	case SETUP:
		// update config
		{
			DWORD nTemp = GetSettingsMap() & 1;
			nTemp |= m_useTabs.GetCheck() ? 2 : 0;
			nTemp |= (m_useTabs.GetCheck()&0x03)<<2;
			nTemp |= (m_useSpaces.GetCheck()&0x03)<<2;
			nTemp |= (m_trimRight.GetCheck()&0x03)<<4;
			nTemp |= (m_fixEols.GetCheck()&0x03)<<6;
			SetSettingsMap(nTemp);
		}
		break;
	}

	__super::OnOK();
}

/// Initialize Gui elements according mode
BOOL CWhitesFixDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

/*	CString sTitle;
	GetWindowText(sTitle);
	SetWindowText(sTitle + view);//*/

	switch (m_eMode)
	{
	case FIX:
		// fix mode
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
		m_titleSetup.ShowWindow(SW_HIDE);
		break;

	case SETUP:
		// setup mode
		m_trimRight.SetButtonStyle(BS_AUTO3STATE);
		m_useSpaces.SetButtonStyle(BS_AUTO3STATE);
		m_useTabs.SetButtonStyle(BS_AUTO3STATE);
		m_fixEols.SetButtonStyle(BS_AUTO3STATE);
		m_EOL.ShowWindow(SW_HIDE);
		m_titleFix.ShowWindow(SW_HIDE);
		m_stopAsking.ShowWindow(SW_HIDE);
		m_setup.ShowWindow(SW_HIDE);
		{
			DWORD nMap = GetSettingsMap();
			m_useTabs.SetCheck(nMap&2 ? (nMap>>2)&0x03 : 0);
			m_useSpaces.SetCheck(nMap&2 ? 0 : (nMap>>2)&0x03);
			m_trimRight.SetCheck((nMap>>4)&0x03);
			m_fixEols.SetCheck((nMap>>6)&0x03);
		}
		break;
	}

	return FALSE;
}

void CWhitesFixDlg::OnStopAskingClick()
{
	m_trimRight.EnableWindow(!m_stopAsking.GetCheck());
	m_useSpaces.EnableWindow(!m_stopAsking.GetCheck());
	m_useTabs.EnableWindow(!m_stopAsking.GetCheck());
	m_fixEols.EnableWindow(!m_stopAsking.GetCheck());
	m_EOL.EnableWindow(!m_stopAsking.GetCheck() && m_fixEols.GetCheck());
	m_setup.EnableWindow(!m_stopAsking.GetCheck());
}


// methods

INT_PTR CWhitesFixDlg::DoModalConfirmMode()
{
	DWORD nFixBeforeSaveMap = GetSettingsMap();
	/*
		Bit map
		 - 0 : Checking On / Off
		 - 1 : Prefer spaces to tabs
		 - 2 : Automatic fix of Indentation style
		 - 3 : Asking for fix of Indentation style
		 - 4 : Automatic fix of Trail enabled
		 - 5 : Asking for fix of Trail enabled
		 - 6 : Automatic fix of EOLs enabled
		 - 7 : Asking for fix of EOLs enabled
		 \note if Automatic and Asking is on Asking have priority
	*/
	if ((nFixBeforeSaveMap & (1<<0)) == 0)
	{
		// checking is disabled, stop all actions
		convertSpaces = false;
		convertTabs = false;
		trimRight = false;
		fixEols = false;
		return IDOK;
	}

	bool bBothIndentationFound = convertSpaces && convertTabs;
	bool askConvertSpaces = bBothIndentationFound && ((nFixBeforeSaveMap & 0x0A) == 0x0A);
	bool askConvertTabs = bBothIndentationFound && ((nFixBeforeSaveMap & 0x0A) == 0x08);
	bool askTrimRight = trimRight && (nFixBeforeSaveMap & (1<<5));
	bool askFixEols = fixEols && (nFixBeforeSaveMap & (1<<7));
	convertSpaces = askConvertSpaces || (bBothIndentationFound && ((nFixBeforeSaveMap & 0x06) == 0x06));
	convertTabs = askConvertTabs || (bBothIndentationFound && ((nFixBeforeSaveMap & 0x06) == 0x04));
	trimRight = askTrimRight || (trimRight && (nFixBeforeSaveMap & (1<<4)));
	fixEols = askFixEols || (fixEols && (nFixBeforeSaveMap & (1<<6)));

	// if checking for format change is enabled and coresponding change is detected show dialog
	if (askConvertSpaces
			|| askConvertTabs
			|| askTrimRight
			|| askFixEols)
	{
		m_eMode = FIX;
		return DoModal();
	}
	return IDOK;
}

INT_PTR CWhitesFixDlg::DoModalSetupMode()
{
	m_eMode = SETUP;
	return DoModal();
}

void CWhitesFixDlg::Enable(bool bEnable)
{
	DWORD nMap = GetSettingsMap() & ~1;
	nMap |= bEnable ? 1 : 0;
	SetSettingsMap(nMap);
}

bool CWhitesFixDlg::IsEnabled()
{
	return GetSettingsMap() & 1;
}
