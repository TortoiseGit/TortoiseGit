// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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
#include "GitProgressDlg.h"
#include "SetDialogs.h"

IMPLEMENT_DYNAMIC(CSetDialogs, ISettingsPropPage)
CSetDialogs::CSetDialogs()
	: ISettingsPropPage(CSetDialogs::IDD)
	, m_bShortDateFormat(FALSE)
	, m_bRelativeTimes(FALSE)
	, m_bAsteriskLogPrefix(TRUE)
	, m_dwFontSize(0)
	, m_sFontName(_T(""))
	, m_bDiffByDoubleClick(FALSE)
	, m_bUseSystemLocaleForDates(FALSE)
	, m_bAbbreviateRenamings(FALSE)
	, m_bSymbolizeRefNames(FALSE)
	, m_bEnableLogCache(TRUE)
	, m_bDrawBranchesTagsOnRightSide(FALSE)
	, m_bEnableGravatar(FALSE)
	, m_bShowDescribe(FALSE)
	, m_DescribeStrategy(GIT_DESCRIBE_DEFAULT)
	, m_DescribeAbbreviatedSize(GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE)
	, m_bDescribeAlwaysLong(FALSE)
	, m_bFullCommitMessageOnLogLine(FALSE)
{
	m_regShortDateFormat = CRegDWORD(_T("Software\\TortoiseGit\\LogDateFormat"), TRUE);
	m_regRelativeTimes = CRegDWORD(_T("Software\\TortoiseGit\\RelativeTimes"), FALSE);
	m_regAsteriskLogPrefix = CRegDWORD(_T("Software\\TortoiseGit\\AsteriskLogPrefix"), TRUE);
	m_regUseSystemLocaleForDates = CRegDWORD(_T("Software\\TortoiseGit\\UseSystemLocaleForDates"), TRUE);
	m_regFontName = CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8);
	m_regDiffByDoubleClick = CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE);
	m_regAbbreviateRenamings = CRegDWORD(_T("Software\\TortoiseGit\\AbbreviateRenamings"), FALSE);
	m_regSymbolizeRefNames = CRegDWORD(_T("Software\\TortoiseGit\\SymbolizeRefNames"), FALSE);
	m_regEnableLogCache = CRegDWORD(_T("Software\\TortoiseGit\\EnableLogCache"), TRUE);
	m_regEnableGravatar = CRegDWORD(_T("Software\\TortoiseGit\\EnableGravatar"), FALSE);
	m_regGravatarUrl = CRegString(_T("Software\\TortoiseGit\\GravatarUrl"), _T("http://www.gravatar.com/avatar/%HASH%?d=identicon"));
	m_regDrawBranchesTagsOnRightSide = CRegDWORD(_T("Software\\TortoiseGit\\DrawTagsBranchesOnRightSide"), FALSE);
	m_regShowDescribe = CRegDWORD(_T("Software\\TortoiseGit\\ShowDescribe"), FALSE);
	m_regDescribeStrategy = CRegDWORD(_T("Software\\TortoiseGit\\DescribeStrategy"), GIT_DESCRIBE_DEFAULT);
	m_regDescribeAbbreviatedSize = CRegDWORD(_T("Software\\TortoiseGit\\DescribeAbbreviatedSize"), GIT_DESCRIBE_DEFAULT_ABBREVIATED_SIZE);
	m_regDescribeAlwaysLong = CRegDWORD(_T("Software\\TortoiseGit\\DescribeAlwaysLong"), FALSE);
	m_regFullCommitMessageOnLogLine = CRegDWORD(_T("Software\\TortoiseGit\\FullCommitMessageOnLogLine"), FALSE);
}

CSetDialogs::~CSetDialogs()
{
}

void CSetDialogs::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
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
	DDX_CBIndex(pDX, IDC_DESCRIBESTRATEGY, m_DescribeStrategy);
	DDX_Control(pDX, IDC_DESCRIBESTRATEGY, m_cDescribeStrategy);
	DDX_Text(pDX, IDC_DESCRIBEABBREVIATEDSIZE, m_DescribeAbbreviatedSize);
	DDX_Check(pDX, IDC_DESCRIBEALWAYSLONG, m_bDescribeAlwaysLong);
	DDX_Check(pDX, IDC_FULLCOMMITMESSAGEONLOGLINE, m_bFullCommitMessageOnLogLine);
}

BEGIN_MESSAGE_MAP(CSetDialogs, ISettingsPropPage)
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
	ON_CBN_SELCHANGE(IDC_DESCRIBESTRATEGY, OnChange)
	ON_EN_CHANGE(IDC_DESCRIBEABBREVIATEDSIZE, OnChange)
	ON_BN_CLICKED(IDC_DESCRIBEALWAYSLONG, OnChange)
	ON_BN_CLICKED(IDC_FULLCOMMITMESSAGEONLOGLINE, OnChange)
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
	AdjustControlSize(IDC_DESCRIBEALWAYSLONG);
	AdjustControlSize(IDC_FULLCOMMITMESSAGEONLOGLINE);

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
	m_DescribeStrategy = m_regDescribeStrategy;
	m_DescribeAbbreviatedSize = m_regDescribeAbbreviatedSize;
	m_bDescribeAlwaysLong = m_regDescribeAlwaysLong;
	m_bFullCommitMessageOnLogLine = m_regFullCommitMessageOnLogLine;

	CString temp;

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
	m_tooltips.AddTool(IDC_DESCRIBESTRATEGY, IDS_SETTINGS_DESCRIBESTRATEGY_TT);
	m_tooltips.AddTool(IDC_DESCRIBEABBREVIATEDSIZE, IDS_SETTINGS_DESCRIBEABBREVIATEDSIZE_TT);
	m_tooltips.AddTool(IDC_DESCRIBEALWAYSLONG, IDS_SETTINGS_DESCRIBEALWAYSLONG_TT);

	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(_T("%d"), i);
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
		temp.Format(_T("%lu"), m_dwFontSize);
		m_cFontSizes.SetWindowText(temp);
	}

	m_cFontNames.Setup(DEVICE_FONTTYPE|RASTER_FONTTYPE|TRUETYPE_FONTTYPE, 1, FIXED_PITCH);
	m_cFontNames.SelectFont(m_sFontName);

	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=mm"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=identicon"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=monsterid"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=wavatar"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=retro"));
	m_cGravatarUrl.AddString(_T("http://www.gravatar.com/avatar/%HASH%?d=blank"));;

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
	Store(m_DescribeStrategy, m_regDescribeStrategy);
	Store(m_DescribeAbbreviatedSize, m_regDescribeAbbreviatedSize);
	Store(m_bDescribeAlwaysLong, m_regDescribeAlwaysLong);
	Store(m_bFullCommitMessageOnLogLine, m_regFullCommitMessageOnLogLine);

	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
