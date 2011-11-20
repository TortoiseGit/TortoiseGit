// TortoiseGit - a Windows shell extension for easy version control

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
#include ".\settingscolors3.h"

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

	return TRUE;
}

void CSettingsColors3::OnBnClickedRestore()
{
	for(int i=0;i<8;i++)
	{
		m_cLine[i].SetColor(m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i)));
	}
	SetModified(TRUE);
}

BOOL CSettingsColors3::OnApply()
{
	for(int i=0;i<8;i++)
	{
		m_Colors.SetColor((CColors::Colors)(CColors::BranchLine1+i),
			m_cLine[i].GetColor() == -1 ? m_cLine[i].GetAutomaticColor() : m_cLine[i].GetColor());
	}
	return ISettingsPropPage::OnApply();
}

void CSettingsColors3::OnBnClickedColor()
{
	SetModified();
}
