// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2013, 2016, 2019 - TortoiseGit
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
#include "SettingsColors3.h"

IMPLEMENT_DYNAMIC(CSettingsColors3, ISettingsPropPage)
CSettingsColors3::CSettingsColors3()
	: ISettingsPropPage(CSettingsColors3::IDD)
{
}

CSettingsColors3::~CSettingsColors3()
{
}

void CSettingsColors3::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLOR_LINE1, m_cLine[0]);
	DDX_Control(pDX, IDC_COLOR_LINE2, m_cLine[1]);
	DDX_Control(pDX, IDC_COLOR_LINE3, m_cLine[2]);
	DDX_Control(pDX, IDC_COLOR_LINE4, m_cLine[3]);
	DDX_Control(pDX, IDC_COLOR_LINE5, m_cLine[4]);
	DDX_Control(pDX, IDC_COLOR_LINE6, m_cLine[5]);
	DDX_Control(pDX, IDC_COLOR_LINE7, m_cLine[6]);
	DDX_Control(pDX, IDC_COLOR_LINE8, m_cLine[7]);
	DDX_Control(pDX, IDC_LOGGRAPHLINEWIDTH, m_LogGraphLineWidth);
	DDX_Control(pDX, IDC_LOGGRAPHNODESIZE, m_LogGraphNodeSize);
}

BEGIN_MESSAGE_MAP(CSettingsColors3, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
	ON_BN_CLICKED(IDC_COLOR_LINE1, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE2, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE3, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE4, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE5, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE6, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE7, &CSettingsColors3::OnBnClickedColor)
	ON_BN_CLICKED(IDC_COLOR_LINE8, &CSettingsColors3::OnBnClickedColor)
	ON_CBN_SELCHANGE(IDC_LOGGRAPHLINEWIDTH, &CSettingsColors3::OnCbnSelchangeLoggraphlinewidth)
	ON_CBN_SELCHANGE(IDC_LOGGRAPHNODESIZE, &CSettingsColors3::OnCbnSelchangeLoggraphlinewidth)
END_MESSAGE_MAP()

BOOL CSettingsColors3::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);

	for(int i=0;i<8;i++)
	{
		m_cLine[i].SetColor(m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i)));
		m_cLine[i].EnableAutomaticButton(sDefaultText, m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i), true));
		m_cLine[i].EnableOtherButton(sCustomText);
	}

	CString text;
	for (int i = 1; i <= 10; i++)
	{
		text.Format(L"%d", i);
		m_LogGraphLineWidth.AddString(text);
	}
	for (int i = 1; i <= 30; i++)
	{
		text.Format(L"%d", i);
		m_LogGraphNodeSize.AddString(text);
	}
	m_regLogGraphLineWidth = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Graph\\LogLineWidth", 2);
	text.Format(L"%d", static_cast<DWORD>(m_regLogGraphLineWidth));
	m_LogGraphLineWidth.SelectString(-1, text);
	m_regLogGraphNodeSize = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Graph\\LogNodeSize", 10);
	text.Format(L"%d", static_cast<DWORD>(m_regLogGraphNodeSize));
	m_LogGraphNodeSize.SelectString(-1, text);

	return TRUE;
}

void CSettingsColors3::OnBnClickedRestore()
{
	for(int i=0;i<8;i++)
		m_cLine[i].SetColor(m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i), true));
	m_LogGraphLineWidth.SelectString(-1, L"2");
	m_LogGraphNodeSize.SelectString(-1, L"10");
	SetModified(TRUE);
}

BOOL CSettingsColors3::OnApply()
{
	for(int i=0;i<8;i++)
	{
		m_Colors.SetColor((CColors::Colors)(CColors::BranchLine1+i),
			m_cLine[i].GetColor() == -1 ? m_cLine[i].GetAutomaticColor() : m_cLine[i].GetColor());
	}

	CString text;
	m_LogGraphLineWidth.GetWindowText(text);
	m_regLogGraphLineWidth = _wtoi(text);
	m_LogGraphNodeSize.GetWindowText(text);
	m_regLogGraphNodeSize = _wtoi(text);

	return ISettingsPropPage::OnApply();
}

void CSettingsColors3::OnBnClickedColor()
{
	SetModified();
}

void CSettingsColors3::OnCbnSelchangeLoggraphlinewidth()
{
	SetModified();
}
