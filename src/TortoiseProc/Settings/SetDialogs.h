// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2018 - TortoiseGit
// Copyright (C) 2003-2008, 2013 - TortoiseSVN

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
#include "registry.h"
#include "HistoryCombo.h"


/**
 * \ingroup TortoiseProc
 * Settings page responsible for dialog settings.
 */
class CSetDialogs : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetDialogs)

public:
	CSetDialogs();
	virtual ~CSetDialogs();

	UINT GetIconID() override { return IDI_DIALOGS; }

// Dialog Data
	enum { IDD = IDD_SETTINGSDIALOGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnChange();
	afx_msg void OnCbnSelchangeDefaultlogscale();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

private:
	BOOL				m_bShortDateFormat;
	BOOL				m_bRelativeTimes;
	CRegDWORD			m_regShortDateFormat;
	CRegDWORD			m_regRelativeTimes;
	BOOL				m_bAsteriskLogPrefix;
	CRegDWORD			m_regAsteriskLogPrefix;
	BOOL				m_bUseSystemLocaleForDates;
	CRegDWORD			m_regUseSystemLocaleForDates;
	CRegDWORD			m_regDefaultLogs;
	CString				m_sDefaultLogs;
	CEdit				m_DefaultNumberOfCtl;
	CRegDWORD			m_regDefaultLogsScale;
	CComboBox			m_cDefaultLogsScale;
	CMFCFontComboBox	m_cFontNames;
	CComboBox			m_cFontSizes;
	CRegDWORD			m_regFontSize;
	DWORD				m_dwFontSize;
	CRegString			m_regFontName;
	CString				m_sFontName;
	CRegDWORD			m_regDiffByDoubleClick;
	BOOL				m_bDiffByDoubleClick;
	CRegDWORD			m_regAbbreviateRenamings;
	BOOL				m_bAbbreviateRenamings;
	CRegDWORD			m_regSymbolizeRefNames;
	BOOL				m_bSymbolizeRefNames;
	CRegDWORD			m_regEnableLogCache;
	BOOL				m_bEnableLogCache;
	CRegDWORD			m_regEnableGravatar;
	BOOL				m_bEnableGravatar;
	CRegString			m_regGravatarUrl;
	CString				m_GravatarUrl;
	CHistoryCombo		m_cGravatarUrl;
	BOOL				m_bDrawBranchesTagsOnRightSide;
	CRegDWORD			m_regDrawBranchesTagsOnRightSide;
	BOOL				m_bShowDescribe;
	CRegDWORD			m_regShowDescribe;
	BOOL				m_bShowBranchRevNo;
	CRegDWORD			m_regShowBranchRevNo;
	int					m_DescribeStrategy;
	CComboBox			m_cDescribeStrategy;
	CRegDWORD			m_regDescribeStrategy;
	DWORD				m_DescribeAbbreviatedSize;
	CRegDWORD			m_regDescribeAbbreviatedSize;
	BOOL				m_bDescribeAlwaysLong;
	CRegDWORD			m_regDescribeAlwaysLong;
	BOOL				m_bDescribeOnlyFollowFirstParent;
	CRegDWORD			m_regDescribeOnlyFollowFirstParent;
	BOOL				m_bFullCommitMessageOnLogLine;
	CRegDWORD			m_regFullCommitMessageOnLogLine;
	BOOL				m_bMailmapOnLog;
	CRegDWORD			m_regMailmapOnLog;
};
