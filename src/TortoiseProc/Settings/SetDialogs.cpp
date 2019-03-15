// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "SetMainPage.h"
#include "AppUtils.h"
#include "SetDialogs.h"

IMPLEMENT_DYNAMIC(CSetDialogs, ISettingsPropPage)
CSetDialogs::CSetDialogs()
	: ISettingsPropPage(CSetDialogs::IDD)
	, m_bShortDateFormat(FALSE)
	, m_bRelativeTimes(FALSE)
	, m_bAsteriskLogPrefix(TRUE)
	, m_dwFontSize(0)
	, m_bDiffByDoubleClick(FALSE)
	, m_bUseSystemLocaleForDates(FALSE)
	, m_bAbbreviateRenamings(FALSE)
	, m_bSymbolizeRefNames(FALSE)
	, m_bEnableLogCache(TRUE)
	, m_bDrawBranchesTagsOnRightSide(FALSE)
	, m_bEnableGravatar(FALSE)
	, m_bShowDescribe(FALSE)
	, m_bShowBranchRevNo(FALSE)
	, m_DescribeStrategy(GIT_DESCRIBE_DEFAULT)
	, m_DescribeAbbreviatedSize(GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE)
	, m_bDescribeAlwaysLong(FALSE)
	, m_bDescribeOnlyFollowFirstParent(FALSE)
	, m_bFullCommitMessageOnLogLine(FALSE)
	, m_bMailmapOnLog(FALSE)
{
	m_regDefaultLogs = CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\NumberOfLogs", 1);
	m_regDefaultLogsScale = CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\NumberOfLogsScale", CFilterData::SHOW_NO_LIMIT);
	m_regShortDateFormat = CRegDWORD(L"Software\\TortoiseGit\\LogDateFormat", TRUE);
	m_regRelativeTimes = CRegDWORD(L"Software\\TortoiseGit\\RelativeTimes", FALSE);
	m_regAsteriskLogPrefix = CRegDWORD(L"Software\\TortoiseGit\\AsteriskLogPrefix", TRUE);
	m_regUseSystemLocaleForDates = CRegDWORD(L"Software\\TortoiseGit\\UseSystemLocaleForDates", TRUE);
	m_regFontName = CRegString(L"Software\\TortoiseGit\\LogFontName", L"Consolas");
	m_regFontSize = CRegDWORD(L"Software\\TortoiseGit\\LogFontSize", 9);
	m_regDiffByDoubleClick = CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE);
	m_regAbbreviateRenamings = CRegDWORD(L"Software\\TortoiseGit\\AbbreviateRenamings", FALSE);
	m_regSymbolizeRefNames = CRegDWORD(L"Software\\TortoiseGit\\SymbolizeRefNames", FALSE);
	m_regEnableLogCache = CRegDWORD(L"Software\\TortoiseGit\\EnableLogCache", TRUE);
	m_regEnableGravatar = CRegDWORD(L"Software\\TortoiseGit\\EnableGravatar", FALSE);
	m_regGravatarUrl = CRegString(L"Software\\TortoiseGit\\GravatarUrl", L"http://www.gravatar.com/avatar/%HASH%?d=identicon");
	m_regDrawBranchesTagsOnRightSide = CRegDWORD(L"Software\\TortoiseGit\\DrawTagsBranchesOnRightSide", FALSE);
	m_regShowDescribe = CRegDWORD(L"Software\\TortoiseGit\\ShowDescribe", FALSE);
	m_regShowBranchRevNo = CRegDWORD(L"Software\\TortoiseGit\\ShowBranchRevisionNumber", FALSE);
	m_regDescribeStrategy = CRegDWORD(L"Software\\TortoiseGit\\DescribeStrategy", GIT_DESCRIBE_DEFAULT);
	m_regDescribeAbbreviatedSize = CRegDWORD(L"Software\\TortoiseGit\\DescribeAbbreviatedSize", GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE);
	m_regDescribeAlwaysLong = CRegDWORD(L"Software\\TortoiseGit\\DescribeAlwaysLong", FALSE);
	m_regDescribeOnlyFollowFirstParent = CRegDWORD(L"Software\\TortoiseGit\\DescribeOnlyFollowFirstParent", FALSE);
	m_regFullCommitMessageOnLogLine = CRegDWORD(L"Software\\TortoiseGit\\FullCommitMessageOnLogLine", FALSE);
	m_regMailmapOnLog = CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\UseMailmap", FALSE);
}

CSetDialogs::~CSetDialogs()
{
}

void CSetDialogs::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = static_cast<DWORD>(m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel()));
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _wtoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
	DDX_Text(pDX, IDC_DEFAULT_NUMBER_OF, m_sDefaultLogs);
	DDX_Control(pDX, IDC_DEFAULT_SCALE, m_cDefaultLogsScale);
	DDX_Check(pDX, IDC_SHORTDATEFORMAT, m_bShortDateFormat);
	DDX_Check(pDX, IDC_RELATIVETIMES, m_bRelativeTimes);
	DDX_Check(pDX, IDC_ASTERISKLOGPREFIX, m_bAsteriskLogPrefix);
	DDX_Check(pDX, IDC_DIFFBYDOUBLECLICK, m_bDiffByDoubleClick);
	DDX_Check(pDX, IDC_SYSTEMLOCALEFORDATES, m_bUseSystemLocaleForDates);
	DDX_Check(pDX, IDC_ABBREVIATERENAMINGS, m_bAbbreviateRenamings);
	DDX_Check(pDX, IDC_SYMBOLIZEREFNAMES, m_bSymbolizeRefNames);
	DDX_Check(pDX, IDC_ENABLELOGCACHE, m_bEnableLogCache);
	DDX_Check(pDX, IDC_ENABLEGRAVATAR, m_bEnableGravatar);
	DDX_Text(pDX, IDC_GRAVATARURL, m_GravatarUrl);
	DDX_Control(pDX, IDC_GRAVATARURL, m_cGravatarUrl);
	DDX_Check(pDX, IDC_RIGHTSIDEBRANCHESTAGS, m_bDrawBranchesTagsOnRightSide);
	DDX_Check(pDX, IDC_SHOWDESCRIBE, m_bShowDescribe);
	DDX_Check(pDX, IDC_SHOWREVCOUNTER, m_bShowBranchRevNo);
	DDX_CBIndex(pDX, IDC_DESCRIBESTRATEGY, m_DescribeStrategy);
	DDX_Control(pDX, IDC_DESCRIBESTRATEGY, m_cDescribeStrategy);
	DDX_Text(pDX, IDC_DESCRIBEABBREVIATEDSIZE, m_DescribeAbbreviatedSize);
	DDX_Check(pDX, IDC_DESCRIBEALWAYSLONG, m_bDescribeAlwaysLong);
	DDX_Check(pDX, IDC_DESCRIBEONLYFIRSTPARENT, m_bDescribeOnlyFollowFirstParent);
	DDX_Check(pDX, IDC_FULLCOMMITMESSAGEONLOGLINE, m_bFullCommitMessageOnLogLine);
	DDX_Control(pDX, IDC_DEFAULT_NUMBER_OF, m_DefaultNumberOfCtl);
	DDX_Check(pDX, IDC_USEMAILMAP, m_bMailmapOnLog);
}

BEGIN_MESSAGE_MAP(CSetDialogs, ISettingsPropPage)
	ON_EN_CHANGE(IDC_DEFAULT_NUMBER_OF, OnChange)
	ON_CBN_SELCHANGE(IDC_DEFAULT_SCALE, OnCbnSelchangeDefaultlogscale)
	ON_BN_CLICKED(IDC_SHORTDATEFORMAT, OnChange)
	ON_BN_CLICKED(IDC_RELATIVETIMES, OnChange)
	ON_BN_CLICKED(IDC_ASTERISKLOGPREFIX, OnChange)
	ON_BN_CLICKED(IDC_SYSTEMLOCALEFORDATES, OnChange)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnChange)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnChange)
	ON_BN_CLICKED(IDC_DIFFBYDOUBLECLICK, OnChange)
	ON_BN_CLICKED(IDC_ABBREVIATERENAMINGS, OnChange)
	ON_BN_CLICKED(IDC_SYMBOLIZEREFNAMES, OnChange)
	ON_BN_CLICKED(IDC_ENABLELOGCACHE, OnChange)
	ON_BN_CLICKED(IDC_ENABLEGRAVATAR, OnChange)
	ON_EN_CHANGE(IDC_GRAVATARURL, OnChange)
	ON_BN_CLICKED(IDC_RIGHTSIDEBRANCHESTAGS, OnChange)
	ON_BN_CLICKED(IDC_SHOWDESCRIBE, OnChange)
	ON_BN_CLICKED(IDC_SHOWREVCOUNTER, OnChange)
	ON_CBN_SELCHANGE(IDC_DESCRIBESTRATEGY, OnChange)
	ON_EN_CHANGE(IDC_DESCRIBEABBREVIATEDSIZE, OnChange)
	ON_BN_CLICKED(IDC_DESCRIBEALWAYSLONG, OnChange)
	ON_BN_CLICKED(IDC_DESCRIBEONLYFIRSTPARENT, OnChange)
	ON_BN_CLICKED(IDC_FULLCOMMITMESSAGEONLOGLINE, OnChange)
	ON_BN_CLICKED(IDC_USEMAILMAP, OnChange)
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()

// CSetDialogs message handlers
BOOL CSetDialogs::OnInitDialog()
{
	CMFCFontComboBox::m_bDrawUsingFont = true;

	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SHORTDATEFORMAT);
	AdjustControlSize(IDC_RELATIVETIMES);
	AdjustControlSize(IDC_ASTERISKLOGPREFIX);
	AdjustControlSize(IDC_DIFFBYDOUBLECLICK);
	AdjustControlSize(IDC_SYSTEMLOCALEFORDATES);
	AdjustControlSize(IDC_ABBREVIATERENAMINGS);
	AdjustControlSize(IDC_SYMBOLIZEREFNAMES);
	AdjustControlSize(IDC_ENABLELOGCACHE);
	AdjustControlSize(IDC_ENABLEGRAVATAR);
	AdjustControlSize(IDC_RIGHTSIDEBRANCHESTAGS);
	AdjustControlSize(IDC_SHOWDESCRIBE);
	AdjustControlSize(IDC_SHOWREVCOUNTER);
	AdjustControlSize(IDC_DESCRIBEALWAYSLONG);
	AdjustControlSize(IDC_DESCRIBEONLYFIRSTPARENT);
	AdjustControlSize(IDC_FULLCOMMITMESSAGEONLOGLINE);
	AdjustControlSize(IDC_USEMAILMAP);

	EnableToolTips();

	m_bShortDateFormat = m_regShortDateFormat;
	m_bRelativeTimes = m_regRelativeTimes;
	m_bAsteriskLogPrefix = m_regAsteriskLogPrefix;
	m_bUseSystemLocaleForDates = m_regUseSystemLocaleForDates;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bDiffByDoubleClick = m_regDiffByDoubleClick;
	m_bAbbreviateRenamings = m_regAbbreviateRenamings;
	m_bSymbolizeRefNames = m_regSymbolizeRefNames;
	m_bEnableLogCache = m_regEnableLogCache;
	m_bEnableGravatar = m_regEnableGravatar;
	m_GravatarUrl = m_regGravatarUrl;
	m_bDrawBranchesTagsOnRightSide = m_regDrawBranchesTagsOnRightSide;
	m_bShowDescribe = m_regShowDescribe;
	m_bShowBranchRevNo = m_regShowBranchRevNo;
	m_DescribeStrategy = m_regDescribeStrategy;
	m_DescribeAbbreviatedSize = m_regDescribeAbbreviatedSize;
	m_bDescribeAlwaysLong = m_regDescribeAlwaysLong;
	m_bDescribeOnlyFollowFirstParent = m_regDescribeOnlyFollowFirstParent;
	m_bFullCommitMessageOnLogLine = m_regFullCommitMessageOnLogLine;
	m_bMailmapOnLog = m_regMailmapOnLog;

	CString temp;

	m_tooltips.AddTool(IDC_DEFAULT_NUMBER_OF, IDS_DEFAULT_NUMBER_OF_TT);
	m_tooltips.AddTool(IDC_DEFAULT_SCALE, IDS_DEFAULT_SCALE_TT);
	m_tooltips.AddTool(IDC_SHORTDATEFORMAT, IDS_SETTINGS_SHORTDATEFORMAT_TT);
	m_tooltips.AddTool(IDC_RELATIVETIMES, IDS_SETTINGS_RELATIVETIMES_TT);
	m_tooltips.AddTool(IDC_ASTERISKLOGPREFIX, IDS_SETTINGS_ASTERISKLOGPREFIX_TT);
	m_tooltips.AddTool(IDC_SYSTEMLOCALEFORDATES, IDS_SETTINGS_USESYSTEMLOCALEFORDATES_TT);
	m_tooltips.AddTool(IDC_DIFFBYDOUBLECLICK, IDS_SETTINGS_DIFFBYDOUBLECLICK_TT);
	m_tooltips.AddTool(IDC_ABBREVIATERENAMINGS, IDS_SETTINGS_ABBREVIATERENAMINGS_TT);
	m_tooltips.AddTool(IDC_SYMBOLIZEREFNAMES, IDS_SETTINGS_SYMBOLIZEREFNAMES_TT);
	m_tooltips.AddTool(IDC_ENABLELOGCACHE, IDS_SETTINGS_ENABLELOGCACHE_TT);
	m_tooltips.AddTool(IDC_ENABLEGRAVATAR, IDS_SETTINGS_ENABLEGRAVATAR_TT);
	m_tooltips.AddTool(IDC_GRAVATARURL, IDS_SETTINGS_GRAVATARURL_TT);
	m_tooltips.AddTool(IDC_SHOWDESCRIBE, IDS_SETTINGS_SHOWDESCRIBE_TT);
	m_tooltips.AddTool(IDC_SHOWREVCOUNTER, IDS_SETTINGS_SHOWREVCOUNTER_TT);
	m_tooltips.AddTool(IDC_DESCRIBESTRATEGY, IDS_SETTINGS_DESCRIBESTRATEGY_TT);
	m_tooltips.AddTool(IDC_DESCRIBEABBREVIATEDSIZE, IDS_SETTINGS_DESCRIBEABBREVIATEDSIZE_TT);
	m_tooltips.AddTool(IDC_DESCRIBEALWAYSLONG, IDS_SETTINGS_DESCRIBEALWAYSLONG_TT);

	m_cDefaultLogsScale.AddString(CString(MAKEINTRESOURCE(IDS_NO_LIMIT)));
	m_cDefaultLogsScale.AddString(CString(MAKEINTRESOURCE(IDS_LAST_SEL_DATE)));
	temp.Format(IDS_LAST_N_COMMITS, L"N");
	m_cDefaultLogsScale.AddString(temp);
	temp.Format(IDS_LAST_N_YEARS, L"N");
	m_cDefaultLogsScale.AddString(temp);
	temp.Format(IDS_LAST_N_MONTHS, L"N");
	m_cDefaultLogsScale.AddString(temp);
	temp.Format(IDS_LAST_N_WEEKS, L"N");
	m_cDefaultLogsScale.AddString(temp);
	m_cDefaultLogsScale.SetCurSel(static_cast<DWORD>(m_regDefaultLogsScale));

	switch (m_regDefaultLogsScale)
	{
	case CFilterData::SHOW_NO_LIMIT:
	case CFilterData::SHOW_LAST_SEL_DATE:
		m_sDefaultLogs.Empty();
		m_DefaultNumberOfCtl.EnableWindow(FALSE);
		break;
	case CFilterData::SHOW_LAST_N_COMMITS:
	case CFilterData::SHOW_LAST_N_YEARS:
	case CFilterData::SHOW_LAST_N_MONTHS:
	case CFilterData::SHOW_LAST_N_WEEKS:
		m_sDefaultLogs.Format(L"%ld", static_cast<DWORD>(m_regDefaultLogs));
		break;
	}

	int count = 0;
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
	m_cFontNames.SendMessage(CB_SETITEMHEIGHT, WPARAM(-1), m_cFontSizes.GetItemHeight(-1));

	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=mm");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=identicon");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=monsterid");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=wavatar");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=retro");
	m_cGravatarUrl.AddString(L"http://www.gravatar.com/avatar/%HASH%?d=blank");;

	m_cDescribeStrategy.AddString(CString(MAKEINTRESOURCE(IDS_ANNOTATEDTAGS)));
	m_cDescribeStrategy.AddString(CString(MAKEINTRESOURCE(IDS_ALLTAGS)));
	m_cDescribeStrategy.AddString(CString(MAKEINTRESOURCE(IDS_ALLREFS)));

	UpdateData(FALSE);
	return TRUE;
}

void CSetDialogs::OnChange()
{
	SetModified();
}

void CSetDialogs::OnCbnSelchangeDefaultlogscale()
{
	UpdateData();
	int sel = m_cDefaultLogsScale.GetCurSel();
	if (sel > 1 && (m_sDefaultLogs.IsEmpty() || _wtol(static_cast<LPCTSTR>(m_sDefaultLogs)) == 0))
		m_sDefaultLogs.Format(L"%ld", static_cast<DWORD>(m_regDefaultLogs));
	else if (sel <= 1)
		m_sDefaultLogs.Empty();
	m_DefaultNumberOfCtl.EnableWindow(sel > 1);
	UpdateData(FALSE);
	SetModified();
}

BOOL CSetDialogs::OnApply()
{
	UpdateData();
	if (m_cFontNames.GetSelFont())
		m_sFontName = m_cFontNames.GetSelFont()->m_strName;
	else
		m_sFontName = m_regFontName;

	Store(m_bShortDateFormat, m_regShortDateFormat);
	Store(m_bRelativeTimes, m_regRelativeTimes);
	Store(m_bAsteriskLogPrefix, m_regAsteriskLogPrefix);
	Store(m_bUseSystemLocaleForDates, m_regUseSystemLocaleForDates);

	int sel = m_cDefaultLogsScale.GetCurSel();
	Store(sel > 0 ? sel : 0, m_regDefaultLogsScale);

	int val = _wtol(static_cast<LPCTSTR>(m_sDefaultLogs));
	if (sel > 1 && val > 0)
		Store(val, m_regDefaultLogs);

	Store(m_sFontName, m_regFontName);
	Store(m_dwFontSize, m_regFontSize);
	Store(m_bDiffByDoubleClick, m_regDiffByDoubleClick);
	Store(m_bAbbreviateRenamings, m_regAbbreviateRenamings);
	Store(m_bSymbolizeRefNames, m_regSymbolizeRefNames);
	Store(m_bEnableLogCache, m_regEnableLogCache);
	Store(m_bEnableGravatar, m_regEnableGravatar);
	Store(m_GravatarUrl, m_regGravatarUrl);
	Store(m_bDrawBranchesTagsOnRightSide, m_regDrawBranchesTagsOnRightSide);
	Store(m_bShowDescribe, m_regShowDescribe);
	Store(m_bShowBranchRevNo, m_regShowBranchRevNo);
	Store(m_DescribeStrategy, m_regDescribeStrategy);
	Store(m_DescribeAbbreviatedSize, m_regDescribeAbbreviatedSize);
	Store(m_bDescribeAlwaysLong, m_regDescribeAlwaysLong);
	Store(m_bDescribeOnlyFollowFirstParent, m_regDescribeOnlyFollowFirstParent);
	Store(m_bFullCommitMessageOnLogLine, m_regFullCommitMessageOnLogLine);
	Store(m_bMailmapOnLog, m_regMailmapOnLog);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetDialogs::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
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
