// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014-2016, 2019 - TortoiseGit
// Copyright (C) 2014, 2018 - TortoiseSVN

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
#include "SettingsTUDiff.h"

// CSettingsUDiff dialog

IMPLEMENT_DYNAMIC(CSettingsUDiff, ISettingsPropPage)

CSettingsUDiff::CSettingsUDiff()
: ISettingsPropPage(CSettingsUDiff::IDD)
, m_dwFontSize(0)
, m_dwTabSize(4)
{
	m_regForeCommandColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForeCommandColor", UDIFF_COLORFORECOMMAND);
	m_regForePositionColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForePositionColor", UDIFF_COLORFOREPOSITION);
	m_regForeHeaderColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForeHeaderColor", UDIFF_COLORFOREHEADER);
	m_regForeCommentColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForeCommentColor", UDIFF_COLORFORECOMMENT);
	m_regForeAddedColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForeAddedColor", UDIFF_COLORFOREADDED);
	m_regForeRemovedColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffForeRemovedColor", UDIFF_COLORFOREREMOVED);

	m_regBackCommandColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackCommandColor", UDIFF_COLORBACKCOMMAND);
	m_regBackPositionColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackPositionColor", UDIFF_COLORBACKPOSITION);
	m_regBackHeaderColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackHeaderColor", UDIFF_COLORBACKHEADER);
	m_regBackCommentColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackCommentColor", UDIFF_COLORBACKCOMMENT);
	m_regBackAddedColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackAddedColor", UDIFF_COLORBACKADDED);
	m_regBackRemovedColor = CRegDWORD(L"Software\\TortoiseGit\\UDiffBackRemovedColor", UDIFF_COLORBACKREMOVED);

	m_regFontName = CRegString(L"Software\\TortoiseGit\\UDiffFontName", L"Consolas");
	m_regFontSize = CRegDWORD(L"Software\\TortoiseGit\\UDiffFontSize", 10);
	m_regTabSize = CRegDWORD(L"Software\\TortoiseGit\\UDiffTabSize", 4);
}

CSettingsUDiff::~CSettingsUDiff()
{
}

void CSettingsUDiff::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FORECOMMANDCOLOR, m_cForeCommandColor);
	DDX_Control(pDX, IDC_FOREPOSITIONCOLOR, m_cForePositionColor);
	DDX_Control(pDX, IDC_FOREHEADERCOLOR, m_cForeHeaderColor);
	DDX_Control(pDX, IDC_FORECOMMENTCOLOR, m_cForeCommentColor);
	DDX_Control(pDX, IDC_FOREADDEDCOLOR, m_cForeAddedColor);
	DDX_Control(pDX, IDC_FOREREMOVEDCOLOR, m_cForeRemovedColor);

	DDX_Control(pDX, IDC_BACKCOMMANDCOLOR, m_cBackCommandColor);
	DDX_Control(pDX, IDC_BACKPOSITIONCOLOR, m_cBackPositionColor);
	DDX_Control(pDX, IDC_BACKHEADERCOLOR, m_cBackHeaderColor);
	DDX_Control(pDX, IDC_BACKCOMMENTCOLOR, m_cBackCommentColor);
	DDX_Control(pDX, IDC_BACKADDEDCOLOR, m_cBackAddedColor);
	DDX_Control(pDX, IDC_BACKREMOVEDCOLOR, m_cBackRemovedColor);

	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = static_cast<DWORD>(m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel()));
	if ((m_dwFontSize == 0) || (m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _wtoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Text(pDX, IDC_TABSIZE, m_dwTabSize);
}

BEGIN_MESSAGE_MAP(CSettingsUDiff, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, &CSettingsUDiff::OnBnClickedRestore)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, &CSettingsUDiff::OnChange)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, &CSettingsUDiff::OnChange)
	ON_EN_CHANGE(IDC_TABSIZE, &CSettingsUDiff::OnChange)
	ON_BN_CLICKED(IDC_FORECOMMANDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FOREPOSITIONCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FOREHEADERCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FORECOMMENTCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FOREADDEDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FOREREMOVEDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKCOMMANDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKPOSITIONCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKHEADERCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKCOMMENTCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKADDEDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_BN_CLICKED(IDC_BACKREMOVEDCOLOR, &CSettingsUDiff::OnBnClickedColor)
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()

// CSettingsUDiff message handlers

BOOL CSettingsUDiff::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	ISettingsPropPage::OnInitDialog();

	m_cForeCommandColor.SetColor(m_regForeCommandColor);
	m_cForePositionColor.SetColor(m_regForePositionColor);
	m_cForeHeaderColor.SetColor(m_regForeHeaderColor);
	m_cForeCommentColor.SetColor(m_regForeCommentColor);
	m_cForeAddedColor.SetColor(m_regForeAddedColor);
	m_cForeRemovedColor.SetColor(m_regForeRemovedColor);

	m_cBackCommandColor.SetColor(m_regBackCommandColor);
	m_cBackPositionColor.SetColor(m_regBackPositionColor);
	m_cBackHeaderColor.SetColor(m_regBackHeaderColor);
	m_cBackCommentColor.SetColor(m_regBackCommentColor);
	m_cBackAddedColor.SetColor(m_regBackAddedColor);
	m_cBackRemovedColor.SetColor(m_regBackRemovedColor);

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
	m_cForeCommandColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFORECOMMAND);
	m_cForeCommandColor.EnableOtherButton(sCustomText);
	m_cForePositionColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFOREPOSITION);
	m_cForePositionColor.EnableOtherButton(sCustomText);
	m_cForeHeaderColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFOREHEADER);
	m_cForeHeaderColor.EnableOtherButton(sCustomText);
	m_cForeCommentColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFORECOMMENT);
	m_cForeCommentColor.EnableOtherButton(sCustomText);
	m_cForeAddedColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFOREADDED);
	m_cForeAddedColor.EnableOtherButton(sCustomText);
	m_cForeRemovedColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORFOREREMOVED);
	m_cForeRemovedColor.EnableOtherButton(sCustomText);

	m_cBackCommandColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKCOMMAND);
	m_cBackCommandColor.EnableOtherButton(sCustomText);
	m_cBackPositionColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKPOSITION);
	m_cBackPositionColor.EnableOtherButton(sCustomText);
	m_cBackHeaderColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKHEADER);
	m_cBackHeaderColor.EnableOtherButton(sCustomText);
	m_cBackCommentColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKCOMMENT);
	m_cBackCommentColor.EnableOtherButton(sCustomText);
	m_cBackAddedColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKADDED);
	m_cBackAddedColor.EnableOtherButton(sCustomText);
	m_cBackRemovedColor.EnableAutomaticButton(sDefaultText, UDIFF_COLORBACKREMOVED);
	m_cBackRemovedColor.EnableOtherButton(sCustomText);

	m_dwTabSize = m_regTabSize;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	int count = 0;
	CString temp;
	for (int i = 6; i<32; i = i + 2)
	{
		temp.Format(L"%d", i);
		m_cFontSizes.AddString(temp);
		m_cFontSizes.SetItemData(count++, i);
	}
	BOOL foundfont = FALSE;
	for (int i = 0; i<m_cFontSizes.GetCount(); i++)
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
	m_cFontNames.Setup(DEVICE_FONTTYPE | RASTER_FONTTYPE | TRUETYPE_FONTTYPE, 1, FIXED_PITCH);
	m_cFontNames.SelectFont(m_sFontName);
	m_cFontNames.SendMessage(CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), m_cFontSizes.GetItemHeight(-1));

	UpdateData(FALSE);
	return TRUE;
}

void CSettingsUDiff::OnChange()
{
	SetModified();
}

void CSettingsUDiff::OnBnClickedRestore()
{
	m_cForeCommandColor.SetColor(UDIFF_COLORFORECOMMAND);
	m_cForePositionColor.SetColor(UDIFF_COLORFOREPOSITION);
	m_cForeHeaderColor.SetColor(UDIFF_COLORFOREHEADER);
	m_cForeCommentColor.SetColor(UDIFF_COLORFORECOMMENT);
	m_cForeAddedColor.SetColor(UDIFF_COLORFOREADDED);
	m_cForeRemovedColor.SetColor(UDIFF_COLORFOREREMOVED);

	m_cBackCommandColor.SetColor(UDIFF_COLORBACKCOMMAND);
	m_cBackPositionColor.SetColor(UDIFF_COLORBACKPOSITION);
	m_cBackHeaderColor.SetColor(UDIFF_COLORBACKHEADER);
	m_cBackCommentColor.SetColor(UDIFF_COLORBACKCOMMENT);
	m_cBackAddedColor.SetColor(UDIFF_COLORBACKADDED);
	m_cBackRemovedColor.SetColor(UDIFF_COLORBACKREMOVED);

	SetModified(TRUE);
}

BOOL CSettingsUDiff::OnApply()
{
	UpdateData();
	if (m_cFontNames.GetSelFont())
		m_sFontName = m_cFontNames.GetSelFont()->m_strName;
	else
		m_sFontName = m_regFontName;

	Store((m_cForeCommandColor.GetColor() == -1 ? m_cForeCommandColor.GetAutomaticColor() : m_cForeCommandColor.GetColor()), m_regForeCommandColor);
	Store((m_cForePositionColor.GetColor() == -1 ? m_cForePositionColor.GetAutomaticColor() : m_cForePositionColor.GetColor()), m_regForePositionColor);
	Store((m_cForeHeaderColor.GetColor() == -1 ? m_cForeHeaderColor.GetAutomaticColor() : m_cForeHeaderColor.GetColor()), m_regForeHeaderColor);
	Store((m_cForeCommentColor.GetColor() == -1 ? m_cForeCommentColor.GetAutomaticColor() : m_cForeCommentColor.GetColor()), m_regForeCommentColor);
	Store((m_cForeAddedColor.GetColor() == -1 ? m_cForeAddedColor.GetAutomaticColor() : m_cForeAddedColor.GetColor()), m_regForeAddedColor);
	Store((m_cForeRemovedColor.GetColor() == -1 ? m_cForeRemovedColor.GetAutomaticColor() : m_cForeRemovedColor.GetColor()), m_regForeRemovedColor);

	Store((m_cBackCommandColor.GetColor() == -1 ? m_cBackCommandColor.GetAutomaticColor() : m_cBackCommandColor.GetColor()), m_regBackCommandColor);
	Store((m_cBackPositionColor.GetColor() == -1 ? m_cBackPositionColor.GetAutomaticColor() : m_cBackPositionColor.GetColor()), m_regBackPositionColor);
	Store((m_cBackHeaderColor.GetColor() == -1 ? m_cBackHeaderColor.GetAutomaticColor() : m_cBackHeaderColor.GetColor()), m_regBackHeaderColor);
	Store((m_cBackCommentColor.GetColor() == -1 ? m_cBackCommentColor.GetAutomaticColor() : m_cBackCommentColor.GetColor()), m_regBackCommentColor);
	Store((m_cBackAddedColor.GetColor() == -1 ? m_cBackAddedColor.GetAutomaticColor() : m_cBackAddedColor.GetColor()), m_regBackAddedColor);
	Store((m_cBackRemovedColor.GetColor() == -1 ? m_cBackRemovedColor.GetAutomaticColor() : m_cBackRemovedColor.GetColor()), m_regBackRemovedColor);

	Store(static_cast<LPCTSTR>(m_sFontName), m_regFontName);
	Store(m_dwFontSize, m_regFontSize);
	Store(m_dwTabSize, m_regTabSize);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsUDiff::OnBnClickedColor()
{
	SetModified();
}

void CSettingsUDiff::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
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
	ISettingsPropPage::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
