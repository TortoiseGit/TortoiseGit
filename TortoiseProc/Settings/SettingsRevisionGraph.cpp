// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "SettingsRevisionGraph.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSettingsRevisionGraph, ISettingsPropPage)

CSettingsRevisionGraph::CSettingsRevisionGraph()
	: ISettingsPropPage(CSettingsRevisionGraph::IDD)
    , regTrunkPattern (_T("Software\\TortoiseSVN\\RevisionGraph\\TrunkPattern"), _T("trunk"))
    , regBranchesPattern (_T("Software\\TortoiseSVN\\RevisionGraph\\BranchPattern"), _T("branches"))
    , regTagsPattern (_T("Software\\TortoiseSVN\\RevisionGraph\\TagsPattern"), _T("tags"))
    , regTweakTrunkColors (_T("Software\\TortoiseSVN\\RevisionGraph\\TweakTrunkColors"), TRUE)
    , regTweakTagsColors (_T("Software\\TortoiseSVN\\RevisionGraph\\TweakTagsColors"), TRUE)
    , trunkPattern (regTrunkPattern)
    , branchesPattern (regBranchesPattern)
    , tagsPattern (regTagsPattern)
    , tweakTrunkColors (regTweakTrunkColors)
    , tweakTagsColors (regTweakTagsColors)
{
}

CSettingsRevisionGraph::~CSettingsRevisionGraph()
{
}

// update cache list

void CSettingsRevisionGraph::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_TRUNKPATTERN, trunkPattern);
	DDX_Text(pDX, IDC_BRANCHESPATTERN, branchesPattern);
	DDX_Text(pDX, IDC_TAGSPATTERN, tagsPattern);

	DDX_Check(pDX, IDC_TWEAKTRUNKCOLORS, tweakTrunkColors);
	DDX_Check(pDX, IDC_TWEAKTAGSCOLORS, tweakTagsColors);
}


BEGIN_MESSAGE_MAP(CSettingsRevisionGraph, ISettingsPropPage)
	ON_EN_CHANGE(IDC_TRUNKPATTERN, OnChanged)
	ON_EN_CHANGE(IDC_BRANCHESPATTERN, OnChanged)
	ON_EN_CHANGE(IDC_TAGSPATTERN, OnChanged)

	ON_BN_CLICKED(IDC_TWEAKTRUNKCOLORS, OnChanged)
	ON_BN_CLICKED(IDC_TWEAKTAGSCOLORS, OnChanged)
END_MESSAGE_MAP()

void CSettingsRevisionGraph::OnChanged()
{
	SetModified();
}

BOOL CSettingsRevisionGraph::OnApply()
{
	UpdateData();

    Store (trunkPattern, regTrunkPattern);
    Store (branchesPattern, regBranchesPattern);
    Store (tagsPattern, regTagsPattern);
    Store (tweakTrunkColors, regTweakTrunkColors);
    Store (tweakTagsColors, regTweakTagsColors);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

BOOL CSettingsRevisionGraph::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

    // tooltips

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_TRUNKPATTERN, IDS_SETTINGS_TRUNKPATTERN);
	m_tooltips.AddTool(IDC_BRANCHESPATTERN, IDS_SETTINGS_BRANCHESPATTERN);
	m_tooltips.AddTool(IDC_TAGSPATTERN, IDS_SETTINGS_TAGSPATTERN);

    m_tooltips.AddTool(IDC_TWEAKTRUNKCOLORS, IDS_SETTINGS_TWEAKTRUNKCOLORS);
    m_tooltips.AddTool(IDC_TWEAKTAGSCOLORS, IDS_SETTINGS_TWEAKTAGSCOLORS);

	return TRUE;
}

BOOL CSettingsRevisionGraph::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}
