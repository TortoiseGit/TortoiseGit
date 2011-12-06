// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN
// Copyright (C) 2011 - TortoiseGit

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
#include "MessageBox.h"
#include "SettingsTBlame.h"


// CSettingsTBlame dialog

//IMPLEMENT_DYNAMIC(CSettingsTBlame, ISettingsPropPage)

CSettingsTBlame::CSettingsTBlame()
	: ISettingsPropPage(CSettingsTBlame::IDD)
	, m_dwFontSize(0)
	, m_sFontName(_T(""))
	, m_dwTabSize(4)
	, m_bFollowRenames(0)
{
	m_regNewLinesColor = CRegDWORD(_T("Software\\TortoiseGit\\BlameNewColor"), RGB(255, 230, 230));
	m_regOldLinesColor = CRegDWORD(_T("Software\\TortoiseGit\\BlameOldColor"), RGB(230, 230, 255));
	m_regFontName = CRegString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10);
	m_regTabSize = CRegDWORD(_T("Software\\TortoiseGit\\BlameTabSize"), 4);
	m_regFollowRenames = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\FollowRenames"), 0);
}

CSettingsTBlame::~CSettingsTBlame()
{
}

void CSettingsTBlame::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NEWLINESCOLOR, m_cNewLinesColor);
	DDX_Control(pDX, IDC_OLDLINESCOLOR, m_cOldLinesColor);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Text(pDX, IDC_TABSIZE, m_dwTabSize);
	DDX_Check(pDX, IDC_FOLLOWRENAMES, m_bFollowRenames);
}


BEGIN_MESSAGE_MAP(CSettingsTBlame, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnChange)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnChange)
	ON_EN_CHANGE(IDC_TABSIZE, OnChange)
	ON_BN_CLICKED(IDC_FOLLOWRENAMES, OnChange)
	ON_BN_CLICKED(IDC_NEWLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
	ON_BN_CLICKED(IDC_OLDLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
END_MESSAGE_MAP()


// CSettingsTBlame message handlers

BOOL CSettingsTBlame::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	ISettingsPropPage::OnInitDialog();

	m_cNewLinesColor.SetColor((DWORD)m_regNewLinesColor);
	m_cOldLinesColor.SetColor((DWORD)m_regOldLinesColor);

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
	m_cNewLinesColor.EnableAutomaticButton(sDefaultText, RGB(255, 230, 230));
	m_cNewLinesColor.EnableOtherButton(sCustomText);
	m_cOldLinesColor.EnableAutomaticButton(sDefaultText, RGB(230, 230, 255));
	m_cOldLinesColor.EnableOtherButton(sCustomText);

	m_dwTabSize = m_regTabSize;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bFollowRenames = m_regFollowRenames;
	int count = 0;
	CString temp;
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
	return TRUE;
}

void CSettingsTBlame::OnChange()
{
	SetModified();
}

void CSettingsTBlame::OnBnClickedRestore()
{
	m_cOldLinesColor.SetColor(RGB(230, 230, 255));
	m_cNewLinesColor.SetColor(RGB(255, 230, 230));
	SetModified(TRUE);
}

BOOL CSettingsTBlame::OnApply()
{
	UpdateData();
	if (m_cFontNames.GetSelFont())
		m_sFontName = m_cFontNames.GetSelFont()->m_strName;
	else
		m_sFontName = m_regFontName;

    Store ((m_cNewLinesColor.GetColor() == -1 ? m_cNewLinesColor.GetAutomaticColor() : m_cNewLinesColor.GetColor()), m_regNewLinesColor); 
    Store ((m_cOldLinesColor.GetColor() == -1 ? m_cOldLinesColor.GetAutomaticColor() : m_cOldLinesColor.GetColor()), m_regOldLinesColor);
    Store ((LPCTSTR)m_sFontName, m_regFontName);
    Store (m_dwFontSize, m_regFontSize);
    Store (m_dwTabSize, m_regTabSize);
	Store (m_bFollowRenames, m_regFollowRenames);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsTBlame::OnBnClickedColor()
{
	SetModified();
}
