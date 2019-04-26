// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2003-2011, 2014-2016, 2018 - TortoiseSVN

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
#include "StatGraphDlg.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "registry.h"
#include "FormatMessageWrapper.h"
#include "SysProgressDlg.h"

#include <cmath>
#include <locale>
#include <utility>
#include <strsafe.h>

using namespace Gdiplus;

// BinaryPredicate for comparing authors based on their commit count
template<class DataType>
class MoreCommitsThan {
public:
	typedef std::map<tstring, DataType> MapType;
	MoreCommitsThan(MapType &author_commits) : m_authorCommits(author_commits) {}

	bool operator()(const tstring& lhs, const tstring& rhs) {
		return (m_authorCommits)[lhs] > (m_authorCommits)[rhs];
	}

private:
	MapType &m_authorCommits;
};


IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableStandAloneDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=nullptr*/)
: CResizableStandAloneDialog(CStatGraphDlg::IDD, pParent)
, m_bStacked(FALSE)
, m_GraphType(MyGraph::Bar)
, m_bAuthorsCaseSensitive(TRUE)
, m_bSortByCommitCount(TRUE)
, m_bUseCommitterNames(FALSE)
, m_bUseCommitDates(TRUE)
, m_nWeeks(-1)
, m_nDays(-1)
, m_langOrder(0)
, m_firstInterval(0)
, m_lastInterval(0)
, m_nTotalCommits(0)
, m_nTotalLinesInc(0)
, m_nTotalLinesDec(0)
, m_nTotalLinesNew(0)
, m_nTotalLinesDel(0)
, m_bDiffFetched(FALSE)
, m_minDate(0)
, m_maxDate(0)
, m_nTotalFileChanges(0)
{
}

CStatGraphDlg::~CStatGraphDlg()
{
	ClearGraph();
}

void CStatGraphDlg::OnOK() {
	StoreCurrentGraphType();
	__super::OnOK();
}

void CStatGraphDlg::OnCancel() {
	StoreCurrentGraphType();
	__super::OnCancel();
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPH, m_graph);
	DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
	DDX_Control(pDX, IDC_SKIPPER, m_Skipper);
	DDX_Check(pDX, IDC_AUTHORSCASESENSITIVE, m_bAuthorsCaseSensitive);
	DDX_Check(pDX, IDC_SORTBYCOMMITCOUNT, m_bSortByCommitCount);
	DDX_Check(pDX, IDC_COMMITTERNAMES, m_bUseCommitterNames);
	DDX_Check(pDX, IDC_COMMITDATES, m_bUseCommitDates);
	DDX_Control(pDX, IDC_GRAPHBARBUTTON, m_btnGraphBar);
	DDX_Control(pDX, IDC_GRAPHBARSTACKEDBUTTON, m_btnGraphBarStacked);
	DDX_Control(pDX, IDC_GRAPHLINEBUTTON, m_btnGraphLine);
	DDX_Control(pDX, IDC_GRAPHLINESTACKEDBUTTON, m_btnGraphLineStacked);
	DDX_Control(pDX, IDC_GRAPHPIEBUTTON, m_btnGraphPie);
}


BEGIN_MESSAGE_MAP(CStatGraphDlg, CResizableStandAloneDialog)
	ON_CBN_SELCHANGE(IDC_GRAPHCOMBO, OnCbnSelchangeGraphcombo)
	ON_WM_HSCROLL()
	ON_NOTIFY(TTN_NEEDTEXT, nullptr, OnNeedText)
	ON_BN_CLICKED(IDC_AUTHORSCASESENSITIVE, &CStatGraphDlg::AuthorsCaseSensitiveChanged)
	ON_BN_CLICKED(IDC_SORTBYCOMMITCOUNT, &CStatGraphDlg::SortModeChanged)
	ON_BN_CLICKED(IDC_GRAPHBARBUTTON, &CStatGraphDlg::OnBnClickedGraphbarbutton)
	ON_BN_CLICKED(IDC_GRAPHBARSTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphbarstackedbutton)
	ON_BN_CLICKED(IDC_GRAPHLINEBUTTON, &CStatGraphDlg::OnBnClickedGraphlinebutton)
	ON_BN_CLICKED(IDC_GRAPHLINESTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphlinestackedbutton)
	ON_BN_CLICKED(IDC_GRAPHPIEBUTTON, &CStatGraphDlg::OnBnClickedGraphpiebutton)
	ON_COMMAND(ID_FILE_SAVESTATGRAPHAS, &CStatGraphDlg::OnFileSavestatgraphas)
	ON_BN_CLICKED(IDC_CALC_DIFF, &CStatGraphDlg::OnBnClickedFetchDiff)
	ON_BN_CLICKED(IDC_COMMITTERNAMES, &CStatGraphDlg::OnBnClickedCommitternames)
	ON_BN_CLICKED(IDC_COMMITDATES, &CStatGraphDlg::OnBnClickedCommitdates)
END_MESSAGE_MAP()

void CStatGraphDlg::LoadStatQueries (__in UINT curStr, Metrics loadMetric, bool setDef /* = false */)
{
	CString temp;
	temp.LoadString(curStr);
	int sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, loadMetric);

	if (setDef) m_cGraphType.SetCurSel(sel);
}

void CStatGraphDlg::SetSkipper (bool reloadSkiper)
{
	// We need to limit the number of authors due to GUI resource limitation.
	// However, since author #251 will properly have < 1000th of the commits,
	// the resolution limit of the screen will already not allow for displaying
	// it in a reasonable way

	int max_authors_count = max(1, static_cast<int>(min(m_authorNames.size(), size_t(250))));
	m_Skipper.SetRange (1, max_authors_count);
	m_Skipper.SetPageSize(5);

	if (reloadSkiper)
		m_Skipper.SetPos (max_authors_count);
}

BOOL CStatGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_tooltips.AddTool(&m_btnGraphPie, IDS_STATGRAPH_PIEBUTTON_TT);
	m_tooltips.AddTool(&m_btnGraphLineStacked, IDS_STATGRAPH_LINESTACKEDBUTTON_TT);
	m_tooltips.AddTool(&m_btnGraphLine, IDS_STATGRAPH_LINEBUTTON_TT);
	m_tooltips.AddTool(&m_btnGraphBarStacked, IDS_STATGRAPH_BARSTACKEDBUTTON_TT);
	m_tooltips.AddTool(&m_btnGraphBar, IDS_STATGRAPH_BARBUTTON_TT);
	m_tooltips.Activate(TRUE);

	m_bAuthorsCaseSensitive = DWORD(CRegDWORD(L"Software\\TortoiseGit\\StatAuthorsCaseSensitive", m_bAuthorsCaseSensitive));
	m_bSortByCommitCount = DWORD(CRegDWORD(L"Software\\TortoiseGit\\StatSortByCommitCount", m_bSortByCommitCount));
	m_bUseCommitterNames = DWORD(CRegDWORD(L"Software\\TortoiseGit\\StatCommiterNames", m_bUseCommitterNames));
	m_bUseCommitDates = DWORD(CRegDWORD(L"Software\\TortoiseGit\\StatCommitDates", m_bUseCommitDates));
	UpdateData(FALSE);

	// gather statistics data, only needs to be updated when the checkbox with
	// the case sensitivity of author names is changed
	GatherData();

	//Load statistical queries
	LoadStatQueries(IDS_STATGRAPH_STATS, AllStat, true);
	LoadStatQueries(IDS_STATGRAPH_COMMITSBYDATE, CommitsByDate);
	LoadStatQueries(IDS_STATGRAPH_COMMITSBYAUTHOR, CommitsByAuthor);
	LoadStatQueries(IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIP, PercentageOfAuthorship);
	LoadStatQueries(IDS_STATGRAPH_LINES_BYDATE_W, LinesWByDate);
	LoadStatQueries(IDS_STATGRAPH_LINES_BYDATE_WO, LinesWOByDate);

	// set the dialog title to "Statistics - path/to/whatever/we/show/the/statistics/for"
	CString sTitle;
	GetWindowText(sTitle);
	if (m_path.IsEmpty())
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sTitle);
	else
		CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sTitle);

	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	m_btnGraphBar.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHBAR, iconWidth, iconHeight));
	m_btnGraphBar.SizeToContent();
	m_btnGraphBar.Invalidate();
	m_btnGraphBarStacked.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHBARSTACKED, iconWidth, iconHeight));
	m_btnGraphBarStacked.SizeToContent();
	m_btnGraphBarStacked.Invalidate();
	m_btnGraphLine.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHLINE, iconWidth, iconHeight));
	m_btnGraphLine.SizeToContent();
	m_btnGraphLine.Invalidate();
	m_btnGraphLineStacked.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHLINESTACKED, iconWidth, iconHeight));
	m_btnGraphLineStacked.SizeToContent();
	m_btnGraphLineStacked.Invalidate();
	m_btnGraphPie.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHPIE, iconWidth, iconHeight));
	m_btnGraphPie.SizeToContent();
	m_btnGraphPie.Invalidate();

	AdjustControlSize(IDC_AUTHORSCASESENSITIVE);
	AdjustControlSize(IDC_SORTBYCOMMITCOUNT);
	AdjustControlSize(IDC_COMMITTERNAMES);
	AdjustControlSize(IDC_COMMITDATES);

	AddAnchor(IDC_GRAPHTYPELABEL, TOP_LEFT);
	AddAnchor(IDC_GRAPH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHCOMBO, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_NUMWEEK, TOP_LEFT);
	AddAnchor(IDC_NUMWEEKVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMAUTHOR, TOP_LEFT);
	AddAnchor(IDC_NUMAUTHORVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMCOMMITS, TOP_LEFT);
	AddAnchor(IDC_NUMCOMMITSVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMFILECHANGES, TOP_LEFT);
	AddAnchor(IDC_NUMFILECHANGESVALUE, TOP_RIGHT);

	AddAnchor(IDC_TOTAL_LINE_WITHOUT_NEW_DEL, TOP_LEFT);
	AddAnchor(IDC_TOTAL_LINE_WITHOUT_NEW_DEL_VALUE, TOP_RIGHT);
	AddAnchor(IDC_TOTAL_LINE_WITH_NEW_DEL, TOP_LEFT);
	AddAnchor(IDC_TOTAL_LINE_WITH_NEW_DEL_VALUE, TOP_RIGHT);

	AddAnchor(IDC_CALC_DIFF, TOP_RIGHT);

	AddAnchor(IDC_AVG, TOP_RIGHT);
	AddAnchor(IDC_MIN, TOP_RIGHT);
	AddAnchor(IDC_MAX, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEK, TOP_LEFT);
	AddAnchor(IDC_MOSTACTIVEAUTHOR, TOP_LEFT);
	AddAnchor(IDC_LEASTACTIVEAUTHOR, TOP_LEFT);
	AddAnchor(IDC_MOSTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEK, TOP_LEFT);
	AddAnchor(IDC_COMMITSEACHWEEKAVG, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEKMIN, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEKMAX, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORAVG, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORMIN, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORMAX, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORAVG, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORMIN, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORMAX, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKAVG, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKMIN, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKMAX, TOP_RIGHT);

	AddAnchor(IDC_GRAPHBARBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHBARSTACKEDBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHLINEBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHLINESTACKEDBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHPIEBUTTON, BOTTOM_RIGHT);

	AddAnchor(IDC_AUTHORSCASESENSITIVE, BOTTOM_LEFT);
	AddAnchor(IDC_SORTBYCOMMITCOUNT, BOTTOM_LEFT);
	AddAnchor(IDC_COMMITTERNAMES, BOTTOM_LEFT);
	AddAnchor(IDC_COMMITDATES, BOTTOM_LEFT);
	AddAnchor(IDC_SKIPPER, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SKIPPERLABEL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(L"StatGraphDlg");

	// set the min/max values on the skipper
	SetSkipper (true);

	// we use a stats page encoding here, 0 stands for the statistics dialog
	CRegDWORD lastStatsPage(L"Software\\TortoiseGit\\LastViewedStatsPage", 0);

	// open last viewed statistics page as first page
	int graphtype = lastStatsPage / 10;
	for (int i = 0; i < m_cGraphType.GetCount(); i++)
	{
		if (static_cast<int>(m_cGraphType.GetItemData(i)) == graphtype)
		{
			m_cGraphType.SetCurSel(i);
			break;
		}
	}

	OnCbnSelchangeGraphcombo();

	int statspage = lastStatsPage % 10;
	switch (statspage) {
		case 1 :
			m_GraphType = MyGraph::Bar;
			m_bStacked = true;
			break;
		case 2 :
			m_GraphType = MyGraph::Bar;
			m_bStacked = false;
			break;
		case 3 :
			m_GraphType = MyGraph::Line;
			m_bStacked = true;
			break;
		case 4 :
			m_GraphType = MyGraph::Line;
			m_bStacked = false;
			break;
		case 5 :
			m_GraphType = MyGraph::PieChart;
			break;

		default : return TRUE;
	}

	LCID m_locale = MAKELCID(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))), SORT_DEFAULT);

	bool bUseSystemLocale = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\UseSystemLocaleForDates", TRUE));
	LCID locale = bUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : m_locale;

	TCHAR langBuf[11] = { 0 };
	GetLocaleInfo(locale, LOCALE_IDATE, langBuf, _countof(langBuf));

	m_langOrder = _wtoi(langBuf);

	return TRUE;
}

void CStatGraphDlg::ShowLabels(BOOL bShow)
{
	if (m_parAuthors.IsEmpty() || m_parDates.IsEmpty() || m_parFileChanges.IsEmpty())
		return;

	int nCmdShow = bShow ? SW_SHOW : SW_HIDE;

	GetDlgItem(IDC_GRAPH)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_NUMWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMWEEKVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMAUTHORVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMCOMMITS)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMCOMMITSVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMFILECHANGES)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMFILECHANGESVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_TOTAL_LINE_WITHOUT_NEW_DEL)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_TOTAL_LINE_WITHOUT_NEW_DEL_VALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_TOTAL_LINE_WITH_NEW_DEL)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_TOTAL_LINE_WITH_NEW_DEL_VALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_CALC_DIFF)->ShowWindow(nCmdShow && !m_bDiffFetched);

	GetDlgItem(IDC_AVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
	//GetDlgItem(IDC_FILECHANGESEACHWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMAX)->ShowWindow(nCmdShow);

	GetDlgItem(IDC_SORTBYCOMMITCOUNT)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SKIPPER)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SKIPPERLABEL)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBar.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBarStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLine.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLineStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphPie.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
}

void CStatGraphDlg::UpdateWeekCount()
{
	// Sanity check
	if (m_parDates.IsEmpty())
		return;

	// Already updated? No need to do it again.
	if (m_nWeeks >= 0)
		return;

	// Determine first and last date in dates array
	__time64_t min_date = static_cast<__time64_t>(m_parDates.GetAt(0));
	__time64_t max_date = min_date;
	INT_PTR count = m_parDates.GetCount();
	for (INT_PTR i=0; i<count; ++i)
	{
		__time64_t d = static_cast<__time64_t>(m_parDates.GetAt(i));
		if (d < min_date)		min_date = d;
		else if (d > max_date)	max_date = d;
	}

	// Store start date of the interval in the member variable m_minDate
	m_minDate = min_date;
	m_maxDate = max_date;

	// How many weeks does the time period cover?

	// Get time difference between start and end date
	double secs = _difftime64(max_date, m_minDate);

	m_nWeeks =	static_cast<int>(ceil(secs / static_cast<double>(m_SecondsInWeek)));
	m_nDays =	static_cast<int>(ceil(secs / static_cast<double>(m_SecondsInDay)));
}

int CStatGraphDlg::GetCalendarWeek(const CTime& time)
{
	// Note:
	// the calculation of the calendar week is wrong if DST is in effect
	// and the date to calculate the week for is in DST and within the range
	// of the DST offset (e.g. one hour).
	// For example, if DST starts on Sunday march 30 and the date to get the week for
	// is Monday, march 31, 0:30:00, then the returned week is one week less than
	// the real week.
	// TODO: ?
	// write a function
	// getDSTOffset(const CTime& time)
	// which returns the DST offset for a given time/date. Then we can use this offset
	// to correct our GetDays() calculation to get the correct week again
	// This of course won't work for 'history' dates, because Windows doesn't have
	// that information (only Vista has such a function: GetTimeZoneInformationForYear() )
	int iWeekOfYear = 0;

	int iYear = time.GetYear();
	int iFirstDayOfWeek = 0;
	int iFirstWeekOfYear = 0;
	TCHAR loc[2] = { 0 };
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, loc, _countof(loc));
	iFirstDayOfWeek = int(loc[0]-'0');
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, loc, _countof(loc));
	iFirstWeekOfYear = int(loc[0]-'0');
	CTime dDateFirstJanuary(iYear,1,1,0,0,0);
	int iDayOfWeek = (dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;

	// Select mode
	// 0 Week containing 1/1 is the first week of that year.
	// 1 First full week following 1/1 is the first week of that year.
	// 2 First week containing at least four days is the first week of that year.
	switch (iFirstWeekOfYear)
	{
		case 0:
			{
				// Week containing 1/1 is the first week of that year.

				// check if this week reaches into the next year
				dDateFirstJanuary = CTime(iYear+1,1,1,0,0,0);

				// Get start of week
				try
				{
					iDayOfWeek = (time.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
				}
				catch (CAtlException)
				{
				}
				CTime dStartOfWeek = time-CTimeSpan(iDayOfWeek,0,0,0);

				// If this week spans over to 1/1 this is week 1
				if (dStartOfWeek + CTimeSpan(6,0,0,0) >= dDateFirstJanuary)
				{
					// we are in the last week of the year that spans over 1/1
					iWeekOfYear = 1;
				}
				else
				{
					// Get week day of 1/1
					dDateFirstJanuary = CTime(iYear,1,1,0,0,0);
					iDayOfWeek = (dDateFirstJanuary.GetDayOfWeek() +5 + iFirstDayOfWeek) % 7;
					// Just count from 1/1
					iWeekOfYear = static_cast<int>(((time-dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) + 1;
				}
			}
			break;
		case 1:
			{
				// First full week following 1/1 is the first week of that year.

				// If the 1.1 is the start of the week everything is ok
				// else we need the next week is the correct result
				iWeekOfYear =
					static_cast<int>(((time-dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) +
					(iDayOfWeek==0 ? 1:0);

				// If we are in week 0 we are in the first not full week
				// calculate from the last year
				if (iWeekOfYear==0)
				{
					// Special case: we are in the week of 1.1 but 1.1. is not on the
					// start of week. Calculate based on the last year
					dDateFirstJanuary = CTime(iYear-1,1,1,0,0,0);
					iDayOfWeek =
						(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
					// and we correct this in the same we we done this before but
					// the result is now 52 or 53 and not 0
					iWeekOfYear =
						static_cast<int>(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
						(iDayOfWeek<=3 ? 1:0);
				}
			}
			break;
		case 2:
			{
				// First week containing at least four days is the first week of that year.

				// Each year can start with any day of the week. But our
				// weeks always start with Monday. So we add the day of week
				// before calculation of the final week of year.
				// Rule: is the 1.1 a Mo,Tu,We,Th than the week starts on the 1.1 with
				// week==1, else a week later, so we add one for all those days if
				// day is less <=3 Mo,Tu,We,Th. Otherwise 1.1 is in the last week of the
				// previous year
				iWeekOfYear =
					static_cast<int>(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
					(iDayOfWeek<=3 ? 1:0);

				// special cases
				if (iWeekOfYear==0)
				{
					// special case week 0. We got a day before the 1.1, 2.1 or 3.1, were the
					// 1.1. is not a Mo, Tu, We, Th. So the week 1 does not start with the 1.1.
					// So we calculate the week according to the 1.1 of the year before

					dDateFirstJanuary = CTime(iYear-1,1,1,0,0,0);
					iDayOfWeek =
						(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
					// and we correct this in the same we we done this before but the result
					// is now 52 or 53 and not 0
					iWeekOfYear =
						static_cast<int>(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
						(iDayOfWeek<=3 ? 1:0);
				}
				else if (iWeekOfYear==53)
				{
					// special case week 53. Either we got the correct week 53 or we just got the
					// week 1 of the next year. So is the 1.1.(year+1) also a Mo, Tu, We, Th than
					// we already have the week 1, otherwise week 53 is correct

					dDateFirstJanuary = CTime(iYear+1,1,1,0,0,0);
					iDayOfWeek =
						(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
					// 1.1. in week 1 or week 53?
					iWeekOfYear = iDayOfWeek<=3 ? 1:53;
				}
			}
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	// return result
	return iWeekOfYear;
}

int CStatGraphDlg::GatherData(BOOL fetchdiff, BOOL keepFetchedData)
{
	m_parAuthors.RemoveAll();
	m_parDates.RemoveAll();
	if (m_parFileChanges2.IsEmpty()) // Fixes issue #1948
		keepFetchedData = FALSE;
	if (!keepFetchedData)
	{
		m_parFileChanges.RemoveAll();
		m_lineInc.RemoveAll();
		m_lineDec.RemoveAll();
		m_lineDel.RemoveAll();
		m_lineNew.RemoveAll();
	}
	else
	{
		m_parFileChanges.Copy(m_parFileChanges2);
		m_lineNew.Copy(m_lineNew2);
		m_lineDel.Copy(m_lineDel2);
		m_lineInc.Copy(m_lineInc2);
		m_lineDec.Copy(m_lineDec2);
	}

	CSysProgressDlg progress;
	if (fetchdiff)
	{
		progress.SetTitle(CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_GATHERSTATISTICS)));
		progress.FormatNonPathLine(1, IDS_PROC_STATISTICS_DIFF);
		progress.SetTime(true);
		progress.ShowModeless(this);
	}

	// create arrays which are aware of the current filter
	ULONGLONG starttime = GetTickCount64();

	if (m_bUseCommitDates)
		std::sort(m_ShowList.begin(), m_ShowList.end(), [](GitRevLoglist* pLhs, GitRevLoglist* pRhs) { return pLhs->GetCommitterDate() > pRhs->GetCommitterDate(); });
	else
		std::sort(m_ShowList.begin(), m_ShowList.end(), [](GitRevLoglist* pLhs, GitRevLoglist* pRhs) { return pLhs->GetAuthorDate() > pRhs->GetAuthorDate(); });

	GIT_MAILMAP mailmap = nullptr;
	git_read_mailmap(&mailmap);
	for (size_t i = 0; i < m_ShowList.size(); ++i)
	{
		auto pLogEntry = m_ShowList[i];
		int inc, dec, incnewfile, decdeletedfile, files;
		inc = dec = incnewfile = decdeletedfile = files= 0;

		CString strAuthor = m_bUseCommitterNames ? pLogEntry->GetCommitterName() : pLogEntry->GetAuthorName();
		if (mailmap)
		{
			CStringA email2A = CUnicodeUtils::GetUTF8(m_bUseCommitterNames ? pLogEntry->GetCommitterEmail() : pLogEntry->GetAuthorEmail());
			struct payload_struct { GitRev* rev; const char *authorName; BOOL useCommitterNames; };
			payload_struct payload = { pLogEntry, nullptr, m_bUseCommitterNames };
			const char* author1 = nullptr;
			git_lookup_mailmap(mailmap, nullptr, &author1, email2A, &payload,
				[](void* payload) -> const char* { return reinterpret_cast<payload_struct*>(payload)->authorName = _strdup(CUnicodeUtils::GetUTF8(reinterpret_cast<payload_struct*>(payload)->useCommitterNames ? reinterpret_cast<payload_struct*>(payload)->rev->GetCommitterName() : reinterpret_cast<payload_struct*>(payload)->rev->GetAuthorName())); });
			free((void *)payload.authorName);
			if (author1)
				strAuthor = CUnicodeUtils::GetUnicode(author1);
		}
		if (strAuthor.IsEmpty())
			strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
		m_parAuthors.Add(strAuthor);
		if (m_bUseCommitDates)
			m_parDates.Add(static_cast<DWORD>(pLogEntry->GetCommitterDate().GetTime()));
		else
			m_parDates.Add(static_cast<DWORD>(pLogEntry->GetAuthorDate().GetTime()));

		if (fetchdiff && (pLogEntry->m_ParentHash.size() <= 1))
		{
			CTGitPathList& list = pLogEntry->GetFiles(nullptr);
			files = list.GetCount();

			for (int j = 0; j < files; j++)
			{
				if (list[j].m_Action & CTGitPath::LOGACTIONS_DELETED)
					decdeletedfile += _wtol(list[j].m_StatDel);
				else if(list[j].m_Action & CTGitPath::LOGACTIONS_ADDED)
					incnewfile += _wtol(list[j].m_StatAdd);
				else
				{
					inc += _wtol(list[j].m_StatAdd);
					dec += _wtol(list[j].m_StatDel);
				}

				if (progress.HasUserCancelled())
				{
					git_free_mailmap(mailmap);
					return -1;
				}
			}
		}
		if (!keepFetchedData)
		{
			m_parFileChanges.Add(files);
			m_lineInc.Add(inc);
			m_lineDec.Add(dec);
			m_lineDel.Add(decdeletedfile);
			m_lineNew.Add(incnewfile);
		}

		if (progress.IsVisible() && (GetTickCount64() - starttime > 100UL))
		{
			progress.FormatNonPathLine(2, L"%s: %s", static_cast<LPCTSTR>(pLogEntry->m_CommitHash.ToString(g_Git.GetShortHASHLength())), static_cast<LPCTSTR>(pLogEntry->GetSubject()));
			progress.SetProgress64(i, m_ShowList.size());
			starttime = GetTickCount64();
		}

	}
	git_free_mailmap(mailmap);

	if (fetchdiff)
	{
		m_parFileChanges2.Copy(m_parFileChanges);
		m_lineNew2.Copy(m_lineNew);
		m_lineDel2.Copy(m_lineDel);
		m_lineInc2.Copy(m_lineInc);
		m_lineDec2.Copy(m_lineDec);
	}

	m_nTotalCommits = m_parAuthors.GetCount();
	m_nTotalFileChanges = 0;

	// Update m_nWeeks and m_minDate
	UpdateWeekCount();

	// Now create a mapping that holds the information per week.
	m_commitsPerUnitAndAuthor.clear();
	m_filechangesPerUnitAndAuthor.clear();
	m_commitsPerAuthor.clear();
	m_PercentageOfAuthorship.clear();
	m_LinesWPerUnitAndAuthor.clear();
	m_LinesWOPerUnitAndAuthor.clear();

	int interval = 0;
	__time64_t d = static_cast<__time64_t>(m_parDates.GetAt(0));
	int nLastUnit = GetUnit(d);
	double AllContributionAuthor = 0;

	m_nTotalLinesInc = m_nTotalLinesDec = m_nTotalLinesNew = m_nTotalLinesDel =0;

	// Now loop over all weeks and gather the info
	for (LONG i=0; i<m_nTotalCommits; ++i)
	{
		// Find the interval number
		__time64_t commitDate = static_cast<__time64_t>(m_parDates.GetAt(i));
		int u = GetUnit(commitDate);
		if (nLastUnit != u)
			interval++;
		nLastUnit = u;
		// Find the authors name
		CString sAuth = m_parAuthors.GetAt(i);
		if (!m_bAuthorsCaseSensitive)
			sAuth = sAuth.MakeLower();
		tstring author = tstring(sAuth);
		// Increase total commit count for this author
		m_commitsPerAuthor[author]++;
		// Increase the commit count for this author in this week
		m_commitsPerUnitAndAuthor[interval][author]++;

		m_LinesWPerUnitAndAuthor[interval][author] += m_lineInc.GetAt(i) + m_lineDec.GetAt(i) + m_lineNew.GetAt(i) + + m_lineDel.GetAt(i);
		m_LinesWOPerUnitAndAuthor[interval][author] += m_lineInc.GetAt(i) + m_lineDec.GetAt(i);

		CTime t = m_parDates.GetAt(i);
		m_unitNames[interval] = GetUnitLabel(nLastUnit, t);
		// Increase the file change count for this author in this week
		int fileChanges = m_parFileChanges.GetAt(i);
		m_filechangesPerUnitAndAuthor[interval][author] += fileChanges;
		m_nTotalFileChanges += fileChanges;

		//calculate Contribution Author
		double contributionAuthor = CoeffContribution(static_cast<int>(m_nTotalCommits) - i -1) * (fileChanges ? fileChanges : 1);
		AllContributionAuthor += contributionAuthor;
		m_PercentageOfAuthorship[author] += contributionAuthor;

		m_nTotalLinesInc += m_lineInc.GetAt(i);
		m_nTotalLinesDec += m_lineDec.GetAt(i);
		m_nTotalLinesNew += m_lineNew.GetAt(i);
		m_nTotalLinesDel += m_lineDel.GetAt(i);
	}

	// Find first and last interval number.
	if (!m_commitsPerUnitAndAuthor.empty())
	{
		IntervalDataMap::iterator interval_it = m_commitsPerUnitAndAuthor.begin();
		m_firstInterval = interval_it->first;
		interval_it = m_commitsPerUnitAndAuthor.end();
		--interval_it;
		m_lastInterval = interval_it->first;
		// Sanity check - if m_lastInterval is too large it could freeze TSVN and take up all memory!!!
		assert(m_lastInterval >= 0 && m_lastInterval < 10000);
	}
	else
	{
		m_firstInterval = 0;
		m_lastInterval = -1;
	}

	// Get a list of authors names
	LoadListOfAuthors(m_commitsPerAuthor);

	// Calculate percent of Contribution Authors
	for (std::list<tstring>::iterator it = m_authorNames.begin(); it != m_authorNames.end(); ++it)
	{
		m_PercentageOfAuthorship[*it] =  (m_PercentageOfAuthorship[*it] *100)/ AllContributionAuthor;
	}

	// All done, now the statistics pages can retrieve the data and
	// extract the information to be shown.

	return 0;
}

void CStatGraphDlg::FilterSkippedAuthors(std::list<tstring>& included_authors,
										 std::list<tstring>& skipped_authors)
{
	included_authors.clear();
	skipped_authors.clear();

	unsigned int included_authors_count = m_Skipper.GetPos();
	// if we only leave out one author, still include him with his name
	if (included_authors_count + 1 == m_authorNames.size())
		++included_authors_count;

	// add the included authors first
	std::list<tstring>::iterator author_it = m_authorNames.begin();
	while (included_authors_count > 0 && author_it != m_authorNames.end())
	{
		// Add him/her to the included list
		included_authors.push_back(*author_it);
		++author_it;
		--included_authors_count;
	}

	// If we haven't reached the end yet, copy all remaining authors into the
	// skipped author list.
	std::copy(author_it, m_authorNames.end(), std::back_inserter(skipped_authors) );

	// Sort authors alphabetically if user wants that.
	if (!m_bSortByCommitCount)
		included_authors.sort();
}

bool  CStatGraphDlg::PreViewStat(bool fShowLabels)
{
	if (m_parAuthors.IsEmpty() || m_parDates.IsEmpty() || m_parFileChanges.IsEmpty())
		return false;
	ShowLabels(fShowLabels);

	//If view graphic
	if (!fShowLabels) ClearGraph();

	// This function relies on a previous call of GatherData().
	// This can be detected by checking the week count.
	// If the week count is equal to -1, it hasn't been called before.
	if (m_nWeeks == -1)
		GatherData(FALSE, TRUE);
	// If week count is still -1, something bad has happened, probably invalid data!
	if (m_nWeeks == -1)
		return false;

	return true;
}

MyGraphSeries *CStatGraphDlg::PreViewGraph(__in UINT GraphTitle, __in UINT YAxisLabel, __in UINT XAxisLabel /*= nullptr*/)
{
	if(!PreViewStat(false))
		return nullptr;

	// We need at least one author
	if (m_authorNames.empty())
		return nullptr;

	// Add a single series to the chart
	MyGraphSeries * graphData = new MyGraphSeries();
	m_graph.AddSeries(*graphData);
	m_graphDataArray.Add(graphData);

	// Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(YAxisLabel);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(XAxisLabel);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(GraphTitle);
	m_graph.SetGraphTitle(temp);

	return graphData;
}

void CStatGraphDlg::ShowPercentageOfAuthorship()
{
	// Set up the graph.
	MyGraphSeries * graphData = PreViewGraph(IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIP,
		IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIPY,
		IDS_STATGRAPH_COMMITSBYAUTHORMOREX);
	if (!graphData) return;

	// Find out which authors are to be shown and which are to be skipped.
	std::list<tstring> authors;
	std::list<tstring> others;


	FilterSkippedAuthors(authors, others);

	// Loop over all authors in the authors list and
	// add them to the graph.

	if (!authors.empty())
	{
		for (std::list<tstring>::iterator it = authors.begin(); it != authors.end(); ++it)
		{
			int group = m_graph.AppendGroup(it->c_str());
			graphData->SetData(group,  RollPercentageOfAuthorship(m_PercentageOfAuthorship[*it]));
		}
	}

	//     If we have other authors, count them and their commits.
	if (!others.empty())
		DrawOthers(others, graphData, m_PercentageOfAuthorship);

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
	// Set up the graph.
	MyGraphSeries * graphData = PreViewGraph(IDS_STATGRAPH_COMMITSBYAUTHOR,
		IDS_STATGRAPH_COMMITSBYAUTHORY,
		IDS_STATGRAPH_COMMITSBYAUTHORX);
	if (!graphData) return;

	// Find out which authors are to be shown and which are to be skipped.
	std::list<tstring> authors;
	std::list<tstring> others;
	FilterSkippedAuthors(authors, others);

	// Loop over all authors in the authors list and
	// add them to the graph.

	if (!authors.empty())
	{
		for (std::list<tstring>::iterator it = authors.begin(); it != authors.end(); ++it)
		{
			int group = m_graph.AppendGroup(it->c_str());
			graphData->SetData(group, m_commitsPerAuthor[*it]);
		}
	}

	//     If we have other authors, count them and their commits.
	if (!others.empty())
		DrawOthers(others, graphData, m_commitsPerAuthor);

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowByDate(int stringx, int title, IntervalDataMap &data)
{
	if(!PreViewStat(false)) return;

	// We need at least one author
	if (m_authorNames.empty()) return;

	// Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(stringx);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(title);
	m_graph.SetGraphTitle(temp);

	m_graph.SetXAxisLabel(GetUnitString());

	// Find out which authors are to be shown and which are to be skipped.
	std::list<tstring> authors;
	std::list<tstring> others;
	FilterSkippedAuthors(authors, others);

	// Add a graph series for each author.
	AuthorDataMap authorGraphMap;
	for (std::list<tstring>::iterator it = authors.begin(); it != authors.end(); ++it)
		authorGraphMap[*it] = m_graph.AppendGroup(it->c_str());
	// If we have skipped authors, add a graph series for all those.
	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	tstring othersName;
	if (!others.empty())
	{
		sOthers.AppendFormat(L" (%Iu)", others.size());
		othersName = static_cast<LPCWSTR>(sOthers);
		authorGraphMap[othersName] = m_graph.AppendGroup(sOthers);
	}

	// Mapping to collect commit counts in each interval
	AuthorDataMap   commitCount;

	// Loop over all intervals/weeks and collect filtered data.
	// Sum up data in each interval until the time unit changes.
	for (int i=m_lastInterval; i>=m_firstInterval; --i)
	{
		// Collect data for authors listed by name.
		if (!authors.empty())
		{
			for (std::list<tstring>::iterator it = authors.begin(); it != authors.end(); ++it)
			{
				// Do we have some data for the current author in the current interval?
				AuthorDataMap::const_iterator data_it = data[i].find(*it);
				if (data_it == data[i].end())
					continue;
				commitCount[*it] += data_it->second;
			}
		}
		// Collect data for all skipped authors.
		if (!others.empty())
		{
			for (std::list<tstring>::iterator it = others.begin(); it != others.end(); ++it)
			{
				// Do we have some data for the author in the current interval?
				AuthorDataMap::const_iterator data_it = data[i].find(*it);
				if (data_it == data[i].end())
					continue;
				commitCount[othersName] += data_it->second;
			}
		}

		// Create a new data series for this unit/interval.
		MyGraphSeries * graphData = new MyGraphSeries();
		// Loop over all created graphs and set the corresponding data.
		if (!authorGraphMap.empty())
		{
			for (AuthorDataMap::const_iterator it = authorGraphMap.begin(); it != authorGraphMap.end(); ++it)
			{
				graphData->SetData(it->second, commitCount[it->first]);
			}
		}
		graphData->SetLabel(m_unitNames[i].c_str());
		m_graph.AddSeries(*graphData);
		m_graphDataArray.Add(graphData);

		// Reset commit count mapping.
		commitCount.clear();
	}

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowStats()
{
	if(!PreViewStat(true)) return;

	// Now we can use the gathered data to update the stats dialog.
	size_t nAuthors = m_authorNames.size();

	// Find most and least active author names.
	tstring mostActiveAuthor;
	tstring leastActiveAuthor;
	if (nAuthors > 0)
	{
		mostActiveAuthor = m_authorNames.front();
		leastActiveAuthor = m_authorNames.back();
	}

	// Obtain the statistics for the table.
	long nCommitsMin = -1;
	long nCommitsMax = -1;
	long nFileChangesMin = -1;
	long nFileChangesMax = -1;

	long nMostActiveMaxCommits = -1;
	long nMostActiveMinCommits = -1;
	long nLeastActiveMaxCommits = -1;
	long nLeastActiveMinCommits = -1;

	// Loop over all intervals and find min and max values for commit count and file changes.
	// Also store the stats for the most and least active authors.
	for (int i=m_firstInterval; i<=m_lastInterval; ++i)
	{
		// Loop over all commits in this interval and count the number of commits by all authors.
		int commitCount = 0;
		AuthorDataMap::iterator commit_endit = m_commitsPerUnitAndAuthor[i].end();
		for (AuthorDataMap::iterator commit_it = m_commitsPerUnitAndAuthor[i].begin();
			commit_it != commit_endit; ++commit_it)
		{
			commitCount += commit_it->second;
		}
		if (nCommitsMin == -1 || commitCount < nCommitsMin)
			nCommitsMin = commitCount;
		if (nCommitsMax == -1 || commitCount > nCommitsMax)
			nCommitsMax = commitCount;

		// Loop over all commits in this interval and count the number of file changes by all authors.
		int fileChangeCount = 0;
		AuthorDataMap::iterator filechange_endit = m_filechangesPerUnitAndAuthor[i].end();
		for (AuthorDataMap::iterator filechange_it = m_filechangesPerUnitAndAuthor[i].begin();
			filechange_it != filechange_endit; ++filechange_it)
		{
			fileChangeCount += filechange_it->second;
		}
		if (nFileChangesMin == -1 || fileChangeCount < nFileChangesMin)
			nFileChangesMin = fileChangeCount;
		if (nFileChangesMax == -1 || fileChangeCount > nFileChangesMax)
			nFileChangesMax = fileChangeCount;

		// also get min/max data for most and least active authors
		if (nAuthors > 0)
		{
			// check if author is present in this interval
			AuthorDataMap::iterator author_it = m_commitsPerUnitAndAuthor[i].find(mostActiveAuthor);
			long authorCommits;
			if (author_it == m_commitsPerUnitAndAuthor[i].end())
				authorCommits = 0;
			else
				authorCommits = author_it->second;
			if (nMostActiveMaxCommits == -1 || authorCommits > nMostActiveMaxCommits)
				nMostActiveMaxCommits = authorCommits;
			if (nMostActiveMinCommits == -1 || authorCommits < nMostActiveMinCommits)
				nMostActiveMinCommits = authorCommits;

			author_it = m_commitsPerUnitAndAuthor[i].find(leastActiveAuthor);
			if (author_it == m_commitsPerUnitAndAuthor[i].end())
				authorCommits = 0;
			else
				authorCommits = author_it->second;
			if (nLeastActiveMaxCommits == -1 || authorCommits > nLeastActiveMaxCommits)
				nLeastActiveMaxCommits = authorCommits;
			if (nLeastActiveMinCommits == -1 || authorCommits < nLeastActiveMinCommits)
				nLeastActiveMinCommits = authorCommits;
		}
	}
	if (nMostActiveMaxCommits == -1)    nMostActiveMaxCommits = 0;
	if (nMostActiveMinCommits == -1)    nMostActiveMinCommits = 0;
	if (nLeastActiveMaxCommits == -1)   nLeastActiveMaxCommits = 0;
	if (nLeastActiveMinCommits == -1)   nLeastActiveMinCommits = 0;

	int nWeeks = m_lastInterval-m_firstInterval;
	if (nWeeks == 0)
		nWeeks = 1;
	// Adjust the labels with the unit type (week, month, ...)
	CString labelText;
	labelText.Format(IDS_STATGRAPH_NUMBEROFUNIT, static_cast<LPCTSTR>(GetUnitString()));
	SetDlgItemText(IDC_NUMWEEK, labelText);
	labelText.Format(IDS_STATGRAPH_COMMITSBYUNIT, static_cast<LPCTSTR>(GetUnitString()));
	SetDlgItemText(IDC_COMMITSEACHWEEK, labelText);
	labelText.Format(IDS_STATGRAPH_FILECHANGESBYUNIT, static_cast<LPCTSTR>(GetUnitString()));
	SetDlgItemText(IDC_FILECHANGESEACHWEEK, static_cast<LPCTSTR>(labelText));
	// We have now all data we want and we can fill in the labels...
	CString number;
	number.Format(L"%d", nWeeks);
	SetDlgItemText(IDC_NUMWEEKVALUE, number);
	number.Format(L"%Iu", nAuthors);
	SetDlgItemText(IDC_NUMAUTHORVALUE, number);
	number.Format(L"%Id", m_nTotalCommits);
	SetDlgItemText(IDC_NUMCOMMITSVALUE, number);
	number.Format(L"%ld", m_nTotalFileChanges);
	if (m_bDiffFetched)
		SetDlgItemText(IDC_NUMFILECHANGESVALUE, number);

	number.Format(L"%Id", m_parAuthors.GetCount() / nWeeks);
	SetDlgItemText(IDC_COMMITSEACHWEEKAVG, number);
	number.Format(L"%ld", nCommitsMax);
	SetDlgItemText(IDC_COMMITSEACHWEEKMAX, number);
	number.Format(L"%ld", nCommitsMin);
	SetDlgItemText(IDC_COMMITSEACHWEEKMIN, number);

	number.Format(L"%ld", m_nTotalFileChanges / nWeeks);
	//SetDlgItemText(IDC_FILECHANGESEACHWEEKAVG, number);
	number.Format(L"%ld", nFileChangesMax);
	//SetDlgItemText(IDC_FILECHANGESEACHWEEKMAX, number);
	number.Format(L"%ld", nFileChangesMin);
	//SetDlgItemText(IDC_FILECHANGESEACHWEEKMIN, number);

	number.Format(L"%ld (%ld (+) %ld (-))", m_nTotalLinesInc + m_nTotalLinesDec, m_nTotalLinesInc, m_nTotalLinesDec);
	if (m_bDiffFetched)
		SetDlgItemText(IDC_TOTAL_LINE_WITHOUT_NEW_DEL_VALUE, number);
	number.Format(L"%ld (%ld (+) %ld (-))", m_nTotalLinesInc + m_nTotalLinesDec + m_nTotalLinesNew + m_nTotalLinesDel,
												m_nTotalLinesInc + m_nTotalLinesNew, m_nTotalLinesDec + m_nTotalLinesDel);
	if (m_bDiffFetched)
		SetDlgItemText(IDC_TOTAL_LINE_WITH_NEW_DEL_VALUE, number);

	if (nAuthors == 0)
	{
		SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, L"");
		SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, L"0");
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, L"0");
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, L"0");
		SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, L"");
		SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, L"0");
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, L"0");
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, L"0");
	}
	else
	{
		SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, mostActiveAuthor.c_str());
		number.Format(L"%ld", m_commitsPerAuthor[mostActiveAuthor] / nWeeks);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, number);
		number.Format(L"%ld", nMostActiveMaxCommits);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, number);
		number.Format(L"%ld", nMostActiveMinCommits);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, number);

		SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, leastActiveAuthor.c_str());
		number.Format(L"%ld", m_commitsPerAuthor[leastActiveAuthor] / nWeeks);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, number);
		number.Format(L"%ld", nLeastActiveMaxCommits);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, number);
		number.Format(L"%ld", nLeastActiveMinCommits);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, number);
	}
}

int CStatGraphDlg::RollPercentageOfAuthorship(double it)
{
	return static_cast<int>(it) + (it - static_cast<int>(it) >= 0.5);
}

void CStatGraphDlg::OnCbnSelchangeGraphcombo()
{
	UpdateData();

	Metrics useMetric = static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));
	switch (useMetric )
	{
	case AllStat:
	case CommitsByDate:
		// by date
		m_btnGraphLine.EnableWindow(TRUE);
		m_btnGraphLineStacked.EnableWindow(TRUE);
		m_btnGraphPie.EnableWindow(TRUE);
		m_GraphType = MyGraph::Line;
		m_bStacked = false;
		break;
	case PercentageOfAuthorship:
	case CommitsByAuthor:
		// by author
		m_btnGraphLine.EnableWindow(FALSE);
		m_btnGraphLineStacked.EnableWindow(FALSE);
		m_btnGraphPie.EnableWindow(TRUE);
		m_GraphType = MyGraph::Bar;
		m_bStacked = false;
		break;
	}
	RedrawGraph();
}

int CStatGraphDlg::GetUnit(const CTime& time)
{
	if (m_nDays < 8)
		return time.GetMonth()*100 + time.GetDay(); // month*100+day as the unit
	if (m_nWeeks < 15)
		return GetCalendarWeek(time);
	if (m_nWeeks < 80)
		return time.GetMonth();
	if (m_nWeeks < 320)
		return ((time.GetMonth()-1)/3)+1; // quarters
	return time.GetYear();
}

CStatGraphDlg::UnitType CStatGraphDlg::GetUnitType()
{
	if (m_nDays < 8)
		return Days;
	if (m_nWeeks < 15)
		return Weeks;
	if (m_nWeeks < 80)
		return Months;
	if (m_nWeeks < 320)
		return Quarters;
	return Years;
}

CString CStatGraphDlg::GetUnitString()
{
	if (m_nDays < 8)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXDAY));
	if (m_nWeeks < 15)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXWEEK));
	if (m_nWeeks < 80)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXMONTH));
	if (m_nWeeks < 320)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXQUARTER));
	return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXYEAR));
}

CString CStatGraphDlg::GetUnitLabel(int unit, CTime &lasttime)
{
	CString temp;
	switch (GetUnitType())
	{
	case Days:
		{
			// month*100+day as the unit
			int day = unit % 100;
			int month = unit / 100;
			switch (m_langOrder)
			{
			case 0: // month day year
				temp.Format(L"%d/%d/%.2d", month, day, lasttime.GetYear() % 100);
				break;
			case 1: // day month year
			default:
				temp.Format(L"%d/%d/%.2d", day, month, lasttime.GetYear() % 100);
				break;
			case 2: // year month day
				temp.Format(L"%.2d/%d/%d", lasttime.GetYear() % 100, month, day);
				break;
			}
		}
		break;
	case Weeks:
		{
			int year = lasttime.GetYear();
			if ((unit == 1)&&(lasttime.GetMonth() == 12))
				year += 1;

			switch (m_langOrder)
			{
			case 0: // month day year
			case 1: // day month year
			default:
				temp.Format(L"%d/%.2d", unit, year % 100);
				break;
			case 2: // year month day
				temp.Format(L"%.2d/%d", year % 100, unit);
				break;
			}
		}
		break;
	case Months:
		switch (m_langOrder)
		{
		case 0: // month day year
		case 1: // day month year
		default:
			temp.Format(L"%d/%.2d", unit, lasttime.GetYear() % 100);
			break;
		case 2: // year month day
			temp.Format(L"%.2d/%d", lasttime.GetYear() % 100, unit);
			break;
		}
		break;
	case Quarters:
		switch (m_langOrder)
		{
		case 0: // month day year
		case 1: // day month year
		default:
			temp.Format(L"%d/%.2d", unit, lasttime.GetYear() % 100);
			break;
		case 2: // year month day
			temp.Format(L"%.2d/%d", lasttime.GetYear() % 100, unit);
			break;
		}
		break;
	case Years:
		temp.Format(L"%d", unit);
		break;
	}
	return temp;
}

void CStatGraphDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (nSBCode == TB_THUMBTRACK)
		return CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	ShowSelectStat(static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel())));
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CStatGraphDlg::OnNeedText(NMHDR *pnmh, LRESULT * /*pResult*/)
{
	auto pttt = reinterpret_cast<TOOLTIPTEXT*>(pnmh);
	if (pttt->hdr.idFrom == reinterpret_cast<UINT_PTR>(m_Skipper.GetSafeHwnd()))
	{
		size_t included_authors_count = m_Skipper.GetPos();
		// if we only leave out one author, still include him with his name
		if (included_authors_count + 1 == m_authorNames.size())
			++included_authors_count;

		// find the minimum number of commits that the shown authors have
		int min_commits = 0;
		included_authors_count = min(included_authors_count, m_authorNames.size());
		std::list<tstring>::iterator author_it = m_authorNames.begin();
		advance(author_it, included_authors_count);
		if (author_it != m_authorNames.begin())
			min_commits = m_commitsPerAuthor[ *(--author_it) ];

		CString string;
		int percentage = int(min_commits*100.0/(m_nTotalCommits ? m_nTotalCommits : 1));
		string.FormatMessage(IDS_STATGRAPH_AUTHORSLIDER_TT, m_Skipper.GetPos(), min_commits, percentage);
		StringCchCopy(pttt->szText, _countof(pttt->szText), static_cast<LPCTSTR>(string));
	}
}

void CStatGraphDlg::AuthorsCaseSensitiveChanged()
{
	UpdateData();   // update checkbox state
	GatherData(FALSE, TRUE);   // first regenerate the statistics data
	RedrawGraph();  // then update the current statistics page
}

void CStatGraphDlg::SortModeChanged()
{
	UpdateData();   // update checkbox state
	RedrawGraph();  // then update the current statistics page
}

void CStatGraphDlg::OnBnClickedCommitternames()
{
	UpdateData();   // update checkbox state
	GatherData(FALSE, TRUE);   // first regenerate the statistics data
	RedrawGraph();  // then update the current statistics page
}

void CStatGraphDlg::OnBnClickedCommitdates()
{
	UpdateData();   // update checkbox state
	GatherData(FALSE, TRUE);   // first regenerate the statistics data
	RedrawGraph();  // then update the current statistics page
}

void CStatGraphDlg::ClearGraph()
{
	m_graph.Clear();
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete static_cast<MyGraphSeries*>(m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
}

void CStatGraphDlg::RedrawGraph()
{
	EnableDisableMenu();
	m_btnGraphBar.SetState(BST_UNCHECKED);
	m_btnGraphBarStacked.SetState(BST_UNCHECKED);
	m_btnGraphLine.SetState(BST_UNCHECKED);
	m_btnGraphLineStacked.SetState(BST_UNCHECKED);
	m_btnGraphPie.SetState(BST_UNCHECKED);

	if ((m_GraphType == MyGraph::Bar)&&(m_bStacked))
	{
		m_btnGraphBarStacked.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Bar)&&(!m_bStacked))
	{
		m_btnGraphBar.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Line)&&(m_bStacked))
	{
		m_btnGraphLineStacked.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Line)&&(!m_bStacked))
	{
		m_btnGraphLine.SetState(BST_CHECKED);
	}
	if (m_GraphType == MyGraph::PieChart)
	{
		m_btnGraphPie.SetState(BST_CHECKED);
	}

	UpdateData();
	ShowSelectStat(static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel())), true);
}
void CStatGraphDlg::OnBnClickedGraphbarbutton()
{
	m_GraphType = MyGraph::Bar;
	m_bStacked = false;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphbarstackedbutton()
{
	m_GraphType = MyGraph::Bar;
	m_bStacked = true;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinebutton()
{
	m_GraphType = MyGraph::Line;
	m_bStacked = false;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinestackedbutton()
{
	m_GraphType = MyGraph::Line;
	m_bStacked = true;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphpiebutton()
{
	m_GraphType = MyGraph::PieChart;
	m_bStacked = false;
	RedrawGraph();
}

void CStatGraphDlg::EnableDisableMenu()
{
	UINT nEnable = MF_BYCOMMAND;

	auto SelectMetric = static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));

	nEnable |= (SelectMetric > TextStatStart && SelectMetric < TextStatEnd)
		? (MF_DISABLED | MF_GRAYED) : MF_ENABLED;

	GetMenu()->EnableMenuItem(ID_FILE_SAVESTATGRAPHAS, nEnable);
}

void CStatGraphDlg::OnFileSavestatgraphas()
{
	CString tempfile;
	int filterindex = 0;
	if (CAppUtils::FileOpenSave(tempfile, &filterindex, IDS_REVGRAPH_SAVEPIC, IDS_STATPICFILEFILTER, false, m_hWnd))
	{
		// if the user doesn't specify a file extension, default to
		// wmf and add that extension to the filename. But only if the
		// user chose the 'pictures' filter. The filename isn't changed
		// if the 'All files' filter was chosen.
		CString extension;
		int dotPos = tempfile.ReverseFind('.');
		int slashPos = tempfile.ReverseFind('\\');
		if (dotPos > slashPos)
			extension = tempfile.Mid(dotPos);
		if ((filterindex == 1)&&(extension.IsEmpty()))
		{
			extension = L".wmf";
			tempfile += extension;
		}
		SaveGraph(tempfile);
	}
}

void CStatGraphDlg::SaveGraph(CString sFilename)
{
	CString extension = CPathUtils::GetFileExtFromPath(sFilename);
	if (extension.CompareNoCase(L".wmf") == 0)
	{
		// save the graph as an enhanced meta file
		CMyMetaFileDC wmfDC;
		wmfDC.CreateEnhanced(nullptr, sFilename, nullptr, L"TortoiseGit\0Statistics\0\0");
		wmfDC.SetAttribDC(GetDC()->GetSafeHdc());
		RedrawGraph();
		m_graph.DrawGraph(wmfDC);
		HENHMETAFILE hemf = wmfDC.CloseEnhanced();
		DeleteEnhMetaFile(hemf);
	}
	else
	{
		// save the graph as a pixel picture instead of a vector picture
		// create dc to paint on
		try
		{
			CWindowDC ddc(this);
			CDC dc;
			if (!dc.CreateCompatibleDC(&ddc))
			{
				ShowErrorMessage();
				return;
			}
			CRect rect;
			GetDlgItem(IDC_GRAPH)->GetClientRect(&rect);
			HBITMAP hbm = ::CreateCompatibleBitmap(ddc.m_hDC, rect.Width(), rect.Height());
			if (!hbm)
			{
				ShowErrorMessage();
				return;
			}
			auto oldbm = static_cast<HBITMAP>(dc.SelectObject(hbm));
			// paint the whole graph
			RedrawGraph();
			m_graph.DrawGraph(dc);
			// now use GDI+ to save the picture
			CLSID encoderClsid;
			GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			CString sErrormessage;
			if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Ok)
			{
				{
					Bitmap bitmap(hbm, nullptr);
					if (bitmap.GetLastStatus()==Ok)
					{
						// Get the CLSID of the encoder.
						int ret = 0;
						if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".png") == 0)
							ret = GetEncoderClsid(L"image/png", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".jpg") == 0)
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".jpeg") == 0)
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".bmp") == 0)
							ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".gif") == 0)
							ret = GetEncoderClsid(L"image/gif", &encoderClsid);
						else
						{
							sFilename += L".jpg";
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						}
						if (ret >= 0)
						{
							CStringW tfile = CStringW(sFilename);
							bitmap.Save(tfile, &encoderClsid, nullptr);
						}
						else
							sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, static_cast<LPCTSTR>(CPathUtils::GetFileExtFromPath(sFilename)));
					}
					else
						sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
				}
				GdiplusShutdown(gdiplusToken);
			}
			else
				sErrormessage.LoadString(IDS_REVGRAPH_ERR_GDIINIT);
			dc.SelectObject(oldbm);
			dc.DeleteDC();
			if (!sErrormessage.IsEmpty())
				::MessageBox(m_hWnd, sErrormessage, L"TortoiseGit", MB_ICONERROR);
		}
		catch (CException * pE)
		{
			TCHAR szErrorMsg[2048] = { 0 };
			pE->GetErrorMessage(szErrorMsg, 2048);
			pE->Delete();
			::MessageBox(m_hWnd, szErrorMsg, L"TortoiseGit", MB_ICONERROR);
		}
	}
}

int CStatGraphDlg::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if (size == 0)
		return -1;  // Failure

	auto pMem = std::make_unique<BYTE[]>(size);
	auto pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(pMem.get());
	if (!pImageCodecInfo)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				return j;  // Success
			}
		}
	}
	return -1;  // Failure
}

void CStatGraphDlg::StoreCurrentGraphType()
{
	UpdateData();
	DWORD graphtype = static_cast<DWORD>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));
	// encode the current chart type
	DWORD statspage = graphtype*10;
	if ((m_GraphType == MyGraph::Bar)&&(m_bStacked))
		statspage += 1;
	if ((m_GraphType == MyGraph::Bar)&&(!m_bStacked))
		statspage += 2;
	if ((m_GraphType == MyGraph::Line)&&(m_bStacked))
		statspage += 3;
	if ((m_GraphType == MyGraph::Line)&&(!m_bStacked))
		statspage += 4;
	if (m_GraphType == MyGraph::PieChart)
		statspage += 5;

	// store current chart type in registry
	CRegDWORD lastStatsPage(L"Software\\TortoiseGit\\LastViewedStatsPage", 0);
	lastStatsPage = statspage;

	CRegDWORD regAuthors(L"Software\\TortoiseGit\\StatAuthorsCaseSensitive");
	regAuthors = m_bAuthorsCaseSensitive;

	CRegDWORD regSort(L"Software\\TortoiseGit\\StatSortByCommitCount");
	regSort = m_bSortByCommitCount;

	CRegDWORD regCommitterName(L"Software\\TortoiseGit\\StatCommiterNames");
	regCommitterName = m_bUseCommitterNames;

	CRegDWORD regCommitDates(L"Software\\TortoiseGit\\StatCommitDates");
	regCommitDates = m_bUseCommitDates;
}

void CStatGraphDlg::ShowErrorMessage()
{
	CFormatMessageWrapper errorDetails;
	if (errorDetails)
		MessageBox(errorDetails, L"Error", MB_OK | MB_ICONINFORMATION);
}

void CStatGraphDlg::ShowSelectStat(Metrics SelectedMetric, bool reloadSkiper /* = false */)
{
	switch (SelectedMetric)
	{
		case AllStat:
			LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
			ShowStats();
			break;
		case CommitsByDate:
			LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
			ShowByDate(IDS_STATGRAPH_COMMITSBYDATEY, IDS_STATGRAPH_COMMITSBYDATE, m_commitsPerUnitAndAuthor);
			break;
		case LinesWByDate:
			OnBnClickedFetchDiff();
			LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
			ShowByDate(IDS_STATGRAPH_LINES_BYDATE_W_Y, IDS_STATGRAPH_LINES_BYDATE_W, m_LinesWPerUnitAndAuthor);
			break;
		case LinesWOByDate:
			OnBnClickedFetchDiff();
			LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
			ShowByDate(IDS_STATGRAPH_LINES_BYDATE_WO_Y, IDS_STATGRAPH_LINES_BYDATE_WO, m_LinesWOPerUnitAndAuthor);
			break;
		case CommitsByAuthor:
			LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
			ShowCommitsByAuthor();
			break;
		case PercentageOfAuthorship:
			OnBnClickedFetchDiff();
			LoadListOfAuthors(m_PercentageOfAuthorship, reloadSkiper, true);
			ShowPercentageOfAuthorship();
			break;
		default:
			ShowErrorMessage();
	}
}

double CStatGraphDlg::CoeffContribution(int distFromEnd) { return distFromEnd  ? 1.0 / m_CoeffAuthorShip * distFromEnd : 1;}


template <class MAP>
void CStatGraphDlg::DrawOthers(const std::list<tstring> &others, MyGraphSeries *graphData, MAP &map)
{
	int  nCommits = 0;
	for (std::list<tstring>::const_iterator it = others.begin(); it != others.end(); ++it)
		nCommits += RollPercentageOfAuthorship(map[*it]);

	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	sOthers.AppendFormat(L" (%Iu)", others.size());
	int group = m_graph.AppendGroup(sOthers);
	graphData->SetData(group, static_cast<int>(nCommits));
}


template <class MAP>
void CStatGraphDlg::LoadListOfAuthors (MAP &map, bool reloadSkiper/*= false*/,  bool compare /*= false*/)
{
	m_authorNames.clear();
	if (!map.empty())
	{
		for (MAP::const_iterator it = map.begin(); it != map.end(); ++it)
		{
			if ((compare && RollPercentageOfAuthorship(map[it->first]) != 0) || !compare)
				m_authorNames.push_back(it->first);
		}
	}

	// Sort the list of authors based on commit count
	m_authorNames.sort(MoreCommitsThan<MAP::mapped_type>(map));

	// Set Skipper
	SetSkipper(reloadSkiper);
}


void CStatGraphDlg::OnBnClickedFetchDiff()
{
	if (m_bDiffFetched)
		return;
	if (GatherData(TRUE))
		return;
	this->m_bDiffFetched = TRUE;
	GetDlgItem(IDC_CALC_DIFF)->ShowWindow(!m_bDiffFetched);

	ShowStats();
}
