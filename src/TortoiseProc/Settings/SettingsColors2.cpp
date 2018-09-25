// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2013, 2018 - TortoiseGit
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
#include "SettingsColors2.h"

IMPLEMENT_DYNAMIC(CSettingsColors2, ISettingsPropPage)
CSettingsColors2::CSettingsColors2()
	: ISettingsPropPage(CSettingsColors2::IDD)
{
}

CSettingsColors2::~CSettingsColors2()
{
}

void CSettingsColors2::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CURRENT_BRANCH, this->m_cCurrentBranch);
	DDX_Control(pDX, IDC_LOCAL_BRANCH,	 this->m_cLocalBranch);
	DDX_Control(pDX, IDC_REMOTE_BRANCH,  this->m_cRemoteBranch);
	DDX_Control(pDX, IDC_TAGS,			this->m_cTags);
	DDX_Control(pDX, IDC_FILTERMATCHCOLOR, m_cFilterMatch);
}


BEGIN_MESSAGE_MAP(CSettingsColors2, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
	ON_BN_CLICKED(IDC_CURRENT_BRANCH, &CSettingsColors2::OnBnClickedColor)
	ON_BN_CLICKED(IDC_LOCAL_BRANCH, &CSettingsColors2::OnBnClickedColor)
	ON_BN_CLICKED(IDC_REMOTE_BRANCH, &CSettingsColors2::OnBnClickedColor)
	ON_BN_CLICKED(IDC_TAGS, &CSettingsColors2::OnBnClickedColor)
	ON_BN_CLICKED(IDC_FILTERMATCHCOLOR, &CSettingsColors2::OnBnClickedColor)
END_MESSAGE_MAP()

BOOL CSettingsColors2::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cCurrentBranch.SetColor(m_Colors.GetColor(CColors::CurrentBranch));
	m_cLocalBranch.SetColor(m_Colors.GetColor(CColors::LocalBranch));
	m_cRemoteBranch.SetColor(m_Colors.GetColor(CColors::RemoteBranch));
	m_cTags.SetColor(m_Colors.GetColor(CColors::Tag));
	m_cFilterMatch.SetColor(m_Colors.GetColor(CColors::FilterMatch));

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);

	m_cCurrentBranch.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::CurrentBranch, true));
	m_cCurrentBranch.EnableOtherButton(sCustomText);

	m_cLocalBranch.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::LocalBranch, true));
	m_cLocalBranch.EnableOtherButton(sCustomText);

	m_cRemoteBranch.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::RemoteBranch, true));
	m_cRemoteBranch.EnableOtherButton(sCustomText);

	m_cTags.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Tag, true));
	m_cTags.EnableOtherButton(sCustomText);

	m_cFilterMatch.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::FilterMatch, true));
	m_cFilterMatch.EnableOtherButton(sCustomText);

	return TRUE;
}

void CSettingsColors2::OnBnClickedRestore()
{
	m_cCurrentBranch.SetColor(m_Colors.GetColor(CColors::CurrentBranch, true));
	m_cLocalBranch.SetColor(m_Colors.GetColor(CColors::LocalBranch, true));
	m_cRemoteBranch.SetColor(m_Colors.GetColor(CColors::RemoteBranch, true));
	m_cTags.SetColor(m_Colors.GetColor(CColors::Tag, true));
	m_cFilterMatch.SetColor(m_Colors.GetColor(CColors::FilterMatch, true));
	SetModified(TRUE);
}

BOOL CSettingsColors2::OnApply()
{
	m_Colors.SetColor(CColors::CurrentBranch,	m_cCurrentBranch.GetColor() == -1 ? m_cCurrentBranch.GetAutomaticColor() :	m_cCurrentBranch.GetColor());
	m_Colors.SetColor(CColors::LocalBranch,		m_cLocalBranch.GetColor() == -1 ?	m_cLocalBranch.GetAutomaticColor() :	m_cLocalBranch.GetColor());
	m_Colors.SetColor(CColors::RemoteBranch,	m_cRemoteBranch.GetColor() == -1 ?	m_cRemoteBranch.GetAutomaticColor() :	m_cRemoteBranch.GetColor());
	m_Colors.SetColor(CColors::Tag,				m_cTags.GetColor() == -1 ?			m_cTags.GetAutomaticColor() :			m_cTags.GetColor());
	m_Colors.SetColor(CColors::FilterMatch,		m_cFilterMatch.GetColor() == -1 ?	m_cFilterMatch.GetAutomaticColor() :	m_cFilterMatch.GetColor());
	return ISettingsPropPage::OnApply();
}

void CSettingsColors2::OnBnClickedColor()
{
	SetModified();
}
