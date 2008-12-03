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
#include "SetLogCache.h"
#include "MessageBox.h"
#include "SVN.h"
#include "SVNError.h"
#include "LogCacheSettings.h"
#include "LogCachePool.h"
#include "LogCacheStatistics.h"
#include "LogCacheStatisticsDlg.h"
#include "ProgressDlg.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "CSVWriter.h"
#include "XPTheme.h"

using namespace LogCache;

IMPLEMENT_DYNAMIC(CSetLogCache, ISettingsPropPage)

CSetLogCache::CSetLogCache()
	: ISettingsPropPage (CSetLogCache::IDD)
    , m_bEnableLogCaching (CSettings::GetEnabled())
    , m_bSupportAmbiguousURL (CSettings::GetAllowAmbiguousURL())
    , m_bSupportAmbiguousUUID (CSettings::GetAllowAmbiguousUUID())
    , m_dwMaxHeadAge (CSettings::GetMaxHeadAge())
    , m_dwCacheDropAge (CSettings::GetCacheDropAge())
    , m_dwCacheDropMaxSize (CSettings::GetCacheDropMaxSize())
    , m_dwMaxFailuresUntilDrop (CSettings::GetMaxFailuresUntilDrop())
{
}

CSetLogCache::~CSetLogCache()
{
}

void CSetLogCache::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ENABLELOGCACHING, m_bEnableLogCaching);
	DDX_Check(pDX, IDC_SUPPORTAMBIGUOUSURL, m_bSupportAmbiguousURL);
	DDX_Check(pDX, IDC_SUPPORTAMBIGUOUSUUID, m_bSupportAmbiguousUUID);

    DDX_Control(pDX, IDC_GOOFFLINESETTING, m_cDefaultConnectionState);

	DDX_Text(pDX, IDC_MAXIMINHEADAGE, m_dwMaxHeadAge);
	DDX_Text(pDX, IDC_CACHEDROPAGE, m_dwCacheDropAge);
	DDX_Text(pDX, IDC_CACHEDROPMAXSIZE, m_dwCacheDropMaxSize);

    DDX_Text(pDX, IDC_MAXFAILUESUNTILDROP, m_dwMaxFailuresUntilDrop);
}


BEGIN_MESSAGE_MAP(CSetLogCache, ISettingsPropPage)
	ON_BN_CLICKED(IDC_ENABLELOGCACHING, OnChanged)
	ON_BN_CLICKED(IDC_SUPPORTAMBIGUOUSURL, OnChanged)
	ON_BN_CLICKED(IDC_SUPPORTAMBIGUOUSUUID, OnChanged)
	ON_CBN_SELCHANGE(IDC_GOOFFLINESETTING, OnChanged)
	ON_EN_CHANGE(IDC_MAXIMINHEADAGE, OnChanged)
	ON_EN_CHANGE(IDC_CACHEDROPAGE, OnChanged)
	ON_EN_CHANGE(IDC_CACHEDROPMAXSIZE, OnChanged)
	ON_EN_CHANGE(IDC_MAXFAILUESUNTILDROP, OnChanged)
    ON_BN_CLICKED(IDC_CACHESTDDEFAULTS, OnStandardDefaults)
	ON_BN_CLICKED(IDC_CACHEPOWERDEFAULTS, OnPowerDefaults)
END_MESSAGE_MAP()

void CSetLogCache::OnChanged()
{
	SetModified();
}

void CSetLogCache::OnStandardDefaults()
{
	m_bEnableLogCaching = TRUE;
    m_bSupportAmbiguousURL = TRUE;
    m_bSupportAmbiguousUUID = TRUE;

    m_cDefaultConnectionState.SetCurSel(0);

    m_dwMaxHeadAge = 0;
    m_dwCacheDropAge = 10;
    m_dwCacheDropMaxSize = 200;

    m_dwMaxFailuresUntilDrop = 0;

    SetModified();
    UpdateData (FALSE);
}

void CSetLogCache::OnPowerDefaults()
{
	m_bEnableLogCaching = TRUE;
    m_bSupportAmbiguousURL = FALSE;
    m_bSupportAmbiguousUUID = FALSE;

    m_cDefaultConnectionState.SetCurSel(1);

    m_dwMaxHeadAge = 300;
    m_dwCacheDropAge = 10;
    m_dwCacheDropMaxSize = 0;

    m_dwMaxFailuresUntilDrop = 20;

    SetModified();
    UpdateData (FALSE);
}

BOOL CSetLogCache::OnApply()
{
	UpdateData();

	CSettings::SetEnabled (m_bEnableLogCaching != FALSE);
    CSettings::SetAllowAmbiguousURL (m_bSupportAmbiguousURL != FALSE);
    CSettings::SetAllowAmbiguousUUID (m_bSupportAmbiguousUUID != FALSE);

    CRepositoryInfo::ConnectionState state 
        = static_cast<CRepositoryInfo::ConnectionState>
            (m_cDefaultConnectionState.GetCurSel());
	CSettings::SetDefaultConnectionState (state);

    CSettings::SetMaxHeadAge (m_dwMaxHeadAge);
    CSettings::SetCacheDropAge (m_dwCacheDropAge);
    CSettings::SetCacheDropMaxSize (m_dwCacheDropMaxSize);

    CSettings::SetMaxFailuresUntilDrop (m_dwMaxFailuresUntilDrop);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

BOOL CSetLogCache::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

    // connectivity combobox

    while (m_cDefaultConnectionState.GetCount() > 0)
        m_cDefaultConnectionState.DeleteItem(0);

	CString temp;
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_ASKUSER);
    m_cDefaultConnectionState.AddString (temp);
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_OFFLINENOW);
    m_cDefaultConnectionState.AddString (temp);
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_OFFLINEFOREVER);
    m_cDefaultConnectionState.AddString (temp);

    m_cDefaultConnectionState.SetCurSel (CSettings::GetDefaultConnectionState());

    // tooltips

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_ENABLELOGCACHING, IDS_SETTINGS_LOGCACHE_ENABLE);
	m_tooltips.AddTool(IDC_SUPPORTAMBIGUOUSURL, IDS_SETTINGS_LOGCACHE_AMBIGUOUSURL);
	m_tooltips.AddTool(IDC_SUPPORTAMBIGUOUSUUID, IDS_SETTINGS_LOGCACHE_AMBIGUOUSUUID);

    m_tooltips.AddTool(IDC_GOOFFLINESETTING, IDS_SETTINGS_LOGCACHE_GOOFFLINE);

    m_tooltips.AddTool(IDC_MAXIMINHEADAGE, IDS_SETTINGS_LOGCACHE_HEADAGE);
    m_tooltips.AddTool(IDC_CACHEDROPAGE, IDS_SETTINGS_LOGCACHE_DROPAGE);
    m_tooltips.AddTool(IDC_CACHEDROPMAXSIZE, IDS_SETTINGS_LOGCACHE_DROPMAXSIZE);

    m_tooltips.AddTool(IDC_MAXFAILUESUNTILDROP, IDS_SETTINGS_LOGCACHE_FAILURELIMIT);

    m_tooltips.AddTool(IDC_CACHESTDDEFAULTS, IDS_SETTINGS_LOGCACHE_STDDEFAULT);
    m_tooltips.AddTool(IDC_CACHEPOWERDEFAULTS, IDS_SETTINGS_LOGCACHE_POWERDEFAULT);

    return TRUE;
}

BOOL CSetLogCache::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}
