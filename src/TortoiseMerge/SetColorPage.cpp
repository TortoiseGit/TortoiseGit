// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2008, 2012-2014, 2017 - TortoiseSVN
// Copyright (C) 2016, 2019 - TortoiseGit

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
#include "SetColorPage.h"
#include "DiffColors.h"


// CSetColorPage dialog

IMPLEMENT_DYNAMIC(CSetColorPage, CPropertyPage)
CSetColorPage::CSetColorPage()
	: CPropertyPage(CSetColorPage::IDD)
	, m_bReloadNeeded(FALSE)
	, m_regInlineAdded(L"Software\\TortoiseGitMerge\\InlineAdded", INLINEADDED_COLOR)
	, m_regInlineRemoved(L"Software\\TortoiseGitMerge\\InlineRemoved", INLINEREMOVED_COLOR)
	, m_regModifiedBackground(L"Software\\TortoiseGitMerge\\Colors\\ColorModifiedB", MODIFIED_COLOR)
	, m_bInit(false)
{
}

CSetColorPage::~CSetColorPage()
{
	m_bInit = FALSE;
}

static COLORREF GetColorFromButton(const CMFCColorButton& button)
{
	COLORREF col = button.GetColor();
	if (col == -1)
		return button.GetAutomaticColor();
	return col;
}

void CSetColorPage::SaveData()
{
	if (!m_bInit)
		return;

	COLORREF cFg = GetColorFromButton(m_cFgNormal);
	COLORREF cBk = GetColorFromButton(m_cBkNormal);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_NORMAL, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_UNKNOWN, cBk, cFg);

	cFg = GetColorFromButton(m_cFgRemoved);
	cBk = GetColorFromButton(m_cBkRemoved);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_REMOVED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_IDENTICALREMOVED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_THEIRSREMOVED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_YOURSREMOVED, cBk, cFg);

	cFg = GetColorFromButton(m_cFgAdded);
	cBk = GetColorFromButton(m_cBkAdded);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_ADDED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_IDENTICALADDED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_THEIRSADDED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_YOURSADDED, cBk, cFg);

	if (m_regInlineAdded != m_cBkInlineAdded.GetColor())
		m_bReloadNeeded = true;
	m_regInlineAdded = GetColorFromButton(m_cBkInlineAdded);
	if (m_regInlineRemoved != m_cBkInlineRemoved.GetColor())
		m_bReloadNeeded = true;
	m_regInlineRemoved = GetColorFromButton(m_cBkInlineRemoved);
	if (m_regModifiedBackground != m_cBkModified.GetColor())
		m_bReloadNeeded = true;
	m_regModifiedBackground = GetColorFromButton(m_cBkModified);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_EDITED, m_regModifiedBackground, cFg);

	cFg = GetColorFromButton(m_cFgEmpty);
	cBk = GetColorFromButton(m_cBkEmpty);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_EMPTY, cBk, cFg);

	COLORREF adjustedcolor = cBk;
	cFg = GetColorFromButton(m_cFgConflict);
	cBk = GetColorFromButton(m_cBkConflict);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_CONFLICTED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_CONFLICTED_IGNORED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_CONFLICTADDED, cBk, cFg);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_CONFLICTEMPTY, adjustedcolor, cFg);

	cFg = GetColorFromButton(m_cFgConflictResolved);
	cBk = GetColorFromButton(m_cBkConflictResolved);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_CONFLICTRESOLVED, cBk, cFg);

	cFg = GetColorFromButton(m_cFgWhitespaces);
	CRegDWORD regWhitespaceColor(L"Software\\TortoiseGitMerge\\Colors\\Whitespace", GetSysColor(COLOR_3DSHADOW));
	regWhitespaceColor = cFg;

	cBk = GetColorFromButton(m_cBkFiltered);
	CDiffColors::GetInstance().SetColors(DIFFSTATE_FILTEREDDIFF, cBk, DIFFSTATE_NORMAL_DEFAULT_FG);
}

void CSetColorPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BKNORMAL, m_cBkNormal);
	DDX_Control(pDX, IDC_BKREMOVED, m_cBkRemoved);
	DDX_Control(pDX, IDC_BKADDED, m_cBkAdded);
	DDX_Control(pDX, IDC_BKWHITESPACES, m_cBkInlineAdded);
	DDX_Control(pDX, IDC_BKWHITESPACEDIFF, m_cBkInlineRemoved);
	DDX_Control(pDX, IDC_BKMODIFIED, m_cBkModified);
	DDX_Control(pDX, IDC_BKEMPTY, m_cBkEmpty);
	DDX_Control(pDX, IDC_BKCONFLICTED, m_cBkConflict);
	DDX_Control(pDX, IDC_BKCONFLICTRESOLVED, m_cBkConflictResolved);
	DDX_Control(pDX, IDC_FGWHITESPACES, m_cFgWhitespaces);
	DDX_Control(pDX, IDC_FGNORMAL, m_cFgNormal);
	DDX_Control(pDX, IDC_FGREMOVED, m_cFgRemoved);
	DDX_Control(pDX, IDC_FGADDED, m_cFgAdded);
	DDX_Control(pDX, IDC_FGEMPTY, m_cFgEmpty);
	DDX_Control(pDX, IDC_FGCONFLICTED, m_cFgConflict);
	DDX_Control(pDX, IDC_FGCONFLICTRESOLVED, m_cFgConflictResolved);
	DDX_Control(pDX, IDC_BKFILTERED, m_cBkFiltered);
}


BEGIN_MESSAGE_MAP(CSetColorPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BKNORMAL, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKREMOVED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKADDED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKWHITESPACES, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKWHITESPACEDIFF, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKMODIFIED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKEMPTY, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKCONFLICTED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKCONFLICTRESOLVED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGNORMAL, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGREMOVED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGADDED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGEMPTY, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGCONFLICTED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGCONFLICTRESOLVED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FGWHITESPACES, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BKFILTERED, &CSetColorPage::OnBnClickedColor)
	ON_BN_CLICKED(IDC_RESTORE, &CSetColorPage::OnBnClickedRestore)
END_MESSAGE_MAP()


// CSetColorPage message handlers

BOOL CSetColorPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	COLORREF cBk;
	COLORREF cFg;

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, cBk, cFg);
	m_cFgNormal.SetColor(cFg);
	m_cFgNormal.EnableAutomaticButton(sDefaultText, DIFFSTATE_NORMAL_DEFAULT_FG);
	m_cFgNormal.EnableOtherButton(sCustomText);
	m_cBkNormal.SetColor(cBk);
	m_cBkNormal.EnableAutomaticButton(sDefaultText, DIFFSTATE_NORMAL_DEFAULT_BG);
	m_cBkNormal.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_REMOVED, cBk, cFg);
	m_cFgRemoved.SetColor(cFg);
	m_cFgRemoved.EnableAutomaticButton(sDefaultText, DIFFSTATE_REMOVED_DEFAULT_FG);
	m_cFgRemoved.EnableOtherButton(sCustomText);
	m_cBkRemoved.SetColor(cBk);
	m_cBkRemoved.EnableAutomaticButton(sDefaultText, DIFFSTATE_REMOVED_DEFAULT_BG);
	m_cBkRemoved.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_ADDED, cBk, cFg);
	m_cFgAdded.SetColor(cFg);
	m_cFgAdded.EnableAutomaticButton(sDefaultText, DIFFSTATE_ADDED_DEFAULT_FG);
	m_cFgAdded.EnableOtherButton(sCustomText);
	m_cBkAdded.SetColor(cBk);
	m_cBkAdded.EnableAutomaticButton(sDefaultText, DIFFSTATE_ADDED_DEFAULT_BG);
	m_cBkAdded.EnableOtherButton(sCustomText);

	m_cBkInlineAdded.SetColor(m_regInlineAdded);
	m_cBkInlineAdded.EnableAutomaticButton(sDefaultText, INLINEADDED_COLOR);
	m_cBkInlineAdded.EnableOtherButton(sCustomText);

	m_cBkInlineRemoved.SetColor(m_regInlineRemoved);
	m_cBkInlineRemoved.EnableAutomaticButton(sDefaultText, INLINEREMOVED_COLOR);
	m_cBkInlineRemoved.EnableOtherButton(sCustomText);

	m_cBkModified.SetColor(m_regModifiedBackground);
	m_cBkModified.EnableAutomaticButton(sDefaultText, MODIFIED_COLOR);
	m_cBkModified.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_EMPTY, cBk, cFg);
	m_cFgEmpty.SetColor(cFg);
	m_cFgEmpty.EnableAutomaticButton(sDefaultText, DIFFSTATE_EMPTY_DEFAULT_FG);
	m_cFgEmpty.EnableOtherButton(sCustomText);
	m_cBkEmpty.SetColor(cBk);
	m_cBkEmpty.EnableAutomaticButton(sDefaultText, DIFFSTATE_EMPTY_DEFAULT_BG);
	m_cBkEmpty.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_CONFLICTED, cBk, cFg);
	m_cFgConflict.SetColor(cFg);
	m_cFgConflict.EnableAutomaticButton(sDefaultText, DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_cFgConflict.EnableOtherButton(sCustomText);
	m_cBkConflict.SetColor(cBk);
	m_cBkConflict.EnableAutomaticButton(sDefaultText, DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_cBkConflict.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_CONFLICTRESOLVED, cBk, cFg);
	m_cFgConflictResolved.SetColor(cFg);
	m_cFgConflictResolved.EnableAutomaticButton(sDefaultText, DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_cFgConflictResolved.EnableOtherButton(sCustomText);
	m_cBkConflictResolved.SetColor(cBk);
	m_cBkConflictResolved.EnableAutomaticButton(sDefaultText, DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_cBkConflictResolved.EnableOtherButton(sCustomText);

	CRegDWORD regWhitespaceColor(L"Software\\TortoiseGitMerge\\Colors\\Whitespace", GetSysColor(COLOR_3DSHADOW));
	m_cFgWhitespaces.SetColor(regWhitespaceColor);
	m_cFgWhitespaces.EnableAutomaticButton(sDefaultText, GetSysColor(COLOR_3DSHADOW));
	m_cFgWhitespaces.EnableOtherButton(sCustomText);

	CDiffColors::GetInstance().GetColors(DIFFSTATE_FILTEREDDIFF, cBk, cFg);
	m_cBkFiltered.SetColor(cBk);
	m_cBkFiltered.EnableAutomaticButton(sDefaultText, DIFFSTATE_FILTERED_DEFAULT_BG);
	m_cBkFiltered.EnableOtherButton(sCustomText);

	m_bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetColorPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSetColorPage::OnBnClickedColor()
{
	SetModified();
}

void CSetColorPage::OnBnClickedRestore()
{
	m_cBkNormal.SetColor(DIFFSTATE_NORMAL_DEFAULT_BG);
	m_cBkRemoved.SetColor(DIFFSTATE_REMOVED_DEFAULT_BG);
	m_cBkAdded.SetColor(DIFFSTATE_ADDED_DEFAULT_BG);
	m_cBkInlineAdded.SetColor(INLINEADDED_COLOR);
	m_cBkInlineRemoved.SetColor(INLINEREMOVED_COLOR);
	m_cBkModified.SetColor(MODIFIED_COLOR);
	m_cBkEmpty.SetColor(DIFFSTATE_EMPTY_DEFAULT_BG);
	m_cBkConflict.SetColor(DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_cBkConflictResolved.SetColor(DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
	m_cFgNormal.SetColor(DIFFSTATE_NORMAL_DEFAULT_FG);
	m_cFgRemoved.SetColor(DIFFSTATE_REMOVED_DEFAULT_FG);
	m_cFgAdded.SetColor(DIFFSTATE_ADDED_DEFAULT_FG);
	m_cFgEmpty.SetColor(DIFFSTATE_EMPTY_DEFAULT_FG);
	m_cFgConflict.SetColor(DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_cFgConflictResolved.SetColor(DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
	m_cFgWhitespaces.SetColor(GetSysColor(COLOR_3DSHADOW));
	m_cBkFiltered.SetColor(DIFFSTATE_FILTERED_DEFAULT_BG);
	SetModified();
}
