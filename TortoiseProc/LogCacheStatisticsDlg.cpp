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
#include "LogCacheStatisticsDlg.h"
#include "LogCacheStatistics.h"
#include "RepositoryInfo.h"
#include "SVN.h"

// CLogCacheStatisticsDlg-Dialogfeld

IMPLEMENT_DYNAMIC(CLogCacheStatisticsDlg, CDialog)

CLogCacheStatisticsDlg::CLogCacheStatisticsDlg 
    ( const LogCache::CLogCacheStatisticsData& data, CWnd * pParentWnd)
	: CDialog(CLogCacheStatisticsDlg::IDD, pParentWnd)
{
    sizeRAM = ToString (data.ramSize / 1024);
    sizeDisk = ToString (data.fileSize / 1024);

    switch (data.connectionState)
    {
    case LogCache::CRepositoryInfo::online:
        connectionState.LoadString (IDS_CONNECTIONSTATE_ONLINE);
        break;
    case LogCache::CRepositoryInfo::tempOffline:
        connectionState.LoadString (IDS_CONNECTIONSTATE_TEMPOFFLINE);
        break;
    case LogCache::CRepositoryInfo::offline:
        connectionState.LoadString (IDS_CONNECTIONSTATE_OFFLINE);
        break;
    }

    lastRead = DateToString (data.lastReadAccess);
    lastWrite = DateToString (data.lastWriteAccess);
    lastHeadUpdate = DateToString (data.headTimeStamp);

    authors = ToString (data.authorCount);
    paths = ToString (data.pathCount);
	pathElements = ToString (data.pathElementCount);
    skipRanges = ToString (data.skipDeltaCount);
    wordTokens = ToString (data.wordTokenCount);
    pairTokens = ToString (data.pairTokenCount);
    textSize = ToString (data.textSize);
	uncompressedSize = ToString (data.uncompressedSize);

    maxRevision = ToString (data.maxRevision);
    revisionCount = ToString (data.revisionCount);

    changesTotal = ToString (data.changesCount);
    changedRevisions = ToString (data.changesRevisionCount);
    changesMissing = ToString (data.changesMissingRevisionCount);
    mergesTotal = ToString (data.mergeInfoCount);
    mergesRevisions = ToString (data.mergeInfoRevisionCount);
    mergesMissing = ToString (data.mergeInfoMissingRevisionCount);
    userRevpropsTotal = ToString (data.userRevPropCount);
    userRevpropsRevisions = ToString (data.userRevPropRevisionCount);
    userRevpropsMissing = ToString (data.userRevPropMissingRevisionCount);
}

CLogCacheStatisticsDlg::~CLogCacheStatisticsDlg()
{
}

void CLogCacheStatisticsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_SIZERAM, sizeRAM);
    DDX_Text(pDX, IDC_SIZEDISK, sizeDisk);
    DDX_Text(pDX, IDC_CONNECTIONSTATE, connectionState);
    DDX_Text(pDX, IDC_LASTREAD, lastRead);
    DDX_Text(pDX, IDC_LASTWRITE, lastWrite);
    DDX_Text(pDX, IDC_LASTHEADUPDATE, lastHeadUpdate);
    DDX_Text(pDX, IDC_AUTHORS, authors);
    DDX_Text(pDX, IDC_PATHELEMENTS, pathElements);
    DDX_Text(pDX, IDC_PATHS, paths);
    DDX_Text(pDX, IDC_SKIPRANGES, skipRanges);
    DDX_Text(pDX, IDC_WORDTOKENS, wordTokens);
    DDX_Text(pDX, IDC_PAIRTOKENS, pairTokens);
    DDX_Text(pDX, IDC_TEXTSIZE, textSize);
    DDX_Text(pDX, IDC_UNCOMPRESSEDSIZE, uncompressedSize);
    DDX_Text(pDX, IDC_MAXREVISION, maxRevision);
    DDX_Text(pDX, IDC_REVISIONCOUNT, revisionCount);
    DDX_Text(pDX, IDC_CHANGESTOTAL, changesTotal);
    DDX_Text(pDX, IDC_CHANGEDREVISIONS, changedRevisions);
    DDX_Text(pDX, IDC_CHANGESMISSING, changesMissing);
    DDX_Text(pDX, IDC_MERGESTOTAL, mergesTotal);
    DDX_Text(pDX, IDC_MERGESREVISIONS, mergesRevisions);
    DDX_Text(pDX, IDC_MERGESMISSING, mergesMissing);
    DDX_Text(pDX, IDC_USERREVPROPSTOTAL, userRevpropsTotal);
    DDX_Text(pDX, IDC_USERREVPROPSREVISISONS, userRevpropsRevisions);
    DDX_Text(pDX, IDC_USERREVPROPSMISSING, userRevpropsMissing);
}


BEGIN_MESSAGE_MAP(CLogCacheStatisticsDlg, CDialog)
END_MESSAGE_MAP()

CString CLogCacheStatisticsDlg::DateToString (__time64_t time)
{
    // transform to 1-second base

    __time64_t systime = time / 1000000L;
    __time64_t now = CTime::GetCurrentTime().GetTime();

    // return time when younger than 1 day
    // return date otherwise

    return (now - systime >= 0) && (now - systime < 86400)
        ? SVN::formatTime (time)
        : SVN::formatDate (time);
}

CString CLogCacheStatisticsDlg::ToString (__int64 value)
{
    TCHAR buffer[20];
    _i64tot_s (value, buffer, sizeof (buffer) / sizeof (TCHAR), 10); 
    return buffer;
}
BOOL CLogCacheStatisticsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_SIZERAM, IDS_SETTINGS_LOGCACHESTATS_RAM);
	m_tooltips.AddTool(IDC_SIZEDISK, IDS_SETTINGS_LOGCACHESTATS_DISK);
	m_tooltips.AddTool(IDC_CONNECTIONSTATE, IDS_SETTINGS_LOGCACHESTATS_CONNECTION);
	m_tooltips.AddTool(IDC_LASTREAD, IDS_SETTINGS_LOGCACHESTATS_LASTREAD);
	m_tooltips.AddTool(IDC_LASTWRITE, IDS_SETTINGS_LOGCACHESTATS_LASTWRITE);
	m_tooltips.AddTool(IDC_LASTHEADUPDATE, IDS_SETTINGS_LOGCACHESTATS_LASTHEADUPDATE);
	m_tooltips.AddTool(IDC_AUTHORS, IDS_SETTINGS_LOGCACHESTATS_AUTHORS);
	m_tooltips.AddTool(IDC_PATHELEMENTS, IDS_SETTINGS_LOGCACHESTATS_PATHELEMENTS);
	m_tooltips.AddTool(IDC_PATHS, IDS_SETTINGS_LOGCACHESTATS_PATHS);
	m_tooltips.AddTool(IDC_SKIPRANGES, IDS_SETTINGS_LOGCACHESTATS_SKIPRANGES);
	m_tooltips.AddTool(IDC_WORDTOKENS, IDS_SETTINGS_LOGCACHESTATS_WORDTOKENS);
	m_tooltips.AddTool(IDC_PAIRTOKENS, IDS_SETTINGS_LOGCACHESTATS_PAIRTOKENS);
	m_tooltips.AddTool(IDC_TEXTSIZE, IDS_SETTINGS_LOGCACHESTATS_TEXTSIZE);
	m_tooltips.AddTool(IDC_UNCOMPRESSEDSIZE, IDS_SETTINGS_LOGCACHESTATS_UNCOMPRESSEDSIZE);
	m_tooltips.AddTool(IDC_MAXREVISION, IDS_SETTINGS_LOGCACHESTATS_MAXREVISION);
	m_tooltips.AddTool(IDC_REVISIONCOUNT, IDS_SETTINGS_LOGCACHESTATS_REVISIONCOUNT);
	m_tooltips.AddTool(IDC_CHANGESTOTAL, IDS_SETTINGS_LOGCACHESTATS_CHANGESTOTAL);
	m_tooltips.AddTool(IDC_CHANGEDREVISIONS, IDS_SETTINGS_LOGCACHESTATS_CHANGEDREVISIONS);
	m_tooltips.AddTool(IDC_CHANGESMISSING, IDS_SETTINGS_LOGCACHESTATS_CHANGESMISSING);
	m_tooltips.AddTool(IDC_MERGESTOTAL, IDS_SETTINGS_LOGCACHESTATS_MERGESTOTAL);
	m_tooltips.AddTool(IDC_MERGESREVISIONS, IDS_SETTINGS_LOGCACHESTATS_MERGESREVISIONS);
	m_tooltips.AddTool(IDC_MERGESMISSING, IDS_SETTINGS_LOGCACHESTATS_MERGESMISSING);
	m_tooltips.AddTool(IDC_USERREVPROPSTOTAL, IDS_SETTINGS_LOGCACHESTATS_USERREVPROPSTOTAL);
	m_tooltips.AddTool(IDC_USERREVPROPSREVISISONS, IDS_SETTINGS_LOGCACHESTATS_USERREVPROPSREVISIONS);
	m_tooltips.AddTool(IDC_USERREVPROPSMISSING, IDS_SETTINGS_LOGCACHESTATS_USERREVPROPSMISSING);


	return TRUE;
}

BOOL CLogCacheStatisticsDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
