// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2018 - TortoiseSVN
// Copyright (C) 2011-2019 - TortoiseGit

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
#include "SettingsTBlame.h"
#include "../TortoiseGitBlame/BlameIndexColors.h"
#include "../TortoiseGitBlame/BlameDetectMovedOrCopiedLines.h"


// CSettingsTBlame dialog

//IMPLEMENT_DYNAMIC(CSettingsTBlame, ISettingsPropPage)

CSettingsTBlame::CSettingsTBlame()
	: ISettingsPropPage(CSettingsTBlame::IDD)
	, m_dwFontSize(0)
	, m_dwTabSize(4)
	, m_dwDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED)
	, m_dwDetectMovedOrCopiedLinesNumCharactersWithinFile(BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE_DEFAULT)
	, m_dwDetectMovedOrCopiedLinesNumCharactersFromFiles(BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES_DEFAULT)
	, m_bIgnoreWhitespace(0)
	, m_bShowCompleteLog(0)
	, m_bFirstParent(0)
	, m_bFollowRenames(0)
{
	m_regNewLinesColor = CRegDWORD(L"Software\\TortoiseGit\\BlameNewColor", BLAMENEWCOLOR);
	m_regOldLinesColor = CRegDWORD(L"Software\\TortoiseGit\\BlameOldColor", BLAMEOLDCOLOR);
	m_regFontName = CRegString(L"Software\\TortoiseGit\\BlameFontName", L"Consolas");
	m_regFontSize = CRegDWORD(L"Software\\TortoiseGit\\BlameFontSize", 10);
	m_regTabSize = CRegDWORD(L"Software\\TortoiseGit\\BlameTabSize", 4);
	m_regDetectMovedOrCopiedLines = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\DetectMovedOrCopiedLines", BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
	m_regDetectMovedOrCopiedLinesNumCharactersWithinFile = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\DetectMovedOrCopiedLinesNumCharactersWithinFile", BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE_DEFAULT);
	m_regDetectMovedOrCopiedLinesNumCharactersFromFiles = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\DetectMovedOrCopiedLinesNumCharactersFromFiles", BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES_DEFAULT);
	m_regIgnoreWhitespace = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\IgnoreWhitespace", 0);
	m_regShowCompleteLog = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\ShowCompleteLog", 1);
	m_regFirstParent = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\OnlyFirstParent", 0);
	m_regFollowRenames = CRegDWORD(L"Software\\TortoiseGit\\TortoiseGitBlame\\Workspace\\FollowRenames", 0);
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
	m_dwFontSize = static_cast<DWORD>(m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel()));
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _wtoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Text(pDX, IDC_TABSIZE, m_dwTabSize);
	DDX_Control(pDX, IDC_DETECT_MOVED_OR_COPIED_LINES, m_cDetectMovedOrCopiedLines);
	m_dwDetectMovedOrCopiedLines = static_cast<DWORD>(m_cDetectMovedOrCopiedLines.GetItemData(m_cDetectMovedOrCopiedLines.GetCurSel()));
	if (m_dwDetectMovedOrCopiedLines == -1){
		m_dwDetectMovedOrCopiedLines = BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED;
	}
	DDX_Text(pDX, IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE, m_dwDetectMovedOrCopiedLinesNumCharactersWithinFile);
	DDX_Text(pDX, IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES, m_dwDetectMovedOrCopiedLinesNumCharactersFromFiles);
	DDX_Check(pDX, IDC_IGNORE_WHITESPACE, m_bIgnoreWhitespace);
	DDX_Check(pDX, IDC_SHOWCOMPLETELOG, m_bShowCompleteLog);
	DDX_Check(pDX, IDC_BLAME_ONLYFIRSTPARENT, m_bFirstParent);
	DDX_Check(pDX, IDC_FOLLOWRENAMES, m_bFollowRenames);
}


BEGIN_MESSAGE_MAP(CSettingsTBlame, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnChange)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnChange)
	ON_EN_CHANGE(IDC_TABSIZE, OnChange)
	ON_CBN_SELCHANGE(IDC_DETECT_MOVED_OR_COPIED_LINES, OnChange)
	ON_EN_CHANGE(IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE, OnChange)
	ON_EN_CHANGE(IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES, OnChange)
	ON_BN_CLICKED(IDC_IGNORE_WHITESPACE, OnChange)
	ON_BN_CLICKED(IDC_SHOWCOMPLETELOG, OnChange)
	ON_BN_CLICKED(IDC_BLAME_ONLYFIRSTPARENT, OnChange)
	ON_BN_CLICKED(IDC_FOLLOWRENAMES, OnChange)
	ON_BN_CLICKED(IDC_NEWLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
	ON_BN_CLICKED(IDC_OLDLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()


// CSettingsTBlame message handlers

BOOL CSettingsTBlame::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_IGNORE_WHITESPACE);
	AdjustControlSize(IDC_SHOWCOMPLETELOG);
	AdjustControlSize(IDC_BLAME_ONLYFIRSTPARENT);
	AdjustControlSize(IDC_FOLLOWRENAMES);

	m_cNewLinesColor.SetColor(m_regNewLinesColor);
	m_cOldLinesColor.SetColor(m_regOldLinesColor);

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
	m_cNewLinesColor.EnableAutomaticButton(sDefaultText, BLAMENEWCOLOR);
	m_cNewLinesColor.EnableOtherButton(sCustomText);
	m_cOldLinesColor.EnableAutomaticButton(sDefaultText, BLAMEOLDCOLOR);
	m_cOldLinesColor.EnableOtherButton(sCustomText);

	m_dwTabSize = m_regTabSize;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_dwDetectMovedOrCopiedLines = m_regDetectMovedOrCopiedLines;
	m_dwDetectMovedOrCopiedLinesNumCharactersWithinFile = m_regDetectMovedOrCopiedLinesNumCharactersWithinFile;
	m_dwDetectMovedOrCopiedLinesNumCharactersFromFiles = m_regDetectMovedOrCopiedLinesNumCharactersFromFiles;
	m_bIgnoreWhitespace = m_regIgnoreWhitespace;
	m_bShowCompleteLog = m_regShowCompleteLog;
	m_bFirstParent = m_regFirstParent;
	m_bFollowRenames = m_regFollowRenames;
	int count = 0;
	CString temp;
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

	CString sDetectMovedOrCopiedLinesDisabled;
	CString sDetectMovedOrCopiedLinesWithinFile;
	CString sDetectMovedOrCopiedLinesFromModifiedFiles;
	CString sDetectMovedOrCopiedLinesFromExistingFilesAtFileCreation;
	CString sDetectMovedOrCopiedLinesFromExistingFiles;
	sDetectMovedOrCopiedLinesDisabled.LoadString(IDS_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
	sDetectMovedOrCopiedLinesWithinFile.LoadString(IDS_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE);
	sDetectMovedOrCopiedLinesFromModifiedFiles.LoadString(IDS_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES);
	sDetectMovedOrCopiedLinesFromExistingFilesAtFileCreation.LoadString(IDS_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION);
	sDetectMovedOrCopiedLinesFromExistingFiles.LoadString(IDS_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES);

	m_cDetectMovedOrCopiedLines.AddString(sDetectMovedOrCopiedLinesDisabled);
	m_cDetectMovedOrCopiedLines.SetItemData(0, BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
	m_cDetectMovedOrCopiedLines.AddString(sDetectMovedOrCopiedLinesWithinFile);
	m_cDetectMovedOrCopiedLines.SetItemData(1, BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE);
	m_cDetectMovedOrCopiedLines.AddString(sDetectMovedOrCopiedLinesFromModifiedFiles);
	m_cDetectMovedOrCopiedLines.SetItemData(2, BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES);
	m_cDetectMovedOrCopiedLines.AddString(sDetectMovedOrCopiedLinesFromExistingFilesAtFileCreation);
	m_cDetectMovedOrCopiedLines.SetItemData(3, BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION);
	m_cDetectMovedOrCopiedLines.AddString(sDetectMovedOrCopiedLinesFromExistingFiles);
	m_cDetectMovedOrCopiedLines.SetItemData(4, BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES);
	for (int i = 0; i < m_cDetectMovedOrCopiedLines.GetCount(); ++i)
	{
		if (m_cDetectMovedOrCopiedLines.GetItemData(i) == m_dwDetectMovedOrCopiedLines)
		{
			m_cDetectMovedOrCopiedLines.SetCurSel(i);
			break;
		}
	}

	UpdateData(FALSE);
	UpdateDependencies();
	return TRUE;
}

void CSettingsTBlame::OnChange()
{
	UpdateData();
	UpdateDependencies();
	SetModified();
}

void CSettingsTBlame::OnBnClickedRestore()
{
	m_cOldLinesColor.SetColor(BLAMEOLDCOLOR);
	m_cNewLinesColor.SetColor(BLAMENEWCOLOR);
	SetModified(TRUE);
}

BOOL CSettingsTBlame::OnApply()
{
	UpdateData();
	if (m_cFontNames.GetSelFont())
		m_sFontName = m_cFontNames.GetSelFont()->m_strName;
	else
		m_sFontName = m_regFontName;

	Store((m_cNewLinesColor.GetColor() == -1 ? m_cNewLinesColor.GetAutomaticColor() : m_cNewLinesColor.GetColor()), m_regNewLinesColor);
	Store((m_cOldLinesColor.GetColor() == -1 ? m_cOldLinesColor.GetAutomaticColor() : m_cOldLinesColor.GetColor()), m_regOldLinesColor);
	Store(static_cast<LPCTSTR>(m_sFontName), m_regFontName);
	Store(m_dwFontSize, m_regFontSize);
	Store(m_dwTabSize, m_regTabSize);
	Store(m_dwDetectMovedOrCopiedLines, m_regDetectMovedOrCopiedLines);
	Store(m_dwDetectMovedOrCopiedLinesNumCharactersWithinFile, m_regDetectMovedOrCopiedLinesNumCharactersWithinFile);
	Store(m_dwDetectMovedOrCopiedLinesNumCharactersFromFiles, m_regDetectMovedOrCopiedLinesNumCharactersFromFiles);
	Store(m_bIgnoreWhitespace, m_regIgnoreWhitespace);
	Store(m_bShowCompleteLog, m_regShowCompleteLog);
	Store(m_bFirstParent, m_regFirstParent);
	Store(m_bFollowRenames, m_regFollowRenames);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSettingsTBlame::OnBnClickedColor()
{
	SetModified();
}

void CSettingsTBlame::UpdateDependencies()
{
	BOOL enableDetectMovedOrCopiedLinesNumCharactersWithinFile = FALSE;
	BOOL enableDetectMovedOrCopiedLinesNumCharactersFromFiles = FALSE;
	if (m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE)
		enableDetectMovedOrCopiedLinesNumCharactersWithinFile = TRUE;
	if (m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES || m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION || m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES)
		enableDetectMovedOrCopiedLinesNumCharactersFromFiles = TRUE;
	BOOL enableShowCompleteLog = FALSE;
	BOOL enableFollowRenames = FALSE;
	if (BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) && !m_bFirstParent)
	{
		enableShowCompleteLog = TRUE;
		if (m_bShowCompleteLog)
			enableFollowRenames = TRUE;
		else if (m_bFollowRenames)
		{
			m_bFollowRenames = FALSE;
			UpdateData(FALSE);
		}
	}
	else if (m_bShowCompleteLog || m_bFollowRenames)
	{
		m_bShowCompleteLog = FALSE;
		m_bFollowRenames = FALSE;
		UpdateData(FALSE);
	}
	GetDlgItem(IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE)->EnableWindow(enableDetectMovedOrCopiedLinesNumCharactersWithinFile);
	GetDlgItem(IDC_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES)->EnableWindow(enableDetectMovedOrCopiedLinesNumCharactersFromFiles);
	GetDlgItem(IDC_SHOWCOMPLETELOG)->EnableWindow(enableShowCompleteLog);
	GetDlgItem(IDC_FOLLOWRENAMES)->EnableWindow(enableFollowRenames);
}

void CSettingsTBlame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
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

