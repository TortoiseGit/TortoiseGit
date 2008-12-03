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
#pragma once
#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "Registry.h"
#include "ILogReceiver.h"

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff. 
 */
class CSettingsRevisionGraph 
    : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsRevisionGraph)

public:
	CSettingsRevisionGraph();
	virtual ~CSettingsRevisionGraph();
	
	UINT GetIconID() {return IDI_SETTINGSREVGRAPH;}

    virtual BOOL OnApply();

// Dialog Data
	enum { IDD = IDD_SETTINGSREVGRAPH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnChanged();

    DECLARE_MESSAGE_MAP()

private:
	CToolTips		m_tooltips;

    CRegString      regTrunkPattern;
    CRegString      regBranchesPattern;
    CRegString      regTagsPattern;
    CRegDWORD       regTweakTrunkColors;
    CRegDWORD       regTweakTagsColors;

    CString         trunkPattern;
    CString         branchesPattern;
    CString         tagsPattern;
    BOOL            tweakTrunkColors;
    BOOL            tweakTagsColors;
};
