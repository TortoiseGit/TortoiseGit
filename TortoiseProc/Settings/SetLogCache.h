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
class CSetLogCache 
    : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetLogCache)

public:
	CSetLogCache();
	virtual ~CSetLogCache();
	
	UINT GetIconID() {return IDI_CACHE;}

// Dialog Data
	enum { IDD = IDD_SETTINGSLOGCACHE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnChanged();
	afx_msg void OnStandardDefaults();
	afx_msg void OnPowerDefaults();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();

    DECLARE_MESSAGE_MAP()
private:
	CToolTips		m_tooltips;

	BOOL			m_bEnableLogCaching;
	BOOL			m_bSupportAmbiguousURL;
	BOOL			m_bSupportAmbiguousUUID;

	CComboBox       m_cDefaultConnectionState;

	DWORD			m_dwMaxHeadAge;
	DWORD			m_dwCacheDropAge;
	DWORD			m_dwCacheDropMaxSize;

	DWORD			m_dwMaxFailuresUntilDrop;
};
