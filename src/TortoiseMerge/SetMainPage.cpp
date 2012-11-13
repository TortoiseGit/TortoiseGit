// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "DirFileEnum.h"
#include "version.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "SetMainPage.h"
#include ".\setmainpage.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_bBackup(FALSE)
	, m_bFirstDiffOnLoad(FALSE)
	, m_nTabSize(0)
	, m_bIgnoreEOL(FALSE)
	, m_bOnePane(FALSE)
	, m_bViewLinenumbers(FALSE)
	, m_bStrikeout(FALSE)
	, m_bReloadNeeded(FALSE)
	, m_bDisplayBinDiff(TRUE)
	, m_bCaseInsensitive(FALSE)
	, m_bUTF8Default(FALSE)
{
	m_regBackup = CRegDWORD(_T("Software\\TortoiseMerge\\Backup"));
	m_regFirstDiffOnLoad = CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"), TRUE);
	m_regTabSize = CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);
	m_regOnePane = CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"));
	m_regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	m_regViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_regStrikeout = CRegDWORD(_T("Software\\TortoiseMerge\\StrikeOut"), TRUE);
	m_regFontName = CRegString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10);
	m_regDisplayBinDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_regCaseInsensitive = CRegDWORD(_T("Software\\TortoiseMerge\\CaseInsensitive"), FALSE);
	m_regUTF8Default = CRegDWORD(_T("Software\\TortoiseMerge\\UseUTF8"), FALSE);

	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_nTabSize = m_regTabSize;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_nIgnoreWS = m_regIgnoreWS;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bStrikeout = m_regStrikeout;
	m_bDisplayBinDiff = m_regDisplayBinDiff;
	m_bCaseInsensitive = m_regCaseInsensitive;
	m_bUTF8Default = m_regUTF8Default;
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_BACKUP, m_bBackup);
	DDX_Check(pDX, IDC_FIRSTDIFFONLOAD, m_bFirstDiffOnLoad);
	DDX_Text(pDX, IDC_TABSIZE, m_nTabSize);
	DDV_MinMaxInt(pDX, m_nTabSize, 1, 1000);
	DDX_Check(pDX, IDC_IGNORELF, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_ONEPANE, m_bOnePane);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Check(pDX, IDC_LINENUMBERS, m_bViewLinenumbers);
	DDX_Check(pDX, IDC_STRIKEOUT, m_bStrikeout);
	DDX_Check(pDX, IDC_USEBDIFF, m_bDisplayBinDiff);
	DDX_Check(pDX, IDC_CASEINSENSITIVE, m_bCaseInsensitive);
	DDX_Check(pDX, IDC_UTF8DEFAULT, m_bUTF8Default);
}

void CSetMainPage::SaveData()
{
	m_regBackup = m_bBackup;
	m_regFirstDiffOnLoad = m_bFirstDiffOnLoad;
	m_regTabSize = m_nTabSize;
	m_regIgnoreEOL = m_bIgnoreEOL;
	m_regOnePane = m_bOnePane;
	m_regIgnoreWS = m_nIgnoreWS;
	m_regFontName = m_sFontName;
	m_regFontSize = m_dwFontSize;
	m_regViewLinenumbers = m_bViewLinenumbers;
	m_regStrikeout = m_bStrikeout;
	m_regDisplayBinDiff = m_bDisplayBinDiff;
	m_regCaseInsensitive = m_bCaseInsensitive;
	m_regUTF8Default = m_bUTF8Default;
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();
	if (m_cFontNames.GetSelFont())
		m_sFontName = m_cFontNames.GetSelFont()->m_strName;
	else
		m_sFontName = m_regFontName;
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

BOOL CSetMainPage::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	CPropertyPage::OnInitDialog();

	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_nTabSize = m_regTabSize;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_nIgnoreWS = m_regIgnoreWS;
	m_bDisplayBinDiff = m_regDisplayBinDiff;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bStrikeout = m_regStrikeout;
	m_bCaseInsensitive = m_regCaseInsensitive;
	m_bUTF8Default = m_regUTF8Default;

	UINT uRadio = IDC_WSCOMPARE;
	switch (m_nIgnoreWS)
	{
	case 0:
		uRadio = IDC_WSCOMPARE;
		break;
	case 1:
		uRadio = IDC_WSIGNOREALL;
		break;
	case 2:
		uRadio = IDC_WSIGNORECHANGED;
		break;
	default:
		break;
	}

	CheckRadioButton(IDC_WSCOMPARE, IDC_WSIGNORECHANGED, uRadio);

	CString temp;
	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(_T("%d"), i);
		m_cFontSizes.AddString(temp);
		m_cFontSizes.SetItemData(count++, i);
	}
	BOOL foundfont = FALSE;
	for (int i=0; i<m_cFontSizes.GetCount(); i++)
	{
		if (m_cFontSizes.GetItemData(i) == m_dwFontSize)
		{
			m_cFontSizes.SetCurSel(i);
			foundfont = TRUE;
		}
	}
	if (!foundfont)
	{
		temp.Format(_T("%d"), m_dwFontSize);
		m_cFontSizes.SetWindowText(temp);
	}
	m_cFontNames.Setup(DEVICE_FONTTYPE|RASTER_FONTTYPE|TRUETYPE_FONTTYPE, 1, FIXED_PITCH);
	m_cFontNames.SelectFont(m_sFontName);

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BACKUP, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_IGNORELF, &CSetMainPage::OnModifiedWithReload)
	ON_BN_CLICKED(IDC_ONEPANE, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_FIRSTDIFFONLOAD, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_WSCOMPARE, &CSetMainPage::OnBnClickedWhitespace)
	ON_BN_CLICKED(IDC_WSIGNORECHANGED, &CSetMainPage::OnBnClickedWhitespace)
	ON_BN_CLICKED(IDC_WSIGNOREALL, &CSetMainPage::OnBnClickedWhitespace)
	ON_BN_CLICKED(IDC_LINENUMBERS, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_STRIKEOUT, &CSetMainPage::OnModified)
	ON_EN_CHANGE(IDC_TABSIZE, &CSetMainPage::OnModified)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, &CSetMainPage::OnModified)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_USEBDIFF, &CSetMainPage::OnModifiedWithReload)
	ON_BN_CLICKED(IDC_CASEINSENSITIVE, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_UTF8DEFAULT, &CSetMainPage::OnModified)
END_MESSAGE_MAP()


// CSetMainPage message handlers

void CSetMainPage::OnModified()
{
	SetModified();
}

void CSetMainPage::OnModifiedWithReload()
{
	m_bReloadNeeded = TRUE;
	SetModified();
}

void CSetMainPage::OnBnClickedWhitespace()
{
	m_bReloadNeeded = TRUE;
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNORECHANGED);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORECHANGED:
		m_nIgnoreWS = 2;
		break;
	default:
		break;
	}
}

