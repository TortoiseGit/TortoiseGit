// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013-2014, 2019 - TortoiseGit
// Copyright (C) 2006-2010, 2012-2014, 2016, 2018 - TortoiseSVN

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
#include "AppUtils.h"
#include "PathUtils.h"
#include "SetMainPage.h"
#include "MainFrm.h"

// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_bBackup(FALSE)
	, m_bFirstDiffOnLoad(FALSE)
	, m_bFirstConflictOnLoad(FALSE)
	, m_bUseSpaces(FALSE)
	, m_bSmartTabChar(FALSE)
	, m_nTabSize(0)
	, m_bEnableEditorConfig(FALSE)
	, m_nContextLines(-1)
	, m_bIgnoreEOL(FALSE)
	, m_bOnePane(FALSE)
	, m_bViewLinenumbers(FALSE)
	, m_bReloadNeeded(FALSE)
	, m_bCaseInsensitive(FALSE)
	, m_bUTF8Default(FALSE)
	, m_bAutoAdd(TRUE)
	, m_nMaxInline(3000)
	, m_dwFontSize(0)
{
	m_regBackup = CRegDWORD(L"Software\\TortoiseGitMerge\\Backup");
	m_regFirstDiffOnLoad = CRegDWORD(L"Software\\TortoiseGitMerge\\FirstDiffOnLoad", TRUE);
	m_regFirstConflictOnLoad = CRegDWORD(L"Software\\TortoiseGitMerge\\FirstConflictOnLoad", TRUE);
	m_regTabMode = CRegDWORD(L"Software\\TortoiseGitMerge\\TabMode", 0);
	m_regTabSize = CRegDWORD(L"Software\\TortoiseGitMerge\\TabSize", 4);
	m_regEnableEditorConfig = CRegDWORD(L"Software\\TortoiseGitMerge\\EnableEditorConfig", FALSE);
	m_regIgnoreEOL = CRegDWORD(L"Software\\TortoiseGitMerge\\IgnoreEOL", TRUE);
	m_regOnePane = CRegDWORD(L"Software\\TortoiseGitMerge\\OnePane");
	m_regViewLinenumbers = CRegDWORD(L"Software\\TortoiseGitMerge\\ViewLinenumbers", 1);
	m_regFontName = CRegString(L"Software\\TortoiseGitMerge\\LogFontName", L"Consolas");
	m_regFontSize = CRegDWORD(L"Software\\TortoiseGitMerge\\LogFontSize", 10);
	m_regCaseInsensitive = CRegDWORD(L"Software\\TortoiseGitMerge\\CaseInsensitive", FALSE);
	m_regUTF8Default = CRegDWORD(L"Software\\TortoiseGitMerge\\UseUTF8", FALSE);
	m_regAutoAdd = CRegDWORD(L"Software\\TortoiseGitMerge\\AutoAdd", TRUE);
	m_regMaxInline = CRegDWORD(L"Software\\TortoiseGitMerge\\InlineDiffMaxLineLength", 3000);
	m_regUseRibbons = CRegDWORD(L"Software\\TortoiseGitMerge\\UseRibbons", TRUE);
	m_regContextLines = CRegDWORD(L"Software\\TortoiseGitMerge\\ContextLines", static_cast<DWORD>(-1));

	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_bFirstConflictOnLoad = m_regFirstConflictOnLoad;
	m_bUseSpaces = (m_regTabMode & TABMODE_USESPACES) ? TRUE : FALSE;
	m_bSmartTabChar = (m_regTabMode & TABMODE_SMARTINDENT) ? TRUE : FALSE;
	m_nTabSize = m_regTabSize;
	m_bEnableEditorConfig = m_regEnableEditorConfig;
	m_nContextLines = m_regContextLines;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bCaseInsensitive = m_regCaseInsensitive;
	m_bUTF8Default = m_regUTF8Default;
	m_bAutoAdd = m_regAutoAdd;
	m_nMaxInline = m_regMaxInline;
	m_bUseRibbons = m_regUseRibbons;
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_BACKUP, m_bBackup);
	DDX_Check(pDX, IDC_FIRSTDIFFONLOAD, m_bFirstDiffOnLoad);
	DDX_Check(pDX, IDC_FIRSTCONFLICTONLOAD, m_bFirstConflictOnLoad);
	DDX_Check(pDX, IDC_USESPACES, m_bUseSpaces);
	DDX_Check(pDX, IDC_SMARTTABCHAR, m_bSmartTabChar);
	DDX_Text(pDX, IDC_TABSIZE, m_nTabSize);
	DDV_MinMaxInt(pDX, m_nTabSize, 1, 1000);
	DDX_Check(pDX, IDC_ENABLEEDITORCONFIG, m_bEnableEditorConfig);
	DDX_Text(pDX, IDC_CONTEXTLINES, m_nContextLines);
	DDV_MinMaxInt(pDX, m_nContextLines, -1, 1000);
	DDX_Check(pDX, IDC_IGNORELF, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_ONEPANE, m_bOnePane);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = static_cast<DWORD>(m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel()));
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _wtoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Check(pDX, IDC_LINENUMBERS, m_bViewLinenumbers);
	DDX_Check(pDX, IDC_CASEINSENSITIVE, m_bCaseInsensitive);
	DDX_Check(pDX, IDC_UTF8DEFAULT, m_bUTF8Default);
	DDX_Check(pDX, IDC_AUTOADD, m_bAutoAdd);
	DDX_Text(pDX, IDC_MAXINLINE, m_nMaxInline);
	DDX_Check(pDX, IDC_USERIBBONS, m_bUseRibbons);
}

void CSetMainPage::SaveData()
{
	m_regBackup = m_bBackup;
	m_regFirstDiffOnLoad = m_bFirstDiffOnLoad;
	m_regFirstConflictOnLoad = m_bFirstConflictOnLoad;
	m_regTabMode = (m_bUseSpaces ? TABMODE_USESPACES : TABMODE_NONE) | (m_bSmartTabChar ? TABMODE_SMARTINDENT : TABMODE_NONE);
	m_regTabSize = m_nTabSize;
	m_regEnableEditorConfig = m_bEnableEditorConfig;
	m_regContextLines = m_nContextLines;
	m_regIgnoreEOL = m_bIgnoreEOL;
	m_regOnePane = m_bOnePane;
	m_regFontName = m_sFontName;
	m_regFontSize = m_dwFontSize;
	m_regViewLinenumbers = m_bViewLinenumbers;
	m_regCaseInsensitive = m_bCaseInsensitive;
	m_regUTF8Default = m_bUTF8Default;
	m_regAutoAdd = m_bAutoAdd;
	m_regMaxInline = m_nMaxInline;
	m_regUseRibbons = m_bUseRibbons;
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
	m_bFirstConflictOnLoad = m_regFirstConflictOnLoad;
	m_bUseSpaces = (m_regTabMode & TABMODE_USESPACES) ? TRUE : FALSE;
	m_bSmartTabChar = (m_regTabMode & TABMODE_SMARTINDENT) ? TRUE : FALSE;
	m_nTabSize = m_regTabSize;
	m_bEnableEditorConfig = m_regEnableEditorConfig;
	m_nContextLines = m_regContextLines;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bCaseInsensitive = m_regCaseInsensitive;
	m_bUTF8Default = m_regUTF8Default;
	m_bAutoAdd = m_regAutoAdd;
	m_nMaxInline = m_regMaxInline;
	m_bUseRibbons = m_regUseRibbons;

	DialogEnableWindow(IDC_FIRSTCONFLICTONLOAD, m_bFirstDiffOnLoad);

	CString temp;
	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(L"%d", i);
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
		temp.Format(L"%lu", m_dwFontSize);
		m_cFontSizes.SetWindowText(temp);
	}
	m_cFontNames.Setup(DEVICE_FONTTYPE|RASTER_FONTTYPE|TRUETYPE_FONTTYPE, 1, FIXED_PITCH);
	m_cFontNames.SelectFont(m_sFontName);

	m_cFontNames.SendMessage(CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), m_cFontSizes.GetItemHeight(-1));

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BACKUP, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_IGNORELF, &CSetMainPage::OnModifiedWithReload)
	ON_BN_CLICKED(IDC_ONEPANE, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_FIRSTDIFFONLOAD, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_FIRSTCONFLICTONLOAD, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_LINENUMBERS, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_USESPACES, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_SMARTTABCHAR, &CSetMainPage::OnModified)
	ON_EN_CHANGE(IDC_TABSIZE, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_ENABLEEDITORCONFIG, &CSetMainPage::OnModified)
	ON_EN_CHANGE(IDC_CONTEXTLINES, &CSetMainPage::OnModified)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, &CSetMainPage::OnModifiedWithReload)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, &CSetMainPage::OnModifiedWithReload)
	ON_BN_CLICKED(IDC_CASEINSENSITIVE, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_UTF8DEFAULT, &CSetMainPage::OnModified)
	ON_BN_CLICKED(IDC_AUTOADD, &CSetMainPage::OnModified)
	ON_EN_CHANGE(IDC_MAXINLINE, &CSetMainPage::OnModifiedWithReload)
	ON_BN_CLICKED(IDC_USERIBBONS, &CSetMainPage::OnModified)
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()


// CSetMainPage message handlers

void CSetMainPage::OnModified()
{
	UpdateData();
	SetModified();
	DialogEnableWindow(IDC_FIRSTCONFLICTONLOAD, m_bFirstDiffOnLoad);
}

void CSetMainPage::OnModifiedWithReload()
{
	m_bReloadNeeded = TRUE;
	SetModified();
}

BOOL CSetMainPage::DialogEnableWindow(UINT nID, BOOL bEnable)
{
	CWnd * pwndDlgItem = GetDlgItem(nID);
	if (!pwndDlgItem)
		return FALSE;
	if (bEnable)
		return pwndDlgItem->EnableWindow(bEnable);
	if (GetFocus() == pwndDlgItem)
	{
		SendMessage(WM_NEXTDLGCTL, 0, FALSE);
	}
	return pwndDlgItem->EnableWindow(bEnable);
}

void CSetMainPage::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CFont* pFont = GetFont();
	if (pFont)
	{
		CDC* pDC = GetDC();
		CFont* pFontPrev = pDC->SelectObject(pFont);
		int iborder = ::GetSystemMetrics(SM_CYBORDER);
		CSize sz = pDC->GetTextExtent(L"0");
		lpMeasureItemStruct->itemHeight = sz.cy + 2 * iborder;
		pDC->SelectObject(pFontPrev);
		ReleaseDC(pDC);
	}
	CPropertyPage::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
